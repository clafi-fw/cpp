export module Themes_App.WindowsPreview;

import Themes_App.DialogMockup;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Menu;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Two dialogs, focused and not, a menu and a tooltip, each casting its window's shadow.
    export class WindowsPreview : public PreviewPageBase
    {
    public:
        explicit WindowsPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    protected:
        void paintChildren(PaintEvent&) override;
    private:
        // Draws the shadow the window a mockup stands for casts at that focus, under the mockup.
        void drawWindowShadow(PaintEvent&, const RichControl& mockup, UiElement,
            const WindowShadow&, float windowFocusedFactor);
        // A menu line, which keeps the click from closing the form it stands in.
        MenuItem& addMenuLine(std::wstring_view caption);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Window surfaces, dialog title, window shadows, focused and not"
        };
        static constexpr float k_tooltipWidth = 180.0f;

        DialogMockup& m_focusedDialog{ add<DialogMockup>(L"Aquarium", WindowFocus::Focused) };

        Label& m_question{ m_focusedDialog.body().add<Label>(
            L"Put the pike back in the lake?"
        ) };

        StackPanel& m_answers{ m_focusedDialog.body().add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 8.0f },
            HorizontalAlign::Right
        ) };

        PrimaryButton& m_releaseButton{ m_answers.add<PrimaryButton>(L"Release") };
        Button& m_keepButton{ m_answers.add<Button>(L"Keep") };

        DialogMockup& m_unfocusedDialog{ add<DialogMockup>(L"Tank log", WindowFocus::Unfocused) };

        Label& m_logLine{ m_unfocusedDialog.body().add<Label>(L"The filter is clean.") };

        Button& m_okButton{ m_unfocusedDialog.body().add<Button>(
            L"OK",
            HorizontalAlign::Right
        ) };

        StackPanel& m_menu{ add<StackPanel>(
            Orientation::Vertical,
            UiElement::Menu,
            MetricsSet::SecondaryWindow,
            Padding{ 4.0f },
            HorizontalAlign::Left,
            ClickTarget::Self,
            MouseFocus::Keep,
            FocusScope::Yes
        ) };

        MenuItem& m_feedLine{ addMenuLine(L"Feed") };
        MenuItem& m_weighLine{ addMenuLine(L"Weigh") };
        MenuItem& m_measureLine{ addMenuLine(L"Measure") };
        Divider& m_menuDivider{ m_menu.add<Divider>() };
        MenuItem& m_releaseLine{ addMenuLine(L"Release") };

        Label& m_tooltip{ add<Label>(
            UiElement::Tooltip,
            MetricsSet::SecondaryWindow,
            Text{ L"Weighs the catch on the brass scale" },
            HorizontalAlign::Right,
            MaxSize{ k_tooltipWidth, k_maxFloat }
        ) };
    };
}
