module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Callout;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // What a callout lays out with its blocks. See Controls#calloutshape
    export enum class CalloutShape
    {
        Marked, // the kind's mark at the left of the blocks
        Banner, // a strip naming the kind, its mark first, over the blocks
        Plain   // the blocks alone
    };

    // A note set apart from the prose around it, holding blocks of its own. See Controls#callout
    export class Callout : public PanelBase
    {
    public:
        template<typename... Args>
        explicit Callout(const CreateParams&, Args&&...);
    public:
        // What the callout says it is, which picks its colours and its mark.
        DECLARE_PROPERTY(MessageIcon, kind, MessageIcon::Information)
        // What the callout lays out with its blocks.
        DECLARE_PROPERTY(CalloutShape, shape, CalloutShape::Marked)
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"Callout"; }
        // The stack the blocks are added to, one under another.
        [[nodiscard]] StackPanel& blocks() { return m_blocks; }
    protected:
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void paintFrame(PaintEvent&) override;
    private:
        // The mark in a bar of its own or leading the banner, as the shape says.
        void addMark();
    private:
        static constexpr float k_markSize = 20.0f; // a message bar's icon size
        StackPanel& m_blocks;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    Callout::Callout(const CreateParams& params, Args&&... args)
        :
        PanelBase{
            params,
            MetricsSet::Callout,
            // The kind, read ahead of its property, picks the element the callout wears.
            calloutElementOf(READ_PROPERTY(MessageIcon, MessageIcon::Information)),
            std::forward<Args>(args)...
        },
        INIT_PROPERTY(kind),
        INIT_PROPERTY(shape),
        m_blocks{ createBody<StackPanel>(
            Orientation::Vertical,
            VerticalAlign::Top
        ) }
    {
        addMark();
    }
}
