export module ClaFi.PathArt.PlanetSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Which planet: each has markings of its own, Saturn its rings and the Earth its Moon.
    export enum class Planet
    {
        Mercury,
        Venus,
        Earth,
        Mars,
        Jupiter,
        Saturn
    };

    // Which way the light comes from: across the screen, down it, and toward the viewer.
    export struct LightDirection
    {
        float x;
        float y;
        float toViewer;
    };

    // A planet where it stands at one moment.
    export struct PlanetPose
    {
        Planet planet;
        FloatPoint center;
        float radius;
        Color color;
        LightDirection light;
        float orbitTilt; // how flat an orbit is seen; the Moon's round the Earth is drawn this flat
        float cycleTime; // the time the planet spins and the Moon goes round by
    };

    export void drawPlanet(const SceneFrame&, const PlanetPose&);
    // Darkens the half of a ball turned from the light. See PathArt
    export void shadeBall(const SceneFrame&, FloatPoint center, float radius, LightDirection,
        Color shade);
}
