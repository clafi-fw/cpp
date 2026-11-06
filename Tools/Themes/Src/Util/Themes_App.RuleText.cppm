export module Themes_App.RuleText;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The saturation and luminosity every hue mark is drawn at. A rule names a hue and nothing
    // else, so two rules carrying the same hue have to read the same whatever the theme puts
    // around them. A control's face, a popup's items and an inherited row's dot draw against these.
    export constexpr float k_blobSaturation{ 0.5f };
    export constexpr float k_blobLuminosity{ 0.5f };

    // The look a rule's words take: an editor's face, or a row a style takes from a base, muted.
    export enum class RuleFace
    {
        Editor,
        Inherited
    };

    // The word a channel is read by, where a theme file's name for it is the serializer's.
    export [[nodiscard]] std::wstring_view channelLabel(PaintChannel);
    // The word an input is read by.
    export [[nodiscard]] std::wstring_view inputLabel(RuleInput);
    // What a value operation does, as its tooltip names it.
    export [[nodiscard]] std::wstring_view operationName(ColorRuleOp);
    // A label that starts what it is shown in, with a capital.
    export [[nodiscard]] std::wstring capitalised(std::wstring_view label);
    // The channel a rule writes and, under it, the states it reads. A rule at rest keeps a blank
    // second line where asked, so every row of a grid stands at one height.
    export void writeApplyTo(Text&, const ColorRule&, RuleFace, bool secondLineAtRest);
    // The first line of the above: the channel, and the word that leads to the states.
    export void writeApplyToChannel(Text&, const ColorRule&, RuleFace);
    // The second line of the above: the states the rule reads, and nothing at rest.
    export void writeApplyToCondition(Text&, const ColorRule&, RuleFace);
    // No change in words, the way the operation popups write their own No change item.
    export void writeNoChange(Text&, std::wstring_view word);
    // The character a hue operation is marked with: a palette entry's digit, or the mark the
    // value operations give Set for an exact hue.
    export [[nodiscard]] std::wstring_view hueMark(ColorRuleHueOp);
    // The colour a hue rule's blob is filled with: its hue, at the saturation and luminosity
    // every blob is drawn at.
    export [[nodiscard]] Color hueBlobColor(const ColorRuleHue&, const ThemeColors&);
    // A value rule as the mark of its operation and its number, or No change in words.
    export void writeValueRule(Text&, const ColorRuleValue&, RuleFace);
    // The value a text typed over a rule value states, or nothing, refused with a reason or not.
    export [[nodiscard]] std::optional<ColorRuleValue> typedRule(AcceptEditEvent&,
        const ColorRuleValue& current);
}
