module SeeDocs_App.FolderBox;

import SeeDocs_App.Icons;

import ClaFi.Controls.DropList;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TextItems;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // What parts the folders of a path on the desktop it runs on.
        constexpr std::wstring_view k_separators =
            std::filesystem::path::preferred_separator == L'\\' ? L"\\/" : L"/";
        constexpr std::size_t k_listedFolders = 200;   // the most a list holds, the first by name
        constexpr float k_iconGap = 5.0f;              // between a line's icon and its name

        // Where the name of the last folder starts in what is typed: past the last separator.
        [[nodiscard]] std::size_t leafStart(const std::wstring_view typed)
        {
            const std::size_t separator = typed.find_last_of(k_separators);
            return separator == std::wstring_view::npos ? 0 : separator + 1;
        }

        // A folder picked is closed with the separator typed last, or the desktop's own.
        [[nodiscard]] wchar_t separatorAfter(const std::wstring_view parent)
        {
            if (parent.empty())
                return static_cast<wchar_t>(std::filesystem::path::preferred_separator);
            return parent.back();
        }

        [[nodiscard]] bool isSeparator(const wchar_t value)
        {
            return k_separators.find(value) != std::wstring_view::npos;
        }

        // **A server named alone is not listed**: its shares are looked up over the network, which
        // a keystroke does not wait on.
        [[nodiscard]] bool namesServerOnly(const std::filesystem::path& folder)
        {
            const std::wstring root = folder.root_name().wstring();
            const bool server = root.size() > 2 && isSeparator(root[0]) && isSeparator(root[1]);
            return server && !folder.has_relative_path();
        }

        [[nodiscard]] wchar_t folded(const wchar_t value)
        {
            return static_cast<wchar_t>(std::towlower(value));
        }

        // The folders in that folder whose names start with the leaf, case aside, by name. A name
        // starting with a dot is listed only for a leaf that does.
        [[nodiscard]] FolderNames subfolders(const std::filesystem::path& folder,
            const std::wstring_view leaf)
        {
            FolderNames names;
            const bool dotted = leaf.starts_with(L'.');
            std::error_code error;
            std::filesystem::directory_iterator entry{
                folder,
                std::filesystem::directory_options::skip_permission_denied,
                error
            };
            for (; !error && entry != std::filesystem::directory_iterator{}; entry.increment(error))
            {
                std::error_code entryError;
                if (!entry->is_directory(entryError))
                    continue;
                std::wstring name = entry->path().filename().wstring();
                if (name.starts_with(L'.') && !dotted)
                    continue;
                if (startsWithFolded(name, leaf))
                    names.push_back(std::move(name));
            }
            std::ranges::sort(names, [](const std::wstring& left, const std::wstring& right) {
                return std::ranges::lexicographical_compare(left, right, std::ranges::less{},
                    folded, folded);
            });
            if (names.size() > k_listedFolders)
                names.resize(k_listedFolders);
            return names;
        }
    }

    // FolderList

    // Walked as a combo box's list is: nothing is current until Down, and Up from the first line
    // goes back to the box.
    FolderList::FolderList(TextBox& box)
        :
        m_box{ box },
        m_list{
            box,
            [this](const DropListPick& picked) {
                take(picked.line);
            },
            DropListOptions{ .upFromFirst = UpFromFirst::Leaves }
        }
    {
    }

    void FolderList::setBase(std::filesystem::path base)
    {
        m_base = std::move(base);
    }

    // **Typing at the end lists**, as an address bar does. An edit further in corrects what stands,
    // and a list dropped over the box would be in its way.
    void FolderList::textEdited()
    {
        const TextRange selection = m_box.selection();
        const bool atEnd = selection.length == 0
            && selection.start == m_box.text().plainText().size();
        if (!atEnd)
        {
            hide();
            return;
        }
        show();
    }

    void FolderList::keyDown(KeyDownEvent& event)
    {
        if (event.handled || m_box.readOnly() == ReadOnly::Yes)
            return;
        switch (event.key)
        {
            case Keys::Return:
            {
                // The line the keys are on is what Return means; with none, Return means what was
                // typed, as it always does, with the list out of the way.
                if (m_list.pickCurrent(event.modifiers, event.stamp))
                {
                    event.handled = true;
                    return;
                }
                m_list.hide();
                return;
            }
            case Keys::Escape:
            {
                if (!m_list.shown())
                    return;
                event.handled = true;
                m_list.hide();
                return;
            }
            case Keys::Up:
            case Keys::Down:
            {
                const ScrollDirection direction = event.key == Keys::Up
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                if (m_list.moveCurrent(direction))
                {
                    event.handled = true;
                    return;
                }
                // Down drops the list over the text as it stands, Alt held or not.
                if (event.key == Keys::Down)
                {
                    show();
                    event.handled = m_list.shown();
                }
                return;
            }
            case Keys::Prior:
            case Keys::Next:
            {
                const ScrollDirection direction = event.key == Keys::Prior
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                event.handled = m_list.moveCurrentByPage(direction);
                return;
            }
            // **Down before the focus moves on**: a list going down hands the focus back to the
            // box, which would take back the step Tab is about to make.
            case Keys::Tab:
                m_list.hide();
                return;
            default:
                return;
        }
    }

    void FolderList::hide()
    {
        m_list.hide();
    }

    // **The text names a folder and the start of a name in it**: what stands up to the last
    // separator, read from the base where it is relative, and what follows. The list stands off
    // the field the box fills, where it fills one, and is at least as wide.
    void FolderList::show()
    {
        const std::wstring typed = m_box.text().plainText();
        const std::size_t start = leafStart(typed);
        std::wstring_view parent = std::wstring_view{ typed }.substr(0, start);
        trimLeft(parent);
        const std::wstring_view leaf = std::wstring_view{ typed }.substr(start);
        const std::filesystem::path written{ std::wstring{ parent } };
        const std::filesystem::path folder = m_base / written;
        if ((!written.is_absolute() && m_base.empty()) || namesServerOnly(folder))
        {
            m_names.clear();
            hide();
            return;
        }

        m_names = subfolders(folder, leaf);
        DropListLines lines;
        for (const std::wstring_view name : m_names)
        {
            DropListLine line;
            line.text << rowIcon(RowIcon::Folder) << Space{ k_iconGap };
            if (!leaf.empty())
                line.text << InkWell::accentInk() << name.substr(0, leaf.size()) << PopColor{};
            line.text << name.substr(leaf.size());
            lines.push_back(std::move(line));
        }
        const Control& anchor = m_box.isHostedAsBody() ? *m_box.parent() : m_box;
        m_list.show(std::move(lines), std::nullopt, FormPlacement::BottomLeft, anchor);
    }

    // **A folder picked is walked into**: it goes into the box closed with a separator, and the
    // folders under it are listed in turn. An edit as typing is, so Ctrl+Z takes it back.
    void FolderList::take(const std::size_t line)
    {
        if (line >= m_names.size())
            return;
        const std::wstring typed = m_box.text().plainText();
        const std::size_t start = leafStart(typed);
        std::wstring taken = typed.substr(0, start);
        taken.append(m_names[line]);
        taken.push_back(separatorAfter(std::wstring_view{ typed }.substr(0, start)));
        m_box.selectAll();
        m_box.replaceSelectedText(taken);
    }
}
