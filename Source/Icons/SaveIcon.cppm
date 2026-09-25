export module ClaFi.Icons.SaveIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::SaveIcon
{
    using namespace ::ClaFi::Graphics;

    namespace {
        struct FloppyMetrics {
            float fSize, r, n, sW, sH, sX, lW, lH, lX, lY;
            FloppyMetrics(float size) {
                fSize = size;
                r = fSize * 0.15f;      // Corner radius
                n = fSize * 0.25f;      // Top-right notch
                sW = fSize * 0.40f;     // Shutter width
                sH = fSize * 0.28f;     // Shutter height
                sX = (fSize - sW) * 0.5f;
                lW = fSize * 0.60f;     // Label width
                lH = fSize * 0.35f;     // Label height
                lX = (fSize - lW) * 0.5f;
                lY = fSize - lH;
            }
        };

        // Draws the closed parts (Outer shell and Shutter)
        void addFloppyBody(PixelPath& path, const FloppyMetrics& m) {
            // Outer Shell
            path.moveTo(0.0f, m.r);
            path.quadTo({ 0.0f, 0.0f }, { m.r, 0.0f });
            path.lineTo(m.fSize - m.n, 0.0f);
            path.lineTo(m.fSize, m.n);
            path.lineTo(m.fSize, m.fSize - m.r);
            path.quadTo({ m.fSize, m.fSize }, { m.fSize - m.r, m.fSize });
            path.lineTo(m.r, m.fSize);
            path.quadTo({ 0.0f, m.fSize }, { 0.0f, m.fSize - m.r });
            path.lineTo(0.0f, m.r);
            path.close();

            // Shutter
            path.moveTo(m.sX, 0.0f);
            path.lineTo(m.sX + m.sW, 0.0f);
            path.lineTo(m.sX + m.sW, m.sH - (m.r * 0.4f));
            path.quadTo({ m.sX + m.sW, m.sH }, { m.sX + m.sW - (m.r * 0.4f), m.sH });
            path.lineTo(m.sX + (m.r * 0.4f), m.sH);
            path.quadTo({ m.sX, m.sH }, { m.sX, m.sH - (m.r * 0.4f) });
            path.close();
        }

        // Draws the open part (Label)
        void addFloppyLabel(PixelPath& path, const FloppyMetrics& m) {
            path.moveTo(m.lX, m.fSize);
            path.lineTo(m.lX, m.lY + m.r);
            path.quadTo({ m.lX, m.lY }, { m.lX + m.r, m.lY });
            path.lineTo(m.lX + m.lW - m.r, m.lY);
            path.quadTo({ m.lX + m.lW, m.lY }, { m.lX + m.lW, m.lY + m.r });
            path.lineTo(m.lX + m.lW, m.fSize);
        }
    }

    export void paintSaveIcon(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        FloatRect iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        FloppyMetrics m(size);
        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        Color strokeColor = event.accentRgb(InkGrade::Strongest);

        addFloppyBody(path, m);
        canvas.drawPath(path, { PathDrawLayer::stroke(strokeColor, strokeWidth) }, &transform);

        path.clear();
        addFloppyLabel(path, m);
        canvas.drawPath(
            path,
            {
                PathDrawLayer::stroke(strokeColor, strokeWidth)
            },
            &transform
        );
    }

    export void paintSaveAllIcon(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        FloatRect iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = std::max(1.0f, size * 0.08f);
        size -= strokeWidth;

        FloppyMetrics m(size * 0.85f); // Slightly larger front icon
        float offset = size * 0.15f;
        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        // 1. Draw BACK FLOPPY (The Shadow)
        // The radius (shadowR) is widened so the curve stays parallel to the front floppy
        float shadowR = m.r * 2.0f;

        // Start on the right side
        path.moveTo(m.fSize + offset, m.r + offset * 1.5f);
        path.lineTo(m.fSize + offset, m.fSize - shadowR + offset);

        // Bottom-right corner with a larger radius
        path.quadTo(
            { m.fSize + offset, m.fSize + offset },          // Control (the sharp corner)
            { m.fSize - shadowR + offset, m.fSize + offset } // End of curve
        );
        // Bottom edge
        path.lineTo(m.r + offset, m.fSize + offset);

        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Muted), strokeWidth) }, &transform);

        // 2. Draw FRONT FLOPPY (Body)
        path.clear();
        addFloppyBody(path, m);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strongest), strokeWidth) }, &transform);

        // 3. Draw FRONT FLOPPY (Label)
        path.clear();
        addFloppyLabel(path, m);
        canvas.drawPath(
            path,
            {
                PathDrawLayer::fill(event.textRgb(InkGrade::Subtle)),
                PathDrawLayer::stroke(event.textRgb(InkGrade::Strongest), strokeWidth)
            },
            &transform
        );
    }

}
