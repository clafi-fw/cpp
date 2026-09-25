module;
#include<chrono> // suddenly, "import <chrono>"  compiles, but gives red wiggles
export module ClaFi.Core.System.Animation;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    using namespace std::chrono_literals;

    constexpr std::chrono::milliseconds k_animationTimerInterval{ 0ms };
    constexpr std::size_t k_maxAnimations{ 48 };

    export using AnimationTag = std::size_t;

    export struct TransitionDurations
    {
        const std::chrono::milliseconds rise{ 66ms };
        const std::chrono::milliseconds fall{ 1000ms };
        // Helper to get duration based on direction
        std::chrono::milliseconds get(bool increasing) const { return increasing ? rise : fall; }
        std::chrono::milliseconds get(float currentValue, float endValue) const { return get(currentValue < endValue); }
    };

    // How a value travels between its two ends over the run of an animation.
    export enum class EasingFactor {
        Linear,     // x
        EaseIn,     // x * x
        EaseOut,     // x * (2-x)
        EaseInOut
    };

    export struct EasingFactors
    {
        const EasingFactor rise{ EasingFactor::EaseOut };
        const EasingFactor fall{ EasingFactor::EaseInOut };
        EasingFactor get(bool increasing) const { return increasing ? rise : fall; }
        EasingFactor get(float currentValue, float endValue) const { return get(currentValue < endValue); }
    };

    export struct AnimationSlot
    {
        const TransitionDurations duration{};
        const EasingFactors easingFactor{};
        const AnimationTag tag{};
    };

    export struct AnimateParams
    {
        void* control;
        const AnimationSlot& slot;
        const float value;
        AnimationTag tag() const { return slot.tag; }
    };

    using Clock = std::chrono::steady_clock;
    export using OnAnimate = std::function<void(AnimateParams&)>;
    using OnEndAnimation = std::function<void()>;
    using OnTickEnd = std::function<void()>;

    struct Animation
    {
    public:
        // ~Animation() { beep(); };
        // keys
        void* control;
        const AnimationSlot* slot;
        //
        OnAnimate onAnimate{};
        std::chrono::milliseconds duration;
        EasingFactor easingFactor;
        Clock::time_point startedAt;
        float endValue;
        float startValue;
    public:
        bool vacant() const { return !(control || slot); }
        void release();
    };

    class AnimationCollection : public std::deque<Animation>
    {
    public:
        Animation* find(void* control, const AnimationSlot&);
    };

    // What one owner asks to have done when a tick has stepped every animation.
    struct TickEndListener
    {
        const void* owner;
        OnTickEnd onTickEnd;
    };

    export class AnimationController
    {
    public:
        AnimationController();
        AnimationController(const OnEndAnimation&);
        void stop(const void* control);
        void stop(const void* control, const AnimationSlot&);
        void start(void* control, const AnimationSlot&, float currentValue, float endValue, const OnAnimate&);
        bool isActive();
        // Whether anything at all is running on this slot, whichever control is running it. The
        // question is about the MOVEMENT rather than about who is making it: something waiting for
        // a scroll to settle waits for scrolling, and which bar is scrolling is not its business.
        [[nodiscard]] bool isActive(const AnimationSlot&) const;
        [[nodiscard]] bool ticking() const { return m_ticking; }
        void externalTimerTick();
        // Work more than one animation asks for, done once when the tick has stepped them all.
        void setTickEndListener(const void* owner, const OnTickEnd&);
        void clearTickEndListener(const void* owner);
    protected:
        void animateControl(void* control, const AnimationSlot&, const OnAnimate&, const float value);
    private:
        void timerTick();
        void restartTimer();
        void notifyTickEnd();
    private:
        AnimationCollection m_animations{};
        std::vector<TickEndListener> m_tickEndListeners{};
        UiTimer m_timer;
        const OnEndAnimation m_onEndAnimation;
        // The collection is being walked. Whatever would restructure it - trimming to capacity -
        // waits for the walk to end. Appending and claiming a vacant place are safe at any time,
        // and are how a tick starts an animation.
        bool m_ticking{ false };
    };

    export struct RepeatEvent
    {
        float elapsedSeconds{ 0.0f }; // time since the previous repeat. See UI-Types
        bool stop{};
    };

    export class EventRepeater
    {
    public:
        typedef std::function<void(RepeatEvent&)> EventFunc;
        //
        EventRepeater(const EventFunc&, MilliSeconds = k_defaultRepeatInterval);
        //
        EventRepeater& start();
        void startOrRepeatNow(MilliSeconds interval = k_defaultRepeatInterval);
        void stop();
        void stopAndReset();
        // The pause after the first firing and the interval between the ones after it. Set by a
        // caller repeating on somebody else's terms - a key held down repeats at the rate the
        // display server states, not at this class's defaults. A repeat already running keeps the
        // interval it was armed with until it next re-arms.
        void setIntervals(MilliSeconds start, MilliSeconds repeat);
        // A handler in flight counts as active: the timer it re-arms at the end is the same
        // repeat continuing, and the platform timer is killed before the handler is entered.
        [[nodiscard]] bool active() const { return m_firing || m_timer.isActive(); }
    public:
        static constexpr MilliSeconds k_timerStartInterval = { 500 };
        static constexpr MilliSeconds k_defaultRepeatInterval = { 110 };
    private:
        void doClickAndRepeat(MilliSeconds delay);
        EventFunc m_event;
        MilliSeconds m_startInterval{ k_timerStartInterval };
        MilliSeconds m_repeatInterval;
        Clock::time_point m_lastRepeatAt{};
        UiTimer m_timer;
        // The platform timer is killed before the handler runs, so a stop reaching the timer
        // itself from inside that handler has nothing to kill. These two carry the handler's
        // answer out to the re-arm at the end of doClickAndRepeat.
        bool m_firing{ false };
        bool m_stopped{ false };
    };


    // ------------------------------------------------------------------------


    // Animation

    void Animation::release()
    {
        control = nullptr;
        slot = nullptr;
        onAnimate = nullptr;
    }

    // AnimationCollection

    Animation* AnimationCollection::find(void* control, const AnimationSlot& slot)
    {
        for (AnimationCollection::reverse_iterator it = rbegin(); it != rend(); ++it)
            if (it->control == control && it->slot == &slot)
                return &*it;
        return nullptr;
    }

    // AnimationController

    AnimationController::AnimationController()
        :
        AnimationController{ nullptr }
    {
    }

    AnimationController::AnimationController(const OnEndAnimation& onEndAnimation)
        :
        m_timer{ OnEvent{ [this](TimerEvent&) { timerTick(); } } },
        m_onEndAnimation{ onEndAnimation }
    // Moving this to the class declaration causes red wiggles in every other module
    // that uses AnimationController ("incomplete class AnimationController not allowed").
    // It's something to do with capturing this into a lambda
    {
    }

    void AnimationController::stop(const void* control)
    {
        for (Animation& animation : m_animations)
            if (animation.control == control)
                animation.release();
    }

    // One slot of one control. A control that runs several animations at once - a scroll bar
    // glides its position while its own hover and press factors run - needs one of them cancelled
    // without touching the rest.
    void AnimationController::stop(const void* control, const AnimationSlot& slot)
    {
        for (Animation& animation : m_animations)
            if (animation.control == control && animation.slot == &slot)
                animation.release();
    }

    // Callable from a tick. A tick that moves a control invalidates that control's visual state,
    // and Control::invalidateState starts an animation on every state slot the control has, so an
    // animation that moves things reaches this from inside the walk it is starting in.
    void AnimationController::start(void* control, const AnimationSlot& slot,
                                    float currentValue, float endValue, const OnAnimate& onAnimate)
    {
        Animation* animation = nullptr;
        // Trimming erases, which the walk would not survive, so it waits. What it leaves standing
        // is one tick's worth of animations, and the next start outside a tick takes them.
        while (!m_ticking && m_animations.size() > k_maxAnimations)
        {
            Animation& animationToRemove = *m_animations.begin();
            if (!animationToRemove.vacant())
            {
                if (animationToRemove.control == control && animationToRemove.slot == &slot)
                {
                    animation = &animationToRemove;
                    break;
                }
                animateControl(
                    animationToRemove.control,
                    *animationToRemove.slot,
                    animationToRemove.onAnimate,
                    animationToRemove.endValue
                );
            }
            m_animations.pop_front();
        }
        if (!animation)
            animation = m_animations.find(control, slot);
        // A request naming the end value a running animation is already heading to is that
        // animation, so it is left alone. Stamping it afresh would restart the whole duration
        // from wherever the value has reached, and a control re-invalidated once per tick - a
        // scroll bar's buttons, which controlFeedBack re-invalidates on every frame of a glide -
        // would hold its factor at the start for as long as the thing re-invalidating it runs.
        if (animation && sameFactors(animation->endValue, endValue))
            return;
        if (!animation)
        {
            if (sameFactors(currentValue, endValue))
                return;
            // searching for vacant(zeroed) data
            for (AnimationCollection::reverse_iterator it = m_animations.rbegin(); it != m_animations.rend(); ++it)
                if (it->vacant())
                {
                    animation = &*it;
                    break;
                }
            if (animation == nullptr)
            {
                m_animations.emplace_back();
                animation = &m_animations.back();
            }
            animation->control = control;
            animation->slot = &slot;
            animation->onAnimate = onAnimate;
        }
        animation->startValue = currentValue;
        animation->startedAt = Clock::now();
        bool isGrowing = endValue > currentValue;
        animation->duration = slot.duration.get(isGrowing);
        animation->easingFactor = slot.easingFactor.get(isGrowing);
        //animation->duration = std::chrono::round<std::chrono::milliseconds>(animation->duration * timeFactor);
        animation->endValue = endValue;
        restartTimer();
    }

    // Whether anything is still running, which is not the same question as whether any CONTROL
    // is. An animation that belongs to the application rather than to one control carries no
    // control at all - the input device fade is one - and reading a null control as finished ends
    // the walk one tick in, leaving that animation's value frozen wherever the tick left it.
    // Released is the only thing that means finished, and vacant() is what says released.
    bool AnimationController::isActive()
    {
        for (const Animation& it : m_animations)
            if (!it.vacant())
                return true;
        return false;
    }

    bool AnimationController::isActive(const AnimationSlot& slot) const
    {
        for (const Animation& it : m_animations)
            if (!it.vacant() && it.slot == &slot)
                return true;
        return false;
    }

    void AnimationController::externalTimerTick()
    {
        // A start from inside a tick restarts the timer, so a caller that pumps animations from
        // inside one - a callback that moves a control, which lands in a mouse move, which pumps -
        // would find the timer active and walk the collection a second time on top of the first.
        if (m_ticking)
            return;
        // if it isn't active, there're no animations
        if (m_timer.isActive())
        {
            m_timer.stop();
            timerTick();
        }
    }

    // One listener to an owner, so an owner asking again restates what it wants rather than
    // adding a second call.
    void AnimationController::setTickEndListener(const void* owner, const OnTickEnd& onTickEnd)
    {
        for (TickEndListener& listener : m_tickEndListeners)
        {
            if (listener.owner == owner)
            {
                listener.onTickEnd = onTickEnd;
                return;
            }
        }
        m_tickEndListeners.push_back({ owner, onTickEnd });
    }

    void AnimationController::clearTickEndListener(const void* owner)
    {
        std::erase_if(m_tickEndListeners,
            [owner](const TickEndListener& listener) {
                return listener.owner == owner;
            });
    }

    void AnimationController::animateControl(void* control, const AnimationSlot& slot, const OnAnimate& onAnimate, const float value)
    {
        if (!onAnimate)
            return;
        AnimateParams params{ control, slot, value };
        onAnimate(params);
    }

    // Walked by index, with size() read afresh at each step, because a callback can append to the
    // collection. Appending leaves every index before it standing, so the walk survives it and the
    // animation appended takes its first frame in this same tick. An iterator would not survive it.
    void AnimationController::timerTick()
    {
        m_ticking = true;
        for (std::size_t i = 0; i < m_animations.size(); ++i)
        {
            Animation& animation = m_animations[i];
            if (animation.vacant())
                continue;
            std::chrono::milliseconds d = duration_cast<std::chrono::milliseconds>(Clock::now() - animation.startedAt);
            if (!d.count())
                continue;

            if (d < animation.duration)
            {
                float x = std::chrono::duration<float>(d) / std::chrono::duration<float>(animation.duration);

                switch (animation.easingFactor)
                {
                case EasingFactor::Linear:
                    break;
                case EasingFactor::EaseIn:
                    x = x * x;
                    break;
                case EasingFactor::EaseOut:
                    x = x * (2.0f - x);
                    break;
                case EasingFactor::EaseInOut:
                    {
                        if (x < 0.5f)
                            x = 2.0f * x * x;
                        else
                            x = (4.0f - 2.0f * x) * x - 1.0f;
                        break;
                    }
                }

                float newValue = animation.startValue + (animation.endValue - animation.startValue) * x;
                animateControl(animation.control, *animation.slot, animation.onAnimate, newValue);
                continue;
            }

            Clock::time_point startedAt = animation.startedAt;
            animateControl(animation.control, *animation.slot, animation.onAnimate, animation.endValue);
            // A callback handed the last value can start this same animation again, and starting
            // it stamps it afresh. Releasing then would throw that start away.
            if (animation.startedAt == startedAt)
                animation.release();
        }
        m_ticking = false;
        // Ahead of the re-arm below: a listener repaints, a repaint pumps externalTimerTick, and
        // an armed timer would let that pump walk the collection a second time. A listener that
        // starts an animation is caught by the isActive that follows.
        notifyTickEnd();

        // Whatever is left running keeps the timer, including anything a callback started during
        // the walk.
        if (isActive())
        {
            restartTimer();
            return;
        }
        m_animations.clear();
        if (m_onEndAnimation)
            m_onEndAnimation();
    }

    void AnimationController::restartTimer()
    {
        m_timer.start(MilliSeconds{ static_cast<std::uint32_t>(k_animationTimerInterval.count()) });
    }

    void AnimationController::notifyTickEnd()
    {
        for (const TickEndListener& listener : m_tickEndListeners)
            listener.onTickEnd();
    }

    // EventRepeater

    EventRepeater::EventRepeater(const EventFunc& event, MilliSeconds repeatInterval)
        :
        m_event{ event },
        m_repeatInterval{ repeatInterval },
        m_timer{ OnEvent{ [this](TimerEvent&) { doClickAndRepeat(m_repeatInterval); } } }
    {
    }

    EventRepeater& EventRepeater::start()
    {
        m_stopped = false;
        m_lastRepeatAt = Clock::now();
        doClickAndRepeat(m_startInterval);
        return *this;
    }

    // The ask itself carries the repeat, from the given interval on, without the pause a press
    // waits out first. A repeat that moves by elapsed time covers the same ground however often
    // it is asked for, so a caller that renews the ask for as long as its condition holds - a
    // pointer dragging near an edge - drives the movement directly, and the timer is what keeps
    // it going once the asks stop. That is what a WM_TIMER alone cannot do: it is the lowest
    // priority message there is, so a pointer streaming moves and the paints they cause hold it
    // off for as long as the pointer keeps moving.
    void EventRepeater::startOrRepeatNow(MilliSeconds interval)
    {
        // An ask arriving from inside the handler is that same repeat already running, and
        // running it from within itself would repeat by recursion.
        m_stopped = false;
        if (m_firing)
            return;
        // A repeat already on the timer keeps the interval and the stamp it is measured from, so
        // this ask covers the time since that one rather than starting the measurement over.
        if (!m_timer.isActive())
        {
            m_repeatInterval = interval;
            m_lastRepeatAt = Clock::now();
        }
        doClickAndRepeat(m_repeatInterval);
    }

    void EventRepeater::stop()
    {
        m_stopped = true;
        m_timer.stop();
    }

    void EventRepeater::stopAndReset()
    {
        stop();
        m_startInterval = k_timerStartInterval;
        m_repeatInterval = k_defaultRepeatInterval;
    }

    void EventRepeater::setIntervals(MilliSeconds start, MilliSeconds repeat)
    {
        m_startInterval = start;
        m_repeatInterval = repeat;
    }

    void EventRepeater::doClickAndRepeat(MilliSeconds delay)
    {
        Clock::time_point now = Clock::now();
        RepeatEvent event{
            .elapsedSeconds = std::chrono::duration<float>(now - m_lastRepeatAt).count(),
        };
        m_lastRepeatAt = now;
        m_firing = true;
        m_event(event);
        m_firing = false;
        // A stop the handler asked for holds. The next repeat is the one thing a stopped
        // repeater must not do, and the timer it would stand on is armed right here.
        if (event.stop || m_stopped)
            return;
        m_timer.start(delay);
    }

}
