// The Windows entry point, and the code-page readings the framework has no word for. The system
// ANSI code page, the OEM code page, and the numbered code pages a byte format may carry text in
// are all Windows notions; the cross-platform Encodings module knows only UTF-16, UTF-8 and
// Latin-1, and this is where the rest are added.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

import ClaFi.Tools.WhatsClip.Encodings;
import ClaFi.Tools.WhatsClip.Languages;
import ClaFi.Tools.WhatsClip.Main;
import ClaFi.Tools.WhatsClip.Page;

import ClaFi.App.Application;
import ClaFi.Platform.Windows;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Cpu.Canvas;

namespace
{
    using namespace ClaFi;
    using namespace ClaFi::Tools::WhatsClip;

    // THE CODE PAGE THE ANSI AND OEM READINGS USE. Resolved from the clipboard's CF_LOCALE each
    // time a text page opens a format - see readLocale - so a code-page reading follows the locale
    // the text was copied in. The system defaults stand for a clipboard that names no locale.
    UINT s_ansiCodePage{ CP_ACP };
    UINT s_oemCodePage{ CP_OEMCP };

    // Bytes read in one Windows code page. What the code page cannot spell becomes its own default
    // character; a code page not installed reads as nothing, which is itself an answer.
    [[nodiscard]] std::wstring fromCodePage(const UINT codePage, const std::string_view bytes)
    {
        if (bytes.empty())
            return {};

        const int size = static_cast<int>(bytes.size());
        const int length = ::MultiByteToWideChar(codePage, 0, bytes.data(), size, nullptr, 0);
        std::wstring result(static_cast<std::size_t>(length), L'\0');
        ::MultiByteToWideChar(codePage, 0, bytes.data(), size, result.data(), length);
        return result;
    }

    // One fixed code page's reading, a function of its own so the entry that names it carries no
    // state - it is picked by hand for a format known to hold text in that code page.
    template<UINT CodePage>
    [[nodiscard]] std::wstring fromFixedCodePage(const std::string_view bytes)
    {
        return fromCodePage(CodePage, bytes);
    }

    // The clipboard's ANSI and OEM readings, in the code page CF_LOCALE named.
    [[nodiscard]] std::wstring fromAnsi(const std::string_view bytes)
    {
        return fromCodePage(s_ansiCodePage, bytes);
    }

    [[nodiscard]] std::wstring fromOem(const std::string_view bytes)
    {
        return fromCodePage(s_oemCodePage, bytes);
    }

    // CF_TEXT is ANSI text and CF_OEMTEXT is OEM text by their platform definition, so their
    // reading is known from the name where another format's is a guess from the bytes.
    [[nodiscard]] bool ansiClaims(const std::wstring_view formatName, std::string_view)
    {
        return formatName == L"CF_TEXT";
    }

    [[nodiscard]] bool oemClaims(const std::wstring_view formatName, std::string_view)
    {
        return formatName == L"CF_OEMTEXT";
    }

    // The code page a locale names for one kind of text, or the fallback where it names none - a
    // Unicode-only locale answers zero.
    [[nodiscard]] UINT codePageOf(const LCID locale, const LCTYPE kind, const UINT fallback)
    {
        wchar_t digits[8]{};
        const int written = ::GetLocaleInfoW(locale, kind, digits,
            static_cast<int>(std::size(digits)));
        if (written == 0)
            return fallback;

        const UINT codePage = static_cast<UINT>(std::wcstoul(digits, nullptr, 10));
        return codePage == 0 ? fallback : codePage;
    }

    // THE LOCALE THE CLIPBOARD SAYS ITS CODE-PAGE TEXT IS IN. CF_LOCALE carries an LCID beside the
    // text, and the ANSI and OEM readings take their code page from it - so text copied under one
    // locale reads right on a machine set to another. A clipboard with no CF_LOCALE leaves the
    // system defaults standing.
    void readLocale(Transfer::Offer& offer)
    {
        s_ansiCodePage = CP_ACP;
        s_oemCodePage = CP_OEMCP;

        const Transfer::FormatList formats = offer.advertisedFormats();
        for (const Transfer::Format& format : formats)
        {
            if (format.kind() != Transfer::Format::Kind::Custom || format.name() != L"CF_LOCALE")
                continue;

            const std::string bytes = offer.readBytes(format);
            if (bytes.size() < sizeof(LCID))
                return;

            LCID locale{ 0 };
            std::memcpy(&locale, bytes.data(), sizeof(locale));
            s_ansiCodePage = codePageOf(locale, LOCALE_IDEFAULTANSICODEPAGE, CP_ACP);
            s_oemCodePage = codePageOf(locale, LOCALE_IDEFAULTCODEPAGE, CP_OEMCP);
            return;
        }
    }

    // THE ENCODINGS A WINDOWS CLIPBOARD IS READ IN. UTF-16 and UTF-8 claim by content; ANSI and
    // OEM claim CF_TEXT and CF_OEMTEXT, whose code page CF_LOCALE names, and stand before UTF-8 so
    // those formats read in their defined code page rather than as a UTF-8 guess. The numbered code
    // pages claim nothing - they are picked by hand for a format carrying text in one. Latin-1
    // reads any bytes and stands last, the answer for what nothing else claimed.
    [[nodiscard]] EncodingList windowsEncodings()
    {
        return {
            { L"UTF-16 LE", &Decode::utf16Le, &Claims::utf16Le },
            { L"ANSI", &fromAnsi, &ansiClaims },
            { L"OEM", &fromOem, &oemClaims },
            { L"UTF-8", &Decode::utf8, &Claims::utf8 },
            { L"1252 Western", &fromFixedCodePage<1252>, nullptr },
            { L"1250 Central", &fromFixedCodePage<1250>, nullptr },
            { L"1251 Cyrillic", &fromFixedCodePage<1251>, nullptr },
            { L"1253 Greek", &fromFixedCodePage<1253>, nullptr },
            { L"1254 Turkish", &fromFixedCodePage<1254>, nullptr },
            { L"1255 Hebrew", &fromFixedCodePage<1255>, nullptr },
            { L"1256 Arabic", &fromFixedCodePage<1256>, nullptr },
            { L"Shift-JIS", &fromFixedCodePage<932>, nullptr },
            { L"GBK", &fromFixedCodePage<936>, nullptr },
            { L"EUC-KR", &fromFixedCodePage<949>, nullptr },
            { L"Big5", &fromFixedCodePage<950>, nullptr },
            { L"OEM 866", &fromFixedCodePage<866>, nullptr },
            { L"OEM 850", &fromFixedCodePage<850>, nullptr },
            { L"OEM 437", &fromFixedCodePage<437>, nullptr },
            { L"Latin-1", &Decode::latin1, &Claims::anyBytes },
        };
    }
}

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point the operating system calls,
// the instance handle it hands over, and the two types the application is built out of. The viewer
// itself is in ClaFi.Tools.WhatsClip.Main, beside the Wayland entry point that names it the
// same way.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t*, int)
{
    using namespace ClaFi;

    Win32Application app{
        Win32Platform::Params{ hInstance },
        Tools::WhatsClip::applicationParams(),
        Dom::Dt::Section{}
    };

    // What the text pages read bytes in and colour text in, plus the resolver that gives the
    // code-page readings the clipboard's own code page. An application built over this viewer adds
    // its own encoding and language here.
    Tools::WhatsClip::PickLists picks{
        windowsEncodings(),
        Tools::WhatsClip::defaultLanguages(),
        &readLocale
    };

    return Tools::WhatsClip::runWhatsClip(app, picks);
}
