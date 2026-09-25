module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.PictureView;

import ClaFi.Core.Foundation;

import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class PictureView;

    // The selected pixel moved: a click, a wheel notch, a setter, a new picture. See Controls
    export struct PixelSelectEvent : public EventOf<PictureView>
    {
        using EventOf<PictureView>::EventOf;
    };

    // The selected pixel was acted on: a double click on it. See Controls
    export struct PixelActivateEvent : public EventOf<PictureView>
    {
        using EventOf<PictureView>::EventOf;
    };

    // The zoom moved: by the wheel, by a setter, or to the fit a new picture opens at. See Controls
    export struct ZoomChangeEvent : public EventOf<PictureView>
    {
        using EventOf<PictureView>::EventOf;
    };

    // A picture at a zoom, standing where the zooms and the pans put it, one pixel of it
    // selected. See Controls
    export class PictureView : public Control
    {
    public:
        template<typename... Args>
        explicit PictureView(const CreateParams&, Args&&...);
    public:
        // The selected pixel moved: a click, a wheel notch, a setter, a new picture. See Controls
        DECLARE_EVENT(PixelSelectEvent, OnPixelSelect, onPixelSelect)
        // The selected pixel was acted on: a double click on it. See Controls
        DECLARE_EVENT(PixelActivateEvent, OnPixelActivate, onPixelActivate)
        // The zoom moved. See Controls
        DECLARE_EVENT(ZoomChangeEvent, OnZoomChange, onZoomChange)
    public:
        // The picture shown, borrowed for as long as it is shown. Null shows nothing. A new
        // picture opens at the fit zoom with its middle pixel selected.
        void setPicture(const Graphics::Bitmap*);
        [[nodiscard]] const Graphics::Bitmap* picture() const { return m_picture; }
        // The pixel the crosshair stands on, or none.
        [[nodiscard]] std::optional<IntPoint> selection() const { return m_selection; }
        // A pixel outside the picture selects nothing.
        void setSelection(std::optional<IntPoint>);
        [[nodiscard]] std::optional<Color> selectedColor() const;
        // Device pixels per picture pixel: 1 draws the picture one for one.
        [[nodiscard]] float zoom() const { return m_zoom; }
        // Zooms about the selected pixel, or about the middle of the view while none is selected.
        void setZoom(float);
        // One notch out or in, about the same point setZoom uses.
        void zoomBySteps(int steps);
        // The zoom that shows the whole picture in the view, never above one for one.
        [[nodiscard]] float fitZoom() const;
        static constexpr float k_minZoom = 0.05f;
        static constexpr float k_maxZoom = 50.0f;
        // The factor one notch of the wheel or one press of a button zooms by.
        static constexpr float k_zoomStep = 1.25f;
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"PictureView"; }
    protected:
        [[nodiscard]] Interactivity interactivity() const override
        {
            return Interactivity::Focusable;
        }
        // What is on screen, so a release after a pan does not carry the view back to the pixel.
        [[nodiscard]] FloatRect scrollHotspot() const override;
        // The selected pixel, and for the same reason. A menu raised by the keyboard drops under
        // the part of its owner the commands are about, and here that is the pixel the crosshair
        // stands on rather than a picture that may be larger than the screen.
        [[nodiscard]] FloatRect contextMenuAnchor() const override;
        [[nodiscard]] ScaledDimensions calculateContent(AlignEvent&) override;
        void paintSurface(PaintEvent&) override;
        void pressDown(PressDownEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
        void drag(DragEvent&) override;
        void mouseMove(MouseMoveEvent&) override;
        void mouseWheel(MouseWheelEvent&) override;
        void contextPopup(ContextPopupEvent&) override;
    private:
        // What a zoom keeps still: a point of the picture, and the point in the form it stays
        // under, both taken from the origin before the placement floored it. See anchorAt
        struct Anchor
        {
            FloatPoint picturePoint{};
            PointInForm formPoint{};
        };
        // One axis of a placement. See placeAlong
        struct AxisPlacement
        {
            float origin{}; // where the picture was asked to stand, before the floor
            float pictureAt{};
            float extent{};
            float scroll{};
        };
        // Where one pixel of the cache reads the picture along an axis: the pixel the sample
        // falls in, the next one, and how far toward the next it falls, in 256ths.
        struct Tap
        {
            int first{};
            int second{};
            int blend{};
        };
    private:
        // The picture as drawn, in device pixels.
        [[nodiscard]] FloatSize pictureSize() const;
        // Puts the picture's top left at origin, measured from the window's top left, and
        // derives from that what the view measures and where the bars have to stand.
        void placeAt(FloatPoint origin);
        // Along one axis: origin held so a margin stays in the window, where the picture then
        // starts in the view, what the view measures, and where the bar has to stand.
        [[nodiscard]] static AxisPlacement placeAlong(float origin, float size, float window,
            float margin);
        // What the box scrolling the view has carried it by from its window's top left: the
        // positions of the bars.
        [[nodiscard]] FloatPoint scrolledBy() const;
        // Where the picture's top left stands against the window's now, carrying the fraction
        // the last placement floored away.
        [[nodiscard]] FloatPoint originNow() const;
        // Sets the bars where a placement needs them.
        void scrollTo(FloatPoint scroll);
        [[nodiscard]] FloatPoint pointInControl(PointInForm) const;
        // A point in the view as a point of the picture, in picture pixels, whether or not the
        // picture reaches it.
        [[nodiscard]] FloatPoint picturePointAt(FloatPoint pointInControl) const;
        [[nodiscard]] PointInForm pointInForm(FloatPoint picturePoint) const;
        // The pixel under a point in the form, or none outside the picture.
        [[nodiscard]] std::optional<IntPoint> pixelAt(PointInForm) const;
        // What a zoom about a point in the form keeps still: the point of the picture under
        // it, taken from the origin before the placement floored it.
        [[nodiscard]] Anchor anchorAt(PointInForm) const;
        [[nodiscard]] Anchor anchorAtSelection() const;
        // Zooms keeping the anchor still.
        void zoomTo(float zoom, Anchor);
        // The form is laid out: the fit a new picture waits for, the window a placement has to
        // be measured against again, the bars a placement left to set.
        void aligned();
        void announceSelection();
        void announceActivate();
        void announceZoom();
        void paintPicture(PaintEvent&, const FloatRect& pictureRect, const FloatRect& visible);
        void paintCrosshair(PaintEvent&, const FloatRect& pictureRect);
        // Brings the cache to hold this region of the picture at the zoom, sampling only what
        // it does not hold yet. The region is in the cache's coordinates: device pixels from
        // the picture's top left.
        void renderCache(const IntRect& region);
        // Samples pixels [from, to) of one row of the cache into line. Both line and columns
        // start at column at.x of row at.y, in the cache's coordinates.
        void renderRun(Color* line, const Tap* columns, int from, int to, IntPoint at) const;
        // Where a pixel of the cache reads the picture along an axis: the nearest pixel from
        // k_nearestFromZoom up, the two around the sample below.
        [[nodiscard]] Tap tapAt(int cachePos, int limit) const;
        [[nodiscard]] const Color* cacheRow(int y) const;
        // The four pixels around a sample blended: tx toward b and d, ty toward c and d.
        [[nodiscard]] static Color blendOf(Color a, Color b, Color c, Color d, int tx, int ty);
        [[nodiscard]] Color pixelClamped(int x, int y) const;
        // The crosshair's pixel at a device point: the contrast of the cache's pixel there, or
        // of the surface's where the picture does not reach.
        [[nodiscard]] Color crosshairPixelAt(IntPoint devicePoint, IntPoint pictureAt,
            Color surfaceMark) const;
        // A colour turned against itself channel by channel, so a hairline of it reads on
        // whatever it crosses.
        [[nodiscard]] static Color contrasted(Color);
    private:
        // From this zoom up a picture pixel is drawn as the block it is.
        static constexpr float k_nearestFromZoom = 4.0f;
        // The crosshair's reach from the pixel it stands on, each way, in design units.
        static constexpr float k_crosshairArm = 24.0f;
        // Where a channel turns: below it the contrast is near white, from it near black.
        static constexpr ColorByte k_contrastThreshold = 120;
        static constexpr ColorByte k_contrastLight = 255;
        static constexpr ColorByte k_contrastDark = 11;
        // The squares a translucent pixel shows through, in device pixels.
        static constexpr int k_checkerSize = 8;
        static constexpr Color k_checkerLight{ 204, 204, 204 };
        static constexpr Color k_checkerDark{ 153, 153, 153 };
        // How far a press travels before it pans rather than clicks, in design units.
        static constexpr float k_panThreshold = 3.0f;
        // How much of the picture stays in the window whatever pushes it out, in design units.
        static constexpr float k_keepInView = 48.0f;
    private:
        const Graphics::Bitmap* m_picture{ nullptr };
        float m_zoom{ 1.0f };
        // Where the picture starts in the view's own coordinates: past the strip it leaves bare
        // on the near side, at the edge where it hangs out.
        FloatPoint m_pictureAt{};
        // The fraction of the origin the last placement floored away, carried so that a zoom or
        // a pan measured from the screen does not lose it again.
        FloatPoint m_originFraction{};
        // What the view measures, from the last placement: the picture, and any strip beside
        // it the bars have to range over.
        FloatSize m_content{};
        // The window the last placement was measured against.
        FloatSize m_window{};
        // Where a placement needs the bars, met once the pass has ranged them over its content.
        std::optional<FloatPoint> m_scrollWanted{};
        // The part of the picture on screen, sampled at the zoom, so a paint copies rows and
        // a pan samples only the strip it uncovers.
        Graphics::Bitmap m_cache{};
        // What the cache holds: which device pixels of the picture, from its top left, and at
        // which zoom.
        IntRect m_cacheRect{};
        float m_cacheZoom{ 0.0f };
        std::optional<IntPoint> m_selection{};
        // Where the pointer last stood over the view, which is what a wheel zoom keeps still.
        PointInForm m_pointerPos{};
        // Where the pressed pointer last stood, from which the next move pans.
        PointInForm m_dragPos{};
        bool m_panning{ false };
        // A new picture opens at the fit, which needs the window the view is seen through, and
        // that is known once the form is laid out.
        bool m_fitPending{ false };
        // Last, so it is dropped first: the form outlives every control on it.
        ScopedEventConnection m_aligned{};
    };


    //-------------------------------------------------------------------------


    // PictureView
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in PictureView.cpp.

    template<typename... Args>
    PictureView::PictureView(const CreateParams& params, Args&&... args)
        :
        Control{
            params,
            // A PICTURE IS THE SIZE IT IS AT ITS ZOOM, so nothing about it is broken to the box
            // it is drawn in, and the bars carry the view across what does not fit. It is also
            // what tells the align pass that a scrolled body keeps the size it measured. It comes
            // before the caller's own arguments, so a stated one still wins - Props takes the
            // last match.
            WordWrap::No,
            std::forward<Args>(args)... }
    {
        m_aligned = params.form.onAligned([this](FormAlignedEvent&) {
            aligned();
        });
    }
}
