# Events

The event layer: EventComponent and its dispatcher, the handler properties a constructor pack
carries, and the concepts that hold a handler to one event.

## EventParamOf

The event a handler's one parameter names.

## OnEventTag

A tag base rather than a template-id match, so that a caller can keep a thin named subclass for
compatibility and still have it recognised as a handler by the routing.

## OnEvent

Deduces the event from the handler's own parameter, so one property type connects any event on
any emitter:

    OnEvent{ [](ComboBoxChangeEvent& event) { ... } }
    OnEvent{ [this](ChangeEvent& event) { ... } }
    makeOnEvent(this, &Page::stateWanted)      // parameter read off the method

One property type serves every event, so an emitter needs no named handler type per event it
raises. The event is already written in the handler, and a name for it would only say the same
thing twice. A generic `[](auto&)` handler names no event and is rejected: with OnEvent the
parameter type is how the event to connect to is chosen.

## Events

Several handlers as one property, so an argument list does not repeat OnEvent per handler:

    Button{ params, Events{
        [](ClickEvent& event) { ... },
        [](PaintIconEvent& event) { ... },
    } }

Each handler names its own event, so the list is heterogeneous and its order says nothing. An
already wrapped handler is accepted alongside bare ones, for a call site that wants the event
named: `Events{ Button::OnClick{ handler }, [](PaintIconEvent& event) { ... } }`.
