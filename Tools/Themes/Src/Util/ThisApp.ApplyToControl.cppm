export module ThisApp.ApplyToControl;

import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What an Apply to control calls after it has changed its rule.
    export using OnRuleChanged = std::function<void()>;

    // Which of a rule's two input sets a column edits - inputs or andInputs.
    export using RuleClause = RuleInputs ColorRule2::*;

    // A rule's Apply to cell: the channel it writes and the inputs it reads, set from one dropdown.
    export class ApplyToControl : public DropdownControlBase
    {
    public:
        template<typename... Args>
        explicit ApplyToControl(const CreateParams&, Args&&...);
    public:
        // Takes the rule by its place, which a growing list keeps, and what to call after a change.
        void bind(ColorRules2&, std::size_t index, OnRuleChanged);
        [[nodiscard]] PaintChannel output() const;
        [[nodiscard]] bool reads(RuleClause, RuleInput) const;
        [[nodiscard]] bool readsAny(RuleClause) const;
        void setOutput(PaintChannel);
        void toggleInput(RuleClause, RuleInput);
    protected:
        [[nodiscard]] bool dropOnPrimaryPress() const override { return true; }
        void showDropdown(Control& initiator) override;
        void getMainText(GetTextEvent&) const override;
    private:
        [[nodiscard]] const ColorRule2& rule() const;
        [[nodiscard]] ColorRule2& rule();
        void changed();
    private:
        ColorRules2* m_rules{}; // null until bound
        std::size_t m_index{};
        OnRuleChanged m_onChanged{};
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
