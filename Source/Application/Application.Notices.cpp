module ClaFi.App.Notices;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.SplitButtonBase;
import ClaFi.Controls.MessageBar;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    namespace
    {
        constexpr float k_cardWidth{ 440.0f };   // the widest a card stands
        constexpr float k_margin{ 16.0f };       // between the cards and the window's edges
        constexpr float k_spacing{ 8.0f };       // between one card and the next
        constexpr std::wstring_view k_dismissCaption{ L"Dismiss" };

        class NoticesLayer;

        // A notice as it stands over the main window. See Application
        class NoticeCard : public MessageBar
        {
        public:
            NoticeCard(const CreateParams&, NoticesLayer&, NoticeId, const Notice&);
        public:
            [[nodiscard]] std::wstring_view diagnosticText() const override
                { return L"NoticeCard"; }
            [[nodiscard]] NoticeId id() const { return m_id; }
            // Whether a press on one of the card's buttons is still running.
            [[nodiscard]] bool pressing() const { return m_pressing > 0; }
        protected:
            [[nodiscard]] SecondaryPartPlacement secondaryPartPlacement() const override
                { return SecondaryPartPlacement::Bottom; }
            void placeSecondaryPart(ScaledDimensions childArea) override;
        private:
            // Runs what a press on one of the buttons does, the card standing until it has run.
            void press(const std::function<void()>& action);
        private:
            NoticesLayer& m_layer;
            NoticeId m_id;
            int m_pressing{ 0 };
        };

        // A card for every notice, over the main window against its bottom right. See Application
        class NoticesLayer : public ContainerBase
        {
        public:
            explicit NoticesLayer(const CreateParams&);
        public:
            [[nodiscard]] std::wstring_view diagnosticText() const override
                { return L"NoticesLayer"; }
            // Deletes the cards whose notices are down once the click being dispatched has unwound.
            void collectGoneCards();
        protected:
            void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        private:
            using Cards = std::vector<NoticeCard*>;
        private:
            // A card for every notice that has none, and none for a notice that is down.
            void sync();
            // Deletes the hidden cards that no press is running on.
            void deleteGoneCards();
        private:
            Cards m_cards{};       // standing, oldest first
            Cards m_goneCards{};   // hidden, and deleted by collectGoneCards
            UiTimer m_collectTimer{};
            ScopedEventConnection m_noticesChanged;
        };
    }

    // NoticeHandle

    NoticeHandle::NoticeHandle(NoticeHandle&& other) noexcept
        :
        m_id{ std::exchange(other.m_id, 0) }
    {
    }

    // **Taking another handle takes this one's notice down**, so an owner holding one handle per
    // subject puts the new notice up in the old one's place by assigning.
    NoticeHandle& NoticeHandle::operator=(NoticeHandle&& other) noexcept
    {
        if (this != &other)
        {
            withdraw();
            m_id = std::exchange(other.m_id, 0);
        }
        return *this;
    }

    NoticeHandle::~NoticeHandle()
    {
        withdraw();
    }

    bool NoticeHandle::standing() const
    {
        return m_id != 0 && notices().stands(m_id);
    }

    void NoticeHandle::withdraw()
    {
        if (m_id == 0)
            return;
        notices().remove(std::exchange(m_id, 0));
    }

    NoticeHandle::NoticeHandle(const NoticeId id)
        :
        m_id{ id }
    {
    }

    // NoticesChangeEvent

    NoticesChangeEvent::NoticesChangeEvent(Notices& sender)
        :
        EventOf<Notices>{ sender }
    {
    }

    // Notices

    NoticeHandle Notices::post(Notice notice)
    {
        const NoticeId id = ++m_lastId;
        m_standing.push_back(Standing{ .id = id, .notice = std::move(notice) });
        emitEvent<NoticesChangeEvent>(*this);
        return NoticeHandle{ id };
    }

    void Notices::close(const NoticeId id)
    {
        remove(id);
    }

    // **Down before the answer runs.** The answer may put a notice up in this one's place, run a
    // loop of its own, or close the window the notice stands in, and in none of them should this
    // notice still be standing.
    void Notices::answer(const NoticeId id, const std::size_t answerIndex, ClickEvent& event)
    {
        const StandingNotices::iterator found = std::ranges::find(m_standing, id, &Standing::id);
        if (found == m_standing.end() || answerIndex >= found->notice.answers.size())
            return;
        const NoticeAnswerHandler run = std::move(found->notice.answers[answerIndex].run);
        remove(id);
        if (run)
            run(event);
    }

    bool Notices::stands(const NoticeId id) const
    {
        return std::ranges::find(m_standing, id, &Standing::id) != m_standing.end();
    }

    void Notices::remove(const NoticeId id)
    {
        const std::size_t removed = std::erase_if(m_standing, [id](const Standing& entry) {
            return entry.id == id;
        });
        if (removed != 0)
            emitEvent<NoticesChangeEvent>(*this);
    }

    // notices

    // **One list for the application**, kept the way the backstage keeps its pages. Every handle
    // belongs to an object that goes down with the application, so the list is empty by the time
    // it goes itself.
    Notices& notices()
    {
        static Notices list{};
        return list;
    }

    // connectNotices

    // **Before the form is shown**, so its first layout already holds the notices posted while the
    // application was starting.
    void connectNotices(AppContext& context)
    {
        context.events().connect<RootLoopEvent>([](RootLoopEvent& event) {
            FormBase& form = event.sender();
            form.setTopLayer(std::make_unique<NoticesLayer>(CreateParams{ form }));
        });
    }

    namespace
    {
        // NoticeCard

        // **The title heads the message in bold, and Dismiss ends the answers.** Every answer is
        // MouseOnly, so a press on one leaves the focus with whatever held it - the notice came on
        // its own time, and the user was doing something else.
        NoticeCard::NoticeCard(const CreateParams& params, NoticesLayer& layer, const NoticeId id,
            const Notice& notice)
            :
            MessageBar{
                params,
                MaxSize{ k_cardWidth, k_maxFloat },
                HorizontalAlign::Fill,
                VerticalAlign::Top
            },
            m_layer{ layer },
            m_id{ id }
        {
            Text shown{};
            if (!notice.title.empty())
                shown << TextOp::PushBold << notice.title << TextOp::PopBold << L'\n';
            shown << notice.message;
            setMessage(notice.icon, shown);
            for (std::size_t index = 0; index < notice.answers.size(); ++index)
            {
                addAnswer(
                    notice.answers[index].caption,
                    [this, id, index](ClickEvent& event) {
                        press([id, index, &event]() {
                            notices().answer(id, index, event);
                        });
                    },
                    Interactivity::MouseOnly
                );
            }
            addAnswer(
                k_dismissCaption,
                [this, id](ClickEvent&) {
                    press([id]() {
                        notices().close(id);
                    });
                },
                Interactivity::MouseOnly
            );
        }

        // **The answers end the card**, under the message and against its right edge, the way a
        // dialog's buttons stand.
        void NoticeCard::placeSecondaryPart(const ScaledDimensions childArea)
        {
            Control& part = *shownSecondaryPart();
            const ScaledDimensions size = part.dimensions();
            setControlPlacement(part, { childArea.x - size.x, childArea.y - size.y }, size);
        }

        // **A card stands until every press on it has run.** An answer may run a loop of its own -
        // a menu, a dialog - and a card deleted under that loop would leave the click that started
        // it nothing to return to. The layer collects once the press has run, on a timer, so the
        // collecting comes after the click has unwound.
        void NoticeCard::press(const std::function<void()>& action)
        {
            ++m_pressing;
            action();
            --m_pressing;
            m_layer.collectGoneCards();
        }

        // NoticesLayer

        NoticesLayer::NoticesLayer(const CreateParams& params)
            :
            ContainerBase{ params, HorizontalAlign::Fill, VerticalAlign::Fill },
            m_noticesChanged{ notices().onChange([this](NoticesChangeEvent&) {
                sync();
            }) }
        {
            m_collectTimer.onTick([this](TimerEvent&) {
                deleteGoneCards();
            });
            sync();
        }

        void NoticesLayer::collectGoneCards()
        {
            if (!m_goneCards.empty())
                m_collectTimer.start(MilliSeconds{ 0u });
        }

        // **Newest at the bottom**, each card the width it is given and as tall as its message at
        // that width, the stack climbing from the window's bottom right. A window too narrow for a
        // card gives it what it has inside the margins.
        void NoticesLayer::alignContent(AlignEvent& event, ScaledPosition,
            ScaledDimensions& contentDimensions)
        {
            const float margin = scaler().scale(k_margin);
            const float spacing = scaler().scale(k_spacing);
            const float width = std::min(
                scaler().scale(k_cardWidth),
                std::max(contentDimensions.x - margin * 2.0f, 0.0f)
            );
            const float left = contentDimensions.x - margin - width;
            float bottom = contentDimensions.y - margin;
            for (NoticeCard* card : m_cards | std::views::reverse)
            {
                alignControl(card, event, { left, 0.0f }, { width, std::max(bottom, 0.0f) });
                bottom -= card->height();
                setControlTop(*card, bottom);
                bottom -= spacing;
            }
        }

        // **A card going is hidden at once and deleted later** - see NoticeCard::press. It goes
        // from inside its own answer's click as often as not, and a control is not deleted from
        // under the click it is dispatching.
        void NoticesLayer::sync()
        {
            const Notices::StandingNotices& standing = notices().standing();
            for (NoticeCard* card : m_cards)
            {
                const Notices::StandingNotices::const_iterator found =
                    std::ranges::find(standing, card->id(), &Notices::Standing::id);
                if (found == standing.end())
                {
                    card->setVisible(false);
                    m_goneCards.push_back(card);
                }
            }
            std::erase_if(m_cards, [this](const NoticeCard* card) {
                return std::ranges::find(m_goneCards, card) != m_goneCards.end();
            });
            for (const Notices::Standing& entry : standing)
            {
                if (std::ranges::find(m_cards, entry.id, &NoticeCard::id) == m_cards.end())
                    m_cards.push_back(&add<NoticeCard>(*this, entry.id, entry.notice));
            }
            collectGoneCards();
            invalidateFormAlign();
        }

        void NoticesLayer::deleteGoneCards()
        {
            Cards pressed{};
            for (NoticeCard* card : m_goneCards)
            {
                if (card->pressing())
                    pressed.push_back(card);
                else
                    card->deleteSelf();
            }
            m_goneCards = std::move(pressed);
        }
    }
}
