module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.App.Notices;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // What pressing one of a notice's answers does with the click. See Application
    export using NoticeAnswerHandler = std::function<void(ClickEvent&)>;

    // One of a notice's answers: the caption its button wears and what pressing it does.
    export struct NoticeAnswer
    {
        std::wstring caption{};
        NoticeAnswerHandler run{};
    };

    export using NoticeAnswers = std::vector<NoticeAnswer>;

    // What a notice says: its kind, title and message, and the answers it offers. See Application
    export struct Notice
    {
        MessageIcon icon{ MessageIcon::Information };
        std::wstring title{};
        Text message{};
        NoticeAnswers answers{};
    };

    // The number a standing notice goes by. Zero names none.
    export using NoticeId = std::uint64_t;

    export class Notices;

    // A notice stands while this does, until the user puts it away. See Application
    export class NoticeHandle
    {
        friend Notices;
    public:
        NoticeHandle() = default;
        NoticeHandle(NoticeHandle&&) noexcept;
        NoticeHandle& operator=(NoticeHandle&&) noexcept;
        NoticeHandle(const NoticeHandle&) = delete;
        NoticeHandle& operator=(const NoticeHandle&) = delete;
        ~NoticeHandle();
    public:
        // Whether the notice still stands - one put away or answered does not.
        [[nodiscard]] bool standing() const;
        // Takes the notice down where it still stands.
        void withdraw();
    private:
        explicit NoticeHandle(NoticeId);
    private:
        NoticeId m_id{ 0 };
    };

    // A notice has been put up, put away, answered or withdrawn.
    export class NoticesChangeEvent : public EventOf<Notices>
    {
    public:
        explicit NoticesChangeEvent(Notices& sender);
    };

    // The notices standing in the application, oldest first. See Application
    export class Notices : public EventComponent
    {
        friend NoticeHandle;
    public:
        // A notice as it stands, under the number its handle and its card know it by.
        struct Standing
        {
            NoticeId id{ 0 };
            Notice notice{};
        };
        using StandingNotices = std::vector<Standing>;
    public:
        Notices() = default;
        Notices(const Notices&) = delete;
        Notices& operator=(const Notices&) = delete;
    public:
        DECLARE_EVENT(NoticesChangeEvent, OnChange, onChange)
    public:
        // Puts a notice up after those standing, for as long as the handle stands. See Application
        [[nodiscard]] NoticeHandle post(Notice);
        [[nodiscard]] const StandingNotices& standing() const { return m_standing; }
        // The user put the notice away.
        void close(NoticeId);
        // The user pressed one of the notice's answers. See Application
        void answer(NoticeId, std::size_t answerIndex, ClickEvent&);
    private:
        [[nodiscard]] bool stands(NoticeId) const;
        // Takes the notice down, and raises the change where it stood.
        void remove(NoticeId);
    private:
        StandingNotices m_standing{};
        NoticeId m_lastId{ 0 };
    };

    // The application's notices, shown over its main window.
    export [[nodiscard]] Notices& notices();

    // Shows the notices over every form that takes the root loop, once per application.
    export void connectNotices(AppContext&);
}
