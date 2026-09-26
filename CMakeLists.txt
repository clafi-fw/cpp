# ClaFi under clang + libc++, for the Linux port.
#
# CLAFI_PLATFORM=none builds Core, Controls and everything above them as a static library and
# nothing else. The platform layer is excluded, so Platform:: and createNativeTextLayout() have no
# definitions. Compiling is the whole point; a link would only report that absence, which is
# already known.
#
#   cmake -S . -B build -G Ninja \
#         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
#   cmake --build build -- -k 0
#
# CLAFI_PLATFORM=wayland builds the OS layer and the Wayland layer with it, and on top of them
# the two showcases and the Themes app. It needs libwayland-dev and wayland-protocols installed -
# the second ships no library, only the XML the bindings are generated from - xkbcommon for the
# keyboard, the font stack's development packages: freetype2, harfbuzz, fontconfig and
# libunibreak, and dbus-1 for the desktop's settings. Each is named per distribution where it is
# found.
#
#   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
#         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
#
# Ninja is required: the Visual Studio and Makefile generators do not scan C++20 module
# dependencies. -stdlib=libc++ is required: clang defaults to libstdc++, which ships no `std`
# module for `import std;` to find. Build into a fresh directory after changing an import - a
# stale BMI keeps a name reachable that the source no longer asks for, and hides the error.

cmake_minimum_required(VERSION 3.30)

# `import std` is behind an experimental gate, and the token that opens it is recycled whenever
# the feature's BEHAVIOUR changes rather than on a schedule - 4.0.3 through 4.2 shared one value,
# 4.3 took its own, 4.4 took another. A token that does not match the CMake in use leaves the gate
# shut, and what is reported then is a missing `std` module, which does not read as a version
# mismatch at all.
#
# A LOOKUP RATHER THAN ONE VALUE, because the two machines this tree builds on run different
# releases: Fedora 44 ships CMake 4.3.0 and WSL runs 4.4. Both configure from this file.
#
# cmake_minimum_required above states the POLICY floor and stays where it is. 4.3 is the floor
# this gate imposes, and it is stated here rather than by raising that, which would change policy
# defaults as a side effect.
#
# The token for a release is in that release's own tree, at
# https://gitlab.kitware.com/cmake/cmake/-/raw/v<version>/Help/dev/experimental.rst
if(CMAKE_VERSION VERSION_LESS 4.3)
    message(FATAL_ERROR
            "ClaFi states no `import std` token for CMake ${CMAKE_VERSION}. Install 4.3 or "
            "newer, or add this release's token from Help/dev/experimental.rst below.")
elseif(CMAKE_VERSION VERSION_LESS 4.4)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
endif()

# SAID BEFORE THE BUILD FAILS, rather than left to a diagnostic that names `std`. 4.4 is the
# newest release with a token here, and a newer one is given 4.4's on the chance the token was
# carried rather than recycled - which is what happened from 4.0.3 through 4.2.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.5)
    message(WARNING
            "ClaFi: CMake ${CMAKE_VERSION} is newer than any release this file states an "
            "`import std` token for, so 4.4's is in use. If the build reports no `std` module, "
            "read the token for this release out of Help/dev/experimental.rst and add it above.")
endif()

project(ClaFi CXX)

# STATED, BECAUSE NOTHING STATES IT OTHERWISE. A single-config generator with no build type passes
# no -O flag at all and no NDEBUG, and the compile line looks complete enough to be mistaken for a
# real build - the tree is a software rasterizer, and one measured that way reads about ten times
# slower than the same code under MSVC. -mavx2 and -mfma below buy nothing without this: there is
# no vectorising and no inlining with the optimiser off.
#
# RelWithDebInfo while chasing a crash - it is -O2 with symbols, against Release's -O3 with none.
# A -DCMAKE_BUILD_TYPE on the command line is already in the cache when this runs and stands.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "ClaFi: build type ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# WHAT AN EDITOR READS THIS TREE THROUGH. Visual Studio's own C++ IntelliSense does not
# understand C++20 modules, so it paints the source red whatever state it is in. ReSharper C++
# parses the tree itself; clangd needs this file, which lands in the build directory.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_MODULE_STD ON)

