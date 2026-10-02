#!/bin/sh
# Answers whether this machine can run a ClaFi application, and names what is missing.
#
#   clafi-check.sh [path/to/executable]
#
# With an executable named, the dynamic linker's own answer is used for the libraries; without
# one, the sonames are looked for on the library path. Exit code 0 = will run, 1 = will not,
# 2 = will run with something reduced.
#
# POSIX sh deliberately: this runs on a machine that may have no bash.

req_fail=0
opt_fail=0
missing_libs=''

say()  { printf '%s\n' "$*"; }
ok()   { printf '  ok    %s\n' "$*"; }
warn() { printf '  less  %s\n' "$*"; opt_fail=1; }
bad()  { printf '  STOP  %s\n' "$*"; req_fail=1; }
note() { printf '        %s\n' "$*"; }

want() { missing_libs="$missing_libs $1"; }

version_at_least() {
    have_major=${1%%.*}
    have_minor=${1#*.}
    have_minor=${have_minor%%.*}
    need_major=${2%%.*}
    need_minor=${2#*.}
    if [ "$have_major" -ne "$need_major" ]; then
        [ "$have_major" -gt "$need_major" ]
        return
    fi
    [ "$have_minor" -ge "$need_minor" ]
}

# ---------------------------------------------------------------- processor

say 'Processor'

arch=$(uname -m)
case "$arch" in
    x86_64|amd64) ok "architecture $arch" ;;
    *) bad "architecture $arch - ClaFi is built for x86-64 only (-mavx2 -mfma, no NEON path)" ;;
esac

if [ -r /proc/cpuinfo ]; then
    flags=$(sed -n 's/^flags[[:space:]]*: //p; s/^Features[[:space:]]*: //p' /proc/cpuinfo | head -n 1)
    cpu_missing=''
    for f in avx2 fma; do
        case " $flags " in
            *" $f "*) ok "$f" ;;
            *)
                bad "$f absent - the build carries -m$f on every translation unit, so it stops at the first $f instruction"
                cpu_missing="$cpu_missing $f"
                ;;
        esac
    done
    case " $flags " in
        *" hypervisor "*)
            if [ -n "$cpu_missing" ]; then
                note 'this is a virtual machine - the host may have what the hypervisor does not pass through'
            fi
            case "$cpu_missing" in
                *fma*) note 'VirtualBox hides FMA from every guest before version 7.1' ;;
            esac
            ;;
    esac
else
    warn 'no /proc/cpuinfo - AVX2 and FMA unchecked'
fi

# ---------------------------------------------------------------- C library

say ''
say 'C library'

# The newest GLIBC_ version the release binaries name - BuildReleases.sh prints it per binary.
glibc_floor=2.35

glibc=''
if command -v getconf >/dev/null 2>&1; then
    glibc=$(getconf GNU_LIBC_VERSION 2>/dev/null | sed -n 's/^glibc //p')
fi
if [ -z "$glibc" ] && command -v ldd >/dev/null 2>&1; then
    glibc=$(ldd --version 2>&1 | head -n 1 | grep -Ei 'glibc|gnu libc' | sed 's/.* //')
fi

# glibc is asked first - Debian's musl package puts the musl loader on glibc systems too.
if [ -n "$glibc" ]; then
    if version_at_least "$glibc" "$glibc_floor"; then
        ok "glibc $glibc"
    else
        bad "glibc $glibc - the release needs $glibc_floor or newer"
    fi
elif [ -e /lib/ld-musl-x86_64.so.1 ] || [ -e /lib/ld-musl-aarch64.so.1 ]; then
    bad 'musl - the release is a glibc build and will not start here'
else
    warn 'C library not identified'
fi

# ---------------------------------------------------------------- display server

say ''
say 'Display server'

