module;
#include <dbus/dbus.h>
export module ClaFi.Platform.Linux.FileChooser;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    // The operation requested from the desktop's file chooser.
    export enum class FileChooserAction
    {
        PickFolder,
        SaveFile
    };

    // One request to the desktop's file chooser through the XDG portal. See Platform
    export class FileChooserRequest
    {
    public:
        using Answer = std::optional<std::filesystem::path>;
    public:
        // Opens the dialog, over the window the handle names where it names one.
        FileChooserRequest(std::string_view parentWindow, std::string_view title, FileChooserAction,
            const std::filesystem::path& startFolder, std::string_view suggestedName);
        ~FileChooserRequest();
        FileChooserRequest(const FileChooserRequest&) = delete;
        FileChooserRequest& operator=(const FileChooserRequest&) = delete;
    public:
        [[nodiscard]] bool settled() const { return m_connection == nullptr; }
        [[nodiscard]] int fd() const { return m_fd; }   // the socket the answer comes on
        void dispatch();   // reads what the bus has sent, a hang-up included
        // The chosen path, or no value if the chooser returned none.
        [[nodiscard]] const Answer& answer() const { return m_answer; }
    private:
        void ask(std::string_view parentWindow, std::string_view title, FileChooserAction,
            const std::filesystem::path& startFolder, std::string_view suggestedName);
        void watchResponse(std::string path);
        void take(DBusMessage*);
        void takeResponse(DBusMessage*);
        void settle();
    private:
        DBusConnection* m_connection{ nullptr };
        int m_fd{ -1 };
        dbus_uint32_t m_callSerial{ 0 };
        std::string m_requestPath{};   // where the portal announces the answer
        Answer m_answer{};
    };
}