# HERE RATHER THAN ON THE TARGET, because the synthesized `std` module has to carry it too. CMake
# builds that module from CMAKE_CXX_FLAGS alone - a target's own options never reach it - and clang
# refuses a BMI whose thread support differs from the consumer's. freetype2, harfbuzz and fontconfig
# each state -pthread in their pkg-config Cflags, so every file importing the text layer has thread
# support switched on, and the `std` module they all import has to agree.
string(APPEND CMAKE_CXX_FLAGS " -pthread")

# WHAT DISPLAY SERVER THE PLATFORM LAYER IS BUILT AGAINST. A string rather than a switch, because
# a second display server is the thing this is for: two booleans would have four states of which
# two are nonsense, and a name reads at the command line as the thing being chosen.
set(CLAFI_PLATFORM "none" CACHE STRING "Display-server backend: none or wayland")
set_property(CACHE CLAFI_PLATFORM PROPERTY STRINGS none wayland)
if(NOT CLAFI_PLATFORM MATCHES "^(none|wayland)$")
    message(FATAL_ERROR "CLAFI_PLATFORM must be none or wayland, not '${CLAFI_PLATFORM}'")
endif()
if(DEFINED CLAFI_LINUX_PLATFORM)
    message(FATAL_ERROR "CLAFI_LINUX_PLATFORM named the X11 layer. "
                        "Use -DCLAFI_PLATFORM=wayland, or none.")
endif()

