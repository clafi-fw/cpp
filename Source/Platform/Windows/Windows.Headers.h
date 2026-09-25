#pragma once

#ifndef _MSC_VER
#error This library requires MSVC for auto-linking support.
#endif
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "DWrite.lib")
#pragma comment(lib, "d2d1.lib")

// Prevent windows.h from polluting the global namespace with min/max macros
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Exclude rarely-used stuff from Windows headers to speed up compilation
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <dwrite_3.h>
#include <d2d1.h>
#include <d2d1_1.h>

#include <wrl/client.h>

// Just in case a 3rd party library included windows.h earlier without NOMINMAX
#undef min
#undef max

#define WINAPI_IMPORT(winapi_name, real_name, return_type, ...)   \
        __pragma(comment(linker,                                  \
            "/alternatename:__imp_" #winapi_name                  \
            "=__imp_" real_name))                                 \
        export extern "C" __declspec(dllimport) return_type __stdcall               \
            winapi_name(__VA_ARGS__);                             \
