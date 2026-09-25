module ClaFi.Core.Foundation;

import :Traversal;
import :Control;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    // TraversalContext

    TraversalContext::TraversalContext(
        ControlTreeWalker& owner,
        FormContext& env,
        TraversalContext* parentContext,
        FloatPoint parentContentPosition,
        const FloatRect& viewport,
        Control& control,
        const OnVisitControl& onVisitControl
    )
        :
        ControlEventBase{ env },
        m_owner{ owner },
        m_parent{ parentContext },
        m_viewport{ viewport },
        m_control{ control },
        m_onVisitControl{ onVisitControl }
    {
        // Calculating Clip and Control Bounds
        // The float offset goes on here and nowhere further down: everything a control reads off
        // its own context - the bounds it paints in, the viewport those bounds are clipped to,
        // the origin its children are measured from - then describes where the control is drawn
        // rather than where it was laid out, and a floating control needs no second idea of its
        // own geometry. See Control::floatOffset.
        m_controlBounds = FloatRect::fromDimensions(
            m_control.topLeft() + m_control.floatOffset() + parentContentPosition,
            m_control.dimensions());

        // The padding is what the control's own text and icons are inset by; the child padding is
        // where its children start. They are the same value unless the control separates them.
        m_padding = m_control.scaledPadding();
        m_contentPosition = m_controlBounds.topLeft() + m_control.childInset(formContext(), m_padding);

        m_viewport.intersectWith(m_controlBounds);
        AdjustViewportEvent event{ formContext(), m_control, m_viewport };
        m_control.doAdjustViewport(event);
        if (event.clipIntoParent && parentContext)
            m_viewport.intersectWith(parentContext->m_viewport);
    }

    TraversalContext::TraversalContext(ControlTreeWalker& owner, Control& control, const OnVisitControl& onVisitControl)
        :
        TraversalContext{
            owner,
            control.formContext(),
            nullptr,
            // What the root's own topLeft is measured from. It must not be the root's
            // boundsInForm(), which already carries that topLeft: passing it counts the origin
            // twice and every descendant inherits the shift. That mistake hides for a walker
            // rooted at the form content, which sits at the origin, and shows up only once a
            // walker is rooted deeper.
            control.parentContentOrigin(),
            // The root viewport is the part of the control its ancestors leave visible. A root
            // context has no parent context, so the intersection that clips a child into its
            // parent does not run here, and this rect is the only clip the subtree inherits.
            // The control's own bounds are the wrong rect for anything scrolled inside a
            // container - they span the whole content, and traverseChildren then scans the child
            // range from the content's first child all the way to the clip. A walker rooted at
            // the form content has no ancestors, so the two rects agree there.
            control.visibleRectInForm(),
            control,
            onVisitControl
        }
    {
    }

    TraversalContext::TraversalContext(TraversalContext& parentContext, Control& control)
        :
        TraversalContext{
            parentContext.m_owner,
            parentContext.formContext(),
            &parentContext,
            parentContext.m_contentPosition,
            parentContext.m_viewport,
            control,
            parentContext.m_onVisitControl
        }
    {
    }

    const FloatRect* TraversalContext::systemClipRect() const
    {
        return m_owner.systemClipRect();
    }

    FloatRect TraversalContext::controlClipRect() const
    {
        return systemClipRect() ?
                   FloatRect::intersection(m_viewport, *systemClipRect())
                   :
                   m_viewport;
    }

    void TraversalContext::traverse()
    {
        if (m_owner.clipMode() != ClipMode::None
            && systemClipRect() && !systemClipRect()->intersects(m_controlBounds))
            return;
        if (shouldVisit())
            m_onVisitControl(*this);
        if (m_owner.traversalMode() == TraversalMode::Auto)
            traverseChildren();
    }

    void TraversalContext::traverseChildren()
    {
        // Traverse Children
        ControlSpan::iterator begin = m_control.controls().begin();
        ControlSpan::iterator end = m_control.controls().end();
        if (begin == end)
            return;

        // Which children the range spans is a geometry question, and an unclipped walk does
        // not ask it: the range is the whole collection.
        const bool clipped = m_owner.clipMode() != ClipMode::None;

        // IN THE SPACE THE CHILDREN'S OWN TOPLEFTS ARE MEASURED IN, which is the content position
        // and not the bounds: a container with padding asked the range against a rect a whole
        // inset too far in, so the cull was stricter here than the one FormBase::controlAt makes
        // for the pointer - and a child the pointer could reach was dropped from the paint.
        FloatRect localViewPort = m_viewport;
        localViewPort.offset(-m_contentPosition);
        // No system clip rect is no clip at all, which is what traverse() above already
        // makes of one - the mode names the rect to clip by, and there is none to widen the
        // child range to.
        if (m_owner.clipMode() == ClipMode::SystemClipOnly && m_owner.systemClipRect())
        {
            FloatRect localClip = *m_owner.systemClipRect();
            localClip.offset(-m_contentPosition);
            localViewPort.unionWith(localClip);
        }

        if (float lookAhead = m_owner.lookAhead())
            localViewPort.inflate(lookAhead);

        ControlSpan::iterator it = clipped ? m_control.firstChildInViewport(localViewPort) : begin;
        for (; it != end; ++it)
        {
            Control& child = **it;
            if (child.visible())
            {
                if (clipped && child.isViewportEnd(localViewPort.right, localViewPort.bottom))
                    break;
                TraversalContext childContext{ *this, child };
                childContext.traverse();
            }
        }
    }

    bool TraversalContext::shouldVisit()
    {
        FloatRect localViewPort = m_viewport;
        if (float lookAhead = m_owner.lookAhead())
            localViewPort.inflate(lookAhead);
        return m_owner.clipMode() != ClipMode::ViewportAndSystemClip || localViewPort.intersects(m_controlBounds);
    }

    // ControlTreeWalker

    ControlTreeWalker::ControlTreeWalker(Control& rootControl, TraversalMode traversalMode, const FloatRect* systemClipRect)
        :
        m_rootControl{ rootControl },
        m_traversalMode{ traversalMode },
        m_systemClipRect{ systemClipRect }
    {
    }

    void ControlTreeWalker::traverse(const TraversalContext::OnVisitControl& onVisitControl)
    {
        TraversalContext rootContext{ *this, m_rootControl, onVisitControl };
        rootContext.traverse();
    }

}
