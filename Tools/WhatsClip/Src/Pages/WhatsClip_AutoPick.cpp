module ClaFi.Tools.WhatsClip.AutoPick;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.TextItems;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    std::optional<std::size_t> AutoPicker::pickedIndex() const
    {
        if (isAuto())
            return std::nullopt;

        // The items stand one past the names: Auto is the first of them.
        return itemIndex().value() - 1;
    }

    void AutoPicker::setFoundName(const std::wstring_view value)
    {
        m_foundName = value;
        invalidate();
    }

    void AutoPicker::getMainText(GetTextEvent& event) const
    {
        if (!isAuto() || m_foundName.empty())
        {
            ComboBox::getMainText(event);
            return;
        }

        event.text << itemText(m_foundName, m_fontSize);
        event.text << InkGrade::Muted << k_autoNote << PopColor{};
    }

    bool AutoPicker::isAuto() const
    {
        const ItemIndexValue picked = itemIndex();
        return !picked.has_value() || picked.value() == k_autoItem;
    }

    Controls::TextItems AutoPicker::itemsFor(const Names& names, const float fontSize)
    {
        Controls::TextItems items{};
        items.push_back(Controls::TextItem{ itemText(L"Auto", fontSize) });
        for (const std::wstring_view name : names)
            items.push_back(Controls::TextItem{ itemText(name, fontSize) });

        return items;
    }

    Text AutoPicker::itemText(const std::wstring_view name, const float fontSize)
    {
        Text text{};
        text << PushFontSize{ fontSize };
        text << name;
        return text;
    }
}
