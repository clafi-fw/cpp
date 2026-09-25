#pragma once

// The Wayland client library and the protocol bindings generated from wayland-protocols XML.
// Included from the global module fragment of every module in this layer: CMake has no support
// for header units, so a C header reaches a module this way or not at all.
//
// Angle brackets on the generated headers deliberately - they come from the build directory
// that wayland-scanner wrote them into, not from beside this file.

#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <cursor-shape-v1-client-protocol.h>
// THE CLIPBOARD WITHOUT THE KEYBOARD FOCUS. A selection on wl_data_device is announced to the
// focused client and to nobody else; this one is announced to whoever made a device, which is what
// a viewer of the clipboard needs and what an ordinary window has no business asking for.
#include <ext-data-control-v1-client-protocol.h>
// WHERE A WINDOW WAS IS THE COMPOSITOR'S TO REMEMBER. A client cannot ask where its window is or
// put it anywhere; it names the window to a session the compositor keeps, and is put back.
#include <xdg-session-management-v1-client-protocol.h>
// RAISING A WINDOW IS THE COMPOSITOR'S. The client asks for a token against the input event that
// wanted the raise, and hands it back naming the window.
#include <xdg-activation-v1-client-protocol.h>
// THE LAYER SHELL NAMES AN ARGUMENT "namespace", which C accepts and C++ cannot, and the
// generated header is C. It reads as namespace_ for the length of the include; nothing else
// in that header spells the word outside a comment.
#define namespace namespace_
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
#undef namespace

// What a wl_keyboard says is a keymap and key numbers; xkbcommon is what reads a keymap and turns
// a number into a key and a character. The compose header is the dead keys.
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

// The kernel's button and key numbers. A wl_pointer.button or wl_keyboard.key event carries one
// of these rather than an index of its own, because the compositor passes the device's code
// through untouched.
#include <linux/input-event-codes.h>

#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
