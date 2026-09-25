module;
#include "Windows.Headers.h"
#include <system_error>
#include <source_location>
export module ClaFi.Platform.Windows.Diagnostic;

import ClaFi.Diagnostic.Options;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    using Diagnostic::Options::ApiErrors;
    constexpr bool k_apiErrorsReported{ Diagnostic::Options::apiErrors != ApiErrors::Ignore };

    // A failed call, done with as Diagnostic::Options::apiErrors says: said into the debugger's
    // output or thrown as a std::system_error, either way naming the site the check stood at.
    export void reportApiError(std::error_code, const std::source_location&);

    // A call answering through GetLastError: a zero or null result is the failure. The default
    // argument captures the caller's site, not this file's.
    export void check(const auto result,
        const std::source_location& loc = std::source_location::current())
    {
        if constexpr (k_apiErrorsReported)
        {
            if (!result)
            {
                const std::error_code code(static_cast<int>(::GetLastError()),
                    std::system_category());
                reportApiError(code, loc);
            }
        }
    }

    // A COM call.
    export void checkHr(const HRESULT hr,
        const std::source_location& loc = std::source_location::current())
    {
        if constexpr (k_apiErrorsReported)
        {
            if (FAILED(hr))
                reportApiError(std::error_code(hr, std::system_category()), loc);
        }
    }
}

namespace ClaFi::PlatformImplementation::Windows
{
    void reportApiError(const std::error_code code, const std::source_location& loc)
    {
        std::string message = std::format("ClaFi: API call failed in {} at {}:{} - {}",
            loc.function_name(), loc.file_name(), loc.line(), code.message());
        if constexpr (Diagnostic::Options::apiErrors == ApiErrors::Throw)
            throw std::system_error(code, message);
        else
        {
            message += '\n';
            ::OutputDebugStringA(message.c_str());
        }
    }
}
