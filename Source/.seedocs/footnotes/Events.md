# Events

The event layer: EventComponent and its dispatcher, the handler properties a constructor pack
carries, and the concepts that hold a handler to one event.

## EventParamOf

It reads a lambda or a functor through its `operator()`, a function pointer and a member function
pointer. `event_param_t<F>` is the event's type, const removed.

## OnEventTag

EventComponent's constructor connects every argument in its pack derived from this tag, by calling
its `connectTo`. OnEvent and Events derive from it, and so may a thin named subclass of either.

## OnEvent

**The parameter type chooses the event**, so one property type connects any event on any emitter:

    OnEvent{ [](ComboBoxChangeEvent& event) { ... } }
    OnEvent{ [this](ChangeEvent& event) { ... } }
    makeOnEvent(this, &Page::stateWanted)      // parameter read off the method

**A generic `[](auto&)` handler is rejected**, since it names no event.

## Events

The handlers go in bare, without an OnEvent around each:

    Button{ params, Events{
        [](ClickEvent& event) { ... },
        [](PaintIconEvent& event) { ... },
    } }

**The order says nothing**, since each handler names its own event. A wrapped handler mixes with
bare ones, for a call site that wants the event named:

    Events{ Button::OnClick{ handler }, [](PaintIconEvent& event) { ... } }
