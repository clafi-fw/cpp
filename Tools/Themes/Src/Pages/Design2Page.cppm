export module ThisApp.Design2Page;

import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A theme's design under the new color theme architecture, one element page at a time.
    export class Design2Page : public Panel
    {
    public:
        template<typename... Args>
        explicit Design2Page(const CreateParams&, Args&&...);
    public:
        // The element whose page shows, and nothing before one is picked.
        [[nodiscard]] OptionalUiElement pickedElement() const;
        void pickElement(UiElement);
        // Connects a handler raised after another element is picked.
        template<typename F>
        EventConnection onElementPick(F&& callback);
    private:
        using ElementControls = std::array<Control*, k_uiElementCount>;
    private:
        void buildTree();
        void showPickedElement();
    private:
        ElementControls m_items{}; // each element's item in the tree, null where it has none
        ElementControls m_elementPages{};

        StackView& m_tree{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section
        ).createBody<StackView>(
            Orientation::Vertical,
            Padding{ 4.0f }
        ) };

        PageControl& m_pages{ createBody<PageControl>() };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    Design2Page::Design2Page(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        buildTree();
    }

    template<typename F>
    EventConnection Design2Page::onElementPick(F&& callback)
    {
        return m_tree.onCurrentItemChange(std::forward<F>(callback));
    }
}
