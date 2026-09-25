export module ClaFi.Core.Foundation :WithTextLayout;

import :Control;
import :RichControl;
import :PaintEvent;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    /// @brief A control that holds its OWN text layout instead of taking one from TextEngine's
    /// cache. For a text that CHANGES.
    ///
    /// @note The cache is addressed by what a text SAYS, which is what lets a hundred buttons
    /// saying one word share a single shaping. A text rewritten every frame - the value under a
    /// slider's thumb, a frame rate, the size of a rectangle being dragged - therefore misses on
    /// every paint, takes a fresh entry, and evicts the static layouts the rest of the form draws
    /// from: a drag of a few seconds empties the cache and leaves every label on the form to be
    /// shaped again. A host here shapes its own text in place instead - no hash, no comparison
    /// against another control's words, nothing evicted, and nothing of its own in there to be
    /// evicted from.
    ///
    /// @note USING IT IS THE DECLARATION. A control whose text changes often is one that derives
    /// from this, and a control whose text is written once and then stands does not. There is no
    /// property to state and nothing to keep in step.
    export template <IsControl HostClass>
    class WithTextLayout : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithTextLayout(const CreateParams&, Args&&...);
    protected:
        /// The two seams a control holding a layout of its own overrides. They are a pair:
        /// override one and the control measures on one layout and draws on another.
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;

        /// @brief The layout, brought up to date with the text it draws, the rect it draws in and
        /// the scale, and returned ready to answer.
        ///
        /// @note Every measurement and the paint both come through here, so a position can never
        /// be read off one build and drawn on another. The overload taking no text gathers one -
        /// see Control::doGetText.
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&, const FloatRect& textBounds) const;
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&, const FloatRect& textBounds,
            const Text&) const;

        /// @brief The width this control's lines are broken at, when that is not the box it is
        /// laid out into. Zero - the standing answer - breaks them at the box.
        [[nodiscard]] virtual float lineBreakWidth() const;

        /// @brief Puts a text the layout has not been told about into it.
        ///
        /// @note The whole of it, which is what a text rewritten from nothing needs. A host that
        /// knows what CHANGED says so instead: TextBox splices the paragraphs its edit reached and
        /// leaves the rest of the document where it stands.
        virtual void takeText(const Text&) const;

        /// @brief The stamp of the control's OWN text, when that is what a gather answered with.
        ///
        /// @note A gather NAMES a control's text rather than copying it, so the answer is that
        /// object itself and is recognised by address. Anything else - a text composed by a
        /// parent, or built by a handler - has no life of its own and no stamp, and is told from
        /// the last one by being compared.
        [[nodiscard]] TextStamp stampOf(const Text& answer) const;
    protected:
        // What the layout was built from. Mutable, both of them: measuring is a const question
        // about the control, and the layout is the answer's working - a memo of the control's own
        // text, not state a caller can observe by any other route. What a const measurement must
        // not do is CHANGE what is drawn, and it cannot: every path here builds the layout from
        // doGetText, which is what paintText hands it too.
        mutable Text m_layoutText;
        mutable TextLayout m_layout;
    private:
        // The last measurement and everything its answer depends on. A wrapping control is
        // measured again whenever the align pass grants it a box, and a box granted the same one
        // twice is owed the same answer rather than a second shaping - so the question is what is
        // compared, and nothing here is read from the layout.
        struct Measurement
        {
            TextStamp text{};
            ScaledDimensions asked{ 0.0f, 0.0f };
            ScaleFactor scaleFactor{ 0.0f };
            float breakWidth{ 0.0f };
            bool wrap{ false };
            bool editable{ false };
            std::uint64_t layoutGeneration{ 0 };
            bool operator==(const Measurement&) const = default;
        };
    private:
        // Where doGetText gathers, held so that the buffers a gather needs are grown once rather
        // than taken from the allocator per measurement. It is working, never an answer: what it
        // holds after a call is whatever the last gather put there, and only m_layoutText says
        // what the layout was built from.
        mutable Text m_gatheredText;
        // The stamp the layout was built from, so an unchanged text is recognised without reading
        // it. A text with no life of its own stamps nothing and is compared instead.
        mutable TextStamp m_layoutStamp{};
        Measurement m_measurement{};
        CalculatedDimensions m_measured{};
        // The engine generation the layout was built at. invalidateLayouts drops the cache and
        // moves that number on for anything a key does not carry - the font table above all - and
        // this layout is not in the cache to be dropped.
        mutable std::uint64_t m_layoutGeneration{ 0 };
    };


