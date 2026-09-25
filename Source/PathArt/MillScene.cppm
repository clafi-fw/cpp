export module ClaFi.PathArt.MillScene;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.System.Animation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.System.UiTypes;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.Themes;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Graphics.Cpu_Rasterizer;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Graphics::Cpu;

    export class MillScene
    {
    public:
        MillScene();
        std::size_t themeIndex() const { return m_themeIndex; }
        // WHAT THE SCENE IS PAINTING, which between two picks is a blend of them and not any
        // theme in the list.
        const SceneTheme& theme() const { return m_theme; }
        // The theme it was told to wear, blend or no blend - what a caller taking colours or a
        // name off the scene means.
        const SceneTheme& pickedTheme() const { return Themes::allThemes[m_themeIndex]; }
        void setTheme(std::size_t index);
        void animationTick();

        // Exact target bounds passed explicitly to anchor layout geometry
        void paint(Canvas& canvas, const FloatRect& bounds, float t);

    private:
        void crossFadeToPickedTheme();
        void prepareBackground(float w, float h);
        float pseudoRand(int i, float shift = 0.0f);
        float calculateSyncRotation(float t, int id, float symmetryFactor);
        void drawShadow(Canvas& canvas, float x, float y, float r) const;

        // Render Helpers (Dynamic elements render directly via Canvas&)
        void drawWaterWheel(Canvas& canvas, float x, float y, float t);
        void drawReeds(Canvas& canvas, float x, float y, float s, float t);
        void drawWaterFoam(Canvas& canvas, float x, float y, float mS, float t) const;
        void drawWaterLily(Canvas& canvas, float x, float y, float scale, float t);
        void drawPredator(Canvas& canvas, float x, float y, float s, float t);
        void drawWolf(Canvas& canvas, float x, float y, float s, float t);
        void drawGator(Canvas& canvas, float x, float y, float s, float t);
        void drawCow(Canvas& canvas, float x, float y, float s, float t);
        void drawTree(Canvas& canvas, float x, float y, float s);
        void drawMillWings(Canvas& canvas, FloatPoint off, float w, float h, float s, float t);
        void drawMillBody(Canvas& canvas, FloatPoint off, float w, float h, float s, float turns);
        void drawSunCorona(Canvas& canvas, float x, float y, float r, float t);
        void drawCloud(Canvas& canvas, float x, float y, float s);
        float drawRiver(Canvas& canvas, FloatPoint off, float w, float h, float t);

        // Background Static Helper (Accepts PixelPathPainter for offline bitmap caching)
        void drawHills(PixelPathPainter& p, FloatPoint off, float w, float h);

        // Atmospheric Effects
        void drawRain(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawLightning(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawStars(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawSnowflake(Canvas& canvas, float x, float y, float s, float t, int id);
        void drawSnow(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawLeaf(Canvas& canvas, float x, float y, float s, float t, int id, Color color, float opacity);
        void drawLeavesOrPetals(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawFireflies(Canvas& canvas, float w, float h, FloatPoint offset, float t);
        void drawDust(Canvas& canvas, float w, float h, FloatPoint offset, float t);
    private:
        std::size_t m_themeIndex{ 0 };
        // NULL UNTIL THE CONSTRUCTOR FADES THE FIRST THEME IN - see MillScene::MillScene.
        SceneTheme m_theme{};
        SceneTheme m_prevTheme{};
        const SceneTheme* m_nextTheme{};
        Bitmap m_background{};
        ScaledDimensions m_backgroundDimensions{};
        FloatPoint m_sunPos;
        float m_sunRadius;
        FloatPoint m_offset;
        float m_objectsScale;
        AnimationController m_animator{};
    };
}

namespace ClaFi::PathArt
{
    using namespace std::chrono_literals;
    AnimationSlot g_switchThemeSlot{
        .duration{.rise{ 555ms} },
        .easingFactor{ EasingFactor::EaseInOut }
    };

    // ALWAYS A FADE IN, NEVER A CUT. The scene stands on a null theme - every colour zero and
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

    void MillScene::paint(Canvas& canvas, const FloatRect& bounds, float t)
    {
        m_offset = bounds.topLeft(); // Anchored to widget's logical position
        float w = bounds.width();
        float h = bounds.height();
        m_objectsScale = std::min(h, w) / 600.0f;
        m_sunPos = { w * 0.75f, h * 0.15f };
        m_sunRadius = 35.0f * m_objectsScale;

        ScaledDimensions newBackgroundDimensions = bounds.dimensions();
        if (m_backgroundDimensions != newBackgroundDimensions)
        {
            m_backgroundDimensions = newBackgroundDimensions;
            prepareBackground(w, h);
        }

        // Draw pre-rendered background matching widget coordinate offset [CP]
        canvas.drawBitmap(m_background, m_offset);

        drawStars(canvas, w, h, m_offset, t);
        drawSunCorona(canvas, m_offset.x + m_sunPos.x, m_offset.y + m_sunPos.y, m_sunRadius, t);

        float orbX = m_offset.x + w * 0.5f, orbY = m_offset.y + h * 0.2f;
        float rX = w * 0.35f, rY = h * 0.04f, a1 = 0.5f * t * k_2Pi;
        drawCloud(canvas, orbX + std::cos(a1) * rX, orbY + std::sin(a1) * rY, (0.8f + (std::sin(a1) + 1.0f) * 0.2f) * m_objectsScale);
        drawCloud(canvas, orbX + std::cos(a1 + 3.1415f) * rX, orbY + std::sin(a1 + 3.1415f) * rY, (0.8f + (std::sin(a1 + 3.1415f) + 1.0f) * 0.2f) * m_objectsScale);

        drawCow(canvas, m_offset.x + w * 0.28f, m_offset.y + h * 0.61f, 0.36f * m_objectsScale, t);

        float mS = m_objectsScale;
        drawWaterWheel(canvas, m_offset.x + w * 0.5f - 65.0f * mS, m_offset.y + h * 0.85f, t);

        drawMillBody(canvas, m_offset, w, h, mS, 0.0f);
        drawMillBody(canvas, m_offset, w, h, mS, 0.5f);
        drawWaterFoam(canvas, m_offset.x + w * 0.5f - 75.0f * mS, m_offset.y + h * 0.85f, m_objectsScale, t * 1.25f);
        drawLightning(canvas, w, h, m_offset, t);

        // Anchors River coordinate boundaries to layout constraints instead of clipBox [CP]
        FloatRect riverBounds = bounds;
        riverBounds.top = drawRiver(canvas, m_offset, w, h, t);

        // 1. Constrain river reflections inside riverbed
        //canvas.pushClip(riverBounds);

        FloatPoint reflectPos = { m_offset.x + m_sunPos.x, riverBounds.center().y };
        canvas.fillRectangle(riverBounds, PointGlow{
            .lightPos = reflectPos,
            .lightColor = m_theme.sunGlow,
            .lightSpread = 66.0f * m_objectsScale,
            .xRatio = 1.3f,
            .shape = GlowShape::Circle,
            .opacity = 0.2f
            });

        drawWaterLily(canvas, m_offset.x + w * 0.2f, riverBounds.top + 40.0f * m_objectsScale, m_objectsScale, t);

        // 2. Pop River Clip
        //canvas.popClip();

        const float reedsScale = m_objectsScale * 1.33f;
        drawReeds(canvas, m_offset.x + w * 0.3f, riverBounds.top, reedsScale, t);
        drawReeds(canvas, m_offset.x + w * 0.62f, riverBounds.top, reedsScale * 1.16f, t + 0.3f);


        drawMillWings(canvas, m_offset, w, h, mS, t);

        drawTree(canvas, m_offset.x + w * 0.15f, m_offset.y + h * 0.75f, 0.8f * m_objectsScale);
        drawPredator(canvas, m_offset.x + w * 0.85f - 29.0f * m_objectsScale, m_offset.y + h * 0.70f - 24.0f * m_objectsScale, m_objectsScale * 0.8f, t);
        drawTree(canvas, m_offset.x + w * 0.85f, m_offset.y + h * 0.70f, 1.2f * m_objectsScale);

        if (m_theme.hasRain) drawRain(canvas, w, h, m_offset, t);
        if (m_theme.hasSnow) drawSnow(canvas, w, h, m_offset, t);
        if (m_theme.hasFireflies) drawFireflies(canvas, w, h, m_offset, t);
        if (m_theme.hasLeaves || m_theme.hasPetals) drawLeavesOrPetals(canvas, w, h, m_offset, t);
        if (m_theme.hasDust) drawDust(canvas, w, h, m_offset, t);

        // river fog
        canvas.fillRectangle(bounds, PointGlow{
            .lightPos = riverBounds.topCenter() + FloatPoint{ 0.0f, 20.0f * m_objectsScale },
            .lightColor = m_theme.cloud,
            .lightSpread = 60.0f * m_objectsScale,
            .xRatio = 0.08f,
            .shape = GlowShape::Circle,
            .opacity = 0.33f
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
                float v1 = m_prevTheme.effectsArray()[i];
                float v2 = m_nextTheme->effectsArray()[i];
                m_theme.effectsArray()[i] = v1 * (1.0f - params.value) + v2 * params.value;
            }
            m_backgroundDimensions = {}; // to rebuild background
            });
    }

    void MillScene::prepareBackground(float w, float h)
    {
        m_background.resize(m_backgroundDimensions.roundOut());

        // 1. Temporarily suspend any active global window clips [CP]
        // This prevents the offscreen buffer from being zero-clipped by parent widgets!
        bool wasActive = g_rasterBuffers.hasActiveClip;
        const float* oldClipMaskPtr = g_rasterBuffers.activeClipMaskPtr;
        FloatRect oldClipBox = g_rasterBuffers.clipBox;
        int oldClipStride = g_rasterBuffers.m_clipStride;

        g_rasterBuffers.hasActiveClip = false;
        g_rasterBuffers.activeClipMaskPtr = nullptr;

        // 2. Render the background in pure local space
        PixelPathPainter p{ m_background.pixelView() };
        PixelPath path;

        // Sky and sun glow
        path.moveTo(0, 0); path.lineTo(w, 0); path.lineTo(w, h); path.lineTo(0, h); path.close();
        p.drawPath(path, {
            PathDrawLayer::linearGradient({ 0, 0 }, { 0, h }, m_theme.skyTop, m_theme.skyBottom),
            PathDrawLayer::pointGlow({
                .lightPos = m_sunPos,
                .lightColor = m_theme.sunGlow,
                .lightSpread = 166.0f * m_objectsScale,
                .xRatio = 0.5f,
                .shape = GlowShape::Circle,
                .opacity = 1.0f
            })
            });

        // Sun Disk / Crescent
        path.clear();
        float s = m_theme.hasStars;
        Matrix3x2 sunTx = Matrix3x2::translation(m_sunPos) * Matrix3x2::rotation((s * 0.1f) * k_2Pi);

        float r = m_sunRadius;
        float c = r * 0.4142f;
        float q = r * 0.7071f;
        float eq = -q * (1.0f - s) + (0.28f * r) * s;
        float er = -r * (1.0f - s) + (0.50f * r) * s;
        float cc1 = -c * (1.0f - s) + (0.15f * r) * s;
        float cc2 = -r * (1.0f - s) + (0.50f * r) * s;

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

        p.drawPath(path, { PathDrawLayer::fill(m_theme.sunDisk), PathDrawLayer::stroke(m_theme.sunStroke, 2.0f) }, &sunTx);

        // Hills
        drawHills(p, {}, w, h);

        // 3. Restore global clipping state to prevent messing up the rest of the canvas loop
        g_rasterBuffers.hasActiveClip = wasActive;
        g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
        g_rasterBuffers.clipBox = oldClipBox;
        g_rasterBuffers.m_clipStride = oldClipStride;
    }

    // --- Utils ---

    float MillScene::pseudoRand(int i, float shift)
    {
        return std::fmod(std::abs(std::sin((float)i * 12.9898f + shift)) * 43758.5453f, 1.0f);
    }

    float MillScene::calculateSyncRotation(float t, int id, float symmetryFactor)
    {
        float spinDir = (id % 2 == 0) ? 1.0f : -1.0f;
        float rotationsPerLoop = static_cast<float>(1 + (id % 2));
        return t * (1.0f / symmetryFactor) * spinDir * rotationsPerLoop + id;
    }

    void MillScene::drawShadow(Canvas& canvas, float x, float y, float r) const
    {
        FloatRect shadowRect = { x - (r / 0.5f), y - r, x + (r / 0.5f), y + r };
        canvas.fillRectangle(shadowRect, PointGlow{
            .lightPos = { x, y },
            .lightColor = m_theme.shadow,
            .lightSpread = r,
            .xRatio = 0.5f,
            .shape = GlowShape::Circle,
            .opacity = 0.3f
            });
    }

    // --- Draw Objects ---

    void MillScene::drawWaterWheel(Canvas& canvas, float x, float y, float t)
    {
        Matrix3x2 baseTx = Matrix3x2::translation(x, y) * Matrix3x2::rotation((-t * 2.0f) * k_2Pi) * Matrix3x2::scale(m_objectsScale);
        PixelPath path;
        path.drawCircle({}, 40.0f);
        const Color rimColor{ m_theme.millWheel, 0xff'000000, 0.2f };
        canvas.drawPath(path, SolidColor{ rimColor }, 5.0f, &baseTx);

        for (int i = 0; i < 8; ++i) {
            path.clear();
            Matrix3x2 spokeTx = Matrix3x2::translation(x, y) * Matrix3x2::rotation((-t * 2.0f + (i * 0.125f)) * k_2Pi) * Matrix3x2::scale(m_objectsScale);
            path.moveTo(-2.0f, 0.0f);
            path.lineTo(2.0f, 0.0f);
            path.lineTo(5.0f, 38.0f);
            path.lineTo(-5.0f, 38.0f);
            path.close();
            canvas.fillPath(path, SolidColor{ m_theme.millWheel }, &spokeTx);
        }
    }

    void MillScene::drawReeds(Canvas& canvas, float x, float y, float s, float t)
    {
        for (int r = 0; r != 2; ++r) {
            for (int i = 0; i < 3; ++i) {
                float ox = x + (i * 12.0f * s), sway = std::sin((t + (r * 0.5f)) * k_2Pi + i) * 0.08f;
                Matrix3x2 tx = Matrix3x2::translation(ox, y) * Matrix3x2::rotation((sway + (r * 0.5f)) * k_2Pi) * Matrix3x2::scale(s);
                float opacity = 1.0f - (r * 0.85f);

                PixelPath path;
                path.moveTo(0.0f, 0.0f);
                path.lineTo(0.0f, -40.0f);
                canvas.drawPath(path, SolidColor{ m_theme.reedStem.withOpacity(opacity) }, 2.0f, &tx);

                path.clear();
                path.moveTo(-2.5f, -25.0f);
                path.lineTo(-2.5f, -42.0f);
                path.quadTo({ -2.5f, -44.5f }, { 0.0f, -44.5f });
                path.quadTo({ 2.5f, -44.5f }, { 2.5f, -42.0f });
                path.lineTo(2.5f, -25.0f);
                path.quadTo({ 2.5f, -22.5f }, { 0.0f, -22.5f });
                path.quadTo({ -2.5f, -22.5f }, { -2.5f, -25.0f });
                path.close();

                Hsl headTop{ m_theme.reedHead };
                Hsl headBot{ m_theme.reedHead };

                headTop.scaleLuminosity(1.15f);
                headBot.scaleLuminosity(0.85f);

                canvas.fillPath(
                    path,
                    LinearGradient::simple(
                        { 0.0f, -44.5f },
                        { 0.0f, -22.5f },
                        headTop.toColor().withOpacity(opacity),
                        headBot.toColor().withOpacity(opacity)), &tx);
            }
        }
    }

    void MillScene::drawWaterFoam(Canvas& canvas, float x, float y, float mS, float t) const
    {
        Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::scale(mS);
        for (int i = -2; i <= 2; ++i) {
            PixelPath path;
            float ox = i * 15.0f + std::sin(t * 10.0f + i) * 5.0f;
            path.moveTo(ox - 10.0f, 0.0f);
            path.quadTo({ ox, -10.0f }, { ox + 10.0f, 0.0f });
            canvas.fillPath(path, SolidColor{ m_theme.foam }, &tx);
        }
    }

    void MillScene::drawWaterLily(Canvas& canvas, float x, float y, float scale, float t)
    {
        float bob = std::sin(t * k_2Pi * 1.5f) * 3.0f * scale;
        Matrix3x2 tx = Matrix3x2::translation(x, y + bob) * Matrix3x2::scale(scale);

        PixelPath path;
        const float r = 22.0f;
        path.moveTo(0.0f, 0.0f);
        path.lineTo(r, -5.0f);
        path.quadTo({ r, r }, { 0, r });
        path.quadTo({ -r, r }, { -r, 0 });
        path.quadTo({ -r, -r }, { 0, -r });
        path.quadTo({ r, -r }, { r, -5 });
        path.close();
        canvas.fillPath(path, SolidColor{ m_theme.lilyPad }, &tx);

        for (int i = 0; i < 5; ++i) {
            path.clear();
            Matrix3x2 rotTx = tx * Matrix3x2::rotation((i * 0.2f) * k_2Pi);
            path.moveTo(0.0f, 0.0f);
            path.lineTo(-6.0f, -12.0f);
            path.lineTo(0.0f, -18.0f);
            path.lineTo(6.0f, -12.0f);
            path.close();
            canvas.fillPath(path, SolidColor{ m_theme.lilyFlower }, &rotTx);
        }
    }

    void MillScene::drawPredator(Canvas& canvas, float x, float y, float s, float t)
    {
        if (m_theme.isAfrica) drawGator(canvas, x, y, s, t);
        if (m_theme.isAfrica != 1.0f) drawWolf(canvas, x, y, s, t);
    }

    void MillScene::drawWolf(Canvas& canvas, float x, float y, float s, float t)
    {
        float opacity = 1.0f - m_theme.isAfrica;
        if (opacity <= 0.0f) return;

        float breath = (std::sin(t * k_2Pi * 2.0f) * 3.0f) - 5.0f * s;

        // Hind Leg
        {
            Matrix3x2 legTx = Matrix3x2::translation(x + s * 25.0f, y - s * 26.0f) * Matrix3x2::scale(s * 0.66f);
            PixelPath path;
            path.moveTo(30.0f, 37.0f + breath * s);
            path.lineTo(65.0f, 52.0f + breath * s * 0.5f);
            path.lineTo(60.0f, 60.0f);
            canvas.fillPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, &legTx);
            canvas.drawPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, 6.0f, &legTx);
        }

        float earT = std::sin(t * 50.0f) > 0.95f ? 0.2f : 0.0f;
        // Ears
        {
            Matrix3x2 earTx1 = Matrix3x2::translation(x - 8.0f * s, y + breath) * Matrix3x2::rotation((-0.3f + earT) * k_2Pi) * Matrix3x2::scale(s * 1.25f);
            PixelPath path;
            path.moveTo(-8.0f, 0.0f); path.lineTo(8.0f, 0.0f); path.lineTo(0.0f, -20.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.wolfEar.withOpacity(opacity) }, &earTx1);
        }
        {
            Matrix3x2 earTx2 = Matrix3x2::translation(x + 8.0f * s, y + 5.0f * s + breath) * Matrix3x2::rotation(0.25f * k_2Pi) * Matrix3x2::scale(s * 1.25f);
            PixelPath path;
            path.moveTo(-8.0f, 0.0f); path.lineTo(8.0f, 0.0f); path.lineTo(0.0f, -20.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.wolfEar.withOpacity(opacity) }, &earTx2);
        }
        // Snout
        {
            Matrix3x2 headTx = Matrix3x2::translation(x, y + breath) * Matrix3x2::scale(s * 1.25f);
            PixelPath path;
            path.moveTo(-10.0f, 0.0f); path.lineTo(10.0f, 0.0f); path.lineTo(15.0f, 15.0f); path.lineTo(-2.0f, 30.0f); path.lineTo(-15.0f, 15.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, &headTx);
        }

        float blink = (std::sin(t * 12.0f) > 0.8f) ? 0.0f : 1.0f;
        if (blink > 0.0f)
        {
            Matrix3x2 eyeTx = Matrix3x2::translation(x, y + breath + s * 4.0f) * Matrix3x2::scale(s * 1.25f);
            PixelPath path;
            float eyeBegin = 3.0f, eyeEnd = 8.0f, xShift = -1.0f, yShift = 8.0f, eyeTop = 10.0f, eyeBottom = 12.0f;
            path.moveTo(xShift - eyeEnd, yShift); path.lineTo(xShift - eyeBegin, eyeTop); path.lineTo(xShift - eyeEnd, eyeBottom); path.close();
            path.moveTo(xShift + eyeBegin, yShift); path.lineTo(xShift + eyeEnd, eyeTop); path.lineTo(xShift + eyeBegin, eyeBottom); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.wolfEye.withOpacity(opacity) }, &eyeTx);
        }
    }

    void MillScene::drawGator(Canvas& canvas, float x, float y, float s, float t)
    {
        float opacity = m_theme.isAfrica;
        if (opacity <= 0.0f) return;

        Matrix3x2 baseTx = Matrix3x2::translation(x, y) * Matrix3x2::scale(s);
        PixelPath path;

        // Jaw
        path.moveTo(80.0f, 25.0f); path.lineTo(-60.0f, 25.0f); path.lineTo(-60.0f, 35.0f); path.lineTo(80.0f, 35.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, &baseTx);

        // Lower Teeth
        path.clear();
        for (int i = 0; i < 4; ++i) {
            float tx = -10.0f - i * 14.0f;
            path.moveTo(tx, 25.0f); path.lineTo(tx - 4.0f, 25.0f); path.lineTo(tx - 2.0f, 20.0f); path.close();
        }
        canvas.fillPath(path, SolidColor{ Color(220, 220, 220).withOpacity(opacity) }, &baseTx);

        // Upper Jaw Snapping
        float snapVal = (std::fmod(t * 5.0f, 1.0f) > 0.88f) ? 0.05f : 0.0f;
        Matrix3x2 jawTx = Matrix3x2::translation(x + 30.0f * s, y + 25.0f * s) * Matrix3x2::rotation(snapVal * k_2Pi) * Matrix3x2::scale(s);

        path.clear();
        path.moveTo(0.0f, 0.0f); path.lineTo(-100.0f, 0.0f); path.lineTo(-95.0f, -15.0f); path.lineTo(0.0f, -15.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, &jawTx);

        // Upper Teeth
        path.clear();
        path.moveTo(-20.0f, 0.0f);
        for (int i = 0; i < 5; ++i) {
            path.lineBy(-3.5f, 10.0f);
            path.lineBy(-3.5f, -10.0f);
            path.moveBy(-9.0f, 0.0f);
        }
        canvas.fillPath(path, SolidColor{ Color(255, 255, 255).withOpacity(opacity) }, &jawTx);

        // Eye Bump
        path.clear();
        path.moveTo(-15.0f, -15.0f); path.quadTo({ -25.0f, -28.0f }, { -35.0f, -15.0f }); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.wolfBody.withOpacity(opacity) }, &jawTx);

        float blink = (std::sin(t * 15.0f) > 0.9f) ? 0.0f : 1.0f;
        if (blink > 0.0f) {
            path.clear();
            path.moveTo(-22.0f, -16.0f); path.lineTo(-30.0f, -20.0f); path.lineTo(-28.0f, -15.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.wolfEye.withOpacity(opacity) }, &jawTx);
        }
    }

    void MillScene::drawCow(Canvas& canvas, float x, float y, float s, float t)
    {
        drawShadow(canvas, x, y + 50.0f * s, 40.0f * s);

        PixelPath path;
        Matrix3x2 tailTx = Matrix3x2::translation(x + 40.0f * s, y - 10.0f * s) * Matrix3x2::rotation((std::sin(t * 30.0f) * 0.4f + 0.5f) * k_2Pi) * Matrix3x2::scale(s);
        path.moveTo(0.0f, 0.0f); path.lineTo(0.0f, 25.0f);
        canvas.drawPath(path, SolidColor{ m_theme.cowDetail }, 3.0f, &tailTx);

        Matrix3x2 bodyTx = Matrix3x2::translation(x, y) * Matrix3x2::scale(s);
        path.clear();
        path.moveTo(-45.0f, 0.0f);
        path.quadTo({ -45.0f, -40.0f }, { 0.0f, -40.0f });
        path.quadTo({ 45.0f, -40.0f }, { 45.0f, 0.0f });
        path.lineTo(45.0f, 20.0f); path.lineTo(-45.0f, 20.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.cowBody }, &bodyTx);

        path.clear();
        path.moveTo(-20.0f, -35.0f); path.quadTo({ -10.0f, -45.0f }, { 0.0f, -30.0f }); path.lineTo(5.0f, -15.0f); path.close();
        path.moveTo(20.0f, -10.0f); path.drawCircle({}, 12.0f);
        canvas.fillPath(path, SolidColor{ m_theme.cowSpots }, &bodyTx);

        path.clear();
        path.moveTo(-35.0f, 20.0f); path.lineTo(-35.0f, 40.0f);
        path.moveTo(-20.0f, 20.0f); path.lineTo(-20.0f, 40.0f);
        path.moveTo(20.0f, 20.0f); path.lineTo(20.0f, 40.0f);
        path.moveTo(35.0f, 20.0f); path.lineTo(35.0f, 40.0f);
        canvas.drawPath(path, SolidColor{ m_theme.cowDetail }, 6.0f, &bodyTx);

        float m = std::abs(std::sin(t * 15.0f)) * 8.0f;
        Matrix3x2 headTx = Matrix3x2::translation(x - 45.0f * s, y - (15.0f - m) * s) * Matrix3x2::rotation(-0.2f * k_2Pi) * Matrix3x2::scale(s);

        path.clear();
        path.moveTo(-15.0f, -15.0f); path.lineTo(15.0f, -15.0f); path.lineTo(20.0f, 15.0f); path.lineTo(-20.0f, 15.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.cowBody }, &headTx);
        canvas.drawPath(path, SolidColor{ m_theme.cowDetail }, 1.0f, &headTx);

        path.clear();
        path.moveTo(-18.0f, 5.0f); path.lineTo(18.0f, 5.0f); path.lineTo(18.0f, 18.0f); path.lineTo(-18.0f, 18.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.cowSnout }, &headTx);
        canvas.drawPath(path, SolidColor{ m_theme.cowDetail }, 1.0f, &headTx);

        path.clear();
        path.moveTo(-15.0f, -15.0f); path.lineTo(-25.0f, -25.0f); path.lineTo(-15.0f, -5.0f); path.close();
        path.moveTo(15.0f, -15.0f); path.lineTo(25.0f, -25.0f); path.lineTo(15.0f, -5.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.cowDetail }, &headTx);
    }

    void MillScene::drawTree(Canvas& canvas, float x, float y, float s)
    {
        drawShadow(canvas, x, y, 24.0f * s);
        Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::scale(s);

        PixelPath path;
        path.moveTo(-10.0f, 0.0f); path.lineTo(10.0f, 0.0f); path.lineTo(10.0f, -30.0f); path.lineTo(-10.0f, -30.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.treeTrunk }, &tx);

        Hsl hsl{ m_theme.treeLeavesBase };
        const float satK = 0.9f;
        const float lumK = 1.0f + std::min(0.16f, (0.95f - hsl.luminosity) * 0.5f);
        for (float i = 0.0f; i < 2.99f; i += 1.0f) {
            float yO = -20.0f - (i * 30.0f);
            path.clear();
            path.moveTo(-40.0f + (i * 5.0f), yO);
            path.lineTo(40.0f - (i * 5.0f), yO);
            path.lineTo(0.0f, yO - 50.0f);
            path.close();
            canvas.fillPath(path, SolidColor{ hsl.toColor() }, &tx);
            hsl.scaleSaturation(satK);
            hsl.scaleLuminosity(lumK);
        }
    }

    void MillScene::drawMillWings(Canvas& canvas, FloatPoint off, float w, float h, float s, float t)
    {
        float cX = off.x + w * 0.5f, pY = off.y + h * 0.85f - 130.0f * s;
        Matrix3x2 baseTx = Matrix3x2::translation(cX, pY) * Matrix3x2::scale(s);

        PixelPath path;
        path.moveTo(-10.0f, -10.0f); path.lineTo(10.0f, -10.0f); path.lineTo(10.0f, 10.0f); path.lineTo(-10.0f, 10.0f); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.millWingWood }, &baseTx);

        for (float i = 0.0f; i < 3.9f; i += 1.0f) {
            Matrix3x2 wingTx = Matrix3x2::translation(cX, pY) * Matrix3x2::rotation((t + (i * 0.25f)) * k_2Pi) * Matrix3x2::scale(s);

            path.clear();
            path.moveTo(-4.0f, 0.0f); path.lineTo(4.0f, 0.0f); path.lineTo(4.0f, -120.0f); path.lineTo(-4.0f, -120.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.millWingWood }, &wingTx);
            canvas.drawPath(path, SolidColor{ m_theme.millWingWoodStroke }, 1.5f, &wingTx);

            path.clear();
            path.moveTo(4.0f, -40.0f); path.lineTo(30.0f, -45.0f); path.lineTo(30.0f, -115.0f); path.lineTo(4.0f, -110.0f); path.close();
            canvas.fillPath(path, SolidColor{ m_theme.millWingFabric.withOpacity(0.75f) }, &wingTx);
            canvas.drawPath(path, SolidColor{ m_theme.millWingFabricStroke.withOpacity(0.75f) }, 1.5f, &wingTx);
        }
    }

    void MillScene::drawMillBody(Canvas& canvas, FloatPoint off, float w, float h, float s, float turns)
    {
        float cX = off.x + w * 0.5f, bY = off.y + h * 0.85f;
        Matrix3x2 tx = Matrix3x2::translation(cX, bY) * Matrix3x2::rotation(turns * k_2Pi) * Matrix3x2::scale(s);

        PixelPath path;
        path.moveTo(-60.0f, 0.0f); path.lineTo(-60.0f, -100.0f); path.lineTo(60.0f, -100.0f); path.lineTo(60.0f, 0.0f); path.close();
        canvas.fillPath(path, LinearGradient::simple({ 40.0f, -100.0f }, { -40.0f, -80.0f }, m_theme.millBodyTop, m_theme.millBodyBottom), &tx);

        path.clear();
        path.moveTo(-75.0f, -100.0f); path.lineTo(0.0f, -160.0f); path.lineTo(75.0f, -100.0f); path.close();
        canvas.fillPath(path, LinearGradient::simple({ 40.0f, -100.0f }, { -40.0f, -80.0f }, m_theme.millRoofTop, m_theme.millRoofBottom), &tx);
    }

    void MillScene::drawSunCorona(Canvas& canvas, float x, float y, float r, float t)
    {
        t *= 0.5f;
        float minL = r * 1.3f, spr = r * 0.25f, rW = r * 0.12f, rB = r * 1.1f;
        for (int i = 0; i < 16; ++i) {
            float sV = std::sin(k_2Pi * (t * 2.0f) + static_cast<float>(i) * 0.8f);
            float dL = minL + (spr * (0.5f + 0.5f * sV));
            Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::rotation((t + static_cast<float>(i) / 16.0f) * k_2Pi);

            PixelPath path;
            path.moveTo(-rW, -rB); path.lineTo(rW, -rB); path.lineTo(0.0f, -dL); path.close();
            canvas.fillPath(path, SolidColor{ i % 2 == 0 ? m_theme.sunCorona1 : m_theme.sunCorona2 }, &tx);
        }
    }

    //void MillScene::drawCloud(Canvas& canvas, float x, float y, float s)
    //{
    //    Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::scale(s);
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
    //                    PathDrawLayer::fill(0xffff00ff)
    //                    //PathDrawLayer::outerGlow(m_theme.cloud, 10.0f * s)
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
    //                PathDrawLayer::fill(m_theme.cloud),
    //                PathDrawLayer::outerGlow(m_theme.cloud, glowRadius)
    //                }, tx);

    //    });
    //    // Restore active clips
    //    g_rasterBuffers.hasActiveClip = wasActive;
    //    g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
    //}

    void MillScene::drawCloud(Canvas& canvas, float x, float y, float s)
    {
        Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::scale(s);
        PixelPath path;
        path.moveTo(-60.0f, 0.0f);
        path.quadTo({ -40.0f, -40.0f }, { -20.0f, -20.0f });
        path.quadTo({ 0.0f, -60.0f }, { 20.0f, -20.0f });
        path.quadTo({ 40.0f, -40.0f }, { 60.0f, 0.0f });
        path.lineTo(60.0f, 20.0f); path.lineTo(-60.0f, 20.0f); path.close();

        // Direct, zero-copy, multi-layered drawing! [CP]
        canvas.drawPath(
            path,
            {
                PathDrawLayer::fill(m_theme.cloud),
                PathDrawLayer::outerGlow(m_theme.cloud, 10.0f)
            },
            &tx
        );
    }

    float MillScene::drawRiver(Canvas& canvas, FloatPoint off, float w, float h, float t)
    {
        float rT = h * 0.85f;
        Matrix3x2 tx = Matrix3x2::translation(off);
        PixelPath path;
        path.moveTo(0.0f, rT); path.lineTo(w, rT); path.lineTo(w, h); path.lineTo(0.0f, h); path.close();

        canvas.fillPath(
            path,
            LinearGradient::simple(
                { w * 0.5f, rT },
                { w * 0.5f, h },
                m_theme.riverTop.withOpacity(0.8f),
                m_theme.riverBottom.withOpacity(0.8f)
            ),
            &tx
        );

        for (int i = 0; i < 3; ++i) {
            float xP = (t + i / 3.0f);
            if (xP > 1.0f) xP -= 1.0f;
            float yO = rT + (h - rT) * 0.4f + i * 10 * m_objectsScale;
            float wS = 16 * m_objectsScale, xS = xP * w + wS;

            path.clear();
            path.moveTo({ xS - wS, yO });
            path.quadTo({ xS, yO - 5 * m_objectsScale }, { xS + wS, yO });
            path.close();
            canvas.drawPath(path, SolidColor{ m_theme.riverWave }, 1.5f * m_objectsScale, &tx);
        }
        return rT + off.y;
    }

    void MillScene::drawHills(PixelPathPainter& p, FloatPoint off, float w, float h)
    {
        Matrix3x2 tx = Matrix3x2::translation(off);
        PixelPath path;
        path.moveTo(0.0f, h * 0.8f);
        path.lineTo(w * 0.5f, h * 0.7f);
        path.quadTo({ w * 0.75f, h * 0.4f }, { w, h * 0.75f });
        path.lineTo(w, h); path.lineTo(0.0f, h); path.close();
        p.drawPath(path, { PathDrawLayer::linearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, m_theme.hillTop, m_theme.hillBottom) }, &tx);

        path.clear();
        path.moveTo(0.0f, h * 0.8f);
        path.quadTo({ w * 0.25f, h * 0.5f }, { w * 0.5f, h * 0.7f });
        path.lineTo(w * 0.5f, h); path.lineTo(0.0f, h); path.close();

        Hsl topHsl{ m_theme.hillTop }; topHsl.scaleLuminosity(0.9f);
        Hsl botHsl{ m_theme.hillBottom }; botHsl.scaleLuminosity(0.9f);
        p.drawPath(path, { PathDrawLayer::linearGradient({ 0.0f, h * 0.4f }, { 0.0f, h }, topHsl.toColor(), botHsl.toColor()) }, &tx);
    }

    // --- Effects ---

    void MillScene::drawRain(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        float opacity = m_theme.hasRain;
        if (opacity <= 0.001f) return;

        Matrix3x2 tx = Matrix3x2::translation(offset);
        const int rainCount = 100;
        const float tilt = 0.15f;
        const float s = std::sin(tilt), c = std::cos(tilt);
        for (int i = 0; i < rainCount; ++i) {
            float x = pseudoRand(i) * w;
            float loopCount = (float)(4 + (i % 3));
            float y = std::fmod(pseudoRand(i, 2.0f) * h + t * (loopCount * h), h);
            float len = 25.0f * m_objectsScale;
            PixelPath path;
            path.moveTo(x, y); path.lineTo(x - len * s, y + len * c);
            canvas.drawPath(
                path,
                SolidColor{ m_theme.effectColor1.withOpacity(opacity * 0.5f) },
                1.5f * m_objectsScale, &tx
            );
        }
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

    //    Matrix3x2 tx = Matrix3x2::translation(offset);
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
    //                PathDrawLayer::stroke(m_theme.sunDisk, 3.0f * m_objectsScale),
    //                PathDrawLayer::outerGlow(m_theme.sunDisk, 20.0f * m_objectsScale)
    //                }, tx, opacityBolt);

    //            canvas.drawPixelView(view);

    //            g_rasterBuffers.hasActiveClip = wasActive;
    //            g_rasterBuffers.activeClipMaskPtr = oldClipMaskPtr;
    //        });
    //}

    void MillScene::drawLightning(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        if (m_theme.hasRain <= 0.001f) return;
        float strikePhase = t * 3.0f;
        float flashT = std::fmod(strikePhase, 1.0f);
        if (flashT > 0.15f) return;

        float seed = std::floor(strikePhase);
        float flicker = (std::sin(flashT * 150.0f) > 0.0f) ? 1.0f : 0.0f;
        float fade = 1.0f - (flashT / 0.15f);

        Matrix3x2 tx = Matrix3x2::translation(offset);
        PixelPath path;

        // Soft sky flash
        float opacityFill = m_theme.hasRain * flicker * fade * 0.15f;
        path.moveTo(0.0f, 0.0f); path.lineTo(w, 0.0f); path.lineTo(w, h); path.lineTo(0.0f, h); path.close();
        canvas.fillPath(path, SolidColor{ m_theme.sunDisk.withOpacity(opacityFill) }, &tx);

        // Lightning bolt path generation
        float opacityBolt = m_theme.hasRain * flicker * fade;
        path.clear();
        float lx = w * (0.3f + 0.4f * pseudoRand(1, seed));
        float ly = 0.0f;
        path.moveTo(lx, ly);

        for (int i = 1; i <= 6; ++i) {
            lx += (pseudoRand(i, seed) * 120.0f - 60.0f) * m_objectsScale;
            ly += h * 0.1f + pseudoRand(i + 10, seed) * h * 0.1f;
            path.lineTo(lx, ly);

            if (pseudoRand(i + 20, seed) > 0.5f) {
                float bx = lx + (pseudoRand(i + 30, seed) * 100.0f - 50.0f) * m_objectsScale;
                float by = ly + h * 0.15f + pseudoRand(i + 40, seed) * h * 0.05f;
                path.lineTo(bx, by);
                path.moveTo(lx, ly);
            }
        }

        Color boltColor = m_theme.sunDisk.withOpacity(opacityBolt);
        canvas.drawPath(
            path,
            {
                PathDrawLayer::stroke(boltColor, 3.0f * m_objectsScale),
                PathDrawLayer::outerGlow(boltColor, 20.0f * m_objectsScale)
            },
            &tx //, opacityBolt
        );
    }
    void MillScene::drawStars(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        float opacity = m_theme.hasStars;
        if (opacity <= 0.001f) return;

        const int starCount = 120;
        for (int i = 0; i < starCount; ++i) {
            float xBase = pseudoRand(i, 0.0f) * w;
            float yBase = pseudoRand(i, 1.0f) * h * 0.65f;
            float flickerSpeed = std::floor(1.0f + pseudoRand(i, 2.0f) * 3.0f);
            float flicker = (std::sin(t * k_2Pi * flickerSpeed + static_cast<float>(i)) + 1.0f) * 0.5f;

            if (flicker > 0.2f) {
                float visibleFlicker = (flicker - 0.2f) / 0.8f;
                float s = (1.0f + pseudoRand(i, 3.0f) * 2.0f) * m_objectsScale * visibleFlicker;

                Matrix3x2 tx = Matrix3x2::translation(offset.x + xBase, offset.y + yBase);
                PixelPath path;
                path.moveTo(0.0f, -s); path.lineTo(s * 0.25f, -s * 0.25f);
                path.lineTo(s, 0.0f); path.lineTo(s * 0.25f, s * 0.25f);
                path.lineTo(0.0f, s); path.lineTo(-s * 0.25f, s * 0.25f);
                path.lineTo(-s, 0.0f); path.lineTo(-s * 0.25f, -s * 0.25f); path.close();
                canvas.fillPath(path, SolidColor{ m_theme.sunDisk.withOpacity(opacity) }, &tx);
            }
        }
    }

    void MillScene::drawSnowflake(Canvas& canvas, float x, float y, float s, float t, int id)
    {
        PixelPath path;
        createSnowFlakePath(path);
        Matrix3x2 transform = Matrix3x2::translation({ x, y })
            * Matrix3x2::scale(s)
            * Matrix3x2::rotation(calculateSyncRotation(t, id, 6.0f) * k_2Pi);
        canvas.drawPath(
            path,
            SolidColor{ m_theme.effectColor1.withOpacity(m_theme.hasSnow) },
            1.5f,
            &transform
        );
    }

    void MillScene::drawSnow(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        if (m_theme.hasSnow <= 0.001f) return;
        for (int i = 0; i < 60; ++i) {
            float xBase = pseudoRand(i) * w;
            float drift = std::sin(t * k_2Pi * 1.0f + i) * 30.0f * m_objectsScale;
            float loopCount = static_cast<float>(1 + (i % 2));
            float y = std::fmod(pseudoRand(i, 3.0f) * h + t * (loopCount * h), h);
            drawSnowflake(canvas, offset.x + xBase + drift, offset.y + y, (0.3f + pseudoRand(i, 5.0f) * 0.3f) * m_objectsScale, t, i);
        }
    }

    void MillScene::drawLeaf(Canvas& canvas, float x, float y, float s, float t, int id, Color color, float opacity)
    {
        Matrix3x2 tx = Matrix3x2::translation(x, y) * Matrix3x2::rotation(calculateSyncRotation(t, id, 2.0f) * k_2Pi) * Matrix3x2::scale(s - m_theme.hasPetals * 0.5f);
        PixelPath path;
        path.moveTo(0.0f, -10.0f); path.lineTo(8.0f, 0.0f); path.lineTo(0.0f, 10.0f); path.lineTo(-8.0f, 0.0f); path.close();
        canvas.fillPath(path, SolidColor{ color.withOpacity(opacity) }, &tx);
    }

    void MillScene::drawLeavesOrPetals(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        float opacity = StateFactors::compose(m_theme.hasLeaves, m_theme.hasPetals);
        if (opacity <= 0.001f) return;

        for (int i = 0; i < 30; ++i) {
            float xBase = pseudoRand(i) * w;
            float loopCount = (float)(2 + (i % 2));
            float y = std::fmod(pseudoRand(i, 4.0f) * h + t * (loopCount * h), h);
            float drift = std::sin(t * k_2Pi * 1.0f + i) * 60.0f * m_objectsScale;
            Color col = (i % 2 == 0 ? m_theme.effectColor1 : m_theme.effectColor2);
            drawLeaf(canvas, offset.x + xBase + drift, offset.y + y, m_objectsScale * 0.65f, t, i, col, opacity);
        }
    }

    void MillScene::drawFireflies(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        float opacity = m_theme.hasFireflies;
        if (opacity <= 0.001f) return;

        for (int i = 0; i < 20; ++i) {
            float xBase = pseudoRand(i) * w;
            float yBase = h * 0.4f + pseudoRand(i, 5.0f) * h * 0.4f;
            float dx = std::sin(t * k_2Pi * 1.0f + i) * 50.0f * m_objectsScale;
            float dy = std::cos(t * k_2Pi * 1.0f + i * 0.7f) * 30.0f * m_objectsScale;
            float flicker = (std::sin(t * k_2Pi * 2.0f + i) + 1.0f) * 0.5f;

            if (flicker > 0.4f) {
                FloatPoint pos = { offset.x + xBase + dx, offset.y + yBase + dy };
                float gS = 15.0f * m_objectsScale;

                canvas.fillRectangle({ pos.x - gS, pos.y - gS, pos.x + gS, pos.y + gS }, PointGlow{
                    .lightPos = pos,
                    .lightColor = m_theme.effectColor2,
                    .lightSpread = gS,
                    .xRatio = 1.0f,
                    .shape = GlowShape::Circle,
                    .opacity = 0.5f * opacity
                    });

                PixelPath path;
                Matrix3x2 tx = Matrix3x2::translation(pos);
                path.drawCircle({}, 3.5f * m_objectsScale);
                canvas.fillPath(path, SolidColor{ m_theme.effectColor1.withOpacity(opacity) }, &tx);
            }
        }
    }

    void MillScene::drawDust(Canvas& canvas, float w, float h, FloatPoint offset, float t)
    {
        float opacity = m_theme.hasDust;
        if (opacity <= 0.001f) return;

        Matrix3x2 tx = Matrix3x2::translation(offset);
        for (int i = 0; i < 50; ++i) {
            float x = std::fmod(pseudoRand(i) * w + t * w * 3.0f, w);
            float y = pseudoRand(i, 7.0f) * h;
            PixelPath path;
            path.moveTo(x, y); path.lineTo(x + 10.0f * m_objectsScale, y + 2.0f * m_objectsScale);
            canvas.drawPath(path, SolidColor{ m_theme.effectColor1.withOpacity(opacity) }, 1.0f, &tx);
        }
    }
}
