export module ClaFi.Core.AppTheme_AnimationSlots;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::AnimationSlots
{
    using namespace std::chrono_literals;

    // The rise has to outlast the movement it carries, not just the colour change. A control's text
    // travels a whole device pixel under the press scale, and one pixel spread over 55ms is two
    // frames at 30Hz - a step, whatever the easing says. At 175ms it is ten, which is enough for
    // the intermediate coverage to read as a glide.
    export constexpr AnimationSlot hovered{
        .duration{.rise = 77ms, .fall = 440ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseInOut },
        .tag = static_cast<AnimationTag>(VisualStateIndex::Hovered)
    };

    export constexpr AnimationSlot pressed{
        .duration{.rise = 44ms, .fall = 33ms },
        .easingFactor{},
        .tag = static_cast<AnimationTag>(VisualStateIndex::Pressed)
    };

    export constexpr AnimationSlot focused{
        .duration{.rise = 66ms, .fall = 440ms },
        .easingFactor{},
        .tag = static_cast<AnimationTag>(VisualStateIndex::Focused)
    };

    export constexpr AnimationSlot selected{
        .duration{.rise = 66ms, .fall = 330ms },
        .easingFactor{EasingFactor::EaseOut, EasingFactor::EaseOut},
        .tag = static_cast<AnimationTag>(VisualStateIndex::Selected)
    };

    export constexpr AnimationSlot enabled{
        .duration{.rise = 330ms, .fall = 330ms },
        .easingFactor{},
        .tag = static_cast<AnimationTag>(VisualStateIndex::Enabled)
    };

    export constexpr AnimationSlot textHovered{
        .duration{.rise = 55ms, .fall = 440ms },
        .easingFactor{ EasingFactor::EaseIn, EasingFactor::EaseInOut },
        .tag = static_cast<AnimationTag>(VisualStateIndex::TextHovered)
    };

    export constexpr AnimationSlot current{
        .duration{.rise = 66ms, .fall = 440ms },
        .easingFactor{},
        .tag = static_cast<AnimationTag>(VisualStateIndex::Current)
    };

    export constexpr AnimationSlot inputDevice{
        .duration{.rise = 90ms, .fall = 600ms },
        .easingFactor{}
    };

    export constexpr AnimationSlot formAlpha{
        .duration{.rise = 66ms, .fall = 1100ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseInOut }
    };

    export constexpr AnimationSlot expander{
        .duration{.rise = 400ms, .fall = 166ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseInOut }
    };

    // The mark that says a popup is open. It turns as briskly as the popup appears and takes the
    // longer way back, so the turn home reads as the popup closing rather than as an event of its
    // own. A mark cannot ride AnimationSlots::selected: an animation is keyed by control and slot
    // together, and the control carrying the mark animates that slot for its own selected state.
    export constexpr AnimationSlot dropdownMark{
        .duration{.rise = 110ms, .fall = 165ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseInOut }
    };

    // One theme coming over another. The same numbers both ways: a switch is one movement
    // whichever theme is being left, and a window that hurried into the dark and strolled back
    // into the light would read as two different commands. Long enough that the whole window is
    // seen crossing rather than found already changed, short enough to stay an answer to the
    // click that asked for it.
    constexpr auto multiplicator = 1;
    export constexpr AnimationSlot themeSwitch{
        .duration{.rise = multiplicator * 500ms, .fall = multiplicator * 500ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseOut }
    };

    // A THEME'S LIGHTNESS CROSSES ON A SLOT OF ITS OWN, because it is the one axis with no half
    // way: contrast closes as it crosses and is gone in the middle, so the window passes through
    // a flat mid-tone whatever the colours are doing. How long that lasts is its own question,
    // and the answer is shorter than the colours take - the palette settles at leisure once the
    // window is on the side it is going to. A crossing between two themes of one mode never runs
    // this slot at all.
    export constexpr AnimationSlot themeLightness{
        .duration{.rise = multiplicator * 0ms, .fall = multiplicator * 0ms },
        .easingFactor{ EasingFactor::Linear, EasingFactor::Linear }
    };

    // A scroll answers an input directly, so it leaves at once and settles at the end. The same
    // numbers both ways: travelling towards the top of a list and travelling towards the bottom
    // are one movement, and a scroll bar that hurried one way would read as two controls. Long
    // enough that the eye follows the content across the gap instead of finding it in a new
    // place, short enough that a held key still pages at reading speed.
    export constexpr AnimationSlot scroll{
        .duration{.rise = multiplicator * 166ms, .fall = multiplicator * 166ms },
        .easingFactor{ EasingFactor::EaseOut, EasingFactor::EaseOut }
    };

}
