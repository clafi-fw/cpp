export module SeeDocs_App.SidebarRows;

import SeeDocs_App.Icons;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export constexpr float k_rowIconGap = 5.0f;   // between a row's mark and its text

    // **The mark of the row's kind, painted ahead of the text** in room the text leaves it, so
    // an editor opened over the text covers the words alone.
    export template<typename Host>
    class WithMark : public Host
    {
    public:
        using Host::Host;
    public:
        void setMark(const RowIcon mark) { m_mark = mark; }
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void adjustTextRect(AdjustTextRectEvent&) const override;
        void paintText(PaintEvent&) override;
    private:
        static constexpr float k_markRoom = k_rowIconSize + k_rowIconGap;   // what the mark takes

        std::optional<RowIcon> m_mark{};
    };

    // The header of a sidebar item's node - a category's, or an All the APIs item's where the
    // folders hang under it: its name typed where it stands, its mark ahead of it.
    export class SidebarNodeHeader : public WithMark<WithInPlaceEdit<TreeNodeHeader>>
    {
    public:
        using WithMark::WithMark;
    };

    // The row of a page or an All the APIs item: its name typed where it stands.
    export class SidebarItemRow : public WithMark<WithInPlaceEdit<TreeItem>>
    {
    public:
        using WithMark::WithMark;
    };

    // A divider's row: a line across the text slot, which a pick lands on like any row.
    export class SidebarDividerRow : public TreeItem
    {
    public:
        using TreeItem::TreeItem;
    protected:
        void paintText(PaintEvent&) override;
    };


    //-------------------------------------------------------------------------


    template<typename Host>
    ScaledSize WithMark<Host>::calculateContent(AlignEvent& event)
    {
        ScaledSize result = Host::calculateContent(event);
        const float room = event.scale(k_markRoom);
        result.x += room;
        if (event.composedMinSize.x > 0.0f)
            event.composedMinSize.x += room;
        return result;
    }

    template<typename Host>
    void WithMark<Host>::adjustTextRect(AdjustTextRectEvent& event) const
    {
        Host::adjustTextRect(event);
        event.textBounds.left += event.scale(k_markRoom);
    }

    template<typename Host>
    void WithMark<Host>::paintText(PaintEvent& event)
    {
        if (m_mark.has_value())
        {
            const FloatRect bounds = this->textBounds(event);
            const float size = event.scaler().scale(k_rowIconSize);
            const float left = bounds.left - event.scaler().scale(k_markRoom);
            const float top = bounds.top + (bounds.height() - size) / 2.0f;
            PaintIconEvent icon{
                event.controlContext(),
                FloatRect{ left, top, left + size, top + size },
                event.control().tag(),
                event.disabledAmount()
            };
            paintRowIcon(icon, *m_mark);
        }
        Host::paintText(event);
    }
}
