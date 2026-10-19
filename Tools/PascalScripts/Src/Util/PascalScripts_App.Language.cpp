module PascalScripts_App.Language;

import PascalScripts_App.Consts;

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

namespace PascalScripts_App
{
    using namespace ::ClaFi;

    namespace
    {
        namespace Keys
        {
            constexpr std::wstring_view Extension = L"Extension";
            constexpr std::wstring_view Information = L"Information";
            constexpr std::wstring_view Templates = L"Templates";
            constexpr std::wstring_view Completion = L"Completion";
            constexpr std::wstring_view Name = L"Name";
            constexpr std::wstring_view NewStem = L"NewStem";
            constexpr std::wstring_view Text = L"Text";
            constexpr std::wstring_view Kind = L"Kind";
            constexpr std::wstring_view Signature = L"Signature";
            constexpr std::wstring_view Hint = L"Hint";
            constexpr std::wstring_view Type = L"Type";
            constexpr std::wstring_view Parent = L"Parent";
            constexpr std::wstring_view Methods = L"Methods";
            constexpr std::wstring_view Properties = L"Properties";
            constexpr std::wstring_view TitlePrefix = L"TitlePrefix";
            constexpr std::wstring_view TitleKeys = L"TitleKeys";
            constexpr std::wstring_view List = L"List";
            constexpr std::wstring_view Values = L"Values";
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
                Value{ Keys::Name, std::wstring{} },
                Value{ Keys::Kind, std::wstring{} },
                Value{ Keys::Signature, std::wstring{} },
                Value{ Keys::Hint, std::wstring{} },
                Value{ Keys::Type, std::wstring{} }
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
                    .name = stringOf(item, Keys::Name),
                    .kind = Syntax::completionKindOf(stringOf(item, Keys::Kind)).value_or(listKind),
                    .signature = stringOf(item, Keys::Signature),
                    .hint = stringOf(item, Keys::Hint),
                    .type = stringOf(item, Keys::Type)
                };
                if (entry.name.empty())
                    continue;
                if (withMembers)
                {
                    entry.parent = stringOf(item, Keys::Parent);
                    readEntries(item / Keys::Methods, false, Syntax::CompletionKind::Procedure,
                        entry.methods);
                    readEntries(item / Keys::Properties, false, Syntax::CompletionKind::Property,
                        entry.properties);
                }
                into.push_back(std::move(entry));
            }
        }

        // Reads the title block's keys and each key's values, leaving out those with no name.
        void readTitleKeys(const Dom::DomNodeBase& list, Syntax::TitleKeys& into)
        {
            for (const Dom::Section& item : list.as<Sections>())
            {
                Syntax::TitleKey key = {
                    .entry = {
                        .name = stringOf(item, Keys::Name),
                        .kind = Syntax::CompletionKind::Key,
                        .signature = stringOf(item, Keys::Signature),
                        .hint = stringOf(item, Keys::Hint)
                    },
                    .list = (item / Keys::List).get<bool>()
                };
                if (key.entry.name.empty())
                    continue;
                for (const Dom::Section& value : (item / Keys::Values).as<Sections>())
                {
                    Syntax::CompletionEntry entry = {
                        .name = stringOf(value, Keys::Name),
                        .kind = Syntax::CompletionKind::Value,
                        .hint = stringOf(value, Keys::Hint)
                    };
                    if (!entry.name.empty())
                        key.values.push_back(std::move(entry));
                }
                into.push_back(std::move(key));
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
            Value{ Keys::Extension, std::wstring{} },
            Value{ Keys::Information, std::wstring{} },
            Sequence
            {
                Keys::Templates,
                Section
                {
                    Value{ Keys::Name, std::wstring{} },
                    Value{ Keys::NewStem, std::wstring{} },
                    Value{ Keys::Text, std::wstring{} }
                }
            },
            Sequence
            {
                Keys::Completion,
                Section
                {
                    entryLayout(),
                    Value{ Keys::Parent, std::wstring{} },
                    Sequence{ Keys::Methods, entryLayout() },
                    Sequence{ Keys::Properties, entryLayout() }
                }
            },
            Value{ Keys::TitlePrefix, std::wstring{} },
            Sequence
            {
                Keys::TitleKeys,
                Section
                {
                    Value{ Keys::Name, std::wstring{} },
                    Value{ Keys::Signature, std::wstring{} },
                    Value{ Keys::Hint, std::wstring{} },
                    Value{ Keys::List, false },
                    Sequence
                    {
                        Keys::Values,
                        Section
                        {
                            Value{ Keys::Name, std::wstring{} },
                            Value{ Keys::Hint, std::wstring{} }
                        }
                    }
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
        const std::wstring extension = stringOf(document, Keys::Extension);
        if (!extension.empty())
        {
            m_extension = extension;
            m_kind.extension = m_extension;
        }
        m_information = stringOf(document, Keys::Information);

        // A template with no name could not be listed, so it is left out.
        for (const Dom::Section& item : (document / Keys::Templates).as<Sections>())
        {
            std::wstring name = stringOf(item, Keys::Name);
            if (name.empty())
                continue;
            m_templates.push_back({
                .name = std::move(name),
                .newStem = stringOf(item, Keys::NewStem),
                .text = stringOf(item, Keys::Text)
            });
        }
        readEntries(document / Keys::Completion, true, Syntax::CompletionKind::Variable,
            m_completion);
        m_titleBlock.prefix = stringOf(document, Keys::TitlePrefix);
        readTitleKeys(document / Keys::TitleKeys, m_titleBlock.keys);
    }
}
