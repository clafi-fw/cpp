module ClaFi.Controls.ComboBox;

import ClaFi.Controls.TextItems;
import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        [[nodiscard]] std::wstring_view trimmed(std::wstring_view text)
        {
            trimLeft(text);
            trimRight(text);
            return text;
        }
    }

    // ComboboxAcceptTextEvent

    ComboboxAcceptTextEvent::ComboboxAcceptTextEvent(const ComboBox& combobox, AcceptEditEvent& accept)
        :
        ComboBoxEventBase{ combobox },
        accept{ accept }
    {
    }

    // ComboBox

    bool ComboBox::dropOnPrimaryPress() const
    {
        // Where a value is typed on the face, the strip keeps the list. Without a strip the face
        // keeps it, and the editor is the keyboard's.
        return m_editorMode != EditorMode::Editable || !shownSecondaryPart();
    }

    bool ComboBox::clickOpensEditor() const
    {
        // The first press already: the face holds nothing to pick before it is typed over.
        return !dropOnPrimaryPress();
    }

    void ComboBox::getEditorText(Text& text) const
    {
        text << valueText();
    }

    void ComboBox::acceptEditorText(AcceptEditEvent& event)
    {
        // A value left as it reads names nothing new. It is read now rather than as the editor
        // opened: the editor runs a loop of its own.
        const std::wstring typedText = typedForm(event.text);
        const std::wstring_view typed = trimmed(typedText);
        const std::wstring value = valueText();
        if (typed == trimmed(value))
            return;

        ComboboxAcceptTextEvent textEvent{ *this, event };
        emitEvent(textEvent);
        if (textEvent.propagationStopped() || event.refused())
            return;

        const ItemIndexValue named = lookUpItem(typed);
        if (named.has_value())
        {
            setItemIndex(named.value());
            return;
        }
        // An emptied value with no item standing for it names nothing, and there is nothing to
        // refuse either.
        if (!typed.empty())
            event.refuse(L"No item starts with that.");
    }

    FloatPoint ComboBox::editorMaxTextSize(const FloatRect&) const
    {
        // The face is one line, so the editor grows to the right with no ceiling of its own.
        return {};
    }

    const TextItems* ComboBox::editorSuggestions() const
    {
        return &items();
    }

    void ComboBox::keyDown(KeyDownEvent& event)
    {
        // Return edits an editable combobox wherever its list is - on its strip, or behind F4 and
        // Alt+Down when the face drops it.
        if (event.key == Keys::Return && m_editorMode == EditorMode::Editable && openEditor())
        {
            event.handled = true;
            return;
        }
        ComboboxBaseClass::keyDown(event);
    }

    void ComboBox::charPress(CharPressEvent& event)
    {
        // A space presses the face, and a character nobody can see starts no value.
        const wchar_t character = event.character();
        const bool startsValue = m_editorMode == EditorMode::Editable
            && character != L' '
            && std::iswprint(character);
        if (startsValue && openEditor({ &character, 1 }))
            return;
        ComboboxBaseClass::charPress(event);
    }

    void ComboBox::itemFaceText(const TextItem& item, Text& text, const EventPhase phase) const
    {
        adjustItemText(item, text, phase);
        if (text.empty() && !item.placeHolderText().empty())
            text << InkGrade::Muted << item.placeHolderText() << PopColor{};
    }

    std::wstring ComboBox::valueText() const
    {
        if (!itemIndex().has_value())
            return {};
        Text value{};
        adjustItemText(items()[itemIndex().value()], value, EventPhase::Paint);
        return typedForm(value);
    }

    ItemIndexValue ComboBox::lookUpItem(const std::wstring_view typed) const
    {
        // Nothing typed names the item that stands for no value: the one whose text is empty.
        if (typed.empty())
        {
            for (std::size_t i = 0; i != items().size(); ++i)
            {
                if (typedForm(items()[i].text()).empty())
                    return i;
            }
            return {};
        }

        // An item is named by its own text, or by its placeholder where it has none - never by
        // what its face adds. The whole name first, then the first name starting with what was
        // typed, case aside in both. The editor's suggestion list is filtered by the same two
        // functions, so it shows what this would take.
        ItemIndexValue startsWithTyped{};
        for (std::size_t i = 0; i != items().size(); ++i)
        {
            const std::wstring name = typedName(items()[i]);
            if (!startsWithFolded(name, typed))
                continue;
            if (name.size() == typed.size())
                return i;
            if (!startsWithTyped.has_value())
                startsWithTyped = i;
        }
        return startsWithTyped;
    }
}
