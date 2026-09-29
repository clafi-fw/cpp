export module ClaFi.Core.Foundation :ContextMessage;

import :Control;

import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    // Something said about a control without stopping the user. See Control-Foundation
    export class ContextMessage
    {
    public:
        /// Raises a message about a control and puts it on screen at once.
        static void show(Control& about, Text);
        /// The control the standing message is about, or nothing.
        [[nodiscard]] static Control* control() { return s_control; }
        /// Whether the message went up too recently for a hover change to be the pointer moving
        /// on. A window going down under a still pointer hands the one below it a move, and a
        /// message a dialog's answer raised is up before that move arrives.
        [[nodiscard]] static bool justRaised();
        /// Whether a message stands about this control, and if so what it says and where it goes.
        /// Asked at each place the tooltip asks a control what it has to say, BEFORE the control
        /// is asked: a message stands in front of whatever its control would answer, and this is
        /// what keeps every nestedGetTooltip in the tree from having to know that.
        [[nodiscard]] static bool answer(const Control&, GetTooltipEvent&);
        /// Drops the message. The tooltip machinery calls this at each point the window goes down
        /// or moves on, which is the whole of a message's life; nothing else has to.
        static void forget();
    private:
        using Clock = std::chrono::steady_clock;
        static constexpr std::chrono::milliseconds k_movesIgnoredFor{ 200 };
        // NOT INLINE. An inline variable with a destructor is put into the module's initializer
        // by clang 22, and the optimizer then reads the global after the definition replaced it
        // (llvm/llvm-project#170099, fixed on main in March 2026 and not in 22.1.8). Defined in
        // ContextMessage.cpp.
        static Control* s_control;
        static Text s_text;
        static Clock::time_point s_raisedAt;
    };

}
