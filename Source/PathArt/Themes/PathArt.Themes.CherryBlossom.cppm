export module ClaFi.PathArt.Themes :CherryBlossom;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct CherryBlossom : SceneTheme {
        constexpr CherryBlossom() : SceneTheme{
            .name = L"Cherry Blossom",
            .skyTop = 0xFF88AED9,        // Calm blue (top sky)
            .skyBottom = 0xFFC8DEF1,     // Pale blue (bottom sky)
            .sunDisk = 0xFFFFD700,       // Bright yellow (sun center)
            .sunGlow = 0xFFFFFAFA,       // White glow around sun
            .cloud = 0xCCFAFAFA,         // Off-white clouds
            .mountainTop = 0xFFCDCACB,
            .mountainFoot = 0xFF9E9B9C,
            .hillTop = 0xFFADC69A,
            .hillBottom = 0xFF729269,
            .riverTop = 0xFF2E559E,      // Deep blue (river top)
            .riverBottom = 0xFF64A1D5,   // Lighter blue (river bottom)
            .riverWave = 0xFFFFC8DC,     // Same as Effect1
            .foam = 0xBBFFFFFE,          // White foam
            .treeLeavesBase = 0xFFF8BBD0, // Light pink (tree leaves base — cherry blossom style)
            .crownLight = 0xFFFFD4F5,
            .crownMiddle = 0xFFFFC5D1,
            .bark = 0xFF7E512E,
            .barkShade = 0xFF50321C,
            .bough = 0xFF603C22,
            .trail = 0xE6CDCED0,
            .grassBlade = {
                .light = 0xFF48644D,
                .dark = 0xFF3B5640
            },
            .grassPatch = {
                .dark = 0xFF607D64,
                .middle = 0xFF77957C,
                .light = 0xFF93B398
            },
            .reedStem = 0xFF48644D,
            .reedHead = 0xFF775234,
            .stoneLight = 0xFFCACAC5,
            .stoneDark = 0xFF8B8C87,
            .stoneCrack = 0xFF70716D,
            .shadow = 0x47000000,
            .cowBody = 0xFFFFFFFE,       // White cow body
            .cowSpots = 0xFF000000,      // Black spots
            .cowSnout = 0xFFCD853F,      // Light brown snout
            .cowDetail = 0xFF8B4513,     // Brown facial detail
            .wolfBody = 0xFF362717,      // Dark gray-brown wolf body
            .wolfEye = 0xFFFF0000,       // Red glowing eyes
            .effectColor1 = 0xFFFFC8DC,
            .effectColor2 = 0xFFFF96B4,
            .petal1 = 0xFFFFAFC8,
            .petal2 = 0xFFFF96B4,
            .petal3 = 0xFFFFC8DC,
            .flowerHeart = 0xFFF0C960,
            .wing = 0xFFF1A042,
            .wingEdge = 0xFF392A22,
            .flock = 0xFF2A2E3D,
            .motherDuckBody = 0xFF886A4C,
            .motherDuckWing = 0xFF634A34,
            .motherDuckEye = 0xFF8B4513,
            .duckLight = 0xFFFBDA4D,
            .duckShade = 0xFFE5AC22,
            .duckBeak = 0xFFEF8B23,
            .duckBeakShade = 0xFFC7651E,
            .fish = 0xFF959EAE,
            .hedge = 0xFF909188,
            .mill = {
                .bodyTop = 0xFFD8CABD,   // pale stone (top mill body)
                .bodyBottom = 0xFFA0928A, // gray stone (bottom mill body)
                .roofTop = 0xFFC08A88,   // dusty rose (lit roof side)
                .roofBottom = 0xFF845655, // deep dusty rose (shaded roof side)
                .timber = 0xFF603C22,
                .wingFabric = 0xCCF3F1F4, // white canvas (fabric of wing)
                .wheel = 0xFF7A6E66      // gray weathered wood (mill wheel)
            },
            .chase = {
                .treeCrown = 0xFFF8BBD0,
                .treeCrownLight = 0xFFFFD4F5,
                .treeCrownMiddle = 0xFFFFC5D1,
                .pebble = 0xFFA6A7AA,
                .ourWolf = {
                    .light = 0xFF747E8D,
                    .shade = 0xFF515967,
                    .coat = 0xFF636D7B,
                    .far = 0xFF47505B,
                    .tips = 0xFF363D48,
                    .belly = 0xE6AAB2BC,
                    .tongue = 0xFFEB6F81,
                    .mouth = 0xFF581C2A,
                    .tooth = 0xFFF7F4E9,
                    .toothEdge = 0xFF99938A
                },
                .ourHare = {
                    .light = 0xFFAA7849,
                    .fur = 0xFF9B6B3D,
                    .shade = 0xFF8A5B2F,
                    .belly = 0xE6D0C2A7,
                    .tail = 0xFFFCFDFF,
                    .earInside = 0xFFEEA6A2,
                    .nose = 0xFFD5777C,
                    .whisker = 0x80593A22,
                    .mouth = 0xFF582B29,
                    .tooth = 0xFFFCFDFF,
                    .toothEdge = 0xFFA59D96
                },
                .theirWolf = {
                    .light = 0xFF4A4E59,
                    .shade = 0xFF333741,
                    .coat = 0xFF3E424C,
                    .far = 0xFF2A2D36,
                    .tips = 0xFF1D2028,
                    .belly = 0xE6878D97,
                    .tongue = 0xFFEB6F81,
                    .mouth = 0xFF581C2A,
                    .tooth = 0xFFF7F4E9,
                    .toothEdge = 0xFF99938A
                },
                .theirHare = {
                    .light = 0xFF7C7E7A,
                    .fur = 0xFF6B6C68,
                    .shade = 0xFF595A56,
                    .belly = 0xE6B6B6B2,
                    .tail = 0xFFFCFDFF,
                    .earInside = 0xFFEEA6A2,
                    .nose = 0xFFD5777C,
                    .whisker = 0x80593A22,
                    .mouth = 0xFF582B29,
                    .tooth = 0xFFFCFDFF,
                    .toothEdge = 0xFFA59D96
                },
                .appleLight = 0xFFE44D41,
                .appleDark = 0xFFA02624,
                .leaf = 0xFF4CA14E,
                .appleFlesh = 0xFFF7EFD4,
                .appleSeed = 0xFF482E1D,
                .crowBody = 0xFF2A2E3D,
                .crowWing = 0xFF1B1F2B,
                .crowWingFar = 0xFF141721,
                .crowBeak = 0xFF494C57,
                .bootLight = 0xFF876443,
                .bootDark = 0xFF5C3F28,
                .bootSole = 0xFF2D2623,
                .bootPatch = 0xFF9D7A52,
                .lace = 0xFFE4D7B7,
                .spinesDark = 0xFF5A4633,
                .spinesLight = 0xFF7A6149,
                .hedgehogFace = 0xFF977658,
                .hedgehogEar = 0xFF775A43,
                .ballPanel1 = 0xFFE44D41,
                .ballPanel2 = 0xFF397DD9,
                .ballPanel3 = 0xFFFCD23F,
                .ballBase = 0xFFF4F4F1,
                .peel = 0xFFF9D64D,
                .peelShade = 0xFFDEB433,
                .peelInside = 0xFFFFF8E1,
                .peelTip = 0xFF63492B,
                .rakeHandle = 0xFFAC7A49,
                .rakeHandleShade = 0xFF875A34,
                .rakeMetal = 0xFF6B737E,
                .capLight = 0xFFE14941,
                .capDark = 0xFFA52824,
                .capSpot = 0xFFF6F2E7,
                .stalk = 0xFFEEE6D4,
                .stalkShade = 0xFFCEC1A2,
                .board = 0xFFB38150,
                .boardShade = 0xFF7E512E,
                .stumpTop = 0xFFCCAE84,
                .stumpRing = 0xFFA38055,
                .stumpSprout = 0xFF32802F,
                .frogLight = 0xFF659E11,
                .frogDark = 0xFF2E6313,
                .frogBelly = 0xFFBBC48D,
                .shellLight = 0xFFCC8A4A,
                .shellDark = 0xFF8B4F27,
                .shellLine = 0xFF693A1D,
                .snailBody = 0xFFC6B69C,
                .fluff = 0xFFEEE7DA,
                .birdBody = 0xFF886A4C,
                .birdBelly = 0xFFE6D9C2,
                .birdWing = 0xFF634A34,
                .birdBeak = 0xFFE1A242,
                .dust = 0xFFDFD0B5,
                .dazeStar = 0xFFFFD34D
            },
            .cowTips = 0xFF543B33,
            .lilyFlowerTint = 0xFFFF96B4,
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = true,
            .hasDust = false,
            .isAfrica = false,
            .celestialGlowStrength = 0.5f
        } {
        }
    };
}
