export module Themes_App.ApplyToControl;


import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What an Apply to control calls after it has changed its rule - a pick, so always settled.
    export using OnRuleChanged = std::function<void(EditPhase, const Text& what)>;

    // Which of a rule's two input sets a column edits - inputs or andInputs.
    export using RuleClause = RuleInputs ColorRule::*;

    // The channels a list's rules may write, any if empty.
    export using PaintChannels = std::span<const PaintChannel>;

    // A rule's Apply to cell: the channel it writes and the inputs it reads, set from one dropdown.
    export class ApplyToControl : public DropdownControlBase
    {
    public:
        template<typename... Args>
        explicit ApplyToControl(const CreateParams&, Args&&...);
    public:
        // Takes the rule by its place, the channels it may write, what to call and the edit's name.
        void bind(ColorRules&, std::size_t index, PaintChannels outputs, OnRuleChanged, Text what);
        [[nodiscard]] PaintChannel output() const;
        [[nodiscard]] bool canWrite(PaintChannel) const;
        [[nodiscard]] bool reads(RuleClause, RuleInput) const;
        [[nodiscard]] bool readsAny(RuleClause) const;
        void setOutput(PaintChannel);
        void toggleInput(RuleClause, RuleInput);
    protected:
        [[nodiscard]] bool dropOnPrimaryPress() const override { return true; }
        void showDropdown(Control&) override;
        void getMainText(GetTextEvent&) const override;
    private:
        [[nodiscard]] const ColorRule& rule() const;
        [[nodiscard]] ColorRule& rule();
        void changed();
    private:
        ColorRules* m_rules{}; // null until bound
        std::size_t m_index{};
        PaintChannels m_outputs{}; // the channels the rule may write
        OnRuleChanged m_onChanged{};
        Text m_what{}; // what an edit here is called, as the page records it
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ApplyToControl::ApplyToControl(const CreateParams& params, Args&&... args)
        :
        DropdownControlBase{ params,
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center,
            // Broken only at its own line end - the grid measures a row before it lays it out.
            WordWrap::No,
            std::forward<Args>(args)...
        }
    {
    }
}
