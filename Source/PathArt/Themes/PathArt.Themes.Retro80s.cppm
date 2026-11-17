export module ClaFi.PathArt.Themes :Retro80s;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct Retro80s : SceneTheme {
        constexpr Retro80s() : SceneTheme{
            .name = L"Retro 80s",
            .skyTop = 0xFF7B68EE,           // Lilac top sky (pastel gradient)
            .skyBottom = 0xFFFFDAB9,        // Peach horizon (80s sunset)
            .sunDisk = 0xFFFF6969,          // Sun core — coral
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — very light peach
            .cloud = 0xCCE6E6FA,            // Clouds — light gray with pastel tint
            .mountainTop = 0xFF8B797F,
            .mountainFoot = 0xFF625157,
            .hillTop = 0xFF90EE90,          // Hilltops — light green (neon tint)
            .hillBottom = 0xFF32CD32,       // Lower parts of hills — bright green
            .riverTop = 0xFF00BFFF,         // Top part of water — blue (neon)
            .riverBottom = 0xFF1E90FF,      // River depths — dark blue
            .riverWave = 0xFF7B68EE,        // Waves — lilac (sky reflection)
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .treeLeavesBase = 0xFF228B22,   // Leaves base — dark green
            .crownLight = 0xFF2B9E29,
            .crownMiddle = 0xFF259223,
            .bark = 0xFF814B34,
            .barkShade = 0xFF572E20,
            .bough = 0xFF653727,
            .trail = 0xE6CED78A,
            .grassBlade = {
                .light = 0xFF4CA95C,
                .dark = 0xFF479450
            },
            .grassPatch = {
                .dark = 0xFF38954C,
                .middle = 0xFF4CB25E,
                .light = 0xFF6AD572
            },
            .reedStem = 0xFF5FA961,
            .reedHead = 0xFF734330,
            .stoneLight = 0xFFAE9999,
            .stoneDark = 0xFF756363,
            .stoneCrack = 0xFF5D4B4D,
            .shadow = 0x47333333,
            .cowBody = 0xFFFDC3AC,          // Cow body — peach (neon coloring)
            .cowSpots = 0xFF7A7BE4,         // Cow spots — lilac
            .cowSnout = 0xFFE49B56,         // Snout — light brown
            .cowDetail = 0xFFA1592B,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF443525,         // Wolf body — dark gray (with neon highlights)
            .wolfEye = 0xFFFF00FF,          // Eyes — bright magenta (neon glow)
            .effectColor1 = 0xFFFF00FF,     // Magenta (neon pink)
            .effectColor2 = 0xFFFF00FF,     // Effect: fast flashes of neon light (like signs and a disco ball), strobe effect
            .petal1 = 0xFFF5E1E8,
            .petal2 = 0xFFFDCFDB,
            .petal3 = 0xFFE9CEF9,
            .flowerHeart = 0xFFEDBC46,
            .wing = 0xFFE08A46,
            .wingEdge = 0xFF3B1F1D,
            .flock = 0xFF302433,
            .motherDuckBody = 0xFF845946,
            .motherDuckWing = 0xFF623C2F,
            .motherDuckEye = 0xFFA1592B,
            .duckLight = 0xFFDDB42E,
            .duckShade = 0xFFC38C22,
            .duckBeak = 0xFFC96F20,
            .duckBeakShade = 0xFFA44D17,
            .fish = 0xFF3A5764,
            .hedge = 0xFF6E2E6E,
            .mill = {
                .bodyTop = 0xFF7A2F02,      // Top part of mill body — brown (wooden texture)
                .bodyBottom = 0xFF5E2901,   // Lower part of body — dark brown
                .roofTop = 0xFF520503,      // Top part of roof — dark red (faded tiles)
                .roofBottom = 0xFF7D0B0F,   // Lower part of roof — burgundy
                .timber = 0xFF653727,
                .wingFabric = 0xFF4B35A5,   // Fabric on blades — lilac (neon fabric)
                .wheel = 0xFF0F2F30         // Mill wheel — dark gray
            },
            .chase = {
                .treeCrown = 0xFF228B22,
                .treeCrownLight = 0xFF2B9E29,
                .treeCrownMiddle = 0xFF259223,
                .pebble = 0xFFB7BD73,
                .ourWolf = {
                    .light = 0xFF7D7182,
                    .shade = 0xFF5B505F,
                    .coat = 0xFF6D6272,
                    .far = 0xFF524754,
                    .tips = 0xFF413643,
                    .belly = 0xE6AEA0AD,
                    .tongue = 0xFFD95F75,
                    .mouth = 0xFF541125,
                    .tooth = 0xFFEED6D2,
                    .toothEdge = 0xFF977E7B
                },
                .ourHare = {
                    .light = 0xFFBB7E5C,
                    .fur = 0xFFAC7151,
                    .shade = 0xFF9D6244,
                    .belly = 0xE6E3C2B0,
                    .tail = 0xFFF3DEE5,
                    .earInside = 0xFFDF9093,
                    .nose = 0xFFC76670,
                    .whisker = 0x80572E1F,
                    .mouth = 0xFF551F24,
                    .tooth = 0xFFF3DEE5,
                    .toothEdge = 0xFFA28786
                },
                .theirWolf = {
                    .light = 0xFF4F414D,
                    .shade = 0xFF392C37,
                    .coat = 0xFF443641,
                    .far = 0xFF31232E,
                    .tips = 0xFF241821,
                    .belly = 0xE6887986,
                    .tongue = 0xFFD95F75,
                    .mouth = 0xFF541125,
                    .tooth = 0xFFEED6D2,
                    .toothEdge = 0xFF977E7B
                },
                .theirHare = {
                    .light = 0xFFAC9899,
                    .fur = 0xFF9A8788,
                    .shade = 0xFF897677,
                    .belly = 0xE6E4CED0,
                    .tail = 0xFFF3DEE5,
                    .earInside = 0xFFDF9093,
                    .nose = 0xFFC76670,
                    .whisker = 0x80572E1F,
                    .mouth = 0xFF551F24,
                    .tooth = 0xFFF3DEE5,
                    .toothEdge = 0xFFA28786
                },
                .appleLight = 0xFFD13F3F,
                .appleDark = 0xFF931924,
                .leaf = 0xFF618B4A,
                .appleFlesh = 0xFFEDD2C0,
                .appleSeed = 0xFF48231A,
                .crowBody = 0xFF302433,
                .crowWing = 0xFF231723,
                .crowWingFar = 0xFF1C101B,
                .crowBeak = 0xFF4E3F4B,
                .bootLight = 0xFF82543E,
                .bootDark = 0xFF5A3225,
                .bootSole = 0xFF311C1E,
                .bootPatch = 0xFF97674C,
                .lace = 0xFFDBBCA6,
                .spinesDark = 0xFF59392E,
                .spinesLight = 0xFF775142,
                .hedgehogFace = 0xFF916450,
                .hedgehogEar = 0xFF744B3D,
                .ballPanel1 = 0xFFD13F3F,
                .ballPanel2 = 0xFF516DBB,
                .ballPanel3 = 0xFFEDBD36,
                .ballBase = 0xFFECD6D8,
                .peel = 0xFFDEB433,
                .peelShade = 0xFFC59617,
                .peelInside = 0xFFE9D4AD,
                .peelTip = 0xFF57321E,
                .rakeHandle = 0xFFA46846,
                .rakeHandleShade = 0xFF814B31,
                .rakeMetal = 0xFF6F626E,
                .capLight = 0xFFCE3C3F,
                .capDark = 0xFF981B24,
                .capSpot = 0xFFEDD4D0,
                .stalk = 0xFFE5CABF,
                .stalkShade = 0xFFC7A893,
                .board = 0xFFB37654,
                .boardShade = 0xFF814B34,
                .stumpTop = 0xFFCCA081,
                .stumpRing = 0xFFA57657,
                .stumpSprout = 0xFF517536,
                .frogLight = 0xFF91AB49,
                .frogDark = 0xFF5A7531,
                .frogBelly = 0xFFDCCEA4,
                .shellLight = 0xFFC07649,
                .shellDark = 0xFF844127,
                .shellLine = 0xFF642E1C,
                .snailBody = 0xFFBF9E8D,
                .fluff = 0xFFE5CBC4,
                .birdBody = 0xFF845946,
                .birdBelly = 0xFFDDBEAF,
                .birdWing = 0xFF623C2F,
                .birdBeak = 0xFFD38C45,
                .dust = 0xFFDDBEA9,
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
