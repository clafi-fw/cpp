module MillScene_App.Main;

import MillScene_App.Options;

import ClaFi.App.Settings;
import ClaFi.PathArt.Scene;
import ClaFi.Diagnostic.Benchmark;
import ClaFi.Controls.Panel;

import ClaFi.Icons.DotMark;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

namespace MillScene_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    MillSceneForm::MillSceneForm(const CreateParams& params, const LaunchOptions& launch)
        :
        Panel{
            params,
            UiElement::Dialog,
            MinSize{ 320.0f, 240.0f },
            PreferredSize{ 840.0f, 680.0f }
        },
        m_launch{ launch },
        m_paintTimer{
            [this](RepeatEvent&) {
                writeMetrics();
                m_scenePanel.invalidate();
            },
            MilliSeconds{ 0 }
        }
    {
        m_scenePanel.onMouseMove([this](MouseMoveEvent&) {
            sceneAt(m_shownScene).animationTick();
        });
        m_scenePanel.onPaint([this](PaintEvent& event) {
            paintScene(event);
        });
        m_readoutBlock.onAdjustTooltip([this](AdjustTooltipEvent& event) {
            writeMetricsTooltip(event);
        });

        appContext().addInformation(
            L"The Earth scene is a piece of art and does not claim to be accurate.");
        appContext().addInformation(
            L"Made with Natural Earth. Free vector and raster map data @ "
            L"https://naturalearthdata.com.");

        // **Before anything reads the scene.** The info bar takes its colors from the theme, so
        // the stored one has to be the scene's by now.
        restoreSceneTheme();
        restoreScene();
        m_sceneTime = sceneTimeAt(m_launch.at, sceneAt(m_shownScene).cycleDuration());
        restoreSceneViews();
        restoreLook();
        // Written as the window closes, while every scene still stands.
        form().onClose([this](FormCloseEvent&) {
            storeSceneViews();
        });

        connectAppPage(L"Scene", Icons::DotMark::draw, [this](AppMenuPage& page) {
            buildScenePicker(page);
        });
        Action& fullscreenToggle = connectFullscreen();
        m_scenePanel.onContextPopup([this, &fullscreenToggle](ContextPopupEvent& event) {
            showSceneMenu(event, fullscreenToggle);
        });
        form().setKeepsScreenOn(true);
        applySceneColors();

        // The readout has its lines before the first layout, so the first frame moves nothing.
        writeMetrics();
        m_paintTimer.start();
    }
}
