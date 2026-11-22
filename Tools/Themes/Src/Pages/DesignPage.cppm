export module Themes_App.DesignPage;

import Themes_App.ElementPage;
import Themes_App.PreviewPage;
import Themes_App.RuleSlider;

import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export using OnGetElementRuleBase =
        std::function<RuleBase(std::wstring_view style, const ColorRule&, RuleChannel)>;

    export struct DesignPlace
    {
        std::wstring page{};
        RuleSelection selection{};
    };

    export class DesignPage;

    class ElementTree : public TreeView
    {
    public:
        ElementTree(const CreateParams&, DesignPage&);
    protected:
        void nestedDrag(DragEvent&) override;
        void nestedPressUp(PressUpEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
    private:
        DesignPage& m_design;
    };

    export class DesignPage : public Panel
    {
        friend ElementTree;
    public:
        template<typename... Args>
        explicit DesignPage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view pickedPage() const;
        [[nodiscard]] NodePreview pickedPreview() const;
        void pickPage(std::wstring_view anchor);
        void showPlace(const DesignPlace&);
        template<typename F>
        EventConnection onPagePick(F&& callback);
        [[nodiscard]] StackPanel& paletteView() { return m_paletteView; }
        void bind(AppTheme&, const OnGetElementRuleBase&, const OnRulesChanged&);
        void rebuildRules();
    protected:
        void visibilityChanged() override;
    private:
        struct TreeEntry
        {
            std::wstring anchor{};
            std::wstring name{};
            RichControl* row{};
            Control* page{};
            TreeNode* node{};
            ElementPage* elementPage{};
        };

        struct ElementLink
        {
            std::wstring name{};
            std::wstring base{};
        };

        using Children = std::map<std::wstring_view, std::vector<const ControlStyle*>>;
        using ShownElements = std::set<std::wstring_view>;
    private:
        void buildTree();
        void rebuildTree();
        void addElementRow(const ControlStyle&, TreeNode& parent, const Children&,
            ShownElements&, const std::set<std::wstring>& collapsed);
        void addEntry(TreeEntry);
        void bindPage(const TreeEntry&);
        void rebuildPagesBasedOn(std::wstring_view style);
        [[nodiscard]] SetNames styleNames() const;
        [[nodiscard]] const TreeEntry* pickedEntry() const;
        [[nodiscard]] const TreeEntry* entryAt(const Control*) const;
        void showPickedPage();
        void focusElement(std::wstring_view name);
        void addStyle(std::wstring_view base);
        void nameAddedStyle();
        void renameStyle(std::wstring_view name, AcceptEditEvent&);
        void renamePendingStyle();
        void deleteStyle(std::wstring_view name);
        void deletePendingStyle();
        void dragElement(DragEvent&);
        void releaseElement(PressUpEvent&);
        [[nodiscard]] bool cancelDrag();
        [[nodiscard]] const TreeEntry* dropEntry(const Control* pointedControl) const;
        [[nodiscard]] bool canMoveStyle(std::wstring_view name, std::wstring_view base) const;
        void setDropTarget(RichControl*);
        void movePendingStyle();
        void setsChanged(const Text&);
    private:
        static constexpr std::size_t k_rootIndex = 1ull;

        std::vector<TreeEntry> m_entries{};
        std::vector<ElementLink> m_links{};
        ThemeColors* m_colors{};
        ThemeStyles* m_styles{};
        OnGetElementRuleBase m_ruleBase{};
        OnRulesChanged m_onRulesChanged{};
        const ThemeStyles m_defaultStyles{ AppTheme{}.styles };

        std::wstring m_newStyleBase{};
        std::wstring m_styleToRename{};
        std::wstring m_newName{};
        std::wstring m_styleToDelete{};
        std::wstring m_styleToMove{};
        std::wstring m_newBase{};
        std::wstring m_draggedStyle{};
        RichControl* m_dropTarget{};
        bool m_dragCanceled{};
        bool m_rebuildingTree{};
        UiTimer m_nameTimer{};
        UiTimer m_renameTimer{};
        UiTimer m_deleteTimer{};
        UiTimer m_moveTimer{};

        ScrollBox& m_treeScroll{ createLeftBar<ScrollBox>(ScrollBars::Vertical, UiElement::Section) };
        ElementTree& m_tree{ m_treeScroll.createBody<ElementTree>(*this) };
        PageControl& m_pages{ createBody<PageControl>() };
        Panel& m_palettePage{ m_pages.add<Panel>() };
        Label& m_paletteTitle{ m_palettePage.createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ).createBody<Label>(
            VerticalTextAnchor::Center
        ) };
        StackPanel& m_paletteView{ m_palettePage.createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f },
            Spacing{ 8.0f }
        ) };
        Panel& m_elementsPage{ m_pages.add<Panel>() };
    };

    template<typename... Args>
    DesignPage::DesignPage(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        buildTree();
        m_nameTimer.onTick([this](TimerEvent&) {
            nameAddedStyle();
        });
        m_renameTimer.onTick([this](TimerEvent&) {
            renamePendingStyle();
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingStyle();
        });
        m_moveTimer.onTick([this](TimerEvent&) {
            movePendingStyle();
        });
        setCurrentItem(m_pages);
    }

    template<typename F>
    EventConnection DesignPage::onPagePick(F&& callback)
    {
        return m_tree.onCurrentItemChange(
            [this, callback = std::forward<F>(callback)](CurrentItemChangeEvent& event) mutable {
                if (!m_rebuildingTree)
                    callback(event);
            });
    }
}
