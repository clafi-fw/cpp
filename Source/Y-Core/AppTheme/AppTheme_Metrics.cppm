export module ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // The design metrics a control is laid out by.
    export class ControlMetrics
    {
    public:
        ControlMetrics() = default;
    public:
        void setFixedWidth(float value) { minSize.x = maxSize.x = value; }
        void setFixedHeight(float value) { minSize.y = maxSize.y = value; }
        void setFixedSize(FixedSize value) {
            minSize = value;
            maxSize = value;
        }
    public:
        Thickness border{};
        float radius{};
        Padding padding{};
        Spacing spacing{};
        MinSize minSize{};
        // WHAT THIS CONTROL ASKS FOR WHEN IT IS FREE TO CHOOSE, per axis. Zero states no
        // preference on that axis and the content decides it, which is the usual case; a stated
        // one is the whole box, padding included, and stands in place of what the content
        // measured. Held between minSize and maxSize like any other answer.
        //
        // It is the third thing a size can be, and the other two cannot say it: a minimum is
        // never smaller, a maximum is never bigger, and neither of them is "this size, unless
        // something with a better claim decides otherwise" - a dialog that opens at a stated size
        // and is still resizable, a pane that starts at a third of the split.
        PreferredSize preferredSize{};
        MaxSize maxSize{ k_maxFloat, k_maxFloat };
        // How deep into the surface this style of control sits, as a multiple of the depths in
        // ThemeMetrics. It governs the whole of the Z movement and not one end of it: the resting
        // depth a control rises out of under the pointer, and the further depth it sinks to while
        // held, are one push seen at two distances, and this scales both. Flush is the fixed point
        // and stays flush at any value.
        //
        // 1 is the theme's depth. 0 opts the style out. Above 1 overstates, which is worth doing
        // only where the projection has left a control too small for its travel to be seen.
        float zDepthFactor{};
    };

    // The shadow a window casts, in logical pixels. See AppTheme
    export struct WindowShadow
    {
        float blur{};
        FloatPoint offset{};
        float spread{};
        float opacity{};
    };

    export struct ThemeMetrics
    {
        ThemeMetrics();
        static constexpr Thickness border = Thickness::Thin;
        // The widest stroke a control can paint: a focused one thickens its border to this.
        // Anything laid over a control's edge has to leave at least this much clear, or it covers
        // the focus ring on the side it reaches.
        static constexpr Thickness focusedBorder = Thickness::Regular;
        // The Z animation stated as depth. A control rests pressRestDepth into the surface, rises
        // flush with it under the pointer and sinks to pressHeldDepth while held - the same
        // distances for every control, so nothing on a surface travels further into it than
        // anything else does.
        //
        // Hover is exactly flush on purpose: nothing ever paints outside its own bounds, so no
        // control needs an inflated invalidate rect or an escape from its parent's clip. It is also
        // why the two depths cannot be scaled apart - flush is where both meet, so anything scaling
        // one of them from there scales the other. ControlMetrics::zDepthFactor is that scale, and
        // Control::allowZAnimation is what decides whether a control has any of this at all.
        //
        // viewerDistance is where the eye is, in the same points. It is a stylistic choice and not
        // a measured one: a real viewer sits some two thousand points from the screen, and at that
        // distance a two point press projects to under a twentieth of a pixel on anything. This is
        // a far closer, far wider angle than reality, picked so the movement registers at all.
        static constexpr float viewerDistance = 98.0f;
        static constexpr float pressRestDepth = 2.0f;
        static constexpr float pressHeldDepth = 3.0f;

        // Perspective projection, and the reason a small control moves less than a large one for
        // the same push. The ratio carries no size term - one depth gives one ratio to everything -
        // so what differs between controls is not how far they travel but how much of that travel
        // reaches the screen, which is proportional to how big they are. Deriving the ratio rather
        // than authoring it is what keeps the depths comparable to each other and to a real one.
        static constexpr float pressRestScale = viewerDistance / (viewerDistance + pressRestDepth);
        static constexpr float pressHeldScale = viewerDistance / (viewerDistance + pressHeldDepth);
        // A control with no surface at rest has none until it is hovered or selected, so the
        // only thing its
        // surface can do on the way in is arrive. This is the size it arrives from, as a
        // fraction of the size it settles at. Unlike the press scales it is a large number on
        // purpose - it is not a nudge, it is where the shape comes from - and it costs nothing in
        // paint area because it only ever makes the surface smaller than the control.
        static constexpr float surfaceGrowInScale = 0.83f;
        static constexpr float scrollBarWidth = 17.0f;
        ControlMetrics button;
        ControlMetrics formTitle;
        // THE FRAME A WINDOW WEARS: border is the ring its content stands inside and radius its
        // corners, both painted by the framework on every platform. The padding of a primary
        // window is that ring and nothing more.
        ControlMetrics primaryWindow;
        ControlMetrics secondaryWindow;
        WindowShadow primaryWindowShadow;
        WindowShadow secondaryWindowShadow;
        ControlMetrics page;
        ControlMetrics checkMark;
        ControlMetrics listItem;
        ControlMetrics scrollButton;
        ControlMetrics scrollThumb;
        ControlMetrics tab;
    };


    //-------------------------------------------------------------------------


    ThemeMetrics::ThemeMetrics()
    {
        static constexpr float btnRadius = 4.0f;

        // button (also used for labels, checkboxes, edits)
        button.border = border;
        button.radius = btnRadius;
        button.padding = { 8.0f, 4.0f };
        button.spacing = { 4.0f, 4.0f };
        button.minSize = { 40.0f, 24.0f };
        button.zDepthFactor = 1.0f;

        // appTitleBar
        //appTitleBar.padding = { 4.0f, 4.0f };
        //appTitleBar.radius = 8.0f;
        //appTitleBar.minSize = { 100.0f, 33.0f };

        // primaryWindow
        primaryWindow.border = Thickness::Thin;
        primaryWindow.radius = 8.0f;
        primaryWindow.padding = { strokeWidth(primaryWindow.border), strokeWidth(primaryWindow.border) };
        primaryWindowShadow.blur = 24.0f;
        primaryWindowShadow.offset = { 0.0f, 6.0f };
        primaryWindowShadow.opacity = 0.4f;

        // secondaryWindow
        secondaryWindow.border = Thickness::Thin;
        secondaryWindow.padding = 4.0f;
        secondaryWindow.radius = 6.0f;
        secondaryWindowShadow.blur = 12.0f;
        secondaryWindowShadow.offset = { 0.0f, 3.0f };
        secondaryWindowShadow.opacity = 0.35f;

        // page
        page.border = border;
        page.padding = 1.0f;
        page.radius = 4.0f;

        // checkMark
        checkMark.border = Thickness::Hairline;
        checkMark.padding = 2.33f;
        checkMark.radius = 3.0f;
        // minSize MUST always be equal maxSize.
        // that allows skipping the text calculating in list items
        // and run list items calculation in parallel (only it the list item themthelves have fixed size)
        checkMark.minSize = 16.0f;
        checkMark.maxSize = 16.0f;
        // The indicator carries the depth for the whole check box or radio button, because its host
        // does not take any - see Control::allowZAnimation. Pushing the row would move a label that
        // has no business moving, and the mark is the part that reads as the thing being pressed.
        //
        // Exaggerated, and the only place in the theme that is. The perspective projection scales
        // travel by how big a control is, so one depth gives a 114 point card 1.14 device pixels of
        // hover travel and this 16 point mark 0.16 - a sixth of a pixel, which is not a movement
        // anyone sees. Everything on a surface travelling the same distance into it is the right
        // rule for controls that sit on that surface together; a mark that is the only moving part
        // of its control is not competing with anything, so it is free to overstate.
        //
        // At 3 the mark travels 0.48 pixels rising to flush and 0.71 sinking from there to held,
        // the same order as the card, and rests at 15.04 pixels rather than 16 - a difference
        // visible only against a second mark that is not resting. Both ends move together: the
        // factor cannot lift the hover without deepening the press by the same ratio.
        checkMark.zDepthFactor = 3.0f;

        // listItem (also used by check boxes and radio buttons)
        listItem.radius = 4.0f;
        listItem.padding = { 5.0f, 3.0f };
        listItem.spacing = { 4.0f, 4.0f }; // as Button
        listItem.border = border /* borderWidth needed for focus rect */;
        listItem.zDepthFactor = 1.0f;

        // scrollButton
        scrollButton.radius = 3.0f;
        scrollButton.minSize = { scrollBarWidth, scrollBarWidth };

        // scrollThumb
        scrollThumb.radius = 5.5f;

        // tab
        tab.border = border;
        // A tab carries an icon in the same slot a button does, so it needs the same gap between
        // that icon and the caption.
        tab.spacing = { 4.0f, 4.0f };
        tab.radius = 4.0f;
        tab.padding = { 8.0f, 4.0f };
    }
}
