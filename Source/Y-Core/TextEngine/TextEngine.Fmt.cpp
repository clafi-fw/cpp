module ClaFi.Core.TextEngine.Fmt;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Dom_UiSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // A colour written as text is ClaFi's own spelling, the one a [color #...] tag reads back.
    void dispatchColor(Text& tb, std::wstring_view spec, const void* p)
    {
        const Color& c = *static_cast<const Color*>(p);
        if (spec == L"push_color")
            tb << PushCustomColor{ c };
        else
            tb << Dom::ScalarSerializer<Color>::toWString(c);
    }
}
