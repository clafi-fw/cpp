export module ClaFi.PathArt.Themes :UnderwaterRealm;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct UnderwaterRealm : SceneTheme {
        constexpr UnderwaterRealm() : SceneTheme{
            .name = L"Underwater Realm",
            .skyTop = 0xFF03279A,           // Dark blue top (ocean depth)
            .skyBottom = 0xFF0066CC,        // Light blue horizon (closer to surface)
            .sunDisk = 0xFFFFD700,          // Sun core — golden (light through water)
            .sunGlow = 0x66'FFFBF0,          // Glow around sun — almost white (scattered light)
            .cloud = 0xCCE0FFFF,            // "Clouds" — air bubbles, light blue
            .mountainTop = 0xFF050711,
            .mountainFoot = 0xFF000001,
            .hillTop = 0xFF20B2AA,          // Hilltops — sea green (algae)
            .hillBottom = 0xFF008080,       // Lower parts of hills — dark turquoise (corals)
            .riverTop = 0xFF00CED1,         // Top part of water — turquoise
            .riverBottom = 0xFF000080,      // River depths — dark blue
            .riverWave = 0xFF7FFFD4,        // Waves — aquamarine (light highlights)
            .foam = 0xBBFFFFFF,             // Foam/bubbles — pure white
            .treeLeavesBase = 0xFF20B2AA,   // Leaves base — sea green
            .crownLight = 0xFF28CAC8,
            .crownMiddle = 0xFF23BBAB,
            .bark = 0xFF345D67,
            .barkShade = 0xFF1C393F,
            .bough = 0xFF24454D,
            .trail = 0xE68DAC84,
            .grassBlade = {
                .light = 0xFF00322A,
                .dark = 0xFF00261C
            },
            .grassPatch = {
                .dark = 0xFF1F8A4B,
                .middle = 0xFF31A558,
                .light = 0xFF4CC567
            },
            .reedStem = 0xFF00403A,
            .reedHead = 0xFF2E7A5C,
            .stoneLight = 0xFF8AB8D1,
            .stoneDark = 0xFF56798C,
            .stoneCrack = 0xFF00FFFF,
            .shadow = 0x47000033,
            .cowBody = 0xFF8FBC8F,          // Cow body — gray-green (algae on the hide)
            .cowSpots = 0xFF5F9EA0,         // Cow spots — turquoise (corals)
            .cowSnout = 0xFFCD853F,         // Snout — light brown (natural color)
            .cowDetail = 0xFF8B4513,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF005B6A,         // Wolf body — turquoise (underwater coloration)
            .wolfEye = 0xFFFFFF00,          // Eyes — bright yellow (glowing in the dark water)
            .effectColor1 = 0xFF00FFFF,     // Cyan (aquamarine)
            .effectColor2 = 0xFF00FFFF,     // Effect: slowly rising air bubbles reflecting light; rare glowing plankton
            .petal1 = 0xFF00FFFF,
            .petal2 = 0xFF70FDD1,
            .petal3 = 0xFFAEEBFE,
            .flowerHeart = 0xFFE5BF55,
            .wing = 0xFFFFD700,
            .wingEdge = 0xFF102E52,
            .flock = 0xFF112F53,
            .motherDuckBody = 0xFFE8C98F, // sea sand, warm against the turquoise
            .motherDuckWing = 0xFFA97C4A,
            .motherDuckEye = 0xFF14232E,
            .duckLight = 0xFFF7DB5E,
            .duckShade = 0xFFE2AE3A,
            .duckBeak = 0xFFEC8D38,
            .duckBeakShade = 0xFFC46730,
            .fish = 0xFF64C0D3,
            .hedge = 0xFF0A6E6C,
            .mill = {
                .bodyTop = 0xFF7F6747,      // Top part of mill body — brown (submerged wood)
                .bodyBottom = 0xFF593816,   // Lower part of body — dark brown (rot)
                .roofTop = 0xFF539294,      // Top part of roof — turquoise (algae covered roof)
                .roofBottom = 0xFF3A76A7,   // Lower part of roof — steel blue (rust)
                .timber = 0xFF24454D,
                .windowLight = 0xFFFFD700,
                .wingFabric = 0xFF539294,   // Fabric on blades — turquoise (algae)
                .wheel = 0xFF244444         // Mill wheel — dark gray (rust underwater)
            },
            .chase = {
                .treeCrown = 0xFF20B2AA,
                .treeCrownLight = 0xFF28CAC8,
                .treeCrownMiddle = 0xFF23BBAB,
                .pebble = 0xFF7A9671,
                .ourWolf = {
                    .light = 0xFFA0B1CC,
                    .shade = 0xFF798AA7,
                    .coat = 0xFF8C9FBC,
                    .far = 0xFF6E7E9A,
                    .tips = 0xFF26334B,
                    .belly = 0xE6CDCECE,
                    .tongue = 0xFFE97181,
                    .mouth = 0xFFAE708A,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFEFF2F8
                },
                .ourHare = {
                    .light = 0xFFBC9172,
                    .fur = 0xFFAC8466,
                    .shade = 0xFF9B7358,
                    .belly = 0xE6E1DBD1,
                    .tail = 0xFFFCFDFF,
                    .earInside = 0xFFE8A7B3,
                    .nose = 0xFFCF798C,
                    .whisker = 0x80533B33,
                    .mouth = 0xFF532C37,
                    .tooth = 0xFFFCFDFF,
                    .toothEdge = 0xFF9D9EA6
                },
                .theirWolf = {
                    .light = 0xFF38435B,
                    .shade = 0xFF222D42,
                    .coat = 0xFF2D374E,
                    .far = 0xFF1B2337,
                    .tips = 0xFF0F1629,
                    .belly = 0xE674819A,
                    .tongue = 0xFFE57191,
                    .mouth = 0xFF531E37,
                    .tooth = 0xFFEFF5FA,
                    .toothEdge = 0xFF91949A
                },
                .theirHare = {
                    .light = 0xFFDDE7F3,
                    .fur = 0xFFC9D3DF,
                    .shade = 0xFFB5BFCB,
                    .belly = 0xE6FFFFFF,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFFFFEFF,
                    .nose = 0xFFFEEFF2,
                    .whisker = 0x80AA8E83,
                    .mouth = 0xFFAB7D88,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFFBFBFF
                },
                .appleLight = 0xFFFF1460,
                .appleDark = 0xFFB30B41,
                .leaf = 0xFF21BB86,
                .appleFlesh = 0xFFEFF0E6,
                .appleSeed = 0xFF432F2C,
                .crowBody = 0xFF112F53,
                .crowWing = 0xFF021F42,
                .crowWingFar = 0xFF021732,
                .crowBeak = 0xFFFFD700,
                .bootLight = 0xFF457193,
                .bootDark = 0xFF274A64,
                .bootSole = 0xFF15293C,
                .bootPatch = 0xFFFFD700,
                .lace = 0xFF00FFFF,
                .spinesDark = 0xFF563389,
                .spinesLight = 0xFF754BB4,
                .hedgehogFace = 0xFF9AD8E8,
                .hedgehogEar = 0xFF82B7C5,
                .ballPanel1 = 0xFFDF5156,
                .ballPanel2 = 0xFF2C7BE7,
                .ballPanel3 = 0xFFF8D452,
                .ballBase = 0xFFEEF4FE,
                .peel = 0xFFF5D75E,
                .peelShade = 0xFFDBB646,
                .peelInside = 0xFFFBF9E8,
                .peelTip = 0xFF5D4A3C,
                .rakeHandle = 0xFF608E9A,
                .rakeHandleShade = 0xFF426D78,
                .rakeMetal = 0xFFFFD700,
                .capLight = 0xFFFF1460,
                .capDark = 0xFFB30B41,
                .capSpot = 0xFFEEF3F8,
                .stalk = 0xFFE6E7E5,
                .stalkShade = 0xFFC6C2B3,
                .board = 0xFFEB6E59,
                .boardShade = 0xFFAA4334,
                .stumpTop = 0xFF84B5BD,
                .stumpRing = 0xFF548A92,
                .stumpSprout = 0xFF218C65,
                .frogLight = 0xFF2EE6A6,
                .frogDark = 0xFF29A175,
                .frogBelly = 0xFFB8FFE8,
                .shellLight = 0xFFFF7F6A,
                .shellDark = 0xFFB04A3B,
                .shellLine = 0xFFFFF0D0,
                .snailBody = 0xFF70FDD1,
                .fluff = 0xFFC8FFF0,
                .birdBody = 0xFF508DC6,
                .birdBelly = 0xFFF3FCFF,
                .birdWing = 0xFF386A98,
                .birdBeak = 0xFFFEF5CE,
                .dust = 0xFF817F81,
                .dazeStar = 0xFFFFD34D
            },
            .cowTips = 0xFF3A3025,
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .airHaze = 0.30f,
            .celestialGlowStrength = 0.5f
        } {
        }
    };
}
