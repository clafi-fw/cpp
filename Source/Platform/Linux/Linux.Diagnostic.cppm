module;
#include <cerrno>
#include <cstdio>
#include <source_location>
#include <system_error>
export module ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Diagnostic.Options;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    using Diagnostic::Options::ApiErrors;
    constexpr bool k_apiErrorsReported{ Diagnostic::Options::apiErrors != ApiErrors::Ignore };

    // A failed call, done with as Diagnostic::Options::apiErrors says: said on stderr or thrown
    // as a std::system_error, either way naming the site the check stood at.
    export void reportApiError(std::error_code, const std::source_location&);

    // The failure errno describes, at the caller's site. Read before anything else can move
    // errno, which is why nothing stands between a call and its check.
    export void reportErrno(const std::source_location& loc = std::source_location::current());

    // Whether a call answering through errno succeeded, as the caller judged its result. The
    // answer comes back so the caller's own way out stays one statement.
    export bool check(const bool succeeded,
        const std::source_location& loc = std::source_location::current())
    {
        if constexpr (k_apiErrorsReported)
        {
            if (!succeeded)
                reportErrno(loc);
        }
        return succeeded;
    }
}

namespace ClaFi::PlatformImplementation::Linux
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
            std::fputs(message.c_str(), stderr);
        }
    }

    void reportErrno(const std::source_location& loc)
    {
        reportApiError(std::error_code(errno, std::generic_category()), loc);
    }
}
