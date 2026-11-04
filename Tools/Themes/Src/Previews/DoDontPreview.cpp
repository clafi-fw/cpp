module Themes_App.DoDontPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.DoDont;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace Themes_App
{
    DoDontPreview::DoDontPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        m_block.addItem(DoDontSide::Do, Text{ L"Feed at dawn" });
        m_block.addItem(DoDontSide::Do, Text{ L"Rinse the net" });
        m_block.addItem(DoDontSide::Dont, Text{ L"Tap the glass" });
        m_block.addItem(DoDontSide::Dont, Text{ L"Feed twice" });
    }
}
