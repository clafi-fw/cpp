export module ClaFi.Browser.Consts;

import ClaFi.StdLib;

namespace ClaFi::Browser::ConfigNames
{
    export constexpr wchar_t pathSeparator = L'/';
    // we may want to make homePath a property of the BrowserControl,
    // then it may be like L"Home/" or L"NewPage/"
    export constexpr std::wstring_view homePath = L"/";
    export constexpr std::wstring_view homePage = { homePath.data(), homePath.length() - 1};

    export constexpr std::wstring_view id{ L"Id" };
    export constexpr std::wstring_view path{ L"Path" };
    export constexpr std::wstring_view title{ L"Title" };

    export constexpr std::wstring_view untitledPage{ L"Untitled" };

    export constexpr std::wstring_view selectedTab{ L"SelectedTab" };
    export constexpr std::wstring_view openTabs{ L"OpenTabs" };
}
