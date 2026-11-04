module Themes_App.CodeBlockPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.CodeBlock;
import ClaFi.Controls.CodeBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace Themes_App
{
    CodeBlockPreview::CodeBlockPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        CodeBox& box = m_block.body();
        box.setLanguage(Syntax::Languages::Cpp);
        box.text() << k_code;
    }
}
