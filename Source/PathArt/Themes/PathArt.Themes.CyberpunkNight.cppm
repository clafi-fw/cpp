export module ClaFi.PathArt.Themes :CyberpunkNight;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct CyberpunkNight : SceneTheme {
        constexpr CyberpunkNight() : SceneTheme{
            .name = L"Cyberpunk Night",
            .skyTop = 0xFF1A004D,        // Dark purple (top of sky)
            .skyBottom = 0xFF4A0080,     // Magenta (bottom of sky)
            .sunDisk = 0xFFFF00FF,       // Neon magenta (sun)
            .sunGlow = 0x66'FFCCFF,       // Pink glow
            .cloud = 0xCC8A00FF,         // Neon purple clouds
            .mountainTop = 0xFF3E3440,
            .mountainFoot = 0xFF241B26,
            .hillTop = 0xFF00CCFF,       // Neon blue (hill tops)
            .hillBottom = 0xFF0066FF,    // Blue (hill base)
            .riverTop = 0xFF003366,      // Dark blue (river surface)
            .riverBottom = 0xFF001A33,   // Almost black (depths)
            .riverWave = 0xFF00FFFF,     // Neon cyan (waves)
            .foam = 0xBBFFFFFE,          // White foam with slight neon glow
            .treeLeavesBase = 0xFF00FF00, // Bright neon green (leaves)
            .crownLight = 0xFF01FF01,
            .crownMiddle = 0xFF01FF01,
            .bark = 0xFF6C50B1,
            .barkShade = 0xFF463376,
            .bough = 0xFF533C8A,
            .trail = 0xE68ABBA6,
            .grassBlade = {
                .light = 0xFF3D0E79,
                .dark = 0xFF2F095F
            },
            .grassPatch = {
                .dark = 0xFFBB1CB5,
                .middle = 0xFFD640CF,
                .light = 0xFFF762EF
            },
            .reedStem = 0xFF441A83,
            .reedHead = 0xFFFF00C8,
            .stoneLight = 0xFFB0A3F9,
            .stoneDark = 0xFF7467AF,
            .stoneCrack = 0xFF00F0FF,
            .shadow = 0x47000022,
            .cowBody = 0xFFFFA0FF,       // Neon pink (cow body)
            .cowSpots = 0xFF00FFFF,      // Neon cyan (spots)
            .cowSnout = 0xFFFF55FF,      // Light magenta (snout)
            .cowDetail = 0xFFA000FF,     // Medium magenta (details)
            .wolfBody = 0xFF00B8C1,      // Neon cyan (wolf body)
            .wolfEye = 0xFFFFFF00,       // Yellow neon glow (eyes)
            .effectColor1 = 0xFFFFFFFF,
            .effectColor2 = 0xFFFFFFFF,
            .petal1 = 0xFFFF00FF,
            .petal2 = 0xFF39FF14,
            .petal3 = 0xFFE6FF1A,
            .flowerHeart = 0xFFEABD4F,
            .wing = 0xFFF97BF6,
            .wingEdge = 0xFF00E5FF,
            .flock = 0xFF291A4D,
            .motherDuckBody = 0xFF7B4ED6,
            .motherDuckWing = 0xFF5932A1,
            .motherDuckEye = 0xFFA000FF,
            .duckLight = 0xFFFDD958,
            .duckShade = 0xFFE7AB33,
            .duckBeak = 0xFFF08A32,
            .duckBeakShade = 0xFFC8642A,
            .fish = 0xFF6064B3,
            .hedge = 0xFF009A72,
            .mill = {
                .bodyTop = 0xFF330066,   // Dark purple (top of mill body)
                .bodyBottom = 0xFF6600CC, // Magenta (bottom of body)
                .roofTop = 0xFFCC00FF,   // Bright magenta (roof top)
                .roofBottom = 0xFF9900CC, // Dark magenta (roof bottom)
                .timber = 0xFF533C8A,
                .windowLight = 0xFF00FFFF,
                .wingFabric = 0xFF00FFFF, // Neon cyan (fabric of wings)
                .wheel = 0xFF212121      // Black (mill wheel)
            },
            .chase = {
                .treeCrown = 0xFF00FF00,
                .treeCrownLight = 0xFF01FF01,
                .treeCrownMiddle = 0xFF01FF01,
                .pebble = 0xFF76A492,
                .ourWolf = {
                    .light = 0xFF7A7C97,
                    .shade = 0xFF575871,
                    .coat = 0xFF696B85,
                    .far = 0xFF4D4E65,
                    .tips = 0xFF3B3B52,
                    .belly = 0xE6B0B0C8,
                    .tongue = 0xFFEE6A8B,
                    .mouth = 0xFF5A1732,
                    .tooth = 0xFFFCF0F4,
                    .toothEdge = 0xFF9D9094
                },
                .ourHare = {
                    .light = 0xFFC58C6B,
                    .fur = 0xFFB57F5F,
                    .shade = 0xFFA46E51,
                    .belly = 0xE6EDD7CA,
                    .tail = 0xFFFEFCFF,
                    .earInside = 0xFFF2A2AD,
                    .nose = 0xFFD87386,
                    .whisker = 0x805B372D,
                    .mouth = 0xFF5A2732,
                    .tooth = 0xFFFEFCFF,
                    .toothEdge = 0xFFA99AA0
                },
                .theirWolf = {
                    .light = 0xFF5C596F,
                    .shade = 0xFF444156,
                    .coat = 0xFF4F4C62,
                    .far = 0xFF3B374B,
                    .tips = 0xFF2D293C,
                    .belly = 0xE69B99B0,
                    .tongue = 0xFFF8819C,
                    .mouth = 0xFF69253F,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFAC9FA3
                },
                .theirHare = {
                    .light = 0xFF8E8891,
                    .fur = 0xFF7C767E,
                    .shade = 0xFF6A646C,
                    .belly = 0xE6C9C1CA,
                    .tail = 0xFFFEFCFF,
                    .earInside = 0xFFF2A2AD,
                    .nose = 0xFFD87386,
                    .whisker = 0x805B372D,
                    .mouth = 0xFF5A2732,
                    .tooth = 0xFFFEFCFF,
                    .toothEdge = 0xFFA99AA0
                },
                .appleLight = 0xFFFF1460,
                .appleDark = 0xFFB30B41,
                .leaf = 0xFF2ED211,
                .appleFlesh = 0xFFFBECDF,
                .appleSeed = 0xFF4A2B27,
                .crowBody = 0xFF291A4D,
                .crowWing = 0xFF1B0A3C,
                .crowWingFar = 0xFF12062C,
                .crowBeak = 0xFFDBF23D,
                .bootLight = 0xFF6E5EA8,
                .bootDark = 0xFF473F68,
                .bootSole = 0xFF282144,
                .bootPatch = 0xFFFF00FF,
                .lace = 0xFF4BF1FE,
                .spinesDark = 0xFF5C28C6,
                .spinesLight = 0xFF7A3CFF,
                .hedgehogFace = 0xFFD0B0FF,
                .hedgehogEar = 0xFFAF94D7,
                .ballPanel1 = 0xFFE6474F,
                .ballPanel2 = 0xFF4478E1,
                .ballPanel3 = 0xFFFED14C,
                .ballBase = 0xFFF9F0FC,
                .peel = 0xFFFBD558,
                .peelShade = 0xFFE0B340,
                .peelInside = 0xFFFEF7EB,
                .peelTip = 0xFF664636,
                .rakeHandle = 0xFF8D6EDD,
                .rakeHandleShade = 0xFF6C4FB3,
                .rakeMetal = 0xFFFF00FF,
                .capLight = 0xFFFF1460,
                .capDark = 0xFFB30B41,
                .capSpot = 0xFFFBEEF2,
                .stalk = 0xFFF2E2DF,
                .stalkShade = 0xFFD2BEAD,
                .board = 0xFFFF1FD2,
                .boardShade = 0xFFB30792,
                .stumpTop = 0xFFCCABFA,
                .stumpRing = 0xFFAC65F9,
                .stumpSprout = 0xFF30B41D,
                .frogLight = 0xFF39FF14,
                .frogDark = 0xFF35B724,
                .frogBelly = 0xFFE6FF1A,
                .shellLight = 0xFFFF2BD6,
                .shellDark = 0xFFA11E86,
                .shellLine = 0xFF00E5FF,
                .snailBody = 0xFFC0A8FF,
                .fluff = 0xFFF3DFFF,
                .birdBody = 0xFF7B4ED6,
                .birdBelly = 0xFF9FF0FF,
                .birdWing = 0xFF5932A1,
                .birdBeak = 0xFFE6FF1A,
                .dust = 0xFF8C7B7B,
                .dazeStar = 0xFFFFD34D
            },
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
}
