module;
#include "Windows.Headers.h"
// Required for WIC
#include <wincodec.h>
#include <wrl/client.h>

export module ClaFi.Platform.Windows.IconFileWriter;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
#pragma pack(push, 1)
    struct ICONDIR {
        std::uint16_t idReserved = 0;
        std::uint16_t idType = 1;
        std::uint16_t idCount = 0;
    };

    struct ICONDIRENTRY_Partial {
        std::uint8_t  bWidth;
        std::uint8_t  bHeight;
        std::uint8_t  bColorCount = 0;
        std::uint8_t  bReserved = 0;
        std::uint16_t wPlanes = 1;
        std::uint16_t wBitCount = 32;
        std::uint32_t dwBytesInRes;
        std::uint32_t dwImageOffset;
    };
#pragma pack(pop)

    export class IconFileWriter {
    public:
        IconFileWriter(std::wstring_view filePath)
        {
            m_fileStream.open(std::filesystem::path{ filePath }, std::ios::binary | std::ios::trunc);

            if (!m_fileStream.is_open())
                throw std::runtime_error("Could not open file for writing.");

            // Reserve space for the header and entries (max 20)
            const std::size_t max_icons = 20;
            m_headerPaddingSize = sizeof(ICONDIR) + (max_icons * sizeof(ICONDIRENTRY_Partial));

            std::vector<char> header_placeholders(m_headerPaddingSize, 0);
            m_fileStream.write(header_placeholders.data(), m_headerPaddingSize);
        }

        ~IconFileWriter()
        {
            finalizeAndClose();
        }

        void addBitmap(std::uint32_t width, std::uint32_t height, const Color* pixelData, std::intptr_t /*stride*/)
        {
            if (!m_fileStream.is_open()) return;

            // Use PNG for 256x256 or larger (Standard Windows practice)
            if (width >= 256 || height >= 256) {
                addBitmapPng(width, height, pixelData);
            }
            else {
                addBitmapDib(width, height, pixelData);
            }
        }

        void finalizeAndClose()
        {
            if (!m_fileStream.is_open()) return;

            m_fileStream.seekp(0, std::ios::beg);
            ICONDIR mainDir;
            mainDir.idCount = static_cast<std::uint16_t>(m_entries.size());
            m_fileStream.write(reinterpret_cast<const char*>(&mainDir), sizeof(ICONDIR));

            for (const auto& entry : m_entries)
                m_fileStream.write(reinterpret_cast<const char*>(&entry), sizeof(ICONDIRENTRY_Partial));

            m_fileStream.close();
        }

    private:

#if 0   // Input data is top-down (Linux bitmaps)

        void addBitmapDib(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            std::uint32_t pixelDataSize = width * height * sizeof(Color);

            // 1-bit AND mask logic (required for ICO DIBs)
            std::uint32_t maskStride = ((width + 31u) / 32u) * 4u;
            std::uint32_t maskSize = maskStride * height;

            WinApi::BITMAPINFOHEADER header = {};
            header.biSize = sizeof(WinApi::BITMAPINFOHEADER);
            header.biWidth = width;
            header.biHeight = height * 2; // ICO DIB height must be doubled (XOR + AND mask)
            header.biPlanes = 1;
            header.biBitCount = 32;
            header.biCompression = 0; // BI_RGB

            ICONDIRENTRY_Partial entry;
            entry.bWidth = (width >= 256) ? 0 : static_cast<std::uint8_t>(width);
            entry.bHeight = (height >= 256) ? 0 : static_cast<std::uint8_t>(height);
            entry.dwBytesInRes = sizeof(header) + pixelDataSize + maskSize;
            entry.dwImageOffset = static_cast<std::uint32_t>(m_fileStream.tellp());
            m_entries.push_back(entry);

            // --- 1. Write Header ---
            m_fileStream.write(reinterpret_cast<const char*>(&header), sizeof(header));

            // --- 2. Write Pixel Data (Flipped to Bottom-Up) ---
            // We write row-by-row, starting from the last row of the input buffer
            for (std::int32_t y = static_cast<std::int32_t>(height) - 1; y >= 0; --y) {
                const Color* rowPtr = pixelData + (y * width);
                m_fileStream.write(reinterpret_cast<const char*>(rowPtr), width * sizeof(Color));
            }

            // --- 3. Write AND Mask (Zeros) ---
            // If the color data is bottom-up, the mask should be too.
            // Since we are writing zeros, the order doesn't technically matter, but we must write the size.
            writeZerosIntoBinaryStream(m_fileStream, maskSize);
        }

        // PNG Compressed path using WIC
        void addBitmapPng(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            auto pngData = encodeToPng(width, height, pixelData);
            if (pngData.empty()) return;

            ICONDIRENTRY_Partial entry;
            entry.bWidth = (width >= 256) ? 0 : static_cast<std::uint8_t>(width);
            entry.bHeight = (height >= 256) ? 0 : static_cast<std::uint8_t>(height);
            entry.dwBytesInRes = static_cast<std::uint32_t>(pngData.size());
            entry.dwImageOffset = static_cast<std::uint32_t>(m_fileStream.tellp());
            m_entries.push_back(entry);

            m_fileStream.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
        }

        std::vector<std::uint8_t> encodeToPng(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            using Microsoft::WRL::ComPtr;

            // 1. Initialize WIC
            ComPtr<IWICImagingFactory> factory;
            if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
                return {};

            // 2. Create stream to hold PNG data
            ComPtr<IStream> stream;
            if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
                return {};

            // 3. Create PNG Encoder
            ComPtr<IWICBitmapEncoder> encoder;
            if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)))
                return {};

            if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)))
                return {};

            // 4. Create Frame
            ComPtr<IWICBitmapFrameEncode> frame;
            if (FAILED(encoder->CreateNewFrame(&frame, nullptr)))
                return {};

            if (FAILED(frame->Initialize(nullptr)))
                return {};

            if (FAILED(frame->SetSize(width, height)))
                return {};

            // Assuming Color is BGRA
            WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
            if (FAILED(frame->SetPixelFormat(&format)))
                return {};

            // 5. Write Pixels
            std::uint32_t stride = width * sizeof(Color);
            if (FAILED(frame->WritePixels(height, stride, stride * height, reinterpret_cast<BYTE*>(const_cast<Color*>(pixelData)))))
                return {};

            if (FAILED(frame->Commit()))
                return {};

            if (FAILED(encoder->Commit()))
                return {};

            // 6. Copy memory out of IStream to std::vector
            STATSTG stats;
            stream->Stat(&stats, STATFLAG_NONAME);
            std::vector<std::uint8_t> buffer(static_cast<std::size_t>(stats.cbSize.QuadPart));

            LARGE_INTEGER zero = { 0 };
            stream->Seek(zero, STREAM_SEEK_SET, nullptr);
            ULONG read;
            stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &read);

            return buffer;
        }

