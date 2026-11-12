export module ClaFi.PathArt.RabbitSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A white rabbit coming up out of a hat, how far up and where it looks.
    export struct RabbitPose
    {
        FloatPoint rim; // the middle of the rim it comes up out of; nothing below it is drawn
        float turn{ 0.0f }; // the rim's slant
        float size{ 1.0f }; // pixels per unit of the rabbit, whose head is 30 units across
        float rise{ 0.0f }; // 0 hidden below the rim, 1 up with its paws on it
        float look{ 0.0f }; // how far its eyes and nose turn to the right, in units
        float time{ 0.0f }; // in seconds; its ears twitch and its nose sniffs by it
    };

    export void drawRabbit(const SceneFrame&, const RabbitPose&);
}
