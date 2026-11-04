export module Themes_App.WaterChoice;

import ClaFi.Controls.RadioButton;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Fresh or salt water: two radio buttons, one under the other, keeping the choice between them.
    export class WaterChoice : public StackPanel
    {
    public:
        explicit WaterChoice(const CreateParams&);
    private:
        // The water a radio button stands for, as its tag names it.
        enum class Water : TagValue
        {
            Fresh,
            Salt
        };
    private:
        Water m_water{ Water::Fresh };
        RadioButton& m_freshWater{ add<RadioButton>(L"Fresh water", Tag{ Water::Fresh }) };
        RadioButton& m_saltWater{ add<RadioButton>(L"Salt water", Tag{ Water::Salt }) };
    };
}
