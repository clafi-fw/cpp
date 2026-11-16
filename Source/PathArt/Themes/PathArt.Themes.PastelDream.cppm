export module ClaFi.PathArt.Themes :PastelDream;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct PastelDream : SceneTheme {
        constexpr PastelDream() : SceneTheme{
            .name = L"Pastel Dream",
            .skyTop = 0xFFC6E2FF,        // Pastel blue
            .skyBottom = 0xFFE0F7FF,     // Very light blue
            .sunDisk = 0xFFFFF599,       // Pastel yellow
            .sunGlow = 0xFFFFFBF0,       // Soft white glow
            .cloud = 0xCCFAFAFA,         // White clouds
            .mountainTop = 0xFF928D8B,
            .mountainFoot = 0xFF64605E,
            .hillTop = 0xFFA8E67F,       // Pastel green
            .hillBottom = 0xFFC8E6A0,    // Light lime
            .riverTop = 0xFFB2EBF2,      // Pastel turquoise
            .riverBottom = 0xFF80D4DC,   // Richer turquoise
            .riverWave = 0xFFCCE9F2,     // Light turquoise
            .foam = 0xBBFFFFFF,          // White foam
            .treeLeavesBase = 0xFFA5D6A7, // Light green
            .crownLight = 0xFFCCF3C5,
            .crownMiddle = 0xFFB2E1A8,
            .bark = 0xFF895A36,
            .barkShade = 0xFF5B3A21,
            .bough = 0xFF6B4427,
            .trail = 0xE6D5D38E,
            .grassBlade = {
                .light = 0xFF58A351,
                .dark = 0xFF4F8F48
            },
            .grassPatch = {
                .dark = 0xFF429043,
                .middle = 0xFF58AC53,
                .light = 0xFF7BCE64
            },
            .reedStem = 0xFF6FA356,
            .reedHead = 0xFF7A5232,
            .stoneLight = 0xFFB8B1A7,
            .stoneDark = 0xFF79746B,
            .stoneCrack = 0xFF5F5A52,
            .shadow = 0x47B0BEC5,
            .cowBody = 0xFFACAB9E,       // Cream
            .cowSpots = 0xFF1B3642,      // Gray instead of black
            .cowSnout = 0xFFAB7D6E,      // Peach
            .cowDetail = 0xFF6E5E59,     // Beige
            .wolfBody = 0xFF6C8591,      // Gray wolf
            .wolfEye = 0xFFFFFFA0,       // Orange eyes (friendly)
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .petal1 = 0xFFFFFFFF,
            .petal2 = 0xFFFCD6DD,
            .petal3 = 0xFFE0D9F9,
            .flowerHeart = 0xFFFCD362,
            .wing = 0xFFF1A249,
            .wingEdge = 0xFF3C291E,
            .flock = 0xFF2F2E37,
            .motherDuckBody = 0xFF8B6A4A,
            .motherDuckWing = 0xFF674A31,
            .motherDuckEye = 0xFF6E5E59,
            .duckLight = 0xFFFFEEB3,
            .duckShade = 0xFFF5BF50,
            .duckBeak = 0xFFFDA04B,
            .duckBeakShade = 0xFFD4793D,
            .fish = 0xFF76847F,
            .mill = {
                .bodyTop = 0xFFB7ACA8,   // Beige
                .bodyBottom = 0xFF9D8B86, // Light brown
                .roofTop = 0xFFCD7B7C,   // Pastel red
                .roofBottom = 0xFFD69CB0, // Pastel pink
                .timber = 0xFF6B4427,
                .wingFabric = 0xFF9BBEDA, // Pastel blue
                .wheel = 0xFF44606D      // Gray stone
            },
            .chase = {
                .treeCrown = 0xFFA5D6A7,
                .treeCrownLight = 0xFFCCF3C5,
                .treeCrownMiddle = 0xFFB2E1A8,
                .pebble = 0xFFBEBA76,
                .ourWolf = {
                    .light = 0xFF82848D,
                    .shade = 0xFF5D5F67,
                    .coat = 0xFF71737B,
                    .far = 0xFF53555B,
                    .tips = 0xFF414248,
                    .belly = 0xE6B8B8BD,
                    .tongue = 0xFFEA747F,
                    .mouth = 0xFF581E26,
                    .tooth = 0xFFFFF5E6,
                    .toothEdge = 0xFF9F9386
                },
                .ourHare = {
                    .light = 0xFFC89362,
                    .fur = 0xFFB88556,
                    .shade = 0xFFA77448,
                    .belly = 0xE6F2DEC0,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFF0A8A0,
                    .nose = 0xFFD57B7A,
                    .whisker = 0x805B3A1F,
                    .mouth = 0xFF592B25,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFAB9D92
                },
                .theirWolf = {
                    .light = 0xFF504E53,
                    .shade = 0xFF39373B,
                    .coat = 0xFF444246,
                    .far = 0xFF2F2D31,
                    .tips = 0xFF222023,
                    .belly = 0xE68E8D92,
                    .tongue = 0xFFEA747F,
                    .mouth = 0xFF581E26,
                    .tooth = 0xFFFFF5E6,
                    .toothEdge = 0xFF9F9386
                },
                .theirHare = {
                    .light = 0xFFB6B0A7,
                    .fur = 0xFFA39D94,
                    .shade = 0xFF908A81,
                    .belly = 0xE6F3ECE3,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFF0A8A0,
                    .nose = 0xFFD57B7A,
                    .whisker = 0x805B3A1F,
                    .mouth = 0xFF592B25,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFAB9D92
                },
                .appleLight = 0xFFE15442,
                .appleDark = 0xFF9E2D23,
                .leaf = 0xFF5FA04F,
                .appleFlesh = 0xFFFEF0D2,
                .appleSeed = 0xFF4A2E1A,
                .crowBody = 0xFF2F2E37,
                .crowWing = 0xFF201F25,
                .crowWingFar = 0xFF19171C,
                .crowBeak = 0xFF4F4C51,
                .bootLight = 0xFF8A6442,
                .bootDark = 0xFF5E3F26,
                .bootSole = 0xFF31251F,
                .bootPatch = 0xFFA07A51,
                .lace = 0xFFEAD8B5,
                .spinesDark = 0xFF5D4530,
                .spinesLight = 0xFF7E6146,
                .hedgehogFace = 0xFF9A7656,
                .hedgehogEar = 0xFF7A5A40,
                .ballPanel1 = 0xFFE15442,
                .ballPanel2 = 0xFF4B80CC,
                .ballPanel3 = 0xFFFCD456,
                .ballBase = 0xFFFCF5ED,
                .peel = 0xFFFDEEBB,
                .peelShade = 0xFFF3CA5D,
                .peelInside = 0xFFFFFFFD,
                .peelTip = 0xFF775939,
                .rakeHandle = 0xFFAF7A4A,
                .rakeHandleShade = 0xFF895A33,
                .rakeMetal = 0xFF727378,
                .capLight = 0xFFDE5142,
                .capDark = 0xFFA32F24,
                .capSpot = 0xFFFDF3E4,
                .stalk = 0xFFF5E7D1,
                .stalkShade = 0xFFD4C1A0,
                .board = 0xFFBF8A59,
                .boardShade = 0xFF895A36,
                .stumpTop = 0xFFDAB88C,
                .stumpRing = 0xFFB08A5D,
                .stumpSprout = 0xFF4E8738,
                .frogLight = 0xFF96C34C,
                .frogDark = 0xFF598731,
                .frogBelly = 0xFFEAEBB2,
                .shellLight = 0xFFCE8B4D,
                .shellDark = 0xFF8C5027,
                .shellLine = 0xFF6A3B1B,
                .snailBody = 0xFFCBB69A,
                .fluff = 0xFFF5E8D6,
                .birdBody = 0xFF8B6A4A,
                .birdBelly = 0xFFECDABF,
                .birdWing = 0xFF674A31,
                .birdBeak = 0xFFE3A348,
                .dust = 0xFFECDAB8,
                .dazeStar = 0xFFFFD34D
            },
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
}
