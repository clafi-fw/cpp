module ThisApp.Language;

import ThisApp.Consts;

import ClaFi.Documents.Folder;
import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer: without it a node's get reads a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;

    namespace
    {
        namespace Keys
        {
            constexpr std::wstring_view extension = L"Extension";
            constexpr std::wstring_view information = L"Information";
            constexpr std::wstring_view templates = L"Templates";
            constexpr std::wstring_view completion = L"Completion";
            constexpr std::wstring_view name = L"Name";
            constexpr std::wstring_view newStem = L"NewStem";
            constexpr std::wstring_view text = L"Text";
            constexpr std::wstring_view kind = L"Kind";
            constexpr std::wstring_view signature = L"Signature";
            constexpr std::wstring_view hint = L"Hint";
            constexpr std::wstring_view type = L"Type";
            constexpr std::wstring_view parent = L"Parent";
            constexpr std::wstring_view methods = L"Methods";
            constexpr std::wstring_view properties = L"Properties";
        }

        using Sections = Dom::Sequence<Dom::Section>;

        [[nodiscard]] std::wstring stringOf(const Dom::DomNodeBase& item,
            const std::wstring_view key)
        {
            return (item / key).get<std::wstring>();
        }

        // What every completion entry states, at either level.
        [[nodiscard]] Dom::Dt::Section entryLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::name, std::wstring{} },
                Value{ Keys::kind, std::wstring{} },
                Value{ Keys::signature, std::wstring{} },
                Value{ Keys::hint, std::wstring{} },
                Value{ Keys::type, std::wstring{} }
            };
        }

        // Reads the entries under a list, leaving out those with no name - nothing to match on,
        // nothing to list. The top level reads a class's parent, methods and properties as well;
        // a member states none of them, and the layout has no such keys to ask for there. An
        // entry stating no kind, or one the framework does not know, is of the list's kind.
        void readEntries(const Dom::DomNodeBase& list, const bool withMembers,
            const Syntax::CompletionKind listKind, Syntax::CompletionEntries& into)
        {
            for (const Dom::Section& item : list.as<Sections>())
            {
                Syntax::CompletionEntry entry = {
                    .name = stringOf(item, Keys::name),
                    .kind = Syntax::completionKindOf(stringOf(item, Keys::kind)).value_or(listKind),
                    .signature = stringOf(item, Keys::signature),
                    .hint = stringOf(item, Keys::hint),
                    .type = stringOf(item, Keys::type)
                };
                if (entry.name.empty())
                    continue;
                if (withMembers)
                {
                    entry.parent = stringOf(item, Keys::parent);
                    readEntries(item / Keys::methods, false, Syntax::CompletionKind::Procedure,
                        entry.methods);
                    readEntries(item / Keys::properties, false, Syntax::CompletionKind::Property,
                        entry.properties);
                }
                into.push_back(std::move(entry));
            }
        }
    }

    Language::Language(const std::filesystem::path& configFolder)
        :
        m_file{ findFile(configFolder) },
        m_extension{ k_scriptKind.extension },
        m_kind{ k_scriptKind }
    {
        if (!m_file.empty())
            read();
    }

    // The config folder's copy stands over the one shipped beside the executable, whole.
    std::filesystem::path Language::findFile(const std::filesystem::path& configFolder)
    {
        std::error_code error;
        if (!configFolder.empty())
        {
            const std::filesystem::path own = configFolder / k_fileName;
            if (std::filesystem::exists(own, error))
                return own;
        }
        const std::wstring executableDirectory = Platform::executableDirectory();
        if (executableDirectory.empty())
            return {};
        const std::filesystem::path shipped =
            std::filesystem::path{ executableDirectory } / k_fileName;
        if (std::filesystem::exists(shipped, error))
            return shipped;
        return {};
    }

    void Language::read()
    {
        using namespace Dom::Dt;
        const Section layout = {
            Value{ Keys::extension, std::wstring{} },
            Value{ Keys::information, std::wstring{} },
            Sequence
            {
                Keys::templates,
                Section
                {
                    Value{ Keys::name, std::wstring{} },
                    Value{ Keys::newStem, std::wstring{} },
                    Value{ Keys::text, std::wstring{} }
                }
            },
            Sequence
            {
                Keys::completion,
                Section
                {
                    entryLayout(),
                    Value{ Keys::parent, std::wstring{} },
                    Sequence{ Keys::methods, entryLayout() },
                    Sequence{ Keys::properties, entryLayout() }
                }
            }
        };
        Dom::Document<Dom::FileFormat::ClaFi> document{
            m_file,
            Dom::AutoSave::No,
            layout,
            Dom::WriteDefaults::No
        };
        if (!document.load())
            return;

        // A stated extension replaces the compiled-in one; a file saying nothing keeps it.
        const std::wstring extension = stringOf(document, Keys::extension);
        if (!extension.empty())
        {
            m_extension = extension;
            m_kind.extension = m_extension;
        }
        m_information = stringOf(document, Keys::information);

        // A template with no name could not be listed, so it is left out.
        for (const Dom::Section& item : (document / Keys::templates).as<Sections>())
        {
            std::wstring name = stringOf(item, Keys::name);
            if (name.empty())
                continue;
            m_templates.push_back({
                .name = std::move(name),
                .newStem = stringOf(item, Keys::newStem),
                .text = stringOf(item, Keys::text)
            });
        }
        readEntries(document / Keys::completion, true, Syntax::CompletionKind::Variable,
            m_completion);
    }
}
