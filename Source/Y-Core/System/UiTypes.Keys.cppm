export module ClaFi.Core.System.UiTypes :Keys;

import ClaFi.StdLib;

namespace ClaFi
{
    export using KeyCode = std::uint32_t;

    export namespace Keys
    {
        constexpr KeyCode BackSpace = 0x08;
        constexpr KeyCode Tab = 0x09;
        constexpr KeyCode Shift = 0x10;
        constexpr KeyCode Ctrl = 0x11;
        constexpr KeyCode Alt = 0x12;
        constexpr KeyCode Return = 0x0d;
        constexpr KeyCode Escape = 0x1b;
        constexpr KeyCode Space = 0x20;
        constexpr KeyCode Prior = 0x21;
        constexpr KeyCode Next = 0x22;
        constexpr KeyCode End = 0x23;
        constexpr KeyCode Home = 0x24;
        constexpr KeyCode Left = 0x25;
        constexpr KeyCode Up = 0x26;
        constexpr KeyCode Right = 0x27;
        constexpr KeyCode Down = 0x28;
        constexpr KeyCode Delete = 0x2e;
        constexpr KeyCode F2 = 0x71;
        constexpr KeyCode F4 = 0x73;
    }
}
