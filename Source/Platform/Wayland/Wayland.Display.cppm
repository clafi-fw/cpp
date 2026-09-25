module;
#include "Wayland.Headers.h"
#include <cerrno>
#include <cstdio>
export module ClaFi.Platform.Wayland.Display;

// Exported with this module because the sink below speaks in its terms: whoever implements the
// sink is handed a KeyPress.
export import ClaFi.Platform.Wayland.Keyboard;

import ClaFi.Platform.Linux.Appearance;
import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Wayland
{
    // What the display delivers to, knowing nothing of what a window is. See Platform
    export class INativeEventSink
    {
    public:
        virtual ~INativeEventSink() = default;

        // EVERY POSITION HERE IS IN SURFACE COORDINATES - the space the display server states,
        // which is not the space anything is drawn in. Whoever implements this converts, and
        // nothing above the platform layer sees this space at all.
        virtual void onPointerMoved(IntPoint surfacePoint) = 0;
        // `time` is the server's own clock in milliseconds. It is what a run of clicks is counted
        // against, no display server counting them for a client. It is NOT the InputStamp beside
        // it: that is the number a request made on the user's behalf is authorised by, and the
        // two are different numbers with different rules.
        virtual void onPointerButton(IntPoint surfacePoint, std::uint32_t time, InputStamp,
            bool down) = 0;
        // A press asking for a menu about whatever is under the pointer. Separate from a button
        // because it is answered on the press and no release is ever matched to it.
        virtual void onPointerContextMenu(IntPoint surfacePoint, InputStamp) = 0;
        virtual void onPointerWheel(IntPoint surfacePoint, float delta, bool horizontal) = 0;
        virtual void onPointerLeft() = 0;

        // A KEY, AS THE KEYBOARD TRANSLATED IT. The keyboard belongs to the seat as the pointer
        // does, and names the surface it is over on enter; that surface's sink hears every press
        // until the leave. The character a press types comes SEPARATELY AND SECOND, as WM_CHAR
        // follows WM_KEYDOWN - the form counts on the order, a press handed to a popup leaving a
        // flag the character then spends - and comes at all only if the press left the window
        // standing: a key may close the form it was typed into.
        virtual void onKeyPressed(const KeyPress&) = 0;
        virtual void onCharacter(wchar_t) = 0;
        virtual void onKeyReleased() = 0;

        // The queue is empty. Anything the sink deferred while events were arriving - a paint,
        // above all - happens here. It is the same arrangement Win32 arrives at from the other
        // side: WM_PAINT is synthesised only when nothing else is waiting, so a window damaged
        // twenty times during a drag paints once at the end of it.
        virtual void onIdle() = 0;
    };

    // Set CLAFI_WAYLAND_TRACE=1 and the layer says what it connected to and what it bound.
    export [[nodiscard]] bool traceEnabled();

    // A selection, and which of the two clipboard channels it arrived on. See Platform
    export struct SelectionOffer
    {
        wl_data_offer* seat{ nullptr };
        ext_data_control_offer_v1* control{ nullptr };

        [[nodiscard]] bool empty() const;
        [[nodiscard]] bool operator==(const SelectionOffer&) const = default;
    };

    // The connection, the globals and the loop, one per process and the platform's member. See
    // Platform
    export class DisplayManager
    {
    public:
        using SelectionHandler = std::function<void()>;
        using Dispatch = std::function<void()>;
        // An extension global this layer uses where the compositor offers it.
        struct OptionalGlobal
        {
            std::string_view name;
            bool offered{ false };
        };
        using OptionalGlobals = std::array<OptionalGlobal, 8>;
    public:
        // CONNECTS, OR THROWS. A client started outside a session has nothing to draw into, and
        // the error names what was looked for - the value of WAYLAND_DISPLAY.
        DisplayManager();
        ~DisplayManager();
        DisplayManager(const DisplayManager&) = delete;
        DisplayManager& operator=(const DisplayManager&) = delete;
    public:
        // THE ONE STANDING, or null while the platform holds none. What the Platform statics -
        // the cursor, the modifiers, the session - reach the connection through, having no
        // object in hand; everything else is handed the display by whoever made it. The
        // manager sets it as it is built and clears it as it goes, and the platform stands
        // before anything that asks and after it, so a static finding none is a broken order.
        [[nodiscard]] static DisplayManager* standing() { return s_standing; }
        [[nodiscard]] wl_display* display() const { return m_display; }
        [[nodiscard]] wl_compositor* compositor() const { return m_compositor; }
        [[nodiscard]] wl_shm* shm() const { return m_shm; }
        [[nodiscard]] xdg_wm_base* windowManager() const { return m_windowManager; }
        // WHAT A REQUEST MADE ON THE USER'S BEHALF IS AUTHORISED AGAINST, together with the
        // serial in an InputStamp. Null until a seat is advertised, which is normal on a machine
        // with no input devices at all.
        [[nodiscard]] wl_seat* seat() const { return m_seat; }
        // Null where the compositor does not advertise it, which is the standing answer on
        // Mutter. A client that draws its own frame then simply draws it, nobody having asked.
        [[nodiscard]] zxdg_decoration_manager_v1* decorationManager() const { return m_decorationManager; }
        // THE SEAT'S CLIPBOARD AND DRAG CHANNEL. Null until both the manager and the seat are
        // advertised. The requests on it are made by whoever owns the content - see
        // Transfer::Clipboard - but the device and its listener live here, because a listener has
        // to be on the device before the first selection event arrives and nothing above this
        // layer exists that early.
        [[nodiscard]] wl_data_device* dataDevice() const { return m_dataDevice; }
        [[nodiscard]] wl_data_device_manager* dataDeviceManager() const { return m_dataDeviceManager; }
        // THE SELECTION AS THE COMPOSITOR STATES IT: the offer object, and the MIME names it
        // advertised in the order they arrived. Empty while there is nothing this client can read.
        //
        // WHICH CHANNEL IT ARRIVES ON IS THE WHOLE OF THE FOCUS QUESTION. On the seat's data
        // device a selection is announced only while this client holds the keyboard focus, so a
        // window that has never been focused sees nothing here. On a data control device it is
        // announced whatever the focus.
        [[nodiscard]] SelectionOffer selectionOffer() const { return m_selectionOffer; }
        // Asks the compositor for the selection's bytes in one format, down a pipe the caller
        // opened. Reading that pipe is the caller's - see Transfer::Clipboard - and this is only
        // the one request the two channels spell differently.
        void receiveSelection(const SelectionOffer&, const char* mimeName, int fd) const;
        // TAKES THE CLIPBOARD OUT OF THE KEYBOARD'S HANDS, for an application that watches the
        // clipboard rather than merely pastes from it. Where the compositor advertises the data
        // control global this makes a device on it, and that device becomes the one channel the
        // selection is read through; where it does not, nothing changes and the focus rule stands.
        // Called once, from the platform's constructor, by an application that asked for it.
        void createDataControlDevice();
        [[nodiscard]] const std::vector<std::string>& selectionMimeTypes() const
        {
            return m_selectionMimeTypes;
        }
        // WHO TO TELL WHEN THE SELECTION CHANGES. One handler, installed by the transfer layer,
        // because the dependency runs that way and only that way: Transfer imports this module,
        // so this module cannot import Transfer. A callable rather than an event - there is one
        // listener, the clipboard the platform owns, which installs it on its first watch and
        // clears it on its way out, and this layer keeps no event vocabulary of its own.
        void setSelectionHandler(SelectionHandler value) { m_selectionHandler = std::move(value); }
        // A SURFACE ANCHORED TO THE SCREEN'S EDGES, which xdg-shell cannot say. Null where the
        // compositor does not advertise it - Mutter - and a window that asked for an edge is
        // then an ordinary toplevel placed by the compositor's own rule.
        [[nodiscard]] zwlr_layer_shell_v1* layerShell() const { return m_layerShell; }
        //
        // WHAT THE DESKTOP KNOWS THIS CLIENT BY - every toplevel is made under it. Stated by the
        // application before its first window; "clafi" stands for a client that states none.
        [[nodiscard]] const char* appId() const { return m_appId.c_str(); }
        void setAppId(std::string value) { m_appId = std::move(value); }
        // WHERE A WINDOW WAS IS THE COMPOSITOR'S TO REMEMBER, against a session id the client
        // persists. Null where the compositor does not advertise the global, and there a
        // toplevel is placed by the compositor's own rule on every launch. See Platform
        [[nodiscard]] xdg_session_manager_v1* sessionManager() const { return m_sessionManager; }
        // The application's session, asked for once with the id its config held - empty for
        // none - and answered by created or restored. Null without the global, and after
        // another instance of the application has taken the session over.
        void openSession(std::string_view storedId);
        [[nodiscard]] xdg_session_v1* session() const { return m_session; }
        // The id to persist: what was asked for, or what the compositor answered created with.
        [[nodiscard]] const std::string& sessionId() const { return m_sessionId; }
        // Waits for the id of a session just opened, and for nothing else. See Platform
        void awaitSessionId();
        // Asks the compositor to raise and focus a toplevel, on the input that asked. See Platform
        void activate(wl_surface* target, InputStamp);
        // The pixel formats wl_shm advertised. ARGB8888 and XRGB8888 are guaranteed by the
        // protocol; anything else is the compositor offering more.
        [[nodiscard]] const std::vector<std::uint32_t>& shmFormats() const { return m_shmFormats; }
        // HOW MANY REAL PIXELS ONE SURFACE PIXEL IS ON THIS OUTPUT. What a surface that overlaps
        // several of them should draw at is the largest of the ones it is on - drawing at the
        // smaller and being stretched up to the larger is the blurry half of the choice.
        [[nodiscard]] int scaleForOutput(wl_output*) const;
        // THE MOST ROWS ANY OUTPUT HAS, in the output's own pixels - its current mode. It is
        // what a window that wants a screen's height asks for before it is on any screen: a
        // toplevel is placed by the compositor, so which output it lands on is not known here,
        // and the tallest is the one ask that is not too short for any of them.
        [[nodiscard]] int tallestOutputHeight() const;
        // BOTH OR NEITHER. A buffer drawn at a fractional ratio has to be presented at the size
        // the compositor asked for, and the viewport is the only thing that can say so. A
        // compositor offering one without the other leaves the integer wl_output path standing.
        [[nodiscard]] wp_fractional_scale_manager_v1* fractionalScaleManager() const
        {
            return m_viewporter ? m_fractionalScaleManager : nullptr;
        }
        [[nodiscard]] wp_viewporter* viewporter() const { return m_viewporter; }
        //
        // THE POINTER IMAGE, BY NAME. The compositor draws it from its own theme at its own size,
        // which is the only way a cursor on Wayland is ever the right size on every screen; a
        // client that drew its own would have to scale it itself and would still miss the theme.
        // The shape is remembered and stated again on every enter, because a compositor forgets
        // it with the leave - and stated to the compositor only when it changes, because the form
        // names it on every motion.
        //
        // NOTHING WITHOUT THE PROTOCOL. A compositor not offering cursor-shape-v1 shows its
        // default arrow over this client's windows and nothing else, an I-beam included. Every
        // current desktop offers it.
        void setCursor(CursorShape);
        // The shape for a window edge, as an xdg_toplevel resize edge; zero names the arrow.
        void setResizeCursor(std::uint32_t edges);
        // The shape being shown, as the protocol numbers it, and one put back after a wait.
        [[nodiscard]] std::uint32_t cursorShape() const { return m_cursorShape; }
        void setCursorShape(std::uint32_t shape);
        // THE COMPOSITOR HAS THE POINTER, for a move or a resize it was asked to do. It shows its
        // own cursor meanwhile and sends nothing when it is done - no leave, no enter, and a
        // window dragged by its title has the pointer at the same point of it afterwards, so the
        // form sees no movement and names no shape. The next pointer event is the only signal,
        // and the remembered shape is stated again on it: a compositor is not obliged to put the
        // client's shape back, and KWin does not.
        void pointerHandedToCompositor();
        // The modifiers held, as of the keyboard's last word on them.
        [[nodiscard]] KeyModifiers keyModifiers() const { return m_keyboard.modifiers(); }
        //
        // A SINK IS REGISTERED WITH THE SURFACE IT IS ABOUT. Most events reach their window
        // without a lookup, because a listener carries the object it was added for - but the
        // POINTER belongs to the seat rather than to any window, and says which surface it is
        // over. That is what the surface has to be matched back to a sink for.
        //
        // A window with no surface - a message-only one, which exists to own a timer - passes
        // null and is reachable by the idle broadcast alone.
        void registerSink(INativeEventSink&, wl_surface*);
        void unregisterSink(INativeEventSink&);
        [[nodiscard]] std::size_t sinkCount() const { return m_sinks.size(); }
        //
        // Hand every sink whatever has arrived, then tell them the queue is empty.
        void dispatchPending();
        // Sleep until something is worth waking for. See the definition - the order of the calls
        // in it is the whole of why it does not deadlock.
        void waitForEvent();
        // A DESCRIPTOR THE LOOP WAITS ON BESIDE THE CONNECTION, and what to call when it is
        // readable - the timers' eventfd, the directory watches' inotify. Registered by whoever
        // owns both the descriptor and this display, which is the platform, so the loop names
        // none of them. See Platform
        void addPollSource(int fd, Dispatch);
        void removePollSource(int fd);
        [[nodiscard]] bool quitRequested() const { return m_quitRequested; }
        void requestQuit() { m_quitRequested = true; }
        //
        // What was connected to and what was bound, on stderr, under CLAFI_WAYLAND_TRACE.
        void reportGlobals() const;
        // Every extension global in the order they are bound, and whether each was offered.
        [[nodiscard]] OptionalGlobals optionalGlobals() const;
    private:
        struct SinkEntry
        {
            INativeEventSink* sink{ nullptr };
            wl_surface* surface{ nullptr };
        };
        // AN OFFER BEING DESCRIBED. A data_offer event creates the object and its MIME names
        // follow on it; the selection or enter event that says what the offer is FOR comes after
        // all of them, so the names are gathered against the object until then.
        struct PendingOffer
        {
            SelectionOffer offer{};
            std::vector<std::string> mimeTypes;
        };
        struct OutputEntry
        {
            wl_output* output{ nullptr };
            int scale{ 1 };
            // The current mode, in the output's own pixels.
            IntSize modeSize{};
        };
        struct Pollable
        {
            int fd;
            Dispatch dispatch;
        };
    private:
        void bindGlobal(wl_registry*, std::uint32_t name, std::string_view interfaceName,
            std::uint32_t version);
        [[nodiscard]] INativeEventSink* sinkForSurface(wl_surface*) const;
        // States the remembered shape to the compositor, against the serial of the enter that put
        // the pointer over this client. Nothing while the pointer is elsewhere.
        void sendCursorShape();
        // States it only if the compositor has not been told since it last forgot - an enter, or
        // a move or resize it performed. Called after a pointer event has been handed to the
        // form, so that a shape the form named on it wins over the remembered one.
        void restateCursorShape();
        // The first pointer event after a move or a resize the compositor performed. Called
        // BEFORE the event is handed to the form: the motion that hands the pointer over is
        // itself still in flight when the flag is raised, and must not be the one that spends it.
        void pointerReturned();
        // Makes the shape device for the pointer, once there is both a pointer and a manager.
        void createCursorShapeDevice();
        // Makes the data device, once there is both a seat and a data device manager.
        void createDataDevice();
        // Destroys one offer through the interface it belongs to. Nothing where it is empty.
        static void destroyOffer(const SelectionOffer&);
        // Hands one translated press to the sink holding the keyboard, in the order the sink
        // states.
        void deliverKeyPress(const KeyPress&);
        // The C callbacks. Static members rather than free functions so they can reach what they
        // are about to write; each is handed `this` as the listener's data pointer.
        static void onGlobal(void* data, wl_registry*, std::uint32_t name, const char* interfaceName,
            std::uint32_t version);
        static void onGlobalRemove(void* data, wl_registry*, std::uint32_t name);
        static void onPing(void* data, xdg_wm_base*, std::uint32_t serial);
        static void onSessionCreated(void* data, xdg_session_v1*, const char* sessionId);
        static void onSessionRestored(void* data, xdg_session_v1*);
        static void onSessionReplaced(void* data, xdg_session_v1*);
        static void onActivationTokenDone(void* data, xdg_activation_token_v1*, const char* token);
        static void onShmFormat(void* data, wl_shm*, std::uint32_t format);
        static void onSeatCapabilities(void* data, wl_seat*, std::uint32_t capabilities);
        static void onSeatName(void* data, wl_seat*, const char* name);
        static void onPointerEnter(void* data, wl_pointer*, std::uint32_t serial, wl_surface*,
            wl_fixed_t x, wl_fixed_t y);
        static void onPointerLeave(void* data, wl_pointer*, std::uint32_t serial, wl_surface*);
        static void onPointerMotion(void* data, wl_pointer*, std::uint32_t time,
            wl_fixed_t x, wl_fixed_t y);
        static void onPointerButton(void* data, wl_pointer*, std::uint32_t serial, std::uint32_t time,
            std::uint32_t button, std::uint32_t state);
        static void onPointerAxis(void* data, wl_pointer*, std::uint32_t time, std::uint32_t axis,
            wl_fixed_t value);
        static void onPointerFrame(void* data, wl_pointer*);
        static void onPointerAxisSource(void* data, wl_pointer*, std::uint32_t axisSource);
        static void onPointerAxisStop(void* data, wl_pointer*, std::uint32_t time,
            std::uint32_t axis);
        static void onPointerAxisDiscrete(void* data, wl_pointer*, std::uint32_t axis,
            std::int32_t discrete);
        static void onKeyboardKeymap(void* data, wl_keyboard*, std::uint32_t format, std::int32_t fd,
            std::uint32_t size);
        static void onKeyboardEnter(void* data, wl_keyboard*, std::uint32_t serial, wl_surface*,
            wl_array* keys);
        static void onKeyboardLeave(void* data, wl_keyboard*, std::uint32_t serial, wl_surface*);
        static void onKeyboardKey(void* data, wl_keyboard*, std::uint32_t serial, std::uint32_t time,
            std::uint32_t key, std::uint32_t state);
        static void onKeyboardModifiers(void* data, wl_keyboard*, std::uint32_t serial,
            std::uint32_t depressed, std::uint32_t latched, std::uint32_t locked, std::uint32_t group);
        static void onKeyboardRepeatInfo(void* data, wl_keyboard*, std::int32_t rate, std::int32_t delay);
        static void onOutputGeometry(void* data, wl_output*, std::int32_t x, std::int32_t y,
            std::int32_t physicalWidth, std::int32_t physicalHeight, std::int32_t subpixel,
            const char* make, const char* model, std::int32_t transform);
        static void onOutputMode(void* data, wl_output*, std::uint32_t flags, std::int32_t width,
            std::int32_t height, std::int32_t refresh);
        static void onOutputDone(void* data, wl_output*);
        static void onOutputScale(void* data, wl_output*, std::int32_t factor);
        static void onDataOfferMimeType(void* data, wl_data_offer*, const char* mimeType);
        static void onDataOffered(void* data, wl_data_device*, wl_data_offer*);
        static void onSelection(void* data, wl_data_device*, wl_data_offer*);
        // A DRAG FROM ANOTHER CLIENT, refused. Taking no drags is said by accepting no MIME type
        // on the enter, which is what stops a drop from ever being delivered; the offer is kept
        // only so that it can be destroyed on the leave.
        static void onDragEnter(void* data, wl_data_device*, std::uint32_t serial, wl_surface*,
            wl_fixed_t x, wl_fixed_t y, wl_data_offer*);
        static void onDragLeave(void* data, wl_data_device*);
        static void onDragMotion(void* data, wl_data_device*, std::uint32_t time,
            wl_fixed_t x, wl_fixed_t y);
        static void onDragDrop(void* data, wl_data_device*);
        static void onControlOffered(void* data, ext_data_control_device_v1*,
            ext_data_control_offer_v1*);
        static void onControlOfferMimeType(void* data, ext_data_control_offer_v1*,
            const char* mimeType);
        static void onControlSelection(void* data, ext_data_control_device_v1*,
            ext_data_control_offer_v1*);
        // The compositor has withdrawn the device. Nothing arrives on it again, and the clipboard
        // falls back to what the seat announces - which is to say, back to the focus rule.
        static void onControlFinished(void* data, ext_data_control_device_v1*);
        // THE MIDDLE-CLICK SELECTION, which nothing in this framework reads. The offer is
        // destroyed as it arrives, and the slot is filled because a listener slot left null is a
        // crash rather than a refusal.
        static void onControlPrimarySelection(void* data, ext_data_control_device_v1*,
            ext_data_control_offer_v1*);
    private:
        inline static DisplayManager* s_standing{ nullptr };
        wl_display* m_display{ nullptr };
        wl_registry* m_registry{ nullptr };
        wl_compositor* m_compositor{ nullptr };
        wl_shm* m_shm{ nullptr };
        xdg_wm_base* m_windowManager{ nullptr };
        zxdg_decoration_manager_v1* m_decorationManager{ nullptr };
        zwlr_layer_shell_v1* m_layerShell{ nullptr };
        xdg_session_manager_v1* m_sessionManager{ nullptr };
        xdg_session_v1* m_session{ nullptr };
        std::string m_sessionId{};
        xdg_activation_v1* m_activation{ nullptr };
        std::string m_appId{ "clafi" };
        wl_seat* m_seat{ nullptr };
        wl_data_device_manager* m_dataDeviceManager{ nullptr };
        wl_data_device* m_dataDevice{ nullptr };
        // THE CLIPBOARD CHANNEL THAT DOES NOT ASK WHO HAS THE FOCUS. The manager is bound wherever
        // the compositor offers it; the device is null unless an application asked for one, and
        // while it stands it and not the seat's data device is what writes the selection down.
        ext_data_control_manager_v1* m_dataControlManager{ nullptr };
        ext_data_control_device_v1* m_dataControlDevice{ nullptr };
        wp_viewporter* m_viewporter{ nullptr };
        wp_fractional_scale_manager_v1* m_fractionalScaleManager{ nullptr };
        wp_cursor_shape_manager_v1* m_cursorShapeManager{ nullptr };
        wl_pointer* m_pointer{ nullptr };
        // The pointer's shape device. Made with the pointer and released with it.
        wp_cursor_shape_device_v1* m_cursorShapeDevice{ nullptr };
        // THE KEYBOARD PROXY AND THE KEYBOARD. The first is the protocol object the events arrive
        // on; the second is what reads them - the keymap, the state, the repeat - and answers in
        // the framework's terms. The keyboard outlives the proxy: a keyboard unplugged and plugged
        // back in is a new proxy and the same translator, told a new keymap.
        wl_keyboard* m_keyboardProxy{ nullptr };
        Keyboard m_keyboard{ [this](const KeyPress& press) {
            deliverKeyPress(press);
        } };
        std::vector<std::uint32_t> m_shmFormats;
        // EVERY OUTPUT, not the one this client is on - a client is not told that, it is told
        // which of them each of its surfaces overlaps.
        std::vector<OutputEntry> m_outputs;
        std::vector<SinkEntry> m_sinks;
        std::vector<PendingOffer> m_pendingOffers;
        // A LIST PER DEVICE. The two announce their offers the same way and neither may claim the
        // other's: a sweep of one device's unclaimed offers would free objects the other is about
        // to name.
        std::vector<PendingOffer> m_pendingControlOffers;
        // The clipboard's offer and the names on it, claimed out of a pending list by the
        // selection event that named it.
        SelectionOffer m_selectionOffer{};
        std::vector<std::string> m_selectionMimeTypes;
        SelectionHandler m_selectionHandler{};
        // A drag currently over one of this client's surfaces. Refused on arrival and destroyed
        // on the leave.
        wl_data_offer* m_dragOffer{ nullptr };
        // WHICH WINDOW THE POINTER IS OVER, and where in it. A motion event names neither: it
        // carries a position and nothing else, because the surface it belongs to was settled by
        // the enter that came before it and does not change until a leave.
        INativeEventSink* m_pointerFocus{ nullptr };
        IntPoint m_pointerPos{};
        // The serial of the enter that put the pointer over m_pointerFocus. A shape request names
        // it, and one naming any other enter is ignored.
        std::uint32_t m_pointerEnterSerial{ 0 };
        // The shape wanted, and whether the compositor has been told it since the last enter.
        std::uint32_t m_cursorShape{ WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT };
        bool m_cursorShapeSent{ false };
        // A move or a resize was handed to the compositor and no pointer event has come back yet.
        bool m_pointerWithCompositor{ false };
        // WHICH WINDOW HAS THE KEYBOARD. Named by the keyboard's own enter, which is a different
        // event from the pointer's and comes and goes on its own: a window under the pointer need
        // not be the one being typed into.
        INativeEventSink* m_keyboardFocus{ nullptr };
        bool m_quitRequested{ false };
        std::vector<Pollable> m_pollSources{};
        // The set handed to poll, kept so a wait allocates nothing once it has grown to size.
        std::vector<pollfd> m_pollFds{};
    };

    // A DESCRIPTOR'S PLACE IN THE LOOP, held for as long as this stands: registered on
    // construction and taken back on destruction, so a platform member declared after the
    // display and the manager it polls for is in the loop exactly while both are up. See Platform
    export class PollSource
    {
    public:
        PollSource(DisplayManager&, int fd, DisplayManager::Dispatch);
        ~PollSource();
        PollSource(const PollSource&) = delete;
        PollSource& operator=(const PollSource&) = delete;
    private:
        DisplayManager& m_display;
        const int m_fd;
    };

    // WHAT STANDS BEHIND IPlatformServices ON WAYLAND. The one platform of a Wayland build derives
    // from this, so a service handed the platform under its neutral name casts to it here and
    // reaches the display. See Platform
    export class IDisplayAccess : public IPlatformServices
    {
    public:
        [[nodiscard]] virtual DisplayManager& display() = 0;
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::PlatformImplementation::Wayland
{
    // The highest version of each global this client knows how to speak. Binding asks for the
    // lower of this and what the compositor offers: asking for more than is offered is a protocol
    // error, and binding less costs only the requests that were added later.
    namespace InterfaceVersion
    {
        // 6 is where wl_surface.preferred_buffer_scale arrives - the compositor naming the scale
        // this surface should draw at, rather than the client working it out from which outputs
        // it overlaps. A compositor offering less simply never sends it, and the scale stays 1.
        // 4 is where damage_buffer arrives, which takes buffer coordinates and so survives a
        // scale change; 3 is where set_buffer_scale does.
        constexpr std::uint32_t compositor = 6;
        constexpr std::uint32_t shm = 1;
        // 3 is where xdg_popup.reposition arrives - moving a popup that is already on screen,
        // which is what a menu re-placed around content that grew needs. A compositor offering
        // less places a popup once, where it was created; see Window::canReposition.
        constexpr std::uint32_t windowManager = 3;
        constexpr std::uint32_t decorationManager = 1;
        // 4 is where on_demand keyboard interactivity arrives, 3 the shell's own destructor, 2
        // set_layer on a surface already mapped. Nothing here needs more than 1; a compositor
        // offering less than 4 is bound at what it has.
        constexpr std::uint32_t layerShell = 4;
        // 1 is the whole protocol as it stands: a session asked for by id, a toplevel named in
        // it, and the three answers - created, restored, replaced.
        constexpr std::uint32_t sessionManager = 1;
        // 1 is the whole protocol: a token asked for against an input event, and the raise.
        constexpr std::uint32_t activation = 1;
        // 5 is where wl_pointer.frame arrives, which groups the events of one hardware movement
        // so a diagonal scroll is one gesture rather than two. 4 is where wl_keyboard.repeat_info
        // arrives, without which a held key would repeat at a rate this client made up.
        constexpr std::uint32_t seat = 5;
        // 1 carries every shape a form asks for - the arrow, the I-beam, the hand, the wait and
        // the eight resize arrows. 2 adds the drag-and-drop shapes, which nothing here shows.
        constexpr std::uint32_t cursorShapeManager = 1;
        // 2 is where wl_output.scale arrives. It is how a client learns the scale of a screen on
        // any compositor older than wl_compositor 6, which is most of them.
        constexpr std::uint32_t output = 2;
        constexpr std::uint32_t viewporter = 1;
        constexpr std::uint32_t fractionalScaleManager = 1;
        // 1 carries the whole of the selection: a source naming its MIME types, an offer listing
        // them, and set_selection against an input serial. 3 adds the drag-and-drop actions, whose
        // events would arrive on listener slots this layer does not fill, so it is not asked for
        // until something negotiates them.
        constexpr std::uint32_t dataDeviceManager = 1;
        // 1 is the whole protocol as it stands: a device announcing the selection and the primary
        // selection, and an offer to read either from.
        constexpr std::uint32_t dataControlManager = 1;
    }

    // ONE NOTCH OF A WHEEL, in the length units an axis event carries. The protocol states a
    // distance rather than a count, and every compositor in practice sends this for one detent.
    // The framework counts notches, so this is what turns one into the other.
    constexpr double k_axisStepPerNotch = 10.0;

    bool traceEnabled()
    {
        static const bool s_enabled = std::getenv("CLAFI_WAYLAND_TRACE") != nullptr;
        return s_enabled;
    }

    bool SelectionOffer::empty() const
    {
        return !seat && !control;
    }

    DisplayManager::DisplayManager()
    {
        m_display = ::wl_display_connect(nullptr);
        if (!m_display)
        {
            const char* wanted = std::getenv("WAYLAND_DISPLAY");
            throw std::runtime_error{ std::string{ "no Wayland compositor: WAYLAND_DISPLAY is " }
                + (wanted ? wanted : "unset") };
        }

        static const wl_registry_listener k_registryListener = {
            .global = &DisplayManager::onGlobal,
            .global_remove = &DisplayManager::onGlobalRemove
        };
        m_registry = ::wl_display_get_registry(m_display);
        ::wl_registry_add_listener(m_registry, &k_registryListener, this);

        // TWO ROUND TRIPS, AND BOTH ARE NEEDED. The first one brings the announcements and binds
        // what they name. The second waits for the events those bindings raise in turn - the shm
        // formats and the seat's capabilities above all, which arrive on objects this client only
        // has after the first.
        ::wl_display_roundtrip(m_display);
        ::wl_display_roundtrip(m_display);

        reportGlobals();
        s_standing = this;
    }

    DisplayManager::~DisplayManager()
    {
        s_standing = nullptr;
        // EVERY OFFER BEFORE THE DEVICE THEY ARRIVED ON, and the device before the manager that
        // made it, for the reason every destroy in this layer states: a child outliving its
        // parent is a protocol error.
        if (m_dragOffer)
            ::wl_data_offer_destroy(m_dragOffer);
        destroyOffer(m_selectionOffer);
        for (const PendingOffer& pending : m_pendingOffers)
            destroyOffer(pending.offer);
        for (const PendingOffer& pending : m_pendingControlOffers)
            destroyOffer(pending.offer);
        if (m_dataControlDevice)
            ::ext_data_control_device_v1_destroy(m_dataControlDevice);
        if (m_dataControlManager)
            ::ext_data_control_manager_v1_destroy(m_dataControlManager);
        if (m_dataDevice)
            ::wl_data_device_destroy(m_dataDevice);
        if (m_dataDeviceManager)
            ::wl_data_device_manager_destroy(m_dataDeviceManager);
        if (m_keyboardProxy)
            ::wl_keyboard_destroy(m_keyboardProxy);
        if (m_cursorShapeDevice)
            ::wp_cursor_shape_device_v1_destroy(m_cursorShapeDevice);
        if (m_pointer)
            ::wl_pointer_destroy(m_pointer);
        if (m_seat)
            ::wl_seat_destroy(m_seat);
        if (m_cursorShapeManager)
            ::wp_cursor_shape_manager_v1_destroy(m_cursorShapeManager);
        if (m_fractionalScaleManager)
            ::wp_fractional_scale_manager_v1_destroy(m_fractionalScaleManager);
        if (m_viewporter)
            ::wp_viewporter_destroy(m_viewporter);
        if (m_layerShell)
            ::zwlr_layer_shell_v1_destroy(m_layerShell);
        // DESTROYED, NOT REMOVED: the compositor keeps what it stored, which is the point.
        if (m_session)
            ::xdg_session_v1_destroy(m_session);
        if (m_sessionManager)
            ::xdg_session_manager_v1_destroy(m_sessionManager);
        if (m_activation)
            ::xdg_activation_v1_destroy(m_activation);
        if (m_decorationManager)
            ::zxdg_decoration_manager_v1_destroy(m_decorationManager);
        if (m_windowManager)
            ::xdg_wm_base_destroy(m_windowManager);
        if (m_shm)
            ::wl_shm_destroy(m_shm);
        if (m_compositor)
            ::wl_compositor_destroy(m_compositor);
        if (m_registry)
            ::wl_registry_destroy(m_registry);
        if (m_display)
            ::wl_display_disconnect(m_display);
    }

    void DisplayManager::receiveSelection(const SelectionOffer& offer, const char* mimeName,
        const int fd) const
    {
        if (offer.seat)
            ::wl_data_offer_receive(offer.seat, mimeName, fd);
        if (offer.control)
            ::ext_data_control_offer_v1_receive(offer.control, mimeName, fd);
    }

    // ONE ROUND TRIP, AND IT IS NOT OPTIONAL. The compositor states the standing selection as soon
    // as the device exists, and a window that opened without waiting for it would show an empty
    // clipboard until somebody copied something.
    void DisplayManager::createDataControlDevice()
    {
        if (m_dataControlDevice || !m_dataControlManager || !m_seat)
            return;

        static const ext_data_control_device_v1_listener k_dataControlListener = {
            .data_offer = &DisplayManager::onControlOffered,
            .selection = &DisplayManager::onControlSelection,
            .finished = &DisplayManager::onControlFinished,
            .primary_selection = &DisplayManager::onControlPrimarySelection
        };
        m_dataControlDevice = ::ext_data_control_manager_v1_get_data_device(m_dataControlManager,
            m_seat);
        ::ext_data_control_device_v1_add_listener(m_dataControlDevice, &k_dataControlListener,
            this);
        ::wl_display_roundtrip(m_display);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] data control device %p\n",
                static_cast<void*>(m_dataControlDevice));
    }

    // ONCE. A second ask with the same id would be this client replacing itself, which the
    // protocol names an error; an id the compositor does not know is treated as none, so the
    // config's id is handed over as it stands and the answer says which it was.
    void DisplayManager::openSession(std::string_view storedId)
    {
        if (m_session || !m_sessionManager)
            return;

        static const xdg_session_v1_listener k_sessionListener = {
            .created = &DisplayManager::onSessionCreated,
            .restored = &DisplayManager::onSessionRestored,
            .replaced = &DisplayManager::onSessionReplaced
        };
        m_sessionId = storedId;
        const char* askedId = m_sessionId.empty() ? nullptr : m_sessionId.c_str();
        m_session = ::xdg_session_manager_v1_get_session(m_sessionManager,
            XDG_SESSION_MANAGER_V1_REASON_LAUNCH, askedId);
        ::xdg_session_v1_add_listener(m_session, &k_sessionListener, this);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session asked id=\"%s\"\n", m_sessionId.c_str());
    }

    // ON A QUEUE OF ITS OWN. This is asked while a window is being hidden, and a plain roundtrip
    // dispatches everything else that has arrived - a configure, a close - into the middle of
    // that. The session's events are moved to a queue nobody else reads, waited for there and
    // moved back; the compositor answers get_session before it answers the wait, so one pass
    // carries created with it. Nothing to wait for where the id is the one that was asked for.
    void DisplayManager::awaitSessionId()
    {
        if (!m_session || !m_sessionId.empty())
            return;

        wl_event_queue* queue = ::wl_display_create_queue(m_display);
        wl_proxy* proxy = reinterpret_cast<wl_proxy*>(m_session);
        ::wl_proxy_set_queue(proxy, queue);
        ::wl_display_roundtrip_queue(m_display, queue);
        ::wl_proxy_set_queue(proxy, nullptr);
        ::wl_event_queue_destroy(queue);
    }

    void DisplayManager::activate(wl_surface* target, InputStamp stamp)
    {
        if (!m_activation || !m_seat || !target)
            return;

        // KWin grants a token to the surface of its active window only, and the keyboard is there.
        const auto focused = std::ranges::find(m_sinks, m_keyboardFocus, &SinkEntry::sink);
        wl_surface* asker = focused != m_sinks.end() ? focused->surface : nullptr;

        static const xdg_activation_token_v1_listener k_tokenListener = {
            .done = &DisplayManager::onActivationTokenDone
        };
        std::string token{};
        wl_event_queue* queue = ::wl_display_create_queue(m_display);
        xdg_activation_token_v1* tokenRequest =
            ::xdg_activation_v1_get_activation_token(m_activation);
        ::wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(tokenRequest), queue);
        ::xdg_activation_token_v1_add_listener(tokenRequest, &k_tokenListener, &token);
        ::xdg_activation_token_v1_set_serial(tokenRequest, stamp.serial, m_seat);
        if (asker)
            ::xdg_activation_token_v1_set_surface(tokenRequest, asker);
        ::xdg_activation_token_v1_commit(tokenRequest);
        ::wl_display_roundtrip_queue(m_display, queue);
        ::xdg_activation_token_v1_destroy(tokenRequest);
        ::wl_event_queue_destroy(queue);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] token   \"%s\" asker sink=%p\n", token.c_str(),
                static_cast<void*>(m_keyboardFocus));

        if (!token.empty())
            ::xdg_activation_v1_activate(m_activation, token.c_str(), target);
    }

    int DisplayManager::scaleForOutput(wl_output* value) const
    {
        for (const OutputEntry& entry : m_outputs)
            if (entry.output == value)
                return entry.scale;

        return 1;
    }

    int DisplayManager::tallestOutputHeight() const
    {
        int result{};
        for (const OutputEntry& entry : m_outputs)
            result = std::max(result, entry.modeSize.y);

        return result;
    }

    void DisplayManager::setCursor(CursorShape value)
    {
        std::uint32_t shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
        switch (value)
        {
        case CursorShape::IBeam:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
            break;
        case CursorShape::Hand:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER;
            break;
        case CursorShape::Wait:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT;
            break;
        case CursorShape::Arrow:
            break;
        }
        setCursorShape(shape);
    }

    void DisplayManager::setResizeCursor(std::uint32_t edges)
    {
        std::uint32_t shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
        switch (edges)
        {
        case XDG_TOPLEVEL_RESIZE_EDGE_TOP:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_LEFT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_RIGHT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NW_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE;
            break;
        case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT:
            shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE;
            break;
        default:
            break;
        }
        setCursorShape(shape);
    }

    // Stated only when it changes, and again when the compositor has forgotten it - see
    // m_cursorShapeSent.
    void DisplayManager::setCursorShape(std::uint32_t shape)
    {
        if (shape == m_cursorShape && m_cursorShapeSent)
            return;

        m_cursorShape = shape;
        sendCursorShape();
    }

    void DisplayManager::pointerHandedToCompositor()
    {
        m_pointerWithCompositor = true;
    }

    void DisplayManager::registerSink(INativeEventSink& sink, wl_surface* surface)
    {
        m_sinks.push_back({ .sink = &sink, .surface = surface });
    }

    void DisplayManager::unregisterSink(INativeEventSink& sink)
    {
        // THE POINTER AND THE KEYBOARD GO WITH THE WINDOW THEY WERE ON. A leave does not always
        // arrive for a surface the client destroyed itself, so a stale focus here would send the
        // next motion or the next key to freed memory - and a repeat in flight is for a key the
        // destroyed window was holding.
        if (m_pointerFocus == &sink)
            m_pointerFocus = nullptr;
        if (m_keyboardFocus == &sink)
        {
            m_keyboardFocus = nullptr;
            m_keyboard.focusLost();
        }

        std::erase_if(m_sinks, [&sink](const SinkEntry& entry) {
            return entry.sink == &sink;
        });
        if (m_sinks.empty())
            m_quitRequested = true;
    }

    void DisplayManager::dispatchPending()
    {
        if (!m_display)
            return;

        ::wl_display_dispatch_pending(m_display);

        // COPIED BEFORE THE WALK. A sink told the queue is empty may close its window from
        // inside the call, which unregisters it, and the walk would be standing in the vector
        // that just moved.
        const std::vector<SinkEntry> sinks = m_sinks;
        for (const SinkEntry& entry : sinks)
            entry.sink->onIdle();
    }

    void DisplayManager::waitForEvent()
    {
        if (!m_display)
            return;

        // THE READ IS ANNOUNCED BEFORE THE SLEEP. Between deciding the queue is empty and going
        // to sleep, libwayland has to know a read is coming, or an event arriving in that gap is
        // queued by another reader and this one sleeps through it. prepare_read fails while this
        // thread still has events queued, which is what the loop is for.
        while (::wl_display_prepare_read(m_display) != 0)
            ::wl_display_dispatch_pending(m_display);

        // FLUSHED BEFORE SLEEPING, because requests are buffered client-side: a commit that is
        // still in the buffer is a frame the compositor has not been asked for, and waiting for
        // its answer would wait forever.
        ::wl_display_flush(m_display);

        // THE CONNECTION FIRST, THE REGISTERED SOURCES IN THE ORDER THEY WERE ADDED, and the
        // appearance bus last: -1 until the desktop's mode is first asked for, and poll passes
        // over a negative fd.
        std::vector<pollfd>& fds = m_pollFds;
        fds.assign(m_pollSources.size() + 2, pollfd{});
        fds[0].fd = ::wl_display_get_fd(m_display);
        fds[0].events = POLLIN;
        for (std::size_t i = 0; i < m_pollSources.size(); ++i)
        {
            fds[i + 1].fd = m_pollSources[i].fd;
            fds[i + 1].events = POLLIN;
        }
        pollfd& appearance = fds.back();
        appearance.fd = Linux::appearanceFd();
        appearance.events = POLLIN;

        const int count = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), -1);
        if (count < 0)
        {
            // A signal cutting the wait short is not a failure; the next pass waits again.
            if (errno != EINTR)
                Linux::reportErrno();
            ::wl_display_cancel_read(m_display);
            return;
        }

        // THE ANNOUNCED READ IS ALWAYS ANSWERED, one way or the other. Leaving it outstanding
        // holds every other reader of this connection asleep behind it.
        if (fds[0].revents & POLLIN)
            ::wl_display_read_events(m_display);
        else
            ::wl_display_cancel_read(m_display);

        // INPUT IS SERVICED BEFORE THE SOURCES, deliberately. An animation timer with a zero
        // interval is ready on every pass, so a loop that took timers first would never reach
        // the pointer.
        for (std::size_t i = 0; i < m_pollSources.size(); ++i)
        {
            if (fds[i + 1].revents & POLLIN)
                m_pollSources[i].dispatch();
        }
        // ANY EVENT, NOT ONLY INPUT: a bus that hangs up is reported whatever was asked for, and
        // left unread it would end every wait at once.
        if (appearance.revents != 0)
            Linux::dispatchAppearance();
    }

    void DisplayManager::addPollSource(const int fd, Dispatch dispatch)
    {
        m_pollSources.push_back({ fd, std::move(dispatch) });
    }

    void DisplayManager::removePollSource(const int fd)
    {
        std::erase_if(m_pollSources, [fd](const Pollable& source) {
            return source.fd == fd;
        });
    }

    void DisplayManager::reportGlobals() const
    {
        if (!traceEnabled())
            return;

        std::fprintf(stderr,
            "[wayland] display=%p compositor=%p shm=%p xdg_wm_base=%p seat=%p decoration=%p\n",
            static_cast<const void*>(m_display), static_cast<const void*>(m_compositor),
            static_cast<const void*>(m_shm), static_cast<const void*>(m_windowManager),
            static_cast<const void*>(m_seat), static_cast<const void*>(m_decorationManager));
        // THE VERSIONS ACTUALLY BOUND, which is the lower of what this client asks for and what
        // the compositor has. It is what decides whether an event can arrive at all.
        if (m_compositor)
            std::fprintf(stderr, "[wayland] version compositor=%u shm=%u xdg_wm_base=%u seat=%u\n",
                ::wl_compositor_get_version(m_compositor),
                m_shm ? ::wl_shm_get_version(m_shm) : 0,
                m_windowManager ? ::xdg_wm_base_get_version(m_windowManager) : 0,
                m_seat ? ::wl_seat_get_version(m_seat) : 0);
        std::fprintf(stderr,
            "[wayland] outputs %zu  viewporter=%p fractional=%p cursor-shape=%p layer-shell=%p\n",
            m_outputs.size(), static_cast<const void*>(m_viewporter),
            static_cast<const void*>(m_fractionalScaleManager),
            static_cast<const void*>(m_cursorShapeManager),
            static_cast<const void*>(m_layerShell));
        std::fprintf(stderr,
            "[wayland] data-control manager=%p  session manager=%p  activation=%p  app id=%s\n",
            static_cast<const void*>(m_dataControlManager),
            static_cast<const void*>(m_sessionManager), static_cast<const void*>(m_activation),
            m_appId.c_str());

        for (std::uint32_t format : m_shmFormats)
            std::fprintf(stderr, "[wayland] shm format 0x%08x%s\n", format,
                format == WL_SHM_FORMAT_ARGB8888 ? "  ARGB8888"
                : format == WL_SHM_FORMAT_XRGB8888 ? "  XRGB8888" : "");
    }

    // The raw pointers, not the accessors: fractionalScaleManager answers null without the
    // viewporter, and this states what the compositor offered.
    DisplayManager::OptionalGlobals DisplayManager::optionalGlobals() const
    {
        return OptionalGlobals{ {
            { ::ext_data_control_manager_v1_interface.name, m_dataControlManager != nullptr },
            { ::wp_viewporter_interface.name, m_viewporter != nullptr },
            { ::wp_fractional_scale_manager_v1_interface.name,
                m_fractionalScaleManager != nullptr },
            { ::wp_cursor_shape_manager_v1_interface.name, m_cursorShapeManager != nullptr },
            { ::zxdg_decoration_manager_v1_interface.name, m_decorationManager != nullptr },
            { ::zwlr_layer_shell_v1_interface.name, m_layerShell != nullptr },
            { ::xdg_session_manager_v1_interface.name, m_sessionManager != nullptr },
            { ::xdg_activation_v1_interface.name, m_activation != nullptr }
        } };
    }

    void DisplayManager::bindGlobal(wl_registry* registry, std::uint32_t name,
        std::string_view interfaceName, std::uint32_t version)
    {
        if (interfaceName == ::wl_compositor_interface.name)
        {
            m_compositor = static_cast<wl_compositor*>(::wl_registry_bind(registry, name,
                &::wl_compositor_interface, std::min(version, InterfaceVersion::compositor)));
            return;
        }
        if (interfaceName == ::wl_shm_interface.name)
        {
            static const wl_shm_listener k_shmListener = {
                .format = &DisplayManager::onShmFormat
            };
            m_shm = static_cast<wl_shm*>(::wl_registry_bind(registry, name,
                &::wl_shm_interface, std::min(version, InterfaceVersion::shm)));
            ::wl_shm_add_listener(m_shm, &k_shmListener, this);
            return;
        }
        if (interfaceName == ::xdg_wm_base_interface.name)
        {
            static const xdg_wm_base_listener k_windowManagerListener = {
                .ping = &DisplayManager::onPing
            };
            m_windowManager = static_cast<xdg_wm_base*>(::wl_registry_bind(registry, name,
                &::xdg_wm_base_interface, std::min(version, InterfaceVersion::windowManager)));
            ::xdg_wm_base_add_listener(m_windowManager, &k_windowManagerListener, this);
            return;
        }
        if (interfaceName == ::wl_seat_interface.name)
        {
            static const wl_seat_listener k_seatListener = {
                .capabilities = &DisplayManager::onSeatCapabilities,
                .name = &DisplayManager::onSeatName
            };
            m_seat = static_cast<wl_seat*>(::wl_registry_bind(registry, name,
                &::wl_seat_interface, std::min(version, InterfaceVersion::seat)));
            ::wl_seat_add_listener(m_seat, &k_seatListener, this);
            // The globals arrive in the order the compositor lists them, and the data device
            // manager may be bound already.
            createDataDevice();
            return;
        }
        if (interfaceName == ::wl_data_device_manager_interface.name)
        {
            m_dataDeviceManager = static_cast<wl_data_device_manager*>(
                ::wl_registry_bind(registry, name, &::wl_data_device_manager_interface,
                    std::min(version, InterfaceVersion::dataDeviceManager)));
            createDataDevice();
            return;
        }
        // BOUND WHEREVER IT IS OFFERED AND USED ONLY WHERE IT IS ASKED FOR. Binding a global says
        // nothing to the compositor beyond that this client knows the interface. It is the device
        // made on it that starts a selection arriving, and that waits for an application to ask.
        if (interfaceName == ::ext_data_control_manager_v1_interface.name)
        {
            m_dataControlManager = static_cast<ext_data_control_manager_v1*>(
                ::wl_registry_bind(registry, name, &::ext_data_control_manager_v1_interface,
                    std::min(version, InterfaceVersion::dataControlManager)));
            return;
        }
        if (interfaceName == ::wp_viewporter_interface.name)
        {
            m_viewporter = static_cast<wp_viewporter*>(::wl_registry_bind(registry, name,
                &::wp_viewporter_interface, std::min(version, InterfaceVersion::viewporter)));
            return;
        }
        if (interfaceName == ::wp_fractional_scale_manager_v1_interface.name)
        {
            m_fractionalScaleManager = static_cast<wp_fractional_scale_manager_v1*>(
                ::wl_registry_bind(registry, name, &::wp_fractional_scale_manager_v1_interface,
                    std::min(version, InterfaceVersion::fractionalScaleManager)));
            return;
        }
        if (interfaceName == ::wp_cursor_shape_manager_v1_interface.name)
        {
            m_cursorShapeManager = static_cast<wp_cursor_shape_manager_v1*>(
                ::wl_registry_bind(registry, name, &::wp_cursor_shape_manager_v1_interface,
                    std::min(version, InterfaceVersion::cursorShapeManager)));
            // The globals arrive in the order the compositor lists them, and the seat may have
            // named its pointer already.
            createCursorShapeDevice();
            return;
        }
        if (interfaceName == ::wl_output_interface.name)
        {
            static const wl_output_listener k_outputListener = {
                .geometry = &DisplayManager::onOutputGeometry,
                .mode = &DisplayManager::onOutputMode,
                .done = &DisplayManager::onOutputDone,
                .scale = &DisplayManager::onOutputScale,
                .name = nullptr,
                .description = nullptr
            };
            // EVERY ONE OF THEM IS BOUND. A machine with three screens announces this global
            // three times, and a surface can be told it is on any of them.
            wl_output* output = static_cast<wl_output*>(::wl_registry_bind(registry, name,
                &::wl_output_interface, std::min(version, InterfaceVersion::output)));
            m_outputs.push_back({ .output = output, .scale = 1, .modeSize = {} });
            ::wl_output_add_listener(output, &k_outputListener, this);
            return;
        }
        if (interfaceName == ::zxdg_decoration_manager_v1_interface.name)
        {
            m_decorationManager = static_cast<zxdg_decoration_manager_v1*>(
                ::wl_registry_bind(registry, name, &::zxdg_decoration_manager_v1_interface,
                    std::min(version, InterfaceVersion::decorationManager)));
            return;
        }
        if (interfaceName == ::zwlr_layer_shell_v1_interface.name)
        {
            m_layerShell = static_cast<zwlr_layer_shell_v1*>(
                ::wl_registry_bind(registry, name, &::zwlr_layer_shell_v1_interface,
                    std::min(version, InterfaceVersion::layerShell)));
            return;
        }
        if (interfaceName == ::xdg_session_manager_v1_interface.name)
        {
            m_sessionManager = static_cast<xdg_session_manager_v1*>(
                ::wl_registry_bind(registry, name, &::xdg_session_manager_v1_interface,
                    std::min(version, InterfaceVersion::sessionManager)));
            return;
        }
        if (interfaceName == ::xdg_activation_v1_interface.name)
        {
            m_activation = static_cast<xdg_activation_v1*>(
                ::wl_registry_bind(registry, name, &::xdg_activation_v1_interface,
                    std::min(version, InterfaceVersion::activation)));
            return;
        }
    }

    INativeEventSink* DisplayManager::sinkForSurface(wl_surface* value) const
    {
        for (const SinkEntry& entry : m_sinks)
            if (entry.surface == value)
                return entry.sink;

        return nullptr;
    }

    void DisplayManager::sendCursorShape()
    {
        if (!m_cursorShapeDevice || !m_pointerFocus)
            return;

        ::wp_cursor_shape_device_v1_set_shape(m_cursorShapeDevice, m_pointerEnterSerial,
            m_cursorShape);
        m_cursorShapeSent = true;

        // FLUSHED HERE, not left to the loop. The loop flushes before it sleeps, and a shape
        // set on the way into a long operation - the wait cursor is exactly that - would sit in
        // the buffer until the operation was over and the cursor no longer wanted. A shape
        // changes at a control's edge and not on every motion, so this is a rare write.
        ::wl_display_flush(m_display);
    }

    void DisplayManager::restateCursorShape()
    {
        if (!m_cursorShapeSent)
            sendCursorShape();
    }

    void DisplayManager::pointerReturned()
    {
        if (!m_pointerWithCompositor)
            return;

        m_pointerWithCompositor = false;
        m_cursorShapeSent = false;
    }

    void DisplayManager::createCursorShapeDevice()
    {
        if (m_cursorShapeDevice || !m_cursorShapeManager || !m_pointer)
            return;

        m_cursorShapeDevice = ::wp_cursor_shape_manager_v1_get_pointer(m_cursorShapeManager,
            m_pointer);
    }

    void DisplayManager::createDataDevice()
    {
        if (m_dataDevice || !m_dataDeviceManager || !m_seat)
            return;

        static const wl_data_device_listener k_dataDeviceListener = {
            .data_offer = &DisplayManager::onDataOffered,
            .enter = &DisplayManager::onDragEnter,
            .leave = &DisplayManager::onDragLeave,
            .motion = &DisplayManager::onDragMotion,
            .drop = &DisplayManager::onDragDrop,
            .selection = &DisplayManager::onSelection
        };
        m_dataDevice = ::wl_data_device_manager_get_data_device(m_dataDeviceManager, m_seat);
        ::wl_data_device_add_listener(m_dataDevice, &k_dataDeviceListener, this);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] data device %p\n", static_cast<void*>(m_dataDevice));
    }

    void DisplayManager::destroyOffer(const SelectionOffer& offer)
    {
        if (offer.seat)
            ::wl_data_offer_destroy(offer.seat);
        if (offer.control)
            ::ext_data_control_offer_v1_destroy(offer.control);
    }

    void DisplayManager::deliverKeyPress(const KeyPress& press)
    {
        INativeEventSink* sink = m_keyboardFocus;
        if (!sink)
            return;

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] key     0x%02x%s char=U+%04x%s%s%s\n",
                press.key, press.isRepeat ? " repeat" : "", static_cast<unsigned>(press.character),
                press.modifiers.shift ? " shift" : "", press.modifiers.ctrl ? " ctrl" : "",
                press.modifiers.alt ? " alt" : "");

        sink->onKeyPressed(press);

        // THE CHARACTER ONLY IF THE PRESS LEFT THE WINDOW STANDING. A window closing unregisters
        // its sink, and unregistering clears this focus - so a focus still naming the sink is
        // the one proof that it is still there to be given a character.
        if (press.character && m_keyboardFocus == sink)
            sink->onCharacter(press.character);
    }

    void DisplayManager::onGlobal(void* data, wl_registry* registry, std::uint32_t name,
        const char* interfaceName, std::uint32_t version)
    {
        static_cast<DisplayManager*>(data)->bindGlobal(registry, name, interfaceName, version);
    }

    // A global going away is not a fault. Nothing this layer binds is one a compositor withdraws
    // while a client is running, so there is nothing to undo.
    void DisplayManager::onGlobalRemove(void*, wl_registry*, std::uint32_t)
    {
    }

    // The compositor asks whether the client is still answering. One that does not reply is
    // declared unresponsive - the window greys and the desktop offers to kill it - so the absence
    // of this looks like a hang rather than like a missing handler.
    void DisplayManager::onPing(void*, xdg_wm_base* windowManager, std::uint32_t serial)
    {
        ::xdg_wm_base_pong(windowManager, serial);
    }

    void DisplayManager::onSessionCreated(void* data, xdg_session_v1*, const char* sessionId)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_sessionId = sessionId;
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session created id=\"%s\"\n", sessionId);
    }

    void DisplayManager::onSessionRestored(void* data, xdg_session_v1*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session restored id=\"%s\"\n",
                manager.m_sessionId.c_str());
    }

    // ANOTHER INSTANCE OF THIS APPLICATION TOOK THE SESSION. Everything made from it is inert
    // from here on and is destroyed; the toplevel sessions go with their windows.
    void DisplayManager::onSessionReplaced(void* data, xdg_session_v1*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session replaced\n");
        ::xdg_session_v1_destroy(manager.m_session);
        manager.m_session = nullptr;
    }

    void DisplayManager::onActivationTokenDone(void* data, xdg_activation_token_v1*,
        const char* token)
    {
        *static_cast<std::string*>(data) = token;
    }

    void DisplayManager::onShmFormat(void* data, wl_shm*, std::uint32_t format)
    {
        static_cast<DisplayManager*>(data)->m_shmFormats.push_back(format);
    }

    // WHAT DEVICES THIS SEAT HAS, and it is told again whenever that changes - a mouse unplugged
    // mid-session takes the pointer away and says so here.
    void DisplayManager::onSeatCapabilities(void* data, wl_seat* seat, std::uint32_t capabilities)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        const bool hasPointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;

        if (hasPointer && !manager.m_pointer)
        {
            static const wl_pointer_listener k_pointerListener = {
                .enter = &DisplayManager::onPointerEnter,
                .leave = &DisplayManager::onPointerLeave,
                .motion = &DisplayManager::onPointerMotion,
                .button = &DisplayManager::onPointerButton,
                .axis = &DisplayManager::onPointerAxis,
                .frame = &DisplayManager::onPointerFrame,
                .axis_source = &DisplayManager::onPointerAxisSource,
                .axis_stop = &DisplayManager::onPointerAxisStop,
                .axis_discrete = &DisplayManager::onPointerAxisDiscrete
            };
            manager.m_pointer = ::wl_seat_get_pointer(seat);
            ::wl_pointer_add_listener(manager.m_pointer, &k_pointerListener, data);
            manager.createCursorShapeDevice();
        }
        else if (!hasPointer && manager.m_pointer)
        {
            // The device before the pointer it was made for, for the reason every destroy in
            // this layer states: a child outliving its parent is a protocol error.
            if (manager.m_cursorShapeDevice)
                ::wp_cursor_shape_device_v1_destroy(manager.m_cursorShapeDevice);
            manager.m_cursorShapeDevice = nullptr;
            manager.m_cursorShapeSent = false;
            ::wl_pointer_release(manager.m_pointer);
            manager.m_pointer = nullptr;
            manager.m_pointerFocus = nullptr;
        }

        const bool hasKeyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;
        if (hasKeyboard && !manager.m_keyboardProxy)
        {
            // Every slot named, as with the pointer: wl_seat 5 delivers all six, and a null slot
            // for an event that arrives ends the process.
            static const wl_keyboard_listener k_keyboardListener = {
                .keymap = &DisplayManager::onKeyboardKeymap,
                .enter = &DisplayManager::onKeyboardEnter,
                .leave = &DisplayManager::onKeyboardLeave,
                .key = &DisplayManager::onKeyboardKey,
                .modifiers = &DisplayManager::onKeyboardModifiers,
                .repeat_info = &DisplayManager::onKeyboardRepeatInfo
            };
            manager.m_keyboardProxy = ::wl_seat_get_keyboard(seat);
            ::wl_keyboard_add_listener(manager.m_keyboardProxy, &k_keyboardListener, data);
        }
        else if (!hasKeyboard && manager.m_keyboardProxy)
        {
            ::wl_keyboard_release(manager.m_keyboardProxy);
            manager.m_keyboardProxy = nullptr;
            manager.m_keyboardFocus = nullptr;
            manager.m_keyboard.focusLost();
        }

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] seat    pointer=%d keyboard=%d touch=%d\n",
                hasPointer ? 1 : 0, hasKeyboard ? 1 : 0,
                (capabilities & WL_SEAT_CAPABILITY_TOUCH) ? 1 : 0);
    }

    void DisplayManager::onSeatName(void*, wl_seat*, const char*)
    {
    }

    // THE POINTER IS OVER THIS SURFACE FROM NOW UNTIL A LEAVE. Everything after this names no
    // surface at all, so the one recorded here is what a motion or a press is about.
    void DisplayManager::onPointerEnter(void* data, wl_pointer*, std::uint32_t serial,
        wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_pointerFocus = manager.sinkForSurface(surface);
        manager.m_pointerEnterSerial = serial;
        manager.m_pointerPos = {
            static_cast<int>(::wl_fixed_to_double(x)),
            static_cast<int>(::wl_fixed_to_double(y))
        };

        // THE IMAGE IS UNDEFINED ON ENTER, the protocol says, until the client states one. The
        // move below has the form name the shape for the control under the pointer, and the
        // remembered shape is stated for a move that named none - a pointer coming back to the
        // point it left from, which the form reads as no movement at all.
        manager.m_cursorShapeSent = false;
        manager.m_pointerWithCompositor = false;

        // AN ENTER IS A MOVE as far as anything above here is concerned: the pointer is over
        // this window at this point, which is the whole of what a move says too.
        if (manager.m_pointerFocus)
            manager.m_pointerFocus->onPointerMoved(manager.m_pointerPos);

        manager.restateCursorShape();
    }

    void DisplayManager::onPointerLeave(void* data, wl_pointer*, std::uint32_t, wl_surface*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (manager.m_pointerFocus)
            manager.m_pointerFocus->onPointerLeft();

        manager.m_pointerFocus = nullptr;
    }

    // NO SERIAL ON A MOTION, and that is the protocol rather than an omission: a compositor will
    // not authorise a request on the strength of the pointer merely being somewhere. Only an
    // event the user CAUSED - a press, a key - carries a number worth naming.
    void DisplayManager::onPointerMotion(void* data, wl_pointer*, std::uint32_t,
        wl_fixed_t x, wl_fixed_t y)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_pointerPos = {
            static_cast<int>(::wl_fixed_to_double(x)),
            static_cast<int>(::wl_fixed_to_double(y))
        };
        manager.pointerReturned();

        if (manager.m_pointerFocus)
            manager.m_pointerFocus->onPointerMoved(manager.m_pointerPos);

        manager.restateCursorShape();
    }

    void DisplayManager::onPointerButton(void* data, wl_pointer*, std::uint32_t serial, std::uint32_t time,
        std::uint32_t button, std::uint32_t state)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (!manager.m_pointerFocus)
            return;

        manager.pointerReturned();
        const bool down = state == WL_POINTER_BUTTON_STATE_PRESSED;

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] button  %s at %d,%d  serial=%u  sink=%p\n",
                down ? "down" : "up  ", manager.m_pointerPos.x, manager.m_pointerPos.y, serial,
                static_cast<void*>(manager.m_pointerFocus));

        // THE SERIAL IS THE NUMBER A WINDOW DRAG OR A MENU IS AUTHORISED BY. It reaches the
        // framework on the press and is carried by whatever that press starts - see InputStamp.
        switch (button)
        {
        case BTN_LEFT:
            manager.m_pointerFocus->onPointerButton(manager.m_pointerPos, time, { serial }, down);
            break;

        case BTN_RIGHT:
            // A right press is a context menu and nothing else here: the framework asks for the
            // menu on the press rather than counting a release it would have to match up.
            if (down)
                manager.m_pointerFocus->onPointerContextMenu(manager.m_pointerPos, { serial });
            break;

        default:
            break;
        }

        // The press may have closed the window it landed on. The focus is cleared with the sink,
        // and the restatement finds nothing to state to.
        manager.restateCursorShape();
    }

    void DisplayManager::onPointerAxis(void* data, wl_pointer*, std::uint32_t, std::uint32_t axis,
        wl_fixed_t value)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (!manager.m_pointerFocus)
            return;

        // NEGATED. An axis event states how far the CONTENT moved, and the framework counts how
        // far the WHEEL turned, which is the other way round.
        const double steps = ::wl_fixed_to_double(value) / k_axisStepPerNotch;
        manager.m_pointerFocus->onPointerWheel(manager.m_pointerPos,
            static_cast<float>(-steps), axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL);
    }

    // Where the events of one hardware movement are grouped. Nothing accumulates across them yet,
    // so there is nothing to flush - a diagonal scroll arrives as two events and acts as two.
    void DisplayManager::onPointerFrame(void*, wl_pointer*)
    {
    }

    // THE REST OF WHAT A WHEEL DELIVERS: where the scroll came from, where it stopped, and how many
    // detents it turned. None of the three is read - the notch count comes from the axis value
    // above - and each is stated all the same, because libwayland invokes the listener slot for
    // every event the bound version can deliver and calls wl_abort on a null one, which ends the
    // process:
    //
    //   listener function for opcode 6 of wl_pointer is NULL
    //
    // A wl_pointer takes its seat's version, and wl_seat 5 carries all three. A slot may be left
    // null only where the bound version cannot reach the event - wl_output.name and .description
    // above arrive at wl_output 4, and this binds 2.
    void DisplayManager::onPointerAxisSource(void*, wl_pointer*, std::uint32_t)
    {
    }

    void DisplayManager::onPointerAxisStop(void*, wl_pointer*, std::uint32_t, std::uint32_t)
    {
    }

    void DisplayManager::onPointerAxisDiscrete(void*, wl_pointer*, std::uint32_t, std::int32_t)
    {
    }

    // THE KEYMAP, AS A FILE. It arrives once when the keyboard is bound and again whenever the
    // user changes layouts, and the whole of the keyboard's knowledge is in it - which key is
    // which, what each types, which repeat.
    void DisplayManager::onKeyboardKeymap(void* data, wl_keyboard*, std::uint32_t format,
        std::int32_t fd, std::uint32_t size)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_keyboard.setKeymap(format, fd, size);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] keymap  format=%u size=%u\n", format, size);
    }

    // THE KEYBOARD IS ON THIS SURFACE FROM NOW UNTIL A LEAVE. The keys already held arrive with
    // it and are not delivered: Win32 does not replay them either, and a shortcut fired by a key
    // that was pressed before the window existed would be a shortcut nobody asked for.
    void DisplayManager::onKeyboardEnter(void* data, wl_keyboard*, std::uint32_t, wl_surface* surface,
        wl_array*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_keyboardFocus = manager.sinkForSurface(surface);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] keyboard enter sink=%p\n",
                static_cast<void*>(manager.m_keyboardFocus));
    }

    // THE RELEASE OF A HELD KEY GOES ELSEWHERE from here on, so the repeat is stopped rather than
    // waited on. The sink is told nothing: focus is a state the toplevel's configure carries and
    // the window reads from there.
    void DisplayManager::onKeyboardLeave(void* data, wl_keyboard*, std::uint32_t, wl_surface*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_keyboard.focusLost();
        manager.m_keyboardFocus = nullptr;
    }

    void DisplayManager::onKeyboardKey(void* data, wl_keyboard*, std::uint32_t serial, std::uint32_t,
        std::uint32_t key, std::uint32_t state)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (!manager.m_keyboardFocus)
            return;

        // THE SERIAL IS THE NUMBER A MENU OPENED FROM THE KEYBOARD IS AUTHORISED BY, as the
        // press's serial is for one opened from the pointer. It goes with the press and with
        // every repeat of it - see Keyboard.
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
            manager.m_keyboard.keyDown(key, { serial });
        else
        {
            manager.m_keyboard.keyUp(key);
            manager.m_keyboardFocus->onKeyReleased();
        }
    }

    void DisplayManager::onKeyboardModifiers(void* data, wl_keyboard*, std::uint32_t,
        std::uint32_t depressed, std::uint32_t latched, std::uint32_t locked, std::uint32_t group)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        manager.m_keyboard.setModifiers(depressed, latched, locked, group);
    }

    void DisplayManager::onKeyboardRepeatInfo(void* data, wl_keyboard*, std::int32_t rate,
        std::int32_t delay)
    {
        static_cast<DisplayManager*>(data)->m_keyboard.setRepeatInfo(rate, delay);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] repeat  rate=%d/s delay=%dms\n", rate, delay);
    }

    void DisplayManager::onOutputGeometry(void*, wl_output*, std::int32_t, std::int32_t, std::int32_t, std::int32_t,
        std::int32_t, const char*, const char*, std::int32_t)
    {
    }

    // Every mode the output has is announced; the one it is in carries the current flag.
    void DisplayManager::onOutputMode(void* data, wl_output* output, std::uint32_t flags,
        std::int32_t width, std::int32_t height, std::int32_t)
    {
        if (!(flags & WL_OUTPUT_MODE_CURRENT))
            return;
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        for (OutputEntry& entry : manager.m_outputs)
            if (entry.output == output)
            {
                entry.modeSize = { width, height };
                if (traceEnabled())
                    std::fprintf(stderr, "[wayland] output  %p mode=%dx%d\n",
                        static_cast<void*>(output), width, height);
                return;
            }
    }

    // The end of one output's run of properties. Nothing is accumulated across them here, so
    // there is nothing waiting to be applied.
    void DisplayManager::onOutputDone(void*, wl_output*)
    {
    }

    void DisplayManager::onOutputScale(void* data, wl_output* output, std::int32_t factor)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        for (OutputEntry& entry : manager.m_outputs)
            if (entry.output == output)
            {
                entry.scale = factor > 0 ? factor : 1;
                if (traceEnabled())
                    std::fprintf(stderr, "[wayland] output  %p scale=%d\n",
                        static_cast<void*>(output), entry.scale);
                return;
            }
    }

    void DisplayManager::onDataOfferMimeType(void* data, wl_data_offer* offer, const char* mimeType)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        for (PendingOffer& pending : manager.m_pendingOffers)
        {
            if (pending.offer.seat != offer)
                continue;

            pending.mimeTypes.emplace_back(mimeType);
            return;
        }
    }

    // A NEW OFFER OBJECT, described by the events that follow on it. What it is for is not said
    // here - a selection and a drag are announced the same way - so it waits in the pending list
    // until one of them claims it.
    void DisplayManager::onDataOffered(void* data, wl_data_device*, wl_data_offer* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);

        static const wl_data_offer_listener k_dataOfferListener = {
            .offer = &DisplayManager::onDataOfferMimeType
        };
        ::wl_data_offer_add_listener(offer, &k_dataOfferListener, data);
        manager.m_pendingOffers.push_back({ .offer = { .seat = offer }, .mimeTypes = {} });
    }

    // WHAT IS ON THE CLIPBOARD NOW, or nothing at all when the offer is null. The previous offer
    // goes with it: an offer is valid only until it is replaced, and keeping one would be reading
    // a selection that no longer exists.
    //
    // Every other pending offer is dropped here too. One that was never claimed has nothing left
    // to claim it while no drag is accepted, and a client is what destroys the offers it does not
    // use. A drag this client took would have to be held back from this sweep.
    void DisplayManager::onSelection(void* data, wl_data_device*, wl_data_offer* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);

        // A CONTROL DEVICE REPLACES THIS EVENT RATHER THAN JOINING IT. Two channels writing one
        // selection down would disagree the moment the focus moved, so while that device stands
        // these offers are only something to dispose of - the drag events are what this device is
        // still here for.
        if (manager.m_dataControlDevice)
        {
            for (const PendingOffer& pending : manager.m_pendingOffers)
                destroyOffer(pending.offer);
            manager.m_pendingOffers.clear();
            return;
        }

        destroyOffer(manager.m_selectionOffer);
        manager.m_selectionOffer = {};
        manager.m_selectionMimeTypes.clear();

        for (const PendingOffer& pending : manager.m_pendingOffers)
        {
            if (pending.offer.seat != offer)
            {
                destroyOffer(pending.offer);
                continue;
            }

            manager.m_selectionOffer = pending.offer;
            manager.m_selectionMimeTypes = pending.mimeTypes;
        }

        manager.m_pendingOffers.clear();

        // AFTER the state above and never before. A handler asks what the clipboard holds now,
        // and what it holds now is what the lines above have just finished writing down.
        if (manager.m_selectionHandler)
            manager.m_selectionHandler();
    }

    void DisplayManager::onDragEnter(void* data, wl_data_device*, std::uint32_t serial, wl_surface*,
        wl_fixed_t, wl_fixed_t, wl_data_offer* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (!offer)
            return;

        // NO MIME TYPE ACCEPTED, which is how the protocol spells a refusal. The source is told
        // the drag cannot be dropped here, and no drop event follows.
        ::wl_data_offer_accept(offer, serial, nullptr);
        manager.m_dragOffer = offer;
        std::erase_if(manager.m_pendingOffers, [offer](const PendingOffer& pending){
            return pending.offer.seat == offer;
        });
    }

    void DisplayManager::onDragLeave(void* data, wl_data_device*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (manager.m_dragOffer)
            ::wl_data_offer_destroy(manager.m_dragOffer);

        manager.m_dragOffer = nullptr;
    }

    void DisplayManager::onDragMotion(void*, wl_data_device*, std::uint32_t, wl_fixed_t, wl_fixed_t)
    {
    }

    // Unreachable while the enter accepts no MIME type, and defined because a listener slot left
    // null is a crash rather than a refusal.
    void DisplayManager::onDragDrop(void*, wl_data_device*)
    {
    }

    // THE SAME SHAPE AS onDataOffered ON A DEVICE THAT CARRIES NO DRAG. What the offer is for is
    // said by the event that claims it, and here that is the selection or the primary selection.
    void DisplayManager::onControlOffered(void* data, ext_data_control_device_v1*,
        ext_data_control_offer_v1* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);

        static const ext_data_control_offer_v1_listener k_controlOfferListener = {
            .offer = &DisplayManager::onControlOfferMimeType
        };
        ::ext_data_control_offer_v1_add_listener(offer, &k_controlOfferListener, data);
        manager.m_pendingControlOffers.push_back({
            .offer = { .control = offer },
            .mimeTypes = {}
        });
    }

    void DisplayManager::onControlOfferMimeType(void* data, ext_data_control_offer_v1* offer,
        const char* mimeType)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        for (PendingOffer& pending : manager.m_pendingControlOffers)
        {
            if (pending.offer.control != offer)
                continue;

            pending.mimeTypes.emplace_back(mimeType);
            return;
        }
    }

    // WHAT IS ON THE CLIPBOARD NOW, SAID WITHOUT ASKING WHO HAS THE FOCUS. That is the whole
    // reason this device exists. The previous offer goes with the event, as on the seat's twin:
    // an offer is valid only until it is replaced.
    //
    // NOTHING IS SWEPT HERE, which the seat's twin does. Every offer this device announces is
    // claimed by the event that follows it, so a pending one is only ever in flight between the
    // two, and freeing an unclaimed offer would free an object the compositor is about to name.
    void DisplayManager::onControlSelection(void* data, ext_data_control_device_v1*,
        ext_data_control_offer_v1* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);

        destroyOffer(manager.m_selectionOffer);
        manager.m_selectionOffer = {};
        manager.m_selectionMimeTypes.clear();

        for (const PendingOffer& pending : manager.m_pendingControlOffers)
        {
            if (pending.offer.control != offer)
                continue;

            manager.m_selectionOffer = pending.offer;
            manager.m_selectionMimeTypes = pending.mimeTypes;
            break;
        }

        std::erase_if(manager.m_pendingControlOffers, [offer](const PendingOffer& pending){
            return pending.offer.control == offer;
        });

        // AFTER the state above and never before. A handler asks what the clipboard holds now,
        // and what it holds now is what the lines above have just finished writing down.
        if (manager.m_selectionHandler)
            manager.m_selectionHandler();
    }

    // The clipboard does not stop working here. What is left is the seat's data device, which
    // answers only while this client holds the keyboard focus - so the window goes back to being
    // told late rather than being told nothing.
    void DisplayManager::onControlFinished(void* data, ext_data_control_device_v1*)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);

        destroyOffer(manager.m_selectionOffer);
        manager.m_selectionOffer = {};
        manager.m_selectionMimeTypes.clear();
        for (const PendingOffer& pending : manager.m_pendingControlOffers)
            destroyOffer(pending.offer);
        manager.m_pendingControlOffers.clear();

        ::ext_data_control_device_v1_destroy(manager.m_dataControlDevice);
        manager.m_dataControlDevice = nullptr;

        if (manager.m_selectionHandler)
            manager.m_selectionHandler();
    }

    void DisplayManager::onControlPrimarySelection(void* data, ext_data_control_device_v1*,
        ext_data_control_offer_v1* offer)
    {
        DisplayManager& manager = *static_cast<DisplayManager*>(data);
        if (!offer)
            return;

        ::ext_data_control_offer_v1_destroy(offer);
        std::erase_if(manager.m_pendingControlOffers, [offer](const PendingOffer& pending){
            return pending.offer.control == offer;
        });
    }

    // PollSource

    PollSource::PollSource(DisplayManager& display, const int fd, DisplayManager::Dispatch dispatch)
        :
        m_display{ display },
        m_fd{ fd }
    {
        m_display.addPollSource(m_fd, std::move(dispatch));
    }

    PollSource::~PollSource()
    {
        m_display.removePollSource(m_fd);
    }
}
