module;
#include "Wayland.Headers.h"
#include <sys/mman.h>
#include <cstdio>
export module ClaFi.Platform.Wayland.ShmBuffer;

// For traceEnabled, the one thing of the display this unit reads.
import ClaFi.Platform.Wayland.Display;
import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Wayland
{
    // Pixels the compositor can read without being sent them. See Platform
    export class ShmBuffers
    {
    public:
        struct Frame
        {
            wl_buffer* handle{ nullptr };
            // Into the mapping, not owned. Tightly packed, top-down, four bytes to a pixel - the
            // same bytes a Graphics::Bitmap holds, which is why a frame is a copy and not a
            // conversion.
            void* pixels{ nullptr };
            // The compositor has this one and has not given it back.
            bool busy{ false };
            // WHAT THIS FRAME HAS YET TO BE TOLD, and the reason damage is held here rather than
            // once for the window. A frame is written every other present, so what it holds is the
            // image from two presents ago and not the one on screen. A partial copy into it is
            // right only if it carries everything that has changed since IT was last written -
            // handed the damage since the last present instead, it keeps whatever the frame between
            // the two changed, and shows a mix of the two images.
            IntRect damage{};
        };
    public:
        // Takes the shm global the pool is made on - null where the compositor advertised none,
        // and then nothing is ever cut.
        explicit ShmBuffers(wl_shm*);
        ~ShmBuffers();
        ShmBuffers(const ShmBuffers&) = delete;
        ShmBuffers& operator=(const ShmBuffers&) = delete;
    public:
        /// @brief Whether these frames carry an alpha channel. Stated once, before the first
        /// resize: it decides the format every buffer is cut with, and a change afterwards would
        /// mean re-cutting all of them.
        void useAlphaChannel(bool value) { m_hasAlphaChannel = value; }
        [[nodiscard]] bool hasAlphaChannel() const { return m_hasAlphaChannel; }
        // Cuts two frames of this size, growing the pool if it is not already big enough.
        // Answers whether there are frames to paint into afterwards.
        bool resize(IntSize);
        [[nodiscard]] IntSize size() const { return m_size; }
        [[nodiscard]] int stride() const { return m_size.x * 4; }
        // The frame nothing is holding, or null while the compositor holds both - which is not a
        // fault. It means this client is drawing faster than the screen refreshes, and the frame
        // it would have drawn is one nobody would have seen.
        [[nodiscard]] Frame* freeFrame();
        // Says the compositor is done with a frame. Called from the buffer's own listener.
        void releaseFrame(wl_buffer*);
        // Adds to what EVERY frame has yet to be told. The one not painted this time still has to
        // carry this when its turn comes.
        void addDamage(const IntRect&);
    private:
        // Makes the pool at least this big, keeping whatever is already mapped. Answers whether
        // there is that much to cut frames from.
        bool growTo(std::size_t bytes);
        void destroyFrames();
        static void onBufferRelease(void* data, wl_buffer*);
    private:
        wl_shm* const m_shm;
        IntSize m_size{ 0, 0 };
        bool m_hasAlphaChannel{ false };
        // KEPT OPEN FOR THE LIFE OF THE POOL. Growing means growing the file first, and a
        // descriptor that was closed cannot be truncated. The compositor holds its own duplicate
        // and is unaffected by what this one does.
        int m_fd{ -1 };
        wl_shm_pool* m_pool{ nullptr };
        void* m_mapping{ nullptr };
        // WHAT IS MAPPED, which is not what is in use. A pool only ever grows, so this is the
        // high-water mark of every size this window has been.
        std::size_t m_capacity{ 0 };
        std::array<Frame, 2> m_frames{};
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::PlatformImplementation::Wayland
{
    // XRGB RATHER THAN ARGB BY DEFAULT, and this is a decision about the BACKEND rather than about
    // the compositor. The CPU backend does not promise a correct alpha channel across the whole
    // surface - it writes alpha where it blends and leaves it alone elsewhere - so a format that
    // is read as translucent shows the desktop through everything that was never blended. Stating
    // no alpha channel says what is true of these bytes.
    //
    // A WINDOW THAT FADES IS THE EXCEPTION, and it is exempt for the same reason the rule exists:
    // it does not trust the bitmap's alpha either. Its frame is written a channel at a time with
    // the WINDOW's opacity - see FormWindow::paint - so every pixel carries the same alpha
    // whatever the backend left there, which is what a layered window means on Win32. Both
    // formats are guaranteed by wl_shm.
    constexpr std::uint32_t k_opaqueFormat = WL_SHM_FORMAT_XRGB8888;
    constexpr std::uint32_t k_alphaFormat = WL_SHM_FORMAT_ARGB8888;
    constexpr int k_bytesPerPixel = 4;

    ShmBuffers::ShmBuffers(wl_shm* shm)
        :
        m_shm{ shm }
    {
    }

    ShmBuffers::~ShmBuffers()
    {
        destroyFrames();
        if (m_pool)
            ::wl_shm_pool_destroy(m_pool);
        if (m_mapping)
            ::munmap(m_mapping, m_capacity);
        if (m_fd >= 0)
            ::close(m_fd);
    }

    bool ShmBuffers::resize(IntSize value)
    {
        if (value.x <= 0 || value.y <= 0)
            return false;
        if (value == m_size && m_pool)
            return true;

        const std::size_t frameSize = static_cast<std::size_t>(value.x) * value.y * k_bytesPerPixel;
        if (!growTo(frameSize * m_frames.size()))
            return false;

        // CUT AFTER THE GROWTH AND NEVER BEFORE. A buffer names an offset and a size within the
        // pool, so every one of them is wrong the moment either the pool or the frame size
        // changes - and the mapping may have moved underneath them as well.
        destroyFrames();

        static const wl_buffer_listener k_bufferListener = {
            .release = &ShmBuffers::onBufferRelease
        };
        for (std::size_t i = 0; i < m_frames.size(); ++i)
        {
            Frame& frame = m_frames[i];
            frame.handle = ::wl_shm_pool_create_buffer(m_pool,
                static_cast<std::int32_t>(frameSize * i),
                value.x, value.y, value.x * k_bytesPerPixel,
                m_hasAlphaChannel ? k_alphaFormat : k_opaqueFormat);
            frame.pixels = static_cast<std::byte*>(m_mapping) + frameSize * i;
            frame.busy = false;
            // WHOLE, BECAUSE A RE-CUT FRAME HOLDS NOTHING IT CAN BE TRUSTED TO KEEP. The pool is
            // never cleared and never shrinks, so these bytes are the last window's image at the
            // last window's stride. Only a frame written edge to edge is rid of it.
            frame.damage = IntRect::fromDimensions({ 0, 0 }, value);
            ::wl_buffer_add_listener(frame.handle, &k_bufferListener, this);
        }

        m_size = value;
        return true;
    }

    ShmBuffers::Frame* ShmBuffers::freeFrame()
    {
        for (Frame& frame : m_frames)
            if (frame.handle && !frame.busy)
                return &frame;

        return nullptr;
    }

    void ShmBuffers::releaseFrame(wl_buffer* buffer)
    {
        for (Frame& frame : m_frames)
            if (frame.handle == buffer)
            {
                frame.busy = false;
                return;
            }
    }

    void ShmBuffers::addDamage(const IntRect& value)
    {
        for (Frame& frame : m_frames)
        {
            if (frame.damage.empty())
                frame.damage = value;
            else
                frame.damage.unionWith(value);
        }
    }

    // A POOL GROWS AND NEVER SHRINKS - the protocol has no request for it - so a window made
    // smaller keeps the mapping it had and cuts smaller frames from it. That is the whole reason
    // this is separate from resize(): dragging an edge asks for a new size on every frame, and
    // rebuilding the file, the mapping and the pool a hundred times a second is a great deal of
    // work to arrive back where it started.
    bool ShmBuffers::growTo(std::size_t bytes)
    {
        if (m_pool && bytes <= m_capacity)
            return true;

        // HALF AS MUCH AGAIN, so a drag that grows a window by a pixel at a time stops asking
        // after the first few. The memory is not touched until it is written into, so a capacity
        // ahead of the size costs address space rather than pages.
        const std::size_t capacity = std::max(bytes, m_capacity + m_capacity / 2);

        if (m_fd < 0)
        {
            // ANONYMOUS AND NAMED NOTHING. memfd is memory with a file descriptor on it and no
            // entry in any directory, which is what this needs: a name would be a file another
            // process could open, and there is nothing here to share with anyone but the
            // compositor.
            m_fd = ::memfd_create("clafi-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
            if (!Linux::check(m_fd >= 0))
                return false;
        }

        if (!Linux::check(::ftruncate(m_fd, static_cast<off_t>(capacity)) == 0))
            return false;

        if (!m_mapping)
        {
            m_mapping = ::mmap(nullptr, capacity, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
            if (!Linux::check(m_mapping != MAP_FAILED))
            {
                m_mapping = nullptr;
                return false;
            }
        }
        else
        {
            // MAY MOVE, so every pointer into the old mapping is stale afterwards. The frames are
            // re-cut by the caller, which is what makes that safe.
            void* moved = ::mremap(m_mapping, m_capacity, capacity, MREMAP_MAYMOVE);
            if (moved == MAP_FAILED)
                return false;

            m_mapping = moved;
        }

        if (m_pool)
        {
            ::wl_shm_pool_resize(m_pool, static_cast<std::int32_t>(capacity));
        }
        else
        {
            if (!m_shm)
                return false;

            m_pool = ::wl_shm_create_pool(m_shm, m_fd, static_cast<std::int32_t>(capacity));
        }

        m_capacity = capacity;

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] shm grow  capacity=%zu\n", m_capacity);

        return true;
    }

    void ShmBuffers::destroyFrames()
    {
        for (Frame& frame : m_frames)
        {
            // A BUFFER THE COMPOSITOR STILL HOLDS MAY BE DESTROYED. It has already read what it
            // needed, and one it has not read is a frame nobody was going to see at a size this
            // window no longer is.
            if (frame.handle)
                ::wl_buffer_destroy(frame.handle);
            frame = {};
        }
    }

    void ShmBuffers::onBufferRelease(void* data, wl_buffer* buffer)
    {
        static_cast<ShmBuffers*>(data)->releaseFrame(buffer);
    }
}
