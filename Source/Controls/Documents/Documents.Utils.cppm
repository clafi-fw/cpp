export module ClaFi.Documents.Utils;

import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Documents
{
    // What a question is about, written into the sentence in the spot ink: one document by its
    // name, or how many there are where the question is about several. See Documents#questions
    export [[nodiscard]] Text documentInQuestionText(std::wstring_view subject);
    // The words with the first letter in upper case, for the start of a sentence or a title.
    export [[nodiscard]] std::wstring capitalized(std::wstring_view words);
    // The browser page a document file opens: its file name under the home page.
    export [[nodiscard]] std::wstring pagePathOfDocument(std::wstring_view fileName);
    // The document file a browser page stands for.
    export [[nodiscard]] std::wstring fileNameOfPage(std::wstring_view pagePath);
}
