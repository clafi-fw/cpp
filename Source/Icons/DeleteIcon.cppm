export module ClaFi.Icons.DeleteIcon;

import ClaFi.Icons.GearIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Icons::DeleteIcon
{
    using namespace ::ClaFi::Graphics;

    export void draw(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::Translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        const Color iconColor = event.textRgb(InkGrade::Strong);

        path.moveTo(size * 0.22f, size * 0.30f);
        path.lineTo(size * 0.78f, size * 0.30f);
        path.lineTo(size * 0.71f, size * 0.92f);
        path.lineTo(size * 0.29f, size * 0.92f);
        path.close();

        path.moveTo(size * 0.42f, size * 0.43f);
        path.lineTo(size * 0.43f, size * 0.77f);
        path.moveTo(size * 0.58f, size * 0.43f);
        path.lineTo(size * 0.57f, size * 0.77f);
        canvas.drawPath(path, { PathDrawLayer::Stroke(iconColor, strokeWidth) }, &transform);

        path.clear();
        path.moveTo(size * 0.39f, size * 0.20f);
        path.lineTo(size * 0.39f, size * 0.10f);
        path.lineTo(size * 0.61f, size * 0.10f);
        path.lineTo(size * 0.61f, size * 0.20f);
        path.moveTo(size * 0.13f, size * 0.24f);
        path.lineTo(size * 0.87f, size * 0.24f);
        canvas.drawPath(path, { PathDrawLayer::Stroke(iconColor, strokeWidth) }, &transform);
    }

    export void drawDocumentDelete(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const Matrix3x2 transform = Matrix3x2::Translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        path.moveTo(size * 0.12f, size * 0.08f);
        path.lineTo(size * 0.61f, size * 0.08f);
        path.lineTo(size * 0.80f, size * 0.27f);
        path.lineTo(size * 0.80f, size * 0.81f);
        path.lineTo(size * 0.12f, size * 0.81f);
        path.close();
        canvas.fillPath(path, event.textRgb(InkGrade::Strong), &transform);

        path.clear();
        path.moveTo(size * 0.48f, size * 0.48f);
        path.lineTo(size * 0.92f, size * 0.92f);
        path.moveTo(size * 0.92f, size * 0.48f);
        path.lineTo(size * 0.48f, size * 0.92f);
        canvas.drawPath(path,
            { PathDrawLayer::Stroke(event.surfaceRgb(), strokeWidth * 2.0f) }, &transform);
        canvas.drawPath(path,
            { PathDrawLayer::Stroke(event.inkRgb(InkWell::Red), strokeWidth) }, &transform);
    }

    export void drawSettingsDelete(DrawIconEvent& event)
    {
        GearIcon::draw(event);

        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const Matrix3x2 transform = Matrix3x2::Translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        path.moveTo(size * 0.48f, size * 0.48f);
        path.lineTo(size * 0.92f, size * 0.92f);
        path.moveTo(size * 0.92f, size * 0.48f);
        path.lineTo(size * 0.48f, size * 0.92f);
        event.canvas().drawPath(path,
            { PathDrawLayer::Stroke(event.surfaceRgb(), strokeWidth * 2.0f) }, &transform);
        event.canvas().drawPath(path,
            { PathDrawLayer::Stroke(event.inkRgb(InkWell::Red), strokeWidth) }, &transform);
    }

}
