export module ClaFi.PathArt.Themes :MidnightFantasy;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct MidnightFantasy : SceneTheme {
        constexpr MidnightFantasy() : SceneTheme{
            .name = L"Midnight Fantasy",
            .skyTop = 0xFF000033,        // Deep dark blue (almost black)
            .skyBottom = 0xFF1A1A50,     // Dark purple
            .sunDisk = 0xFFFFF0FF,       // Pale lilac (moon)
            .sunGlow = 0xFF8A2BE2,       // Purple glow
            .cloud = 0xCC4B0082,         // Dark purple clouds
            .mountainTop = 0xFF48424B,
            .mountainFoot = 0xFF2D2830,
            .hillTop = 0xFF2F4F4F,       // Dark green (fir tree color)
            .hillBottom = 0xFF0A3A3A,    // Very dark green
            .riverTop = 0xFF001A33,      // Deep blue
            .riverBottom = 0xFF000F1A,   // Almost black
            .riverWave = 0xFF6699CC,     // Turquoise waves
            .foam = 0xBBFFFFFF,          // White foam
            .treeLeavesBase = 0xFF263238, // Dark turquoise
            .crownLight = 0xFF303942,
            .crownMiddle = 0xFF2A3539,
            .bark = 0xFF526294,
            .barkShade = 0xFF343F61,
            .bough = 0xFF3E4A72,
            .trail = 0xE66B684F,
            .grassBlade = {
                .light = 0xFF052521,
                .dark = 0xFF041F18
            },
            .grassPatch = {
                .dark = 0xFF12322A,
                .middle = 0xFF193B34,
                .light = 0xFF23473F
            },
            .reedStem = 0xFF1F3836,
            .reedHead = 0xFF6E60A8,
            .stoneLight = 0xFF515D83,
            .stoneDark = 0xFF1D2749,
            .stoneCrack = 0xFFCBC4FD,
            .shadow = 0x47000022,
            .cowBody = 0xFFFFFEF0,       // Cream (not quite white)
            .cowSpots = 0xFF333366,      // Dark blue instead of black
            .cowSnout = 0xFFA18876,      // Lilac snout
            .cowDetail = 0xFF755D52,     // Warm brown
            .wolfBody = 0xFF493436,      // Chocolate (not scary)
            .wolfEye = 0xFFFFFF00,       // Yellow glowing eyes (not red)
            .effectColor1 = 0xFFD2B48C,  // Pale lavender-white for a magical, ethereal glow
            .effectColor2 = 0xFFFFF0FF,  // Very light lilac, almost white (secondary glow)
            .petal1 = 0xFFECE4F0,
            .petal2 = 0xFFFDCFE1,
            .petal3 = 0xFFE0D2FD,
            .flowerHeart = 0xFFE9BE4E,
            .wing = 0xFFE4D6FF,
            .wingEdge = 0xFF4B2A8A,
            .flock = 0xFF585C87,
            .motherDuckBody = 0xFF5E5EA8,
            .motherDuckWing = 0xFF403F7B,
            .motherDuckEye = 0xFF755D52,
            .duckLight = 0xFFE9C64C,
            .duckShade = 0xFFD49C29,
            .duckBeak = 0xFFDD7D28,
            .duckBeakShade = 0xFFB75A22,
            .fish = 0xFF465A82,
            .hedge = 0xFF173639,
            .mill = {
                .bodyTop = 0xFF6C5843,   // Dark chocolate
                .bodyBottom = 0xFF8F6F65, // Brown-red
                .roofTop = 0xFF7F625B,   // Dark brown
                .roofBottom = 0xFFA07F74, // Burgundy
                .timber = 0xFF3E4A72,
                .windowLight = 0xFFE8C8FF,
                .wingFabric = 0x887F64E1, // Purple sail
                .wheel = 0xFF4C4C4C      // Black
            },
            .chase = {
                .treeCrown = 0xFF263238,
                .treeCrownLight = 0xFF303942,
                .treeCrownMiddle = 0xFF2A3539,
                .pebble = 0xFF5D5941,
                .ourWolf = {
                    .light = 0xFF807399,
                    .shade = 0xFF675A7F,
                    .coat = 0xFF74678D,
                    .far = 0xFF605478,
                    .tips = 0xFF53476A,
                    .belly = 0xE6A197B5,
                    .tongue = 0xFFE8A4B4,
                    .mouth = 0xFF886270,
                    .tooth = 0xFFFCF2F8,
                    .toothEdge = 0xFFBBB0B6
                },
                .ourHare = {
                    .light = 0xFF97796E,
                    .fur = 0xFF8D7066,
                    .shade = 0xFF82655C,
                    .belly = 0xE6B5A7A5,
                    .tail = 0xFFC2BAC6,
                    .earInside = 0xFFAF878F,
                    .nose = 0xFF9E6C77,
                    .whisker = 0x80523E3C,
                    .mouth = 0xFF51353C,
                    .tooth = 0xFFC2BAC6,
                    .toothEdge = 0xFF887D84
                },
                .theirWolf = {
                    .light = 0xFF5A5462,
                    .shade = 0xFF494451,
                    .coat = 0xFF514C5A,
                    .far = 0xFF433D4A,
                    .tips = 0xFF393440,
                    .belly = 0xE6837E8D,
                    .tongue = 0xFFE8A4B4,
                    .mouth = 0xFF886270,
                    .tooth = 0xFFFCF2F8,
                    .toothEdge = 0xFFBBB0B6
                },
                .theirHare = {
                    .light = 0xFF8D868E,
                    .fur = 0xFF817A82,
                    .shade = 0xFF756D75,
                    .belly = 0xE6B4ACB4,
                    .tail = 0xFFBFB7C3,
                    .earInside = 0xFFAC848C,
                    .nose = 0xFF9A6974,
                    .whisker = 0x80503B3A,
                    .mouth = 0xFF4E323A,
                    .tooth = 0xFFBFB7C3,
                    .toothEdge = 0xFF857A81
                },
                .appleLight = 0xFFBF4648,
                .appleDark = 0xFF85242C,
                .leaf = 0xFF5ABEA4,
                .appleFlesh = 0xFFD9CBC0,
                .appleSeed = 0xFF3E2420,
                .crowBody = 0xFF585C87,
                .crowWing = 0xFF454972,
                .crowWingFar = 0xFF3B3E67,
                .crowBeak = 0xFFD3B37C,
                .bootLight = 0xFF646795,
                .bootDark = 0xFF404266,
                .bootSole = 0xFF24253D,
                .bootPatch = 0xFFE8C890,
                .lace = 0xFFE0D2FD,
                .spinesDark = 0xFF646497,
                .spinesLight = 0xFF8080C0,
                .hedgehogFace = 0xFFDCD4F4,
                .hedgehogEar = 0xFFBCB5D0,
                .ballPanel1 = 0xFFB74547,
                .ballPanel2 = 0xFF4265AF,
                .ballPanel3 = 0xFFE9BF40,
                .ballBase = 0xFFD1C8D0,
                .peel = 0xFFDFBB43,
                .peelShade = 0xFFC69C2C,
                .peelInside = 0xFFE9DBB6,
                .peelTip = 0xFF55392A,
                .rakeHandle = 0xFF7082BB,
                .rakeHandleShade = 0xFF526195,
                .rakeMetal = 0xFFC8C0FF,
                .capLight = 0xFFB14E4D,
                .capDark = 0xFF7E2F31,
                .capSpot = 0xFFDED6D6,
                .stalk = 0xFFD5CAC3,
                .stalkShade = 0xFFB5A692,
                .board = 0xFFE0D2FD,
                .boardShade = 0xFFAB9FC3,
                .stumpTop = 0xFF9AA8D0,
                .stumpRing = 0xFF6E7CA6,
                .stumpSprout = 0xFF45A58C,
                .frogLight = 0xFF93C8B8,
                .frogDark = 0xFF689285,
                .frogBelly = 0xFFE4F4F0,
                .shellLight = 0xFFCDB4F0,
                .shellDark = 0xFF8E7AA9,
                .shellLine = 0xFF3A3A70,
                .snailBody = 0xFFFDCFE1,
                .fluff = 0xFFFFF0FF,
                .birdBody = 0xFF5E5EA8,
                .birdBelly = 0xFFFFF0FF,
                .birdWing = 0xFF403F7B,
                .birdBeak = 0xFFE8C890,
                .dust = 0xFF897D79,
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