wayland_ok=0
if [ -n "${WAYLAND_DISPLAY:-}" ]; then
    sock="${WAYLAND_DISPLAY}"
    case "$sock" in
        /*) : ;;
        *) sock="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/$sock" ;;
    esac
    if [ -S "$sock" ]; then
        ok "Wayland display $WAYLAND_DISPLAY"
        wayland_ok=1
    else
        bad "WAYLAND_DISPLAY is $WAYLAND_DISPLAY but $sock is not a socket"
    fi
elif [ -n "${DISPLAY:-}" ]; then
    bad 'X11 session - unsupported. A ClaFi application needs a Wayland session.'
    note 'a nested compositor can host one inside X11; that is untested and not supported'
else
    bad 'no display server - neither WAYLAND_DISPLAY nor DISPLAY is set'
fi

say ''
say 'Compositor protocols'

if [ "$wayland_ok" -eq 0 ]; then
    note 'skipped - there is no Wayland session to ask'
elif command -v wayland-info >/dev/null 2>&1; then
    globals=$(wayland-info 2>/dev/null || true)
    check_global() {
        # $1 interface, $2 what is lost without it, $3 required|optional
        if printf '%s' "$globals" | grep -q "interface: '$1'"; then
            ok "$1"
        elif [ "$3" = required ]; then
            bad "$1 absent - $2"
        else
            warn "$1 absent - $2"
        fi
    }
    check_global xdg_wm_base                     'no window can be created at all'        required
    check_global wl_shm                          'no buffer can be shared - ClaFi draws on the CPU' required
    check_global wp_viewporter                   'fractional scaling falls back to whole-number scale' optional
    check_global wp_fractional_scale_manager_v1  'a 125% or 175% screen is drawn at 100% or 200% and stretched' optional
    check_global wp_cursor_shape_manager_v1      'the compositor default arrow stands in for every cursor' optional
    check_global zxdg_decoration_manager_v1      'the compositor may add a title bar above the one ClaFi draws' optional
    check_global ext_data_control_manager_v1     'the clipboard is readable only while the window has the keyboard' optional
    check_global zwlr_layer_shell_v1             'a screen-edge window is placed as an ordinary window'  optional
    check_global xdg_session_manager_v1          'window placement is not remembered across runs'        optional
else
    note 'wayland-info is not installed - unchecked (package: wayland-utils)'
fi

# ---------------------------------------------------------------- libraries

say ''
say 'Libraries'

find_lib() {
    if command -v ldconfig >/dev/null 2>&1 && ldconfig -p 2>/dev/null | grep -q "[[:space:]]$1 "; then
        return 0
    fi
    for d in /lib /lib64 /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/local/lib; do
        [ -e "$d/$1" ] && return 0
    done
    return 1
}

# soname                what it is
check_lib() {
    if find_lib "$1"; then
        ok "$1  ($2)"
    else
        bad "$1 absent  ($2)"
        want "$1"
    fi
}

if [ -n "${1:-}" ]; then
    if [ ! -x "$1" ]; then
        bad "$1 is not an executable file"
    elif command -v ldd >/dev/null 2>&1; then
        unresolved=$(ldd "$1" 2>/dev/null | grep 'not found' || true)
        if [ -n "$unresolved" ]; then
            oldifs=$IFS
            IFS='
'
            for line in $unresolved; do
                lib=$(printf '%s\n' "$line" | sed 's/^[[:space:]]*//; s/[[:space:]].*//')
                bad "$lib absent"
                want "$lib"
            done
            IFS=$oldifs
            note 'the linker itself reports these - names above are what the build was linked against'
        else
            ok "$1 resolves every library it names"
        fi
    fi
else
    # libunibreak and libc++ are linked into the release binaries.
    check_lib libwayland-client.so.0 'the Wayland client library'
    check_lib libxkbcommon.so.0      'keyboard layouts'
    check_lib libfontconfig.so.1     'which file a family name means'
    check_lib libfreetype.so.6       'glyph rasterizing'
    check_lib libharfbuzz.so.0       'text shaping'
    check_lib libpng16.so.16         'PNG decoding'
    check_lib libz.so.1              'compression'
    check_lib libdbus-1.so.3         'the session bus'
fi

# ---------------------------------------------------------------- fonts

say ''
say 'Fonts'

if command -v fc-match >/dev/null 2>&1; then
    for family in sans-serif monospace; do
        answer=$(fc-match "$family" 2>/dev/null | head -n 1)
        if [ -n "$answer" ]; then
            ok "$family -> $answer"
        else
            bad "fontconfig answers nothing for $family - install a font"
        fi
    done