#else // Input data is bottom-up (Windows bitmaps)

        // Traditional DIB (Uncompressed) path
        void addBitmapDib(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            std::uint32_t pixelDataSize = width * height * sizeof(Color);
            std::uint32_t maskStride = ((width + 31u) / 32u) * 4u;
            std::uint32_t maskSize = maskStride * height;

            BITMAPINFOHEADER header = {};
            header.biSize = sizeof(BITMAPINFOHEADER);
            header.biWidth = width;
            header.biHeight = height * 2; // ICO DIB height is doubled
            header.biPlanes = 1;
            header.biBitCount = 32;
            header.biCompression = 0; // BI_RGB

            ICONDIRENTRY_Partial entry;
            entry.bWidth = (width >= 256) ? 0 : static_cast<std::uint8_t>(width);
            entry.bHeight = (height >= 256) ? 0 : static_cast<std::uint8_t>(height);
            entry.dwBytesInRes = sizeof(header) + pixelDataSize + maskSize;
            entry.dwImageOffset = static_cast<std::uint32_t>(m_fileStream.tellp());
            m_entries.push_back(entry);

            m_fileStream.write(reinterpret_cast<const char*>(&header), sizeof(header));
            m_fileStream.write(reinterpret_cast<const char*>(pixelData), pixelDataSize);
            writeZerosIntoBinaryStream(m_fileStream, maskSize);
        }

        // PNG Compressed path using WIC
        void addBitmapPng(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            auto pngData = encodeToPng(width, height, pixelData);
            if (pngData.empty()) return;

            ICONDIRENTRY_Partial entry;
            entry.bWidth = (width >= 256) ? 0 : static_cast<std::uint8_t>(width);
            entry.bHeight = (height >= 256) ? 0 : static_cast<std::uint8_t>(height);
            entry.dwBytesInRes = static_cast<std::uint32_t>(pngData.size());
            entry.dwImageOffset = static_cast<std::uint32_t>(m_fileStream.tellp());
            m_entries.push_back(entry);

            m_fileStream.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
        }

        std::vector<std::uint8_t> encodeToPng(std::uint32_t width, std::uint32_t height, const Color* pixelData)
        {
            using Microsoft::WRL::ComPtr;

            ComPtr<IWICImagingFactory> factory;
            if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
                return {};

            ComPtr<IStream> stream;
            if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
                return {};

            ComPtr<IWICBitmapEncoder> encoder;
            if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)))
                return {};

            if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)))
                return {};

            ComPtr<IWICBitmapFrameEncode> frame;
            if (FAILED(encoder->CreateNewFrame(&frame, nullptr)))
                return {};

            if (FAILED(frame->Initialize(nullptr)))
                return {};

            if (FAILED(frame->SetSize(width, height)))
                return {};

            // Standard ICO PNGs use BGRA
            WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
            if (FAILED(frame->SetPixelFormat(&format)))
                return {};

            // --- FIX FOR BOTTOM-UP INPUT DATA ---
            std::uint32_t stride = width * sizeof(Color);

            // We iterate backwards through the source rows (bottom-to-top)
            // and write them into the PNG encoder (which expects top-to-bottom).
            for (std::int32_t y = static_cast<std::int32_t>(height) - 1; y >= 0; --y)
            {
                const Color* rowPtr = pixelData + (y * width);

                // Write exactly one row at a time
                HRESULT hr = frame->WritePixels(
                    1,                          // lineCount: 1 row
                    stride,                     // cbStride: size of one row
                    stride,                     // cbBufferSize: size of the buffer provided
                    reinterpret_cast<BYTE*>(const_cast<Color*>(rowPtr))
                );

                if (FAILED(hr)) return {};
            }

            if (FAILED(frame->Commit()))
                return {};

            if (FAILED(encoder->Commit()))
                return {};

            // Copy stream to vector (same as before)
            STATSTG stats;
            stream->Stat(&stats, STATFLAG_NONAME);
            std::vector<std::uint8_t> buffer(static_cast<std::size_t>(stats.cbSize.QuadPart));

            LARGE_INTEGER zero = { 0 };
            stream->Seek(zero, STREAM_SEEK_SET, nullptr);
            ULONG read;
            stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &read);

            return buffer;
        }
#endif

        std::ofstream m_fileStream;
        std::vector<ICONDIRENTRY_Partial> m_entries;
        std::size_t m_headerPaddingSize;
    };
}
