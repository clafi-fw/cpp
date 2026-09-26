export module ThisApp.ElementPage;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // One element of a theme, under the new color theme architecture.
    export class ElementPage : public Panel
    {
    public:
        template<typename... Args>
        explicit ElementPage(const CreateParams&, UiElement, Args&&...);
    private:
        UiElement m_element;

        Label& m_title{ createTopBar<Label>(
            Text{ TextStyleId::SubTitle, uiElementOf(m_element).name },
            Padding{ 12.0f, 8.0f }
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ElementPage::ElementPage(const CreateParams& params, const UiElement element, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_element{ element }
    {
    }
}
