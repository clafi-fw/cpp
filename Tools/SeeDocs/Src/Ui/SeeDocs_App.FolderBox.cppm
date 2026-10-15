export module SeeDocs_App.FolderBox;

import ClaFi.Controls.DropList;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

// **A folder typed into a box**, with the folders under what is typed listed under the box: a
// folder picked goes into the box, and the folders under it are listed in turn. See README
namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    using FolderNames = std::vector<std::wstring>;

    // The folders under what is typed, a line each; a folder picked is walked into. See README
    export class FolderSuggester : public Suggester
    {
    public:
        // Where a path typed relative starts from; with none, only a path from a root lists.
        void setBase(std::filesystem::path);
        [[nodiscard]] DropListLines suggest(std::wstring_view typed) override;
        [[nodiscard]] std::wstring pick(std::size_t line) override;
        [[nodiscard]] bool pickAccepts() const override { return false; }
    private:
        std::filesystem::path m_base{};
        std::wstring m_typed{};   // what the lines were listed for
        FolderNames m_names{};    // the folders listed, a line each
    };

    // The folders the text of a box names, dropped under the box and walked from it.
    export class FolderList
    {
    public:
        explicit FolderList(TextBox&);
    public:
        // Where a path typed relative starts from; with none, only a path from a root lists.
        void setBase(std::filesystem::path base) { m_folders.setBase(std::move(base)); }
        // An edit leaving the caret at the text's end lists what the text names; any other, none.
        void textEdited();
        // Takes the keys that walk, pick and drop the list, marking the event handled.
        void keyDown(KeyDownEvent&);
        void hide();
    private:
        void show();
        void take(std::size_t line);
    private:
        TextBox& m_box;
        FolderSuggester m_folders{};
        DropList m_list;
    };

    // A box a folder is typed into, listing the folders under what is typed.
    export template<typename Box>
    class FolderBox : public Box
    {
    public:
        template<typename... Args>
        explicit FolderBox(const CreateParams&, Args&&...);
    public:
        // Where a path typed relative starts from.
        void setBaseFolder(std::filesystem::path base) { m_folders.setBase(std::move(base)); }
    protected:
        void textEdited() override;
        void focusChanged() override;                 // leaving takes the list down
        void nestedKeyDown(KeyDownEvent&) override;   // the list's keys before the box's
    private:
        FolderList m_folders{ *this };
    };


    //-------------------------------------------------------------------------


    template<typename Box>
    template<typename... Args>
    FolderBox<Box>::FolderBox(const CreateParams& params, Args&&... args)
        :
        Box{ params, std::forward<Args>(args)... }
    {
    }

    template<typename Box>
    void FolderBox<Box>::textEdited()
    {
        Box::textEdited();
        m_folders.textEdited();
    }

    template<typename Box>
    void FolderBox<Box>::focusChanged()
    {
        Box::focusChanged();
        if (!this->isFocused())
            m_folders.hide();
    }

    template<typename Box>
    void FolderBox<Box>::nestedKeyDown(KeyDownEvent& event)
    {
        m_folders.keyDown(event);
        if (event.handled)
            return;
        Box::nestedKeyDown(event);
    }
}
