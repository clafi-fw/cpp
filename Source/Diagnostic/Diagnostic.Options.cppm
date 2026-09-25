export module ClaFi.Diagnostic.Options;

// The switches the diagnostic functions stand behind, stated once and at compile time: what is
// off is not built, and turning one on is a rebuild. Nothing is imported here, so any layer may
// import this - the platform layer included.
namespace ClaFi::Diagnostic::Options
{
    /// @brief What a failed platform API call comes to: nothing, a line in the debugger's output
    /// naming the call site, or a std::system_error thrown with the same words.
    export enum class ApiErrors
    {
        Ignore,
        Log,
        Throw
    };

    /// @brief The global diagnostic switch: turns everything off
    export constexpr bool enabled{ true };

    /// @brief How a failed platform API call is handled - see ApiErrors. Anything but Ignore
    /// raises the Direct2D debug layer on Windows.
    export constexpr ApiErrors apiErrors{ ApiErrors::Ignore };

    /// @brief Every frame a theme crossing makes goes to the Output page: the moment it was
    /// made and what painting it cost. The gaps between the moments are the crossing's own
    /// cadence, so a crossing that drops frames shows the missing time in one column or the
    /// other - a late tick widens the gaps, a slow paint widens the cost - and never in both.
    export constexpr bool logThemeCrossing{ false };

    /// @brief Every wait the swap chain made the thread pay goes to the debugger's output: a
    /// resize of the chain or a present that took a millisecond or more, with what it took. A
    /// present made from a resize step finds room, the step before it having been composed
    /// before this one began - see FormWindow's WM_SIZE - so one that waits there is the pacing
    /// gone wrong.
    export constexpr bool logPresentWait{ false };

    /// @brief Every scroll-into-view request goes to the Output page: the rect asked for,
    /// the client it is measured against, the four overlaps, the glide still in flight and
    /// both bars. A view that walks away from the rect it was told to show has a target that
    /// moves on every line while the rect does not.
    export constexpr bool logScrollIntoView{ false };

    /// @brief Every pass a form's placement makes goes to the Output page: the factor and the
    /// window the pass started from, then what the content asked for, what the placement granted,
    /// whether either axis was refused, and whether the alignment survived. A window that comes
    /// out short shows it as an ask the grant does not match, or as an ask already short on the
    /// first pass - which says the measure was bounded before the placement ran.
    export constexpr bool logFormPlacement{ false };

    /// @brief Every text drawn through the TextEngine is painted over: the box the text took
    /// filled, the bounds it was given outlined. Where a text stands, and how much of its box
    /// it uses, read off the screen. A text the pointer stands on is outlined while it is
    /// hovered, which is the hit zone rather than the box.
    export constexpr bool highlightTextAreas{ false };
}
