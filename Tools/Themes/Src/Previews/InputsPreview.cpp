module Themes_App.InputsPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.Slider;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
        constexpr float k_temperaturePosition = 0.6f;
    }

    InputsPreview::InputsPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        m_releaseButton.onGetState([](GetStateEvent& event) {
            event.state.enabled = false;
        });
        m_temperatureSlider.setRelativePosition(k_temperaturePosition, false);
    }

    Text InputsPreview::captionOf(const std::wstring_view words)
    {
        return Text{ StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, words } } };
    }
}
