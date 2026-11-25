module PascalScripts_App.Scripts;

import PascalScripts_App.AppIcon;
import PascalScripts_App.Language;

import ClaFi.Documents.Folder;
import ClaFi.Documents.TextFile;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.DomEngine;
// The std::wstring serializer: without it the node's set and get read a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace PascalScripts_App
{
    using namespace ::ClaFi;

    namespace
    {
#ifdef _WIN32
        constexpr auto k_legacyMacroEncoding = Documents::TextFileFallback::WindowsAnsi;
#else
        // GrandSmeta ships legacy Cyrillic macros alongside its UTF-8 files.
        constexpr auto k_legacyMacroEncoding = Documents::TextFileFallback::Windows1251;
#endif

        [[nodiscard]] bool isGrandSmetaMacro(const std::filesystem::path& path)
        {
            std::wstring extension = path.extension().wstring();
            foldCase(extension);
            return extension == L".gsps";
        }
    }

    ScriptsFolder::ScriptsFolder(const std::filesystem::path& directory, const Language& language)
        :
        DocumentsFolder{ directory, language.kind() },
        m_language{ language }
    {
        using TextNode = Dom::Value<std::wstring>;
        for (const ScriptTemplate& scriptTemplate : language.templates())
        {
            templates().push_back({
                .name = scriptTemplate.name,
                .newStem = scriptTemplate.newStem,
                .document = std::make_unique<TextNode>(nullptr, scriptTemplate.text)
            });
        }
    }

    bool ScriptsFolder::readDocument(const std::filesystem::path& path,
        Dom::DomNodeBase& into) const
    {
        const Documents::TextFileFallback fallback = isGrandSmetaMacro(path)
            ? k_legacyMacroEncoding : Documents::TextFileFallback::None;
        std::optional<std::wstring> text = Documents::readTextFile(path, fallback);
        const bool read = text.has_value();
        // A file that could not be read leaves the empty script, as the contract says.
        into.set(std::move(text).value_or(std::wstring{}));
        return read;
    }

    bool ScriptsFolder::writeDocument(const Dom::DomNodeBase& node,
        const std::filesystem::path& path) const
    {
        const Documents::Utf8Bom bom = isGrandSmetaMacro(path)
            ? Documents::Utf8Bom::Write : Documents::Utf8Bom::Omit;
        return Documents::writeTextFile(path, node.get<std::wstring>(), bom);
    }

    // A script holds work when it holds anything a template did not put there.
    bool ScriptsFolder::isEdited(const std::filesystem::path& path) const
    {
        std::error_code errorCode;
        const std::uintmax_t size = std::filesystem::file_size(path, errorCode);
        if (errorCode || size == 0ull)
            return false;
        return !matchesTemplate(path);
    }

    // One mark for every script, whatever it holds.
    void ScriptsFolder::drawIcon(std::wstring_view, DrawIconEvent& event)
    {
        AppIcon::drawScriptIcon(event);
    }
}
