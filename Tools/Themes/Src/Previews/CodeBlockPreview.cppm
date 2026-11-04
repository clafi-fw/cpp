export module Themes_App.CodeBlockPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.CodeBlock;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A short block of C++, coloured as the language is.
    export class CodeBlockPreview : public PreviewPageBase
    {
    public:
        explicit CodeBlockPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{ L"Code block tint, frame and band" };
        static constexpr std::wstring_view k_code{
            L"struct Fish\n"
            L"{\n"
            L"    float length{};\n"
            L"    int fins{};\n"
            L"};\n"
            L"\n"
            L"Fish pike{ 64.0f, 7 };"
        };

        CodeBlock& m_block{ add<CodeBlock>() };
    };
}
