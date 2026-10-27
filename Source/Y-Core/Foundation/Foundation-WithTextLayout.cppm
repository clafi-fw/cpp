module;
#include "../System/Switches.h"

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
    // A control that shapes its own text in place, for a text that changes. See Control-Foundation
    export template <IsRichControl HostClass>
    class WithTextLayout : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithTextLayout(const CreateParams&, Args&&...);
    public:
        // What a line set in one monospace font does with a character the font lacks.
        [[nodiscard]] CellFallback cellFallback() const { return m_cellFallback; }
        void setCellFallback(CellFallback);
    protected:
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
        ScaledSize measureText(AlignEvent&, ScaledSize maxDimensions, const Text&) override;
        // The layout, in step with the control's text, the rect it draws in and the scale.
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&,
            const FloatRect& textBounds) const;
        // The same, once the answer a gather gave is the control's text.
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&, const FloatRect& textBounds,
            const Text& answer);
        // The width the lines are broken at; zero, the standing answer, breaks them at the box.
        [[nodiscard]] virtual float lineBreakWidth() const;
        // Tells the layout the control's text at its present revision - the whole of it.
        virtual void takeText() const;
    protected:
        // Mutable because measuring is a const question, and the layout is the answer's working.
        mutable TextLayout m_layout;
    private:
        // The last measurement and everything its answer depends on. A wrapping control is
        // measured again whenever the align pass grants it a box, and a box granted the same one
        // twice is owed the same answer rather than a second shaping - so the question is what is
        // compared, and nothing here is read from the layout.
        struct Measurement
        {
            TextStamp text{};
            ScaledSize maxDimensions{ 0.0f, 0.0f };
            ScaleFactor scaleFactor{ 0.0f };
            float breakWidth{ 0.0f };
            bool wrap{ false };
            CellFallback cellFallback{ CellFallback::Native };
            bool editable{ false };
            std::uint64_t layoutGeneration{ 0 };
            bool operator==(const Measurement&) const = default;
        };
    private:
        // What a gather answered becomes the control's text, where it says something else.
        void takeAnswer(const Text& answer);
    private:
        mutable TextStamp m_layoutStamp{}; // the revision of the control's text the layout holds
        Measurement m_measurement{};
        ScaledSize m_measuredTextSize{};
        TextFormat m_measuredFormat{}; // what the last measurement was laid out over
        // The engine generation the layout was built at. invalidateLayouts drops the cache and
        // moves that number on for anything a key does not carry - the font table above all - and
        // this layout is not in the cache to be dropped.
        mutable std::uint64_t m_layoutGeneration{ 0 };
        CellFallback m_cellFallback{ CellFallback::Native };
    };


//-----------------------------------------------------------------------------


    template <IsRichControl HostClass>
    template <typename... Args>
    WithTextLayout<HostClass>::WithTextLayout(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... }
    {
        // Pointed at the control's own text once. From here on syncedLayout keeps the layout in
        // step with the text's revision, and nothing else is ever stated to it.
        m_layout.setText(this->text());
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::setCellFallback(const CellFallback value)
    {
        if (m_cellFallback == value)
            return;

        m_cellFallback = value;
        this->invalidateFormAlign();
    }

    template <IsRichControl HostClass>
    DrawTextResult WithTextLayout<HostClass>::drawText(PaintEvent& event,
        const FloatRect& textBounds, const Text& answer)
    {
        TextLayout& layout = syncedLayout(event.formContext(), textBounds, answer);
#if CLAFI_TEXT_MOVING
        return layout.draw(
            event.controlContext(),
            anchoredOrigin(textBounds, layout.calculatedDimensions(), this->textAnchor()),
            this->editProps(),
            this->textRenderMode()
        );
#else
        return layout.draw(
            event.controlContext(),
            anchoredOrigin(textBounds, layout.calculatedDimensions(), this->textAnchor()),
            this->editProps()
        );
#endif
    }

    // Measured on the layout the paint will draw from. Through TextEngine's cache instead, a
    // control building a text of its own gets a **second** layout of it: the key carries the text's
    // hash, so a control whose text moved misses on every measurement, shapes it again to answer
    // one question, and leaves the entry for the evictor to sweep.
    template <IsRichControl HostClass>
    ScaledSize WithTextLayout<HostClass>::measureText(AlignEvent& event,
        ScaledSize maxDimensions, const Text& answer)
    {
        // Ahead of the stamp, which is the answer's once the answer is the control's text.
        takeAnswer(answer);
        const Measurement measurement{
            .text = this->text().stamp(),
            .maxDimensions = maxDimensions,
            .scaleFactor = event.scaleFactor(),
            .breakWidth = this->lineBreakWidth(),
            .wrap = this->wordWrap(),
            .cellFallback = m_cellFallback,
            .editable = this->editProps() != nullptr,
            .layoutGeneration = Control::textEngine().layoutGeneration(),
        };
        const TextFormat* format = this->textFormat();
        if (measurement == m_measurement && m_measuredFormat.matches(format))
            return m_measuredTextSize;

        // Only the size reaches the layout, so the rect is built at the origin - see syncedLayout,
        // which reads nothing else from it.
        m_measuredTextSize = syncedLayout(
            event.formContext(),
            FloatRect::FromDimensions({ 0.0f, 0.0f }, maxDimensions)
        ).calculatedDimensions();
        m_measurement = measurement;
        if (!m_measuredFormat.matches(format))
            m_measuredFormat = format ? *format : TextFormat{};
        return m_measuredTextSize;
    }

    template <IsRichControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds) const
    {
        // The stamp says whether the text was written since the layout took it, without reading
        // the text. Compared rather than taken every time: setText invalidates, so a control
        // measured four times in one pass would rebuild four times.
        const TextStamp stamp = this->text().stamp();
        if (stamp != m_layoutStamp)
        {
            takeText();
            m_layoutStamp = stamp;
        }
        if (m_layoutGeneration != Control::textEngine().layoutGeneration())
        {
            m_layoutGeneration = Control::textEngine().layoutGeneration();
            m_layout.invalidate();
        }
        m_layout.setFormat(this->textFormat());
        // Set here rather than in the constructor: it is a property, and a control told to stop
        // wrapping after it was built draws the answer to that on the next paint.
        m_layout.setWrap(this->wordWrap());
        m_layout.setCellFallback(m_cellFallback);
        // Ahead of the bounds, which are accepted or refused against it.
        m_layout.setBreakWidth(this->lineBreakWidth());
        m_layout.setHorizontalAnchor(this->horizontalTextAnchor());
        // Only the size: where the rect sits is the caller's business and the anchor's, and a
        // control scrolled sideways would otherwise rebuild for every pixel it moved.
        m_layout.setBoundsAndScale(textBounds.dimensions(), formContext.scaleFactor());
        return m_layout;
    }

    template <IsRichControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds, const Text& answer)
    {
        takeAnswer(answer);
        return syncedLayout(formContext, textBounds);
    }

    template <IsRichControl HostClass>
    float WithTextLayout<HostClass>::lineBreakWidth() const
    {
        return 0.0f;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::takeText() const
    {
        m_layout.setText(this->text());
    }

    // The layout draws the control's text and nothing else, so an answer a gather built - a
    // handler's or a parent's words - is made that text. Compared first: a built answer has no
    // stamp, and one saying what the text already says would move the stamp on for nothing.
    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::takeAnswer(const Text& answer)
    {
        const Text& own = this->text();
        if (&answer != &own && answer != own)
            this->text() = answer;
    }

}
