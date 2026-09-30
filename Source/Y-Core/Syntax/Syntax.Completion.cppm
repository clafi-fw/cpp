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

    // The call the caret stands in: what is called, and which argument. See Syntax#parameters
    export struct CompletionCall
    {
        std::size_t line{ 0 };       // the line the call's opening bracket stands on
        std::size_t bracket{ 0 };    // where the bracket stands in that line
        std::size_t argument{ 0 };   // the argument the caret stands in, the first being zero
        CompletionSubject callee;    // what is called, read the way a member's subject is
    };

    // The innermost call the caret stands in, or nothing where it stands in none: the walk back
    // from the caret, over the lines above, to the bracket left open. See Syntax#parameters
    export [[nodiscard]] std::optional<CompletionCall> completionCall(const Language&,
        const SourceLines&, std::size_t line, std::size_t caret);
    // The entry a call is to - a routine the text declares, in force at the caret, else one the
    // host offers, a member through its subject's classes - or null where none is known.
    export [[nodiscard]] const CompletionEntry* completionCallee(const Language&,
        const CompletionEntries&, const Declarations&, const CompletionSubject& callee,
        std::size_t caret);

    export using CompletionNames = std::vector<std::wstring_view>;

    // Where a completion on a line of a text's title block would go. See Syntax#titleblock
    export struct TitlePlace
    {
        std::size_t wordStart{ 0 };       // where the key or the value being written starts
        std::size_t caret{ 0 };
        const TitleKey* key{ nullptr };   // the key a value is written for - null for a key
        // What is left out: the keys on the block's other lines, or the line's other values.
        CompletionNames stated;
        // The key or the value as typed so far, which may be empty.
        [[nodiscard]] std::wstring_view typed(std::wstring_view line) const;
    };

    // The place a caret on a title block's line names, if it names one. See Syntax#titleblock
    export [[nodiscard]] std::optional<TitlePlace> completionTitlePlace(const Language&,
        const TitleBlock&, const SourceLines&, std::size_t line, std::size_t caret);
}
