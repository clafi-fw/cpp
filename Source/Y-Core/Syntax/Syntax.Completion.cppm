export module ClaFi.Core.Syntax.Completion;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // The kind's word, lower case - what a list prints beside a name.
    export [[nodiscard]] std::wstring_view completionKindName(CompletionKind);
    // The kind a word names, in any case, or nothing for a word that names none.
    export [[nodiscard]] std::optional<CompletionKind> completionKindOf(std::wstring_view word);

    // Where a completion on a line would go. See Syntax#completion
    export struct CompletionPlace
    {
        std::size_t wordStart{ 0 };   // where the name the caret ends starts - the caret with none
        std::size_t caret{ 0 };
        bool member{ false };         // a dot stands right before the name, so a member is meant
        // The name as typed so far, which may be empty.
        [[nodiscard]] std::wstring_view typed(std::wstring_view line) const;
    };

    // The place a caret on a line names.
    export [[nodiscard]] CompletionPlace completionPlace(const Language&, std::wstring_view line,
        std::size_t caret);
    // Whether a name begins with what was typed, in the language's case rule.
    export [[nodiscard]] bool completionMatches(const Language&, std::wstring_view name,
        std::wstring_view typed);
    // Whether one name lists before another, in the language's case rule.
    export [[nodiscard]] bool completionNameLess(const Language&, std::wstring_view left,
        std::wstring_view right);
    // Whether two names are one, in the language's case rule.
    export [[nodiscard]] bool completionNamesEqual(const Language&, std::wstring_view left,
        std::wstring_view right);
    // Whether the caret stands in a comment or a string of the line those tokens were read off,
    // where nothing is completed.
    export [[nodiscard]] bool completionBlocked(const Tokens& lineTokens, std::size_t caret);

    // A link of a member's subject: a name, and whether brackets follow it. See Syntax#completion
    export struct CompletionLink
    {
        std::wstring_view name;
        bool bracketed{ false };   // a call's or an index's brackets stand between it and its dot
    };

    export using CompletionSubject = std::vector<CompletionLink>;
    export using CompletionClasses = std::vector<const CompletionEntry*>;

    // What the name before a member's dot is read through, root first. See Syntax#completion
    export [[nodiscard]] CompletionSubject completionSubject(const Language&,
        std::wstring_view line, const Tokens& lineTokens, const CompletionPlace&);
    // The subject's class and its parents - none where its type is not known. See Syntax#completion
    export [[nodiscard]] CompletionClasses completionMemberClasses(const Language&,
        const CompletionEntries&, const Declarations&, const CompletionSubject&,
        std::size_t caret);
}
