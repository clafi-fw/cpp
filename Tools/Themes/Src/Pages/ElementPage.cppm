export module ThisApp.ElementPage;

import ClaFi.Icons.HueIcon;
import ClaFi.Icons.LuminosityIcon;
import ClaFi.Icons.PlusMark;
import ClaFi.Icons.SaturationIcon;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Button;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

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

    // One element of a theme under the new color theme architecture, a grid row for each rule.
    export class ElementPage : public Panel
    {
    public:
        template<typename... Args>
        explicit ElementPage(const CreateParams&, UiElement, Args&&...);
    public:
        // Takes the rules the page edits and what it calls after changing them, and shows them.
        void bind(ColorRules2&, OnRulesChanged);
        // Builds a row for every rule naming this element, in list order.
        void rebuild();
    private:
        // The grid's columns, as their tags name them.
        enum class RuleColumn : TagValue
        {
            Output,
            Inputs,
            Hue,
            Saturation,
            Elevation,
            Delete
        };
        using RuleRows = std::vector<Control*>;
    private:
        void addRule();
        void deleteRule(std::size_t index);
        void deletePendingRule();
        void addRow(std::size_t index);
        void ruleCellText(Grids::GetCellTextEvent&) const;
        void rulesChanged() const;
    private:
        UiElement m_element;
        ColorRules2* m_rules{}; // every element's rules, of which the page shows its own
        OnRulesChanged m_onRulesChanged{};
        UiTimer m_deleteTimer{}; // deletes on the next tick, outside the click that asked
        std::optional<std::size_t> m_pendingDelete{};
        RuleRows m_ruleRows{}; // one per rule, the header aside

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center,
            Text{ TextStyleId::SubTitle, uiElementOf(m_element).name }
        ) };

        ToolButton& m_addButton{ m_topBar.createRightBar<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::LeftIcon,
            Button::OnPaintIcon{ Icons::PlusMark::paint },
            L"Add rule"
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
                Grids::Dt::Column{ Tag{ RuleColumn::Output },
                    Text{ L"Output" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Inputs },
                    Text{ L"Inputs" },
                    Grids::ColumnWidthMode::Fill
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Hue },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::paint }, L" Hue" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Saturation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::paint }, L" Saturation" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Elevation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Elevation" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Delete },
                    Text{},
                    Grids::CellHighlightMode::Control
                }
            },
            Grids::Dt::Header{}
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ElementPage::ElementPage(const CreateParams& params, const UiElement element, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_element{ element }
    {
        m_addButton.onClick([this](ClickEvent&) {
            addRule();
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingRule();
        });
        m_grid.onGetCellText([this](Grids::GetCellTextEvent& event) {
            ruleCellText(event);
        });
    }
}
