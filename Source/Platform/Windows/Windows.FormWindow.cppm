module;
#include "Windows.Headers.h"
export module ClaFi.Platform.Windows.FormWindow;

import ClaFi.Platform.Windows.Window;
import ClaFi.Platform.Windows.Composition;
import ClaFi.Platform.Windows.ShadowWindow;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    export class FormWindow : public Window
    {
    public:
        FormWindow(WindowsManager& owner, IForm&, WindowRole, IForm* parentForm);
        ~FormWindow() override;
    public:
        void show() override;
        void update() override;
        void setFrame(const WindowFrame&) override;
        void invalidateRect(const IntRect*) override;
        ColorByte alpha() override;
        void setAlpha(ColorByte) override;
    protected:
        void setStoredBounds(const IntRect& value) override;
        LRESULT getBorderSizingHitTest(IntPoint mp);
        void wndProc(WinApiMsg& msg) override;
        virtual void focusChanged() override;
    private:
        static bool isSystemHitTest(WPARAM);
        static bool isFrameHitTest(WPARAM);
        void dwmEnableDarkMode();
        // DWM draws nothing around the window: no shadow, no rounding, no border line. The form
        // draws all three. Stated at the first show and not in the constructor: changing the
        // rendering policy recalculates the frame, and the WM_WINDOWPOSCHANGED that reaches the
        // form from inside its own construction finds a form with no content yet.
        void dwmDisableFrame();
        // Whether this press is the third of one run of clicks. Windows counts to two: it reports
        // the second press as WM_LBUTTONDBLCLK and every press after it as another
        // WM_LBUTTONDOWN, so the third is recognised here or nowhere. Answering ends the run
        // either way, so a fourth press starts a new one and the count never runs past three.
        [[nodiscard]] bool takeTripleClick(IntPoint mousePos);
        // A point in the client area, in the form's surface.
        [[nodiscard]] IntPoint toSurface(IntPoint clientPoint) const;
        [[nodiscard]] FloatPoint toSurface(FloatPoint clientPoint) const;
        // The client area as a rect in the form's surface.
        [[nodiscard]] IntRect clientArea() const;
        // THE DESIGN CHANGED UNDER A WINDOW OF THE SAME SIZE - a theme switch, a scale. The ring
        // moves the window rect so the geometry stays where it is; anything else is a resize the
        // form is told of directly.
        void reframe();
        // Where the shadow window stands and whether it shows, from the window rect given or the
        // current one. Called on every move and on every change of state.
        void syncShadow(const IntRect* windowRect);
        // The frame's dirty rect to the window through its presenter - from the bitmap when the
        // backend handed one back, from the presenter's frame texture otherwise - and the shadow
        // window filled again where the rect reaches the margins.
        void present(const Graphics::Bitmap*, const IntRect& dirty);
        void paint();
        void schedulePaint();
    private:
        // WHAT THE FORM HAS INVALIDATED SINCE THE LAST PAINT, in the surface: one rect, or the
        // whole of it after a size, a frame or a whole invalidation. The window's own record -
        // see invalidateRect.
        static constexpr UINT k_paintMessage = WM_APP + 1;
        IForm& m_form;
        bool m_lockDpiChange{ false };
        // Where and when the run's second press landed. A third press counts only while it is
        // inside the system's double click time and drag box measured from that point, which are
        // the same two limits Windows itself applied to the second.
        bool m_doubleClickPending{ false };
        LONG m_doubleClickTime{ 0 };
        IntPoint m_doubleClickPos{};
        // A right press landed on the caption; the release over it is the window menu's.
        bool m_captionRightPressed{ false };
        std::unique_ptr<CompositionPresenter> m_presenter;
        // A resize step has gone out and the compositor has not shown it yet; the posted paint
        // waits until it has - see WM_SIZE.
        bool m_awaitComposition{ false };
        std::unique_ptr<ShadowWindow> m_shadow;
        ColorByte m_alpha{ 255 };
        bool m_dwmFrameDisabled{ false };
        // The design changed while the window was not shown; the show tells the form.
        bool m_frameStale{ false };
        IntRect m_surfaceDirty{};
        bool m_surfaceWholeDirty{ true };
        bool m_paintScheduled{ false };
    };

}
