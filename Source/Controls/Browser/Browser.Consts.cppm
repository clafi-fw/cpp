export module ClaFi.Browser.Consts;

import ClaFi.StdLib;

namespace ClaFi::Browser::ConfigNames
{
    export constexpr wchar_t PathSeparator = L'/';
    // we may want to make homePath a property of the BrowserControl,
    // then it may be like L"Home/" or L"NewPage/"
    export constexpr std::wstring_view HomePath = L"/";
    export constexpr std::wstring_view HomePage = { HomePath.data(), HomePath.length() - 1};

    export constexpr std::wstring_view Id{ L"Id" };
    export constexpr std::wstring_view Path{ L"Path" };
    export constexpr std::wstring_view Title{ L"Title" };
    export constexpr std::wstring_view Position{ L"Position" };
    export constexpr std::wstring_view Url{ L"Url" };
    export constexpr std::wstring_view BackVisits{ L"BackVisits" };
    export constexpr std::wstring_view ForwardVisits{ L"ForwardVisits" };

    export constexpr std::wstring_view UntitledPage{ L"Untitled" };

    export constexpr std::wstring_view SelectedTab{ L"SelectedTab" };
    export constexpr std::wstring_view OpenTabs{ L"OpenTabs" };
}