# Sources are collected rather than listed. ClaFi.vcxitems names 250 files by hand and is
# rewritten by Visual Studio; a second hand-kept list would be a second thing to forget. The
# cost is that a new file needs a re-configure, which CONFIGURE_DEPENDS asks Ninja to do.
file(GLOB_RECURSE clafi_modules CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cppm")
file(GLOB_RECURSE clafi_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

# Windows-only, parked, generated, or scratch. No trailing slash after Windows: it has to catch
# both Platform/Windows/ and Platform/Windows_Platform.cppm beside it.
#
# PROGRAMS RATHER THAN LIBRARY CODE live outside Source/ - Showcase/ holds one subsystem
# demonstrated thoroughly (MillScene, TextEngine), Tools/ the applications used to do work (Themes
# App, Dom Benchmark) - so the glob never sees them.
set(clafi_excluded "/Platform/Windows")
string(APPEND clafi_excluded "|/_to_delete/|/build|/out/")
if(CLAFI_PLATFORM STREQUAL "none")
    # TWO FOLDERS ON TWO DIFFERENT AXES, switched together because no build wants one without the
    # other. Platform/Linux/ is the OPERATING SYSTEM - eventfd timers, inotify directory watching,
    # the font stack, the desktop's settings over the session bus - and nothing in it knows what a
    # display server is. Platform/Wayland/ is the DISPLAY SERVER. They are siblings rather than
    # alternatives.
    #
    # No trailing slash after Wayland, for the same reason as Windows: it has to catch both
    # Platform/Wayland/ and Platform/Wayland_Facade.cppm beside it.
    string(APPEND clafi_excluded "|/Platform/Linux/|/Platform/Wayland")
endif()

list(FILTER clafi_modules EXCLUDE REGEX "${clafi_excluded}")
list(FILTER clafi_sources EXCLUDE REGEX "${clafi_excluded}")

# Platform/ itself holds nothing but a facade per platform. What a backend needs is the backend's,
# down to the arithmetic: two files that looked shared turned out to have one caller each, and a
# thing with one caller belongs beside it.

list(LENGTH clafi_modules module_count)
list(LENGTH clafi_sources source_count)
message(STATUS "ClaFi: ${module_count} module interface units, ${source_count} implementation units")

add_library(ClaFi STATIC ${clafi_sources})
# PUBLIC, not PRIVATE: a module in a PRIVATE file set is visible only inside this target, and an
# application importing ClaFi.App.Application is told the module is not accessible. The header
# equivalent would be target_include_directories.
target_sources(ClaFi PUBLIC FILE_SET CXX_MODULES FILES ${clafi_modules})

# ClaFi.vcxitems adds Source/Controls as an include path, and the `#include
# "../Y-Core/System/EventBindings.h"` in Controls/ resolves through it rather than relative to
# the including file - which is why the same spelling works from Controls/ and from
# Controls/Base/. Reproduced here rather than rewritten, so both builds read one spelling.
target_include_directories(ClaFi PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source/Controls")

# Every target that imports these modules is compiled with this list, not the library alone. A
# module interface's inline code is compiled in whoever imports it, and clang refuses to load a BMI
# whose target features differ from the consumer's - CMake's synthesized `std` module included.
set(clafi_compile_options
    # Eight files carry AVX2, and Cpu_Rasterizer uses FMA on top of it. Applied to the whole
    # target rather than per file: this build exists to find out whether the tree compiles, and
    # a per-file dispatch table is a decision for whoever answers the ARM question.
    -mavx2 -mfma
    # #pragma warning(push/disable/pop) and #pragma region carry no meaning here.
    -Wno-unknown-pragmas
)
target_compile_options(ClaFi PRIVATE ${clafi_compile_options})

if(CLAFI_PLATFORM STREQUAL "wayland")
    # A Wayland protocol is XML, and wayland-scanner turns it into a C header of inline request
    # senders plus a C file of interface tables. Neither is written by hand and neither is checked
    # in: the XML that ships with wayland-protocols is the source, and generating from it is what
    # keeps the bindings agreeing with the libwayland actually installed.
    enable_language(C)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)

    # Two pkg-config packages that ship no library at all: wayland-scanner names the generator,
    # wayland-protocols names the directory holding the XML. Debian/Ubuntu libwayland-dev and
    # wayland-protocols; Fedora wayland-devel and wayland-protocols-devel.
    pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)
    if(NOT WAYLAND_SCANNER_BIN)
        # An older wayland-scanner.pc does not publish the binary's own path.
        find_program(WAYLAND_SCANNER_BIN NAMES wayland-scanner)
    endif()
    if(NOT WAYLAND_SCANNER_BIN)
        message(FATAL_ERROR "wayland-scanner not found. Install libwayland-dev / wayland-devel.")
    endif()
    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    if(NOT WAYLAND_PROTOCOLS_DIR)
        message(FATAL_ERROR
            "wayland-protocols not found. Install wayland-protocols / wayland-protocols-devel.")
    endif()

    set(wl_gen "${CMAKE_CURRENT_BINARY_DIR}/wayland-generated")
    file(MAKE_DIRECTORY "${wl_gen}")
    set(wl_protocol_headers "")
    set(wl_protocol_sources "")

    # A macro rather than a function, so list(APPEND) reaches the caller without a PARENT_SCOPE
    # dance. Every protocol needs the same two commands and the list only grows - a seat milestone
    # adds nothing but more XML.
    #
    # private-code, not the deprecated code: the generated file then carries the interface tables
    # itself rather than expecting them from a shared library, which is what lets a static library
    # hold them.
    macro(clafi_wayland_protocol xml_path basename)
        add_custom_command(
            OUTPUT "${wl_gen}/${basename}-client-protocol.h"
            COMMAND "${WAYLAND_SCANNER_BIN}" client-header
                    "${xml_path}" "${wl_gen}/${basename}-client-protocol.h"
            DEPENDS "${xml_path}" VERBATIM)
        add_custom_command(
            OUTPUT "${wl_gen}/${basename}-protocol.c"
            COMMAND "${WAYLAND_SCANNER_BIN}" private-code
                    "${xml_path}" "${wl_gen}/${basename}-protocol.c"
            DEPENDS "${xml_path}" VERBATIM)
        list(APPEND wl_protocol_headers "${wl_gen}/${basename}-client-protocol.h")
        list(APPEND wl_protocol_sources "${wl_gen}/${basename}-protocol.c")
    endmacro()

    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml" xdg-shell)
    # Not a nicety. A framework that draws its own frame has to say so, or KWin puts a second
    # title bar above the one already painted and "is this window ours" stops having an answer.
    # Mutter does not advertise the global at all, and binding it there is a no-op.
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
        xdg-decoration-unstable-v1)
    # A SCREEN AT 175% IS NOT AN INTEGER. wl_output states a whole-number scale and nothing else,
    # so without these two a window on such a screen is drawn at 1x or 2x and stretched to fit -
    # which is the blurry text every toolkit exists to avoid. fractional-scale states the real
    # ratio; viewporter is how a buffer drawn at that ratio is presented at the size the
    # compositor asked for. Neither is useful without the other.
    if(NOT EXISTS "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml")
        message(FATAL_ERROR
            "wayland-protocols is too old for fractional-scale-v1 (needs 1.31 or newer).")
    endif()
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/stable/viewporter/viewporter.xml" viewporter)
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml"
        fractional-scale-v1)
    # THE POINTER IMAGE, BY NAME. The compositor draws the I-beam and the resize arrows from its
    # own theme at its own size, and a client that drew them itself would be the one window on
    # the desk with the wrong cursor. The protocol names a tablet tool as one thing a shape can
    # be set on, so its interface table has to be linked too - which is the only reason tablet-v2
    # is here. It is stable in wayland-protocols 1.36 and unstable before that; the interfaces are
    # the same either way.
    if(NOT EXISTS "${WAYLAND_PROTOCOLS_DIR}/staging/cursor-shape/cursor-shape-v1.xml")
        message(FATAL_ERROR
            "wayland-protocols is too old for cursor-shape-v1 (needs 1.32 or newer).")
    endif()
    if(EXISTS "${WAYLAND_PROTOCOLS_DIR}/stable/tablet/tablet-v2.xml")
        set(wl_tablet_xml "${WAYLAND_PROTOCOLS_DIR}/stable/tablet/tablet-v2.xml")
    else()
        set(wl_tablet_xml "${WAYLAND_PROTOCOLS_DIR}/unstable/tablet/tablet-unstable-v2.xml")
    endif()
    clafi_wayland_protocol("${wl_tablet_xml}" tablet-v2)
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/staging/cursor-shape/cursor-shape-v1.xml" cursor-shape-v1)
    # WHAT A CLIPBOARD MANAGER IS ALLOWED AND AN APPLICATION IS NOT. wl_data_device announces a
    # selection to the client holding the keyboard focus and to nobody else, so a window watching
    # the clipboard is told nothing while it is not the one being typed into. This is the channel
    # that answers whatever the focus - asked for through WaylandPlatform::Params and bound in
    # DisplayManager::createDataControlDevice. Staging since wayland-protocols 1.39. KWin
    # implements it; Mutter does not, and there the focus rule stands whatever is asked for.
    if(NOT EXISTS "${WAYLAND_PROTOCOLS_DIR}/staging/ext-data-control/ext-data-control-v1.xml")
        message(FATAL_ERROR
            "wayland-protocols is too old for ext-data-control-v1 (needs 1.39 or newer).")
    endif()
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/staging/ext-data-control/ext-data-control-v1.xml"
        ext-data-control-v1)
    # WHERE A WINDOW WAS. A client cannot ask, so the compositor remembers a toplevel's place
    # against a session id the client persists and puts it back at the next launch - see
    # Window::createToplevel. Staging since wayland-protocols 1.48 (April 2026). KWin answers it
    # from Plasma 6.7; a compositor without the global places the window by its own rule, and
    # the size and state the config kept are the client's own answer there.
    set(wl_session_xml
        "${WAYLAND_PROTOCOLS_DIR}/staging/xdg-session-management/xdg-session-management-v1.xml")
    if(NOT EXISTS "${wl_session_xml}")
        message(FATAL_ERROR
            "wayland-protocols is too old for xdg-session-management-v1 (needs 1.48 or newer).")
    endif()
    clafi_wayland_protocol("${wl_session_xml}" xdg-session-management-v1)
    # RAISING A WINDOW THAT IS ALREADY UP, which only the compositor does. The client asks for a
    # token against the input event that wanted the raise, then hands it back naming the window;
    # see DisplayManager::activate. Every wayland-protocols accepted above carries it.
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/staging/xdg-activation/xdg-activation-v1.xml"
        xdg-activation-v1)
    # A WINDOW'S OPACITY, multiplied in by the compositor - see Window::setAlpha. Every
    # wayland-protocols accepted above carries it.
    clafi_wayland_protocol(
        "${WAYLAND_PROTOCOLS_DIR}/staging/alpha-modifier/alpha-modifier-v1.xml"
        alpha-modifier-v1)

    # A WINDOW ON THE SCREEN'S EDGE. xdg-shell has no request that names an edge; the layer shell
    # states an anchor, a size and a reserved strip, and the compositor does the rest - see
    # Window::createLayerSurface. Never upstreamed into wayland-protocols, so its XML ships in a
    # package of its own: Fedora wlr-protocols-devel, Arch wlr-protocols, both publishing
    # wlr-protocols.pc. KWin implements it; Mutter does not, and a compositor without the global
    # leaves such a window an ordinary toplevel.
    pkg_get_variable(WLR_PROTOCOLS_DIR wlr-protocols pkgdatadir)
    if(NOT WLR_PROTOCOLS_DIR)
        message(FATAL_ERROR
            "wlr-protocols not found. Install wlr-protocols-devel (Fedora) / wlr-protocols.")
    endif()
    clafi_wayland_protocol(
        "${WLR_PROTOCOLS_DIR}/unstable/wlr-layer-shell-unstable-v1.xml"
        wlr-layer-shell-unstable-v1)

    # A C target of its own. ClaFi is CXX-only, and every source in it is walked by CMake's
    # module dependency scanner - which has no business reading a generated protocol table, and no
    # C compiler to read it with.
    add_library(ClaFiWaylandProtocols STATIC ${wl_protocol_sources})
    set_target_properties(ClaFiWaylandProtocols PROPERTIES
        C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
    target_include_directories(ClaFiWaylandProtocols PUBLIC "${wl_gen}")
    target_link_libraries(ClaFiWaylandProtocols PUBLIC PkgConfig::WAYLAND_CLIENT)

    # The headers are a target of their own so ClaFi can depend on their EXISTENCE without
    # depending on the C library being compiled. The module scan is its own build step, and it
    # reads a global module fragment that includes them - which is earlier than a link-time
    # dependency alone would guarantee.
    add_custom_target(ClaFiWaylandHeaders DEPENDS ${wl_protocol_headers})
    add_dependencies(ClaFiWaylandProtocols ClaFiWaylandHeaders)
    add_dependencies(ClaFi ClaFiWaylandHeaders)

    # Stated on ClaFi directly rather than left to propagate through the link, because the scan
    # step needs the include path too.
    target_include_directories(ClaFi PRIVATE "${wl_gen}")
    target_link_libraries(ClaFi PUBLIC ClaFiWaylandProtocols)

    # THE KEYBOARD. wl_keyboard hands a client a keymap and key numbers and nothing else; xkbcommon
    # is what reads the one and turns the other into keys and characters. Debian/Ubuntu
    # libxkbcommon-dev; Fedora libxkbcommon-devel.
    pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
    target_include_directories(ClaFi PRIVATE ${XKBCOMMON_INCLUDE_DIRS})
    target_link_libraries(ClaFi PUBLIC PkgConfig::XKBCOMMON)

    # THE FONT STACK, which is the operating system layer's and not the display server's, but is
    # built with it: Platform/Linux/ is in the build only alongside a display server. FreeType
    # rasterizes a glyph, HarfBuzz shapes a run, fontconfig names the file for a family and
    # libunibreak names where a line may break. Debian/Ubuntu libfreetype-dev libharfbuzz-dev
    # libfontconfig-dev libunibreak-dev; Fedora freetype-devel harfbuzz-devel fontconfig-devel
    # libunibreak-devel. The include paths are stated on ClaFi directly for the same reason as
    # the protocol headers above.
    pkg_check_modules(CLAFI_TEXT REQUIRED IMPORTED_TARGET
        freetype2 harfbuzz fontconfig libunibreak)
    target_include_directories(ClaFi PRIVATE ${CLAFI_TEXT_INCLUDE_DIRS})
    target_link_libraries(ClaFi PUBLIC PkgConfig::CLAFI_TEXT)

    # THE PNG DECODER, which the platform layer supplies behind ClaFi.Core.Graphics.Png the
    # way Windows supplies it through WIC. Debian/Ubuntu libpng-dev; Fedora libpng-devel.
    pkg_check_modules(CLAFI_PNG REQUIRED IMPORTED_TARGET libpng)
    target_include_directories(ClaFi PRIVATE ${CLAFI_PNG_INCLUDE_DIRS})
    target_link_libraries(ClaFi PUBLIC PkgConfig::CLAFI_PNG)

    # THE DESKTOP'S SETTINGS, which the XDG portal serves over the session bus - the colour mode
    # an application set to Auto follows. libdbus-1 is on every desktop; its headers are not.
    # Debian/Ubuntu libdbus-1-dev; Fedora dbus-devel.
    pkg_check_modules(CLAFI_DBUS REQUIRED IMPORTED_TARGET dbus-1)
    target_include_directories(ClaFi PRIVATE ${CLAFI_DBUS_INCLUDE_DIRS})
    target_link_libraries(ClaFi PUBLIC PkgConfig::CLAFI_DBUS)
endif()

if(NOT CLAFI_PLATFORM STREQUAL "none")
    # THE EXECUTABLES, built only with a platform layer because without one there is nothing to
    # link against. The MillScene showcase's sources are excluded from the library glob above -
    # a showcase is application code that imports ClaFi, not part of it - so the six files are
    # named here instead. Five of them know nothing of any platform; the sixth is the entry
    # point, and there is one per platform.
    set(millscene_dir "${CMAKE_CURRENT_SOURCE_DIR}/Showcase/MillScene")
    add_executable(MillScene
        "${millscene_dir}/MillScene_Icon.cpp"
        "${millscene_dir}/MillScene_Showcase.cpp"
        "${millscene_dir}/MillScene_Wayland.cpp")
    target_sources(MillScene PRIVATE FILE_SET CXX_MODULES FILES
        "${millscene_dir}/MillScene_Icon.cppm"
        "${millscene_dir}/MillScene_Showcase.cppm"
        "${millscene_dir}/MillScene_Main.cppm")
    target_link_libraries(MillScene PRIVATE ClaFi)
    target_compile_options(MillScene PRIVATE ${clafi_compile_options})

    # THE TEXT ENGINE SHOWCASE, laid out the same way: six page modules, the Icon module and the
    # Main module know nothing of any platform, and the entry point is one file per platform. It
    # is the text stack's test bed - fifty thousand paragraphs on the code page, formulas set in
    # mathematical symbols, prose in a second family, emoji set in one family after another - so
    # what the font stack lacks shows here first.
    set(textengine_dir "${CMAKE_CURRENT_SOURCE_DIR}/Showcase/TextEngine/Src")
    add_executable(TextEngine
        "${textengine_dir}/TextEngine_Main.cpp"
        "${textengine_dir}/TextEngine_Icon.cpp"
        "${textengine_dir}/TextEngine_Wayland.cpp"
        "${textengine_dir}/Pages/TextEngine_Code.cpp"
        "${textengine_dir}/Pages/TextEngine_Ledger.cpp"
        "${textengine_dir}/Pages/TextEngine_Math.cpp"
        "${textengine_dir}/Pages/TextEngine_Emoji.cpp"
        "${textengine_dir}/Pages/TextEngine_Scripts.cpp")
    target_sources(TextEngine PRIVATE FILE_SET CXX_MODULES FILES
        "${textengine_dir}/TextEngine_Main.cppm"
        "${textengine_dir}/TextEngine_Icon.cppm"
        "${textengine_dir}/Pages/TextEngine_Code.cppm"
        "${textengine_dir}/Pages/TextEngine_Features.cppm"
        "${textengine_dir}/Pages/TextEngine_Ledger.cppm"
        "${textengine_dir}/Pages/TextEngine_Math.cppm"
        "${textengine_dir}/Pages/TextEngine_Emoji.cppm"
        "${textengine_dir}/Pages/TextEngine_Scripts.cppm")
    target_link_libraries(TextEngine PRIVATE ClaFi)
    target_compile_options(TextEngine PRIVATE ${clafi_compile_options})

    # WHATSCLIP, which lists what is on the clipboard and reads it - including the formats
    # nothing in the framework understands, which it shows as the bytes they arrived as. It
    # is the application the Transfer subsystem's known-against-unknown split exists for, so
    # it is also the test of it.
    #
    # A LIBRARY AND AN ENTRY POINT, so that a viewer built outside this tree links the library
    # and writes the entry point alone - the split WhatsClip_Lib.vcxitems makes for Visual
    # Studio. The entry point is where the pick lists are built, which is what such a viewer
    # adds to.
    set(whatsclip_dir "${CMAKE_CURRENT_SOURCE_DIR}/Tools/WhatsClip/Src")
    add_library(WhatsClip_Lib STATIC
        "${whatsclip_dir}/WhatsClip_Encodings.cpp"
        "${whatsclip_dir}/WhatsClip_Languages.cpp"
        "${whatsclip_dir}/WhatsClip_Main.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_AutoPick.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_BytesPage.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_EncodingPick.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_FormatView.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_HandlePage.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_LanguagePick.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_Page.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_PicturePage.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_TextPreviewPage.cpp"
        "${whatsclip_dir}/Pages/WhatsClip_TextPage.cpp")
    # PUBLIC for the reason ClaFi's file set is: the entry point imports these modules.
    target_sources(WhatsClip_Lib PUBLIC FILE_SET CXX_MODULES FILES
        "${whatsclip_dir}/WhatsClip_Encodings.cppm"
        "${whatsclip_dir}/WhatsClip_Languages.cppm"
        "${whatsclip_dir}/WhatsClip_Main.cppm"
        "${whatsclip_dir}/WhatsClip_Icon.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_Page.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_AutoPick.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_BytesPage.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_EncodingPick.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_FormatView.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_HandlePage.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_LanguagePick.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_PicturePage.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_TextPreviewPage.cppm"
        "${whatsclip_dir}/Pages/WhatsClip_TextPage.cppm")
    target_link_libraries(WhatsClip_Lib PUBLIC ClaFi)
    target_compile_options(WhatsClip_Lib PRIVATE ${clafi_compile_options})

    add_executable(WhatsClip "${whatsclip_dir}/WhatsClip_Wayland.cpp")
    target_link_libraries(WhatsClip PRIVATE WhatsClip_Lib)
    target_compile_options(WhatsClip PRIVATE ${clafi_compile_options})

    # THEMES, the browser the themes are made in, laid out the same way: the Main module
    # and every page and control under it know nothing of any platform, and the entry point is one
    # file per platform. Its modules are named ThisApp.*, as an application's are - it imports the
    # library and is no part of it. Dom Benchmark, the other tool, is a console program built by
    # Visual Studio only.
    set(themes_dir "${CMAKE_CURRENT_SOURCE_DIR}/Tools/Themes/Src")
    add_executable(Themes
        "${themes_dir}/Themes_Main.cpp"
        "${themes_dir}/Themes_Icon.cpp"
        "${themes_dir}/Themes_Wayland.cpp"
        "${themes_dir}/Pages/ThemePage.cpp"
        "${themes_dir}/Util/ThisApp.CodeOptions.cpp"
        "${themes_dir}/Util/ThisApp.FloorSlider.cpp"
        "${themes_dir}/Util/ThisApp.ThemeToCppCode.cpp")
    target_sources(Themes PRIVATE FILE_SET CXX_MODULES FILES
        "${themes_dir}/Themes_Main.cppm"
        "${themes_dir}/Themes_Icon.cppm"
        "${themes_dir}/Themes_BrowserControl.cppm"
        "${themes_dir}/Pages/BasePage.cppm"
        "${themes_dir}/Pages/HomePage.cppm"
        "${themes_dir}/Pages/PreviewPanel.cppm"
        "${themes_dir}/Pages/ThemePage.cppm"
        "${themes_dir}/Util/ThisApp.CodeOptions.cppm"
        "${themes_dir}/Util/ThisApp.Consts.cppm"
        "${themes_dir}/Util/ThisApp.FloorSlider.cppm"
        "${themes_dir}/Util/ThisApp.HueRuleControl.cppm"
        "${themes_dir}/Util/ThisApp.Palette.Controls.cppm"
        "${themes_dir}/Util/ThisApp.RuleSlider.cppm"
        "${themes_dir}/Util/ThisApp.ThemeToCppCode.cppm"
        "${themes_dir}/Util/ThisApp.Utils.cppm")
    target_link_libraries(Themes PRIVATE ClaFi)
    target_compile_options(Themes PRIVATE ${clafi_compile_options})
endif()
