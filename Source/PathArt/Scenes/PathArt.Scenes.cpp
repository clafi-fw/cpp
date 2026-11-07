module ClaFi.PathArt.Scenes;

import ClaFi.PathArt.ChaseScene;
import ClaFi.PathArt.ClockworkScene;
import ClaFi.PathArt.CometScene;
import ClaFi.PathArt.GhostScene;
import ClaFi.PathArt.OrreryScene;
import ClaFi.PathArt.PuppeteerScene;
import ClaFi.PathArt.SatelliteScene;
import ClaFi.PathArt.Scene;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    SceneList createScenes()
    {
        SceneList scenes;
        scenes.push_back(std::make_unique<PuppeteerScene>());
        scenes.push_back(std::make_unique<GhostScene>());
        scenes.push_back(std::make_unique<ClockworkScene>());
        scenes.push_back(std::make_unique<OrreryScene>());
        scenes.push_back(std::make_unique<CometScene>());
        scenes.push_back(std::make_unique<SatelliteScene>());
        scenes.push_back(std::make_unique<ChaseScene>());
        return scenes;
    }
}
