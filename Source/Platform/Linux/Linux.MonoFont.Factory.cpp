// An implementation unit of the module that declares the factory, not of the platform module
// that supplies the class - the same arrangement as Linux.TextLayout.Factory.cpp, and for the
// same reason: a function belongs to the module it was declared in.
module ClaFi.Core.TextEngine.Mono;

import ClaFi.Platform.Linux.TextLayout;
import ClaFi.Platform.Linux.Fonts;

import ClaFi.StdLib;

namespace ClaFi
{
    // The face the layouts would set the request in: the primary of fontconfig's chain for it.
    // fontconfig always names a face, so the face's own word on its pitch is the whole of the
    // answer - a proportional one answers null once, and no paragraph set in it asks again.
    std::unique_ptr<INativeMonoFont> createNativeMonoFont(const MonoFontRequest& request)
    {
        Platform::Linux::FaceChain& chain = Platform::Linux::fontSet().chain(request.family,
            request.weight, request.style);
        Platform::Linux::FontFace& face = chain.primary();
        if (!face.isFixedWidth() || !face.hasGlyph(U' '))
            return nullptr;
        return std::make_unique<Platform::Linux::FreeTypeMonoFont>(face, request.size);
    }
}
