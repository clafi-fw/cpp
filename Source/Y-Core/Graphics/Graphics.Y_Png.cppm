export module ClaFi.Core.Graphics.Png;

import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // A PNG file's pixels, straight alpha, or nothing for bytes that are not a PNG. DECODED BY
    // THE PLATFORM: the body lives in the platform layer, WIC on Windows and libpng on Linux,
    // the way Transfer::Clipboard's do. See Graphics-Types
    export [[nodiscard]] std::optional<Bitmap> decodePng(std::string_view bytes);
}
