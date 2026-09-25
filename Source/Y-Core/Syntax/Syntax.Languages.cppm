export module ClaFi.Core.Syntax.Languages;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // The rules a table cannot state, one hook per language that has any. Defined in
    // Languages.cpp; a language names its hook here, which is what lets the language stand as a
    // constant.
    bool cppHook(Scan&);
    bool pascalHook(Scan&);
    bool claFiHook(Scan&);
    bool xmlHook(Scan&);

    // How each language tells a text as its own, defined beside its hook.
    Claim cppDetector(std::wstring_view text);
    Claim pascalDetector(std::wstring_view text);
    Claim jsonDetector(std::wstring_view text);
    Claim xmlDetector(std::wstring_view text);
    Claim claFiDetector(std::wstring_view text);
    Claim textDetector(std::wstring_view text);

    // Every table is written in code unit order, which is what a bisection reads it in, and the
    // assertion beside it is what catches a word put in out of place.
    namespace CppTables
    {
        constexpr auto keywords = std::to_array<std::wstring_view>({
            L"alignas", L"alignof", L"and", L"and_eq", L"asm", L"auto", L"bitand", L"bitor",
            L"break", L"case", L"catch", L"class", L"co_await", L"co_return", L"co_yield",
            L"compl", L"concept", L"const", L"const_cast", L"consteval", L"constexpr",
            L"constinit", L"continue", L"decltype", L"default", L"delete", L"do",
            L"dynamic_cast", L"else", L"enum", L"explicit", L"export", L"extern", L"final",
            L"for", L"friend", L"goto", L"if", L"import", L"inline", L"module", L"mutable",
            L"namespace", L"new", L"noexcept", L"not", L"not_eq", L"operator", L"or", L"or_eq",
            L"override", L"private", L"protected", L"public", L"register", L"reinterpret_cast",
            L"requires", L"return", L"sizeof", L"static", L"static_assert", L"static_cast",
            L"struct", L"switch", L"template", L"this", L"thread_local", L"throw", L"try",
            L"typedef", L"typeid", L"typename", L"union", L"using", L"virtual", L"volatile",
            L"while", L"xor", L"xor_eq"
        });
        static_assert(std::ranges::is_sorted(keywords));

        constexpr auto types = std::to_array<std::wstring_view>({
            L"bool", L"char", L"char16_t", L"char32_t", L"char8_t", L"double", L"float", L"int",
            L"int16_t", L"int32_t", L"int64_t", L"int8_t", L"intptr_t", L"long", L"ptrdiff_t",
            L"short", L"signed", L"size_t", L"uint16_t", L"uint32_t", L"uint64_t", L"uint8_t",
            L"uintptr_t", L"unsigned", L"void", L"wchar_t"
        });
        static_assert(std::ranges::is_sorted(types));

        constexpr auto constants = std::to_array<std::wstring_view>({
            L"false", L"nullptr", L"true"
        });
        static_assert(std::ranges::is_sorted(constants));

        constexpr auto comments = std::to_array<CommentRule>({
            { L"//", L"" },
            { L"/*", L"*/" }
        });

        constexpr auto strings = std::to_array<StringRule>({
            { L'"', L'"', L'\\' },
            { L'\'', L'\'', L'\\' }
        });
    }

    // Written in lower case, since the language reads a word in any case. The directives that
    // are also everyday names - read, write, index, name - are the hook's, and stand in its own
    // table.
    namespace PascalTables
    {
        constexpr auto keywords = std::to_array<std::wstring_view>({
            L"absolute", L"abstract", L"and", L"array", L"as", L"asm", L"assembler", L"begin",
            L"case", L"cdecl", L"class", L"const", L"constructor", L"delayed", L"deprecated",
            L"destructor", L"dispinterface", L"div", L"do", L"downto", L"dynamic", L"else", L"end",
            L"except", L"experimental", L"export", L"exports", L"external", L"file", L"final",
            L"finalization", L"finally", L"for", L"forward", L"function", L"goto", L"helper", L"if",
            L"implementation", L"in", L"inherited", L"initialization", L"inline", L"interface",
            L"is", L"label", L"library", L"mod", L"not", L"object", L"of", L"operator", L"or",
            L"out", L"overload", L"override", L"package", L"packed", L"pascal", L"private",
            L"procedure", L"program", L"property", L"protected", L"public", L"published", L"raise",
            L"readonly", L"record", L"reference", L"register", L"reintroduce", L"repeat",
            L"requires", L"resourcestring", L"safecall", L"sealed", L"self", L"set", L"shl", L"shr",
            L"static", L"stdcall", L"strict", L"then", L"threadvar", L"to", L"try", L"type",
            L"unit", L"unsafe", L"until", L"uses", L"var", L"varargs", L"virtual", L"while",
            L"winapi", L"with", L"writeonly", L"xor"
        });
        static_assert(std::ranges::is_sorted(keywords));

        constexpr auto types = std::to_array<std::wstring_view>({
            L"ansichar", L"ansistring", L"boolean", L"byte", L"bytebool", L"cardinal", L"char",
            L"comp", L"currency", L"double", L"extended", L"int16", L"int32", L"int64", L"int8",
            L"integer", L"longbool", L"longint", L"longword", L"nativeint", L"nativeuint",
            L"olevariant", L"pansichar", L"pansistring", L"pbyte", L"pchar", L"pinteger",
            L"pointer", L"pwidechar", L"rawbytestring", L"real", L"real48", L"shortint",
            L"shortstring", L"single", L"smallint", L"string", L"textfile", L"uint16", L"uint32",
            L"uint64", L"uint8", L"unicodestring", L"utf8string", L"variant", L"widechar",
            L"widestring", L"word", L"wordbool"
        });
        static_assert(std::ranges::is_sorted(types));

        constexpr auto constants = std::to_array<std::wstring_view>({
            L"false", L"nil", L"true"
        });
        static_assert(std::ranges::is_sorted(constants));

        constexpr auto comments = std::to_array<CommentRule>({
            { L"//", L"" },
            { L"{", L"}" },
            { L"(*", L"*)" }
        });
    }

    namespace JsonTables
    {
        constexpr auto constants = std::to_array<std::wstring_view>({
            L"false", L"null", L"true"
        });
        static_assert(std::ranges::is_sorted(constants));

        constexpr auto strings = std::to_array<StringRule>({
            { L'"', L'"', L'\\' }
        });
    }

    namespace XmlTables
    {
        constexpr auto comments = std::to_array<CommentRule>({
            { L"<!--", L"-->" }
        });
    }

    // The languages the framework colours. Each is a constant: a caller passes one by value and
    // keeps nothing alive for it.
    export namespace Languages
    {
        constexpr Language cpp{
            .name = L"C++",
            .keywords = CppTables::keywords,
            .types = CppTables::types,
            .constants = CppTables::constants,
            .comments = CppTables::comments,
            .strings = CppTables::strings,
            .operators = L"!%&*+-./:<=>?^|~",
            .punctuation = L"(){}[],;",
            .numbers = true,
            .typeRule = TypeRule::LeadingCapital,
            .callIsFunction = true,
            .hook = cppHook,
            .detect = cppDetector,
        };

        // Delphi's Object Pascal, and the scripts written in its subset. The tables state no
        // string: its escape is a doubled quote, which is the hook's to read, as are the compiler
        // directives inside a comment's brackets and the numbers spelled with $ and %.
        constexpr Language pascal{
            .name = L"Pascal",
            .keywords = PascalTables::keywords,
            .types = PascalTables::types,
            .constants = PascalTables::constants,
            .comments = PascalTables::comments,
            .operators = L"*+-./:<=>@^",
            .punctuation = L"()[],;",
            .numbers = true,
            .ignoreCase = true,
            .typeRule = TypeRule::PrefixedCapital,
            .callIsFunction = true,
            .hook = pascalHook,
            .detect = pascalDetector,
        };

        // Comments as well, since the files that hold settings carry them. Strict JSON has none,
        // and a document with none is read the same.
        constexpr Language json{
            .name = L"JSON",
            .constants = JsonTables::constants,
            .comments = CppTables::comments,
            .strings = JsonTables::strings,
            .punctuation = L"{}[],:",
            .numbers = true,
            .keyBeforeColon = true,
            .detect = jsonDetector,
        };

        // Text between the tags is plain, so the tables say nothing and the hook says the rest:
        // a name is a tag or an attribute by where it stands, not by what it spells.
        constexpr Language xml{
            .name = L"XML",
            .comments = XmlTables::comments,
            .nameChars = L"-.:",
            .numbers = false,
            .hook = xmlHook,
            .detect = xmlDetector,
        };

        // A line's first character says what the line is, so the hook takes every line whole and
        // the tables stay empty.
        constexpr Language claFi{
            .name = L"ClaFi",
            .hook = claFiHook,
            .detect = claFiDetector,
        };

        // Text read as text: no rule, every run plain, any text Possible - the name a list answers
        // for whatever nothing else claims.
        constexpr Language text{
            .name = L"Text",
            .detect = textDetector,
        };
    }
}
