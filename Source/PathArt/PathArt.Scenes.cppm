export module ClaFi.PathArt.Scenes;

import ClaFi.PathArt.Scene;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    export using SceneList = std::vector<std::unique_ptr<Scene>>;

    // Every scene of the showcase but the mill, which the showcase holds itself for its themes.
    export [[nodiscard]] SceneList createScenes();
}
