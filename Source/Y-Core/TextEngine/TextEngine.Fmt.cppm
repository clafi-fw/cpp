export module ClaFi.Core.TextEngine.Fmt;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Tags;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export template<typename... Args>
    struct Fmt
    {
        explicit constexpr Fmt(std::wstring_view s, const Args&... a)
            :
            str{ s },
            args{ a... }
        {}
        const std::wstring_view str;
        const std::tuple<const Args&...> args;
    };
    template<typename... Args>
    Fmt(std::wstring_view, Args&&...) -> Fmt<Args...>;

    export template<typename... Args>
        Text fmtText(std::wstring_view s, const Args&&... a)
    {
        return Text{ Fmt{ s, std::forward<Args>(a)... } };
    }

    struct FmtArg
    {
        const void* ptr;
        void (*handler)(Text&, std::wstring_view, const void*);
    };

    void dispatchInt32(Text&, std::wstring_view spec, const void* p);
    void dispatchUInt32(Text&, std::wstring_view spec, const void* p);
    void dispatchInt64(Text&, std::wstring_view spec, const void* p);
    void dispatchUInt64(Text&, std::wstring_view spec, const void* p);
    void dispatchDouble(Text&, std::wstring_view spec, const void* p);
    void dispatchFloat(Text&, std::wstring_view spec, const void* p);
    void processFmtCore(Text&, std::wstring_view fmt, const FmtArg* args, std::size_t argCount);

    template<typename T>
    void dispatchDirect(Text& tb, std::wstring_view /*spec*/, const void* p)
    {
        tb << *static_cast<const T*>(p);
    }
    void dispatchColor(Text& tb, std::wstring_view spec, const void* p)
    {
        const ClaFi::Color& c = *static_cast<const ClaFi::Color*>(p);
        if (spec == L"push_color")
            tb << PushCustomColor{ c };
        else
            tb << c.toStr();
    }

    // The spec is the extent behind a placeholder word: "{} 24,24,20" from [icon {} 24,24,20],
    // "icon 18,18" from {icon 18,18}, and nothing at all from a bare {}.
    template<typename T>
    void dispatchIconPainter(Text& tb, std::wstring_view spec, const void* p)
    {
        const std::size_t spacePos = spec.find(L' ');
        const std::wstring_view extent = spacePos == std::wstring_view::npos ? std::wstring_view{} : spec.substr(spacePos + 1);
        InTextIcon icon = iconOf(extent);
        icon.paintLambda = *static_cast<const T*>(p);
        tb << icon;
    }

    template<typename T>
    auto getTextHandler() {
        // Things here (in thes module) are not in the most possible cleanest C++ way because of MSVC ICEs
        using DT = std::decay_t<T>;
        // elses are needed for "if constexpr" in spite of returns
        if constexpr (std::is_same_v<DT, int>)
            return dispatchInt32;
        else if constexpr (std::is_same_v<DT, unsigned int>)
            return dispatchUInt32;
        else if constexpr (std::is_same_v<DT, long long>)
            return dispatchInt64;
        else if constexpr (std::is_same_v<DT, unsigned long long>)
            return dispatchUInt64;
        else if constexpr (std::is_same_v<DT, float>)
            return dispatchFloat;
        else if constexpr (std::is_same_v<DT, double>)
            return dispatchDouble;
        else if constexpr (std::is_same_v<DT, ClaFi::Color>)
            return dispatchColor;
        else if constexpr (requires(DT t, PaintIconEvent & e) { t(e); })
            return dispatchIconPainter<T>;
        else
            return dispatchDirect<T>;
    }

    export template<typename... Args>
    Text& operator<<(Text& tb, const Fmt<Args...>& f)
    {
        if constexpr (constexpr std::size_t count = sizeof...(Args); count == 0)
        {
            processFmtCore(tb, f.str, nullptr, 0);
        }
        else
        {
            std::apply([&tb, &f](const auto&... args) {
                FmtArg argArray[] = {
                    {
                        static_cast<const void*>(&args),
                        getTextHandler<std::remove_reference_t<decltype(args)>>()
                    }...
                };
                processFmtCore(tb, f.str, argArray, count);
                }, f.args);
        }
        return tb;
    }

    template<typename T>
    void doFormat(Text& tb, std::wstring_view spec, const void* p)
    {
        wchar_t fbuf[64];
        fbuf[0] = L'{';
        std::size_t slen = spec.size() > 60 ? 60 : spec.size();
        if (slen > 0)
            std::memcpy(&fbuf[1], spec.data(), slen * sizeof(wchar_t));
        fbuf[slen + 1] = L'}';
        fbuf[slen + 2] = L'\0';
        try
        {
            const T& val = *static_cast<const T*>(p);
            std::wstring res = std::vformat(
                std::wstring_view(fbuf, slen + 2),
                std::make_wformat_args(val)
            );
            tb << res;
        }
        catch (...) { tb << L"{FmtErr}"; }
    }

    void dispatchInt32(Text& tb, std::wstring_view spec, const void* p) { doFormat<int>(tb, spec, p); }
    void dispatchUInt32(Text& tb, std::wstring_view spec, const void* p) { doFormat<unsigned int>(tb, spec, p); }
    void dispatchInt64(Text& tb, std::wstring_view spec, const void* p) { doFormat<long long>(tb, spec, p); }
    void dispatchUInt64(Text& tb, std::wstring_view spec, const void* p) { doFormat<unsigned long long>(tb, spec, p); }
    void dispatchDouble(Text& tb, std::wstring_view spec, const void* p) { doFormat<double>(tb, spec, p); }

    void dispatchFloat(Text& text, std::wstring_view spec, const void* p)
    {
        doFormat<float>(text, spec, p);
    }

    // A marker put into the text as itself. Two are stated through other spellings of Text's
    // stream: a style is written as its TextStyleId, and a line spacing through setLineSpacing.
    void streamFormatItem(Text& tb, const FormatItem& item)
    {
        std::visit([&tb](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, PushTextStyle>)
                tb << value.style;
            else if constexpr (std::is_same_v<T, SetLineSpacing>)
                tb.setLineSpacing(value.spacing);
            else
                tb << value;
        }, item);
    }

    void processFmtCore(Text& tb, std::wstring_view fmt, const FmtArg* args, std::size_t argCount) {
        std::size_t argIdx = 0, pos = 0;
        while (pos < fmt.size())
        {
            std::size_t next = fmt.find_first_of(L"[{\n", pos);
            if (next > pos)
                tb << fmt.substr(pos, next - pos);
            if (next == std::wstring_view::npos)
                break;

            wchar_t opener = fmt[next];
            if (opener == L'\n')
            {
                tb << TextOp::EndLine;
                pos = next + 1;
                continue;
            }

            wchar_t closer = (opener == L'[') ? L']' : L'}';
            std::size_t end = fmt.find(closer, next);
            if (end == std::wstring_view::npos)
                break;

            std::wstring_view content = fmt.substr(next + 1, end - next - 1);

            if (opener == L'[') {
                auto firstSpace = content.find(L' ');
                std::wstring_view cmd = content.substr(0, firstSpace);
                std::wstring_view cmdArgs = (firstSpace != std::wstring_view::npos)
                    ? content.substr(firstSpace + 1) : L"";

                // The two tags that take an argument, then every tag that stands on its own.
                if (cmd == L"icon" && argIdx < argCount)
                {
                    args[argIdx].handler(tb, cmdArgs, args[argIdx].ptr);
                    argIdx++;
                }
                else if (cmd == L"color" && cmdArgs == L"{}" && argIdx < argCount)
                {
                    args[argIdx].handler(tb, L"push_color", args[argIdx].ptr);
                    argIdx++;
                }
                else if (const std::optional<FormatItem> item = formatItemOf(content))
                {
                    streamFormatItem(tb, *item);
                }
            }
            else
            {
                if (argIdx < argCount)
                {
                    args[argIdx].handler(tb, content, args[argIdx].ptr); argIdx++;
                }
            }
            pos = end + 1;
        }
    }

}
