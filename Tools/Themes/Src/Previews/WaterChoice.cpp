module Themes_App.WaterChoice;

import ClaFi.Controls.RadioButton;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    WaterChoice::WaterChoice(const CreateParams& params)
        :
        StackPanel{ params, Orientation::Vertical }
    {
        for (RadioButton* button : { &m_freshWater, &m_saltWater })
        {
            button->onClick([this](ClickEvent& event) {
                m_water = event.control->tag<Water>();
                m_freshWater.invalidateState();
                m_saltWater.invalidateState();
            });
            button->onGetState([this](GetStateEvent& event) {
                event.state.selected = event.control.tag<Water>() == m_water;
            });
        }
    }
}
