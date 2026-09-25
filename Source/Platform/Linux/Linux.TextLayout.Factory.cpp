// An implementation unit of the module that declares the factory, not of the platform module
// that supplies the class. createNativeTextLayout is attached to ClaFi.Core.TextEngine.Layout
// because that is where it was declared, and a function belongs to the module it was declared
// in - so this is the only place it can be defined. Linux.DirWatch.cpp does the same thing for
// ClaFi.Core.System.DirWatch.
//
// The Windows twin declares it a second time inside Windows.DWrite.cppm and defines it there,
// which MSVC allows and clang does not.
module ClaFi.Core.TextEngine.Layout;

import ClaFi.Platform.Linux.TextLayout;

import ClaFi.StdLib;

namespace ClaFi
{
    std::unique_ptr<INativeTextLayout> createNativeTextLayout()
    {
        return std::make_unique<Platform::Linux::HarfBuzzLayout>();
    }
}
