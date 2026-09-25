#pragma once

// The font stack. FreeType opens a face and rasterizes a glyph, HarfBuzz shapes a run, fontconfig
// names the file for a family, libunibreak says where a line may break. Included from the global
// module fragment of every module that touches them: CMake has no support for header units, so a
// C header reaches a module this way or not at all.

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <fontconfig/fontconfig.h>
#include <linebreak.h>
