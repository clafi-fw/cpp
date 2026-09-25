module;
// libpng, from the global module fragment for the reason Linux.TextHeaders.h gives.
#include <png.h>

// AN IMPLEMENTATION UNIT OF THE MODULE THAT DECLARED decodePng, not of the Linux layer - see
// Wayland.Transfer.cpp, which stands in the same relation to Clipboard.
module ClaFi.Core.Graphics.Png;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    namespace
    {
        // The same caps the DIB decoder holds to: a picture past them is refused, not attempted.
        constexpr png_uint_32 k_maxSide = 65535;
        constexpr std::size_t k_maxPixels = std::size_t{ 1 } << 28;

        // The bytes a read callback draws from, and how far it has drawn.
        struct ByteSource
        {
            std::string_view bytes;
            std::size_t at{ 0 };
        };

        void readFrom(png_structp png, png_bytep target, png_size_t length)
        {
            ByteSource* source = static_cast<ByteSource*>(::png_get_io_ptr(png));
            if (!source || length > source->bytes.size() - source->at)
            {
                ::png_error(png, "read past the end of the PNG");
                return;
            }

            std::memcpy(target, source->bytes.data() + source->at, length);
            source->at += length;
        }

        // Quiet: a bad PNG is answered with nothing, and libpng's own words go nowhere - the
        // stock error handler would print them before jumping.
        [[noreturn]] void abandon(png_structp png, png_const_charp)
        {
            ::png_longjmp(png, 1);
        }

        void ignoreWarning(png_structp, png_const_charp)
        {
        }
    }

    // LIBPNG REPORTS AN ERROR BY LONGJMP, so everything the landing has to free is made before
    // setjmp and nothing that owns anything is created in the guarded region after it. The
    // result and the row table are declared ahead for that reason: a jump back lands with
    // them alive, and the return path destroys them as it would any other local.
    std::optional<Bitmap> decodePng(const std::string_view bytes)
    {
        constexpr std::size_t k_signatureSize = 8;
        if (bytes.size() < k_signatureSize)
            return std::nullopt;
        if (::png_sig_cmp(reinterpret_cast<png_const_bytep>(bytes.data()), 0, k_signatureSize))
            return std::nullopt;

        png_structp png = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, &abandon,
            &ignoreWarning);
        if (!png)
            return std::nullopt;
        png_infop info = ::png_create_info_struct(png);
        if (!info)
        {
            ::png_destroy_read_struct(&png, nullptr, nullptr);
            return std::nullopt;
        }

        ByteSource source{ bytes };
        std::optional<Bitmap> result{};
        std::vector<png_bytep> rows{};
        if (setjmp(png_jmpbuf(png)))
        {
            ::png_destroy_read_struct(&png, &info, nullptr);
            return std::nullopt;
        }

        ::png_set_read_fn(png, &source, &readFrom);
        ::png_read_info(png, info);

        const png_uint_32 width = ::png_get_image_width(png, info);
        const png_uint_32 height = ::png_get_image_height(png, info);
        const int bitDepth = ::png_get_bit_depth(png, info);
        const int colorType = ::png_get_color_type(png, info);
        if (width == 0 || height == 0 || width > k_maxSide || height > k_maxSide
            || static_cast<std::size_t>(width) * height > k_maxPixels)
        {
            ::png_destroy_read_struct(&png, &info, nullptr);
            return std::nullopt;
        }

        // Everything to eight bits per channel, BGRA, straight alpha: a palette expanded, a
        // low-depth grey widened, a transparency chunk turned into alpha, sixteen bits cut to
        // eight, grey spread to three channels, an alpha of 255 filled in where the file has
        // none, and the channels swapped into the order Bitmap holds.
        if (colorType == PNG_COLOR_TYPE_PALETTE)
            ::png_set_palette_to_rgb(png);
        if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
            ::png_set_expand_gray_1_2_4_to_8(png);
        if (::png_get_valid(png, info, PNG_INFO_tRNS))
            ::png_set_tRNS_to_alpha(png);
        if (bitDepth == 16)
            ::png_set_strip_16(png);
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            ::png_set_gray_to_rgb(png);
        ::png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        ::png_set_bgr(png);
        ::png_set_interlace_handling(png);
        ::png_read_update_info(png, info);
        if (::png_get_rowbytes(png, info) != static_cast<std::size_t>(width) * sizeof(Color))
        {
            ::png_destroy_read_struct(&png, &info, nullptr);
            return std::nullopt;
        }

        result.emplace(IntSize{ static_cast<int>(width), static_cast<int>(height) });
        rows.resize(height);
        for (png_uint_32 y = 0; y < height; ++y)
        {
            Color* row = result->data() + static_cast<std::ptrdiff_t>(y) * result->stride();
            rows[y] = reinterpret_cast<png_bytep>(row);
        }

        // The whole picture at once, which is what lets libpng lay an interlaced file down in
        // its passes over the same rows.
        ::png_read_image(png, rows.data());
        ::png_read_end(png, nullptr);
        ::png_destroy_read_struct(&png, &info, nullptr);
        return result;
    }
}
