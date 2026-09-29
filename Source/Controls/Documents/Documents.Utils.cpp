module ClaFi.Documents.Utils;

import ClaFi.Browser.Consts;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    Text documentInQuestionText(const std::wstring_view subject)
    {
        Text result{};
        result << PushThemeColor{ InkWell::spotInk() } << subject << PopColor{};
        return result;
    }

    std::wstring capitalized(const std::wstring_view words)
    {
        std::wstring result{ words };
        if (!result.empty())
            result[0ull] = static_cast<wchar_t>(std::towupper(result[0ull]));
        return result;
    }

    std::wstring pagePathOfDocument(const std::wstring_view fileName)
    {
        if (fileName.empty())
            return {};
        return std::wstring{ Browser::ConfigNames::homePath }.append(fileName);
    }

    std::wstring fileNameOfPage(const std::wstring_view pagePath)
    {
        return std::filesystem::path{ pagePath }.filename().wstring();
    }
}
