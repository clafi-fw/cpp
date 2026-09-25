module;
#include "Windows.Headers.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
export module ClaFi.Platform.Windows.Composition;

import ClaFi.Platform.Windows.Diagnostic;
import ClaFi.Diagnostic.Options;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    using Microsoft::WRL::ComPtr;

    // The pixels of a composed window, hung on a DirectComposition visual. See Platform
    export class CompositionPresenter
    {
    public:
        explicit CompositionPresenter(HWND);
        // The size of the swap chain - the window's client area. Contents are not kept across a
        // resize; the caller repaints whole, which it does anyway for a window that changed size.
        void resize(IntSize);
        // The device everything is built on, made on demand. Null when none can be made.
        [[nodiscard]] ID3D11Device* d3dDevice();
        // Which device this is. Moves when the device is lost and made again.
        [[nodiscard]] unsigned generation() const { return m_generation; }
        // THE FRAME TEXTURE, at the size of the whole surface, for a backend that draws on the
        // device. Made or remade to the size asked for; its contents are then undefined. Null
        // when there is no device.
        [[nodiscard]] IDXGISurface* renderSurface(IntSize);
        // Copies sourceRect of the bitmap into the back buffer at destination, and presents.
        // A present holds the thread while the chain has no room for another frame, until the
        // compositor takes one; a caller inside a resize step is paced so that it never has to -
        // see FormWindow's WM_SIZE.
        [[nodiscard]] bool present(const Graphics::Bitmap& source, const IntRect& sourceRect,
            IntPoint destination);
        // The same from the frame texture.
        [[nodiscard]] bool presentSurface(const IntRect& sourceRect, IntPoint destination);
        void setOpacity(float);
    private:
        [[nodiscard]] bool ensureDevice();
        void discardDevice();
        [[nodiscard]] bool ensureSwapChain();
        [[nodiscard]] bool commit();
        // Presents the frame region that lands on target in the chain. The source is the bitmap
        // where one is given and the frame texture otherwise; shift carries chain coordinates
        // back to source coordinates.
        [[nodiscard]] bool presentFrom(const Graphics::Bitmap* bitmap, IntSize sourceSize,
            const IntRect& target, IntPoint shift);
        void upload(ID3D11Texture2D&, const Graphics::Bitmap& source, const IntRect& sourceRect,
            IntPoint destination) const;
        void copy(ID3D11Texture2D& into, ID3D11Texture2D& from, const IntRect& sourceRect,
            IntPoint destination) const;
    private:
        static constexpr UINT k_bufferCount = 2;
        HWND m_window;
        IntSize m_size{};
        IntSize m_chainSize{};
        IntSize m_frameSize{};
        float m_opacity{ 1.0f };
        unsigned m_generation{ 0 };
        // What the last present changed, which the buffer about to be written has not seen; and
        // how many buffers still hold nothing at all, after a resize or a rebuild.
        IntRect m_stale{};
        UINT m_wholeUploadsLeft{ k_bufferCount };
        ComPtr<ID3D11Device> m_d3dDevice;
        ComPtr<ID3D11DeviceContext> m_d3dContext;
        ComPtr<IDXGISwapChain1> m_swapChain;
        ComPtr<ID3D11Texture2D> m_frameTexture;
        ComPtr<IDXGISurface> m_frameSurface;
        ComPtr<IDCompositionDevice> m_device;
        ComPtr<IDCompositionTarget> m_target;
        ComPtr<IDCompositionVisual> m_visual;
        ComPtr<IDCompositionEffectGroup> m_effect;
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Windows
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        // What a swap chain call held the thread for, said to the debugger when it is a wait
        // and not the call itself. See Diagnostic::Options::logPresentWait
        void logWait(const wchar_t* call, Clock::time_point startedAt)
        {
            const double waitedMs =
                std::chrono::duration<double, std::milli>(Clock::now() - startedAt).count();
            if (waitedMs < 1.0)
                return;
            const std::wstring line = std::format(L"{} waited {:.1f} ms\n", call, waitedMs);
            ::OutputDebugStringW(line.c_str());
        }
    }

    CompositionPresenter::CompositionPresenter(HWND window)
        :
        m_window{ window }
    {
    }

    void CompositionPresenter::resize(IntSize value)
    {
        m_size = { std::max(0, value.x), std::max(0, value.y) };
    }

    ID3D11Device* CompositionPresenter::d3dDevice()
    {
        if (!ensureDevice())
            return nullptr;
        return m_d3dDevice.Get();
    }

    IDXGISurface* CompositionPresenter::renderSurface(IntSize size)
    {
        if (!ensureDevice() || size.x <= 0 || size.y <= 0)
            return nullptr;
        if (m_frameSurface && m_frameSize == size)
            return m_frameSurface.Get();

        m_frameSurface.Reset();
        m_frameTexture.Reset();
        m_frameSize = {};

        D3D11_TEXTURE2D_DESC description{};
        description.Width = static_cast<UINT>(size.x);
        description.Height = static_cast<UINT>(size.y);
        description.MipLevels = 1;
        description.ArraySize = 1;
        description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = m_d3dDevice->CreateTexture2D(&description, nullptr, &m_frameTexture);
        if (SUCCEEDED(hr))
            hr = m_frameTexture.As(&m_frameSurface);
        if (FAILED(hr))
        {
            discardDevice();
            return nullptr;
        }
        m_frameSize = size;
        return m_frameSurface.Get();
    }

    bool CompositionPresenter::present(const Graphics::Bitmap& source, const IntRect& sourceRect,
        IntPoint destination)
    {
        const IntPoint shift{ destination.x - sourceRect.left, destination.y - sourceRect.top };
        IntRect target = sourceRect;
        target.offset(shift);
        return presentFrom(&source, { source.width(), source.height() }, target, shift);
    }

    bool CompositionPresenter::presentSurface(const IntRect& sourceRect, IntPoint destination)
    {
        if (!m_frameTexture)
            return false;
        const IntPoint shift{ destination.x - sourceRect.left, destination.y - sourceRect.top };
        IntRect target = sourceRect;
        target.offset(shift);
        return presentFrom(nullptr, m_frameSize, target, shift);
    }

    void CompositionPresenter::setOpacity(float value)
    {
        m_opacity = value;
        if (!m_effect)
            return;
        checkHr(m_effect->SetOpacity(value));
        std::ignore = commit();
    }

    // HARDWARE FIRST, WARP SECOND. A session without a usable GPU - a remote desktop, a virtual
    // machine, a driver mid-update - still composes, only in software.
    bool CompositionPresenter::ensureDevice()
    {
        if (m_device)
            return true;

        constexpr UINT k_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, k_flags,
            nullptr, 0, D3D11_SDK_VERSION, &m_d3dDevice, nullptr, &m_d3dContext);
        if (FAILED(hr))
            hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, k_flags,
                nullptr, 0, D3D11_SDK_VERSION, &m_d3dDevice, nullptr, &m_d3dContext);
        if (FAILED(hr))
        {
            discardDevice();
            return false;
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_d3dDevice.As(&dxgiDevice);
        if (SUCCEEDED(hr))
            hr = ::DCompositionCreateDevice(dxgiDevice.Get(), __uuidof(IDCompositionDevice),
                reinterpret_cast<void**>(m_device.GetAddressOf()));
        if (SUCCEEDED(hr))
            hr = m_device->CreateTargetForHwnd(m_window, TRUE, &m_target);
        if (SUCCEEDED(hr))
            hr = m_device->CreateVisual(&m_visual);
        if (SUCCEEDED(hr))
            hr = m_device->CreateEffectGroup(&m_effect);
        if (SUCCEEDED(hr))
            hr = m_effect->SetOpacity(m_opacity);
        if (SUCCEEDED(hr))
            hr = m_visual->SetEffect(m_effect.Get());
        if (SUCCEEDED(hr))
            hr = m_target->SetRoot(m_visual.Get());
        if (FAILED(hr))
        {
            discardDevice();
            return false;
        }
        ++m_generation;
        return true;
    }

    void CompositionPresenter::discardDevice()
    {
        m_frameSurface.Reset();
        m_frameTexture.Reset();
        m_frameSize = {};
        m_swapChain.Reset();
        m_chainSize = {};
        m_wholeUploadsLeft = k_bufferCount;
        m_effect.Reset();
        m_visual.Reset();
        m_target.Reset();
        m_device.Reset();
        m_d3dContext.Reset();
        m_d3dDevice.Reset();
    }

    // Never made at zero: a window between sizes has nothing to show and a zero chain is an error.
    bool CompositionPresenter::ensureSwapChain()
    {
        if (m_size.x <= 0 || m_size.y <= 0)
            return false;
        if (m_swapChain && m_chainSize == m_size)
            return true;

        HRESULT hr;
        if (!m_swapChain)
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIFactory2> factory;
            hr = m_d3dDevice.As(&dxgiDevice);
            if (SUCCEEDED(hr))
                hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
                hr = adapter->GetParent(__uuidof(IDXGIFactory2),
                    reinterpret_cast<void**>(factory.GetAddressOf()));

            DXGI_SWAP_CHAIN_DESC1 description{};
            description.Width = static_cast<UINT>(m_size.x);
            description.Height = static_cast<UINT>(m_size.y);
            description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            description.SampleDesc.Count = 1;
            description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            description.BufferCount = k_bufferCount;
            description.Scaling = DXGI_SCALING_STRETCH;
            description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            if (SUCCEEDED(hr))
                hr = factory->CreateSwapChainForComposition(m_d3dDevice.Get(), &description,
                    nullptr, &m_swapChain);
            if (SUCCEEDED(hr))
                hr = m_visual->SetContent(m_swapChain.Get());
            if (SUCCEEDED(hr))
                hr = m_device->Commit();
        }
        else
        {
            const Clock::time_point startedAt =
                Diagnostic::Options::logPresentWait ? Clock::now() : Clock::time_point{};
            hr = m_swapChain->ResizeBuffers(0, static_cast<UINT>(m_size.x),
                static_cast<UINT>(m_size.y), DXGI_FORMAT_UNKNOWN, 0);
            if constexpr (Diagnostic::Options::logPresentWait)
                logWait(L"ResizeBuffers", startedAt);
        }

        if (FAILED(hr))
        {
            discardDevice();
            return false;
        }
        m_chainSize = m_size;
        m_wholeUploadsLeft = k_bufferCount;
        return true;
    }

    bool CompositionPresenter::commit()
    {
        if (!m_device)
            return false;
        if (FAILED(m_device->Commit()))
        {
            discardDevice();
            return false;
        }
        return true;
    }

    bool CompositionPresenter::presentFrom(const Graphics::Bitmap* bitmap, IntSize sourceSize,
        const IntRect& target, IntPoint shift)
    {
        if (!ensureDevice() || !ensureSwapChain())
            return false;

        // Held inside both the source and the chain: a resize can leave the two a frame apart.
        IntRect chainTarget = target;
        chainTarget.intersectWith({ 0, 0, m_chainSize.x, m_chainSize.y });
        if (chainTarget.empty())
            return true;

        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(backBuffer.GetAddressOf()));
        if (FAILED(hr))
        {
            discardDevice();
            return false;
        }

        // THE NEW FRAME'S RECT AND WHAT THIS BUFFER MISSED, both from the source, which holds the
        // whole current frame; the runtime is told the same rect, since that is what changed in
        // this buffer.
        IntRect copyTarget = chainTarget;
        if (m_wholeUploadsLeft > 0)
        {
            copyTarget = { 0, 0, m_chainSize.x, m_chainSize.y };
            --m_wholeUploadsLeft;
        }
        else
            copyTarget.unionWith(m_stale);
        IntRect copySource = copyTarget;
        copySource.offset(-shift.x, -shift.y);
        copySource.intersectWith({ 0, 0, sourceSize.x, sourceSize.y });
        copyTarget = copySource;
        copyTarget.offset(shift);
        if (copyTarget.empty())
            return true;
        if (bitmap)
            upload(*backBuffer.Get(), *bitmap, copySource, copyTarget.topLeft());
        else
            copy(*backBuffer.Get(), *m_frameTexture.Get(), copySource, copyTarget.topLeft());

        RECT dirty{ copyTarget.left, copyTarget.top, copyTarget.right, copyTarget.bottom };
        DXGI_PRESENT_PARAMETERS parameters{};
        parameters.DirtyRectsCount = 1;
        parameters.pDirtyRects = &dirty;
        constexpr Clock::time_point startedAt =
            Diagnostic::Options::logPresentWait ? Clock::now() : Clock::time_point{};
        hr = m_swapChain->Present1(1, 0, &parameters);
        if constexpr (Diagnostic::Options::logPresentWait)
            logWait(L"Present1", startedAt);
        if (FAILED(hr))
        {
            discardDevice();
            return false;
        }
        m_stale = chainTarget;
        return true;
    }

    void CompositionPresenter::upload(ID3D11Texture2D& texture, const Graphics::Bitmap& source,
        const IntRect& sourceRect, IntPoint destination) const
    {
        if (sourceRect.empty())
            return;
        const D3D11_BOX box{
            static_cast<UINT>(destination.x),
            static_cast<UINT>(destination.y),
            0,
            static_cast<UINT>(destination.x + sourceRect.width()),
            static_cast<UINT>(destination.y + sourceRect.height()),
            1
        };
        const Color* pixels = source.data()
            + static_cast<std::size_t>(sourceRect.top) * source.stride() + sourceRect.left;
        m_d3dContext->UpdateSubresource(&texture, 0, &box, pixels,
            static_cast<UINT>(source.stride() * sizeof(Color)), 0);
    }

    void CompositionPresenter::copy(ID3D11Texture2D& into, ID3D11Texture2D& from,
        const IntRect& sourceRect, IntPoint destination) const
    {
        if (sourceRect.empty())
            return;
        const D3D11_BOX box{
            static_cast<UINT>(sourceRect.left),
            static_cast<UINT>(sourceRect.top),
            0,
            static_cast<UINT>(sourceRect.right),
            static_cast<UINT>(sourceRect.bottom),
            1
        };
        m_d3dContext->CopySubresourceRegion(&into, 0,
            static_cast<UINT>(destination.x), static_cast<UINT>(destination.y), 0, &from, 0, &box);
    }
}
