module ClaFi.Core.Graphics.Dib;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    namespace
    {
        // The header sizes a block may open with. Anything from the info header up carries the
        // same first fields; the V2 and V3 info headers and the V4 and V5 headers add the masks.
        constexpr std::size_t k_coreHeaderSize = 12;
        constexpr std::size_t k_infoHeaderSize = 40;
        constexpr std::size_t k_infoHeaderWithMasks = 52;
        constexpr std::size_t k_infoHeaderWithAlphaMask = 56;
        constexpr std::size_t k_fileHeaderSize = 14;

        namespace Compression
        {
            constexpr std::uint32_t rgb = 0;
            constexpr std::uint32_t bitFields = 3;
            constexpr std::uint32_t alphaBitFields = 6;
        }

        // Past these a header is read as damaged rather than allocated for.
        constexpr int k_maxSide = 65535;
        constexpr std::size_t k_maxPixels = std::size_t{ 1 } << 28;

        // One channel's place in a packed pixel.
        struct ChannelMask
        {
            std::uint32_t mask{ 0 };
            int shift{ 0 };
            int bits{ 0 };
        };

        struct Header
        {
            int width{ 0 };
            int height{ 0 };   // as stated: negative for rows written top down
            int bitCount{ 0 };
            std::uint32_t compression{ Compression::rgb };
            std::uint32_t colorsUsed{ 0 };
            std::size_t size{ 0 };
            // Whether the masks stand after the header rather than inside it, and how many.
            std::size_t trailingMaskBytes{ 0 };
            std::size_t paletteEntrySize{ 4 };
            ChannelMask red{};
            ChannelMask green{};
            ChannelMask blue{};
            ChannelMask alpha{};
        };

        [[nodiscard]] std::uint32_t readU32(const std::string_view bytes, const std::size_t at)
        {
            const auto byte = [bytes, at](const std::size_t index) {
                return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at + index]));
            };
            return byte(0) | (byte(1) << 8) | (byte(2) << 16) | (byte(3) << 24);
        }

        [[nodiscard]] std::uint16_t readU16(const std::string_view bytes, const std::size_t at)
        {
            const std::uint32_t low = static_cast<unsigned char>(bytes[at]);
            const std::uint32_t high = static_cast<unsigned char>(bytes[at + 1]);
            return static_cast<std::uint16_t>(low | (high << 8));
        }

        [[nodiscard]] std::int32_t readI32(const std::string_view bytes, const std::size_t at)
        {
            return static_cast<std::int32_t>(readU32(bytes, at));
        }

        [[nodiscard]] ChannelMask maskOf(const std::uint32_t mask)
        {
            if (!mask)
                return {};

            return {
                mask,
                std::countr_zero(mask),
                std::popcount(mask),
            };
        }

        // The channel scaled to a byte. A mask naming no bits answers nothing.
        [[nodiscard]] ColorByte channelOf(const std::uint32_t pixel, const ChannelMask& channel)
        {
            if (!channel.bits)
                return 0;

            const std::uint32_t value = (pixel & channel.mask) >> channel.shift;
            if (channel.bits >= 8)
                return static_cast<ColorByte>(value >> (channel.bits - 8));

            const std::uint32_t full = (std::uint32_t{ 1 } << channel.bits) - 1;
            return static_cast<ColorByte>((value * 255 + full / 2) / full);
        }

        [[nodiscard]] Color colorOf(const std::uint32_t pixel, const Header& header)
        {
            const ColorByte alpha = header.alpha.bits ? channelOf(pixel, header.alpha) : 255;
            return {
                channelOf(pixel, header.red),
                channelOf(pixel, header.green),
                channelOf(pixel, header.blue),
                alpha,
            };
        }

        [[nodiscard]] bool readsAsPacked(const int bitCount)
        {
            return bitCount == 16 || bitCount == 32;
        }

        [[nodiscard]] bool validBitCount(const int bitCount)
        {
            switch (bitCount)
            {
                case 1:
                case 4:
                case 8:
                case 16:
                case 24:
                case 32:
                    return true;
                default:
                    return false;
            }
        }

        // The masks a packed pixel is read with where the block states none.
        void applyDefaultMasks(Header& header)
        {
            if (header.bitCount == 16)
            {
                header.red = maskOf(0x7c00);
                header.green = maskOf(0x03e0);
                header.blue = maskOf(0x001f);
            }
            else
            {
                header.red = maskOf(0x00ff0000);
                header.green = maskOf(0x0000ff00);
                header.blue = maskOf(0x000000ff);
            }
            header.alpha = {};
        }

        [[nodiscard]] std::optional<Header> readHeader(const std::string_view bytes)
        {
            if (bytes.size() < k_coreHeaderSize)
                return std::nullopt;

            Header header{};
            header.size = readU32(bytes, 0);
            if (header.size > bytes.size())
                return std::nullopt;

            if (header.size == k_coreHeaderSize)
            {
                header.width = readU16(bytes, 4);
                header.height = readU16(bytes, 6);
                header.bitCount = readU16(bytes, 10);
                header.paletteEntrySize = 3;
            }
            else if (header.size >= k_infoHeaderSize)
            {
                header.width = readI32(bytes, 4);
                header.height = readI32(bytes, 8);
                header.bitCount = readU16(bytes, 14);
                header.compression = readU32(bytes, 16);
                header.colorsUsed = readU32(bytes, 32);
            }
            else
            {
                return std::nullopt;
            }

            if (header.width <= 0 || header.height == 0)
                return std::nullopt;
            if (header.width > k_maxSide || std::abs(header.height) > k_maxSide)
                return std::nullopt;
            const std::size_t pixels = static_cast<std::size_t>(header.width)
                * static_cast<std::size_t>(std::abs(header.height));
            if (pixels > k_maxPixels)
                return std::nullopt;
            if (!validBitCount(header.bitCount))
                return std::nullopt;

            const bool masked = header.compression == Compression::bitFields
                || header.compression == Compression::alphaBitFields;
            if (header.compression != Compression::rgb && !masked)
                return std::nullopt;
            if (masked && !readsAsPacked(header.bitCount))
                return std::nullopt;

            if (!masked)
            {
                if (readsAsPacked(header.bitCount))
                    applyDefaultMasks(header);
                return header;
            }

            // WHERE THE MASKS STAND. A header of 40 bytes is followed by them, three or four; a
            // longer one carries them in its own fields.
            std::size_t masksAt = header.size;
            if (header.size >= k_infoHeaderWithMasks)
            {
                masksAt = k_infoHeaderSize;
            }
            else
            {
                const bool withAlpha = header.compression == Compression::alphaBitFields;
                header.trailingMaskBytes = withAlpha ? 16 : 12;
                if (bytes.size() < header.size + header.trailingMaskBytes)
                    return std::nullopt;
            }

            header.red = maskOf(readU32(bytes, masksAt));
            header.green = maskOf(readU32(bytes, masksAt + 4));
            header.blue = maskOf(readU32(bytes, masksAt + 8));
            const bool alphaStated = header.size >= k_infoHeaderWithAlphaMask
                || header.trailingMaskBytes == 16;
            if (alphaStated)
                header.alpha = maskOf(readU32(bytes, masksAt + 12));

            if (!header.red.bits || !header.green.bits || !header.blue.bits)
                return std::nullopt;

            return header;
        }

        [[nodiscard]] std::size_t paletteEntries(const Header& header)
        {
            if (header.bitCount > 8)
                return 0;

            const std::size_t all = std::size_t{ 1 } << header.bitCount;
            if (header.size == k_coreHeaderSize || !header.colorsUsed)
                return all;

            return std::min<std::size_t>(header.colorsUsed, all);
        }

        [[nodiscard]] std::size_t rowStride(const Header& header)
        {
            const std::size_t bits = static_cast<std::size_t>(header.width) * header.bitCount;
            return ((bits + 31) / 32) * 4;
        }

        using Palette = std::vector<Color>;

        [[nodiscard]] std::optional<Palette> readPalette(const std::string_view bytes,
            const Header& header, const std::size_t at)
        {
            const std::size_t entries = paletteEntries(header);
            if (at + entries * header.paletteEntrySize > bytes.size())
                return std::nullopt;

            Palette palette{};
            palette.reserve(entries);
            for (std::size_t i = 0; i < entries; ++i)
            {
                const std::size_t entry = at + i * header.paletteEntrySize;
                palette.emplace_back(
                    static_cast<ColorByte>(bytes[entry + 2]),
                    static_cast<ColorByte>(bytes[entry + 1]),
                    static_cast<ColorByte>(bytes[entry]),
                    static_cast<ColorByte>(255)
                );
            }

            return palette;
        }

        [[nodiscard]] Color paletteColor(const Palette& palette, const std::size_t index)
        {
            if (index < palette.size())
                return palette[index];

            return { 0, 0, 0, 255 };
        }

        void decodeRow(const std::string_view row, const Header& header, const Palette& palette,
            Color* target)
        {
            const std::size_t width = static_cast<std::size_t>(header.width);
            switch (header.bitCount)
            {
                case 1:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        const unsigned char byte = static_cast<unsigned char>(row[x >> 3]);
                        target[x] = paletteColor(palette, (byte >> (7 - (x & 7))) & 1);
                    }
                    break;
                case 4:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        const unsigned char byte = static_cast<unsigned char>(row[x >> 1]);
                        target[x] = paletteColor(palette, (x & 1) ? (byte & 0x0f) : (byte >> 4));
                    }
                    break;
                case 8:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        target[x] = paletteColor(palette, static_cast<unsigned char>(row[x]));
                    }
                    break;
                case 16:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        target[x] = colorOf(readU16(row, x * 2), header);
                    }
                    break;
                case 24:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        const std::size_t at = x * 3;
                        target[x] = {
                            static_cast<ColorByte>(row[at + 2]),
                            static_cast<ColorByte>(row[at + 1]),
                            static_cast<ColorByte>(row[at]),
                            static_cast<ColorByte>(255),
                        };
                    }
                    break;
                case 32:
                    for (std::size_t x = 0; x < width; ++x)
                    {
                        target[x] = colorOf(readU32(row, x * 4), header);
                    }
                    break;
            }
        }

        // The rows into a bitmap. `rowsAt` is where the first stored row starts; a stored row
        // is the bottom one unless the height says top down.
        [[nodiscard]] std::optional<Bitmap> decodeRows(const std::string_view bytes,
            const Header& header, const Palette& palette, const std::size_t rowsAt)
        {
            const std::size_t rows = static_cast<std::size_t>(std::abs(header.height));
            const std::size_t stride = rowStride(header);
            const std::size_t lastRowBytes =
                (static_cast<std::size_t>(header.width) * header.bitCount + 7) / 8;
            if (rowsAt > bytes.size())
                return std::nullopt;
            if (rowsAt + stride * (rows - 1) + lastRowBytes > bytes.size())
                return std::nullopt;

            const bool topDown = header.height < 0;
            Bitmap result{ IntSize{ header.width, static_cast<int>(rows) } };
            for (std::size_t y = 0; y < rows; ++y)
            {
                const std::size_t stored = topDown ? y : rows - 1 - y;
                const std::string_view row = bytes.substr(rowsAt + stored * stride, stride);
                decodeRow(row, header, palette, result.pixel(0, static_cast<int>(y)));
            }

            return result;
        }

        // The header, its masks and its colour table, in that order: where the rows start when
        // nothing states otherwise.
        [[nodiscard]] std::optional<Bitmap> decodeBlock(const std::string_view bytes,
            const std::size_t rowsAtStated)
        {
            const std::optional<Header> header = readHeader(bytes);
            if (!header)
                return std::nullopt;

            const std::size_t paletteAt = header->size + header->trailingMaskBytes;
            const std::optional<Palette> palette = readPalette(bytes, *header, paletteAt);
            if (!palette)
                return std::nullopt;

            const std::size_t rowsAt = paletteAt + palette->size() * header->paletteEntrySize;
            return decodeRows(bytes, *header, *palette, std::max(rowsAt, rowsAtStated));
        }
    }

    std::optional<Bitmap> decodeDib(const std::string_view bytes)
    {
        return decodeBlock(bytes, 0);
    }

    std::optional<Bitmap> decodeBmpFile(const std::string_view bytes)
    {
        if (bytes.size() < k_fileHeaderSize || bytes[0] != 'B' || bytes[1] != 'M')
            return std::nullopt;

        // The file header says where the rows start, counted from the file's first byte. It is
        // taken only where it lies past what the block itself accounts for: a gap is data the
        // writer chose to leave, a stated offset inside the header is a damaged one.
        const std::size_t rowsAt = readU32(bytes, 10);
        const std::size_t rowsAtInBlock = rowsAt > k_fileHeaderSize ? rowsAt - k_fileHeaderSize : 0;
        return decodeBlock(bytes.substr(k_fileHeaderSize), rowsAtInBlock);
    }
}
