export module ClaFi.PathArt.PuppeteerScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // An android works a marionette on a small stage. See PathArt
    export class PuppeteerScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Puppeteer"; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        // The marionette's joints at one moment of its dance.
        struct Puppet
        {
            FloatPoint pelvis;
            FloatPoint neck;
            FloatPoint head;
            float lean;
            FloatPoint shoulderLeft;
            FloatPoint shoulderRight;
            FloatPoint hipLeft;
            FloatPoint hipRight;
            Limb legLeft;
            Limb legRight;
            Limb armLeft;
            Limb armRight;
        };
        // The wooden control the strings hang from: a main bar and a leg bar hooked under it.
        struct Control
        {
            FloatPoint center;
            FloatPoint endLeft;
            FloatPoint endRight;
            FloatPoint legBarCenter;
            FloatPoint kneeEndLeft;
            FloatPoint kneeEndRight;
        };
    private:
        static void drawStage(const SceneFrame&);
        static void drawSpotlight(const SceneFrame&, float lampX, float puppetX, float floorY);
        static void drawAndroid(const SceneFrame&, float androidX, float floorY, float cycleTime);
        static void drawStrings(const SceneFrame&, const Puppet&, const Control&);
        static void drawControl(const SceneFrame&, const Control&);
        static void drawPuppet(const SceneFrame&, const Puppet&);
        static void drawDust(const SceneFrame&, float lampX, float puppetX, float floorY,
            float cycleTime);
        static void drawCurtains(const SceneFrame&, float cycleTime);
    };
}
