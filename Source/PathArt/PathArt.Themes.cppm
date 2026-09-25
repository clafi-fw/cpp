export module ClaFi.PathArt.Themes;

import ClaFi.PathArt.Types;
import ClaFi.StdLib;

namespace ClaFi::PathArt::Themes {

    export struct FreshSpring : SceneTheme {
        constexpr FreshSpring() : SceneTheme{
            .name = L"Fresh Spring",
            .skyTop = 0xFF78C8FF,
            .skyBottom = 0xFFB4F0FF,
            .sunDisk = 0xFFFFDCA8,
            .sunStroke = 0xFFE6D25A,
            .sunCorona1 = 0xB4FFF5B4,
            .sunCorona2 = 0x64FFE664,
            .sunGlow = 0x64FFF078,
            .cloud = 0xCCFFFFFF,
            .hillTop = 0xFF78DC64,
            .hillBottom = 0xFF3CA03C,
            .riverTop = 0xFF46B4DC,
            .riverBottom = 0xFF286DA0,
            .riverWave = 0xB4FFFFFF,
            .riverReflect = 0x64FFDCA8,
            .foam = 0xB4DCF5FF,
            .millBodyTop = 0xFFE6A064,
            .millBodyBottom = 0xFFA17046,
            .millRoofTop = 0xFFDC5046,
            .millRoofBottom = 0xFF9A3831,
            .millWingWood = 0xFFA0785A,
            .millWingWoodStroke = 0xFF70543F,
            .millWingFabric = 0xFFFAFAFA,
            .millWingFabricStroke = 0xFFAFAFAF,
            .millWheel = 0xFFA0785A,
            .treeTrunk = 0xFF785028,
            .treeLeavesBase = 0xFF2D9036,
            .reedStem = 0xFF648C3C,
            .reedHead = 0xFF46622A,
            .lilyPad = 0xFF50B464,
            .lilyFlower = 0xFFFFB4DC,
            .shadow = 0xFF000000,
            .cowBody = 0xFFFFFFFF,
            .cowSpots = 0xFF323232,
            .cowSnout = 0xFFF0C0CB,
            .cowDetail = 0xFF282828,
            .wolfBody = 0xFF323237,
            .wolfEar = 0xFF1E1E23,
            .wolfEye = 0xFFFFFF00,
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct HighNoon : SceneTheme {
        constexpr HighNoon() : SceneTheme{
            .name = L"High Noon",
            .skyTop = 0xFF1E90FF,
            .skyBottom = 0xFFADD8E6,
            .sunDisk = 0xFFFFFD59,
            .sunStroke = 0xFFFFD700,
            .sunCorona1 = 0xFFFFEA8F,
            .sunCorona2 = 0xFFFFFAA5,
            .sunGlow = 0xFFFFFBF0,
            .cloud = 0xCCFAFAFA,
            .hillTop = 0xFF6B8E23,
            .hillBottom = 0xFF90EE90,
            .riverTop = 0xFF00BFFF,
            .riverBottom = 0xFF1E90FF,
            .riverWave = 0xBBFFFFFE,
            .riverReflect = 0xFFE6E6FA,
            .foam = 0xBBFFFFFE,
            .millBodyTop = 0xFFA0522D,
            .millBodyBottom = 0xFF8B4513,
            .millRoofTop = 0xFF8B0000,
            .millRoofBottom = 0xFFB22222,
            .millWingWood = 0xFFDEB887,
            .millWingWoodStroke = 0xFFD2B48C,
            .millWingFabric = 0x886A5ACD,
            .millWingFabricStroke = 0x887B68EE,
            .millWheel = 0xFF2F4F4F,
            .treeTrunk = 0xFFA52A2A,
            .treeLeavesBase = 0xFF228B22,
            .reedStem = 0xFF8B4513,
            .reedHead = 0xFFC19A6B,
            .lilyPad = 0xFF32CD32,
            .lilyFlower = 0xFFFFA07A,
            .shadow = 0xFF000000,
            .cowBody = 0xFFFFFFFE,
            .cowSpots = 0xFF000000,
            .cowSnout = 0xFFCD853F,
            .cowDetail = 0xFF8B4513,
            .wolfBody = 0xFF4B3621,
            .wolfEar = 0xFF696969,
            .wolfEye = 0xFFFDCBC0,
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct CherryBlossom : SceneTheme {
        constexpr CherryBlossom() : SceneTheme{
            .name = L"Cherry Blossom",
            .skyTop = 0xFF1E90FF,        // Light blue (top sky)
            .skyBottom = 0xFF6495ED,     // Deeper blue (bottom sky)
            .sunDisk = 0xFFFFD700,       // Bright yellow (sun center)
            .sunStroke = 0xFFFFEBCD,     // Light yellow outline
            .sunCorona1 = 0xFFFFA07A,    // Orange corona layer 1
            .sunCorona2 = 0xFFFFDAB9,    // Light orange corona layer 2
            .sunGlow = 0xFFFFFAFA,       // White glow around sun
            .cloud = 0xCCFAFAFA,         // Off-white clouds
            .hillTop = 0xFF7CFC00,       // Light green (hill top)
            .hillBottom = 0xFF556B2F,    // Darker green (hill base)
            .riverTop = 0xFF002166,      // Deep blue (river top)
            .riverBottom = 0xFF4682B4,   // Lighter blue (river bottom)
            .riverWave = 0xFFFFC8DC,     // Same as Effect1
            .riverReflect = 0xFFB0E0E6,  // Light cyan (water reflection)
            .foam = 0xBBFFFFFE,          // White foam
            .millBodyTop = 0xFFA0522D,   // Light brown (top mill body)
            .millBodyBottom = 0xFF8B4513, // Dark brown (bottom mill body)
            .millRoofTop = 0xFF8B0000,   // Dark red (roof top)
            .millRoofBottom = 0xFFCD5C5C, // Light red (roof bottom)
            .millWingWood = 0xFFD2B48C,  // Light tan (wood part of wing)
            .millWingWoodStroke = 0xFFA0522D, // Brown stroke for wood
            .millWingFabric = 0x88E6E6FA, // Light grey (fabric of wing)
            .millWingFabricStroke = 0x8887CEFA, // Sky blue stroke for fabric
            .millWheel = 0xFF2F4F4F,     // Dark grey (mill wheel)
            .treeTrunk = 0xFF8B4513,     // Brown tree trunk
            .treeLeavesBase = 0xFFF8BBD0, // Light pink (tree leaves base — cherry blossom style)
            .reedStem = 0xFF8B4513,      // Brown reed stem
            .reedHead = 0xFFCDB5CD,      // Light brown reed head
            .lilyPad = 0xFF6DC511,       // Bright green lily pad
            .lilyFlower = 0xFFFFA07A,    // Orange lily flower
            .shadow = 0xFF000000,        // Shadow
            .cowBody = 0xFFFFFFFE,       // White cow body
            .cowSpots = 0xFF000000,      // Black spots
            .cowSnout = 0xFFCD853F,      // Light brown snout
            .cowDetail = 0xFF8B4513,     // Brown detail (hooves, etc.)
            .wolfBody = 0xFF4B3621,      // Dark grey-brown wolf body
            .wolfEar = 0xFF696969,       // Medium grey ears
            .wolfEye = 0xFFFF0000,       // Red glowing eyes
            .effectColor1 = 0xFFFFC8DC,
            .effectColor2 = 0xFFFF96B4,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = true,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct GoldenHour : SceneTheme {
        constexpr GoldenHour() : SceneTheme{
            .name = L"Golden Hour",
            .skyTop = 0xFFFFB478,
            .skyBottom = 0xFFFFDCAA,
            .sunDisk = 0xFFFFDC8C,
            .sunStroke = 0xFFE6BE78,
            .sunCorona1 = 0xB4FFE696,
            .sunCorona2 = 0x64FFC864,
            .sunGlow = 0x64FFDC8C,
            .cloud = 0xCCFFF0DC,
            .hillTop = 0xFFB4C85A,
            .hillBottom = 0xFF78963C,
            .riverTop = 0xFF508CB4,
            .riverBottom = 0xFF326482,
            .riverWave = 0xB4FFFFFF,
            .riverReflect = 0x64FFDC8C,
            .foam = 0xB4E6F0FF,
            .millBodyTop = 0xFFD28C5A,
            .millBodyBottom = 0xFF93623F,
            .millRoofTop = 0xFFB4463C,
            .millRoofBottom = 0xFF7E312A,
            .millWingWood = 0xFF8C6446,
            .millWingWoodStroke = 0xFF624631,
            .millWingFabric = 0xFFF0DCC8,
            .millWingFabricStroke = 0xFFA89A8C,
            .millWheel = 0xFF8C6446,
            .treeTrunk = 0xFF6E4628,
            .treeLeavesBase = 0xFF78A046,
            .reedStem = 0xFF8C6432,
            .reedHead = 0xFF624623,
            .lilyPad = 0xFF648C3C,
            .lilyFlower = 0xFFFFD2B4,
            .shadow = 0x78321E00,
            .cowBody = 0xFFFFF5EB,
            .cowSpots = 0xFF5A3C28,
            .cowSnout = 0xFFFFC8B4,
            .cowDetail = 0xFF3C2814,
            .wolfBody = 0xFF463228,
            .wolfEar = 0xFF321E14,
            .wolfEye = 0xFFFFFF00,
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct SummerSunset : SceneTheme {
        constexpr SummerSunset() : SceneTheme{
            .name = L"Summer Sunset",
            .skyTop = 0xFF7E9FD4,
            .skyBottom = 0xFFF2DED4,
            .sunDisk = 0xFFFFFBD9,
            .sunStroke = 0xFFF5E6B3,
            .sunCorona1 = 0x80FDE2A7,
            .sunCorona2 = 0x60F8C8A0,
            .sunGlow = 0xFFF9B494,
            .cloud = 0xCCE5EBF2,
            .hillTop = 0xFF497E55,
            .hillBottom = 0xFF3B6645,
            .riverTop = 0xFF96ABC1,
            .riverBottom = 0xFF7D8FA1,
            .riverWave = 0x64405060,
            .riverReflect = 0x78F5E1D5,
            .foam = 0x96F0F0F0,
            .millBodyTop = 0xFF604239,
            .millBodyBottom = 0xFF4A342E,
            .millRoofTop = 0xFF964B4B,
            .millRoofBottom = 0xFF7D3C3C,
            .millWingWood = 0xFF433F3C,
            .millWingWoodStroke = 0xFF2D1F1A,
            .millWingFabric = 0x96BCAAA4,
            .millWingFabricStroke = 0xFF523D35,
            .millWheel = 0xFF4A342E,
            .treeTrunk = 0xFF402E28,
            .treeLeavesBase = 0xFF2F4F35,
            .reedStem = 0xFF212B23,
            .reedHead = 0xFF4A342E,
            .lilyPad = 0xFF2F4F35,
            .lilyFlower = 0xFF5D2D5D,
            .shadow = 0x80000000,
            .cowBody = 0xFFEEEEEE,
            .cowSpots = 0xFF2D2D2D,
            .cowSnout = 0xFFFFB6C1,
            .cowDetail = 0xFF000000,
            .wolfBody = 0xFF263238,
            .wolfEar = 0xFF101010,
            .wolfEye = 0xFFFF9800,
            .effectColor1 = 0xFFF2DED4,
            .effectColor2 = 0xFFA6B7CC,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct WarmOrangeFall : SceneTheme {
        constexpr WarmOrangeFall() : SceneTheme{
            .name = L"Warm Orange Fall",
            .skyTop = 0xFF814833,
            .skyBottom = 0xFFCE7F48,
            .sunDisk = 0xFFFFE5B4,
            .sunStroke = 0xFFFFB347,
            .sunCorona1 = 0x80E67E22,
            .sunCorona2 = 0x50D35400,
            .sunGlow = 0x70A04000,
            .cloud = 0xCCE0B0A0,
            .hillTop = 0xFF2D4F1E,
            .hillBottom = 0xFF1B3012,
            .riverTop = 0xFF4A2B4D,
            .riverBottom = 0xFF2D1A2E,
            .riverWave = 0x64CA9480,
            .riverReflect = 0x78D35400,
            .foam = 0x96DCDCDC,
            .millBodyTop = 0xFF5D4037,
            .millBodyBottom = 0xFF3E2723,
            .millRoofTop = 0xFF8D3B2A,
            .millRoofBottom = 0xFF5D2419,
            .millWingWood = 0xFF4E342E,
            .millWingWoodStroke = 0xFF2D1F1E,
            .millWingFabric = 0x96A1887F,
            .millWingFabricStroke = 0xFF4E342E,
            .millWheel = 0xFF3E2723,
            .treeTrunk = 0xFF3E2723,
            .treeLeavesBase = 0xFFB85E3D,
            .reedStem = 0xFF5D4037,
            .reedHead = 0xFF3E2723,
            .lilyPad = 0xFF344E2C,
            .lilyFlower = 0xFFB03060,
            .shadow = 0x96000000,
            .cowBody = 0xFFE0D7C6,
            .cowSpots = 0xFF4E342E,
            .cowSnout = 0xFFE5A1A1,
            .cowDetail = 0xFF1A1A1A,
            .wolfBody = 0xFF2C2C2E,
            .wolfEar = 0xFF1C1C1E,
            .wolfEye = 0xFFFFD700,
            .effectColor1 = 0xFFB85E3D,
            .effectColor2 = 0xFFA04000,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = true,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct SaharaHeat : SceneTheme {
        constexpr SaharaHeat() : SceneTheme{
            .name = L"Sahara Heat",
            .skyTop = 0xFFFF8C00,
            .skyBottom = 0xFFFFDC64,
            .sunDisk = 0xFFFFFF96,
            .sunStroke = 0xFFFFB432,
            .sunCorona1 = 0x96FFDC64,
            .sunCorona2 = 0x64FF9632,
            .sunGlow = 0xFF'FFDC64,
            .cloud = 0xCCFFDCB4,
            .hillTop = 0xFFD2B478,
            .hillBottom = 0xFFA08250,
            .riverTop = 0xFF648CB4,
            .riverBottom = 0xFF3C648C,
            .riverWave = 0x64FFFFFF,
            .riverReflect = 0x50FFDC64,
            .foam = 0x78DCE6FF,
            .millBodyTop = 0xFFB47850,
            .millBodyBottom = 0xFF8C5A3C,
            .millRoofTop = 0xFFA03C32,
            .millRoofBottom = 0xFF6E281E,
            .millWingWood = 0xFF8C6446,
            .millWingWoodStroke = 0xFF6E4632,
            .millWingFabric = 0xFFF0DCC8,
            .millWingFabricStroke = 0xFFA08C78,
            .millWheel = 0xFF8C6446,
            .treeTrunk = 0xFF644628,
            .treeLeavesBase = 0xFF8C783C,
            .reedStem = 0xFF788C50,
            .reedHead = 0xFF5A6E3C,
            .lilyPad = 0xFF78A064,
            .lilyFlower = 0xFFF0E6C8,
            .shadow = 0x78502800,
            .cowBody = 0xFFF0E6C8,
            .cowSpots = 0xFF644632,
            .cowSnout = 0xFFF0D2C8,
            .cowDetail = 0xFF463228,
            .wolfBody = 0xFF50463C,
            .wolfEar = 0xFF3C3228,
            .wolfEye = 0xFFFFFF00,
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = true,
            .isAfrica = true
        } {
        }
    };

    export struct Midnight : SceneTheme {
        constexpr Midnight() : SceneTheme{
            .name = L"Midnight",
            .skyTop = 0xFF050A1E,
            .skyBottom = 0xFF141E46,
            .sunDisk = 0xFFE6E6FA,
            .sunStroke = 0xFF9696C8,
            .sunCorona1 = 0x64C8C8FF,
            .sunCorona2 = 0x326464C8,
            .sunGlow = 0x789696FF,
            .cloud = 0xCC323C64,
            .hillTop = 0xFF051E05,
            .hillBottom = 0xFF020F02,
            .riverTop = 0xFF050F28,
            .riverBottom = 0xFF020514,
            .riverWave = 0x786496FF,
            .riverReflect = 0x3C000000,
            .foam = 0x786482B4,
            .millBodyTop = 0xFF281E3C,
            .millBodyBottom = 0xFF140F1E,
            .millRoofTop = 0xFF321428,
            .millRoofBottom = 0xFF1E0A14,
            .millWingWood = 0xFF1E1932,
            .millWingWoodStroke = 0xFF141428,
            .millWingFabric = 0x645064C8,
            .millWingFabricStroke = 0xFF1E1932,
            .millWheel = 0xFF19142D,
            .treeTrunk = 0xFF140F1E,
            .treeLeavesBase = 0xFF05190A,
            .reedStem = 0xFF0A1E0F,
            .reedHead = 0xFF28141E,
            .lilyPad = 0xFF0A2814,
            .lilyFlower = 0xFFB464FF,
            .shadow = 0xC8000000,
            .cowBody = 0xFFB4B4DC,
            .cowSpots = 0xFF0A0A1E,
            .cowSnout = 0xFFC896C8,
            .cowDetail = 0xFF000014,
            .wolfBody = 0xFF0A0A0F,
            .wolfEar = 0xFF05050A,
            .wolfEye = 0xFF00FFFF,
            .effectColor1 = 0xFFFFFFB4,
            .effectColor2 = 0x64FFFF64,
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct CalmNight : SceneTheme {
        constexpr CalmNight() : SceneTheme{
            .name = L"Calm Night",
            .skyTop = 0xFF0A1428,
            .skyBottom = 0xFF1E3250,
            .sunDisk = 0xFFC8D2FF,
            .sunStroke = 0xFF96A0C8,
            .sunCorona1 = 0x50DCE6FF,
            .sunCorona2 = 0x32B4C8FF,
            .sunGlow = 0x64C8D2FF,
            .cloud = 0xCC50648C,
            .hillTop = 0xFF28463C,
            .hillBottom = 0xFF142828,
            .riverTop = 0xFF143246,
            .riverBottom = 0xFF0A1E2D,
            .riverWave = 0x6496B4FF,
            .riverReflect = 0x3CC8D2FF,
            .foam = 0x786482B4,
            .millBodyTop = 0xFF505A6E,
            .millBodyBottom = 0xFF383F4D,
            .millRoofTop = 0xFF3C3246,
            .millRoofBottom = 0xFF2A2331,
            .millWingWood = 0xFF5A6478,
            .millWingWoodStroke = 0xFF3F4654,
            .millWingFabric = 0xFFB4C8DC,
            .millWingFabricStroke = 0xFF7E8C9A,
            .millWheel = 0xFF5A6478,
            .treeTrunk = 0xFF3C3232,
            .treeLeavesBase = 0xFF325046,
            .reedStem = 0xFF46503C,
            .reedHead = 0xFF31382A,
            .lilyPad = 0xFF284632,
            .lilyFlower = 0xFFB48CFF,
            .shadow = 0xFF000000,
            .cowBody = 0xFFC8D2DC,
            .cowSpots = 0xFF1E1E28,
            .cowSnout = 0xFFB4A0A5,
            .cowDetail = 0xFF141419,
            .wolfBody = 0xFF0F0F14,
            .wolfEar = 0xFF0A0A0F,
            .wolfEye = 0xFF00FFFF,
            .effectColor1 = 0xFFFFFFB4,
            .effectColor2 = 0x64FFFF64,
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct StormyNight : SceneTheme {
        constexpr StormyNight() : SceneTheme{
            .name = L"Stormy Night",
            .skyTop = 0xFF0A1428,
            .skyBottom = 0xFF1E3250,
            .sunDisk = 0xFFC8D2FF,
            .sunStroke = 0xFF96A0C8,
            .sunCorona1 = 0x50DCE6FF,
            .sunCorona2 = 0x32B4C8FF,
            .sunGlow = 0x64C8D2FF,
            .cloud = 0xCC50648C,
            .hillTop = 0xFF28463C,
            .hillBottom = 0xFF142828,
            .riverTop = 0xFF143246,
            .riverBottom = 0xFF0A1E2D,
            .riverWave = 0x6496B4FF,
            .riverReflect = 0x3CC8D2FF,
            .foam = 0x786482B4,
            .millBodyTop = 0xFF505A6E,
            .millBodyBottom = 0xFF383F4D,
            .millRoofTop = 0xFF3C3246,
            .millRoofBottom = 0xFF2A2331,
            .millWingWood = 0xFF5A6478,
            .millWingWoodStroke = 0xFF3F4654,
            .millWingFabric = 0xFFB4C8DC,
            .millWingFabricStroke = 0xFF7E8C9A,
            .millWheel = 0xFF5A6478,
            .treeTrunk = 0xFF3C3232,
            .treeLeavesBase = 0xFF325046,
            .reedStem = 0xFF46503C,
            .reedHead = 0xFF31382A,
            .lilyPad = 0xFF284632,
            .lilyFlower = 0xFFB48CFF,
            .shadow = 0xFF000000,
            .cowBody = 0xFFC8D2DC,
            .cowSpots = 0xFF1E1E28,
            .cowSnout = 0xFFB4A0A5,
            .cowDetail = 0xFF141419,
            .wolfBody = 0xFF0F0F14,
            .wolfEar = 0xFF0A0A0F,
            .wolfEye = 0xFF00FFFF,
            .effectColor1 = 0x78B4C8FF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = true,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct Twilight : SceneTheme {
        constexpr Twilight() : SceneTheme{
            .name = L"Twilight",
            .skyTop = 0xFF50468C,
            .skyBottom = 0xFF8C78C8,
            .sunDisk = 0xFFFFC8DC,
            .sunStroke = 0xFFC8A0B4,
            .sunCorona1 = 0x84FFD2E6,
            .sunCorona2 = 0xA4AB9FD1,
            .sunGlow = 0xCC'C8A0DC,
            .cloud = 0xCCC8BEE6,
            .hillTop = 0xFF64788C,
            .hillBottom = 0xFF3C506E,
            .riverTop = 0xFF465A8C,
            .riverBottom = 0xFF28375A,
            .riverWave = 0xB4C8D2FF,
            .riverReflect = 0x50FFC8DC,
            .foam = 0xB4B4BEE6,
            .millBodyTop = 0xFF8C6E82,
            .millBodyBottom = 0xFF624D5B,
            .millRoofTop = 0xFF6E4664,
            .millRoofBottom = 0xFF4D3146,
            .millWingWood = 0xFF645078,
            .millWingWoodStroke = 0xFF463854,
            .millWingFabric = 0xFFDCD2F0,
            .millWingFabricStroke = 0xFF9A93A8,
            .millWheel = 0xFF645078,
            .treeTrunk = 0xFF503C46,
            .treeLeavesBase = 0xFF5A6E82,
            .reedStem = 0xFF5A506E,
            .reedHead = 0xFF3F384D,
            .lilyPad = 0xFF466478,
            .lilyFlower = 0xFFE6B4FF,
            .shadow = 0xFF000000,
            .cowBody = 0xFFE6E6FA,
            .cowSpots = 0xFF504664,
            .cowSnout = 0xFFF0C8DC,
            .cowDetail = 0xFF281E32,
            .wolfBody = 0xFF28233C,
            .wolfEar = 0xFF14141E,
            .wolfEye = 0xFFFF6400,
            .effectColor1 = 0xFFFFC8DC,
            .effectColor2 = 0xFFFFC8DC,
            .hasRain = false,
            .hasFireflies = true, // Effect: slowly falling glowing particles (magical sparks or rare meteors)
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct PaperSketch : SceneTheme {
        constexpr PaperSketch() : SceneTheme{
            .name = L"Paper Sketch",
            .skyTop = 0xFFF5F0E1,
            .skyBottom = 0xFFEBE6D7,
            .sunDisk = 0xFFFFFFFF,
            .sunStroke = 0xFFB4AA96,
            .sunCorona1 = 0x64B4AA96,
            .sunCorona2 = 0x4EB4AA96,
            .sunGlow = 0x28B4AA96,
            .cloud = 0xCCFFFFFF,
            .hillTop = 0xFFA0B4A0,
            .hillBottom = 0xFF8CA08C,
            .riverTop = 0xFFB4C8D2,
            .riverBottom = 0xFFA0B4BE,
            .riverWave = 0xC8FFFFFF,
            .riverReflect = 0x64FFFFFF,
            .foam = 0xBBFFFFFF,
            .millBodyTop = 0xFF8C6E5A,
            .millBodyBottom = 0xFF785A46,
            .millRoofTop = 0xFFB4836E,
            .millRoofBottom = 0xFF966450,
            .millWingWood = 0xFF5A5046,
            .millWingWoodStroke = 0xFF463C32,
            .millWingFabric = 0xFFF0DCD2,
            .millWingFabricStroke = 0xFF5A5046,
            .millWheel = 0xFF50463C,
            .treeTrunk = 0xFF5A5046,
            .treeLeavesBase = 0xFF507850,
            .reedStem = 0xFF506E50,
            .reedHead = 0xFF645046,
            .lilyPad = 0xFF648264,
            .lilyFlower = 0xFFF0B4B4,
            .shadow = 0x3C000000,
            .cowBody = 0xFFFAFAFA,
            .cowSpots = 0xFF645A50,
            .cowSnout = 0xFFF0D2D2,
            .cowDetail = 0xFF3C3228,
            .wolfBody = 0xFF3C3C41,
            .wolfEar = 0xFF323237,
            .wolfEye = 0xFFE9E411,
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct WinterFrost : SceneTheme {
        constexpr WinterFrost() : SceneTheme{
            .name = L"Winter Frost",
            .skyTop = 0xFFA0BEE0,
            .skyBottom = 0xFFD2E6FA,
            .sunDisk = 0xFFF0F0FF,
            .sunStroke = 0xFFB4C8E0,
            .sunCorona1 = 0x64FFFFFF,
            .sunCorona2 = 0x32C8E6FF,
            .sunGlow = 0x28FFFFFF,
            .cloud = 0xCCFFFFFF,
            .hillTop = 0xFFF0F5FF,
            .hillBottom = 0xFFB4C8E0,
            .riverTop = 0xFF96B4D2,
            .riverBottom = 0xFF6482A0,
            .riverWave = 0xC8FFFFFF,
            .riverReflect = 0x64FFFFFF,
            .foam = 0xB4F0FAFF,
            .millBodyTop = 0xFF8C6E5A,
            .millBodyBottom = 0xFF64503C,
            .millRoofTop = 0xFF783232,
            .millRoofBottom = 0xFF501E1E,
            .millWingWood = 0xFF64503C,
            .millWingWoodStroke = 0xFF503C28,
            .millWingFabric = 0xFFE6E6F0,
            .millWingFabricStroke = 0xFF9696A0,
            .millWheel = 0xFF64503C,
            .treeTrunk = 0xFF64503C,
            .treeLeavesBase = 0xFFCDD2D7,
            .reedStem = 0xFF64788C,
            .reedHead = 0xFF465A6E,
            .lilyPad = 0xFFC8DCFF,
            .lilyFlower = 0xFFFFFFFF,
            .shadow = 0x3C000000,
            .cowBody = 0xFFFFFFFF,
            .cowSpots = 0xFFC8C8DC,
            .cowSnout = 0xFFF0DCDC,
            .cowDetail = 0xFF646478,
            .wolfBody = 0xFF282D3C,
            .wolfEar = 0xFF1E2332,
            .wolfEye = 0xFF00FFFF,
            .effectColor1 = 0xDCFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = true,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct WinterNight : SceneTheme {
        constexpr WinterNight() : SceneTheme{
            .name = L"Winter Night",
            .skyTop = 0xFF050A1E,
            .skyBottom = 0xFF141E3C,
            .sunDisk = 0xFFE6F0FF,
            .sunStroke = 0xFF96AAC8,
            .sunCorona1 = 0x50C8DCFF,
            .sunCorona2 = 0x286478C8,
            .sunGlow = 0x6696B4FF,
            .cloud = 0x28FFFFFF,
            .hillTop = 0xFFB4C8E6,
            .hillBottom = 0xFF647896,
            .riverTop = 0xFF0F2846,
            .riverBottom = 0xFF050F1E,
            .riverWave = 0x64C8DCFF,
            .riverReflect = 0x3CE6F0FF,
            .foam = 0x786482B4,
            .millBodyTop = 0xFF3C3246,
            .millBodyBottom = 0xFF1E1923,
            .millRoofTop = 0xFF32283C,
            .millRoofBottom = 0xFF19141E,
            .millWingWood = 0xFF465064,
            .millWingWoodStroke = 0xFF323C50,
            .millWingFabric = 0xFFDCE6F0,
            .millWingFabricStroke = 0xFF96A0AA,
            .millWheel = 0xFF465064,
            .treeTrunk = 0xFF322832,
            .treeLeavesBase = 0xFFB4C8DC,
            .reedStem = 0xFF46503C,
            .reedHead = 0xFF283223,
            .lilyPad = 0xFF143246,
            .lilyFlower = 0xFFB48CFF,
            .shadow = 0xC8000014,
            .cowBody = 0xFFC8D2DC,
            .cowSpots = 0xFF1E1E28,
            .cowSnout = 0xFFB4A0A5,
            .cowDetail = 0xFF141419,
            .wolfBody = 0xFF0F0F19,
            .wolfEar = 0xFF0A0A14,
            .wolfEye = 0xFF00FFFF,
            .effectColor1 = 0xDCFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = true,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct ToxicWaste : SceneTheme {
        constexpr ToxicWaste() : SceneTheme{
            .name = L"Toxic Waste",
            .skyTop = 0xFF0A1E0A,
            .skyBottom = 0xFF285028,
            .sunDisk = 0xFFB4FF64,
            .sunStroke = 0xFF64C832,
            .sunCorona1 = 0x6496FF32,
            .sunCorona2 = 0x3250B41E,
            .sunGlow = 0x32B4FF64,
            .cloud = 0xCC3C643C,
            .hillTop = 0xFF143214,
            .hillBottom = 0xFF0A1E0A,
            .riverTop = 0xFF1E3C1E,
            .riverBottom = 0xFF0F1E0F,
            .riverWave = 0x7864FF64,
            .riverReflect = 0x3CC8FF64,
            .foam = 0x9678B478,
            .millBodyTop = 0xFF50463C,
            .millBodyBottom = 0xFF322D28,
            .millRoofTop = 0xFF643232,
            .millRoofBottom = 0xFF461E1E,
            .millWingWood = 0xFF5A5046,
            .millWingWoodStroke = 0xFF3C3228,
            .millWingFabric = 0xFFB4DCB4,
            .millWingFabricStroke = 0xFF647864,
            .millWheel = 0xFF5A5046,
            .treeTrunk = 0xFF3C3228,
            .treeLeavesBase = 0xFF326432,
            .reedStem = 0xFF465A3C,
            .reedHead = 0xFF283C1E,
            .lilyPad = 0xFF285A3C,
            .lilyFlower = 0xFF96FF96,
            .shadow = 0xC8000000,
            .cowBody = 0xFFB4B4B4,
            .cowSpots = 0xFF282828,
            .cowSnout = 0xFFC86464,
            .cowDetail = 0xFF141414,
            .wolfBody = 0xFF0A0A0A,
            .wolfEar = 0xFF050505,
            .wolfEye = 0xFF64FF00,
            .effectColor1 = 0xB464FF00,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = true,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = false
        } {
        }
    };

    export struct VintageNoir : SceneTheme {
        constexpr VintageNoir() : SceneTheme{
            .name = L"Vintage Noir",
            .skyTop = 0xFF282828,
            .skyBottom = 0xFF787878,
            .sunDisk = 0xFFDCDCDC,
            .sunStroke = 0xFF969696,
            .sunCorona1 = 0x66'C8C8C8,
            .sunCorona2 = 0x66'969696,
            .sunGlow = 0x66'C8C8C8,
            .cloud = 0xCCB4B4B4,
            .hillTop = 0xFF505050,
            .hillBottom = 0xFF282828,
            .riverTop = 0xFF3C3C3C,
            .riverBottom = 0xFF1E1E1E,
            .riverWave = 0xB4FFFFFF,
            .riverReflect = 0x64C8C8C8,
            .foam = 0x96B4B4B4,
            .millBodyTop = 0xFF5A5A5A,
            .millBodyBottom = 0xFF3C3C3C,
            .millRoofTop = 0xFF6E6E6E,
            .millRoofBottom = 0xFF464646,
            .millWingWood = 0xFF646464,
            .millWingWoodStroke = 0xFF464646,
            .millWingFabric = 0xFFE6E6E6,
            .millWingFabricStroke = 0xFF969696,
            .millWheel = 0xFF646464,
            .treeTrunk = 0xFF464646,
            .treeLeavesBase = 0xFF5A5A5A,
            .reedStem = 0xFF646464,
            .reedHead = 0xFF464646,
            .lilyPad = 0xFF505050,
            .lilyFlower = 0xFFDCDCDC,
            .shadow = 0xB4000000,
            .cowBody = 0xFFF0F0F0,
            .cowSpots = 0xFF282828,
            .cowSnout = 0xFFC8C8C8,
            .cowDetail = 0xFF141414,
            .wolfBody = 0xFF1E1E1E,
            .wolfEar = 0xFF141414,
            .wolfEye = 0xFFFFFFFF,
            .effectColor1 = 0x44FFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = true, // wind effect
            .isAfrica = false
        } {
        }
    };

    export struct MidnightFantasy : SceneTheme {
        constexpr MidnightFantasy() : SceneTheme{
            .name = L"Midnight Fantasy",
            .skyTop = 0xFF000033,        // Deep dark blue (almost black)
            .skyBottom = 0xFF1A1A50,     // Dark purple
            .sunDisk = 0xFFFFF0FF,       // Pale lilac (moon)
            .sunStroke = 0xFFE6E6FA,     // Light lilac outline
            .sunCorona1 = 0xFFDDA0DD,    // Lilac corona
            .sunCorona2 = 0xFFB569B5,    // Dark lilac corona
            .sunGlow = 0xFF8A2BE2,       // Purple glow
            .cloud = 0xCC4B0082,         // Dark purple clouds
            .hillTop = 0xFF2F4F4F,       // Dark green (fir tree color)
            .hillBottom = 0xFF0A3A3A,    // Very dark green
            .riverTop = 0xFF001A33,      // Deep blue
            .riverBottom = 0xFF000F1A,   // Almost black
            .riverWave = 0xFF6699CC,     // Turquoise waves
            .riverReflect = 0xFF99CCFF,  // Light moon reflection
            .foam = 0xBBFFFFFF,          // White foam
            .millBodyTop = 0xFF3D2B17,   // Dark chocolate
            .millBodyBottom = 0xFF5D4037, // Brown-red
            .millRoofTop = 0xFF4E342E,   // Dark brown
            .millRoofBottom = 0xFF6D4E44, // Burgundy
            .millWingWood = 0xFF795548,  // Medium brown
            .millWingWoodStroke = 0xFF5D4037,
            .millWingFabric = 0x88512DA8, // Purple sail
            .millWingFabricStroke = 0x88311B6B,
            .millWheel = 0xFF212121,     // Black
            .treeTrunk = 0xFF4E342E,     // Dark brown
            .treeLeavesBase = 0xFF263238, // Dark turquoise
            .reedStem = 0xFF3E2729,      // Burgundy-brown
            .reedHead = 0xFF727272,      // Gray
            .lilyPad = 0xFF1B5E5B,       // Dark green
            .lilyFlower = 0xFFE1BEE7,    // Pale purple flower
            .shadow = 0xFF000022,        // Very dark shadow
            .cowBody = 0xFFFFFEF0,       // Cream (not quite white)
            .cowSpots = 0xFF333366,      // Dark blue instead of black
            .cowSnout = 0xFFA18876,      // Lilac snout
            .cowDetail = 0xFF755D52,     // Warm brown
            .wolfBody = 0xFF5D4037,      // Chocolate (not scary)
            .wolfEar = 0xFF795548,       // Medium brown
            .wolfEye = 0xFFFFFF00,       // Yellow glowing eyes (not red)
            .effectColor1 = 0xFFD2B48C,  // Pale lavender-white for a magical, ethereal glow
            .effectColor2 = 0xFFFFF0FF,  // Very light lilac, almost white (secondary glow)
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct PastelDream : SceneTheme {
        constexpr PastelDream() : SceneTheme{
            .name = L"Pastel Dream",
            .skyTop = 0xFFC6E2FF,        // Pastel blue
            .skyBottom = 0xFFE0F7FF,     // Very light blue
            .sunDisk = 0xFFFFF599,       // Pastel yellow
            .sunStroke = 0xFFFFE699,     // Light orange stroke
            .sunCorona1 = 0xFFFFD699,
            .sunCorona2 = 0xFFFFC699,
            .sunGlow = 0xFFFFFBF0,       // Soft white glow
            .cloud = 0xCCFAFAFA,         // White clouds
            .hillTop = 0xFFA8E67F,       // Pastel green
            .hillBottom = 0xFFC8E6A0,    // Light lime
            .riverTop = 0xFFB2EBF2,      // Pastel turquoise
            .riverBottom = 0xFF80D4DC,   // Richer turquoise
            .riverWave = 0xFFCCE9F2,     // Light turquoise
            .riverReflect = 0xFFE0F7FF,  // Sky reflection
            .foam = 0xBBFFFFFF,          // White foam
            .millBodyTop = 0xFFD7CCC8,   // Beige
            .millBodyBottom = 0xFFBCAAA4, // Light brown
            .millRoofTop = 0xFFEF9A9A,   // Pastel red
            .millRoofBottom = 0xFFF8BBD0, // Pastel pink
            .millWingWood = 0xFFDCE7DB,  // Light green
            .millWingWoodStroke = 0xFFA5D6A7,
            .millWingFabric = 0xFFBBDEFB, // Pastel blue
            .millWingFabricStroke = 0xFF81D4FA,
            .millWheel = 0xFF607D8B,     // Gray stone
            .treeTrunk = 0xFFA18876,     // Warm brown
            .treeLeavesBase = 0xFFA5D6A7, // Light green
            .reedStem = 0xFFBCAAA4,      // Beige
            .reedHead = 0xFFDCE7DB,      // Light green
            .lilyPad = 0xFFA5D6A7,       // Green
            .lilyFlower = 0xFFE1BEE7,    // Pastel purple
            .shadow = 0xFFB0BEC5,        // Gray
            .cowBody = 0xFFFFFEF0,       // Cream
            .cowSpots = 0xFF607D8B,      // Gray instead of black
            .cowSnout = 0xFFFFCCBC,      // Peach
            .cowDetail = 0xFFBCAAA4,     // Beige
            .wolfBody = 0xFF607D8B,      // Gray wolf
            .wolfEar = 0xFFA5D6A7,       // Light green
            .wolfEye = 0xFFFFFFA0,       // Orange eyes (friendly)
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct CyberpunkNight : SceneTheme {
        constexpr CyberpunkNight() : SceneTheme{
            .name = L"Cyberpunk Night",
            .skyTop = 0xFF1A004D,        // Dark purple (top of sky)
            .skyBottom = 0xFF4A0080,     // Magenta (bottom of sky)
            .sunDisk = 0xFFFF00FF,       // Neon magenta (sun)
            .sunStroke = 0xFFFF33FF,     // Bright magenta stroke
            .sunCorona1 = 0xFFFF66FF,    // Lighter magenta
            .sunCorona2 = 0xFFFF99FF,    // Even lighter
            .sunGlow = 0x66'FFCCFF,       // Pink glow
            .cloud = 0xCC8A00FF,         // Neon purple clouds
            .hillTop = 0xFF00CCFF,       // Neon blue (hill tops)
            .hillBottom = 0xFF0066FF,    // Blue (hill base)
            .riverTop = 0xFF003366,      // Dark blue (river surface)
            .riverBottom = 0xFF001A33,   // Almost black (depths)
            .riverWave = 0xFF00FFFF,     // Neon cyan (waves)
            .riverReflect = 0xFF66FFFF,  // Light sky reflection
            .foam = 0xBBFFFFFE,          // White foam with slight neon glow
            .millBodyTop = 0xFF330066,   // Dark purple (top of mill body)
            .millBodyBottom = 0xFF6600CC, // Magenta (bottom of body)
            .millRoofTop = 0xFFCC00FF,   // Bright magenta (roof top)
            .millRoofBottom = 0xFF9900CC, // Dark magenta (roof bottom)
            .millWingWood = 0xFF7A00E6,  // Purple (wooden parts of wings)
            .millWingWoodStroke = 0xFF5A00B8, // Darker purple (stroke)
            .millWingFabric = 0xFF00FFFF, // Neon cyan (fabric of wings)
            .millWingFabricStroke = 0xFF0099CC, // Dark cyan (fabric stroke)
            .millWheel = 0xFF212121,     // Black (mill wheel)
            .treeTrunk = 0xFF4E342E,     // Dark brown (tree trunks)
            .treeLeavesBase = 0xFF00FF00, // Bright neon green (leaves)
            .reedStem = 0xFF6A00CC,      // Magenta (reed stems)
            .reedHead = 0xFF9A00FF,      // Neon purple (reed heads)
            .lilyPad = 0xFF00FF7F,       // Neon green (lily pads)
            .lilyFlower = 0xFFFF00FF,    // Neon magenta (flowers)
            .shadow = 0xFF000022,        // Very dark shadow with neon tint
            .cowBody = 0xFFFFA0FF,       // Neon pink (cow body)
            .cowSpots = 0xFF00FFFF,      // Neon cyan (spots)
            .cowSnout = 0xFFFF55FF,      // Light magenta (snout)
            .cowDetail = 0xFFA000FF,     // Medium magenta (details)
            .wolfBody = 0xFF00FFFF,      // Neon cyan (wolf body)
            .wolfEar = 0xFF00F0F0,       // Neon cyan (ears)
            .wolfEye = 0xFFFFFF00,       // Yellow neon glow (eyes)
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct LunarEclipse : SceneTheme {
        constexpr LunarEclipse() : SceneTheme{
            .name = L"Lunar Eclipse",
            .skyTop = 0xFF333366,           // Dark blue top sky (night sky)
            .skyBottom = 0xFF663399,        // Purple horizon (eclipse effect)
            .sunDisk = 0xFFFF99CC,          // Core of the "moon" — light pink (eclipsed moon)
            .sunStroke = 0xFFCC99FF,        // Moon stroke — pinkish-purple
            .sunCorona1 = 0xFF9966CC,       // First layer of corona — dark purple
            .sunCorona2 = 0xFF663399,       // Second layer of corona — deep purple
            .sunGlow = 0x66'CCCCFF,          // Glow around the "moon" — light purple
            .cloud = 0xCCCCCCFF,            // Clouds — light gray with purple tint
            .hillTop = 0xFF333333,          // Hilltops — almost black
            .hillBottom = 0xFF666666,       // Lower parts of hills — dark gray
            .riverTop = 0xFF3333FF,         // Top part of water — dark blue with purple
            .riverBottom = 0xFF000066,      // River depths — very dark blue
            .riverWave = 0xFF6666CC,        // Waves — gray-purple
            .riverReflect = 0xFF9999CC,     // Sky reflection in water — gray-purple
            .foam = 0xBAFFFFFF,             // Foam — pure white
            .millBodyTop = 0xFF333333,      // Top part of mill body — black
            .millBodyBottom = 0xFF666666,   // Lower part of body — dark gray
            .millRoofTop = 0xFF993399,      // Top part of roof — dark purple
            .millRoofBottom = 0xFFCC99FF,   // Lower part of roof — light purple
            .millWingWood = 0xFF666666,     // Wooden part of blades — dark gray
            .millWingWoodStroke = 0xFF333333, // Wood stroke — black
            .millWingFabric = 0xFF9966CC,   // Fabric on blades — dark purple
            .millWingFabricStroke = 0xFFCC99FF, // Fabric stroke — light purple
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray
            .treeTrunk = 0xFF333333,        // Tree trunk — black
            .treeLeavesBase = 0xFF333333,   // Leaves base — dark (barely visible at night)
            .reedStem = 0xFF666666,         // Reed stem — dark gray
            .reedHead = 0xFF999999,         // Reed head — light gray
            .lilyPad = 0xFF333333,          // Lily pad — almost black
            .lilyFlower = 0xFFFF99CC,       // Lily flower — light pink (glowing in the dark)
            .shadow = 0xFF000000,           // Shadows — black
            .cowBody = 0xFF666666,          // Cow body — dark gray
            .cowSpots = 0xFF333333,         // Cow spots — black
            .cowSnout = 0xFF996666,         // Snout — gray-brown
            .cowDetail = 0xFF666666,        // Details (nostrils, eyes) — dark gray
            .wolfBody = 0xFF333333,         // Wolf body — black
            .wolfEar = 0xFF666666,          // Ears — dark gray
            .wolfEye = 0xFFFF9999,          // Eyes — glowing pink
            .effectColor1 = 0xFF9966CC,     // Purple-pink, like the glow of an eclipsed moon
            .effectColor2 = 0xFF9966CC,     // Effect: slowly falling glowing particles
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export struct NeonJungle : SceneTheme {
        constexpr NeonJungle() : SceneTheme{
            .name = L"Neon Jungle",
            .skyTop = 0xFF00FFFF,           // Bright blue top sky
            .skyBottom = 0xFF33FF99,        // Neon green horizon
            .sunDisk = 0xFFFF0000,          // Sun core — bright red
            .sunStroke = 0xFFFF9900,        // Sun stroke — orange
            .sunCorona1 = 0xFFFFCC00,       // First layer of corona — yellow
            .sunCorona2 = 0xFFFFFF00,       // Second layer of corona — bright yellow
            .sunGlow = 0xFFFFFFCC,          // Glow around the sun — very light yellow
            .cloud = 0xCCCCFFFF,            // Clouds — light blue with neon glow
            .hillTop = 0xFF00FF00,          // Hilltops — bright green
            .hillBottom = 0xFF66FF66,       // Lower parts of hills — soft neon green
            .riverTop = 0xFF33FFFF,         // Top part of water — light blue neon
            .riverBottom = 0xFF0066CC,      // River depths — dark blue with neon tint
            .riverWave = 0xFF33FFFF,        // Waves — light blue neon
            .riverReflect = 0xFFCCFFFF,     // Sky reflection in water — light blue
            .foam = 0xBBFFFFFF,             // Foam — white
            .millBodyTop = 0xFF666666,      // Top part of mill body — gray
            .millBodyBottom = 0xFF333333,   // Lower part of body — dark gray
            .millRoofTop = 0xFF9999FF,      // Top part of roof — neon blue
            .millRoofBottom = 0xFF6666FF,   // Lower part of roof — dark blue
            .millWingWood = 0xFF666666,     // Wooden part of blades — gray
            .millWingWoodStroke = 0xFF333333, // Wood stroke — dark gray
            .millWingFabric = 0x88FF00FF,   // Fabric on blades — neon magenta
            .millWingFabricStroke = 0x88333333, // Dark stroke for definition
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray
            .treeTrunk = 0xFF666666,        // Tree trunk — gray
            .treeLeavesBase = 0xFF00CC00,   // Leaves base — bright green
            .reedStem = 0xFF333333,         // Reed stem — dark gray
            .reedHead = 0xFFCCFFCC,         // Reed head — bright neon green
            .lilyPad = 0xFF00FF00,          // Lily pad — bright green
            .lilyFlower = 0xFFFF9933,       // Lily flower — orange
            .shadow = 0xFF333333,           // Shadows — dark gray
            .cowBody = 0xFF666666,          // Cow body — gray
            .cowSpots = 0xFF666666,         // Cow spots — light gray
            .cowSnout = 0xFF996666,         // Snout — gray-brown
            .cowDetail = 0xFF666666,        // Details (nostrils, eyes) — gray
            .wolfBody = 0xFF333333,         // Wolf body — black
            .wolfEar = 0xFF666666,          // Ears — gray
            .wolfEye = 0xFFFF0000,          // Eyes — bright red (contrast fix)
            .effectColor1 = 0xAAFF00FF,     // Neon magenta (purple)
            .effectColor2 = 0xAAFF00FF,     // Effect: flying fireflies with variations — from bright purple to pink
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct ArcticDawn : SceneTheme {
        constexpr ArcticDawn() : SceneTheme{
            .name = L"Arctic Dawn",
            .skyTop = 0xFF87CEFA,           // Light blue top sky (cold tone)
            .skyBottom = 0xFFE0FFFF,        // Almost white horizon with slight blue tint
            .sunDisk = 0xFFFFA07A,          // Sun core — peach, simulating a cold dawn
            .sunStroke = 0xFFFFDAB9,        // Sun stroke — light orange
            .sunCorona1 = 0xFFFFE4C4,       // First layer of corona — very light peach
            .sunCorona2 = 0xFFFFFFD9,       // Second layer of corona — almost white with light yellow
            .sunGlow = 0xFFFFFFFA,          // Glow around sun — maximum light, almost transparent
            .cloud = 0xCCE6E6FA,            // Clouds — light bluish-white, semi-transparent
            .hillTop = 0xFFD1C5A1,          // Hilltops — light beige (simulation of snow with shadow)
            .hillBottom = 0xFFCBCDC9,       // Lower parts of hills — gray with light brown (rock under snow)
            .riverTop = 0xFFADD8E6,         // Top part of water — light blue (ice reflecting the sky)
            .riverBottom = 0xFFA9A9A9,      // River depths — gray (dark ice or water under ice)
            .riverWave = 0xFFE0FFFF,        // Waves — almost white (simulation of ice shards)
            .riverReflect = 0xFFB0E0E6,     // Sky reflection in water — light blue with green tint
            .foam = 0xBBFFFFFE,             // Foam — pure white (snow or ice crumbs)
            .millBodyTop = 0xFF8B4513,      // Top part of mill body — dark brown (wooden texture)
            .millBodyBottom = 0xFF696969,   // Lower part of body — dark gray (shaded part)
            .millRoofTop = 0xFF808080,      // Top part of roof — gray (metal or snow-covered roof)
            .millRoofBottom = 0xFFC0C0C0,   // Lower part of roof — light gray (light reflection)
            .millWingWood = 0xFFD2B48C,     // Wooden part of blades — light brown
            .millWingWoodStroke = 0xFF86604E, // Wood stroke — dark brown
            .millWingFabric = 0xFFB0E0E6,   // Fabric on blades — light blue (frozen fabric or frost)
            .millWingFabricStroke = 0xFF5D9CAB, // Fabric stroke — blue (ice on edges)
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (metallic, covered in frost)
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown (frost-resistant bark)
            .treeLeavesBase = 0xFF66CDAA,   // Leaves base — light green with blue (coniferous, covered in frost)
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange (dry, winter)
            .reedHead = 0xFFC19A6B,         // Reed head — light brown (dry reed with frost)
            .lilyPad = 0xFFB0E0E6,          // Lily pad — light blue (frozen surface)
            .lilyFlower = 0xFFFFFAFA,       // Lily flower — very light pink (frozen flower)
            .shadow = 0xFF808080,           // Shadows — gray (cold shadows of the arctic morning)
            .cowBody = 0xFFCBCDC9,          // Cow body — gray-brown (winter fur)
            .cowSpots = 0xFF696969,         // Cow spots — dark gray
            .cowSnout = 0xFFCD853F,         // Snout — light brown (warm body part)
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFFF5F5DC,         // Wolf body — light beige (winter fur, blending with snow)
            .wolfEar = 0xFFD2B48C,          // Ears — light brown (warm parts)
            .wolfEye = 0xFFFFA500,          // Orange (#FFA500) — for a dramatic effect
            .effectColor1 = 0xFFFFFAFA,     // Very light pink (almost white with pinkish tint)
            .effectColor2 = 0xFFFFFAFA,     // Snow effect color
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = true,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct DesertDusk : SceneTheme {
        constexpr DesertDusk() : SceneTheme{
            .name = L"Desert Dusk",
            .skyTop = 0xFFA52A2A,           // Dark orange top sky (desert sunset)
            .skyBottom = 0xFFD2B48C,        // Light brown horizon (sand dunes at sunset)
            .sunDisk = 0xFFFF8C00,          // Sun core — bright orange
            .sunStroke = 0xFFFFA500,        // Sun stroke — light orange
            .sunCorona1 = 0xFFFFD700,       // First layer of corona — yellow
            .sunCorona2 = 0xFFFFFF00,       // Second layer of corona — bright yellow
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCE6E6FA,            // Clouds — light gray (rare clouds in the desert)
            .hillTop = 0xFFD2B48C,          // Hilltops — light brown (sand)
            .hillBottom = 0xFFA52A2A,       // Lower parts of hills — dark orange (sunset shadow)
            .riverTop = 0xFF1E90FF,         // Top part of water — blue (oasis)
            .riverBottom = 0xFF00BFFF,      // River depths — dark blue (cold oasis water)
            .riverWave = 0xFF7B68EE,        // Waves — lilac (sunset reflection)
            .riverReflect = 0xFFE6E6FA,     // Sky reflection in water — light blue
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .millBodyTop = 0xFF8B4513,      // Top part of mill body — dark brown (sun-baked wood)
            .millBodyBottom = 0xFF696969,   // Lower part of body — dark gray (dusty base)
            .millRoofTop = 0xFF8B0000,      // Top part of roof — dark red (faded roof)
            .millRoofBottom = 0xFFB22222,   // Lower part of roof — burgundy (shadow)
            .millWingWood = 0xFFDEB887,     // Wooden part of blades — light brown
            .millWingWoodStroke = 0xFFD2B48C, // Wood stroke — slightly darker
            .millWingFabric = 0xFF6A5ACD,   // Fabric on blades — lilac (dust with sky reflection)
            .millWingFabricStroke = 0xFF7B68EE, // Fabric stroke — lighter
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (rusty)
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown (dry wood)
            .treeLeavesBase = 0xFF228B22,   // Leaves base — dark green (rare oasis greenery)
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange
            .reedHead = 0xFFC19A6B,         // Reed head — light brown
            .lilyPad = 0xFF32CD32,          // Lily pad — bright green (oasis)
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach (contrast with sunset)
            .shadow = 0xFF696969,           // Shadows — gray (dusty, warm desert shadows)
            .cowBody = 0xFFCBCDC9,          // Cow body — gray-brown (sandy camouflage)
            .cowSpots = 0xFF696969,         // Cow spots — dark gray (shadows)
            .cowSnout = 0xFFCD853F,         // Snout — light brown (warm body part)
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFFF5F5DC,         // Wolf body — light beige (sandy camouflage)
            .wolfEar = 0xFFD2B48C,          // Ears — light brown (warm parts)
            .wolfEye = 0xFF6A5ACD,          // Same as the wing blades
            .effectColor1 = 0xFFFFD700,     // Golden (like sand lit by sunset)
            .effectColor2 = 0xFFFFD700,     // Effect: swirling sand particles in the air, illuminated by the last rays of the sun
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = true,
            .isAfrica = false
        } {
        }
    };

    export struct MagicForest : SceneTheme {
        constexpr MagicForest() : SceneTheme{
            .name = L"Magic Forest",
            .skyTop = 0xFF7B68EE,           // Lilac top sky (magical atmosphere)
            .skyBottom = 0xFFE6E6FA,        // Light blue horizon (magical light)
            .sunDisk = 0xFFFFA07A,          // Sun core — peach (magical light)
            .sunStroke = 0xFFFFDAB9,        // Sun stroke — light orange
            .sunCorona1 = 0xFFFFE4C4,       // First layer of corona — peach
            .sunCorona2 = 0xFFFFFFD9,       // Second layer of corona — almost white
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCE6E6FA,            // Clouds — light gray with magical tint
            .hillTop = 0xFF228B22,          // Hilltops — dark green (dense forest)
            .hillBottom = 0xFF90EE90,       // Lower parts of hills — light green (undergrowth)
            .riverTop = 0xFF00BFFF,         // Top part of water — blue (clear forest river)
            .riverBottom = 0xFF1E90FF,      // River depths — dark blue (mysterious depth)
            .riverWave = 0xFF7B68EE,        // Waves — lilac (magical reflection)
            .riverReflect = 0xFFE6E6FA,     // Sky reflection in water — light blue
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .millBodyTop = 0xFFA0522D,      // Top part of mill body — brown (wood)
            .millBodyBottom = 0xFF8B4513,   // Lower part of body — dark brown (old wood)
            .millRoofTop = 0xFF8B0000,      // Top part of roof — dark red (old tiles)
            .millRoofBottom = 0xFFB22222,   // Lower part of roof — burgundy (shadow)
            .millWingWood = 0xFFDEB887,     // Wooden part of blades — light brown
            .millWingWoodStroke = 0xFFD2B48C, // Wood stroke — slightly darker
            .millWingFabric = 0xFF6A5ACD,   // Fabric on blades — lilac (magical overlay)
            .millWingFabricStroke = 0xFF7B68EE, // Fabric stroke — lighter
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (rusty)
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown (old tree)
            .treeLeavesBase = 0xFF228B22,   // Leaves base — dark green (dense foliage)
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange
            .reedHead = 0xFFC19A6B,         // Reed head — light brown
            .lilyPad = 0xFF32CD32,          // Lily pad — bright green
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach (glowing)
            .shadow = 0xFF333333,           // Shadows — black (mysterious forest shadows)
            .cowBody = 0xFFCBCDC9,          // Cow body — gray-brown (blends with undergrowth)
            .cowSpots = 0xFF696969,         // Cow spots — dark gray
            .cowSnout = 0xFFCD853F,         // Snout — light brown
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF4B3621,         // Wolf body — dark gray (stealth in the shadows)
            .wolfEar = 0xFF696969,          // Ears — gray
            .wolfEye = 0xFFFFDCBC,          // Eyes — glowing, with mystic reflection
            .effectColor1 = 0xFF6A5ACD,     // Lilac (magical tint)
            .effectColor2 = 0xFF6A5ACD,     // Effect: twinkling magical sparks slowly rising from the ground and trees
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct SteampunkAdventure : SceneTheme {
        constexpr SteampunkAdventure() : SceneTheme{
            .name = L"Steampunk",
            .skyTop = 0xFF797979,           // Gray sky (smoky industrial landscape)
            .skyBottom = 0xFFD3D3D3,        // Light gray horizon (smog)
            .sunDisk = 0xFFFF9966,          // Sun core — rusty orange (through haze)
            .sunStroke = 0xFFFFCC99,        // Sun stroke — light orange
            .sunCorona1 = 0xFFFFE5CC,       // First layer of corona — yellow
            .sunCorona2 = 0xFFFFFFD9,       // Second layer of corona — almost white
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCCCCCCC,            // Clouds — gray, industrial
            .hillTop = 0xFF7B4630,          // Hilltops — dark brown (soil, coal)
            .hillBottom = 0xFF6B3B31,       // Lower parts of hills — brown-red (mines)
            .riverTop = 0xFF666666,         // Top part of water — gray (polluted river)
            .riverBottom = 0xFF333333,      // River depths — black (murky water)
            .riverWave = 0xFF8B4513,        // Waves — brown (mud, silt)
            .riverReflect = 0xFFCCCCCC,     // Sky reflection in water — gray
            .foam = 0xBBCCCCCC,             // Foam — gray (soapy, from industrial waste)
            .millBodyTop = 0xFF808080,      // Top part of mill body — gray metal
            .millBodyBottom = 0xFF696969,   // Lower part of body — dark gray (rust)
            .millRoofTop = 0xFFB22222,      // Top part of roof — burgundy (corroded steel)
            .millRoofBottom = 0xFF8B0000,   // Lower part of roof — dark red (soot marks)
            .millWingWood = 0xFFD2B48C,     // Wooden part of blades — light brown (old wood)
            .millWingWoodStroke = 0xFFA0522D, // Wood stroke — dark brown (cracks, wear)
            .millWingFabric = 0xFF6A5ACD,   // Fabric on blades — lilac (dust, grease)
            .millWingFabricStroke = 0xFF7B68EE, // Fabric stroke — lighter (steam reflections)
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (rust, grease)
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown (surviving tree among factories)
            .treeLeavesBase = 0xFF377537,   // Leaves base — dark green (rare greenery)
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange
            .reedHead = 0xFFC19A6B,         // Reed head — light brown
            .lilyPad = 0xFF349434,          // Lily pad — bright green (islands of life)
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach
            .shadow = 0xFF333333,           // Shadows — black (deep shadows of industrial zones)
            .cowBody = 0xFFCBCDC9,          // Cow body — gray-brown (dusty fur)
            .cowSpots = 0xFF696969,         // Cow spots — dark gray (soot-covered)
            .cowSnout = 0xFFCD853F,         // Snout — light brown (grease marks)
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF4B3621,         // Wolf body — dark gray (metallic tint)
            .wolfEar = 0xFF696969,          // Ears — gray (with mechanical inserts)
            .wolfEye = 0xFFFFA07A,          // Eyes — glowing peach (mechanical implants)
            .effectColor1 = 0xFFCC9966,     // Rusty orange (like steam from pipes)
            .effectColor2 = 0xFFCC9966,     // Effect: puffs of steam and smoke periodically released; rare sparks
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = true,
            .isAfrica = false
        } {
        }
    };

    export struct Retro80s : SceneTheme {
        constexpr Retro80s() : SceneTheme{
            .name = L"Retro 80s",
            .skyTop = 0xFF7B68EE,           // Lilac top sky (pastel gradient)
            .skyBottom = 0xFFFFDAB9,        // Peach horizon (80s sunset)
            .sunDisk = 0xFFFF6969,          // Sun core — coral
            .sunStroke = 0xFFFF9966,        // Sun stroke — orange
            .sunCorona1 = 0xFFFFCC99,       // First layer of corona — light orange
            .sunCorona2 = 0xFFFFFFD9,       // Second layer of corona — almost white
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — very light peach
            .cloud = 0xCCE6E6FA,            // Clouds — light gray with pastel tint
            .hillTop = 0xFF90EE90,          // Hilltops — light green (neon tint)
            .hillBottom = 0xFF32CD32,       // Lower parts of hills — bright green
            .riverTop = 0xFF00BFFF,         // Top part of water — blue (neon)
            .riverBottom = 0xFF1E90FF,      // River depths — dark blue
            .riverWave = 0xFF7B68EE,        // Waves — lilac (sky reflection)
            .riverReflect = 0xFFE6E6FA,     // Sky reflection in water — light blue
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .millBodyTop = 0xFFA0522D,      // Top part of mill body — brown (wooden texture)
            .millBodyBottom = 0xFF8B4513,   // Lower part of body — dark brown
            .millRoofTop = 0xFF8B0000,      // Top part of roof — dark red (faded tiles)
            .millRoofBottom = 0xFFB22222,   // Lower part of roof — burgundy
            .millWingWood = 0xFFDEB887,     // Wooden part of blades — light brown
            .millWingWoodStroke = 0xFFD2B48C, // Wood stroke — slightly darker
            .millWingFabric = 0xFF6A5ACD,   // Fabric on blades — lilac (neon fabric)
            .millWingFabricStroke = 0xFF7B68EE, // Fabric stroke — lighter
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown
            .treeLeavesBase = 0xFF228B22,   // Leaves base — dark green
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange
            .reedHead = 0xFFC19A6B,         // Reed head — light brown
            .lilyPad = 0xFF32CD32,          // Lily pad — bright green
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach
            .shadow = 0xFF333333,           // Shadows — black (high contrast, like 8-bit graphics)
            .cowBody = 0xFFFFA07A,          // Cow body — peach (neon coloring)
            .cowSpots = 0xFF6666CC,         // Cow spots — lilac
            .cowSnout = 0xFFCD853F,         // Snout — light brown
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF4B3621,         // Wolf body — dark gray (with neon highlights)
            .wolfEar = 0xFF696969,          // Ears — gray
            .wolfEye = 0xFFFF00FF,          // Eyes — bright magenta (neon glow)
            .effectColor1 = 0xFFFF00FF,     // Magenta (neon pink)
            .effectColor2 = 0xFFFF00FF,     // Effect: fast flashes of neon light (like signs and a disco ball), strobe effect
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct UnderwaterRealm : SceneTheme {
        constexpr UnderwaterRealm() : SceneTheme{
            .name = L"Underwater Realm",
            .skyTop = 0xFF000080,           // Dark blue top (ocean depth)
            .skyBottom = 0xFF0066CC,        // Light blue horizon (closer to surface)
            .sunDisk = 0xFFFFD700,          // Sun core — golden (light through water)
            .sunStroke = 0xFFFFF566,        // Sun stroke — light yellow
            .sunCorona1 = 0xFFFFFF99,       // First layer of corona — yellow
            .sunCorona2 = 0xFFFFFFCC,       // Second layer of corona — very light yellow
            .sunGlow = 0x66'FFFBF0,          // Glow around sun — almost white (scattered light)
            .cloud = 0xCCE0FFFF,            // "Clouds" — air bubbles, light blue
            .hillTop = 0xFF20B2AA,          // Hilltops — sea green (algae)
            .hillBottom = 0xFF008080,       // Lower parts of hills — dark turquoise (corals)
            .riverTop = 0xFF00CED1,         // Top part of water — turquoise
            .riverBottom = 0xFF000080,      // River depths — dark blue
            .riverWave = 0xFF7FFFD4,        // Waves — aquamarine (light highlights)
            .riverReflect = 0xFFE0FFFF,     // Sky reflection in water — blue
            .foam = 0xBBFFFFFF,             // Foam/bubbles — pure white
            .millBodyTop = 0xFF8B7352,      // Top part of mill body — brown (submerged wood)
            .millBodyBottom = 0xFF654321,   // Lower part of body — dark brown (rot)
            .millRoofTop = 0xFF5F9EA0,      // Top part of roof — turquoise (algae covered roof)
            .millRoofBottom = 0xFF4682B4,   // Lower part of roof — steel blue (rust)
            .millWingWood = 0xFFD2B48C,     // Wooden part of blades — light brown (wet wood)
            .millWingWoodStroke = 0xFFA0522D, // Wood stroke — dark brown
            .millWingFabric = 0xFF5F9EA0,   // Fabric on blades — turquoise (algae)
            .millWingFabricStroke = 0xFF4682B4, // Fabric stroke — steel blue
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (rust underwater)
            .treeTrunk = 0xFF8B7352,        // Tree trunk — brown (sunken tree)
            .treeLeavesBase = 0xFF20B2AA,   // Leaves base — sea green
            .reedStem = 0xFF8FBC8F,         // Reed stem — gray-green (underwater)
            .reedHead = 0xFF98FB98,         // Reed head — light green (underwater)
            .lilyPad = 0xFF20B2AA,          // Lily pad — sea green
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach (glowing underwater)
            .shadow = 0xFF000033,           // Shadows — dark blue (scattered light underwater)
            .cowBody = 0xFF8FBC8F,          // Cow body — gray-green (algae on the hide)
            .cowSpots = 0xFF5F9EA0,         // Cow spots — turquoise (corals)
            .cowSnout = 0xFFCD853F,         // Snout — light brown (natural color)
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF40E0D0,         // Wolf body — turquoise (underwater coloration)
            .wolfEar = 0xFF7FFFD4,          // Ears — aquamarine
            .wolfEye = 0xFFFFFF00,          // Eyes — bright yellow (glowing in the dark water)
            .effectColor1 = 0xFF00FFFF,     // Cyan (aquamarine)
            .effectColor2 = 0xFF00FFFF,     // Effect: slowly rising air bubbles reflecting light; rare glowing plankton
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false
        } {
        }
    };

    export struct NeonCityNight : SceneTheme {
        constexpr NeonCityNight() : SceneTheme{
            .name = L"Neon City Night",
            .skyTop = 0xFF191970,           // Dark blue top sky (metropolis night sky)
            .skyBottom = 0xFF4B0082,        // Purple horizon (city glow)
            .sunDisk = 0xFFFF00FF,          // "Sun" core — magenta (neon sign)
            .sunStroke = 0xFF00FFFF,        // Stroke — cyan (neighboring sign)
            .sunCorona1 = 0xFFFF1493,       // First layer of corona — neon pink
            .sunCorona2 = 0xFF00FF7F,       // Second layer of corona — neon green
            .sunGlow = 0x66'FFFBF0,          // Glow around — almost white (scattered light)
            .cloud = 0xCC696969,            // Clouds — dark gray (smoky sky)
            .hillTop = 0xFF333333,          // Hilltops — dark gray (building roofs)
            .hillBottom = 0xFF1C1C1C,       // Lower parts of hills — black (shadows between skyscrapers)
            .riverTop = 0xFF000080,         // Top part of water — dark blue (river reflecting neon lights)
            .riverBottom = 0xFF000033,      // River depths — almost black
            .riverWave = 0xFF00FFFF,        // Waves — cyan (sign reflections)
            .riverReflect = 0xFF4B0082,     // Sky reflection in water — purple
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .millBodyTop = 0xFF808080,      // Top part of mill body — gray (concrete)
            .millBodyBottom = 0xFF696969,   // Lower part of body — dark gray
            .millRoofTop = 0xFFFF00FF,      // Top part of roof — magenta (neon backlight)
            .millRoofBottom = 0xFF00FFFF,   // Lower part of roof — cyan
            .millWingWood = 0xFFD2B48C,     // Wooden part of blades — light brown (old structure)
            .millWingWoodStroke = 0xFFA0522D, // Wood stroke — dark brown
            .millWingFabric = 0xFF00FF00,   // Fabric on blades — bright neon green
            .millWingFabricStroke = 0xFF32CD32, // Fabric stroke — lime
            .millWheel = 0xFF2F4F4F,        // Mill wheel — dark gray (metal)
            .treeTrunk = 0xFFA52A2A,        // Tree trunk — brown (artificial tree)
            .treeLeavesBase = 0xFF32CD32,   // Leaves base — bright green
            .reedStem = 0xFF8B4513,         // Reed stem — brown-orange
            .reedHead = 0xFFC19A6B,         // Reed head — light brown
            .lilyPad = 0xFF32CD32,          // Lily pad — bright green
            .lilyFlower = 0xFFFFA07A,       // Lily flower — peach
            .shadow = 0xFF000000,           // Shadows — black (deep city shadows)
            .cowBody = 0xFFCCCCCC,          // Cow body — light gray (metallic tint)
            .cowSpots = 0xFF00FFFF,         // Cow spots — cyan (neon markers)
            .cowSnout = 0xFFCD853F,         // Snout — light brown
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF1C1C1C,         // Wolf body — black (blends with city shadows)
            .wolfEar = 0xFF696969,          // Ears — gray
            .wolfEye = 0xFFFF0000,          // Eyes — bright red (glowing)
            .effectColor1 = 0xFFFFFF00,     // Bright yellow (like light from a neon "TAXI" sign)
            .effectColor2 = 0xFFFFFF00,     // Effect: fast changing color flashes (yellow, pink, blue) — simulating billboards, traffic lights, car headlights
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true
        } {
        }
    };

    export constexpr std::array<SceneTheme, 24> allThemes = {
        CalmNight{},
        StormyNight{},
        Midnight{},
        MidnightFantasy{},
        WinterNight{},
        Twilight{},
        LunarEclipse{},
        UnderwaterRealm{},

        CyberpunkNight{},
        NeonCityNight{},
        MagicForest{},
        SteampunkAdventure{},
        ToxicWaste{},
        VintageNoir{},
        ArcticDawn{},
        WinterFrost{},

        WarmOrangeFall{},
        SaharaHeat{},
        GoldenHour{},
        PaperSketch{},
        SummerSunset{},
        HighNoon{},
        CherryBlossom{},
        FreshSpring{},

        //PastelDream{},
        //NeonJungle{},
        //DesertDusk{},
        //Retro80s{}
    };

    export const SceneTheme& getTheme(std::size_t index)
    {
        return allThemes[index];
    }
}
