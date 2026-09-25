export module ClaFi.Core.Transfer.Formats;

import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // THE FRAMEWORK'S OWN FORMATS, what a paste asks for; each platform serves one from a
    // platform format of its own. See Transfer
    export enum class StandardFormat
    {
        Text,
        Picture
    };

    // WHICH FORMAT, at runtime. See Transfer
    export class Format
    {
    public:
        // WHICH VOCABULARY NAMES IT - the framework's own closed set, or anything else. Both
        // kinds have names; only the standard set has one this layer can state, every platform
        // spelling the others for itself.
        //
        // Declared in the same order as the identity alternatives below, which is what lets
        // kind() read off the index.
        enum class Kind
        {
            Standard,
            Custom
        };

        [[nodiscard]] static Format of(StandardFormat);
        [[nodiscard]] static Format custom(std::wstring name);

        [[nodiscard]] Kind kind() const;
        // Each answers for its own kind alone. Asking the other is a precondition failure.
        [[nodiscard]] StandardFormat standard() const;
        [[nodiscard]] const std::wstring& name() const;

        bool operator==(const Format&) const = default;
    private:
        using Identity = std::variant<StandardFormat, std::wstring>;

        explicit Format(Identity);

        Identity m_identity;
    };

    // Ordered best first wherever one appears. Order is the only fidelity hint either platform
    // carries, on both the offering and the accepting side.
    export using FormatList = std::vector<Format>;

    // What a format needs to be usable at compile time: the type its content is captured as,
    // and its runtime identity.
    export template<typename F>
        concept IsTransferFormat = requires
        {
            typename F::Content;
            { F::format() } -> std::same_as<Format>;
        };

    // Plain text, served from the platform's own text format. See Transfer
    export struct PlainText
    {
        using Content = std::wstring;
        [[nodiscard]] static Format format();
    };

    // A picture as pixels, served from whichever platform format carries one best. See Transfer
    export struct Picture
    {
        using Content = Graphics::Bitmap;
        [[nodiscard]] static Format format();
    };
}
