export module Themes_App.SizesPage;

import ClaFi.Icons.ResetIcon;

import ClaFi.Controls.Button;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What a sizes page calls after it has moved a size: where the edit stands, and its name.
    export using OnSizesChanged = std::function<void(EditPhase, const Text& what)>;

    // One set of sizes a theme states, a grid row for each size.
    export class SizesPage : public Panel
    {
    public:
        template<typename... Args>
        explicit SizesPage(const CreateParams&, MetricsSet, Args&&...);
    public:
        // Takes the theme's sizes, the ones a reset puts back, and what to call after an edit.
        void bind(ThemeMetrics&, const ThemeMetrics& defaults, OnSizesChanged);
        // Shows the sizes as they stand, after the theme moved under the page.
        void refresh();
    private:
        // The grid's columns, as their tags name them.
        enum class SizeColumn : TagValue
        {
            Name,
            Value, // a size of one number, its cell spanning the two columns under it
            Across,
            Down
        };
        // What one row shows and edits.
        enum class SizeRow : TagValue
        {
            Border,
            Band,
            Radius,
            Padding,
            Spacing,
            MinSize,
            PreferredSize,
            Depth,
            GrowIn,
            ShadowBlur,
            ShadowOffset,
            ShadowSpread,
            ShadowOpacity
        };
        using SizeRows = std::initializer_list<SizeRow>;
    private:
        void addRows();
        // The weights are a fixed list, so the border's is picked from them.
        void addBorderRow();
        void addRow(Grids::GridBase&, SizeRow);
        // A group of rows under a header that names what they share.
        void addGroup(std::wstring_view name, SizeRows);
        void connectRow(Grids::Row&, SizeRow);
        void writeCell(Grids::GetCellTextEvent&, SizeRow);
        void writeEditorText(Grids::GetCellEditorTextEvent&, SizeRow);
        void accept(Grids::AcceptCellTextEvent&, SizeRow);
        void borderPicked();
        // The refusal for a typed number, empty where it reads as one the row may hold.
        [[nodiscard]] static std::wstring readNumber(SizeRow, std::wstring_view typed,
            float& value);
        void sizeEdited(SizeRow);
        void reset();
        void changed(const Text& what);
        [[nodiscard]] bool differsFromDefaults() const;
        // The number a cell edits, null for a cell holding none and before the page is bound.
        [[nodiscard]] float* numberOf(SizeRow, const Grids::Column&);
        [[nodiscard]] FloatPoint& pairOf(SizeRow);
        [[nodiscard]] std::wstring_view nameOf(SizeRow) const;
        [[nodiscard]] std::wstring_view setName() const;
        // Whether the set is square and sized by one number - a check mark, a scroll button.
        [[nodiscard]] bool hasSingleSize() const;
        // Whether a row holds two numbers, across and down, rather than one.
        [[nodiscard]] bool isPair(SizeRow) const;
        [[nodiscard]] bool hasCell(SizeRow, const Grids::Column&) const;
        [[nodiscard]] static bool isLength(SizeRow);
    private:
        const MetricsSet m_set;
        ThemeMetrics* m_metrics{}; // the theme's sizes, null until bound
        const ThemeMetrics* m_defaults{}; // what a reset puts back, null until bound
        OnSizesChanged m_onSizesChanged{};
        ComboBox* m_borderPick{}; // the Border row's weight, built with the rows

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center
        ) };

        ToolButton& m_resetButton{ m_topBar.createRightBar<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::ResetIcon::draw },
            L"Reset to defaults",
            MouseFocus::Keep
        ) };

        Grids::Dt::Grid& m_grid{ createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f }
        ).add<Grids::Dt::Grid>(
            MetricsSet::Page,
            UiElement::Section,
            HorizontalAlign::Left,
            Grids::GridLines::Horizontal,
            Grids::Dt::Columns{
                Grids::Dt::Column{ Tag{ SizeColumn::Name },
                    Text{ L"Size" },
                    Grids::ColumnWidthMode::FitContent
                },
                Grids::Dt::Column{ Tag{ SizeColumn::Value },
                    Text{ L"Value" },
                    Grids::ColumnWidthMode::Fixed,
                    Grids::ColumnWidth{ 160.0f },
                    TextAlign::Right,
                    EditorMode::Editable,
                    Grids::Dt::Column{ Tag{ SizeColumn::Across },
                        Text{ L"Across" },
                        Grids::ColumnWidthMode::Fill,
                        TextAlign::Right,
                        EditorMode::Editable
                    },
                    Grids::Dt::Column{ Tag{ SizeColumn::Down },
                        Text{ L"Down" },
                        Grids::ColumnWidthMode::Fill,
                        TextAlign::Right,
                        EditorMode::Editable
                    }
                }
            },
            Grids::Dt::Header{}
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    SizesPage::SizesPage(const CreateParams& params, const MetricsSet set, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_set{ set }
    {
        m_title.text() << StyledText{ TextStyles::SubTitle, setName() };
        m_title.text() << L"  " << StyledText{ TextStyles::SubBody,
            ColoredText{ InkGrade::Muted, L"sizes" } };
        addRows();
        m_resetButton.onClick([this](ClickEvent&) {
            reset();
        });
        // Offered only where a reset would change something.
        m_resetButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = differsFromDefaults();
            event.stopPropagation();
        });
    }
}
