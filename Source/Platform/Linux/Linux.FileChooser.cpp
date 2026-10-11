module;
#include <dbus/dbus.h>
module ClaFi.Platform.Linux.FileChooser;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    namespace
    {
        // Where the portal serves the file chooser, and where it announces an answer.
        namespace ChooserPortal
        {
            constexpr const char* busName = "org.freedesktop.portal.Desktop";
            constexpr const char* objectPath = "/org/freedesktop/portal/desktop";
            constexpr const char* interfaceName = "org.freedesktop.portal.FileChooser";
            constexpr const char* openMethod = "OpenFile";
            constexpr std::string_view requestInterface = "org.freedesktop.portal.Request";
            constexpr std::string_view responseSignal = "Response";
            // A request's path: this, the caller's unique name, and the token it chose.
            constexpr std::string_view requestFolder = "/org/freedesktop/portal/desktop/request/";
        }

        // The options OpenFile takes, and the result it answers with.
        namespace ChooserKeys
        {
            constexpr const char* handleToken = "handle_token";
            constexpr const char* modal = "modal";
            constexpr const char* directory = "directory";
            constexpr const char* currentFolder = "current_folder";
            constexpr std::string_view uris = "uris";
        }

        constexpr const char* k_optionsSignature = "{sv}";
        constexpr dbus_uint32_t k_responseSuccess = 0;
        constexpr std::string_view k_fileScheme = "file://";
        constexpr std::string_view k_tokenPrefix = "clafi";

        using Answer = FolderRequest::Answer;

        [[nodiscard]] std::string nextToken();   // unique for the process's life
        // A unique bus name as a request path spells it: ":1.42" is "1_42".
        [[nodiscard]] std::string senderSegment(std::string_view uniqueName);
        // One entry of an a{sv}: the key, then the value in a variant of the signature given.
        [[nodiscard]] bool openOption(DBusMessageIter& options, const char* key,
            const char* signature, DBusMessageIter& entry, DBusMessageIter& variant);
        [[nodiscard]] bool closeOption(DBusMessageIter& options, DBusMessageIter& entry,
            DBusMessageIter& variant);
        [[nodiscard]] bool appendTextOption(DBusMessageIter& options, const char* key,
            const char* value);
        [[nodiscard]] bool appendFlagOption(DBusMessageIter& options, const char* key,
            dbus_bool_t value);
        [[nodiscard]] bool appendBytesOption(DBusMessageIter& options, const char* key,
            std::string_view bytes);
        // The folder the results' uris name: the first of them, as a path on this machine.
        [[nodiscard]] Answer folderOfResults(DBusMessageIter& results);
        [[nodiscard]] Answer folderOfUris(DBusMessageIter& variant);
        // A file URI's path with its escapes read; nothing for a URI of any other scheme.
        [[nodiscard]] Answer pathOfUri(std::string_view uri);
        [[nodiscard]] int hexValue(char digit);   // -1 for a character that is no hex digit
    }
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Linux
{
    // FolderRequest

    FolderRequest::FolderRequest(const std::string_view parentWindow, const std::string_view title,
        const std::filesystem::path& startFolder)
    {
        ask(parentWindow, title, startFolder);
    }

    FolderRequest::~FolderRequest()
    {
        settle();
    }

    // What the loop calls after each wait. The socket is read whether or not it stirred, so a
    // hang-up, which poll reports whatever was asked for, settles the request rather than
    // waking every wait after it.
    void FolderRequest::dispatch()
    {
        if (!m_connection)
            return;
        if (!dbus_connection_read_write(m_connection, 0))
        {
            settle();
            return;
        }
        while (m_connection)
        {
            DBusMessage* message = dbus_connection_pop_message(m_connection);
            if (!message)
                break;
            take(message);
            dbus_message_unref(message);
        }
        if (m_connection && !dbus_connection_get_is_connected(m_connection))
            settle();
    }

    // **A private connection**, so that its lifetime is the request's. The answer is watched for
    // before OpenFile is sent, under the path the token names, so it cannot arrive unwatched.
    // Every way this fails settles the request with no answer.
    void FolderRequest::ask(const std::string_view parentWindow, const std::string_view title,
        const std::filesystem::path& startFolder)
    {
        DBusError error{};
        dbus_error_init(&error);
        m_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
        dbus_error_free(&error);
        if (!m_connection)
            return;
        // A bus that goes away ends this request and not the application.
        dbus_connection_set_exit_on_disconnect(m_connection, FALSE);
        const char* uniqueName = dbus_bus_get_unique_name(m_connection);
        if (!dbus_connection_get_unix_fd(m_connection, &m_fd) || !uniqueName)
        {
            settle();
            return;
        }

        const std::string token = nextToken();
        std::string requestPath{ ChooserPortal::requestFolder };
        requestPath.append(senderSegment(uniqueName));
        requestPath.push_back('/');
        requestPath.append(token);
        watchResponse(std::move(requestPath));

        DBusMessage* message = dbus_message_new_method_call(ChooserPortal::busName,
            ChooserPortal::objectPath, ChooserPortal::interfaceName, ChooserPortal::openMethod);
        if (!message)
        {
            settle();
            return;
        }
        const std::string parent{ parentWindow };
        const std::string caption{ title };
        const char* parentText = parent.c_str();
        const char* captionText = caption.c_str();
        // The portal takes the start folder as the path's bytes ending in a null.
        std::string startBytes = startFolder.native();
        startBytes.push_back('\0');
        DBusMessageIter arguments{};
        DBusMessageIter options{};
        dbus_message_iter_init_append(message, &arguments);
        bool built = dbus_message_iter_append_basic(&arguments, DBUS_TYPE_STRING, &parentText)
            && dbus_message_iter_append_basic(&arguments, DBUS_TYPE_STRING, &captionText)
            && dbus_message_iter_open_container(&arguments, DBUS_TYPE_ARRAY, k_optionsSignature,
                &options)
            && appendTextOption(options, ChooserKeys::handleToken, token.c_str())
            && appendFlagOption(options, ChooserKeys::modal, TRUE)
            && appendFlagOption(options, ChooserKeys::directory, TRUE);
        if (built && !startFolder.empty())
            built = appendBytesOption(options, ChooserKeys::currentFolder, startBytes);
        built = built && dbus_message_iter_close_container(&arguments, &options);
        const bool sent = built && dbus_connection_send(m_connection, message, &m_callSerial);
        dbus_message_unref(message);
        if (!sent)
        {
            settle();
            return;
        }
        dbus_connection_flush(m_connection);
    }

    // The bus delivers the one signal this request waits for. Given no error to fill, the match
    // is sent without waiting for the bus, and goes out ahead of what is sent after it.
    void FolderRequest::watchResponse(std::string path)
    {
        m_requestPath = std::move(path);
        std::string rule = "type='signal',interface='";
        rule.append(ChooserPortal::requestInterface);
        rule.append("',member='");
        rule.append(ChooserPortal::responseSignal);
        rule.append("',path='");
        rule.append(m_requestPath);
        rule.push_back('\'');
        dbus_bus_add_match(m_connection, rule.c_str(), nullptr);
    }

    // **One message off the bus**: OpenFile's reply, its refusal, or the answer. A portal older
    // than the token answers OpenFile with a request path of its own, and that path is watched
    // instead. Anything else - the bus's own greeting among them - is passed over.
    void FolderRequest::take(DBusMessage* message)
    {
        switch (dbus_message_get_type(message))
        {
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            {
                if (dbus_message_get_reply_serial(message) != m_callSerial)
                    return;
                const char* requestPath = nullptr;
                const bool read = dbus_message_get_args(message, nullptr,
                    DBUS_TYPE_OBJECT_PATH, &requestPath,
                    DBUS_TYPE_INVALID);
                if (read && m_requestPath != requestPath)
                    watchResponse(requestPath);
                return;
            }
            case DBUS_MESSAGE_TYPE_ERROR:
            {
                if (dbus_message_get_reply_serial(message) == m_callSerial)
                    settle();
                return;
            }
            case DBUS_MESSAGE_TYPE_SIGNAL:
            {
                takeResponse(message);
                return;
            }
        }
    }

    // **The answer**: a response code, then the results. Any code but success is the dialog
    // cancelled or failed, and settles the request with nothing.
    void FolderRequest::takeResponse(DBusMessage* message)
    {
        const char* interfaceName = dbus_message_get_interface(message);
        const char* member = dbus_message_get_member(message);
        const char* path = dbus_message_get_path(message);
        if (!interfaceName || !member || !path)
            return;
        if (ChooserPortal::requestInterface != interfaceName
            || ChooserPortal::responseSignal != member
            || m_requestPath != path)
            return;

        DBusMessageIter arguments{};
        if (dbus_message_iter_init(message, &arguments)
            && dbus_message_iter_get_arg_type(&arguments) == DBUS_TYPE_UINT32)
        {
            dbus_uint32_t response = 0;
            dbus_message_iter_get_basic(&arguments, &response);
            if (response == k_responseSuccess && dbus_message_iter_next(&arguments))
                m_answer = folderOfResults(arguments);
        }
        settle();
    }

    void FolderRequest::settle()
    {
        if (!m_connection)
            return;
        dbus_connection_close(m_connection);
        dbus_connection_unref(m_connection);
        m_connection = nullptr;
        m_fd = -1;
    }

    namespace
    {
        // A handle token is an object path element: letters, digits and underscores.
        std::string nextToken()
        {
            static std::uint32_t s_issued = 0;
            ++s_issued;
            std::string token{ k_tokenPrefix };
            token.append(std::to_string(s_issued));
            return token;
        }

        std::string senderSegment(const std::string_view uniqueName)
        {
            std::string segment{ uniqueName.starts_with(':') ? uniqueName.substr(1) : uniqueName };
            std::ranges::replace(segment, '.', '_');
            return segment;
        }

        bool openOption(DBusMessageIter& options, const char* key, const char* signature,
            DBusMessageIter& entry, DBusMessageIter& variant)
        {
            return dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, nullptr, &entry)
                && dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key)
                && dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, signature, &variant);
        }

        bool closeOption(DBusMessageIter& options, DBusMessageIter& entry, DBusMessageIter& variant)
        {
            return dbus_message_iter_close_container(&entry, &variant)
                && dbus_message_iter_close_container(&options, &entry);
        }

        bool appendTextOption(DBusMessageIter& options, const char* key, const char* value)
        {
            DBusMessageIter entry{};
            DBusMessageIter variant{};
            return openOption(options, key, DBUS_TYPE_STRING_AS_STRING, entry, variant)
                && dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value)
                && closeOption(options, entry, variant);
        }

        bool appendFlagOption(DBusMessageIter& options, const char* key, const dbus_bool_t value)
        {
            DBusMessageIter entry{};
            DBusMessageIter variant{};
            return openOption(options, key, DBUS_TYPE_BOOLEAN_AS_STRING, entry, variant)
                && dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value)
                && closeOption(options, entry, variant);
        }

        bool appendBytesOption(DBusMessageIter& options, const char* key,
            const std::string_view bytes)
        {
            DBusMessageIter entry{};
            DBusMessageIter variant{};
            DBusMessageIter array{};
            const char* data = bytes.data();
            return openOption(options, key, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING,
                    entry, variant)
                && dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
                    DBUS_TYPE_BYTE_AS_STRING, &array)
                && dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE, &data,
                    static_cast<int>(bytes.size()))
                && dbus_message_iter_close_container(&variant, &array)
                && closeOption(options, entry, variant);
        }

        Answer folderOfResults(DBusMessageIter& results)
        {
            if (dbus_message_iter_get_arg_type(&results) != DBUS_TYPE_ARRAY)
                return std::nullopt;
            DBusMessageIter entries{};
            dbus_message_iter_recurse(&results, &entries);
            while (dbus_message_iter_get_arg_type(&entries) == DBUS_TYPE_DICT_ENTRY)
            {
                DBusMessageIter entry{};
                dbus_message_iter_recurse(&entries, &entry);
                const char* key = nullptr;
                if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
                    dbus_message_iter_get_basic(&entry, &key);
                if (key && ChooserKeys::uris == key && dbus_message_iter_next(&entry))
                    return folderOfUris(entry);
                dbus_message_iter_next(&entries);
            }
            return std::nullopt;
        }

        // A variant holding an array of strings.
        Answer folderOfUris(DBusMessageIter& variant)
        {
            if (dbus_message_iter_get_arg_type(&variant) != DBUS_TYPE_VARIANT)
                return std::nullopt;
            DBusMessageIter uris{};
            dbus_message_iter_recurse(&variant, &uris);
            if (dbus_message_iter_get_arg_type(&uris) != DBUS_TYPE_ARRAY)
                return std::nullopt;
            DBusMessageIter uri{};
            dbus_message_iter_recurse(&uris, &uri);
            if (dbus_message_iter_get_arg_type(&uri) != DBUS_TYPE_STRING)
                return std::nullopt;
            const char* text = nullptr;
            dbus_message_iter_get_basic(&uri, &text);
            return pathOfUri(text);
        }

        // The host stands between the scheme and the path, and is empty for this machine.
        Answer pathOfUri(const std::string_view uri)
        {
            if (!uri.starts_with(k_fileScheme))
                return std::nullopt;
            const std::string_view afterScheme = uri.substr(k_fileScheme.size());
            const std::size_t pathStart = afterScheme.find('/');
            if (pathStart == std::string_view::npos)
                return std::nullopt;
            const std::string_view escaped = afterScheme.substr(pathStart);
            std::string path;
            path.reserve(escaped.size());
            for (std::size_t i = 0; i < escaped.size(); ++i)
            {
                const char character = escaped[i];
                const bool escape = character == '%' && i + 2 < escaped.size();
                const int high = escape ? hexValue(escaped[i + 1]) : -1;
                const int low = high >= 0 ? hexValue(escaped[i + 2]) : -1;
                if (low < 0)
                {
                    path.push_back(character);
                    continue;
                }
                path.push_back(static_cast<char>(high * 16 + low));
                i += 2;
            }
            return std::filesystem::path{ path };
        }

        int hexValue(const char digit)
        {
            if (digit >= '0' && digit <= '9')
                return digit - '0';
            if (digit >= 'a' && digit <= 'f')
                return digit - 'a' + 10;
            if (digit >= 'A' && digit <= 'F')
                return digit - 'A' + 10;
            return -1;
        }
    }
}
