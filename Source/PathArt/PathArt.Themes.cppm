export module ClaFi.PathArt.Themes;

export import :ClearDay;
export import :CherryBlossom;
export import :HighNoon;
export import :SummerSunset;
export import :PaperSketch;
export import :GoldenHour;
export import :SaharaHeat;
export import :WarmOrangeFall;
export import :WinterFrost;
export import :ArcticDawn;
export import :VintageNoir;
export import :ToxicWaste;
export import :SteampunkAdventure;
export import :MagicForest;
export import :NeonCityNight;
export import :CyberpunkNight;
export import :UnderwaterRealm;
export import :LunarEclipse;
export import :Twilight;
export import :WinterNight;
export import :MidnightFantasy;
export import :Midnight;
export import :StormyNight;
export import :CalmNight;

export import :PastelDream;
export import :NeonJungle;
export import :DesertDusk;
export import :Retro80s;

import ClaFi.PathArt.Types;
import ClaFi.StdLib;

namespace ClaFi::PathArt::Themes {

    export constexpr std::array<SceneTheme, 24> allThemes = {
        ClearDay{},
        CherryBlossom{},
        HighNoon{},
        SummerSunset{},
        PaperSketch{},
        GoldenHour{},
        SaharaHeat{},
        WarmOrangeFall{},

        WinterFrost{},
        ArcticDawn{},
        VintageNoir{},
        ToxicWaste{},
        SteampunkAdventure{},
        MagicForest{},
        NeonCityNight{},
        CyberpunkNight{},

        UnderwaterRealm{},
        LunarEclipse{},
        Twilight{},
        WinterNight{},
        MidnightFantasy{},
        Midnight{},
        StormyNight{},
        CalmNight{},

        //PastelDream{},
        //NeonJungle{},
        //DesertDusk{},
        //Retro80s{}
    };

    export const SceneTheme& getTheme(std::size_t index)
    {
        return allThemes[index];
    }
}