else
    warn 'fc-match is not installed - fonts unchecked (package: fontconfig)'
    note 'ClaFi asks fontconfig for sans-serif and monospace; a machine with no fonts draws nothing'
fi

# ---------------------------------------------------------------- verdict

say ''
if [ "$req_fail" -ne 0 ]; then
    say 'Verdict: a ClaFi application will not run here.'
elif [ "$opt_fail" -ne 0 ]; then
    say 'Verdict: a ClaFi application will run here, with the reductions marked "less" above.'
else
    say 'Verdict: a ClaFi application will run here.'
fi

# ---------------------------------------------------------------- what to install

# dnf and zypper take a soname as it is; apt and pacman need the package that holds it.
# Debian names are candidates, newest first - Debian 13 and Ubuntu 24.04 renamed libpng's package.
debian_package() {
    case "$1" in
        libwayland-client.so.0) echo libwayland-client0 ;;
        libxkbcommon.so.0)      echo libxkbcommon0 ;;
        libfontconfig.so.1)     echo libfontconfig1 ;;
        libfreetype.so.6)       echo libfreetype6 ;;
        libharfbuzz.so.0)       echo libharfbuzz0b ;;
        libpng16.so.16)         echo libpng16-16t64 libpng16-16 ;;
        libz.so.1)              echo zlib1g ;;
        libdbus-1.so.3)         echo libdbus-1-3 ;;
    esac
}

arch_package() {
    case "$1" in
        libwayland-client.so.0) echo wayland ;;
        libxkbcommon.so.0)      echo libxkbcommon ;;
        libfontconfig.so.1)     echo fontconfig ;;
        libfreetype.so.6)       echo freetype2 ;;
        libharfbuzz.so.0)       echo harfbuzz ;;
        libpng16.so.16)         echo libpng ;;
        libz.so.1)              echo zlib ;;
        libdbus-1.so.3)         echo dbus ;;
    esac
}

# The first candidate this machine's package lists know, else the oldest name.
debian_pick() {
    last=''
    for candidate in $(debian_package "$1"); do
        if apt-cache show "$candidate" >/dev/null 2>&1; then
            printf '%s' "$candidate"
            return
        fi
        last=$candidate
    done
    printf '%s' "$last"
}

if [ -n "$missing_libs" ]; then
    os_release=/etc/os-release
    [ -r "$os_release" ] || os_release=/usr/lib/os-release
    os_ids=''
    if [ -r "$os_release" ]; then
        os_ids=$(. "$os_release"; printf '%s %s' "${ID:-}" "${ID_LIKE:-}")
    fi

    manager=''
    packages=''
    unnamed=''
    case " $os_ids " in
        *" debian "*|*" ubuntu "*)
            manager='sudo apt install'
            for lib in $missing_libs; do
                package=$(debian_pick "$lib")
                if [ -n "$package" ]; then
                    packages="$packages $package"
                else
                    unnamed="$unnamed $lib"
                fi
            done
            ;;
        *" arch "*)
            manager='sudo pacman -S'
            for lib in $missing_libs; do
                package=$(arch_package "$lib")
                if [ -n "$package" ]; then
                    packages="$packages $package"
                else
                    unnamed="$unnamed $lib"
                fi
            done
            ;;
        *" fedora "*|*" rhel "*|*" centos "*)
            manager='sudo dnf install'
            for lib in $missing_libs; do
                packages="$packages '$lib()(64bit)'"
            done
            ;;
        *" suse "*|*" opensuse "*)
            manager='sudo zypper install'
            for lib in $missing_libs; do
                packages="$packages '$lib()(64bit)'"
            done
            ;;
    esac

    say ''
    if [ -n "$manager" ]; then
        say 'To install what is missing:'
        if [ -n "$packages" ]; then
            note "$manager$packages"
        fi
        if [ -n "$unnamed" ]; then
            note "and the packages that hold:$unnamed"
        fi
    else
        say "Missing:$missing_libs"
        say 'Install the package that holds each - its name depends on the distribution.'
    fi
fi

[ "$req_fail" -ne 0 ] && exit 1
[ "$opt_fail" -ne 0 ] && exit 2
exit 0
