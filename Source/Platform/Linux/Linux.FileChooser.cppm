module;
#include <dbus/dbus.h>
export module ClaFi.Platform.Linux.FileChooser;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    // One folder asked of the desktop's file chooser through the XDG portal. See Platform
    export class FolderRequest
    {
    public:
        using Answer = std::optional<std::filesystem::path>;
    public:
        // Opens the dialog, over the window the handle names where it names one.
        FolderRequest(std::string_view parentWindow, std::string_view title,
            const std::filesystem::path& startFolder);
        ~FolderRequest();
        FolderRequest(const FolderRequest&) = delete;
        FolderRequest& operator=(const FolderRequest&) = delete;
    public:
        [[nodiscard]] bool settled() const { return m_connection == nullptr; }
        [[nodiscard]] int fd() const { return m_fd; }   // the socket the answer comes on
        void dispatch();   // reads what the bus has sent, a hang-up included
        // The folder picked; nothing where the dialog was cancelled or no chooser answered.
        [[nodiscard]] const Answer& answer() const { return m_answer; }
    private:
        void ask(std::string_view parentWindow, std::string_view title,
            const std::filesystem::path& startFolder);
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
