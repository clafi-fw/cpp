export module ThisApp.Palette.Controls;

import ThisApp.Utils;
//
import ClaFi;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Types;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.Context.PaintIconEvent;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export using OnSelectPaletteMap = std::function<void()>;

    export struct ColorHarmonyItemInitializer
    {
        ColorHarmony& harmony;
        const OnSelectPaletteMap& onSelectPaletteMap;
    };

    using HarmonyItemClass = SplitButton;
    export class ColorHarmonyItem : public HarmonyItemClass
    {
    public:
        template <typename... Args>
        explicit ColorHarmonyItem(const CreateParams&, Args&&...);
        ColorHarmony& harmony() const { return m_harmony; }
    protected:
        void getTooltip(GetTooltipEvent&) override;
        void keyDown(KeyDownEvent&) override;
        void dropdown(DropdownEvent&) override;
        void paintIcon(PaintIconEvent&) override;
    private:
        class DropDownMenu : public StackPanel
        {
        public:
            DropDownMenu(const CreateParams&, const ColorHarmonyItem&);
            void getChildText(GetChildTextEvent&) const override;
            void click(ClickEvent&) override;
            void doubleClick(DoubleClickEvent& event) override { event.closeForm(); }
        private:
            static const TagValue kLabelTag = (std::numeric_limits<TagValue>::max)();
        private:
            const ColorHarmonyItem& m_owner;
            ColorHarmony& m_harmony{ m_owner.m_harmony };
        };
        class DropDownPanel : public Panel
        {
        public:
            DropDownPanel(const CreateParams&, const ColorHarmonyItem&);
        private:
            const ColorHarmonyItem& m_harmonyItem;
            Controls::Divider& m_separator{ createTopBar<Controls::Divider>(
                Padding{ 0.0f, 4.0f }
            ) };
            DropDownMenu& itemsView = createBody<DropDownMenu>(m_harmonyItem);
        };
    private:
        void dropDown(Control& dropDownButton);
        //void doubleClick(DoubleClickEvent&) override { dropDown(m_dropDownButton); }
        //void click(ClickEvent&) override
        //{
        //  //if (controller() == Controller::Keyboard) dropDown(m_dropDownButton);
        //}
        void mapSelected() const;
    private:
        ColorHarmony& m_harmony;
        const OnSelectPaletteMap& m_onSelectPaletteMap{};
        //RichControl& m_dropDownButton{ createRightBar<ToolButton>(
        //  Interactivity::MouseOnly,
        //  MinSize{ 8.0f },
        //  OnAdjustPaint{ [](AdjustPaintEvent& event) {
        //      event.setRoundCorners({ false, true, true, false });
        //  } }
        //  ) };
    };


    //----------------------------------------------------------------------------


    // ColorHarmonyItem::DropDownMenu

    ColorHarmonyItem::DropDownMenu::DropDownMenu(const CreateParams& params, const ColorHarmonyItem& item)
        :
        StackPanel{
            params,
            Orientation::HorizontalWrap,
            Interactivity::ActiveContainer,
            LaneSize{ 3 }
        },
        m_owner{ item }
    {
        // Not a tool, so not a ToolButton. A local class cannot carry a forwarding constructor -
        // no member templates in one - so the look is stated where the button is added.
        class MapButton : public Button
        {
            using Button::Button;
        protected:
            MinSize indicatorSize(const AppTheme&) const override { return { 16.0f }; }
        };

        // Create Items
        for (std::size_t i = 0ull; i != m_harmony.maps().size(); ++i)
        {
            MapButton& subItem{ add<MapButton>(
                Tag{ i },
                ShowSurfaceAtRest::No,
                IndicatorVisibility::Always,
                IndicatorStyle::Radio,
                ShowSelectionOnSurface::Yes,
                IconSize{ 32.0f },
                ButtonViewMode::IconOnly,
                OnPaintIcon{[this](PaintIconEvent& event){
                    std::size_t index = event.tag().value;
                    paintPaletteMap(event, m_harmony, m_harmony.maps()[index]);
                } }
                ) };
            if (m_harmony.selectedMap() == &m_harmony.maps()[i])
                setCurrentItem(&subItem);
        }
    }

    void ThisApp::ColorHarmonyItem::DropDownMenu::getChildText(GetChildTextEvent& event) const
    {
        const TagValue tagValue = event.control().tag().value;
        paintPaletteMap(event.text, m_harmony, m_harmony.maps()[tagValue]);
    }

    void ColorHarmonyItem::DropDownMenu::click(ClickEvent& event)
    {
        StackPanel::click(event);
        std::size_t tagValue = event.control->tag().value;
        m_harmony.selectMap(m_harmony.maps()[tagValue]);
        //invalidateChildrenStates();
        m_owner.mapSelected();
    }

    // ColorHarmonyItem::DropDownPanel

    // ColorHarmonyItem
    template<typename ...Args>
    ColorHarmonyItem::ColorHarmonyItem(const CreateParams& params, Args&&... args)
        :
        HarmonyItemClass{ params,
            ButtonViewMode::IconOnly,
            IconSize{ 32.f, 32.0f },
            ShowSurfaceAtRest::Yes,
            VerticalTextAnchor::Center,
            //params.theme().colors.button,
            //params.theme().metrics.listItem,
            Padding{ 0.0f },
            k_harmonyItemSize,
            std::forward<Args>(args)...
        },
        m_harmony{ Props::find<ColorHarmonyItemInitializer>(std::forward<Args>(args)...)->harmony },
        m_onSelectPaletteMap{ Props::find<ColorHarmonyItemInitializer>(std::forward<Args>(args)...)->onSelectPaletteMap }
    {
        if (selected())
            invalidateState();


        //m_dropDownButton.onClick([this](ClickEvent& params) { dropDown(*params.control);  });
        //m_dropDownButton.onPaint([](PaintEvent& event) {
        //  ChevronPainter::paint(
        //      event.canvas(),
        //      event.center(),
        //      textInk(InkGrade::Muted).toColor(event),
        //      event.selectedFactor(),
        //      event.scaleFactor()
        //  );
        //  });
    }

    void ColorHarmonyItem::getTooltip(GetTooltipEvent& event)
    {
        event.placement = FormPlacement::Bottom;
        event.text << m_harmony.name();
    }

    void ColorHarmonyItem::keyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Return:
            {
                event.handled = true;
                showDropdown(*this);
                //dropDown(*this);
                return;
            }
        }
        HarmonyItemClass::keyDown(event);
    }

    void ColorHarmonyItem::dropdown(DropdownEvent& event)
    {
        static_cast<StackPanel*>(parent())->setCurrentItem(this);
        event.executeDropdown<DropDownPanel>(*this);
    }

    void ColorHarmonyItem::paintIcon(PaintIconEvent& event)
    {
        paintPaletteMap(event, m_harmony, *m_harmony.selectedMap(), slantForIndex(static_cast<std::size_t>(m_harmony.kind())));
    }

    ColorHarmonyItem::DropDownPanel::DropDownPanel(const CreateParams& params, const ColorHarmonyItem& harmonyItem)
        :
        Panel{
            params,
            Interactivity::None,
            TextPlacement::Top,
            Padding{ 4.0f },
            Spacing{ 4.0f },
            UiElement::Menu,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow
        },
        m_harmonyItem{ harmonyItem }
    {
        text() << InkWell::textInk(InkGrade::Muted) << harmonyItem.harmony().name();
    }

    void ColorHarmonyItem::dropDown(Control& dropDownButton)
    {
        static_cast<StackPanel*>(parent())->setCurrentItem(this);

        Form<DropDownPanel> menuForm{ form().createPopup<DropDownPanel>(&dropDownButton, *this) };

        menuForm.setPlacement(FormPlacement::Bottom, boundsInForm());
        menuForm.body()->setFocus();
        menuForm.execute();
    }

    void ColorHarmonyItem::mapSelected() const
    {
        invalidate();
        if (m_onSelectPaletteMap)
            m_onSelectPaletteMap();
    }
}
