export module ClaFi.Core.Foundation :Traversal;

import ClaFi.Core.Context.FormContext;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    export class Control;
    export class ControlTreeWalker;

    // Whether the visitor walks the children or the traversal does. See Control-Foundation
    export enum class TraversalMode
    {
        Manual,
        Auto
    };

    export class TraversalContext : public ControlEventBase
    {
        friend ControlTreeWalker;
    public:
        using OnVisitControl = std::function<void(TraversalContext&)>;
    public:
        TraversalContext(ControlTreeWalker& owner, Control&, const OnVisitControl&);
        TraversalContext(TraversalContext& parentEvent, Control&);
    public:
        const FloatRect* systemClipRect() const;
        const TraversalContext* parent() { return m_parent; }
        Control& control() { return m_control; }
        const Control& control() const { return m_control; }
        FloatRect controlBounds() const { return m_controlBounds; }
        float left() const { return m_controlBounds.left; }
        float top() const { return m_controlBounds.top; }
        float right() const { return m_controlBounds.right; }
        float bottom() const { return m_controlBounds.bottom; }
        float width() const { return m_controlBounds.width(); }
        float height() const { return m_controlBounds.height(); }
        FloatPoint topLeft() const { return m_controlBounds.topLeft(); }
        FloatPoint center() const { return m_controlBounds.center(); }
        FloatRect centerRect(float x, float y) const { return m_controlBounds.centerRect(x, y); }
        FloatRect centerRect(float size) const { return m_controlBounds.centerRect(size); }
        ScaledPadding padding() const { return m_padding; }
        FloatPoint contentPosition() const { return m_contentPosition; }
        FloatRect viewport() const { return m_viewport; }
        FloatRect controlClipRect() const;
        void* visitorData() const { return m_visitorData; }
        void setVisitorData(void* value) { m_visitorData = value; }
        void traverse();
        void traverseChildren();
    protected:
        TraversalContext(
            class ControlTreeWalker& owner,
            FormContext& env,
            TraversalContext* parentContext,
            FloatPoint parentContentPosition,
            const FloatRect& viewport,
            Control&,
            const OnVisitControl&
        );
    private:
        bool shouldVisit();
    private:
        ControlTreeWalker& m_owner;
        TraversalContext* m_parent{};
        FloatRect m_viewport;
        Control& m_control;
        const OnVisitControl& m_onVisitControl;
        FloatRect m_controlBounds;
        ScaledPadding m_padding;
        FloatPoint m_contentPosition;
        void* m_visitorData{ nullptr };
    };

    // How much of the subtree a walk reaches, and what narrows it.
    export enum class ClipMode{
        ViewportAndSystemClip,
        SystemClipOnly,
        None // every control in the subtree, whatever the geometry says. See Control-Foundation
    };

    export class ControlTreeWalker
    {
    public:
        ControlTreeWalker(Control&, TraversalMode, const FloatRect* systemClipRect = nullptr);
        const FloatRect* systemClipRect() { return m_systemClipRect; }
        TraversalMode traversalMode() const { return m_traversalMode; }
        void setClipMode(ClipMode value) { m_clipMode = value; }
        ClipMode clipMode() const { return m_clipMode; }
        void traverse(const TraversalContext::OnVisitControl&);
        void setLookAhead(float value) { m_lookAhead = value; }
        float lookAhead() const { return m_lookAhead; }
    private:
        const FloatRect* m_systemClipRect;
        Control& m_rootControl;
        TraversalMode m_traversalMode;
        ClipMode m_clipMode{ ClipMode::ViewportAndSystemClip };
        float m_lookAhead{};
    };


}
