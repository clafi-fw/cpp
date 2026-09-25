module;
#include "Wayland.Headers.h"
export module ClaFi.Platform.Wayland.Keyboard;

import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Wayland
{
    // One press as the framework hears it: the key, the modifiers, the character. See Platform
    export struct KeyPress
    {
        KeyCode key{ 0 };
        KeyModifiers modifiers{};
        // Zero for a press that types nothing - an arrow, a modifier, a dead key waiting for the
        // letter it will accent.
        wchar_t character{ 0 };
        bool isRepeat{ false };
        InputStamp stamp{};
    };

    // Win32 virtual-key codes for the keys that have no Keys:: name of their own. Named here
    // rather than as bare numbers at the mapping, so the mapping reads as what it maps to, and
    // exported so the spelling of a shortcut reads the same names.
    export namespace VirtualKey
    {
        constexpr KeyCode clear = 0x0c;
        constexpr KeyCode pause = 0x13;
        constexpr KeyCode capsLock = 0x14;
        constexpr KeyCode print = 0x2c;
        constexpr KeyCode insert = 0x2d;
        constexpr KeyCode help = 0x2f;
        constexpr KeyCode leftWindows = 0x5b;
        constexpr KeyCode rightWindows = 0x5c;
        constexpr KeyCode applications = 0x5d;
        constexpr KeyCode numpad0 = 0x60;
        constexpr KeyCode multiply = 0x6a;
        constexpr KeyCode add = 0x6b;
        constexpr KeyCode separator = 0x6c;
        constexpr KeyCode subtract = 0x6d;
        constexpr KeyCode decimal = 0x6e;
        constexpr KeyCode divide = 0x6f;
        constexpr KeyCode f1 = 0x70;
        constexpr KeyCode numLock = 0x90;
        constexpr KeyCode scrollLock = 0x91;
        // The punctuation keys, named by their position on a US keyboard as Win32 names them.
        constexpr KeyCode oem1 = 0xba;          // ;:
        constexpr KeyCode oemPlus = 0xbb;       // =+
        constexpr KeyCode oemComma = 0xbc;      // ,<
        constexpr KeyCode oemMinus = 0xbd;      // -_
        constexpr KeyCode oemPeriod = 0xbe;     // .>
        constexpr KeyCode oem2 = 0xbf;          // /?
        constexpr KeyCode oem3 = 0xc0;          // `~
        constexpr KeyCode oem4 = 0xdb;          // [{
        constexpr KeyCode oem5 = 0xdc;          // \|
        constexpr KeyCode oem6 = 0xdd;          // ]}
        constexpr KeyCode oem7 = 0xde;          // '"
        constexpr KeyCode oem102 = 0xe2;        // the key beside the left Shift on a 102-key board
    }

    // The keyboard as xkbcommon describes it, and nothing of the protocol. See Platform
    export class Keyboard
    {
    public:
        using PressHandler = std::function<void(const KeyPress&)>;
        explicit Keyboard(const PressHandler&);
        ~Keyboard();
        Keyboard(const Keyboard&) = delete;
        Keyboard& operator=(const Keyboard&) = delete;
        // wl_keyboard.keymap. The descriptor is mapped, compiled and closed here whatever the
        // outcome. A format other than XKB_V1 leaves the keyboard without a keymap, and a
        // keyboard without a keymap types nothing.
        void setKeymap(std::uint32_t format, int fd, std::uint32_t size);
        // wl_keyboard.modifiers, in the masks and group the protocol states them in.
        void setModifiers(std::uint32_t depressed, std::uint32_t latched, std::uint32_t locked,
            std::uint32_t group);
        // wl_keyboard.repeat_info. A rate of zero disables the repeat.
        void setRepeatInfo(std::int32_t rate, std::int32_t delay);
        // wl_keyboard.key, with the code as the protocol states it - an evdev code, which is the
        // xkb keycode less eight. A press is delivered through the handler, at once and then on
        // every repeat; a release stops the repeat if it is that key's.
        void keyDown(std::uint32_t evdevCode, InputStamp);
        void keyUp(std::uint32_t evdevCode);
        // wl_keyboard.leave. The repeat stops - the release will go to whoever has the keyboard
        // now - and the modifiers are forgotten, because nothing will say when they change.
        void focusLost();
        // The modifiers held as of the last modifiers event. Meaningful only while a surface of
        // this client has the keyboard: a client without it is told nothing.
        [[nodiscard]] KeyModifiers modifiers() const { return m_modifiers; }
    private:
        [[nodiscard]] KeyPress translate(xkb_keycode_t, bool isRepeat, InputStamp);
        [[nodiscard]] KeyCode keyCodeFor(xkb_keycode_t, xkb_keysym_t) const;
        [[nodiscard]] wchar_t characterFor(xkb_keycode_t, xkb_keysym_t, KeyCode);
        void repeat(RepeatEvent&);
        void readModifiers();
        void dropKeymap();
    private:
        PressHandler m_onPress;
        xkb_context* m_context{ nullptr };
        xkb_keymap* m_keymap{ nullptr };
        xkb_state* m_state{ nullptr };
        // Dead keys. Null where the locale has no compose table, in which case a dead key types
        // nothing and accents nothing.
        xkb_compose_table* m_composeTable{ nullptr };
        xkb_compose_state* m_composeState{ nullptr };
        KeyModifiers m_modifiers{};
        // The key being repeated and the stamp of the press that started it. Every repeat carries
        // the serial of the one press the user actually made, that being the only event the
        // compositor put a number on.
        xkb_keycode_t m_repeatKey{ XKB_KEYCODE_INVALID };
        InputStamp m_repeatStamp{};
        // Whether the repeater has fired for this key yet. Its first firing IS the press; the
        // ones after it are the repeats.
        bool m_repeatFired{ false };
        bool m_repeatEnabled{ true };
        EventRepeater m_repeater;
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::PlatformImplementation::Wayland
{
    // The keyboard character type has to hold a code point whole. It does on every Linux ABI, and
    // xkbcommon answers in code points.
    static_assert(sizeof(wchar_t) == 4);

    // An xkb keycode is the evdev code plus this, by the X convention xkbcommon keeps.
    constexpr std::uint32_t k_evdevOffset = 8;

    // THE KEYS THAT ARE NAMED RATHER THAN PRINTED, by the keysym the state answers for them. That
    // keysym is the one WITH the modifiers, and deliberately: the keypad's 7 is Home without Num
    // Lock and a digit with it, and Win32 names each press by what it is at that moment.
    [[nodiscard]] KeyCode namedKey(xkb_keysym_t keysym)
    {
        if (keysym >= XKB_KEY_KP_0 && keysym <= XKB_KEY_KP_9)
            return VirtualKey::numpad0 + (keysym - XKB_KEY_KP_0);
        if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F24)
            return VirtualKey::f1 + (keysym - XKB_KEY_F1);

        switch (keysym)
        {
        case XKB_KEY_BackSpace:
            return Keys::BackSpace;
        case XKB_KEY_Tab:
        case XKB_KEY_ISO_Left_Tab:
        case XKB_KEY_KP_Tab:
            return Keys::Tab;
        case XKB_KEY_Clear:
        case XKB_KEY_KP_Begin:
            return VirtualKey::clear;
        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
            return Keys::Return;
        case XKB_KEY_Pause:
        case XKB_KEY_Break:
            return VirtualKey::pause;
        case XKB_KEY_Scroll_Lock:
            return VirtualKey::scrollLock;
        case XKB_KEY_Escape:
            return Keys::Escape;
        case XKB_KEY_space:
        case XKB_KEY_KP_Space:
            return Keys::Space;
        case XKB_KEY_Page_Up:
        case XKB_KEY_KP_Page_Up:
            return Keys::Prior;
        case XKB_KEY_Page_Down:
        case XKB_KEY_KP_Page_Down:
            return Keys::Next;
        case XKB_KEY_End:
        case XKB_KEY_KP_End:
            return Keys::End;
        case XKB_KEY_Home:
        case XKB_KEY_KP_Home:
            return Keys::Home;
        case XKB_KEY_Left:
        case XKB_KEY_KP_Left:
            return Keys::Left;
        case XKB_KEY_Up:
        case XKB_KEY_KP_Up:
            return Keys::Up;
        case XKB_KEY_Right:
        case XKB_KEY_KP_Right:
            return Keys::Right;
        case XKB_KEY_Down:
        case XKB_KEY_KP_Down:
            return Keys::Down;
        case XKB_KEY_Print:
        case XKB_KEY_Sys_Req:
            return VirtualKey::print;
        case XKB_KEY_Insert:
        case XKB_KEY_KP_Insert:
            return VirtualKey::insert;
        case XKB_KEY_Delete:
        case XKB_KEY_KP_Delete:
            return Keys::Delete;
        case XKB_KEY_Help:
            return VirtualKey::help;
        case XKB_KEY_Super_L:
            return VirtualKey::leftWindows;
        case XKB_KEY_Super_R:
            return VirtualKey::rightWindows;
        case XKB_KEY_Menu:
            return VirtualKey::applications;
        case XKB_KEY_KP_Multiply:
            return VirtualKey::multiply;
        case XKB_KEY_KP_Add:
            return VirtualKey::add;
        case XKB_KEY_KP_Separator:
            return VirtualKey::separator;
        case XKB_KEY_KP_Subtract:
            return VirtualKey::subtract;
        case XKB_KEY_KP_Decimal:
            return VirtualKey::decimal;
        case XKB_KEY_KP_Divide:
            return VirtualKey::divide;
        case XKB_KEY_Num_Lock:
            return VirtualKey::numLock;
        case XKB_KEY_Caps_Lock:
            return VirtualKey::capsLock;
        case XKB_KEY_Shift_L:
        case XKB_KEY_Shift_R:
            return Keys::Shift;
        case XKB_KEY_Control_L:
        case XKB_KEY_Control_R:
            return Keys::Ctrl;
        case XKB_KEY_Alt_L:
        case XKB_KEY_Alt_R:
        case XKB_KEY_Meta_L:
        case XKB_KEY_Meta_R:
        case XKB_KEY_ISO_Level3_Shift:
            return Keys::Alt;
        default:
            return 0;
        }
    }

    // The virtual-key code of a Latin letter or a digit, which Win32 defines as the character
    // itself, upper case. Zero for any other keysym.
    [[nodiscard]] KeyCode latinKey(xkb_keysym_t keysym)
    {
        if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z)
            return static_cast<KeyCode>(keysym - XKB_KEY_a + 'A');
        if (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z)
            return static_cast<KeyCode>(keysym);
        if (keysym >= XKB_KEY_0 && keysym <= XKB_KEY_9)
            return static_cast<KeyCode>(keysym);
        return 0;
    }

    // WHAT IS PRINTED ON THE KEY in one layout: its first level, which is the key with nothing
    // held. Shift and AltGr change what the key types and not what it is, so the level with them
    // is never read here.
    [[nodiscard]] KeyCode printedKey(xkb_keymap* keymap, xkb_keycode_t keycode,
        xkb_layout_index_t layout)
    {
        const xkb_keysym_t* keysyms = nullptr;
        const int count = ::xkb_keymap_key_get_syms_by_level(keymap, keycode, layout, 0, &keysyms);
        for (int i = 0; i < count; ++i)
        {
            if (const KeyCode latin = latinKey(keysyms[i]))
                return latin;
            if (const KeyCode named = namedKey(keysyms[i]))
                return named;
        }
        return 0;
    }

    // THE KEY BY WHERE IT IS, read as a US keyboard, which is what Win32's OEM codes name. The
    // last resort, for a key no layout of the keymap puts a Latin letter on.
    [[nodiscard]] KeyCode positionKey(std::uint32_t evdevCode)
    {
        if (evdevCode >= KEY_1 && evdevCode <= KEY_9)
            return static_cast<KeyCode>('1' + (evdevCode - KEY_1));

        switch (evdevCode)
        {
        case KEY_0:
            return '0';
        case KEY_MINUS:
            return VirtualKey::oemMinus;
        case KEY_EQUAL:
            return VirtualKey::oemPlus;
        case KEY_Q:
            return 'Q';
        case KEY_W:
            return 'W';
        case KEY_E:
            return 'E';
        case KEY_R:
            return 'R';
        case KEY_T:
            return 'T';
        case KEY_Y:
            return 'Y';
        case KEY_U:
            return 'U';
        case KEY_I:
            return 'I';
        case KEY_O:
            return 'O';
        case KEY_P:
            return 'P';
        case KEY_LEFTBRACE:
            return VirtualKey::oem4;
        case KEY_RIGHTBRACE:
            return VirtualKey::oem6;
        case KEY_A:
            return 'A';
        case KEY_S:
            return 'S';
        case KEY_D:
            return 'D';
        case KEY_F:
            return 'F';
        case KEY_G:
            return 'G';
        case KEY_H:
            return 'H';
        case KEY_J:
            return 'J';
        case KEY_K:
            return 'K';
        case KEY_L:
            return 'L';
        case KEY_SEMICOLON:
            return VirtualKey::oem1;
        case KEY_APOSTROPHE:
            return VirtualKey::oem7;
        case KEY_GRAVE:
            return VirtualKey::oem3;
        case KEY_BACKSLASH:
            return VirtualKey::oem5;
        case KEY_Z:
            return 'Z';
        case KEY_X:
            return 'X';
        case KEY_C:
            return 'C';
        case KEY_V:
            return 'V';
        case KEY_B:
            return 'B';
        case KEY_N:
            return 'N';
        case KEY_M:
            return 'M';
        case KEY_COMMA:
            return VirtualKey::oemComma;
        case KEY_DOT:
            return VirtualKey::oemPeriod;
        case KEY_SLASH:
            return VirtualKey::oem2;
        case KEY_102ND:
            return VirtualKey::oem102;
        default:
            return 0;
        }
    }

    // The locale the compose table is read for, as the C library would settle it: LC_ALL over
    // LC_CTYPE over LANG, and "C" when none is set.
    [[nodiscard]] const char* composeLocale()
    {
        for (const char* name : { "LC_ALL", "LC_CTYPE", "LANG" })
            if (const char* value = std::getenv(name))
                if (*value)
                    return value;
        return "C";
    }

    // The first code point of a UTF-8 string, or zero for an empty one. A compose sequence answers
    // a string, and one that composes to several characters delivers its first.
    [[nodiscard]] wchar_t firstCodePoint(const char* utf8)
    {
        const unsigned char lead = static_cast<unsigned char>(utf8[0]);
        if (lead < 0x80)
            return static_cast<wchar_t>(lead);

        int continuation = 0;
        std::uint32_t codePoint = 0;
        if ((lead & 0xe0) == 0xc0)
        {
            continuation = 1;
            codePoint = lead & 0x1f;
        }
        else if ((lead & 0xf0) == 0xe0)
        {
            continuation = 2;
            codePoint = lead & 0x0f;
        }
        else if ((lead & 0xf8) == 0xf0)
        {
            continuation = 3;
            codePoint = lead & 0x07;
        }
        else
            return 0;

        for (int i = 1; i <= continuation; ++i)
        {
            const unsigned char byte = static_cast<unsigned char>(utf8[i]);
            if ((byte & 0xc0) != 0x80)
                return 0;
            codePoint = (codePoint << 6) | (byte & 0x3f);
        }
        return static_cast<wchar_t>(codePoint);
    }


    //-------------------------------------------------------------------------


    Keyboard::Keyboard(const PressHandler& onPress)
        :
        m_onPress{ onPress },
        m_repeater{ [this](RepeatEvent& event) {
            repeat(event);
        } }
    {
        m_context = ::xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!m_context)
            return;

        m_composeTable = ::xkb_compose_table_new_from_locale(m_context, composeLocale(),
            XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (m_composeTable)
            m_composeState = ::xkb_compose_state_new(m_composeTable, XKB_COMPOSE_STATE_NO_FLAGS);
    }

    Keyboard::~Keyboard()
    {
        dropKeymap();
        if (m_composeState)
            ::xkb_compose_state_unref(m_composeState);
        if (m_composeTable)
            ::xkb_compose_table_unref(m_composeTable);
        if (m_context)
            ::xkb_context_unref(m_context);
    }

    void Keyboard::setKeymap(std::uint32_t format, int fd, std::uint32_t size)
    {
        dropKeymap();

        // The compositor writes the keymap as a NUL-terminated string and states the size with
        // the terminator, so the mapping is a string as it stands.
        if (m_context && format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && size > 0)
        {
            void* mapped = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (Linux::check(mapped != MAP_FAILED))
            {
                m_keymap = ::xkb_keymap_new_from_string(m_context, static_cast<const char*>(mapped),
                    XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
                ::munmap(mapped, size);
            }
        }
        ::close(fd);

        if (m_keymap)
            m_state = ::xkb_state_new(m_keymap);
    }

    // THE MASKS ARE TAKEN AS STATED AND NEVER WORKED OUT FROM THE KEYS. The compositor is the one
    // that knows which modifiers are held - a Shift pressed before this window had the keyboard
    // was never seen here as a key - so the state is updated from its word and a key event never
    // touches it.
    void Keyboard::setModifiers(std::uint32_t depressed, std::uint32_t latched, std::uint32_t locked,
        std::uint32_t group)
    {
        if (!m_state)
            return;

        ::xkb_state_update_mask(m_state, depressed, latched, locked, 0, 0, group);
        readModifiers();
    }

    void Keyboard::setRepeatInfo(std::int32_t rate, std::int32_t delay)
    {
        // The rate is presses per second; the repeater wants the time between two of them.
        m_repeatEnabled = rate > 0 && delay >= 0;
        if (m_repeatEnabled)
            m_repeater.setIntervals(
                MilliSeconds{ static_cast<std::uint32_t>(delay) },
                MilliSeconds{ static_cast<std::uint32_t>(std::max(1, 1000 / rate)) });
    }

    void Keyboard::keyDown(std::uint32_t evdevCode, InputStamp stamp)
    {
        if (!m_state)
            return;

        const xkb_keycode_t keycode = evdevCode + k_evdevOffset;

        // A KEY THAT DOES NOT REPEAT IS DELIVERED AND FORGOTTEN, and leaves a repeat in flight
        // alone: Shift pressed while a letter is held does not end the letter, it capitalises
        // the repeats that follow. Any other key takes the repeat over, which is what a second
        // key does under every display server.
        if (!m_repeatEnabled || !::xkb_keymap_key_repeats(m_keymap, keycode))
        {
            m_onPress(translate(keycode, false, stamp));
            return;
        }

        m_repeatKey = keycode;
        m_repeatStamp = stamp;
        m_repeatFired = false;
        // Fires at once - that firing is the press - and again after the delay, then at the rate.
        m_repeater.start();
    }

    void Keyboard::keyUp(std::uint32_t evdevCode)
    {
        if (evdevCode + k_evdevOffset != m_repeatKey)
            return;

        m_repeater.stop();
        m_repeatKey = XKB_KEYCODE_INVALID;
    }

    void Keyboard::focusLost()
    {
        m_repeater.stop();
        m_repeatKey = XKB_KEYCODE_INVALID;
        m_modifiers = {};
        if (m_state)
            ::xkb_state_update_mask(m_state, 0, 0, 0, 0, 0, 0);
        if (m_composeState)
            ::xkb_compose_state_reset(m_composeState);
    }

    KeyPress Keyboard::translate(xkb_keycode_t keycode, bool isRepeat, InputStamp stamp)
    {
        const xkb_keysym_t keysym = ::xkb_state_key_get_one_sym(m_state, keycode);
        const KeyCode key = keyCodeFor(keycode, keysym);
        return {
            .key = key,
            .modifiers = m_modifiers,
            .character = characterFor(keycode, keysym, key),
            .isRepeat = isRepeat,
            .stamp = stamp,
        };
    }

    KeyCode Keyboard::keyCodeFor(xkb_keycode_t keycode, xkb_keysym_t keysym) const
    {
        // What the key means with the modifiers held, for the keys whose meaning changes with
        // them - see namedKey.
        if (const KeyCode named = namedKey(keysym))
            return named;

        // What is printed on it in the active layout.
        const xkb_layout_index_t layout = ::xkb_state_key_get_layout(m_state, keycode);
        if (const KeyCode printed = printedKey(m_keymap, keycode, layout))
            return printed;

        // What is printed on it in every other layout. A Cyrillic layout stands beside a Latin
        // one, and the Latin letter on the same key is the one a shortcut was written against.
        const xkb_layout_index_t layouts = ::xkb_keymap_num_layouts_for_key(m_keymap, keycode);
        for (xkb_layout_index_t other = 0; other < layouts; ++other)
            if (other != layout)
                if (const KeyCode printed = printedKey(m_keymap, keycode, other))
                    return printed;

        return positionKey(keycode - k_evdevOffset);
    }

    wchar_t Keyboard::characterFor(xkb_keycode_t keycode, xkb_keysym_t keysym, KeyCode key)
    {
        // ALT TYPES NOTHING. Win32 turns a letter pressed with Alt into WM_SYSCHAR, which no
        // window of the framework reads as text, and a menu accelerator that also typed its letter
        // would be one no text box could stand in front of.
        if (m_modifiers.alt)
            return 0;

        // CTRL TYPES A CONTROL CODE OR NOTHING. WM_CHAR delivers Ctrl+C as 0x03 whatever the
        // layout, and the framework's text boxes count on it: the shortcut takes the key and the
        // character that follows is unprintable. Read from the layout instead, Ctrl+C under a
        // Cyrillic layout is the letter es, which a text box would happily type after the copy.
        if (m_modifiers.ctrl)
        {
            if (key >= 'A' && key <= 'Z')
                return static_cast<wchar_t>(key - 'A' + 1);
            const std::uint32_t codePoint = ::xkb_state_key_get_utf32(m_state, keycode);
            const bool control = codePoint <= Keys::Space || codePoint == 0x7f;
            return control ? static_cast<wchar_t>(codePoint) : 0;
        }

        // THE DEAD KEYS. A keysym fed to the compose state either starts or continues a sequence,
        // which types nothing yet; completes one, which types the composed character; breaks one,
        // which drops the sequence and types the key itself; or is no part of any, and types
        // itself.
        if (m_composeState
            && ::xkb_compose_state_feed(m_composeState, keysym) == XKB_COMPOSE_FEED_ACCEPTED)
        {
            switch (::xkb_compose_state_get_status(m_composeState))
            {
            case XKB_COMPOSE_COMPOSING:
                return 0;
            case XKB_COMPOSE_COMPOSED:
            {
                std::array<char, 16> composed{};
                ::xkb_compose_state_get_utf8(m_composeState, composed.data(), composed.size());
                ::xkb_compose_state_reset(m_composeState);
                return firstCodePoint(composed.data());
            }
            case XKB_COMPOSE_CANCELLED:
            case XKB_COMPOSE_NOTHING:
                break;
            }
        }

        // Return, Tab, Backspace and Escape come out of this as the control codes WM_CHAR
        // delivers for them, because that is what their keysyms encode to. Delete encodes to
        // DEL and is dropped: WM_CHAR delivers it for Ctrl+Backspace and never for the Delete
        // key, and a text box would read it as a character to insert.
        const std::uint32_t codePoint = ::xkb_state_key_get_utf32(m_state, keycode);
        return codePoint == 0x7f ? 0 : static_cast<wchar_t>(codePoint);
    }

    void Keyboard::repeat(RepeatEvent& event)
    {
        if (!m_state || m_repeatKey == XKB_KEYCODE_INVALID)
        {
            event.stop = true;
            return;
        }

        const bool isRepeat = m_repeatFired;
        m_repeatFired = true;

        // TRANSLATED AGAIN ON EVERY REPEAT, against the modifiers held now. A letter held while
        // Shift goes down starts repeating its capital, which is what the keyboard on the desk
        // does.
        const KeyPress press = translate(m_repeatKey, isRepeat, m_repeatStamp);

        // A DEAD KEY DOES NOT REPEAT. Held, it is one accent waiting for one letter; fed to the
        // compose state again it would cancel itself.
        if (m_composeState
            && ::xkb_compose_state_get_status(m_composeState) == XKB_COMPOSE_COMPOSING)
            event.stop = true;

        m_onPress(press);
    }

    void Keyboard::readModifiers()
    {
        // Effective: held, latched or locked alike. A locked Shift capitalises as a held one does.
        const auto active = [this](const char* modifier) {
            return ::xkb_state_mod_name_is_active(m_state, modifier, XKB_STATE_MODS_EFFECTIVE) > 0;
        };
        m_modifiers = {
            .shift = active(XKB_MOD_NAME_SHIFT),
            .ctrl = active(XKB_MOD_NAME_CTRL),
            .alt = active(XKB_MOD_NAME_ALT),
        };
    }

    void Keyboard::dropKeymap()
    {
        m_repeater.stop();
        m_repeatKey = XKB_KEYCODE_INVALID;
        if (m_state)
            ::xkb_state_unref(m_state);
        if (m_keymap)
            ::xkb_keymap_unref(m_keymap);
        m_state = nullptr;
        m_keymap = nullptr;
        m_modifiers = {};
    }
}
