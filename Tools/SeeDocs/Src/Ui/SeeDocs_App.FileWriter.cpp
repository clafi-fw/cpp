module SeeDocs_App.FileWriter;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    // WriteFailedEvent

    WriteFailedEvent::WriteFailedEvent(std::filesystem::path failed)
        :
        ::ClaFi::Event{},
        file{ std::move(failed) }
    {
    }

    // FileWriter

    FileWriter::FileWriter()
    {
        m_failedTimer.onTick([this](TimerEvent&) {
            deliver();
        });
        m_thread = std::thread{ &FileWriter::run, this };
    }

    FileWriter::~FileWriter()
    {
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            m_ending = true;
        }
        m_changed.notify_all();
        m_thread.join();
    }

    void FileWriter::write(std::filesystem::path file, FileContent content)
    {
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            std::erase_if(m_jobs, [&file](const Job& job) {
                return job.file == file;
            });
            m_jobs.push_back({ .file = std::move(file), .content = std::move(content) });
        }
        m_changed.notify_all();
    }

    void FileWriter::waitIdle()
    {
        std::unique_lock<std::mutex> lock{ m_mutex };
        m_changed.wait(lock, [this]() {
            return m_jobs.empty() && !m_writing;
        });
    }

    // **The end lets the thread go only once the writes asked for are done.** A write's content
    // is let go before the lock is taken again, since letting go of a document takes a while.
    void FileWriter::run()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock{ m_mutex };
            m_changed.wait(lock, [this]() {
                return !m_jobs.empty() || m_ending;
            });
            if (m_jobs.empty())
                return;
            Job job = std::move(m_jobs.front());
            m_jobs.pop_front();
            m_writing = true;
            lock.unlock();
            const bool written = writeWhole(job);
            job.content = nullptr;
            lock.lock();
            m_writing = false;
            if (!written)
                m_failed.push_back(std::move(job.file));
            lock.unlock();
            m_changed.notify_all();
            if (!written)
                m_failedTimer.start(MilliSeconds{ 0u });
        }
    }

    // **Written beside the file and renamed over it**, so that a write cut short leaves the file
    // as it was.
    bool FileWriter::writeWhole(const Job& job)
    {
        std::filesystem::path beside = job.file;
        beside += k_writingSuffix;
        bool written = false;
        try
        {
            written = job.content(beside);
        }
        catch (...)
        {
            written = false;
        }
        std::error_code error;
        if (written)
            std::filesystem::rename(beside, job.file, error);
        if (written && !error)
            return true;
        std::filesystem::remove(beside, error);
        return false;
    }

    void FileWriter::deliver()
    {
        Files failed;
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            failed.swap(m_failed);
        }
        for (std::filesystem::path& file : failed)
        {
            WriteFailedEvent event{ std::move(file) };
            emitEvent(event);
        }
    }
}
