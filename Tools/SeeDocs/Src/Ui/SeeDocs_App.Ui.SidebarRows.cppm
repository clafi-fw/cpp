export module SeeDocs_App.Ui.SidebarRows;

import SeeDocs_App.Ui.Icons;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Icons.RowIcons;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
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
        void paintOwnText(PaintEvent&) override;
    private:
        // What the mark takes.
        static constexpr float k_markRoom = Icons::RowIcons::Size + k_rowIconGap;

        std::optional<RowIcon> m_mark{};
    };

    // The header of a category's node: its name typed where it stands, its mark ahead of it.
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
        void paintOwnText(PaintEvent&) override;
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
    void WithMark<Host>::paintOwnText(PaintEvent& event)
    {
        if (m_mark.has_value())
        {
            const FloatRect bounds = this->textBounds(event);
            const float size = event.scaler().scale(Icons::RowIcons::Size);
            const float left = bounds.left - event.scaler().scale(k_markRoom);
            const float top = bounds.top + (bounds.height() - size) / 2.0f;
            DrawIconEvent icon{
                event.controlContext(),
                FloatRect{ left, top, left + size, top + size },
                event.control().tag(),
                event.disabledAmount()
            };
            drawRowIcon(icon, *m_mark);
        }
        Host::paintOwnText(event);
    }
}
