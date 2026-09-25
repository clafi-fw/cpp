export module ClaFi.Core.DomEngine_Document;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Dom
{
    export class FileFormatBase
    {
    public:
        virtual ~FileFormatBase() = default;
        virtual void writeHeader(std::wostream&) const {}
        virtual void readHeader(std::wistream&) const {}
        virtual void loadSectionFromStream(Section&, std::wistream&) const = 0;
        virtual void saveSectionToStream(const Section&, std::wostream&) const = 0;
        void saveSectionToFile(const Section&, const std::filesystem::path&) const;
        // Answers whether there was a file to read.
        bool loadSectionFromFile(Section&, const std::filesystem::path&) const;
    };

    export template <typename Format>
        concept IsFormatType = requires()
    {
        std::is_base_of<FileFormatBase, Format>();
    };


    // Whether the document writes itself back as it changes.
    export enum class AutoSave
    {
        Yes,
        No
    };

    // Whether a saved document repeats what its layout already says. See Dom
    export enum class WriteDefaults
    {
        Yes,
        No
    };

    export class DocumentBase;

    // The document has been written to its file; what keeps files beside it writes them here.
    export class SaveEvent : public EventOf<DocumentBase>
    {
    public:
        explicit SaveEvent(DocumentBase& sender);
    };

    export class DocumentBase : public Section
    {
    public:
        explicit DocumentBase(const std::filesystem::path& path, AutoSave autosave = AutoSave::No,
            const Dt::Section& layout = {}, WriteDefaults = WriteDefaults::Yes);
        [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
        // Reads the file, answering whether there was one.
        virtual bool load() = 0;
        virtual void save() = 0;
    protected:
        virtual void autoSave() = 0;
        // What the layout alone builds. Null unless the document states only what it changes.
        [[nodiscard]] const Section* defaults() const;
    private:
        void bindAutoSave();
    private:
        const std::filesystem::path m_path;
        AutoSave m_autoSave;
        // A node's default is the value it was built with, and the first set overwrites it, so
        // this is taken while the tree still holds what the layout put there.
        std::unique_ptr<DomNodeBase> m_defaults;
    };

    export template <IsFormatType Format>
        class Document : public DocumentBase
    {
    public:
        using DocumentBase::DocumentBase;
        [[nodiscard]] Format& format();
        bool load() override;
        void save() override;
    protected:
        void autoSave() override;
    private:
        Format m_format{};
        bool m_loading{};
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    // --- DocumentBase ---

    void FileFormatBase::saveSectionToFile(const Section& section, const std::filesystem::path& filePath) const
    {
        auto directory = filePath.parent_path();
        if (!std::filesystem::exists(directory))
        {
            std::filesystem::create_directories(directory);
        }
        // The document is built as text and encoded once, at the edge. A wide file stream would
        // narrow through the stream's locale instead, and the default locale answers the first
        // character above 0x7F by failing - which stops the write and leaves the file short of
        // the theme it was asked to save.
        std::wstringstream text;
        writeHeader(text);
        saveSectionToStream(section, text);

        // Text mode, so a line ends the way the platform ends one. No byte of a multi-byte
        // UTF-8 sequence is 0x0A, so that translation cannot reach inside a character.
        const std::string utf8 = toUtf8(text.str());
        std::ofstream file{ filePath };
        file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    }

    bool FileFormatBase::loadSectionFromFile(Section& section, const std::filesystem::path& path) const
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }
        std::ifstream file{ path };
        std::string utf8{ std::istreambuf_iterator<char>{ file },
            std::istreambuf_iterator<char>{} };

        // A file written by another editor may carry a byte order mark. It states the encoding
        // this reader already knows, so it is dropped rather than read as a character - it
        // would otherwise arrive as a zero-width space at the head of the first key.
        constexpr std::string_view k_utf8Bom = "\xEF\xBB\xBF";
        std::string_view body = utf8;
        if (body.starts_with(k_utf8Bom))
        {
            body.remove_prefix(k_utf8Bom.size());
        }

        std::wstringstream text{ fromUtf8(body) };
        readHeader(text);
        {
            auto transaction = section.startTransaction();
            loadSectionFromStream(section, text);
        }
        return true;
    }

    // --- SaveEvent ---

    SaveEvent::SaveEvent(DocumentBase& sender)
        :
        EventOf<DocumentBase>{ sender }
    {
    }

    DocumentBase::DocumentBase(const std::filesystem::path& path, AutoSave autosave,
        const Dt::Section& layout, WriteDefaults writeDefaults)
        :
        Section{ nullptr },
        m_path{ path },
        m_autoSave{ autosave }
    {
        layout.apply(*this);
        // Here, before anything has been loaded or set, the tree holds the layout and nothing
        // else. DomSection::clone builds fresh nodes, so the copy carries no handlers and stays
        // inert for the document's lifetime.
        if (writeDefaults == WriteDefaults::No)
        {
            m_defaults = clone(nullptr);
        }
        bindAutoSave();
    }

    const Section* DocumentBase::defaults() const
    {
        // DomSection::clone returns a DomSection whatever it was called on.
        return static_cast<const Section*>(m_defaults.get());
    }

    void DocumentBase::bindAutoSave()
    {
        if (m_autoSave == AutoSave::No)
        {
            return;
        }

        connectEvent<NestedChangeEvent>([this](NestedChangeEvent&)
            {
                autoSave();
            });
    }

    // --- Document<Format> ---

    template <IsFormatType Format>
    Format& Document<Format>::format()
    {
        return m_format;
    }

    template <IsFormatType Format>
    bool Document<Format>::load()
    {
        ScopedPushPop _{ m_loading, true };
        return m_format.loadSectionFromFile(*this, path());
    }

    template <IsFormatType Format>
    void Document<Format>::save()
    {
        const Section* layoutDefaults = defaults();
        if (layoutDefaults)
        {
            m_format.saveSectionToFile(*withoutDefaults(*this, *layoutDefaults), path());
        }
        else
        {
            m_format.saveSectionToFile(*this, path());
        }
        SaveEvent event = SaveEvent{ *this };
        emitEvent(event);
    }

    template <IsFormatType Format>
    void Document<Format>::autoSave()
    {
        if (!m_loading)
        {
            save();
        }
    }
}
