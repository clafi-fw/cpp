export module ClaFi.PathArt.Themes :DesertDusk;

import ClaFi.PathArt.Types;

namespace ClaFi::PathArt::Themes
{
    export struct DesertDusk : SceneTheme {
        constexpr DesertDusk() : SceneTheme{
            .name = L"Desert Dusk",
            .skyTop = 0xFFA52A2A,           // Dark orange top sky (desert sunset)
            .skyBottom = 0xFFD2B48C,        // Light brown horizon (sand dunes at sunset)
            .sunDisk = 0xFFFF8C00,          // Sun core — bright orange
            .sunGlow = 0xFFFFFBF0,          // Glow around sun — almost white
            .cloud = 0xCCE6E6FA,            // Clouds — light gray (rare clouds in the desert)
            .mountainTop = 0xFF7B6A63,
            .mountainFoot = 0xFF564740,
            .hillTop = 0xFFD2B48C,          // Hilltops — light brown (sand)
            .hillBottom = 0xFFA52A2A,       // Lower parts of hills — dark orange (sunset shadow)
            .riverTop = 0xFF1E90FF,         // Top part of water — blue (oasis)
            .riverBottom = 0xFF00BFFF,      // River depths — dark blue (cold oasis water)
            .riverWave = 0xFF7B68EE,        // Waves — lilac (sunset reflection)
            .foam = 0xBBFFFFFE,             // Foam — almost white
            .treeLeavesBase = 0xFF228B22,   // Leaves base — dark green (rare oasis greenery)
            .crownLight = 0xFF2B9E29,
            .crownMiddle = 0xFF259223,
            .bark = 0xFF603311,
            .barkShade = 0xFF3C1A02,
            .bough = 0xFF482205,
            .trail = 0xE6DBB982,
            .grassBlade = {
                .light = 0xFF6E8059,
                .dark = 0xFF5D714C
            },
            .grassPatch = {
                .dark = 0xFF52714A,
                .middle = 0xFF6E875B,
                .light = 0xFF9AA16F
            },
            .reedStem = 0xFF8A805F,
            .reedHead = 0xFF643B1E,
            .stoneLight = 0xFF7D6B5F,
            .stoneDark = 0xFF4D3D32,
            .stoneCrack = 0xFF382920,
            .shadow = 0x47696969,
            .cowBody = 0xFFC0C2BE,          // Cow body — gray-brown (sandy camouflage)
            .cowSpots = 0xFF5F5F5F,         // Cow spots — dark gray (shadows)
            .cowSnout = 0xFFC17A33,         // Snout — light brown (warm body part)
            .cowDetail = 0xFF803B03,        // Details (nostrils, eyes) — brown
            .wolfBody = 0xFFCECEBC,         // Wolf body — light beige (sandy camouflage)
            .wolfEye = 0xFF6A5ACD,          // Same as the wing blades
            .effectColor1 = 0xFFFFD700,     // Golden (like sand lit by sunset)
            .effectColor2 = 0xFFFFD700,     // Effect: swirling sand particles in the air, illuminated by the last rays of the sun
            .petal1 = 0xFFF6E3DB,
            .petal2 = 0xFFFED1D1,
            .petal3 = 0xFFEAD0ED,
            .flowerHeart = 0xFFEEBC37,
            .wing = 0xFFB46D22,
            .wingEdge = 0xFF281004,
            .flock = 0xFF2B1F22,
            .motherDuckBody = 0xFF734E31,
            .motherDuckWing = 0xFF55341D,
            .motherDuckEye = 0xFF803B03,
            .duckLight = 0xFFF6CD3F,
            .duckShade = 0xFFE1A20C,
            .duckBeak = 0xFFE98211,
            .duckBeakShade = 0xFFC35F10,
            .fish = 0xFF435468,
            .mill = {
                .bodyTop = 0xFFE09365,      // Top part of mill body — dark brown (sun-baked wood)
                .bodyBottom = 0xFFB7B7B7,   // Lower part of body — dark gray (dusty base)
                .roofTop = 0xFFE56454,      // Top part of roof — dark red (faded roof)
                .roofBottom = 0xFFFF897D,   // Lower part of roof — burgundy (shadow)
                .timber = 0xFF482205,
                .wingFabric = 0xFFB8B6FE,   // Fabric on blades — lilac (dust with sky reflection)
                .wheel = 0xFF799B9A         // Mill wheel — dark gray (rusty)
            },
            .chase = {
                .treeCrown = 0xFF228B22,
                .treeCrownLight = 0xFF2B9E29,
                .treeCrownMiddle = 0xFF259223,
                .pebble = 0xFFC5A16D,
                .ourWolf = {
                    .light = 0xFF665B5D,
                    .shade = 0xFF493E3F,
                    .coat = 0xFF594E4F,
                    .far = 0xFF413636,
                    .tips = 0xFF322828,
                    .belly = 0xE6918382,
                    .tongue = 0xFFBB575B,
                    .mouth = 0xFF481115,
                    .tooth = 0xFFD3BDAC,
                    .toothEdge = 0xFF856F60
                },
                .ourHare = {
                    .light = 0xFF7B4A1F,
                    .fur = 0xFF6E3F15,
                    .shade = 0xFF613307,
                    .belly = 0xE69F8368,
                    .tail = 0xFFD7C4BC,
                    .earInside = 0xFFC28175,
                    .nose = 0xFFAC5C57,
                    .whisker = 0x804C280E,
                    .mouth = 0xFF491C14,
                    .tooth = 0xFFD7C4BC,
                    .toothEdge = 0xFF8F776A
                },
                .theirWolf = {
                    .light = 0xFF463938,
                    .shade = 0xFF332625,
                    .coat = 0xFF3C2F2E,
                    .far = 0xFF2B1E1D,
                    .tips = 0xFF201412,
                    .belly = 0xE6796A69,
                    .tongue = 0xFFBB575B,
                    .mouth = 0xFF481115,
                    .tooth = 0xFFD3BDAC,
                    .toothEdge = 0xFF856F60
                },
                .theirHare = {
                    .light = 0xFF69584D,
                    .fur = 0xFF5A4A3F,
                    .shade = 0xFF4C3C31,
                    .belly = 0xE6978479,
                    .tail = 0xFFD7C4BC,
                    .earInside = 0xFFC28175,
                    .nose = 0xFFAC5C57,
                    .whisker = 0x804C280E,
                    .mouth = 0xFF491C14,
                    .tooth = 0xFFD7C4BC,
                    .toothEdge = 0xFF8F776A
                },
                .appleLight = 0xFFC6402D,
                .appleDark = 0xFF8C1B10,
                .leaf = 0xFF608538,
                .appleFlesh = 0xFFE4C9AA,
                .appleSeed = 0xFF45210A,
                .crowBody = 0xFF2B1F22,
                .crowWing = 0xFF1F1414,
                .crowWingFar = 0xFF180D0D,
                .crowBeak = 0xFF453736,
                .bootLight = 0xFF72492B,
                .bootDark = 0xFF4E2C13,
                .bootSole = 0xFF2B180F,
                .bootPatch = 0xFF845B38,
                .lace = 0xFFC2A687,
                .spinesDark = 0xFF4E311C,
                .spinesLight = 0xFF68472E,
                .hedgehogFace = 0xFF7F583B,
                .hedgehogEar = 0xFF65412A,
                .ballPanel1 = 0xFFBE3F2C,
                .ballPanel2 = 0xFF4D679E,
                .ballPanel3 = 0xFFEEBD20,
                .ballBase = 0xFFDBC7BB,
                .peel = 0xFFF9CF45,
                .peelShade = 0xFFE0AF2D,
                .peelInside = 0xFFFEF0D1,
                .peelTip = 0xFF704823,
                .rakeHandle = 0xFF8F5B31,
                .rakeHandleShade = 0xFF70421F,
                .rakeMetal = 0xFF625654,
                .capLight = 0xFFAD3828,
                .capDark = 0xFF7F1B0E,
                .capSpot = 0xFFCFB8A7,
                .stalk = 0xFFC7AE99,
                .stalkShade = 0xFFAC9073,
                .board = 0xFF8A582E,
                .boardShade = 0xFF603311,
                .stumpTop = 0xFFA27C57,
                .stumpRing = 0xFF805831,
                .stumpSprout = 0xFF3E5510,
                .frogLight = 0xFF839435,
                .frogDark = 0xFF54651D,
                .frogBelly = 0xFFC3B585,
                .shellLight = 0xFF90541E,
                .shellDark = 0xFF5A2906,
                .shellLine = 0xFF3E1A04,
                .snailBody = 0xFF92765C,
                .fluff = 0xFFCBB2A0,
                .birdBody = 0xFF734E31,
                .birdBelly = 0xFFC3A78E,
                .birdWing = 0xFF55341D,
                .birdBeak = 0xFFB87B32,
                .dust = 0xFFC3A789,
                .dazeStar = 0xFFFFD34D
            },
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
}