//-----------------------------------------------------------------------------


    template <IsControl HostClass>
    template <typename... Args>
    WithTextLayout<HostClass>::WithTextLayout(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... }
    {
    }

    template <IsControl HostClass>
    DrawTextResult WithTextLayout<HostClass>::drawText(PaintEvent& event,
        const FloatRect& textBounds, const Text& text)
    {
        TextLayout& layout = syncedLayout(event.formContext(), textBounds, text);
        return layout.draw(
            event.controlContext(),
            anchoredOrigin(textBounds, layout.calculatedDimensions(), this->textAnchor()),
            this->editProps(),
            this->textRenderMode()
        );
    }

    // Measured on the layout the paint will draw from. Through TextEngine's cache instead, a
    // control building a text of its own gets a SECOND layout of it: the key carries the text's
    // hash, so a control whose text moved misses on every measurement, shapes it again to answer
    // one question, and leaves the entry for the evictor to sweep.
    template <IsControl HostClass>
    CalculatedDimensions WithTextLayout<HostClass>::measureText(AlignEvent& event,
        ScaledDimensions asked, const Text& text)
    {
        const Measurement measurement{
            .text = stampOf(text),
            .asked = asked,
            .scaleFactor = event.scaleFactor(),
            .breakWidth = this->lineBreakWidth(),
            .wrap = this->wordWrap(),
            .editable = this->editProps() != nullptr,
            .layoutGeneration = Control::textEngine().layoutGeneration(),
        };
        // A measurement of a text with no life of its own is not kept: it would be recognised by
        // a stamp that names nothing, which is every other such measurement.
        if (measurement.text.named() && measurement == m_measurement)
            return m_measured;

        // Only the size reaches the layout, so the rect is built at the origin - see syncedLayout,
        // which reads nothing else from it.
        m_measured = syncedLayout(
            event.formContext(),
            FloatRect::fromDimensions({ 0.0f, 0.0f }, asked),
            text
        ).calculatedDimensions();
        m_measurement = measurement;
        return m_measured;
    }

    template <IsControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds) const
    {
        // Gathered into a member rather than a local. A gather that has to BUILD its answer copies
        // the whole text and every marker in it, and a Text declared here would take the buffers
        // for that copy from the allocator on every measurement. Cleared instead, so the buffers
        // one gather grew are the ones the next fills.
        m_gatheredText.clear();
        const Text& gathered = this->doGetText(formContext, m_gatheredText, EventPhase::Paint);
        return syncedLayout(formContext, textBounds, gathered);
    }

    template <IsControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds, const Text& text) const
    {
        // The stamp first, because it answers without reading either text: the control's own text
        // at the revision the layout was built from is the same text, and nothing has to be
        // compared or copied. A stamp that names nothing - a composed or built answer - falls
        // through to the comparison, which is what such an answer needs.
        //
        // Compared rather than assigned: setText invalidates whatever it is handed, so a control
        // measured four times in one pass would rebuild four times.
        //
        // A LAYOUT NOT YET STATED A TEXT IS TOLD ONE WHATEVER THE COMPARISON SAYS. An empty first
        // text compares equal to the empty text this was constructed beside, so nothing would be
        // taken and the layout would be asked to shape from a text it never held - a control whose
        // reading stands empty until something is there to read draws exactly that.
        const TextStamp stamp = stampOf(text);
        const bool recognised = stamp.named() && stamp == m_layoutStamp;
        if (!m_layout.isTextStated() || (!recognised && text != m_layoutText))
            takeText(text);

        // Taken whether or not anything was rebuilt: a text that compared equal is one the next
        // measurement can recognise without comparing it again.
        m_layoutStamp = stamp;
        if (m_layoutGeneration != Control::textEngine().layoutGeneration())
        {
            m_layoutGeneration = Control::textEngine().layoutGeneration();
            m_layout.invalidate();
        }
        // Set here rather than in the constructor: it is a property, and a control told to stop
        // wrapping after it was built draws the answer to that on the next paint.
        m_layout.setWrap(this->wordWrap());
        // Ahead of the bounds, which are accepted or refused against it.
        m_layout.setBreakWidth(this->lineBreakWidth());
        // Only the size: where the rect sits is the caller's business and the anchor's, and a
        // control scrolled sideways would otherwise rebuild for every pixel it moved.
        m_layout.setBoundsAndScale(textBounds.dimensions(), formContext.scaleFactor());
        return m_layout;
    }

    template <IsControl HostClass>
    float WithTextLayout<HostClass>::lineBreakWidth() const
    {
        return 0.0f;
    }

    template <IsControl HostClass>
    void WithTextLayout<HostClass>::takeText(const Text& text) const
    {
        m_layoutText = text;
        m_layout.setText(m_layoutText);
    }

    template <IsControl HostClass>
    TextStamp WithTextLayout<HostClass>::stampOf(const Text& answer) const
    {
        const ControlText& own = this->text();
        if (&answer != static_cast<const Text*>(&own))
            return {};

        return own.stamp();
    }

}
