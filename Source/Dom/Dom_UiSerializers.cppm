export module ClaFi.Core.Dom_UiSerializers;

import ClaFi.Core.DomEngine;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    // A colour as ClaFi writes it for ClaFi to read back. See Dom#scalarserializer-color
    template <>
    struct ScalarSerializer<Color>
    {
        [[nodiscard]] static std::wstring toWString(const Color&);
        static void fromWString(std::wstring_view str, Color&);
    };
}
