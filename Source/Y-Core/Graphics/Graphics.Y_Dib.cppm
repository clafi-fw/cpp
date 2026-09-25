export module ClaFi.Core.Graphics.Dib;

import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // A device-independent bitmap read into pixels - the block a Windows clipboard carries as
    // CF_DIB or CF_DIBV5: an info header, the masks or the colour table it needs, the rows.
    // Nothing where the header is damaged or the rows are compressed. See Graphics-Types
    export [[nodiscard]] std::optional<Bitmap> decodeDib(std::string_view bytes);

    // The same block behind the file header a .bmp starts with, which is what image/bmp carries.
    export [[nodiscard]] std::optional<Bitmap> decodeBmpFile(std::string_view bytes);
}
