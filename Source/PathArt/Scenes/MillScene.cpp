module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.BlowingDustEffect;
import ClaFi.PathArt.CowSprite;
import ClaFi.PathArt.FallingLeavesEffect;
import ClaFi.PathArt.FirTreeSprite;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.LightningEffect;
import ClaFi.PathArt.MillSprite;
import ClaFi.PathArt.PeekingWolfSprite;
import ClaFi.PathArt.RainEffect;
import ClaFi.PathArt.ReedsSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SnowEffect;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterLilySprite;
import ClaFi.PathArt.WaterWheelSprite;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Graphics::Cpu;
    using namespace std::chrono_literals;

    namespace
    {
        AnimationSlot g_switchThemeSlot{
            .duration{ .rise{ 555ms } },
            .easingFactor{ EasingFactor::EaseInOut }
        };
    }

    // **Always a fade in, never a cut.** The scene stands on a null theme - every colour zero and
    // every effect off - until this, so the first theme rises out of nothing the way every later
    // one crossfades out of the one before it. A scene restored from a config fades into the
    // stored theme rather than into the first of the list: the pick lands before the first paint,
    // and a crossfade restarted from a blend is still one movement.
    MillScene::MillScene()
    {
        crossFadeToPickedTheme();
    }

    void MillScene::setTheme(const std::size_t index)
    {
        if (m_themeIndex == index)
            return;

        m_themeIndex = index;
        crossFadeToPickedTheme();
    }

    void MillScene::animationTick()
    {
        m_animator.externalTimerTick();
    }

    void MillScene::draw(Canvas& canvas, const FloatRect& bounds, const float t)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        m_offset = bounds.topLeft(); // Anchored to widget's logical position
        const float w = bounds.width();
        const float h = bounds.height();
        m_objectsScale = std::min(h, w) / 600.0f;
        m_sunPos = { w * 0.75f, h * 0.15f };
        m_sunRadius = 35.0f * m_objectsScale;
        const float mS = m_objectsScale;

        const ScaledSize newBackgroundDimensions = bounds.dimensions();
        if (m_backgroundDimensions != newBackgroundDimensions)
        {
            m_backgroundDimensions = newBackgroundDimensions;
            prepareBackground(w, h);
        }

        // Draw pre-rendered background matching widget coordinate offset [CP]
        canvas.drawBitmap(m_background, m_offset);

        drawStars(frame, t, StarField{
            .style = StarStyle::Sparkles,
            .count = 120,
            .top = 0.0f,
            .bottom = h * 0.65f,
            .color = m_theme.sunDisk,
            .opacity = m_theme.hasStars
        });
        drawSunCorona(canvas, m_offset.x + m_sunPos.x, m_offset.y + m_sunPos.y, m_sunRadius, t);

        const float orbX = m_offset.x + w * 0.5f;
        const float orbY = m_offset.y + h * 0.2f;
        const float rX = w * 0.35f;
        const float rY = h * 0.04f;
        const float a1 = 0.5f * t * k_2Pi;
        const float a2 = a1 + 3.1415f;
        drawCloud(canvas, orbX + std::cos(a1) * rX, orbY + std::sin(a1) * rY,
            (0.8f + (std::sin(a1) + 1.0f) * 0.2f) * m_objectsScale);
        drawCloud(canvas, orbX + std::cos(a2) * rX, orbY + std::sin(a2) * rY,
            (0.8f + (std::sin(a2) + 1.0f) * 0.2f) * m_objectsScale);

        drawCow(frame, CowPose{
            .at = { w * 0.28f, h * 0.61f },
            .size = 0.36f * m_objectsScale,
            .cycleTime = t
        }, CowColors{
            .body = m_theme.cowBody,
            .spots = m_theme.cowSpots,
            .snout = m_theme.cowSnout,
            .detail = m_theme.cowDetail,
            .shadow = m_theme.shadow
        });

        drawWaterWheel(frame, WaterWheelPose{
            .center = { w * 0.5f - 65.0f * mS, h * 0.85f },
            .size = m_objectsScale,
            .turns = t * 2.0f
        }, m_theme.millWheel);

        const MillPose mill = {
            .base = { w * 0.5f, h * 0.85f },
            .size = mS,
            .wingTurns = t
        };
        const MillColors millColors = {
            .bodyTop = m_theme.millBodyTop,
            .bodyBottom = m_theme.millBodyBottom,
            .roofTop = m_theme.millRoofTop,
            .roofBottom = m_theme.millRoofBottom,
            .wingWood = m_theme.millWingWood,
            .wingWoodStroke = m_theme.millWingWoodStroke,
            .wingFabric = m_theme.millWingFabric,
            .wingFabricStroke = m_theme.millWingFabricStroke
        };
        drawMillHouse(frame, mill, millColors);
        drawWaterFoam(canvas, m_offset.x + w * 0.5f - 75.0f * mS, m_offset.y + h * 0.85f,
            m_objectsScale, t * 1.25f);
        drawLightning(frame, t, Lightning{
            .strength = m_theme.hasRain,
            .color = m_theme.sunDisk
        });

        // Anchors River coordinate boundaries to layout constraints instead of clipBox [CP]
        FloatRect riverBounds = bounds;
        riverBounds.top = drawRiver(canvas, m_offset, w, h, t);
        const float riverTop = riverBounds.top - m_offset.y;

        // 1. Constrain river reflections inside riverbed
        //canvas.pushClip(riverBounds);

        const FloatPoint reflectPos = { m_offset.x + m_sunPos.x, riverBounds.center().y };
        canvas.fillRectangle(riverBounds, PointGlow{
            .lightPos = reflectPos,
            .lightColor = faded(m_theme.sunGlow, 0.2f),
            .lightSpread = 66.0f * m_objectsScale,
            .xRatio = 1.3f,
            .shape = GlowShape::Circle
        });

        drawWaterLily(frame, WaterLilyPose{
            .center = { w * 0.2f, riverTop + 40.0f * m_objectsScale },
            .size = m_objectsScale,
            .cycleTime = t
        }, WaterLilyColors{
            .pad = m_theme.lilyPad,
            .flower = m_theme.lilyFlower
        });

        // 2. Pop River Clip
        //canvas.popClip();

        const float reedsScale = m_objectsScale * 1.33f;
        const ReedColors reedColors = {
            .stem = m_theme.reedStem,
            .head = m_theme.reedHead
        };
        drawReeds(frame, ReedsPose{
            .base = { w * 0.3f, riverTop },
            .size = reedsScale,
            .cycleTime = t
        }, reedColors);
        drawReeds(frame, ReedsPose{
            .base = { w * 0.62f, riverTop },
            .size = reedsScale * 1.16f,
            .cycleTime = t + 0.3f
        }, reedColors);

        drawMillWings(frame, mill, millColors);

        const FirTreeColors treeColors = {
            .trunk = m_theme.treeTrunk,
            .leaves = m_theme.treeLeavesBase,
            .shadow = m_theme.shadow
        };
        drawFirTree(frame, { w * 0.15f, h * 0.75f }, 0.8f * m_objectsScale, treeColors);
        // The predator behind the far tree: a gator in the African themes, a wolf elsewhere,
        // crossfading between them.
        const FloatPoint predator = {
            w * 0.85f - 29.0f * m_objectsScale,
            h * 0.70f - 24.0f * m_objectsScale
        };
        drawGator(frame, GatorPose{
            .at = predator,
            .size = m_objectsScale * 0.8f,
            .cycleTime = t,
            .opacity = m_theme.isAfrica
        }, GatorColors{
            .body = m_theme.wolfBody,
            .eye = m_theme.wolfEye
        });
        drawPeekingWolf(frame, PeekingWolfPose{
            .at = predator,
            .size = m_objectsScale * 0.8f,
            .cycleTime = t,
            .opacity = 1.0f - m_theme.isAfrica
        }, PeekingWolfColors{
            .body = m_theme.wolfBody,
            .ear = m_theme.wolfEar,
            .eye = m_theme.wolfEye
        });
        drawFirTree(frame, { w * 0.85f, h * 0.70f }, 1.2f * m_objectsScale, treeColors);

        drawRain(frame, t, Rain{
            .opacity = m_theme.hasRain,
            .color = m_theme.effectColor1
        });
        drawSnow(frame, t, Snow{
            .opacity = m_theme.hasSnow,
            .color = m_theme.effectColor1
        });
        drawFireflies(frame, t, Fireflies{
            .opacity = m_theme.hasFireflies,
            .light = m_theme.effectColor1,
            .glow = m_theme.effectColor2
        });
        drawFallingLeaves(frame, t, FallingLeaves{
            .opacity = StateFactors::compose(m_theme.hasLeaves, m_theme.hasPetals),
            .petals = m_theme.hasPetals,
            .color1 = m_theme.effectColor1,
            .color2 = m_theme.effectColor2
        });
        drawBlowingDust(frame, t, BlowingDust{
            .opacity = m_theme.hasDust,
            .color = m_theme.effectColor1
        });

        // river fog
        canvas.fillRectangle(bounds, PointGlow{
            .lightPos = riverBounds.topCenter() + FloatPoint{ 0.0f, 20.0f * m_objectsScale },
            .lightColor = faded(m_theme.cloud, 0.33f),
            .lightSpread = 60.0f * m_objectsScale,
            .xRatio = 0.08f,
            .shape = GlowShape::Circle
        });
    }

    void MillScene::crossFadeToPickedTheme()
    {
        m_prevTheme = m_theme;
        m_nextTheme = &pickedTheme();
        // The name is the destination's at once, like the index: what crossfades below is the
        // colours, and a blend answering the name it came from tells its caller the wrong theme.
        m_theme.name = m_nextTheme->name;
        m_animator.start(this, g_switchThemeSlot, 0.0f, 1.0f, [this](AnimateParams& params) {
            for (std::size_t i = 0; i != SceneTheme::colorsNum(); ++i)
            {
                m_theme.asArray()[i] = Color{
                    m_prevTheme.asArray()[i],
                    m_nextTheme->asArray()[i],
                    params.value
                };
            }
            for (std::size_t i = 0; i != SceneTheme::effectsNum(); ++i)
            {
                const float v1 = m_prevTheme.effectsArray()[i];
                const float v2 = m_nextTheme->effectsArray()[i];
                m_theme.effectsArray()[i] = v1 * (1.0f - params.value) + v2 * params.value;
            }
            m_backgroundDimensions = {}; // to rebuild background
        });
    }

    void MillScene::prepareBackground(const float w, const float h)
    {
        m_background.resize(m_backgroundDimensions.roundOut());

        // 1. Temporarily suspend any active global window clips [CP]
        // This prevents the offscreen buffer from being zero-clipped by parent widgets!
        const bool wasActive = g_rasterBuffers.hasActiveClip;
        const float* oldClipMaskPtr = g_rasterBuffers.activeClipMaskPtr;
        const FloatRect oldClipBox = g_rasterBuffers.clipBox;
        const int oldClipStride = g_rasterBuffers.m_clipStride;

        g_rasterBuffers.hasActiveClip = false;
        g_rasterBuffers.activeClipMaskPtr = nullptr;

        // 2. Render the background in pure local space
        PixelPathPainter p{ m_background.pixelView() };
        PixelPath path;

        // Sky and sun glow
        path.moveTo(0.0f, 0.0f);
        path.lineTo(w, 0.0f);
        path.lineTo(w, h);
        path.lineTo(0.0f, h);
        path.close();
        p.drawPath(path, {
            PathDrawLayer::LinearGradient({ 0.0f, 0.0f }, { 0.0f, h }, m_theme.skyTop,
                m_theme.skyBottom),
            PathDrawLayer::PointGlow({
                .lightPos = m_sunPos,
                .lightColor = m_theme.sunGlow,
                .lightSpread = 166.0f * m_objectsScale,
                .xRatio = 0.5f,
                .shape = GlowShape::Circle
            })
        });

        // Sun Disk / Crescent
        path.clear();
        const float s = m_theme.hasStars;
        const Matrix3x2 sunTx = Matrix3x2::Translation(m_sunPos)
            * Matrix3x2::Rotation((s * 0.1f) * k_2Pi);

        const float r = m_sunRadius;
        const float c = r * 0.4142f;
        const float q = r * 0.7071f;
        const float eq = -q * (1.0f - s) + (0.28f * r) * s;
        const float er = -r * (1.0f - s) + (0.50f * r) * s;
        const float cc1 = -c * (1.0f - s) + (0.15f * r) * s;
        const float cc2 = -r * (1.0f - s) + (0.50f * r) * s;

        path.moveTo(0.0f, -r);
        path.quadTo({ c, -r }, { q, -q });
        path.quadTo({ r, -c }, { r, 0.0f });
        path.quadTo({ r, c }, { q, q });
        path.quadTo({ c, r }, { 0.0f, r });
        path.quadTo({ cc1, r }, { eq, q });
        path.quadTo({ cc2, c }, { er, 0.0f });
        path.quadTo({ cc2, -c }, { eq, -q });
        path.quadTo({ cc1, -r }, { 0.0f, -r });
        path.close();

        p.drawPath(path, {
            PathDrawLayer::Fill(m_theme.sunDisk),
            PathDrawLayer::Stroke(m_theme.sunStroke, 2.0f)
        }, &sunTx);

        // Hills
        drawHills(p, {}, w, h);

        // 3. Restore global clipping state to prevent messing up the rest of the canvas loop
        g_rasterBuffers.hasActiveClip = wasActive;
        g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
        g_rasterBuffers.clipBox = oldClipBox;
        g_rasterBuffers.m_clipStride = oldClipStride;
    }

    void MillScene::drawWaterFoam(Canvas& canvas, const float x, const float y, const float mS,
        const float t) const
    {
        const Matrix3x2 tx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(mS);
        for (int i = -2; i <= 2; ++i)
        {
            const float index = static_cast<float>(i);
            PixelPath path;
            const float ox = index * 15.0f + std::sin(t * 10.0f + index) * 5.0f;
            path.moveTo(ox - 10.0f, 0.0f);
            path.quadTo({ ox, -10.0f }, { ox + 10.0f, 0.0f });
            canvas.fillPath(path, SolidColor{ m_theme.foam }, &tx);
        }
    }

    void MillScene::drawSunCorona(Canvas& canvas, const float x, const float y, const float r,
        const float t)
    {
        const float halfTime = t * 0.5f;
        const float minL = r * 1.3f;
        const float spr = r * 0.25f;
        const float rW = r * 0.12f;
        const float rB = r * 1.1f;
        for (int i = 0; i < 16; ++i)
        {
            const float index = static_cast<float>(i);
            const float sV = std::sin(k_2Pi * (halfTime * 2.0f) + index * 0.8f);
            const float dL = minL + (spr * (0.5f + 0.5f * sV));
            const Matrix3x2 tx = Matrix3x2::Translation(x, y)
                * Matrix3x2::Rotation((halfTime + index / 16.0f) * k_2Pi);

            PixelPath path;
            path.moveTo(-rW, -rB);
            path.lineTo(rW, -rB);
            path.lineTo(0.0f, -dL);
            path.close();
            const Color ray = i % 2 == 0 ? m_theme.sunCorona1 : m_theme.sunCorona2;
            canvas.fillPath(path, SolidColor{ ray }, &tx);
        }
    }

    //void MillScene::drawCloud(Canvas& canvas, float x, float y, float s)
    //{
    //    Matrix3x2 tx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(s);
    //    PixelPath path;
    //    path.moveTo(-60.0f, 0.0f);
    //    path.quadTo({ -40.0f, -40.0f }, { -20.0f, -20.0f });
    //    path.quadTo({ 0.0f, -60.0f }, { 20.0f, -20.0f });
    //    path.quadTo({ 40.0f, -40.0f }, { 60.0f, 0.0f });
    //    path.lineTo(60.0f, 20.0f); path.lineTo(-60.0f, 20.0f); path.close();

    //    // 1. Calculate a bounding box matching the cloud geometry, padded by the glow radius
    //    float glowRadius = 10.0f;
    //    FloatRect cloudBounds = {
    //        x - (60.0f + glowRadius) * s,
    //        y - (60.0f + glowRadius) * s,
    //        x + (60.0f + glowRadius) * s,
    //        y + (20.0f + glowRadius) * s
    //    };
    //    //canvas.fillPath(path, 0xffff00ff, &tx);
    //           canvas.drawPath(
    //                path,
    //                {
    //                    PathDrawLayer::Fill(0xffff00ff)
    //                    //PathDrawLayer::OuterGlow(m_theme.cloud, 10.0f * s)
    //                },
    //                &tx
    //            );
    //           return;

    //                // Suspend global clips during offscreen staging to prevent zero-clipping [CP]
    //    bool wasActive = g_rasterBuffers.hasActiveClip;
    //    const float* oldClipMaskPtr = g_rasterBuffers.activeClipMaskPtr;
    //    //g_rasterBuffers.hasActiveClip = false;
    //    //g_rasterBuffers.activeClipMaskPtr = nullptr;

    //    // 2. Request a staging view. On CPU this is a direct backbuffer slice;
    //    // on GPU, it generates a texture source [CP]
    //    canvas.stagingDraw(cloudBounds, [&path, this, &tx, glowRadius](Cpu::PixelView view)
    //    {
    //            PixelPathPainter p{ view };
    //            p.drawPath(path, {
    //                PathDrawLayer::Fill(m_theme.cloud),
    //                PathDrawLayer::OuterGlow(m_theme.cloud, glowRadius)
    //                }, tx);

    //    });
    //    // Restore active clips
    //    g_rasterBuffers.hasActiveClip = wasActive;
    //    g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
    //}

    void MillScene::drawCloud(Canvas& canvas, const float x, const float y, const float s)
    {
        const Matrix3x2 tx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(s);
        PixelPath path;
        path.moveTo(-60.0f, 0.0f);
        path.quadTo({ -40.0f, -40.0f }, { -20.0f, -20.0f });
        path.quadTo({ 0.0f, -60.0f }, { 20.0f, -20.0f });
        path.quadTo({ 40.0f, -40.0f }, { 60.0f, 0.0f });
        path.lineTo(60.0f, 20.0f);
        path.lineTo(-60.0f, 20.0f);
        path.close();

        // Direct, zero-copy, multi-layered drawing! [CP]
        canvas.drawPath(
            path,
            {
                PathDrawLayer::Fill(m_theme.cloud),
                PathDrawLayer::OuterGlow(m_theme.cloud, 10.0f)
            },
            &tx
        );
    }

    float MillScene::drawRiver(Canvas& canvas, const FloatPoint off, const float w, const float h,
        const float t)
    {
        const float rT = h * 0.85f;
        const Matrix3x2 tx = Matrix3x2::Translation(off);
        PixelPath path;
        path.moveTo(0.0f, rT);
        path.lineTo(w, rT);
        path.lineTo(w, h);
        path.lineTo(0.0f, h);
        path.close();

        canvas.fillPath(
            path,
            LinearGradient::Simple(
                { w * 0.5f, rT },
                { w * 0.5f, h },
                m_theme.riverTop.withOpacity(0.8f),
                m_theme.riverBottom.withOpacity(0.8f)
            ),
            &tx
        );

        for (int i = 0; i < 3; ++i)
        {
            float xP = (t + static_cast<float>(i) / 3.0f);
            if (xP > 1.0f)
                xP -= 1.0f;
            const float yO = rT + (h - rT) * 0.4f + static_cast<float>(i * 10) * m_objectsScale;
            const float wS = 16.0f * m_objectsScale;
            const float xS = xP * w + wS;

            path.clear();
            path.moveTo({ xS - wS, yO });
            path.quadTo({ xS, yO - 5.0f * m_objectsScale }, { xS + wS, yO });
            path.close();
            canvas.drawPath(path, SolidColor{ m_theme.riverWave }, 1.5f * m_objectsScale, &tx);
        }
        return rT + off.y;
    }

    void MillScene::drawHills(PixelPathPainter& p, const FloatPoint off, const float w,
        const float h)
    {
        const Matrix3x2 tx = Matrix3x2::Translation(off);
        PixelPath path;
        path.moveTo(0.0f, h * 0.8f);
        path.lineTo(w * 0.5f, h * 0.7f);
        path.quadTo({ w * 0.75f, h * 0.4f }, { w, h * 0.75f });
        path.lineTo(w, h);
        path.lineTo(0.0f, h);
        path.close();
        p.drawPath(path, {
            PathDrawLayer::LinearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, m_theme.hillTop,
                m_theme.hillBottom)
        }, &tx);

        path.clear();
        path.moveTo(0.0f, h * 0.8f);
        path.quadTo({ w * 0.25f, h * 0.5f }, { w * 0.5f, h * 0.7f });
        path.lineTo(w * 0.5f, h);
        path.lineTo(0.0f, h);
        path.close();

        Hsl topHsl{ m_theme.hillTop };
        topHsl.scaleLuminosity(0.9f);
        Hsl botHsl{ m_theme.hillBottom };
        botHsl.scaleLuminosity(0.9f);
        p.drawPath(path, {
            PathDrawLayer::LinearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, topHsl.toColor(),
                botHsl.toColor())
        }, &tx);
    }

    //void MillScene::drawLightning(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    //{
    //    if (m_theme.hasRain <= 0.001f) return;
    //    float strikePhase = t * 3.0f;
    //    float flashT = std::fmod(strikePhase, 1.0f);
    //    if (flashT > 0.15f) return;

    //    float seed = std::floor(strikePhase);
    //    float flicker = (std::sin(flashT * 150.0f) > 0.0f) ? 1.0f : 0.0f;
    //    float fade = 1.0f - (flashT / 0.15f);

    //    Matrix3x2 tx = Matrix3x2::Translation(offset);
    //    PixelPath path;

    //    // Soft sky flash (Fills the entire screen)
    //    float opacityFill = m_theme.hasRain * flicker * fade * 0.15f;
    //    path.moveTo(0.0f, 0.0f); path.lineTo(w, 0.0f); path.lineTo(w, h); path.lineTo(0.0f, h); path.close();
    //    canvas.fillPath(path, SolidColor{ m_theme.sunDisk.withOpacity(opacityFill) }, &tx);

    //    // Lightning bolt path generation
    //    float opacityBolt = m_theme.hasRain * flicker * fade;
    //    path.clear();
    //    float lx = w * (0.3f + 0.4f * pseudoRand(1, seed));
    //    float ly = 0.0f;
    //    path.moveTo(lx, ly);

    //    for (int i = 1; i <= 6; ++i) {
    //        lx += (pseudoRand(i, seed) * 120.0f - 60.0f) * m_objectsScale;
    //        ly += h * 0.1f + pseudoRand(i + 10, seed) * h * 0.1f;
    //        path.lineTo(lx, ly);

    //        if (pseudoRand(i + 20, seed) > 0.5f) {
    //            float bx = lx + (pseudoRand(i + 30, seed) * 100.0f - 50.0f) * m_objectsScale;
    //            float by = ly + h * 0.15f + pseudoRand(i + 40, seed) * h * 0.05f;
    //            path.lineTo(bx, by);
    //            path.moveTo(lx, ly);
    //        }
    //    }

    //    // 1. Stage the entire bounds for the lightning bolt and its volumetric glow
    //    FloatRect boltBounds = { offset.x, offset.y, offset.x + w, offset.y + h };
    //    canvas.stagingDraw(boltBounds, [this, &canvas, &path, &tx, opacityBolt](Cpu::PixelView view)
    //        {
    //            bool wasActive = g_rasterBuffers.hasActiveClip;
    //            const float* oldClipMaskPtr = g_rasterBuffers.activeClipMaskPtr;
    //            g_rasterBuffers.hasActiveClip = false;
    //            g_rasterBuffers.activeClipMaskPtr = nullptr;

    //            PixelPathPainter p{ view };
    //            p.drawPath(path, {
    //                PathDrawLayer::Stroke(m_theme.sunDisk, 3.0f * m_objectsScale),
    //                PathDrawLayer::OuterGlow(m_theme.sunDisk, 20.0f * m_objectsScale)
    //                }, tx);

    //            canvas.drawPixelView(view);

    //            g_rasterBuffers.hasActiveClip = wasActive;
    //            g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
    //        });
    //}
}
