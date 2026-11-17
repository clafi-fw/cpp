module;
#include "Windows.Headers.h"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// **An implementation unit of the module that declared** decodePng, not of the Windows layer - see
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

    // **Through the PNG decoder alone**, not whichever codec claims the bytes: what was handed over
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

    // **Through the PNG encoder alone**, as decodePng goes through its decoder: the pixels are
    // handed over as straight BGRA, which is what Bitmap holds, and the file is written over
    // memory rather than to a path.
    std::optional<std::string> encodePng(const Bitmap& bitmap)
    {
        const int width = bitmap.width();
        const int height = bitmap.height();
        if (width <= 0 || height <= 0)
            return std::nullopt;
        if (width > static_cast<int>(k_maxSide) || height > static_cast<int>(k_maxSide))
            return std::nullopt;
        if (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) > k_maxPixels)
            return std::nullopt;

        ComPtr<IWICImagingFactory> factory;
        if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))))
        {
            return std::nullopt;
        }

        ComPtr<IStream> stream;
        if (FAILED(::CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
            return std::nullopt;

        ComPtr<IWICBitmapEncoder> encoder;
        if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)))
            return std::nullopt;
        if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)))
            return std::nullopt;

        ComPtr<IWICBitmapFrameEncode> frame;
        if (FAILED(encoder->CreateNewFrame(&frame, nullptr)))
            return std::nullopt;
        if (FAILED(frame->Initialize(nullptr)))
            return std::nullopt;
        if (FAILED(frame->SetSize(static_cast<UINT>(width), static_cast<UINT>(height))))
            return std::nullopt;

        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(frame->SetPixelFormat(&format)))
            return std::nullopt;

        const UINT stride = static_cast<UINT>(width * static_cast<int>(sizeof(Color)));
        const UINT size = stride * static_cast<UINT>(height);
        Color* pixels = const_cast<Color*>(bitmap.data());
        if (FAILED(frame->WritePixels(static_cast<UINT>(height), stride, size,
            reinterpret_cast<BYTE*>(pixels))))
        {
            return std::nullopt;
        }
        if (FAILED(frame->Commit()) || FAILED(encoder->Commit()))
            return std::nullopt;

        STATSTG stats{};
        if (FAILED(stream->Stat(&stats, STATFLAG_NONAME)))
            return std::nullopt;
        std::string bytes(static_cast<std::size_t>(stats.cbSize.QuadPart), '\0');
        LARGE_INTEGER start{};
        if (FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr)))
            return std::nullopt;
        ULONG read = 0;
        if (FAILED(stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read)))
            return std::nullopt;
        if (read != bytes.size())
            return std::nullopt;

        return bytes;
    }
}
