export module ThisApp.ElementPage;

import ThisApp.RuleSlider;

import ClaFi.Icons.HueIcon;
import ClaFi.Icons.LuminosityIcon;
import ClaFi.Icons.SaturationIcon;

import ClaFi.Controls.Button;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What an element page calls after it has changed the rules it is bound to.
    export using OnRulesChanged = std::function<void()>;
    // The colour a rule of the list is applied to, which its value ramps are drawn from.
    export using OnGetListRuleBase = std::function<RuleBase(const ColorRule2&, RuleChannel)>;

    // One list of the new rules - an element's own or the shared ones - a grid row for each rule.
    export class ElementPage : public Panel
    {
    public:
        template<typename... Args>
        explicit ElementPage(const CreateParams&, std::wstring_view title, Args&&...);
    public:
        // Takes the list, the colours its hues read, its ramps' base and what to call after edits.
        void bind(ColorRules2&, const ThemeColors&, OnGetListRuleBase, OnRulesChanged);
        // Builds a row for every rule in the list, in list order.
        void rebuild();
    private:
        // The grid's columns, as their tags name them.
        enum class RuleColumn : TagValue
        {
            ApplyTo,
            Hue,
            Saturation,
            Elevation
        };
        using RuleRows = std::vector<Grids::RowContainer*>;
        using RuleIndices = std::vector<std::size_t>;
    private:
        void addRule();
        void deleteSelectedRules();
        void deletePendingRules();
        void addRow(std::size_t index);
        void bindEditors(Grids::RowContainer&);
        [[nodiscard]] Grids::Column& column(RuleColumn);
        [[nodiscard]] OnGetRuleBase ruleBaseOf(std::size_t index, RuleChannel) const;
        void rulesChanged() const;
    private:
        ColorRules2* m_rules{}; // the list the page shows, null until bound
        const ThemeColors* m_colors{}; // what the hue editors read the palette from
        OnGetListRuleBase m_ruleBase{};
        OnRulesChanged m_onRulesChanged{};
        UiTimer m_deleteTimer{}; // deletes on the next tick, outside the event that asked
        RuleIndices m_pendingDeletes{};
        RuleRows m_ruleRows{}; // one per rule, the header aside

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title;

        StackPanel& m_tools{ m_topBar.createRightBar<StackPanel>(
            Orientation::Horizontal,
            Interactivity::ActiveContainer,
            Spacing{ 4.0f }
        ) };

        // Words, icon and state come from the action; mouse only keeps the focus in the grid.
        ToolButton& m_deleteButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            StdActions::del,
            Interactivity::MouseOnly
        ) };

        // A row names its rule by the rule's place in the list, carried as the row's tag.
        Grids::Dt::Grid& m_grid{ createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f }
        ).add<Grids::Dt::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,
            SelectionMode::Multi,
            Grids::Dt::Columns{
                Grids::Dt::Column{ Tag{ RuleColumn::ApplyTo },
                    Text{ L"Apply to" },
                    Grids::ColumnWidthMode::Fill
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Hue },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::paint }, L" Hue" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Saturation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::paint },
                        L" Saturation" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Elevation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Elevation" }
                }
            },
            Grids::Dt::Header{},
            Grids::Dt::NewItem{ PlaceHolderText{ L"Add rule" } }
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ElementPage::ElementPage(const CreateParams& params, const std::wstring_view title,
        Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center,
            Text{ TextStyleId::SubTitle, title }
        ) }
    {
        m_grid.onNewItem([this](Grids::NewItemEvent&) {
            addRule();
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingRules();
        });
        m_grid.onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        // The toolbar button and the Delete key land here alike, and the selection they act on is
        // this page's grid.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = !m_grid.selection().empty() });
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::del)
                deleteSelectedRules();
        });
    }
}
