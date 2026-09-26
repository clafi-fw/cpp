module ThisApp.Design2Page;

import ThisApp.ElementPage;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        using CategoryNode = ExpanderWith<StackPanel>;

        // A branch of the tree: its name, and the elements under it in the order they are listed.
        struct ElementCategory
        {
            std::wstring_view name;
            std::span<const UiElement> elements;
        };

        constexpr std::array k_windowRoots{
            UiElement::Form,
            UiElement::Menu,
            UiElement::Tooltip
        };

        constexpr std::array k_surfaces{
            UiElement::Page,
            UiElement::TabLine,
            UiElement::Section,
            UiElement::Header,
            UiElement::Divider,
            UiElement::Bar,
            UiElement::FormTitle,
            UiElement::Grid,
            UiElement::GridRow,
            UiElement::GridLine
        };

        constexpr std::array k_controls{
            UiElement::Button,
            UiElement::ScrollButton,
            UiElement::ScrollThumb
        };

        constexpr std::array k_focusAndSelection{
            UiElement::Accent,
            UiElement::Spot,
            UiElement::InactiveIndicator,
            UiElement::SelectedText
        };

        constexpr std::array k_testSubjects{
            UiElement::Testee,
            UiElement::Bestee
        };

        constexpr std::array k_elementCategories{
            ElementCategory{ L"Window roots", k_windowRoots },
            ElementCategory{ L"Surfaces", k_surfaces },
            ElementCategory{ L"Controls", k_controls },
            ElementCategory{ L"Focus and Selection", k_focusAndSelection },
            ElementCategory{ L"Test subjects", k_testSubjects }
        };

        // How far an element's name stands in from its category's.
        constexpr float k_itemIndent{ 16.0f };
    }

    OptionalUiElement Design2Page::pickedElement() const
    {
        if (const Control* item = m_tree.currentItem())
            return item->tag<UiElement>();
        return std::nullopt;
    }

    void Design2Page::pickElement(const UiElement element)
    {
        if (Control* item = m_items[static_cast<std::size_t>(element)])
            m_tree.setCurrentItem(item);
    }

    void Design2Page::buildTree()
    {
        m_tree.onCanFocusItem([this](CanFocusItemEvent& event) {
            event.canFocus = std::ranges::find(m_items, &event.item) != m_items.end();
        });
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            showPickedElement();
        });

        for (const ElementCategory& category : k_elementCategories)
        {
            CategoryNode& node = m_tree.add<CategoryNode>(
                HostProps{
                    VerticalAlign::Top,
                    ExpanderViewMode::TreeNode,
                    HeaderText{ category.name }
                },
                BodyProps{
                    Orientation::Vertical,
                    Padding{ k_itemIndent, 0.0f }
                }
            );
            for (const UiElement element : category.elements)
            {
                const std::size_t index = static_cast<std::size_t>(element);
                ToolButton& item = node.body().add<ToolButton>(
                    Text{ uiElementOf(element).name },
                    Tag{ element },
                    HorizontalTextAnchor::Left,
                    ShowSelectionOnSurface::Yes
                );
                m_items[index] = &item;
                m_elementPages[index] = &m_pages.add<ElementPage>(element);
                // The first element stands picked, so the body never opens empty.
                if (!m_tree.currentItem())
                    m_tree.setCurrentItem(item);
            }
        }
    }

    void Design2Page::showPickedElement()
    {
        if (const OptionalUiElement element = pickedElement())
            m_pages.setCurrentItem(m_elementPages[static_cast<std::size_t>(*element)]);
    }
}
