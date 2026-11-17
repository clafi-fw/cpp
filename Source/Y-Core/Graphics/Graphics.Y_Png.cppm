export module ClaFi.Core.Graphics.Png;

import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // A PNG file's pixels, straight alpha, or nothing for bytes that are not a PNG. **Decoded by
    // the platform**: the body lives in the platform layer, WIC on Windows and libpng on Linux,
    // the way Transfer::Clipboard's do. See Graphics-Types
    export [[nodiscard]] std::optional<Bitmap> decodePng(std::string_view bytes);

    // The bytes of a PNG holding `bitmap`'s pixels, straight alpha, or nothing where the platform
    // cannot write one. **Encoded by the platform**, through the PNG encoder alone as decodePng
    // goes through its decoder. See Graphics-Types
    export [[nodiscard]] std::optional<std::string> encodePng(const Bitmap&);
}
