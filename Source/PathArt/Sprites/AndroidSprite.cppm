export module ClaFi.PathArt.AndroidSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A side of a figure or of the stage, as the audience sees it.
    export enum class AudienceSide
    {
        Left,
        Right
    };

    // Which way an elbow goes where an arm could bend either way.
    export enum class ElbowBend
    {
        Out,    // away from the body
        Down
    };

    // What a hand is at the end of its forearm.
    export enum class HandShape
    {
        Open,   // a round palm
        Fist    // closed round what it holds
    };

    // Which of the three chest lights are lit.
    export enum class ChestLights
    {
        Chase,  // one at a time, chasing the beat by the cycle time
        All,
        None,
        Pink    // the pink one alone, on the right
    };

    // How the light on the tip of the antenna shows.
    export enum class AntennaLight
    {
        Blinking,   // on and off by the cycle time
        On,
        Off
    };

    // Places on the android, in units from the middle of its pelvis as it stands. See PathArt
    export namespace AndroidBody
    {
        constexpr float StandingHeight{ 117.0f }; // the middle of the pelvis above the floor
        constexpr float AnkleHeight{ 14.0f }; // an ankle above the sole of its foot
        constexpr float Waist{ -11.0f }; // the height the upper body bends at
        constexpr FloatPoint RightHip{ 17.0f, 9.0f };
        constexpr FloatPoint RightShoulder{ 47.0f, -101.0f };
        constexpr FloatPoint Neck{ 0.0f, -129.0f }; // the head turns about it
        constexpr FloatPoint HeadMiddle{ 0.0f, -158.0f };
        constexpr FloatPoint AntennaTip{ 0.0f, -217.0f };
    }

    // How one side of the android is placed, left or right as the audience sees it.
    export struct AndroidSide
    {
        FloatPoint foot{}; // its ankle, or its knee while it kneels, in units about the pelvis
        float footTurn{ 0.0f };
        FloatPoint hand{}; // in units about the pelvis, in the frame the upper body bends
        float upperArm{ 57.0f };
        float forearm{ 51.0f };
        ElbowBend elbow{ ElbowBend::Out };
        HandShape handShape{ HandShape::Open };
        bool forearmOnTop{ false }; // left to drawAndroidForearm, over what the hand holds
    };

    // How the android stands, moves and shows at one moment. See PathArt
    export struct AndroidPose
    {
        FloatPoint pelvis{}; // the middle of its pelvis, in the frame
        float tilt{ 0.0f }; // the whole figure's turn about the pelvis, in radians
        float facing{ 1.0f }; // 1 facing the audience, -1 its back to them, between as it turns
        float bend{ 0.0f }; // the upper body's lean at the waist, in radians
        bool kneeling{ false };
        AndroidSide left{};
        AndroidSide right{};
        float headTurn{ 0.0f };
        float look{ 0.0f }; // how far its eyes look to the right, in units
        float eyeOpening{ 1.0f }; // 1 wide open; a blink closes them further
        ChestLights lights{ ChestLights::Chase };
        AntennaLight antenna{ AntennaLight::Blinking };
        bool wearsHat{ false }; // a top hat at a slant, the antenna through its crown
        bool wearsTutu{ false }; // a classical tutu round the hips
        float tutuTurn{ 0.0f }; // how far the tutu's pleats have turned as it spins, in radians
        bool wearsPointeShoes{ false }; // pink satin, their ribbons crossed over the shins
        float cycleTime{ 0.0f }; // the time its eyes blink and its lights chase by

        [[nodiscard]] AndroidSide& side(AudienceSide);
        [[nodiscard]] const AndroidSide& side(AudienceSide) const;
    };

    // The android on its feet at `floorPoint`, `stance` units either side, its knees bent by `dip`.
    export [[nodiscard]] AndroidPose standingAndroid(const SceneFrame&, FloatPoint floorPoint,
        float cycleTime, float dip = 0.0f, float stance = 19.0f);
    // A point in units about the pelvis, placed in the frame.
    export [[nodiscard]] FloatPoint androidBodyToFrame(const SceneFrame&, const AndroidPose&,
        FloatPoint bodyPoint);
    export [[nodiscard]] FloatPoint androidFrameToBody(const SceneFrame&, const AndroidPose&,
        FloatPoint framePoint);
    // A point of the upper body, which leans by the bend, placed in the frame.
    export [[nodiscard]] FloatPoint androidUpperToFrame(const SceneFrame&, const AndroidPose&,
        FloatPoint upperPoint);
    export [[nodiscard]] FloatPoint androidFrameToUpper(const SceneFrame&, const AndroidPose&,
        FloatPoint framePoint);
    // The frame of the upper body in units, for what the hands hold.
    export [[nodiscard]] SceneFrame androidUpperFrame(const SceneFrame&, const AndroidPose&);
    // Sets the feet to stand with their ankles on two points of the frame.
    export void plantAndroidFeet(const SceneFrame&, AndroidPose&, FloatPoint leftAnkle,
        FloatPoint rightAnkle);
    // Moves the pelvis up or down until the lowest sole, hand or antenna tip touches the floor.
    export void restAndroidOnFloor(const SceneFrame&, AndroidPose&, float floorY);

    // The android and every forearm not left on top for drawAndroidForearm.
    export void drawAndroid(const SceneFrame&, const AndroidPose&);
    // A forearm and hand left on top, drawn over what the hand holds.
    export void drawAndroidForearm(const SceneFrame&, const AndroidPose&, AudienceSide);
}
