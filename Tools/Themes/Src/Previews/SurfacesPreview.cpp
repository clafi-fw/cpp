module Themes_App.SurfacesPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Expander;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    SurfacesPreview::SurfacesPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        m_bitterlingSection.header().setExpanded(false);
    }

    SurfacesPreview::Section& SurfacesPreview::addSection(const std::wstring_view caption,
        const std::wstring_view paragraph)
    {
        Section& section = add<Section>(
            HostProps{
                VerticalAlign::Top,
                ExpanderViewMode::Section,
                HeaderText{ caption }
            },
            BodyProps{
                Orientation::Vertical,
                Padding{ 10.0f, 8.0f }
            }
        );
        section.body().add<Label>(paragraph);
        return section;
    }
}
