module SeeDocs_App.Studio.Editing;

import SeeDocs_App.Edits;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr KeyCode k_saveKey = L'S';
    }

    // Actions

    namespace Actions
    {
        Action editHint{
            Text{ L"Edit hint" },
            Shortcut{ Keys::F2 }
        };
        // F2 as well: a box holds a hint or a footnote, never both, so one of the two claims it.
        Action editFootnote{
            Text{ L"Edit footnote" },
            Shortcut{ Keys::F2 }
        };
        Action addFootnote{
            Text{ L"Add footnote" }
        };

        void registerAll()
        {
            static bool registered = false;
            if (registered)
                return;
            registered = true;
            AppActions& actions = AppActions::get();
            actions.add(editHint);
            actions.add(editFootnote);
            actions.add(addFootnote);
        }
    }

    // FootnoteSlot

    void FootnoteSlot::edit(IPageWriter& writer, FootnoteDraft draft)
    {
        m_writer = &writer;
        m_draft = std::move(draft);
        buildEditor();
        const bool refused = !m_draft.refusal.empty();
        m_box->text() = Text{ m_draft.text };
        m_box->setVisible(!refused);
        m_save->setVisible(!refused);
        if (refused)
        {
            showRefusal(m_draft.refusal);
        }
        else
        {
            Text place;
            place << InkGrade::Muted << m_draft.place << PopColor{};
            m_place->text() = place;
        }
        m_shown.setVisible(false);
        m_editor->setVisible(true);
        m_editing = true;
        if (refused)
            m_cancel->setFocus();
        else
            m_box->setFocus();
        invalidateFormAlign();
    }

    void FootnoteSlot::nestedKeyDown(KeyDownEvent& event)
    {
        if (m_editing && !event.handled)
        {
            if (event.key == Keys::Escape)
            {
                event.handled = true;
                finish(false);
                return;
            }
            if (event.key == k_saveKey && event.modifiers.ctrl && !event.modifiers.alt)
            {
                event.handled = true;
                finish(true);
                return;
            }
        }
        Stack::nestedKeyDown(event);
    }

    // The editor is made the first time the footnote is edited and kept: a page holds many
    // footnotes, and most are never opened.
    void FootnoteSlot::buildEditor()
    {
        if (m_editor)
            return;
        m_editor = &add<Stack>(Orientation::Vertical, Spacing{ k_editorSpacing });
        m_box = &m_editor->add<TextBox>(
            TextFormat{ TextStyleId::Code },
            Padding{ k_editorPadding }
        );
        Stack& bar = m_editor->add<Stack>(Orientation::Horizontal, Spacing{ k_barSpacing });
        m_place = &bar.add<Label>(WordWrap::Yes);
        bar.add<FlexSpacer>();
        m_cancel = &bar.add<Button>(
            L"Cancel",
            HintText{ L"Leaves the footnote as it was - Escape" },
            Button::OnClick{ [this](ClickEvent&) {
                finish(false);
            } }
        );
        m_save = &bar.add<Button>(
            L"Save",
            HintText{ L"Writes the footnote into its note - Ctrl+S" },
            Button::OnClick{ [this](ClickEvent&) {
                finish(true);
            } }
        );
    }

    // A WRITE THAT WAS REFUSED KEEPS THE EDITOR UP, the words as typed and the reason in the bar,
    // so nothing typed is lost to a file that changed or a heading in the wrong place. A footnote
    // not written yet that is left goes with its slot.
    void FootnoteSlot::finish(const bool write)
    {
        if (write && m_box->visible())
        {
            const std::wstring refusal = m_writer->writeFootnote(m_draft,
                m_box->text().plainText());
            if (!refusal.empty())
            {
                showRefusal(refusal);
                return;
            }
        }
        m_editing = false;
        m_editor->setVisible(false);
        if (m_draft.placed.has_value() && !write)
        {
            setVisible(false);
            return;
        }
        m_shown.setVisible(true);
        m_shown.setFocus();
        invalidateFormAlign();
    }

    void FootnoteSlot::showRefusal(const std::wstring_view reason)
    {
        Text text;
        text << InkWell::redInk(InkGrade::Strong) << reason << PopColor{};
        m_place->text() = text;
        invalidateFormAlign();
    }
}
