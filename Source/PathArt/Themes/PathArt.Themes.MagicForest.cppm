export module ClaFi.PathArt.Themes :MagicForest;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct MagicForest : SceneTheme {
        constexpr MagicForest() : SceneTheme{
            .name = L"Magic Forest",
            .skyTop = 0xFF7C77C2,           // Lilac top sky (magical atmosphere)
            .skyBottom = 0xFFE6E6FA,        // Light blue horizon (magical light)
            .sunDisk = 0xFFECAA90,          // Sun core — peach (magical light)
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCE6E6FA,            // Clouds — light gray with magical tint
            .mountainTop = 0xFFFEF0FF,
            .mountainFoot = 0xFF433543,
            .hillTop = 0xFF7CA179,          // Hilltops - light green (sunlit canopy)
            .hillBottom = 0xFF416340,       // Lower parts of hills - dark green (dense forest)
            .riverTop = 0xFF6885BA,         // Top part of water — blue (clear forest river)
            .riverBottom = 0xFF4868A3,      // River depths — dark blue (mysterious depth)
            .riverWave = 0xFF7C77C2,        // Waves — lilac (magical reflection)
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .treeLeavesBase = 0xFF5A7F58,   // Leaves base - muted green (dense foliage)
            .crownLight = 0xFF699166,
            .crownMiddle = 0xFF5F855C,
            .bark = 0xFF87798F,
            .barkShade = 0xFF605567,
            .bough = 0xFF6A4E78,
            .trail = 0xE6A6B681,
            .grassBlade = {
                .light = 0xFF244122,
                .dark = 0xFF1D361B
            },
            .grassPatch = {
                .dark = 0xFF7A6FA1,
                .middle = 0xFF998DC5,
                .light = 0xFFD0B9C9
            },
            .reedStem = 0xFF3C5B3A,
            .reedHead = 0xFF987BB8,
            .stoneLight = 0xFFB7B4CA,
            .stoneDark = 0xFF7A768B,
            .stoneCrack = 0xFF605C70,
            .shadow = 0x47333333,
            .cowBody = 0xFFE2E4E0,          // Cow body — gray-brown (blends with undergrowth)
            .cowSpots = 0xFF7D7D7D,         // Cow spots — dark gray
            .cowSnout = 0xFFD4A275,         // Snout — light brown
            .cowDetail = 0xFF946042,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFF443525,         // Wolf body — dark gray (stealth in the shadows)
            .wolfEye = 0xFFFFDCBC,          // Eyes — glowing, with mystic reflection
            .effectColor1 = 0xFF6A66AA,     // Lilac (magical tint)
            .effectColor2 = 0xFF6A66AA,     // Effect: twinkling magical sparks slowly rising from the ground and trees
            .petal1 = 0xFFFAC5D5,
            .petal2 = 0xFFE6BBE6,
            .petal3 = 0xFFC5B4EB,
            .flowerHeart = 0xFFDEC07F,
            .wing = 0xFFD69DB2,
            .wingEdge = 0xFF423C77,
            .flock = 0xFF2F274F,
            .motherDuckBody = 0xFFE3A7C0, // rose, which this theme already wears
            .motherDuckWing = 0xFFB3748F,
            .motherDuckEye = 0xFF2A1A20,
            .duckLight = 0xFFF2DB91,
            .duckShade = 0xFFD6B06F,
            .duckBeak = 0xFFD89768,
            .duckBeakShade = 0xFFC86329,
            .fish = 0xFF335382,
            .hedge = 0xFF51754F,
            .mill = {
                .bodyTop = 0xFF98806D,
                .bodyBottom = 0xFF876F5D,
                .roofTop = 0xFF4E2E24,
                .roofBottom = 0xFF6C483B,
                .timber = 0xFF564B5D,
                .wingFabric = 0xFFCBCBE1,
                .wheel = 0xFF2F4F4F         // Mill wheel — dark gray (rusty)
            },
            .chase = {
                .treeCrown = 0xFF5E855E,
                .treeCrownLight = 0xFF6C986C,
                .treeCrownMiddle = 0xFF638C62,
                .pebble = 0xFF92A562,
                .ourWolf = {
                    .light = 0xFFA3A3BF,
                    .shade = 0xFF8787A2,
                    .coat = 0xFF9B9DB7,
                    .far = 0xFF7D7D96,
                    .tips = 0xFF403F55,
                    .belly = 0xE6E6E4FD,
                    .tongue = 0xFFEE6A8A,
                    .mouth = 0xFF5A1632,
                    .tooth = 0xFFFDF0F3,
                    .toothEdge = 0xFF9E9093
                },
                .ourHare = {
                    .light = 0xFFFFF0F5,
                    .fur = 0xFFFFDAE7,
                    .shade = 0xFFFFBED6,
                    .belly = 0xE6FFF1F6,
                    .tail = 0xFFFEFCFF,
                    .earInside = 0xFFF2A2AC,
                    .nose = 0xFFD97385,
                    .whisker = 0x805C372C,
                    .mouth = 0xFF5B2731,
                    .tooth = 0xFFFEFCFF,
                    .toothEdge = 0xFFAA999F
                },
                .theirWolf = {
                    .light = 0xFF464256,
                    .shade = 0xFF2E2B3F,
                    .coat = 0xFF393649,
                    .far = 0xFF262134,
                    .tips = 0xFF1A1525,
                    .belly = 0xE6827F94,
                    .tongue = 0xFFEE6A8A,
                    .mouth = 0xFF5A1632,
                    .tooth = 0xFFFDF0F3,
                    .toothEdge = 0xFF9E9093
                },
                .theirHare = {
                    .light = 0xFFABA3AC,
                    .fur = 0xFF7D767D,
                    .shade = 0xFF6A646B,
                    .belly = 0xE6C9C0C9,
                    .tail = 0xFFFEFCFF,
                    .earInside = 0xFFF2A2AC,
                    .nose = 0xFFD97385,
                    .whisker = 0x805C372C,
                    .mouth = 0xFF5B2731,
                    .tooth = 0xFFFEFCFF,
                    .toothEdge = 0xFFAA999F
                },
                .appleLight = 0xFFE7464E,
                .appleDark = 0xFFA21E30,
                .leaf = 0xFF2EB56E,
                .appleFlesh = 0xFFFCEBDE,
                .appleSeed = 0xFF4B2B26,
                .crowBody = 0xFF302555,
                .crowWing = 0xFF221544,
                .crowWingFar = 0xFF1A0B3A,
                .crowBeak = 0xFFFFA07A,
                .bootLight = 0xFF7B5E93,
                .bootDark = 0xFF513B64,
                .bootSole = 0xFF2F1F3E,
                .bootPatch = 0xFFC8B0F9,
                .lace = 0xFFFFC6D7,
                .spinesDark = 0xFF7B6CAD,
                .spinesLight = 0xFF9A88D8,
                .hedgehogFace = 0xFFF2D8EA,
                .hedgehogEar = 0xFFD0B9C9,
                .ballPanel1 = 0xFFE7464E,
                .ballPanel2 = 0xFF4578E1,
                .ballPanel3 = 0xFFFED14B,
                .ballBase = 0xFFFAF0FB,
                .peel = 0xFFFCD557,
                .peelShade = 0xFFE0B33F,
                .peelInside = 0xFFFFF7EB,
                .peelTip = 0xFF674635,
                .rakeHandle = 0xFF915FAA,
                .rakeHandleShade = 0xFF6E4184,
                .rakeMetal = 0xFF6C5697,
                .capLight = 0xFFE4434D,
                .capDark = 0xFFA72031,
                .capSpot = 0xFFFCEEF1,
                .stalk = 0xFFF3E2DE,
                .stalkShade = 0xFFD3BDAC,
                .board = 0xFFC8B0F9,
                .boardShade = 0xFF9480BC,
                .stumpTop = 0xFFD5AAE9,
                .stumpRing = 0xFFA97BBE,
                .stumpSprout = 0xFF0A9B59,
                .frogLight = 0xFF3EE08A,
                .frogDark = 0xFF159E5C,
                .frogBelly = 0xFFD8FFE8,
                .shellLight = 0xFFEDB6EE,
                .shellDark = 0xFFA87CA9,
                .shellLine = 0xFF6A5ACD,
                .snailBody = 0xFFC8F5DC,
                .fluff = 0xFFFFF0FF,
                .birdBody = 0xFF8E72D8,
                .birdBelly = 0xFFFFE6F2,
                .birdWing = 0xFF6B54A6,
                .birdBeak = 0xFFFFA07A,
                .dust = 0xFFDBC2B4,
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
