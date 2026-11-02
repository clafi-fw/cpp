module SeeDocs_App.Articles;

import SeeDocs_App.Project;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.Dom_TextSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer: without it a node's set reads a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace SeeDocs_App::Articles
{
    using namespace ClaFi;

    namespace
    {
        namespace Keys
        {
            constexpr std::wstring_view Title = L"Title";
            constexpr std::wstring_view Lead = L"Lead";
            constexpr std::wstring_view Blocks = L"Blocks";
            constexpr std::wstring_view Kind = L"Kind";
            constexpr std::wstring_view Text = L"Text";
            constexpr std::wstring_view Level = L"Level";
            constexpr std::wstring_view Anchor = L"Anchor";
            constexpr std::wstring_view List = L"List";
            constexpr std::wstring_view Language = L"Language";
            constexpr std::wstring_view Code = L"Code";
            constexpr std::wstring_view Header = L"Header";
            constexpr std::wstring_view Rows = L"Rows";
            constexpr std::wstring_view Cells = L"Cells";
            constexpr std::wstring_view Group = L"Group";
            constexpr std::wstring_view File = L"File";
            constexpr std::wstring_view Description = L"Description";
            constexpr std::wstring_view Do = L"Do";
            constexpr std::wstring_view Dont = L"Dont";
            constexpr std::wstring_view Callout = L"Callout";
            constexpr std::wstring_view Summary = L"Summary";
        }

        // The words a block's Kind key takes.
        namespace Kinds
        {
            constexpr std::wstring_view Paragraph = L"Paragraph";
            constexpr std::wstring_view Heading = L"Heading";
            constexpr std::wstring_view ListItem = L"ListItem";
            constexpr std::wstring_view Code = L"Code";
            constexpr std::wstring_view Table = L"Table";
            constexpr std::wstring_view Figure = L"Figure";
            constexpr std::wstring_view DoDont = L"DoDont";
            constexpr std::wstring_view Callout = L"Callout";
            constexpr std::wstring_view Fold = L"Fold";
        }

        using ArticleDocument = Dom::Document<Dom::FileFormat::ClaFi>;
        using Sections = Dom::Sequence<Dom::Section>;
        using SectionPtr = std::unique_ptr<Dom::Section>;
        using SectionPtrs = std::vector<SectionPtr>;
        using StringNode = Dom::Value<std::wstring>;
        using TextValueNode = Dom::Value<Text>;

        // The keys of every block a callout or a fold may hold, at its block's defaults.
        [[nodiscard]] Dom::Dt::Section leafLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::Kind, std::wstring{} },
                Value{ Keys::Text, Text{} },
                Value{ Keys::Level, Heading{}.level },
                Value{ Keys::Anchor, std::wstring{} },
                Value{ Keys::List, ListItem{}.kind },
                Value{ Keys::Language, std::wstring{} },
                Value{ Keys::Code, std::wstring{} },
                Sequence{ Keys::Header, Text{} },
                Sequence{
                    Keys::Rows,
                    Section{
                        Sequence{ Keys::Cells, Text{} },
                        Value{ Keys::Group, TableRow{}.isGroup }
                    }
                },
                Value{ Keys::File, std::wstring{} },
                Value{ Keys::Description, std::wstring{} },
                Sequence{ Keys::Do, Text{} },
                Sequence{ Keys::Dont, Text{} }
            };
        }

        // The keys of every block, a container's included.
        [[nodiscard]] Dom::Dt::Section blockLayout()
        {
            using namespace Dom::Dt;
            return {
                leafLayout(),
                Value{ Keys::Callout, Callout{}.kind },
                Value{ Keys::Summary, Text{} },
                Sequence{ Keys::Blocks, leafLayout() }
            };
        }

        [[nodiscard]] Dom::Dt::Section articleLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::Title, std::wstring{} },
                Value{ Keys::Lead, Text{} },
                Sequence{ Keys::Blocks, blockLayout() }
            };
        }

        // --- Writing ---

        // **An entry states only what it has**: a key at its layout's default is left out, and
        // the layout puts it back on reading.
        void put(Dom::Section& entry, const std::wstring_view key,
            std::unique_ptr<Dom::DomNodeBase> node)
        {
            node->setParent(&entry);
            entry.setChild(key, std::move(node));
        }

        void putWords(Dom::Section& entry, const std::wstring_view key,
            const std::wstring_view words)
        {
            if (!words.empty())
                put(entry, key, std::make_unique<StringNode>(nullptr, std::wstring{ words }));
        }

        template <typename T>
        void putValue(Dom::Section& entry, const std::wstring_view key, const T& value,
            const T& layoutValue)
        {
            if (value != layoutValue)
                put(entry, key, std::make_unique<Dom::Value<T>>(nullptr, value));
        }

        // A text as its composite writes it, less the markers where it holds none.
        [[nodiscard]] SectionPtr textEntry(const Text& text)
        {
            const TextValueNode stated{ nullptr, text };
            const TextValueNode blank{ nullptr, Text{} };
            return Dom::withoutDefaults(stated, blank);
        }

        void putText(Dom::Section& entry, const std::wstring_view key, const Text& text)
        {
            if (!text.plainText().empty() || !text.markers().empty())
                put(entry, key, textEntry(text));
        }

        void putEntries(Dom::Section& entry, const std::wstring_view key, SectionPtrs items)
        {
            if (items.empty())
                return;
            std::unique_ptr<Sections> list = std::make_unique<Sections>(nullptr,
                Dom::Section{ nullptr });
            for (SectionPtr& item : items)
            {
                item->setParent(list.get());
                list->children().push_back(std::move(item));
            }
            put(entry, key, std::move(list));
        }

        // Every text of the list goes in, a blank one too, so that the rest keep their places.
        void putTexts(Dom::Section& entry, const std::wstring_view key, const Texts& texts)
        {
            SectionPtrs items;
            for (const Text& text : texts)
                items.push_back(textEntry(text));
            putEntries(entry, key, std::move(items));
        }

        [[nodiscard]] SectionPtr kindEntry(const std::wstring_view kind)
        {
            SectionPtr entry = std::make_unique<Dom::Section>(nullptr);
            putWords(*entry, Keys::Kind, kind);
            return entry;
        }

        [[nodiscard]] SectionPtr entryOf(const Paragraph&);
        [[nodiscard]] SectionPtr entryOf(const Heading&);
        [[nodiscard]] SectionPtr entryOf(const ListItem&);
        [[nodiscard]] SectionPtr entryOf(const CodeBlock&);
        [[nodiscard]] SectionPtr entryOf(const Table&);
        [[nodiscard]] SectionPtr entryOf(const Figure&);
        [[nodiscard]] SectionPtr entryOf(const DoDont&);
        [[nodiscard]] SectionPtr entryOf(const Callout&);
        [[nodiscard]] SectionPtr entryOf(const Fold&);

        template <typename BlockT>
        [[nodiscard]] SectionPtrs entriesOf(const std::vector<BlockT>& blocks)
        {
            SectionPtrs entries;
            for (const BlockT& block : blocks)
            {
                entries.push_back(std::visit([](const auto& held) {
                    return entryOf(held);
                }, block));
            }
            return entries;
        }

        SectionPtr entryOf(const Paragraph& paragraph)
        {
            SectionPtr entry = kindEntry(Kinds::Paragraph);
            putText(*entry, Keys::Text, paragraph.text);
            return entry;
        }

        SectionPtr entryOf(const Heading& heading)
        {
            SectionPtr entry = kindEntry(Kinds::Heading);
            putValue(*entry, Keys::Level, heading.level, Heading{}.level);
            putWords(*entry, Keys::Anchor, heading.anchor);
            putText(*entry, Keys::Text, heading.text);
            return entry;
        }

        SectionPtr entryOf(const ListItem& item)
        {
            SectionPtr entry = kindEntry(Kinds::ListItem);
            putValue(*entry, Keys::List, item.kind, ListItem{}.kind);
            putText(*entry, Keys::Text, item.text);
            return entry;
        }

        SectionPtr entryOf(const CodeBlock& codeBlock)
        {
            SectionPtr entry = kindEntry(Kinds::Code);
            putWords(*entry, Keys::Language, codeBlock.language);
            putWords(*entry, Keys::Code, codeBlock.code);
            return entry;
        }

        SectionPtr entryOf(const Table& table)
        {
            SectionPtr entry = kindEntry(Kinds::Table);
            putTexts(*entry, Keys::Header, table.header);
            SectionPtrs rows;
            for (const TableRow& row : table.rows)
            {
                SectionPtr rowEntry = std::make_unique<Dom::Section>(nullptr);
                putTexts(*rowEntry, Keys::Cells, row.cells);
                putValue(*rowEntry, Keys::Group, row.isGroup, TableRow{}.isGroup);
                rows.push_back(std::move(rowEntry));
            }
            putEntries(*entry, Keys::Rows, std::move(rows));
            return entry;
        }

        SectionPtr entryOf(const Figure& figure)
        {
            SectionPtr entry = kindEntry(Kinds::Figure);
            putWords(*entry, Keys::File, figure.file);
            putWords(*entry, Keys::Description, figure.description);
            return entry;
        }

        SectionPtr entryOf(const DoDont& doDont)
        {
            SectionPtr entry = kindEntry(Kinds::DoDont);
            putTexts(*entry, Keys::Do, doDont.doItems);
            putTexts(*entry, Keys::Dont, doDont.dontItems);
            return entry;
        }

        SectionPtr entryOf(const Callout& callout)
        {
            SectionPtr entry = kindEntry(Kinds::Callout);
            putValue(*entry, Keys::Callout, callout.kind, Callout{}.kind);
            putEntries(*entry, Keys::Blocks, entriesOf(callout.blocks));
            return entry;
        }

        SectionPtr entryOf(const Fold& fold)
        {
            SectionPtr entry = kindEntry(Kinds::Fold);
            putText(*entry, Keys::Summary, fold.summary);
            putEntries(*entry, Keys::Blocks, entriesOf(fold.blocks));
            return entry;
        }

        // --- Reading ---

        [[nodiscard]] std::wstring wordsOf(const Dom::Section& entry, const std::wstring_view key)
        {
            return (entry / key).get<std::wstring>();
        }

        [[nodiscard]] Text textOf(const Dom::Section& entry, const std::wstring_view key)
        {
            return (entry / key).get<Text>();
        }

        [[nodiscard]] Texts textsOf(const Dom::Section& entry, const std::wstring_view key)
        {
            Texts texts;
            (entry / key).getTo(texts);
            return texts;
        }

        [[nodiscard]] Paragraph paragraphOf(const Dom::Section& entry)
        {
            return { textOf(entry, Keys::Text) };
        }

        [[nodiscard]] Heading headingOf(const Dom::Section& entry)
        {
            return {
                .level = (entry / Keys::Level).get<std::size_t>(),
                .anchor = wordsOf(entry, Keys::Anchor),
                .text = textOf(entry, Keys::Text)
            };
        }

        [[nodiscard]] ListItem listItemOf(const Dom::Section& entry)
        {
            return {
                .kind = (entry / Keys::List).get<ListKind>(),
                .text = textOf(entry, Keys::Text)
            };
        }

        [[nodiscard]] CodeBlock codeBlockOf(const Dom::Section& entry)
        {
            return {
                .language = wordsOf(entry, Keys::Language),
                .code = wordsOf(entry, Keys::Code)
            };
        }

        [[nodiscard]] Table tableOf(const Dom::Section& entry)
        {
            TableRows rows;
            for (const Dom::Section& row : (entry / Keys::Rows).as<Sections>())
            {
                rows.push_back({
                    .cells = textsOf(row, Keys::Cells),
                    .isGroup = (row / Keys::Group).get<bool>()
                });
            }
            return {
                .header = textsOf(entry, Keys::Header),
                .rows = std::move(rows)
            };
        }

        [[nodiscard]] Figure figureOf(const Dom::Section& entry)
        {
            return {
                .file = wordsOf(entry, Keys::File),
                .description = wordsOf(entry, Keys::Description)
            };
        }

        [[nodiscard]] DoDont doDontOf(const Dom::Section& entry)
        {
            return {
                .doItems = textsOf(entry, Keys::Do),
                .dontItems = textsOf(entry, Keys::Dont)
            };
        }

        [[nodiscard]] Callout calloutOf(const Dom::Section& entry);
        [[nodiscard]] Fold foldOf(const Dom::Section& entry);

        // The block, as one of the kinds the list may hold; nothing for any other.
        template <typename BlockT, typename KindT>
        [[nodiscard]] std::optional<BlockT> heldAs(KindT block)
        {
            if constexpr (std::is_constructible_v<BlockT, KindT>)
                return BlockT{ std::move(block) };
            else
                return std::nullopt;
        }

        // **A block of a kind the list may not hold is left out**, and so is one whose Kind no
        // block takes - a callout written inside a fold, a word misspelled by hand.
        template <typename BlockT>
        [[nodiscard]] std::optional<BlockT> blockOf(const Dom::Section& entry)
        {
            const std::wstring kind = wordsOf(entry, Keys::Kind);
            if (kind == Kinds::Paragraph)
                return heldAs<BlockT>(paragraphOf(entry));
            if (kind == Kinds::Heading)
                return heldAs<BlockT>(headingOf(entry));
            if (kind == Kinds::ListItem)
                return heldAs<BlockT>(listItemOf(entry));
            if (kind == Kinds::Code)
                return heldAs<BlockT>(codeBlockOf(entry));
            if (kind == Kinds::Table)
                return heldAs<BlockT>(tableOf(entry));
            if (kind == Kinds::Figure)
                return heldAs<BlockT>(figureOf(entry));
            if (kind == Kinds::DoDont)
                return heldAs<BlockT>(doDontOf(entry));
            if constexpr (std::is_constructible_v<BlockT, Callout>)
            {
                if (kind == Kinds::Callout)
                    return calloutOf(entry);
                if (kind == Kinds::Fold)
                    return foldOf(entry);
            }
            return std::nullopt;
        }

        template <typename BlockT>
        [[nodiscard]] std::vector<BlockT> blocksOf(const Sections& entries)
        {
            std::vector<BlockT> blocks;
            for (const Dom::Section& entry : entries)
            {
                std::optional<BlockT> block = blockOf<BlockT>(entry);
                if (block)
                    blocks.push_back(std::move(*block));
            }
            return blocks;
        }

        Callout calloutOf(const Dom::Section& entry)
        {
            return {
                .kind = (entry / Keys::Callout).get<CalloutKind>(),
                .blocks = blocksOf<CalloutBlock>((entry / Keys::Blocks).as<Sections>())
            };
        }

        Fold foldOf(const Dom::Section& entry)
        {
            return {
                .summary = textOf(entry, Keys::Summary),
                .blocks = blocksOf<FoldBlock>((entry / Keys::Blocks).as<Sections>())
            };
        }

        // --- Places ---

        // The folder the names lead to under the project's articles folder: a category's own.
        [[nodiscard]] std::filesystem::path folderOf(const Project& project,
            const SidebarNames& names)
        {
            std::filesystem::path folder = project.articlesFolder();
            for (const std::wstring& name : names)
                folder /= name;
            return folder;
        }

        // The articles the items reach that stand on the disk, those under a category too.
        void addReached(std::vector<std::filesystem::path>& reached, const Project& project,
            const SidebarItems& items, const SidebarNames& above)
        {
            for (const SidebarItem& item : items)
            {
                SidebarNames names = above;
                names.push_back(item.name);
                if (item.kind == SidebarKind::Category)
                {
                    addReached(reached, project, item.items, names);
                    continue;
                }
                if (item.kind != SidebarKind::Content)
                    continue;
                std::filesystem::path path = pathOf(project, names);
                std::error_code error;
                if (std::filesystem::exists(path, error))
                    reached.push_back(std::move(path));
            }
        }
    }

    std::filesystem::path pathOf(const Project& project, const SidebarNames& names)
    {
        std::filesystem::path path = folderOf(project, names);
        path += k_fileExtension;
        return path;
    }

    std::optional<Article> read(const std::filesystem::path& file)
    {
        ArticleDocument document{ file, Dom::AutoSave::No, articleLayout() };
        if (!document.load())
            return std::nullopt;
        return Article{
            .title = wordsOf(document, Keys::Title),
            .lead = textOf(document, Keys::Lead),
            .blocks = blocksOf<Block>((document / Keys::Blocks).as<Sections>())
        };
    }

    bool write(const Article& article, const std::filesystem::path& file)
    {
        Dom::Section root{ nullptr };
        putWords(root, Keys::Title, article.title);
        putText(root, Keys::Lead, article.lead);
        putEntries(root, Keys::Blocks, entriesOf(article.blocks));
        const Dom::FileFormat::ClaFi format{};
        return format.saveSectionToFile(root, file);
    }

    // **A rename in letter case alone** names the same file where the folder ignores case, so a
    // file standing at the new place is taken only where it is another one.
    void followRename(const Project& project, const SidebarNames& before,
        const SidebarNames& after, const SidebarKind kind)
    {
        std::filesystem::path from;
        std::filesystem::path to;
        switch (kind)
        {
            case SidebarKind::Content:
                from = pathOf(project, before);
                to = pathOf(project, after);
                break;
            case SidebarKind::Category:
                from = folderOf(project, before);
                to = folderOf(project, after);
                break;
            case SidebarKind::Divider:
            case SidebarKind::AllTheApis:
                return;
        }
        std::error_code error;
        if (!std::filesystem::exists(from, error))
            return;
        const bool taken = std::filesystem::exists(to, error)
            && !std::filesystem::equivalent(from, to, error);
        if (!taken)
            std::filesystem::rename(from, to, error);
    }

    std::vector<std::filesystem::path> unreached(const Project& project)
    {
        std::vector<std::filesystem::path> reached;
        addReached(reached, project, project.sidebar, {});
        std::vector<std::filesystem::path> result;
        std::error_code error;
        const std::filesystem::path folder = project.articlesFolder();
        if (!std::filesystem::is_directory(folder, error))
            return result;
        for (const std::filesystem::directory_entry& entry :
            std::filesystem::recursive_directory_iterator{ folder, error })
        {
            if (!entry.is_regular_file(error) || entry.path().extension() != k_fileExtension)
                continue;
            const bool isReached = std::ranges::any_of(reached,
                [&entry, &error](const std::filesystem::path& path) {
                    return std::filesystem::equivalent(path, entry.path(), error);
                });
            if (!isReached)
                result.push_back(entry.path());
        }
        std::ranges::sort(result);
        return result;
    }
}
