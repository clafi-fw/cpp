module;
#include "Windows.Headers.h"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// AN IMPLEMENTATION UNIT OF THE MODULE THAT DECLARED decodePng, not of the Windows layer - see
// Windows.Transfer.cpp, which stands in the same relation to Clipboard.
module ClaFi.Core.Graphics.Png;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        // The same caps the DIB decoder holds to: a picture past them is refused, not attempted.
        constexpr UINT k_maxSide = 65535;
        constexpr std::size_t k_maxPixels = std::size_t{ 1 } << 28;
    }

    // THROUGH THE PNG DECODER ALONE, not whichever codec claims the bytes: what was handed over
    // as a PNG is read as one or not at all. The frame is converted to straight BGRA, which is
    // what Bitmap holds, whatever depth, palette or interlace the file has.
    std::optional<Bitmap> decodePng(const std::string_view bytes)
    {
        if (bytes.empty() || bytes.size() > std::numeric_limits<DWORD>::max())
            return std::nullopt;

        ComPtr<IWICImagingFactory> factory;
        if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))))
        {
            return std::nullopt;
        }

        // Over the caller's bytes, not a copy of them: they outlive the decode, which is the
        // rest of this function.
        ComPtr<IWICStream> stream;
        if (FAILED(factory->CreateStream(&stream)))
            return std::nullopt;
        BYTE* start = reinterpret_cast<BYTE*>(const_cast<char*>(bytes.data()));
        if (FAILED(stream->InitializeFromMemory(start, static_cast<DWORD>(bytes.size()))))
            return std::nullopt;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoder(GUID_ContainerFormatPng, nullptr, &decoder)))
            return std::nullopt;
        if (FAILED(decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnDemand)))
            return std::nullopt;

        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, &frame)))
            return std::nullopt;

        UINT width = 0;
        UINT height = 0;
        if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0)
            return std::nullopt;
        if (width > k_maxSide || height > k_maxSide)
            return std::nullopt;
        if (static_cast<std::size_t>(width) * height > k_maxPixels)
            return std::nullopt;

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(&converter)))
            return std::nullopt;
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        {
            return std::nullopt;
        }

        Bitmap result{ IntSize{ static_cast<int>(width), static_cast<int>(height) } };
        const UINT stride = static_cast<UINT>(width * sizeof(Color));
        const UINT size = stride * height;
        if (FAILED(converter->CopyPixels(nullptr, stride, size,
            reinterpret_cast<BYTE*>(result.data()))))
        {
            return std::nullopt;
        }

        return result;
    }
}
