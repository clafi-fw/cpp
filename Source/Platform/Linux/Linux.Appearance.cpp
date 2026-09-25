module;
#include <dbus/dbus.h>
module ClaFi.Platform.Linux.Appearance;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    namespace
    {
        // WHERE THE DESKTOP'S OWN SETTINGS ARE SERVED: the Settings interface of the XDG portal.
        namespace SettingsPortal
        {
            constexpr const char* busName = "org.freedesktop.portal.Desktop";
            constexpr const char* objectPath = "/org/freedesktop/portal/desktop";
            constexpr const char* interfaceName = "org.freedesktop.portal.Settings";
            constexpr const char* changeSignal = "SettingChanged";
            // ReadOne answers in one variant; Read, all an older portal has, answers in two.
            constexpr const char* readOneMethod = "ReadOne";
            constexpr const char* readMethod = "Read";
        }

        // The one value read. 1 asks for dark; 0 is no preference, 2 light, anything later none.
        namespace ColorScheme
        {
            constexpr const char* settingsNamespace = "org.freedesktop.appearance";
            constexpr const char* key = "color-scheme";
            constexpr dbus_uint32_t preferDark = 1;
        }

        // THE BUS FILTERS THE SIGNALS, so a change of the colour scheme is the one delivered.
        constexpr const char* k_changeRule =
            "type='signal',"
            "interface='org.freedesktop.portal.Settings',"
            "member='SettingChanged',"
            "path='/org/freedesktop/portal/desktop',"
            "arg0='org.freedesktop.appearance',"
            "arg1='color-scheme'";

        // How long the first ask waits for the portal before the application goes on without it.
        constexpr std::chrono::milliseconds k_firstAnswerWait{ 1000 };

        // The one connection the desktop is read through, and what it has answered so far.
        class Portal
        {
        public:
            Portal() = default;
            ~Portal();
            Portal(const Portal&) = delete;
            Portal& operator=(const Portal&) = delete;
        public:
            [[nodiscard]] ColorMode colorMode();
            void setHandler(AppearanceHandler value) { m_handler = value; }
            [[nodiscard]] int fd() const { return m_fd; }
            void dispatchPending();
        private:
            void connect();
            void closeConnection();
            void requestScheme(const char* method);
            bool takeQueued();
            bool take(DBusMessage*);
            bool takeSignal(DBusMessage*);
            bool takeScheme(DBusMessageIter& value);
        private:
            DBusConnection* m_connection{ nullptr };
            int m_fd{ -1 };
            bool m_connectTried{ false };
            // The read still out: the serial its answer will carry, and the method it went as.
            dbus_uint32_t m_readSerial{ 0 };
            std::string_view m_readMethod{};
            ColorMode m_colorMode{ ColorMode::Light };
            AppearanceHandler m_handler{ nullptr };
        };

        [[nodiscard]] Portal& portal();
    }
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Linux
{
    ColorMode desktopColorMode()
    {
        return portal().colorMode();
    }

    void setAppearanceHandler(const AppearanceHandler handler)
    {
        portal().setHandler(handler);
    }

    int appearanceFd()
    {
        return portal().fd();
    }

    void dispatchAppearance()
    {
        portal().dispatchPending();
    }

    namespace
    {
        // Portal

        Portal& portal()
        {
            static Portal instance{};
            return instance;
        }

        Portal::~Portal()
        {
            closeConnection();
        }

        // THE FIRST ASK CONNECTS AND READS, and every later one is answered from what the bus
        // has said since.
        ColorMode Portal::colorMode()
        {
            if (!m_connectTried)
                connect();
            return m_colorMode;
        }

        // What the loop calls when the socket stirs, a hang-up included.
        void Portal::dispatchPending()
        {
            if (!m_connection)
                return;
            if (!dbus_connection_read_write(m_connection, 0))
            {
                closeConnection();
                return;
            }
            if (takeQueued() && m_handler)
                m_handler();
        }

        // A PRIVATE CONNECTION, so its lifetime is this reader's and nothing else in the process
        // can close it. Every way this can fail leaves the mode at Light: a desktop without a
        // session bus or a portal states no preference.
        void Portal::connect()
        {
            m_connectTried = true;
            DBusError error{};
            dbus_error_init(&error);
            m_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
            dbus_error_free(&error);
            if (!m_connection)
                return;
            // A bus that goes away ends this reader and not the application.
            dbus_connection_set_exit_on_disconnect(m_connection, FALSE);
            if (!dbus_connection_get_unix_fd(m_connection, &m_fd))
                m_fd = -1;
            // Given no error to fill, this does not wait for the bus; the read's flush sends it.
            dbus_bus_add_match(m_connection, k_changeRule, nullptr);
            requestScheme(SettingsPortal::readOneMethod);

            // THE FIRST ANSWER IS WAITED FOR, so the first window opens in the desktop's mode
            // rather than crossing to it. A portal slower than this - one the session starts for
            // this application - is answered from the loop when it comes.
            const std::chrono::steady_clock::time_point deadline =
                std::chrono::steady_clock::now() + k_firstAnswerWait;
            while (m_connection && m_readSerial != 0)
            {
                const std::chrono::milliseconds left =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        deadline - std::chrono::steady_clock::now());
                if (left.count() <= 0)
                    break;
                if (!dbus_connection_read_write(m_connection, static_cast<int>(left.count())))
                {
                    closeConnection();
                    break;
                }
                // The answer goes back through colorMode, so no handler is told of it.
                takeQueued();
            }
        }

        void Portal::closeConnection()
        {
            if (!m_connection)
                return;
            dbus_connection_close(m_connection);
            dbus_connection_unref(m_connection);
            m_connection = nullptr;
            m_fd = -1;
            m_readSerial = 0;
        }

        // Asks for the colour scheme through this method of the Settings interface, noting the
        // serial its answer will carry.
        void Portal::requestScheme(const char* method)
        {
            DBusMessage* message = dbus_message_new_method_call(SettingsPortal::busName,
                SettingsPortal::objectPath, SettingsPortal::interfaceName, method);
            if (!message)
                return;
            const char* settingsNamespace = ColorScheme::settingsNamespace;
            const char* key = ColorScheme::key;
            dbus_uint32_t serial = 0;
            const bool sent = dbus_message_append_args(message,
                    DBUS_TYPE_STRING, &settingsNamespace,
                    DBUS_TYPE_STRING, &key,
                    DBUS_TYPE_INVALID)
                && dbus_connection_send(m_connection, message, &serial);
            dbus_message_unref(message);
            if (!sent)
                return;
            m_readSerial = serial;
            m_readMethod = method;
            dbus_connection_flush(m_connection);
        }

        // EVERY MESSAGE THE CONNECTION HOLDS, in the order they came. A connection the bus has
        // closed is let go once its last message is read. True where the mode has moved.
        bool Portal::takeQueued()
        {
            bool moved = false;
            while (DBusMessage* message = dbus_connection_pop_message(m_connection))
            {
                if (take(message))
                    moved = true;
                dbus_message_unref(message);
            }
            if (!dbus_connection_get_is_connected(m_connection))
                closeConnection();
            return moved;
        }

        // ONE MESSAGE OFF THE BUS: the answer to the read still out, or a change the desktop
        // announced. Anything else - the bus's own greeting among them - is passed over.
        bool Portal::take(DBusMessage* message)
        {
            switch (dbus_message_get_type(message))
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
                return takeSignal(message);
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            {
                if (dbus_message_get_reply_serial(message) != m_readSerial)
                    return false;
                m_readSerial = 0;
                DBusMessageIter arguments{};
                if (!dbus_message_iter_init(message, &arguments))
                    return false;
                return takeScheme(arguments);
            }
            case DBUS_MESSAGE_TYPE_ERROR:
            {
                if (dbus_message_get_reply_serial(message) != m_readSerial)
                    return false;
                m_readSerial = 0;
                // A portal older than version 2 knows the read by its first name only.
                if (m_readMethod == SettingsPortal::readOneMethod
                    && dbus_message_is_error(message, DBUS_ERROR_UNKNOWN_METHOD))
                    requestScheme(SettingsPortal::readMethod);
                return false;
            }
            }
            return false;
        }

        // A CHANGE THE DESKTOP ANNOUNCED: the namespace, the key and the new value. The rule
        // asks the bus for this one change, and the arguments are checked all the same.
        bool Portal::takeSignal(DBusMessage* message)
        {
            if (!dbus_message_is_signal(message, SettingsPortal::interfaceName,
                SettingsPortal::changeSignal))
                return false;
            DBusMessageIter arguments{};
            const char* settingsNamespace = nullptr;
            const char* key = nullptr;
            if (!dbus_message_iter_init(message, &arguments)
                || dbus_message_iter_get_arg_type(&arguments) != DBUS_TYPE_STRING)
                return false;
            dbus_message_iter_get_basic(&arguments, &settingsNamespace);
            if (!dbus_message_iter_next(&arguments)
                || dbus_message_iter_get_arg_type(&arguments) != DBUS_TYPE_STRING)
                return false;
            dbus_message_iter_get_basic(&arguments, &key);
            if (std::string_view{ settingsNamespace } != ColorScheme::settingsNamespace
                || std::string_view{ key } != ColorScheme::key)
                return false;
            if (!dbus_message_iter_next(&arguments))
                return false;
            return takeScheme(arguments);
        }

        // THE VALUE, INSIDE AS MANY VARIANTS AS IT ARRIVED IN - one from ReadOne and from the
        // signal, two from Read. True where it moves the mode.
        bool Portal::takeScheme(DBusMessageIter& value)
        {
            if (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_VARIANT)
            {
                DBusMessageIter inner{};
                dbus_message_iter_recurse(&value, &inner);
                return takeScheme(inner);
            }
            dbus_uint32_t scheme = 0;
            if (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_UINT32)
                dbus_message_iter_get_basic(&value, &scheme);
            const ColorMode mode = scheme == ColorScheme::preferDark
                ? ColorMode::Dark
                : ColorMode::Light;
            if (mode == m_colorMode)
                return false;
            m_colorMode = mode;
            return true;
        }
    }
}
