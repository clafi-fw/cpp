export module ClaFi.PathArt.Themes :LunarEclipse;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct LunarEclipse : SceneTheme {
        constexpr LunarEclipse() : SceneTheme{
            .name = L"Lunar Eclipse",
            .skyTop = 0xFF333366,           // Dark blue top sky (night sky)
            .skyBottom = 0xFF663399,        // Purple horizon (eclipse effect)
            .sunDisk = 0xFFFF99CC,          // Core of the "moon" — light pink (eclipsed moon)
            .sunGlow = 0x66'CCCCFF,          // Glow around the "moon" — light purple
            .cloud = 0xCCCCCCFF,            // Clouds — light gray with purple tint
            .mountainTop = 0xFF584C58,
            .mountainFoot = 0xFF3C313C,
            .hillTop = 0xFF333333,          // Hilltops — almost black
            .hillBottom = 0xFF666666,       // Lower parts of hills — dark gray
            .riverTop = 0xFF494B84,
            .riverBottom = 0xFF242443,
            .riverWave = 0xFF6666CC,        // Waves — gray-purple
            .foam = 0xBAFFFFFF,             // Foam — pure white
            .treeLeavesBase = 0xFF333333,   // Leaves base — dark (barely visible at night)
            .crownLight = 0xFF403A3D,
            .crownMiddle = 0xFF383634,
            .bark = 0xFF5E807F,
            .barkShade = 0xFF4B6665,
            .bough = 0xFF594842,
            .trail = 0xE65E523C,
            .grassBlade = {
                .light = 0xFF7F937D,
                .dark = 0xFF637863
            },
            .grassPatch = {
                .dark = 0xFF697A68,
                .middle = 0xFF80927F,
                .light = 0xFF9DAF9B
            },
            .reedStem = 0xFF6D8073,
            .reedHead = 0xFF5E807F,
            .stoneLight = 0xFFB7ADB5,
            .stoneDark = 0xFF797178,
            .stoneCrack = 0xFF5F575F,
            .shadow = 0x47000000,
            .cowBody = 0xFF666666,          // Cow body — dark gray
            .cowSpots = 0xFF333333,         // Cow spots — black
            .cowSnout = 0xFF996666,         // Snout — gray-brown
            .cowDetail = 0xFF666666,        // Details (nostrils, eyes) — dark gray
            .wolfBody = 0xFF292929,         // Wolf body — black
            .wolfEye = 0xFFFF9999,          // Eyes — glowing pink
            .effectColor1 = 0xFF9966CC,     // Purple-pink, like the glow of an eclipsed moon
            .effectColor2 = 0xFF9966CC,     // Effect: slowly falling glowing particles
            .petal1 = 0xFFA07EDC,
            .petal2 = 0xFF7E8DD8,
            .petal3 = 0xFFBC73C3,
            .flowerHeart = 0xFFEBBD4E,
            .wing = 0xFF8AAEAD,
            .wingEdge = 0xFF769291,
            .flock = 0xFF2F2B43,
            .motherDuckBody = 0xFF5E807F,
            .motherDuckWing = 0xFF4B6665,
            .motherDuckEye = 0xFF666666,
            .duckLight = 0xFFFED957,
            .duckShade = 0xFFE7AB31,
            .duckBeak = 0xFFF18930,
            .duckBeakShade = 0xFFC96329,
            .fish = 0xFF8780C4,
            .hedge = 0xFF4F4F4F,
            .mill = {
                .bodyTop = 0xFF616161,      // Top part of mill body — black
                .bodyBottom = 0xFF989898,   // Lower part of body — dark gray
                .roofTop = 0xFF865878,
                .roofBottom = 0xFF513849,
                .timber = 0xFF594842,
                .windowLight = 0xFFFF8C66,
                .wingFabric = 0xFFCC9EFE,   // Fabric on blades — dark purple
                .wheel = 0xFF5E807F         // Mill wheel — dark gray
            },
            .chase = {
                .treeCrown = 0xFF6D7F6C,
                .treeCrownLight = 0xFF7F917E,
                .treeCrownMiddle = 0xFF748673,
                .pebble = 0xFF524531,
                .ourWolf = {
                    .light = 0xFFB7A1D4,
                    .shade = 0xFF786493,
                    .coat = 0xFFA58FC1,
                    .far = 0xFF84709F,
                    .tips = 0xFF705C8A,
                    .belly = 0xE6EADCFF,
                    .tongue = 0xFFFEACBB,
                    .mouth = 0xFF843C55,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFC9B9BD
                },
                .ourHare = {
                    .light = 0xFFC48D61,
                    .fur = 0xFFB48055,
                    .shade = 0xFFA36F47,
                    .belly = 0xE6E2D3B8,
                    .tail = 0xFFFEFCFF,
                    .earInside = 0xFFF3A1AB,
                    .nose = 0xFFD97385,
                    .whisker = 0x805C372C,
                    .mouth = 0xFF5B2631,
                    .tooth = 0xFFFEFCFF,
                    .toothEdge = 0xFFAA999F
                },
                .theirWolf = {
                    .light = 0xFF88849B,
                    .shade = 0xFF6E6A81,
                    .coat = 0xFF7B778D,
                    .far = 0xFF655F74,
                    .tips = 0xFF565064,
                    .belly = 0xE6CBC8DF,
                    .tongue = 0xFFFFCDD5,
                    .mouth = 0xFF994F67,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFDECED1
                },
                .theirHare = {
                    .light = 0xFFAEA6AE,
                    .fur = 0xFF9B939B,
                    .shade = 0xFF888188,
                    .belly = 0xE6EAE1E9,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFFCCED3,
                    .nose = 0xFFFB92A3,
                    .whisker = 0x807A5246,
                    .mouth = 0xFF79414B,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFCAB9BE
                },
                .appleLight = 0xFFE7464D,
                .appleDark = 0xFFA21E30,
                .leaf = 0xFF559F5A,
                .appleFlesh = 0xFFFDEBDE,
                .appleSeed = 0xFF4B2B26,
                .crowBody = 0xFF2F2B43,
                .crowWing = 0xFF201C31,
                .crowWingFar = 0xFF191427,
                .crowBeak = 0xFF4E495E,
                .bootLight = 0xFF8B614D,
                .bootDark = 0xFF603C31,
                .bootSole = 0xFF31232A,
                .bootPatch = 0xFFA1775C,
                .lace = 0xFFE9D3C1,
                .spinesDark = 0xFF644841,
                .spinesLight = 0xFF846358,
                .hedgehogFace = 0xFFA27867,
                .hedgehogEar = 0xFF815C52,
                .ballPanel1 = 0xFFE7464D,
                .ballPanel2 = 0xFF4678E0,
                .ballPanel3 = 0xFFFFD14A,
                .ballBase = 0xFFFAF0FA,
                .peel = 0xFFFCD557,
                .peelShade = 0xFFE0B33E,
                .peelInside = 0xFFFFF7EB,
                .peelTip = 0xFF674635,
                .rakeHandle = 0xFFB07654,
                .rakeHandleShade = 0xFF8B563F,
                .rakeMetal = 0xFF717086,
                .capLight = 0xFFE4434D,
                .capDark = 0xFFA72030,
                .capSpot = 0xFFFCEEF0,
                .stalk = 0xFF8AAEAD,
                .stalkShade = 0xFF769291,
                .board = 0xFFC08664,
                .boardShade = 0xFF8B5641,
                .stumpTop = 0xFFDBB497,
                .stumpRing = 0xFFB18568,
                .stumpSprout = 0xFF468744,
                .frogLight = 0xFF5E807F,
                .frogDark = 0xFF4B6665,
                .frogBelly = 0xFFC1C197,
                .shellLight = 0xFFD08657,
                .shellDark = 0xFF8E4B33,
                .shellLine = 0xFF6C3628,
                .snailBody = 0xFFCBB2A5,
                .fluff = 0xFFF5E5E5,
                .birdBody = 0xFF5E807F,
                .birdBelly = 0xFFEBD5CB,
                .birdWing = 0xFF4B6665,
                .birdBeak = 0xFFE59E52,
                .dust = 0xFF8D7B79,
                .dazeStar = 0xFFFF99CC
            },
            .hasRain = false,
            .hasFireflies = true,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = false,
            .isAfrica = false,
            .hasStars = true,
            .celestialGlowStrength = 0.5f
        } {
        }
    };
}
