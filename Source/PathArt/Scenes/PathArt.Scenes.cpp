module ClaFi.PathArt.Scenes;

import ClaFi.PathArt.ChaseScene;
import ClaFi.PathArt.ClockworkScene;
import ClaFi.PathArt.CometScene;
import ClaFi.PathArt.EarthScene;
import ClaFi.PathArt.GhostScene;
import ClaFi.PathArt.OrreryScene;
import ClaFi.PathArt.PuppeteerScene;
import ClaFi.PathArt.Scene;
import ClaFi.Core.DomEngine_Dt;

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
        scenes.push_back(std::make_unique<EarthScene>());
        scenes.push_back(std::make_unique<ChaseScene>());
        return scenes;
    }

    // Keyless, so each scene's section stands straight in the section that holds this one.
    Dom::Dt::Section createSceneViewsSchema()
    {
        Dom::Dt::Section views{};
        views.children.push_back(std::make_unique<Dom::Dt::Section>(OrreryScene::viewSchema()));
        views.children.push_back(std::make_unique<Dom::Dt::Section>(EarthScene::viewSchema()));
        return views;
    }
}
