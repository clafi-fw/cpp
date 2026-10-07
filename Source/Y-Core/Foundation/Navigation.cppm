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
            Control* searchRoot, Control* it);
        // The walk is around the source, a screenful each way, wherever the viewport stands, and
        // it is rooted at searchRoot: the form content is the root only where there is none.
        Control* spatialSearch(KeyCode, const OrientedRect& src,
            Control* searchRoot, Control* excludeSubtree,
            SearchMethod, SearchFilter);
        OrientedRect orientedRect(Control*, KeyCode);
    private:
        FormBase& m_form;
    };

}
