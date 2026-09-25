export module ClaFi.Core.Foundation :Navigation;

import :Control;
import :Form;
import :Spatial;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Context.FormContext;

namespace ClaFi
{
    enum class SearchMethod
    {
        WrapLanes,
        Spatial
    };

    enum class SearchFilter // by Interactivity
    {
        Focusable,
        DirectChildren
    };

    export class FocusNavigator
    {
    public:
        explicit FocusNavigator(FormBase& form) : m_form{ form } {}
        void formKeyDown(KeyDownEvent& event);
    private:
        // The form has no outside. It is an ActiveContainer so that Home, End and the page keys
        // have something to move within, and so that it can keep the item the focus returns to -
        // but it is never entered, never left, and never landed on, because the focus is already
        // inside it wherever it is. Every rule about crossing a container's edge has to say so.
        [[nodiscard]] bool isFormRoot(const Control*) const;
        Control* entryPoint(Control* candidate, const Control* sourceContainer,
            ScrollDirection entryEdge);
        // edge names which end of the container is wanted: ToBegin the first control in
        // navigation order, ToEnd the last.
        static Control* edgeItem(Control& container, ScrollDirection edge);
        Control* pageItem(Control& container, Control& focusedItem, ScrollDirection);
        // entryEdge is the end a container met along the way is entered at, which is the end the
        // step arrived from.
        Control* structuralSearch(KeyCode, ScrollDirection entryEdge, OrientedRect,
            const Control* searchRoot, Control* it);
        // lookAhead widens the walk beyond the viewport edge margin. A search whose source sits
        // outside the viewport must pass the distance it reaches over, since the walker visits
        // nothing further out than this and no candidate there can be scored.
        Control* spatialSearch(KeyCode, const OrientedRect& src,
            const Control* searchRoot, Control* excludeSubtree,
            SearchMethod, SearchFilter, float lookAhead = 0.0f);
        OrientedRect orientedRect(Control*, KeyCode);
    private:
        FormBase& m_form;
    };

}
