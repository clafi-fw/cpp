module;
#include <cerrno>
#ifdef _WIN32
#include <cstdlib>
#endif
module ClaFi.Core.DomEngine_Document;

import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Dom
{
    namespace
    {
        void clearFileError()
        {
            errno = 0;
#ifdef _WIN32
            ::_set_doserrno(0);
#endif
        }

        [[noreturn]] void throwFileError(const char* operation)
        {
            const int saved = errno;
#ifdef _WIN32
            unsigned long native = 0;
            ::_get_doserrno(&native);
            if (native != 0)
            {
                throw std::ios_base::failure(operation,
                    std::error_code{ static_cast<int>(native), std::system_category() });
            }
#endif
            const std::error_code code = saved == 0 ? std::make_error_code(std::errc::io_error)
                : std::error_code{ saved, std::generic_category() };
            throw std::ios_base::failure(operation, code);
        }
    }

    bool FileFormatBase::saveSectionToFile(const Section& section,
        const std::filesystem::path& path) const
    {
        try
        {
            saveSectionToFileOrThrow(section, path);
            return true;
        }
        catch (const std::ios_base::failure&)
        {
            return false;
        }
    }

    void FileFormatBase::saveSectionToFileOrThrow(const Section& section,
        const std::filesystem::path& path) const
    {
        const std::filesystem::path directory = path.parent_path();
        if (!directory.empty() && !std::filesystem::exists(directory))
            std::filesystem::create_directories(directory);

        std::wostringstream text;
        text.exceptions(std::ios::badbit | std::ios::failbit);
        writeHeader(text);
        saveSectionToStream(section, text);
        const std::string utf8 = toUtf8(text.view());

        // Text mode preserves the platform's line endings after encoding the document as UTF-8.
        clearFileError();
        std::ofstream file{ path };
        if (!file)
            throwFileError("Could not open the file for writing");
        clearFileError();
        file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
        if (!file)
            throwFileError("Could not write the file");
        clearFileError();
        file.close();
        if (!file)
            throwFileError("Could not finish writing the file");
    }
}
