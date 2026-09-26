export module ThisApp.RuleText;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The value a text typed over a rule value states, or nothing, refused with a reason or not.
    export [[nodiscard]] std::optional<ColorRuleValue> typedRule(AcceptEditEvent&,
        const ColorRuleValue& current);
}
