module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.CowSprite;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.FirTreeSprite;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.LightningEffect;
import ClaFi.PathArt.MillSprite;
import ClaFi.PathArt.PeekingWolfSprite;
import ClaFi.PathArt.ReedsSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterLilySprite;
import ClaFi.PathArt.WaterWheelSprite;
import ClaFi.PathArt.WeatherEffect;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Graphics::Cpu;

    namespace
    {
        [[nodiscard]] MillColors millColorsOf(const SceneTheme&);
    }

    void MillScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
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
        if (m_backgroundDimensions != newBackgroundDimensions
            || m_backgroundThemeVersion != m_themes.wornVersion())
        {
            m_backgroundDimensions = newBackgroundDimensions;
            m_backgroundThemeVersion = m_themes.wornVersion();
            prepareBackground(w, h);
        }

        // Draw pre-rendered background matching widget coordinate offset [CP]
        canvas.drawBitmap(m_background, m_offset);

        drawStars(frame, time.cycleTime, StarField{
            .style = StarStyle::Sparkles,
            .count = 120,
            .top = 0.0f,
            .bottom = h * 0.65f,
            .color = theme().sunDisk,
            .opacity = theme().hasStars
        });
        drawSunCorona(canvas, m_offset.x + m_sunPos.x, m_offset.y + m_sunPos.y, m_sunRadius,
            time.cycleTime);

        const float orbX = m_offset.x + w * 0.5f;
        const float orbY = m_offset.y + h * 0.2f;
        const float rX = w * 0.35f;
        const float rY = h * 0.04f;
        const float a1 = 0.5f * time.cycleTime * k_2Pi;
        const float a2 = a1 + 3.1415f;
        drawCloud(canvas, orbX + std::cos(a1) * rX, orbY + std::sin(a1) * rY,
            (0.8f + (std::sin(a1) + 1.0f) * 0.2f) * m_objectsScale);
        drawCloud(canvas, orbX + std::cos(a2) * rX, orbY + std::sin(a2) * rY,
            (0.8f + (std::sin(a2) + 1.0f) * 0.2f) * m_objectsScale);

        // The cow's horns stand against the sky at 0.58 of its height, its legs against the front
        // hill, which drawHills darkens by a tenth, at 0.38 of the hill's gradient.
        Hsl hillTop{ theme().hillTop };
        hillTop.scaleLuminosity(0.9f);
        Hsl hillBottom{ theme().hillBottom };
        hillBottom.scaleLuminosity(0.9f);
        drawCow(frame, CowPose{
            .at = { w * 0.28f, h * 0.61f },
            .size = 0.36f * m_objectsScale,
            .cycleTime = time.cycleTime
        }, CowColors{
            .body = theme().cowBody,
            .spots = theme().cowSpots,
            .snout = theme().cowSnout,
            .detail = theme().cowDetail,
            .shadow = theme().shadow,
            .skyBehind = colorBetween(theme().skyTop, theme().skyBottom, 0.58f),
            .groundBehind = colorBetween(hillTop.toColor(), hillBottom.toColor(), 0.38f)
        });

        drawWaterWheel(frame, WaterWheelPose{
            .center = { w * 0.5f - 65.0f * mS, h * 0.85f },
            .size = m_objectsScale,
            .turns = time.cycleTime * 2.0f
        }, theme().millWheel);

        const MillPose mill = {
            .base = { w * 0.5f, h * 0.85f },
            .size = mS,
            .wingTurns = time.cycleTime
        };
        const MillColors millColors = millColorsOf(theme());
        drawMillHouse(frame, mill, millColors);
        drawWaterFoam(canvas, m_offset.x + w * 0.5f - 75.0f * mS, m_offset.y + h * 0.85f,
            m_objectsScale, time.cycleTime * 1.25f);
        drawLightning(frame, time.cycleTime, Lightning{
            .strength = theme().hasRain,
            .color = theme().sunDisk
        });

        // Anchors River coordinate boundaries to layout constraints instead of clipBox [CP]
        FloatRect riverBounds = bounds;
        riverBounds.top = drawRiver(canvas, m_offset, w, h, time.cycleTime);
        const float riverTop = riverBounds.top - m_offset.y;

        // 1. Constrain river reflections inside riverbed
        //canvas.pushClip(riverBounds);

        const FloatPoint reflectPos = { m_offset.x + m_sunPos.x, riverBounds.center().y };
        canvas.fillRectangle(riverBounds, PointGlow{
            .lightPos = reflectPos,
            .lightColor = faded(theme().sunGlow, 0.2f),
            .lightSpread = 66.0f * m_objectsScale,
            .xRatio = 1.3f,
            .shape = GlowShape::Circle
        });

        drawWaterLily(frame, WaterLilyPose{
            .center = { w * 0.2f, riverTop + 40.0f * m_objectsScale },
            .size = m_objectsScale,
            .cycleTime = time.cycleTime
        }, WaterLilyColors{
            .pad = theme().lilyPad,
            .flower = theme().lilyFlower
        });

        // 2. Pop River Clip
        //canvas.popClip();

        const float reedsScale = m_objectsScale * 1.33f;
        const ReedColors reedColors = {
            .stem = theme().reedStem,
            .head = theme().reedHead
        };
        drawReeds(frame, ReedsPose{
            .base = { w * 0.3f, riverTop },
            .size = reedsScale,
            .cycleTime = time.cycleTime
        }, reedColors);
        drawReeds(frame, ReedsPose{
            .base = { w * 0.62f, riverTop },
            .size = reedsScale * 1.16f,
            .cycleTime = time.cycleTime + 0.3f
        }, reedColors);

        drawMillWings(frame, mill, millColors);

        const FirTreeColors treeColors = {
            .trunk = theme().treeTrunk,
            .leaves = theme().treeLeavesBase,
            .shadow = theme().shadow
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
            .cycleTime = time.cycleTime,
            .opacity = theme().isAfrica
        }, GatorColors{
            .body = theme().wolfBody,
            .eye = theme().wolfEye
        });
        drawPeekingWolf(frame, PeekingWolfPose{
            .at = predator,
            .size = m_objectsScale * 0.8f,
            .cycleTime = time.cycleTime,
            .opacity = 1.0f - theme().isAfrica
        }, PeekingWolfColors{
            .body = theme().wolfBody,
            .ear = theme().wolfEar,
            .eye = theme().wolfEye
        });
        drawFirTree(frame, { w * 0.85f, h * 0.70f }, 1.2f * m_objectsScale, treeColors);

        Fireflies fireflies = firefliesOf(theme());
        fireflies.area = { 0.0f, h * 0.4f, w, h * 0.8f };
        drawFireflies(frame, time.cycleTime, fireflies);
        drawWeather(frame, time.cycleTime, theme());

        // river fog
        canvas.fillRectangle(bounds, PointGlow{
            .lightPos = riverBounds.topCenter() + FloatPoint{ 0.0f, 20.0f * m_objectsScale },
            .lightColor = faded(theme().cloud, 0.33f),
            .lightSpread = 60.0f * m_objectsScale,
            .xRatio = 0.08f,
            .shape = GlowShape::Circle
        });
    }

    // In Fresh Spring's colours whatever theme is worn: an icon is one picture in every theme.
    void MillScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 480.0f };
        constexpr Themes::FreshSpring k_iconTheme{};
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        const MillPose mill = {
            .base = { frame.width * 0.5f, frame.height * 0.5f + 240.0f * unit },
            .size = 2.0f * unit,
            .wingTurns = 0.06f
        };
        const MillColors millColors = millColorsOf(k_iconTheme);
        // The house's reflection hangs below its foot, and the wing tips reach past the top.
        canvas.pushClip(FloatRect{ bounds.left, bounds.top, bounds.right,
            bounds.top + mill.base.y });
        drawMillHouse(frame, mill, millColors);
        drawMillWings(frame, mill, millColors);
        canvas.popClip();
    }

    // **The picked theme's colours, not the blend's**: a host asks the moment a theme is picked,
    // while the blend still wears the one before it.
    SceneReadoutColors MillScene::readoutColors() const
    {
        return readoutColorsOf(pickedTheme());
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
            PathDrawLayer::LinearGradient({ 0.0f, 0.0f }, { 0.0f, h }, theme().skyTop,
                theme().skyBottom),
            PathDrawLayer::PointGlow({
                .lightPos = m_sunPos,
                .lightColor = theme().sunGlow,
                .lightSpread = 166.0f * m_objectsScale,
                .xRatio = 0.5f,
                .shape = GlowShape::Circle
            })
        });

        // Sun Disk / Crescent
        path.clear();
        const float s = theme().hasStars;
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
            PathDrawLayer::Fill(theme().sunDisk),
            PathDrawLayer::Stroke(theme().sunStroke, 2.0f)
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
            canvas.fillPath(path, SolidColor{ theme().foam }, &tx);
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
            const Color ray = i % 2 == 0 ? theme().sunCorona1 : theme().sunCorona2;
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
    //                    //PathDrawLayer::OuterGlow(theme().cloud, 10.0f * s)
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
    //                PathDrawLayer::Fill(theme().cloud),
    //                PathDrawLayer::OuterGlow(theme().cloud, glowRadius)
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
                PathDrawLayer::Fill(theme().cloud),
                PathDrawLayer::OuterGlow(theme().cloud, 10.0f)
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
                theme().riverTop.withOpacity(0.8f),
                theme().riverBottom.withOpacity(0.8f)
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
            canvas.drawPath(path, SolidColor{ theme().riverWave }, 1.5f * m_objectsScale, &tx);
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
            PathDrawLayer::LinearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, theme().hillTop,
                theme().hillBottom)
        }, &tx);

        path.clear();
        path.moveTo(0.0f, h * 0.8f);
        path.quadTo({ w * 0.25f, h * 0.5f }, { w * 0.5f, h * 0.7f });
        path.lineTo(w * 0.5f, h);
        path.lineTo(0.0f, h);
        path.close();

        Hsl topHsl{ theme().hillTop };
        topHsl.scaleLuminosity(0.9f);
        Hsl botHsl{ theme().hillBottom };
        botHsl.scaleLuminosity(0.9f);
        p.drawPath(path, {
            PathDrawLayer::LinearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, topHsl.toColor(),
                botHsl.toColor())
        }, &tx);
    }

    namespace
    {
        MillColors millColorsOf(const SceneTheme& theme)
        {
            return {
                .bodyTop = theme.millBodyTop,
                .bodyBottom = theme.millBodyBottom,
                .roofTop = theme.millRoofTop,
                .roofBottom = theme.millRoofBottom,
                .wingWood = theme.millWingWood,
                .wingWoodStroke = theme.millWingWoodStroke,
                .wingFabric = theme.millWingFabric,
                .wingFabricStroke = theme.millWingFabricStroke
            };
        }
    }

    //void MillScene::drawLightning(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    //{
    //    if (theme().hasRain <= 0.001f) return;
    //    float strikePhase = t * 3.0f;
    //    float flashT = std::fmod(strikePhase, 1.0f);
    //    if (flashT > 0.15f) return;

    //    float seed = std::floor(strikePhase);
    //    float flicker = (std::sin(flashT * 150.0f) > 0.0f) ? 1.0f : 0.0f;
    //    float fade = 1.0f - (flashT / 0.15f);

    //    Matrix3x2 tx = Matrix3x2::Translation(offset);
    //    PixelPath path;

    //    // Soft sky flash (Fills the entire screen)
    //    float opacityFill = theme().hasRain * flicker * fade * 0.15f;
    //    path.moveTo(0.0f, 0.0f); path.lineTo(w, 0.0f); path.lineTo(w, h); path.lineTo(0.0f, h); path.close();
    //    canvas.fillPath(path, SolidColor{ theme().sunDisk.withOpacity(opacityFill) }, &tx);

    //    // Lightning bolt path generation
    //    float opacityBolt = theme().hasRain * flicker * fade;
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
    //                PathDrawLayer::Stroke(theme().sunDisk, 3.0f * m_objectsScale),
    //                PathDrawLayer::OuterGlow(theme().sunDisk, 20.0f * m_objectsScale)
    //                }, tx);

    //            canvas.drawPixelView(view);

    //            g_rasterBuffers.hasActiveClip = wasActive;
    //            g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
    //        });
    //}
}
