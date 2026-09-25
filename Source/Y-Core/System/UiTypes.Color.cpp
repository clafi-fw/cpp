module ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // What carries an Hsl to and from sRGB. Every name below serves those two conversions and
    // nothing else reaches them.
    namespace Okhsl
    {
        // A colour in Oklab: one perceptual lightness and two opponent axes. Lightness is
        // named apart from Hsl::luminosity throughout, the two being different numbers - the
        // toe below is what carries one to the other.
        struct Oklab
        {
            float lightness;
            float a;
            float b;
        };

        // A colour in sRGB's primaries with the transfer function undone, which is the space
        // the cone response below is a linear map of.
        struct LinearRgb
        {
            float red;
            float green;
            float blue;
        };

        // The most colourful point of a hue: the lightness at which the gamut reaches
        // furthest from grey, and how far it reaches there.
        struct HueCusp
        {
            float lightness;
            float chroma;
        };

        // How fast chroma may grow as a colour leaves black, and as it leaves white. A gamut
        // slice at one hue is close to the triangle these two slopes describe.
        struct ChromaSlopes
        {
            float fromBlack;
            float fromWhite;
        };

        // The three chroma values the saturation curve is built from at one lightness and
        // hue: the slope it leaves zero on, where it passes at the knee, and the gamut
        // boundary it reaches at saturation 1. Two rational segments meet at the knee.
        struct ChromaCurve
        {
            float start;
            float knee;
            float gamut;
        };

        // Where the knee sits on the saturation axis. Below it saturation runs out to the
        // knee chroma, above it to the gamut boundary, so most of the axis describes colours
        // the boundary's shape is not steering.
        constexpr float k_kneeSaturation = 0.8f;

        // Chroma this small names no direction, so a hue read from it would be whichever way
        // the last bit of the matrix rounded.
        constexpr float k_achromaticChroma = 1e-6f;

        // Both segments divide by the span above the value below them, so the three chroma
        // values have to stay apart and above zero however the fits come out.
        constexpr float k_leastChroma = 1e-5f;

        // The toe carries Oklab's lightness to a lightness matching CIE L* for greys, so half
        // way up the channel is half way up to the eye. The offset and the softening are the
        // published fit; the slope follows from them, and is what holds 0 and 1 in place.
        constexpr float k_toeOffset = 0.206f;
        constexpr float k_toeSoftening = 0.03f;
        constexpr float k_toeSlope = (1.0f + k_toeOffset) / (1.0f + k_toeSoftening);

        [[nodiscard]] static float toe(float lightness)
        {
            const float shifted = k_toeSlope * lightness - k_toeOffset;
            const float softened = 4.0f * k_toeSoftening * k_toeSlope * lightness;
            return 0.5f * (shifted + std::sqrt(shifted * shifted + softened));
        }

        [[nodiscard]] static float toeInverse(float luminosity)
        {
            return (luminosity * luminosity + k_toeOffset * luminosity)
                / (k_toeSlope * (luminosity + k_toeSoftening));
        }

        [[nodiscard]] static float srgbEncode(float value)
        {
            if (value <= 0.0031308f)
            {
                return 12.92f * value;
            }
            return 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
        }

        [[nodiscard]] static float srgbDecode(float value)
        {
            if (value <= 0.04045f)
            {
                return value / 12.92f;
            }
            return std::pow((value + 0.055f) / 1.055f, 2.4f);
        }

        [[nodiscard]] static Oklab oklabFromLinearRgb(LinearRgb rgb)
        {
            const float longCone = 0.4122214708f * rgb.red + 0.5363325363f * rgb.green + 0.0514459929f * rgb.blue;
            const float mediumCone = 0.2119034982f * rgb.red + 0.6806995451f * rgb.green + 0.1073969566f * rgb.blue;
            const float shortCone = 0.0883024619f * rgb.red + 0.2817188376f * rgb.green + 0.6299787005f * rgb.blue;

            // The cube root is where the perceptual scale comes from: response compresses the
            // same way at every level, so a fixed step means a fixed visible step.
            const float longRoot = std::cbrt(longCone);
            const float mediumRoot = std::cbrt(mediumCone);
            const float shortRoot = std::cbrt(shortCone);

            return {
                0.2104542553f * longRoot + 0.7936177850f * mediumRoot - 0.0040720468f * shortRoot,
                1.9779984951f * longRoot - 2.4285922050f * mediumRoot + 0.4505937099f * shortRoot,
                0.0259040371f * longRoot + 0.7827717662f * mediumRoot - 0.8086757660f * shortRoot,
            };
        }

        [[nodiscard]] static LinearRgb linearRgbFromOklab(Oklab lab)
        {
            const float longRoot = lab.lightness + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
            const float mediumRoot = lab.lightness - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
            const float shortRoot = lab.lightness - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

            const float longCone = longRoot * longRoot * longRoot;
            const float mediumCone = mediumRoot * mediumRoot * mediumRoot;
            const float shortCone = shortRoot * shortRoot * shortRoot;

            return {
                +4.0767416621f * longCone - 3.3077115913f * mediumCone + 0.2309699292f * shortCone,
                -1.2684380046f * longCone + 2.6097574011f * mediumCone - 0.3413193965f * shortCone,
                -0.0041960863f * longCone - 0.7034186147f * mediumCone + 1.7076147010f * shortCone,
            };
        }

        // The largest chroma per unit lightness a hue can carry before a channel goes
        // negative. The direction is a unit vector on the Oklab plane, so the answer depends
        // on the hue alone. Which channel binds first divides the wheel into three arcs; a
        // polynomial fitted over the arc gives a start, and one Halley step lands on the root.
        [[nodiscard]] static float maxSaturationOf(float a, float b)
        {
            struct Struct
            {
                float k0;
                float k1;
                float k2;
                float k3;
                float k4;
                float weightLong;
                float weightMedium;
                float weightShort;
            };
            constexpr Struct sRed{
                .k0 = +1.19086277f,
                .k1 = +1.76576728f,
                .k2 = +0.59662641f,
                .k3 = +0.75515197f,
                .k4 = +0.56771245f,
                .weightLong = +4.0767416621f,
                .weightMedium = -3.3077115913f,
                .weightShort = +0.2309699292f
            };
            constexpr Struct sGreen{
                .k0 = +0.73956515f,
                .k1 = -0.45954404f,
                .k2 = +0.08285427f,
                .k3 = +0.12541070f,
                .k4 = +0.14503204f,
                .weightLong = -1.2684380046f,
                .weightMedium = +2.6097574011f,
                .weightShort = -0.3413193965f
            };
            constexpr Struct sBlue{
                .k0 = +1.35733652f,
                .k1 = -0.00915799f,
                .k2 = -1.15130210f,
                .k3 = -0.50559606f,
                .k4 = +0.00692167f,
                .weightLong = -0.0041960863f,
                .weightMedium = -0.7034186147f,
                .weightShort = +1.7076147010f
            };

            Struct s;

            if (-1.88170328f * a - 0.80936493f * b > 1.0f)
            {
                s = sRed;
            }
            else if (1.81444104f * a - 1.19445276f * b > 1.0f)
            {
                s = sGreen;
            }
            else
            {
                s = sBlue;
            }

            const float start = s.k0 + s.k1 * a + s.k2 * b + s.k3 * a * a + s.k4 * a * b;

            const float stepLong = +0.3963377774f * a + 0.2158037573f * b;
            const float stepMedium = -0.1055613458f * a - 0.0638541728f * b;
            const float stepShort = -0.0894841775f * a - 1.2914855480f * b;

            const float longRoot = 1.0f + start * stepLong;
            const float mediumRoot = 1.0f + start * stepMedium;
            const float shortRoot = 1.0f + start * stepShort;

            const float longRootSquare = longRoot * longRoot;
            const float mediumRootSquare = mediumRoot * mediumRoot;
            const float shortRootSquare = shortRoot * shortRoot;

            const float longCone = longRootSquare * longRoot;
            const float mediumCone = mediumRootSquare * mediumRoot;
            const float shortCone = shortRootSquare * shortRoot;

            const float longSlope = 3.0f * stepLong * longRootSquare;
            const float mediumSlope = 3.0f * stepMedium * mediumRootSquare;
            const float shortSlope = 3.0f * stepShort * shortRootSquare;

            const float longCurve = 6.0f * stepLong * stepLong * longRoot;
            const float mediumCurve = 6.0f * stepMedium * stepMedium * mediumRoot;
            const float shortCurve = 6.0f * stepShort * stepShort * shortRoot;

            const float channel = s.weightLong * longCone
                + s.weightMedium * mediumCone
                + s.weightShort * shortCone;
            const float firstDerivative = s.weightLong * longSlope
                + s.weightMedium * mediumSlope
                + s.weightShort * shortSlope;
            const float secondDerivative = s.weightLong * longCurve
                + s.weightMedium * mediumCurve
                + s.weightShort * shortCurve;

            return start - channel * firstDerivative
                / (firstDerivative * firstDerivative - 0.5f * channel * secondDerivative);
        }

        [[nodiscard]] static HueCusp cuspOf(float a, float b)
        {
            const float saturation = maxSaturationOf(a, b);

            // Scaling a linear RGB colour scales its Oklab lightness and chroma together, so
            // the ray out of black is straight and can be read at any lightness. Read at one,
            // the channel standing highest says how far the ray has to be brought back.
            const LinearRgb atOne = linearRgbFromOklab({ 1.0f, saturation * a, saturation * b });
            const float highest = (std::max)({ atOne.red, atOne.green, atOne.blue });
            const float lightness = std::cbrt(1.0f / highest);
            return { lightness, lightness * saturation };
        }

        // How far along the line from a point on the grey axis toward a chroma the gamut is
        // left. Below the cusp the boundary is the straight ray out of black, which the
        // triangle answers exactly; above it the boundary curves, so the triangle's answer
        // starts one Halley step per channel.
        [[nodiscard]] static float gamutIntersection(float a, float b, float lightness,
            float chroma, float fromLightness, HueCusp cusp)
        {
            const float belowCusp = (lightness - fromLightness) * cusp.chroma
                - (cusp.lightness - fromLightness) * chroma;
            if (belowCusp <= 0.0f)
            {
                return cusp.chroma * fromLightness
                    / (chroma * cusp.lightness + cusp.chroma * (fromLightness - lightness));
            }

            float found = cusp.chroma * (fromLightness - 1.0f)
                / (chroma * (cusp.lightness - 1.0f) + cusp.chroma * (fromLightness - lightness));

            const float stepLong = +0.3963377774f * a + 0.2158037573f * b;
            const float stepMedium = -0.1055613458f * a - 0.0638541728f * b;
            const float stepShort = -0.0894841775f * a - 1.2914855480f * b;

            const float lightnessSpan = lightness - fromLightness;
            const float longRate = lightnessSpan + chroma * stepLong;
            const float mediumRate = lightnessSpan + chroma * stepMedium;
            const float shortRate = lightnessSpan + chroma * stepShort;

            const float atFound = fromLightness * (1.0f - found) + found * lightness;
            const float chromaAtFound = found * chroma;

            const float longRoot = atFound + chromaAtFound * stepLong;
            const float mediumRoot = atFound + chromaAtFound * stepMedium;
            const float shortRoot = atFound + chromaAtFound * stepShort;

            const float longCone = longRoot * longRoot * longRoot;
            const float mediumCone = mediumRoot * mediumRoot * mediumRoot;
            const float shortCone = shortRoot * shortRoot * shortRoot;

            const float longSlope = 3.0f * longRate * longRoot * longRoot;
            const float mediumSlope = 3.0f * mediumRate * mediumRoot * mediumRoot;
            const float shortSlope = 3.0f * shortRate * shortRoot * shortRoot;

            const float longCurve = 6.0f * longRate * longRate * longRoot;
            const float mediumCurve = 6.0f * mediumRate * mediumRate * mediumRoot;
            const float shortCurve = 6.0f * shortRate * shortRate * shortRoot;

            // The rows carrying each channel out of cone response. One channel is walked at a
            // time: how far past 1 it stands, and the step that would bring it back.
            constexpr std::array<std::array<float, 3ull>, 3ull> k_channelWeights{
                std::array{ +4.0767416621f, -3.3077115913f, +0.2309699292f },
                std::array{ -1.2684380046f, +2.6097574011f, -0.3413193965f },
                std::array{ -0.0041960863f, -0.7034186147f, +1.7076147010f },
            };

            float nearest = k_maxFloat;
            for (const std::array<float, 3ull>& weights : k_channelWeights)
            {
                const float excess = weights[0ull] * longCone
                    + weights[1ull] * mediumCone
                    + weights[2ull] * shortCone - 1.0f;
                const float firstDerivative = weights[0ull] * longSlope
                    + weights[1ull] * mediumSlope
                    + weights[2ull] * shortSlope;
                const float secondDerivative = weights[0ull] * longCurve
                    + weights[1ull] * mediumCurve
                    + weights[2ull] * shortCurve;

                const float step = firstDerivative
                    / (firstDerivative * firstDerivative - 0.5f * excess * secondDerivative);

                // A step running backwards names no crossing ahead, that channel's boundary
                // standing behind the point already found.
                if (step < 0.0f)
                {
                    continue;
                }
                nearest = (std::min)(nearest, -excess * step);
            }

            if (nearest != k_maxFloat)
            {
                found += nearest;
            }
            return found;
        }

        [[nodiscard]] static ChromaSlopes cuspSlopesOf(HueCusp cusp)
        {
            return {
                cusp.chroma / cusp.lightness,
                cusp.chroma / (1.0f - cusp.lightness),
            };
        }

        // The slopes of a triangle laid through the knee rather than through the cusp. The
        // two fits describe how far the real gamut stands off the cusp triangle at each hue.
        [[nodiscard]] static ChromaSlopes kneeSlopesOf(float a, float b)
        {
            const float fromBlack = 0.11516993f + 1.0f / (
                +7.44778970f + 4.15901240f * b
                + a * (-2.19557347f + 1.75198401f * b
                    + a * (-2.13704948f - 10.02301043f * b
                        + a * (-4.24894561f + 5.38770819f * b + 4.69891013f * a))));

            const float fromWhite = 0.11239642f + 1.0f / (
                +1.61320320f - 0.68124379f * b
                + a * (+0.40370612f + 0.90148123f * b
                    + a * (-0.27087943f + 0.61223990f * b
                        + a * (+0.00299215f - 0.45399568f * b - 0.14661872f * a))));

            return { fromBlack, fromWhite };
        }

        // Less than the smaller of two reaches, and smoothly so, which is what keeps a curve
        // crossing from the black side of the gamut to the white side free of a corner where
        // the two swap over.
        [[nodiscard]] static float softMinimum(float first, float second)
        {
            return std::sqrt(1.0f / (1.0f / (first * first) + 1.0f / (second * second)));
        }

        // The same, drawn closer to the smaller of the two.
        [[nodiscard]] static float tightSoftMinimum(float first, float second)
        {
            const float firstSquared = first * first;
            const float secondSquared = second * second;
            return std::sqrt(softMinimum(firstSquared, secondSquared));
        }

        [[nodiscard]] static ChromaCurve chromaCurveOf(float lightness, float a, float b)
        {
            const HueCusp cusp = cuspOf(a, b);
            const float gamut = gamutIntersection(a, b, lightness, 1.0f, lightness, cusp);

            const ChromaSlopes cuspSlopes = cuspSlopesOf(cusp);
            const float standOff = gamut / (std::min)(lightness * cuspSlopes.fromBlack,
                (1.0f - lightness) * cuspSlopes.fromWhite);

            const ChromaSlopes kneeSlopes = kneeSlopesOf(a, b);
            const float knee = 0.9f * standOff * tightSoftMinimum(lightness * kneeSlopes.fromBlack,
                (1.0f - lightness) * kneeSlopes.fromWhite);

            // Where the curve leaves zero is the same shape at every hue, so one pair of
            // slopes near the average of the real ones stands for all of them.
            const float start = softMinimum(lightness * 0.4f, (1.0f - lightness) * 0.8f);

            // A lightness at either extreme drives all three toward zero, which is the case
            // the floor and the ordering catch.
            const float orderedKnee = (std::max)(knee, k_leastChroma);
            return {
                (std::max)(start, k_leastChroma),
                orderedKnee,
                (std::max)(gamut, orderedKnee * 1.0009765625f),
            };
        }
    }

    Color::Color(Color c1, Color c2, float k2)
        :
        Color{ c1 }
    {
        blend(c2, k2);
    }

    std::wstring Color::toStr(HexFormat format) const
    {
        const wchar_t* abc = L"0123456789ABCDEF";

        if (alpha == 255)
        {
            wchar_t buf[] = {
                L'#',
                abc[red >> 4], abc[red & 15],
                abc[green >> 4], abc[green & 15],
                abc[blue >> 4], abc[blue & 15],
                0
            };
            return std::wstring(buf);
        }

        if (format == HexFormat::RGBA)
        {
            wchar_t buf[] = {
                L'#',
                abc[red >> 4], abc[red & 15],
                abc[green >> 4], abc[green & 15],
                abc[blue >> 4], abc[blue & 15],
                abc[alpha >> 4], abc[alpha & 15],
                0
            };
            return std::wstring(buf);
        }

        wchar_t buf[] = {
            L'#',
            abc[alpha >> 4], abc[alpha & 15],
            abc[red >> 4], abc[red & 15],
            abc[green >> 4], abc[green & 15],
            abc[blue >> 4], abc[blue & 15],
            0
        };
        return std::wstring(buf);
    }

    Hsl::Hsl(float hue, float saturation, float luminosity)
        :
        hue{ hue },
        saturation{ saturation },
        luminosity{ luminosity }
    {
    }

    Hsl::Hsl(int hueDegree, int saturationPercent, int lumPercent)
        :
        hue{ static_cast<float>(hueDegree) / 360.0f },
        saturation{ static_cast<float>(saturationPercent) / 100.0f },
        luminosity{ static_cast<float>(lumPercent) / 100.0f }
    {
    }

    // Mixes two colours with a factor of color2. Luminosity is a plain interpolation, and means
    // the same visible amount wherever the two stand, the channel being perceptual.
    //
    // Hue and saturation are not independent, so they cannot be interpolated separately: read the
    // pair as a point on a disc, with hue the angle and saturation the distance from the centre.
    // The result is the point that fraction of the way along the straight line between them - a
    // Cartesian interpolation - which is why mixing two opposite hues correctly passes through grey
    // rather than sweeping around the rim.
    //
    // The disc is one of saturation rather than of chroma, so where the two colours differ in
    // luminosity the path is not quite the straight line through Oklab that mixing light would
    // take. What it holds instead is the property the callers want: a mix of two colours the
    // gamut holds is a colour the gamut holds.
    Hsl::Hsl(Hsl color1, Hsl color2, float factor)
    {
        luminosity = std::lerp(color1.luminosity, color2.luminosity, factor);
        // Protect luminosity against out-of-bounds factors, just like saturation
        luminosity = std::clamp(luminosity, 0.0f, 1.0f);

        const float angle1 = color1.hue * k_2Pi;
        const float angle2 = color2.hue * k_2Pi;

        // Explicit std:: prefixes ensure we use the float overloads, preventing double-precision conversion
        const float x1 = color1.saturation * std::cos(angle1);
        const float y1 = color1.saturation * std::sin(angle1);
        const float x2 = color2.saturation * std::cos(angle2);
        const float y2 = color2.saturation * std::sin(angle2);

        const float resultX = std::lerp(x1, x2, factor);
        const float resultY = std::lerp(y1, y2, factor);

        saturation = std::sqrt(resultX * resultX + resultY * resultY);

        // At the exact centre of the disc there is no angle to recover, so atan2 cannot preserve the
        // hue and it has to be interpolated directly.
        if (saturation <= 0.00001f)
        {
            float hueDiff = color2.hue - color1.hue;

            // Wrap the difference so it takes the shortest path around the wheel
            if (hueDiff > 0.5f)
            {
                hueDiff -= 1.0f;
            }

            if (hueDiff < -0.5f)
            {
                hueDiff += 1.0f;
            }

            hue = color1.hue + hueDiff * factor;

            // normalization to [0.0, 1.0)
            hue -= std::floor(hue);
            return;
        }

        const float resultAngle = std::atan2(resultY, resultX);
        hue = resultAngle / k_2Pi;
        // Because atan2 restricts output to [-PI, PI], hue is guaranteed to be in [-0.5, 0.5].
        if (hue < 0.0f)
        {
            hue += 1.0f;
        }
    }

    Hsl::Hsl(const Color& color)
    {
        const Okhsl::Oklab lab = Okhsl::oklabFromLinearRgb({
            Okhsl::srgbDecode(color.red / 255.0f),
            Okhsl::srgbDecode(color.green / 255.0f),
            Okhsl::srgbDecode(color.blue / 255.0f),
        });

        const float chroma = std::sqrt(lab.a * lab.a + lab.b * lab.b);
        luminosity = Okhsl::toe(lab.lightness);

        if (chroma < Okhsl::k_achromaticChroma or lab.lightness <= 0.0f or lab.lightness >= 1.0f)
        {
            hue = 0.0f;
            saturation = 0.0f;
            return;
        }

        const float a = lab.a / chroma;
        const float b = lab.b / chroma;

        hue = 0.5f + std::atan2(-lab.b, -lab.a) / k_2Pi;

        const Okhsl::ChromaCurve curve = Okhsl::chromaCurveOf(lab.lightness, a, b);
        if (chroma < curve.knee)
        {
            const float scale = Okhsl::k_kneeSaturation * curve.start;
            const float bend = 1.0f - scale / curve.knee;
            saturation = Okhsl::k_kneeSaturation * chroma / (scale + bend * chroma);
            return;
        }

        const float scale = (1.0f - Okhsl::k_kneeSaturation) * curve.knee * curve.knee
            / (Okhsl::k_kneeSaturation * Okhsl::k_kneeSaturation * curve.start);
        const float bend = 1.0f - scale / (curve.gamut - curve.knee);
        const float beyondKnee = chroma - curve.knee;

        // A colour standing outside the boundary the curve found reads as fully saturated.
        // The one place that happens is the sRGB gamut's black to blue edge, which stands
        // proud of the body the fits describe; leaving the value above 1 would send toColor
        // past the pole the second segment is built around.
        saturation = (std::min)(1.0f, Okhsl::k_kneeSaturation
            + (1.0f - Okhsl::k_kneeSaturation) * beyondKnee / (scale + bend * beyondKnee));
    }

    Color Hsl::toColor() const
    {
        if (luminosity <= 0.0f)
        {
            return { 0u, 0u, 0u };
        }

        if (luminosity >= 1.0f)
        {
            return { 255u, 255u, 255u };
        }

        const float lightness = Okhsl::toeInverse(luminosity);

        if (saturation <= 0.0f)
        {
            // A grey drives all three cones equally, so the one value they share is its
            // lightness cubed.
            const ColorByte gray = static_cast<ColorByte>(std::round(
                Okhsl::srgbEncode(lightness * lightness * lightness) * 255.0f));
            return { gray, gray, gray };
        }

        const float a = std::cos(k_2Pi * hue);
        const float b = std::sin(k_2Pi * hue);

        const Okhsl::ChromaCurve curve = Okhsl::chromaCurveOf(lightness, a, b);

        float chroma = 0.0f;
        if (saturation < Okhsl::k_kneeSaturation)
        {
            const float along = saturation / Okhsl::k_kneeSaturation;
            const float scale = Okhsl::k_kneeSaturation * curve.start;
            const float bend = 1.0f - scale / curve.knee;
            chroma = along * scale / (1.0f - bend * along);
        }
        else
        {
            const float along = (saturation - Okhsl::k_kneeSaturation)
                / (1.0f - Okhsl::k_kneeSaturation);
            const float scale = (1.0f - Okhsl::k_kneeSaturation) * curve.knee * curve.knee
                / (Okhsl::k_kneeSaturation * Okhsl::k_kneeSaturation * curve.start);
            const float bend = 1.0f - scale / (curve.gamut - curve.knee);
            chroma = curve.knee + along * scale / (1.0f - bend * along);
        }

        const Okhsl::LinearRgb rgb = Okhsl::linearRgbFromOklab({ lightness, chroma * a, chroma * b });

        // The cusp and the boundary through it are both fits, so a colour asked for at the
        // very edge of the gamut can land a fraction of a byte outside it.
        return {
            static_cast<ColorByte>(std::round(Okhsl::srgbEncode(std::clamp(rgb.red, 0.0f, 1.0f)) * 255.0f)),
            static_cast<ColorByte>(std::round(Okhsl::srgbEncode(std::clamp(rgb.green, 0.0f, 1.0f)) * 255.0f)),
            static_cast<ColorByte>(std::round(Okhsl::srgbEncode(std::clamp(rgb.blue, 0.0f, 1.0f)) * 255.0f)),
        };
    }

    void Hsl::offsetHue(float value)
    {
        hue += value;
        normalizeValue(hue);
    }

    void Hsl::scaleSaturation(float k)
    {
        saturation *= k;
        clampValue(saturation);
    }

    void Hsl::scaleLuminosity(float k)
    {
        luminosity *= k;
        clampValue(luminosity);
    }

    void Hsl::normalizeValue(float& value)
    {
        while (value < 0.0f)
        {
            value += 1.0f;
        }

        while (value >= 1.0f)
        {
            value -= 1.0f;
        }
    }

    void Hsl::clampValue(float& value)
    {
        value = std::clamp(value, 0.0f, 1.0f);
    }
}
