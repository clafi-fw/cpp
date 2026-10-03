# ClaFi on Linux: supported distributions

A ClaFi application is a native Wayland client. It asks of a machine: glibc 2.35 or newer, a Wayland
session, an x86-64 processor with AVX2 and FMA, and eight shared libraries every desktop already
carries - libwayland-client, libxkbcommon, fontconfig, FreeType, HarfBuzz, libpng, zlib and
libdbus-1. Everything else it needs is linked in. Checking for updates uses the system's libcurl
where one is installed.

| Distribution (desktop) | Sharp at 125% | Placement kept | Clipboard unfocused | Edge-anchored windows | Level |
|---|---|---|---|---|---|
| Fedora 43 and above (KDE) | Yes | Yes, Plasma 6.7+ | No, awaiting KWin | Yes | **Verified** |
| openSUSE Leap 16 and above, Tumbleweed (KDE) | Yes | Yes, Plasma 6.7+ | No, awaiting KWin | Yes | Expected |
| Arch, EndeavourOS, SteamOS 3.8 and above (KDE) | Yes | Yes, Plasma 6.7+ | No, awaiting KWin | Yes | Expected |
| Fedora 43 and above (Workstation) | Yes | No | No | No | Expected |
| Ubuntu 22.04 LTS and above (GNOME) | Yes | No | No | No | Expected |
| Debian 12 and above (GNOME) | Yes | No | No | No | Expected |
| RHEL, AlmaLinux, Rocky 10 and above (GNOME) | Yes | No | No | No | Expected |

The desktop decides all four feature columns, not the distribution: a KDE spin of any GNOME row
answers as the KDE rows do, and the reverse. Cursor shapes, text rendering and every other
behaviour are the same everywhere. A No costs that one behaviour and never a launch.

**Sharp at 125%** - a screen at a fractional scale is drawn at its real ratio rather than at a
whole number and stretched, through fractional-scale-v1 and viewporter.

**Placement kept** - a window is put back where it last stood, through xdg-session-management-v1.
A compositor without it places the window by its own rule.

**Clipboard unfocused** - the clipboard is readable while another window holds the keyboard,
through ext-data-control-v1. KWin implements the older wlr-data-control, which ClaFi does not
bind, and the port is an open merge request - so all three KDE rows turn to Yes when it lands.

**Edge-anchored windows** - a dialog placed `ScreenRight` reserves a strip at the screen edge,
through wlr-layer-shell. Without it such a window is an ordinary one.

**Verified** means built and launched there. **Expected** means every measured requirement is met
and the application has not been started there yet.

## Out of scope

- **RHEL 9, AlmaLinux 9, Rocky 9** - glibc 2.34, one version under the floor. Reachable by
  building the release image on an EL9 base if that population ever matters.
- **Alpine, Chimera and other musl distributions** - a musl build of their own.
- **Desktops started on X11** - Cinnamon, Xfce, MATE and Budgie by default. Where a Wayland
  session exists it can be chosen at login; there is no X11 backend.
- **ARM** - the build carries AVX2 and FMA on every translation unit, so x86-64 only.

## Checking a machine

`clafi-check.sh <executable>` answers whether a given machine can run a ClaFi application and names
anything missing - processor features, the C library, the Wayland session, each compositor protocol
with what is lost without it, the shared libraries, and whether fontconfig can answer `sans-serif`
and `monospace`. For a missing library it prints the install command for the distribution it runs
on.

A virtual machine shows its guest only the processor features the hypervisor passes through,
whatever the host has - VirtualBox hides FMA from every guest before version 7.1.

2026-09-17.
