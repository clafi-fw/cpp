export module ClaFi.PathArt.Themes :SteampunkAdventure;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct SteampunkAdventure : SceneTheme {
        constexpr SteampunkAdventure() : SceneTheme{
            .name = L"Steampunk",
            .skyTop = 0xFF797979,           // Gray sky (smoky industrial landscape)
            .skyBottom = 0xFFD3D3D3,        // Light gray horizon (smog)
            .sunDisk = 0xFFFF9966,          // Sun core — rusty orange (through haze)
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCCCCCCC,            // Clouds — gray, industrial
            .mountainTop = 0xFF7D7774,
            .mountainFoot = 0xFF55504D,
            .hillTop = 0xFF64514A,          // Hilltops - gray-brown (soil, coal)
            .hillBottom = 0xFF574642,       // Lower parts of hills - dark gray-brown (mines)
            .riverTop = 0xFF666666,         // Top part of water — gray (polluted river)
            .riverBottom = 0xFF333333,      // River depths — black (murky water)
            .riverWave = 0xFF8B4513,        // Waves — brown (mud, silt)
            .foam = 0xBBCCCCCC,             // Foam — gray (soapy, from industrial waste)
            .treeLeavesBase = 0xFF618479,   // Leaves base - gray verdigris
            .crownLight = 0xFF6E9488,
            .crownMiddle = 0xFF668A7E,
            .bark = 0xFF483013,
            .barkShade = 0xFF2A1904,
            .bough = 0xFFA07A50,
            .trail = 0xE67D6D5D,
            .grassBlade = {
                .light = 0xFF41321F,
                .dark = 0xFF352D1B
            },
            .grassPatch = {
                .dark = 0xFF302C19,
                .middle = 0xFF41351F,
                .light = 0xFF5A3F26
            },
            .reedStem = 0xFF513221,
            .reedHead = 0xFFC88A48,
            .stoneLight = 0xFF64646C,
            .stoneDark = 0xFF2E2E35,
            .stoneCrack = 0xFFBF793A,
            .shadow = 0x47333333,
            .cowBody = 0xFFDEE1DC,          // Cow body — gray-brown (dusty fur)
            .cowSpots = 0xFF7A7A7A,         // Cow spots — dark gray (soot-covered)
            .cowSnout = 0xFFE19853,         // Snout — light brown (grease marks)
            .cowDetail = 0xFF9E5628,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF3E2F20,         // Wolf body — dark gray (metallic tint)
            .wolfEye = 0xFFFFA07A,          // Eyes — glowing peach (mechanical implants)
            .effectColor1 = 0xFF7B4630,     // Rust brown (the blown dust)
            .effectColor2 = 0xFFF9F4F2,
            .petal1 = 0xFFECE6E2,
            .petal2 = 0xFFF1D6D6,
            .petal3 = 0xFFE0D6E4,
            .flowerHeart = 0xFFD7C28E,
            .wing = 0xFFD4A84E,
            .wingEdge = 0xFF7A4A1E,
            .flock = 0xFF2C2C32,
            .motherDuckBody = 0xFFB5834F, // warm tan, clear of the river's gray
            .motherDuckWing = 0xFF82542C,
            .motherDuckEye = 0xFF2A1C10,
            .duckLight = 0xFFDAC890,
            .duckShade = 0xFFBEA371,
            .duckBeak = 0xFFBD8E68,
            .duckBeakShade = 0xFF9A6C51,
            .fish = 0xFF97979E,
            .hedge = 0xFF4B3A34,
            .mill = {
                .bodyTop = 0xFFF2F0E9,     // the lit wall
                .bodyBottom = 0xFFDCD6CA,  // the wall in its own shade
                .roofTop = 0xFFBC4C3E,     // brick, where the light falls
                .roofBottom = 0xFF8F3428,  // brick under the eave
                .timber = 0xFF9A8262,      // the stocks, the frames, the trough
                .windowLight = 0xFFF3C68B, // the lamp behind the windows
                .wingFabric = 0x6E9C95CC,  // the sail, thin over what stands behind it
                .wheel = 0xFF8FB3BE        // the wheel, wet
            },
            .chase = {
                .treeCrown = 0xFF618479,
                .treeCrownLight = 0xFF6E9488,
                .treeCrownMiddle = 0xFF668A7E,
                .pebble = 0xFF6F5E4F,
                .ourWolf = {
                    .light = 0xFF464445,
                    .shade = 0xFF292627,
                    .coat = 0xFF383637,
                    .far = 0xFF211E1E,
                    .tips = 0xFF121010,
                    .belly = 0xE6726E6E,
                    .tongue = 0xFFB07A7A,
                    .mouth = 0xFF402424,
                    .tooth = 0xFFE0D7CE,
                    .toothEdge = 0xFF8A8179
                },
                .ourHare = {
                    .light = 0xFFE9CDB4,
                    .fur = 0xFFDBBFA8,
                    .shade = 0xFFCBAF99,
                    .belly = 0xE6FFFFFE,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFFEEAE6,
                    .nose = 0xFFEDBFBA,
                    .whisker = 0x80877365,
                    .mouth = 0xFF836862,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFDBD1C8
                },
                .theirWolf = {
                    .light = 0xFF292525,
                    .shade = 0xFF161312,
                    .coat = 0xFF1F1B1B,
                    .far = 0xFF0E0B0A,
                    .tips = 0xFF050303,
                    .belly = 0xE65D5958,
                    .tongue = 0xFFB07A7A,
                    .mouth = 0xFF402424,
                    .tooth = 0xFFE0D7CE,
                    .toothEdge = 0xFF8A8179
                },
                .theirHare = {
                    .light = 0xFFDED8D1,
                    .fur = 0xFFCCC6BF,
                    .shade = 0xFFBAB4AD,
                    .belly = 0xE6FFFFFF,
                    .tail = 0xFFFFFFFF,
                    .earInside = 0xFFFEDCD5,
                    .nose = 0xFFE3B5B1,
                    .whisker = 0x807E6B5C,
                    .mouth = 0xFF7A605A,
                    .tooth = 0xFFFFFFFF,
                    .toothEdge = 0xFFD1C7BF
                },
                .appleLight = 0xFFE6443D,
                .appleDark = 0xFFA12623,
                .leaf = 0xFF458A77,
                .appleFlesh = 0xFFECE2D2,
                .appleSeed = 0xFF45362B,
                .crowBody = 0xFF2C2C32,
                .crowWing = 0xFF1D1D23,
                .crowWingFar = 0xFF15151B,
                .crowBeak = 0xFFC9A04A,
                .bootLight = 0xFF6E4736,
                .bootDark = 0xFF432518,
                .bootSole = 0xFF312E2C,
                .bootPatch = 0xFFE5BB66,
                .lace = 0xFFD48D4E,
                .spinesDark = 0xFF44444C,
                .spinesLight = 0xFF60606A,
                .hedgehogFace = 0xFFC8A882,
                .hedgehogEar = 0xFFA68B6A,
                .ballPanel1 = 0xFFB37366,
                .ballPanel2 = 0xFF6B7E9C,
                .ballPanel3 = 0xFFE6D198,
                .ballBase = 0xFFEDE6E0,
                .peel = 0xFFCFBE87,
                .peelShade = 0xFFB5A06F,
                .peelInside = 0xFFE4DBC5,
                .peelTip = 0xFF4D3D2D,
                .rakeHandle = 0xFF483013,
                .rakeHandleShade = 0xFF2A1904,
                .rakeMetal = 0xFF58441C,
                .capLight = 0xFFF66752,
                .capDark = 0xFFB9412D,
                .capSpot = 0xFFFFFFFF,
                .stalk = 0xFFFFF8F1,
                .stalkShade = 0xFFE2D5C4,
                .board = 0xFFC9A04A,
                .boardShade = 0xFF917025,
                .stumpTop = 0xFFD8B888,
                .stumpRing = 0xFFAC8B57,
                .stumpSprout = 0xFF216655,
                .frogLight = 0xFF4FA38C,
                .frogDark = 0xFF246856,
                .frogBelly = 0xFFC8E8D8,
                .shellLight = 0xFFC78142,
                .shellDark = 0xFF7E4C1E,
                .shellLine = 0xFFEFCF7E,
                .snailBody = 0xFFCABCAA,
                .fluff = 0xFFE8DCC8,
                .birdBody = 0xFF5A6A7A,
                .birdBelly = 0xFFDCCBB0,
                .birdWing = 0xFF3D4956,
                .birdBeak = 0xFFC9A04A,
                .dust = 0xFFCCC0AF,
                .dazeStar = 0xFFFFD34D
            },
            .hasRain = false,
            .hasFireflies = false,
            .hasLeaves = false,
            .hasSnow = false,
            .hasPetals = false,
            .hasDust = true,
            .isAfrica = false,
            .airHaze = 0.20f
        } {
        }
    };
}
