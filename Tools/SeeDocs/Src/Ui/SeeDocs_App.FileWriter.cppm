export module SeeDocs_App.FileWriter;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    // A file could not be written, and stands as it was. The writer has an Event of its own.
    export class WriteFailedEvent : public ::ClaFi::Event
    {
    public:
        explicit WriteFailedEvent(std::filesystem::path);
    public:
        const std::filesystem::path file;
    };

    // Writes a file's content to the path it is handed; false where it could not.
    export using FileContent = std::function<bool(const std::filesystem::path&)>;

    // **Writes files on a thread of its own**, one at a time, in the order asked. See README#saving
    export class FileWriter : public EventComponent
    {
    public:
        FileWriter();
        FileWriter(const FileWriter&) = delete;
        FileWriter& operator=(const FileWriter&) = delete;
        ~FileWriter();   // the writes asked for are done first
    public:
        // Connects a handler raised on the UI thread when a file could not be written.
        template<typename F>
        EventConnection onWriteFailed(F&& callback);
    public:
        // Writes the file beside itself and renames it over the old one; a write of the file still
        // waiting gives way to this one.
        void write(std::filesystem::path file, FileContent);
        void waitIdle();   // until every write asked for is done
    private:
        struct Job
        {
            std::filesystem::path file;
            FileContent content;
        };
        using Jobs = std::deque<Job>;
        using Files = std::vector<std::filesystem::path>;
    private:
        void run();
        [[nodiscard]] static bool writeWhole(const Job&);
        void deliver();   // the failures the thread left, told on the UI thread
    private:
        static constexpr std::wstring_view k_writingSuffix = L".writing";
        std::mutex m_mutex{};
        std::condition_variable m_changed{};   // a write asked for or done, or the end
        Jobs m_jobs{};
        bool m_writing{ false };
        bool m_ending{ false };
        Files m_failed{};
        UiTimer m_failedTimer{};
        std::thread m_thread{};   // started once every member it reaches stands
    };

    template<typename F>
    EventConnection FileWriter::onWriteFailed(F&& callback)
    {
        return connectEvent<WriteFailedEvent>(std::forward<F>(callback));
    }
}
