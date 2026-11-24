module ClaFi.Documents.Folder;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.DomEngine;
import ClaFi.Core.System.DirWatch;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    namespace
    {
        // A name split into its words and the number it ends in, so that (2) sorts before (10).
        struct SortKey
        {
            std::wstring_view words;
            long number;
        };

        [[nodiscard]] SortKey sortKeyOf(const std::wstring_view stem)
        {
            std::size_t i = stem.size();
            while (i != 0ull && (std::iswdigit(stem[i - 1ull]) || stem[i - 1ull] == L')'))
                --i;
            if (i == stem.size())
                return { .words = stem, .number = 0l };

            wchar_t* end{};
            return {
                .words = stem.substr(0ull, i),
                .number = std::wcstol(stem.data() + i, &end, 10)
            };
        }

        [[nodiscard]] std::filesystem::path resolvedDirectory(const std::filesystem::path& path)
        {
            if (path.empty())
            {
                return {};
            }
            std::error_code error;
            const std::filesystem::path absolute = std::filesystem::absolute(path, error);
            if (error)
            {
                return {};
            }
            std::filesystem::path resolved = std::filesystem::weakly_canonical(absolute, error);
            if (error)
            {
                return {};
            }
            if (resolved != resolved.root_path() && !resolved.has_filename())
            {
                resolved = resolved.parent_path();
            }
            return resolved;
        }
    }

    DocumentsFolder::DocumentsFolder(const std::filesystem::path& directory,
        const DocumentKind& kind)
        :
        m_directory{ directory },
        m_resolvedDirectory{ resolvedDirectory(directory) },
        m_kind{ kind }
    {
        watchDirectory();
    }

    void DocumentsFolder::setReadOnly(const bool readOnly)
    {
        m_readOnly = readOnly;
    }

    void DocumentsFolder::detach()
    {
        m_available = false;
        m_dirWatchConnection = {};
        m_dirWatcher.reset();
        m_loaded = false;
        notifyListeners();
    }

    const FilePaths& DocumentsFolder::files()
    {
        update();
        return m_files;
    }

    std::filesystem::path DocumentsFolder::fileOf(const std::wstring_view fileName) const
    {
        const std::filesystem::path name{ fileName };
        if (m_directory.empty() || name.empty() || name.has_root_path()
            || name != name.filename() || name == L"." || name == L".."
            || fileName.find(L'\0') != std::wstring_view::npos)
        {
            return {};
        }
        if constexpr (std::filesystem::path::preferred_separator == L'\\')
        {
            if (fileName.find(L':') != std::wstring_view::npos)
            {
                return {};
            }
        }
        return m_directory / name;
    }

    std::wstring DocumentsFolder::fileNameOf(const std::wstring_view stem) const
    {
        return std::wstring{ stem }.append(m_kind.extension);
    }

    bool DocumentsFolder::canWrite() const
    {
        return m_available && !m_readOnly && !m_directory.empty();
    }

    bool DocumentsFolder::canWriteFile(const std::filesystem::path& path) const
    {
        if (!canWrite() || path.empty() || path != fileOf(path.filename().wstring())
            || !directoryUnchanged())
        {
            return false;
        }
        std::error_code error;
        const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);
        return status.type() == std::filesystem::file_type::not_found
            || (!error && std::filesystem::is_regular_file(status));
    }

    bool DocumentsFolder::saveDocument(const Dom::DomNodeBase& document,
        const std::filesystem::path& path)
    {
        if (!canWriteFile(path))
        {
            return false;
        }
        needDirectory();
        std::error_code error;
        return std::filesystem::is_directory(m_directory, error)
            && canWriteFile(path) && writeDocument(document, path);
    }

    bool DocumentsFolder::deleteFile(const std::filesystem::path& path) const
    {
        if (!canWriteFile(path))
        {
            return false;
        }
        std::error_code error;
        return std::filesystem::remove(path, error);
    }

    bool DocumentsFolder::needDirectory()
    {
        std::error_code errorCode;
        if (!canWrite() || !directoryUnchanged() || std::filesystem::exists(m_directory, errorCode))
            return false;
        if (!std::filesystem::create_directories(m_directory, errorCode))
            return false;
        watchDirectory();
        return true;
    }

    std::wstring DocumentsFolder::createFile(const DocumentTemplate* documentTemplate)
    {
        if (!canWrite())
        {
            return {};
        }
        needDirectory();

        const std::wstring_view stem = documentTemplate && !documentTemplate->newStem.empty()
            ? std::wstring_view{ documentTemplate->newStem }
            : m_kind.newStem;
        int counter = 0;
        std::filesystem::path file{};
        std::error_code errorCode;
        do
        {
            ++counter;
            file = newFile(stem, counter);
        }
        while (std::filesystem::exists(std::filesystem::symlink_status(file, errorCode)));

        if (!canWriteFile(file))
        {
            return {};
        }

        if (documentTemplate && documentTemplate->document)
        {
            if (!saveDocument(*documentTemplate->document, file))
            {
                return {};
            }
        }
        else
        {
            const std::ofstream stream{ file, std::ios::binary | std::ios::trunc };
            if (!stream)
                return {};
        }
        return file.filename().wstring();
    }

    std::wstring DocumentsFolder::renameFile(const std::filesystem::path& oldPath,
        AcceptEditEvent& event) const
    {
        if (!canWriteFile(oldPath))
        {
            event.refuse(L"This file is read-only.");
            return {};
        }
        const std::wstring newStem{ event.text.plainText() };
        checkNameShape(event, newStem);
        if (event.refused())
            return {};
        // Nothing to do is not a refusal - the user looked at the name and left it alone.
        if (newStem == oldPath.stem().wstring())
            return {};

        const std::wstring newName = fileNameOf(newStem);
        std::filesystem::path newPath = oldPath;
        newPath.replace_filename(newName);
        if (!canWriteFile(newPath))
        {
            event.refuse(L"This name cannot be used.");
            return {};
        }

        std::error_code errorCode;
        // The name that is already taken may be this file's own: the stem comparison above is
        // case sensitive and the file system is not, so `unit1` renamed to `Unit1` reaches here
        // and finds itself. equivalent() asks whether the two paths name one file, which lets a
        // change of capitalization through.
        if (std::filesystem::exists(newPath, errorCode)
            && !std::filesystem::equivalent(oldPath, newPath, errorCode))
        {
            event.refuse(std::wstring{ L"Another " }.append(m_kind.noun)
                .append(L" already has that name."));
            return {};
        }

        errorCode.clear();
        std::filesystem::rename(oldPath, newPath, errorCode);
        if (errorCode)
        {
            event.refuse(L"The file could not be renamed.");
            return {};
        }

        return newName;
    }

    void DocumentsFolder::checkNameShape(AcceptEditEvent& event, const std::wstring_view stem) const
    {
        if (stem.empty())
        {
            event.refuse(std::wstring{ L"The " }.append(m_kind.noun).append(L" needs a name."));
            return;
        }
        if (stem.find_first_of(L"\\/:*?\"<>|") != std::wstring_view::npos)
            event.refuse(L"A name cannot contain any of \\ / : * ? \" < > |");
    }

    bool DocumentsFolder::matchesTemplate(const std::filesystem::path& path) const
    {
        for (const DocumentTemplate& documentTemplate : m_templates)
        {
            if (!documentTemplate.document)
                continue;
            // A node of the template's own type, so the read fills what the comparison expects.
            const std::unique_ptr<Dom::DomNodeBase> read =
                documentTemplate.document->clone(nullptr);
            if (readDocument(path, *read) && Dom::sameValue(*read, *documentTemplate.document))
                return true;
        }
        return false;
    }

    void DocumentsFolder::update()
    {
        watchDirectory();
        if (m_loaded)
            return;
        m_loaded = true;
        m_files.clear();

        std::error_code errorCode;
        if (m_available && std::filesystem::is_directory(m_directory, errorCode))
        {
            const std::filesystem::directory_iterator end{};
            for (std::filesystem::directory_iterator item{ m_directory, errorCode };
                item != end && !errorCode; item.increment(errorCode))
            {
                std::error_code statusError;
                if (item->is_regular_file(statusError) && item->path().extension() == m_kind.extension)
                {
                    m_files.push_back(item->path());
                }
            }

            using Path = std::filesystem::path;
            std::ranges::sort(m_files, [](const Path& alpha, const Path& beta) {
                const std::wstring stemA = alpha.stem().wstring();
                const std::wstring stemB = beta.stem().wstring();
                const SortKey keyA = sortKeyOf(stemA);
                const SortKey keyB = sortKeyOf(stemB);
                if (keyA.words == keyB.words)
                    return keyA.number < keyB.number;
                return keyA.words < keyB.words;
                });
        }
        // A directory that is not there reads as no files, and that is told too.
        filesRead(m_files);
    }

    bool DocumentsFolder::directoryUnchanged() const
    {
        return !m_resolvedDirectory.empty() && resolvedDirectory(m_directory) == m_resolvedDirectory;
    }

    std::filesystem::path DocumentsFolder::newFile(const std::wstring_view stem,
                                                   const int counter) const
    {
        std::wstring name{ stem };
        if (counter > 1)
            name.append(L" (").append(std::to_wstring(counter)).append(L")");
        name.append(m_kind.extension);
        return m_directory / name;
    }

    void DocumentsFolder::watchDirectory()
    {
        if (!m_available || (m_dirWatcher && m_dirWatcher->watching()))
        {
            return;
        }
        std::error_code errorCode;
        if (!std::filesystem::is_directory(m_directory, errorCode))
        {
            return;
        }
        if (m_dirWatcher)
        {
            m_dirWatcher->restart();
        }
        else
        {
            m_dirWatcher.emplace(m_directory);
            m_dirWatchConnection = m_dirWatcher->onChange([this](DirWatchChangeEvent&) {
                m_loaded = false;
                notifyListeners();
            });
        }
        // Whatever the list holds was read while nothing was watching, so it is read again.
        m_loaded = false;
    }

    void DocumentsFolder::notifyListeners() const
    {
        for (IListener* listener : m_listeners)
            listener->documentsFolderChanged();
    }
}
