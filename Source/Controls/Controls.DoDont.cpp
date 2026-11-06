module ClaFi.Controls.DoDont;

import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        using namespace ::ClaFi::Graphics;

        // A mark is the size of the words it leads, as a message's icon is.
        constexpr float k_markSize = 14.0f;
        constexpr float k_markSpacing = 5.0f;

        // Strokes from point to point of the icon's box, each point given as a share of its sides.
        void drawStrokes(DrawIconEvent& event, const std::initializer_list<FloatPoint> points,
            const Ink& ink)
        {
            const FloatRect& box = event.iconRect();
            const float lineWidth = event.scaledStrokeWidth(Thickness::Regular);
            PixelPath path;
            bool first = true;
            for (const FloatPoint point : points)
            {
                const FloatPoint at = {
                    box.left + box.width() * point.x,
                    box.top + box.height() * point.y
                };
                if (first)
                    path.moveTo(at);
                else
                    path.lineTo(at);
                first = false;
            }
            event.canvas().drawPath(path, { PathDrawLayer::Stroke(event.inkRgb(ink), lineWidth) });
        }

        void drawDoMark(DrawIconEvent& event)
        {
            drawStrokes(event, { { 0.15f, 0.55f }, { 0.4f, 0.8f }, { 0.88f, 0.22f } },
                InkWell::Green);
        }

        void drawDontMark(DrawIconEvent& event)
        {
            drawStrokes(event, { { 0.2f, 0.2f }, { 0.8f, 0.8f } }, InkWell::Red);
            drawStrokes(event, { { 0.2f, 0.8f }, { 0.8f, 0.2f } }, InkWell::Red);
        }

        // What leads an item under a heading, the heading naming the side.
        void drawBullet(DrawIconEvent& event)
        {
            const float radius = event.iconWidth() * 0.15f;
            event.canvas().fillCircle(event.iconCenter(), radius,
                event.textRgb(InkGrade::Strong));
        }

        [[nodiscard]] DrawIconFunc markOf(const DoDontSide side)
        {
            return side == DoDontSide::Do ? drawDoMark : drawDontMark;
        }

        [[nodiscard]] UiElement elementOf(const DoDontSide side)
        {
            return side == DoDontSide::Do ? UiElement::Do : UiElement::Dont;
        }

        [[nodiscard]] std::wstring_view nameOf(const DoDontSide side)
        {
            return side == DoDontSide::Do ? L"Do" : L"Don't";
        }

        // The words led by the mark, as one text, so the mark keeps to the first line.
        [[nodiscard]] Text ledBy(const DrawIconFunc& mark, const Text& words)
        {
            Text result{};
            result << InTextIcon{ k_markSize, mark } << Space{ k_markSpacing } << words;
            return result;
        }
    }

    // DoDont

    void DoDont::addItem(const DoDontSide side, const Text& words)
    {
        const DrawIconFunc mark = shape() == DoDontShape::Marks ? markOf(side) : drawBullet;
        sideOf(side).add<Label>(ledBy(mark, words), WordWrap::Yes);
        invalidateFormAlign();
    }

    // Read off the theme as the block is measured, so a switch of sizes reaches it. A spacing the
    // block was given is its own.
    void DoDont::adjustMetrics(AdjustMetricsEvent& event) const
    {
        StackPanelBase::adjustMetrics(event);
        if (statedMetrics().has(MetricsField::Spacing))
            return;
        const Spacing setSpacing = event.styleMetrics(UiElement::Do).spacing;
        switch (shape())
        {
            case DoDontShape::Columns:
                event.metrics.spacing = { setSpacing.x, 0.0f };
                break;
            case DoDontShape::Stacked:
                event.metrics.spacing = { 0.0f, setSpacing.x };
                break;
            case DoDontShape::Marks:
                event.metrics.spacing = { 0.0f, setSpacing.y };
                break;
        }
    }

    // **Under Marks a side is a run of the one list**, with no surface and no room round it.
    void DoDont::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        StackPanelBase::adjustChildMetrics(event);
        if (shape() != DoDontShape::Marks)
            return;
        event.metrics.spacing = { 0.0f, event.styleMetrics(UiElement::Do).spacing.y };
    }

    bool DoDont::isChildWidthGiven(const Control& child) const
    {
        return shape() == DoDontShape::Columns || StackPanelBase::isChildWidthGiven(child);
    }

    // **Columns divide the width the block was given**, where a row of its own would be as wide
    // as the sides measured: the words in each side wrap at its share, and the next measure
    // reads the share back.
    void DoDont::alignContent(AlignEvent& event, const ScaledPosition position,
        ScaledSize& contentDimensions)
    {
        if (shape() != DoDontShape::Columns)
        {
            StackPanelBase::alignContent(event, position, contentDimensions);
            return;
        }
        const float gap = event.scaledSpacingX();
        const float doWidth = std::max(0.0f, std::round((contentDimensions.x - gap) / 2.0f));
        const float dontWidth = std::max(0.0f, contentDimensions.x - doWidth - gap);
        alignControl(&m_doSide, event, position, { doWidth, contentDimensions.y });
        alignControl(&m_dontSide, event, { position.x + doWidth + gap, position.y },
            { dontWidth, contentDimensions.y });
    }

    Orientation DoDont::orientationOf(const DoDontShape shape)
    {
        return shape == DoDontShape::Columns ? Orientation::Horizontal : Orientation::Vertical;
    }

    // Columns share the width evenly, whatever each side holds.
    ItemSizing DoDont::itemSizingOf(const DoDontShape shape)
    {
        return shape == DoDontShape::Columns ? ItemSizing::Equal : ItemSizing::Natural;
    }

    StackPanel& DoDont::addSide(const DoDontSide side)
    {
        if (shape() == DoDontShape::Marks)
            return add<StackPanel>(Orientation::Vertical);
        StackPanel& result = add<StackPanel>(
            Orientation::Vertical,
            elementOf(side)
        );
        result.add<Label>(ledBy(markOf(side), Text{ BoldText{ nameOf(side) } }), WordWrap::Yes);
        return result;
    }

    StackPanel& DoDont::sideOf(const DoDontSide side)
    {
        return side == DoDontSide::Do ? m_doSide : m_dontSide;
    }
}
