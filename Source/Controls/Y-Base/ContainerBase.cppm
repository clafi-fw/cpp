export module ClaFi.Controls.Base.Container;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Controls
{

    // A control that holds other controls, and hosts overlay entries. See Item-Containers
    export class ContainerBase : public FormControlBase
    {
    private:
        template<IsControl ControlClass>
        using CastingLambda = std::function<ControlClass&(ControlPtr&)>;
    public:
        template<IsControl ControlClass>
        using ControlsAs = std::ranges::transform_view<std::ranges::ref_view<ControlCollection>, CastingLambda<ControlClass>>;
    public:
        template <typename... Args>
        explicit ContainerBase(const CreateParams&, Args&&...);
        ~ContainerBase() override;
    public:
        void setPlaceHolderText(const Text& value);
        // addOverlayControl only affects painting order and clipping:
        // Overlay controls are painted in a second pass on top of other controls
        // and are not clipped by their own bounds
        // (but still clipped by this container's bounds).
        // An overlay control must belong to this container's control tree;
        // otherwise, addOverlayControl has no effect.
        //
        // Which container holds the entry is therefore a choice about how far the second pass may
        // reach, and it need not be the control's own parent: a grid hands its header's entry to
        // the control the grid stands in, so that what the header draws around itself lands on
        // the page instead of stopping at the grid's edge. What hosting costs is that this
        // container's children are walked twice for every paint.
        bool addOverlayControl(const Control&, ClippingMode) override;
        void removeOverlayControl(const Control&) override;
        bool isLeaf() const override { return false; }
        void deleteControl(Control&) override;
        // PUTS A CHILD AT A PLACE IN THE ORDER, the rest keeping theirs. The collection's order is
        // the order the children are laid out in, and a container built one child at a time is in
        // the order they were asked for - so this is what a container whose order is given to it
        // from outside calls: a tab strip made a tab at a time standing in the order its data
        // lists them. An index past the end is the last place, and a control this container does
        // not hold is left alone.
        void moveControl(Control&, std::size_t index);
        //
        template<IsControl ControlClass>
        ControlsAs<ControlClass> controlsAs() // view of the controls with each casted to the ControlClass&
        {
            static CastingLambda<ControlClass> lambda = [](ControlPtr& item)->ControlClass& {
                return static_cast<ControlClass&>(*item);
                };
            return std::move(std::ranges::views::transform(m_controls, lambda));
        };
        // Index of the first control the predicate accepts. The predicate normally asks
        // something outside the container about the control: a StackView's selection spreads
        // over the panels nested inside it, so each of those panels is asked for its own
        // first selected item instead of the view being asked to search itself.
        template<typename Predicate>
        [[nodiscard]] std::optional<std::size_t> indexOfFirst(Predicate&&) const;
    protected:
        //  - gives an ICE, so reimplementing both controls()
        // using FormControlBase::controls;
        ControlSpan controls() override { return { m_controls }; }
        ControlSpanC controls() const { return { m_controls }; } // reimplemented because of ICE
        bool hasControls() const { return !m_controls.empty(); }
        void reserve(std::size_t value) { m_controls.reserve(value); }
        void clearControls() { m_controls.clear(); }
        std::span<const OverlayEntry> overlayControls() const override { return { m_overlayControls }; }
        void tryRemoveOverlayControl(const Control&);
        template<IsControl ControlClass>
        static ControlClass& setField(Control*& field, ControlClass& value);
        template <IsControl ControlClass, typename... Args>
        ControlClass& add(Args&&... args);
        virtual void controlAdded(Control&) {}
        // Whether nothing is shown, which is when the placeholder text stands in.
        [[nodiscard]] virtual bool isEmpty() const { return controls().empty(); }
        // An entry may name a control this container does not own, so the entry has to be dropped
        // wherever that control dies rather than only where it was held. ~Control walks every
        // ancestor, which makes this the one place that hears about all of them.
        void nestedControlDeleted(Control*) override;
        void getText(GetTextEvent&) const override;
    private:
        ControlCollection m_controls{};
        std::vector<OverlayEntry> m_overlayControls{};
        PlaceHolderText m_placeHolderText{};
    };

    template<typename ...Args>
    ContainerBase::ContainerBase(const CreateParams& params, Args && ...args)
        :
        FormControlBase{ params, std::forward<Args>(args)... }
    {
        Props::ifThereIs<PlaceHolderText>([&](const auto& value) {
            setPlaceHolderText(value);
            }, std::forward<Args>(args)...);
    }


    //-------------------------------------------------------------------------


    template<typename Predicate>
    std::optional<std::size_t> ContainerBase::indexOfFirst(Predicate&& predicate) const
    {
        std::size_t result = 0;
        for (const ControlPtr& control : controls())
        {
            if (predicate(*control))
                return result;

            ++result;
        }
        return {};
    }

    template<IsControl ControlClass>
    ControlClass& ContainerBase::setField(Control*& field, ControlClass& value)
    {
#ifdef DEBUG
        if (field)
            throw std::runtime_error("The field already assigned");
#endif
        return *static_cast<ControlClass*>(field = &value);
    }

    template<IsControl ControlClass, typename ...Args>
    ControlClass& ContainerBase::add(Args && ...args)
    {
        m_controls.emplace_back(std::make_unique<ControlClass>(CreateParams{ *this }, std::forward<Args>(args)...));
        ControlClass& result = static_cast<ControlClass&>(*m_controls.back());
        controlAdded(result);
        return result;
    }

    export using Container = ContainerBase;


    //-------------------------------------------------------------------------


    // ContainerBase

    ContainerBase::~ContainerBase()
    {
        releaseChildren();
    }

    void ContainerBase::setPlaceHolderText(const Text& value)
    {
        //if (m_placeHolderText == value)
        //  return;
        m_placeHolderText = value;
        invalidate();
    }

    bool ContainerBase::addOverlayControl(const Control& control, ClippingMode clippingMode)
    {
        m_overlayControls.emplace_back(&control, clippingMode);
        control.invalidate();
        return true;
    }

    void ContainerBase::removeOverlayControl(const Control& control)
    {
        tryRemoveOverlayControl(control);
        control.invalidate();
    }

    void ContainerBase::deleteControl(Control& control)
    {
        std::erase_if(m_controls, [&](ControlPtr& current) {
            return &control == &*current;
            });
        tryRemoveOverlayControl(control);
    }

    void ContainerBase::moveControl(Control& control, const std::size_t index)
    {
        const ControlCollection::iterator found = std::ranges::find_if(m_controls,
            [&control](const ControlPtr& current) {
                return &control == &*current;
            });
        if (found == m_controls.end())
            return;

        const std::ptrdiff_t from = found - m_controls.begin();
        const std::ptrdiff_t to = std::min(static_cast<std::ptrdiff_t>(index),
            static_cast<std::ptrdiff_t>(m_controls.size()) - 1);
        if (from == to)
            return;

        // Rotating the span between the two places moves the one control and carries the others
        // one place the other way, which is what leaves their order between them as it was.
        if (from < to)
            std::rotate(found, found + 1, m_controls.begin() + to + 1);
        else
            std::rotate(m_controls.begin() + to, found, found + 1);

        invalidateFormAlign();
    }

    void ContainerBase::tryRemoveOverlayControl(const Control& control)
    {
        std::erase_if(m_overlayControls, [&](const OverlayEntry& entry) {
            return &control == entry.control;
            });
    }

    void ContainerBase::nestedControlDeleted(Control* control)
    {
        FormControlBase::nestedControlDeleted(control);
        tryRemoveOverlayControl(*control);
    }

    void ContainerBase::getText(GetTextEvent& event) const
    {
        FormControlBase::getText(event);
        if (!m_placeHolderText.empty() && isEmpty())
            event.text << TextAlign::Center << InkWell::textInk(InkGrade::Muted) << m_placeHolderText;
    }

}
