export module ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    export class PaletteColor
    {
    public:
        explicit PaletteColor(Hsl hsl);
        //
        Hsl hsl() const { return m_hsl; }
        Color rgb() const { return m_rgb; }
        float hue() const { return m_hsl.hue; }
        int hueDegree() const { return m_hsl.hueDegree(); }
    private:
        void updateRgb();
        Hsl m_hsl;
        Color m_rgb;
    };

    export using PaletteColors = std::vector<PaletteColor>;

    export using PaletteMap = std::array<std::size_t, 3ull>;

    // How the palette's further hues are placed against the first.
    export enum class ColorHarmonyKind : std::size_t {
        First = 0,
        Monochromatic = 0,
        TwoAnalogous,
        Complementary,
        ThreeAnalogous,
        Triadic,
        SplitComplementary,
        Count
    };

    export ColorHarmonyKind& operator++(ColorHarmonyKind& value)
    {
        std::size_t& i = reinterpret_cast<std::size_t&>(value);
        return reinterpret_cast<ColorHarmonyKind&>(++i);
    }
    export constexpr ColorHarmonyKind k_defaultHarmonyKind = ColorHarmonyKind::Monochromatic;

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_colorHarmonyTitles{
        L"Monochromatic",
        L"Two analogous colors",
        L"Complementary",
        L"Three analogous colors",
        L"Triadic",
        L"Split complementary"
    };

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_harmonyKeys{
        L"Monochromatic",
        L"TwoAnalogous",
        L"Complementary",
        L"ThreeAnalogous",
        L"Triadic",
        L"SplitComplementary"
    };

    constexpr std::array<std::size_t, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_colorHarmonyColorsCount{
        1ull,
        2ull,
        2ull,
        3ull,
        3ull,
        3ull
    };

    // What a harmony fills and what an ink is drawn from are one question, so Pigment answers
    // both: the four hues it names are the four a harmony settles, in the order it settles them.

    using PigmentHues = std::array<float, k_pigmentsCount>;
    using ClaimedPigments = std::array<bool, k_pigmentsCount>;

    // Where yellow, green, blue and red stand: for each name, the hue that still reads as that name
    // at the luminosities these pigments are painted at. A pigment is named after a colour, so its
    // reference is the middle of that colour, and the hue channel places the four by how they look
    // rather than by even thirds of a wheel.
    //
    // The middle of the colour, not the sRGB corner of it. A corner is the most saturated exemplar
    // a display holds, which is the edge of what the name covers rather than its centre: sRGB's
    // blue carries hue 264, and 264 at the palette display pair paints #7A9FEC, a periwinkle. A
    // corner named here would put every Information and Question icon on the violet side of blue,
    // and would leave the blue a theme actually holds too far out to reach the pigment.
    //
    // THE FOUR ARE NOT EVENLY SPREAD, and no rule here may assume they are. Yellow and green
    // stand 58 degrees apart while blue and red stand 140.
    constexpr std::array<int, k_pigmentsCount> k_pigmentReferenceDegrees{
        90,
        148,
        245,
        25
    };

    namespace
    {
        constexpr PigmentHues makePigmentReferences()
        {
            PigmentHues references{};
            for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
                references[pigment] = hueFromDegree(k_pigmentReferenceDegrees[pigment]);
            return references;
        }
    }

    constexpr PigmentHues k_pigmentReferences = makePigmentReferences();

    // How far from its reference a hue may stand and still be called by that pigment's name. This
    // is what a claim means: a hue standing outside every reach names no pigment, and that pigment
    // is answered by a fill rule rather than by a colour nobody would call blue.
    constexpr int k_pigmentReachDegrees = 30;

    namespace
    {
        // A pigment's claim window, in whole degrees to either side of its reference. The two sides
        // are stated apart because the references are not evenly spread: a pigment with a near
        // neighbour on one side and a far one on the other reaches further into the side that
        // stands empty.
        struct PigmentWindow
        {
            int below;
            int above;
        };
    }

    using PigmentWindows = std::array<PigmentWindow, k_pigmentsCount>;

    // Each side is the reach, cut back to half the gap to the nearest reference on that side.
    // Half a gap is what keeps every hue inside a window nearer this pigment's reference than any
    // other, and the whole-degree division leaves the seam over an odd gap to neither window
    // rather than to both. A gap cuts only the two pigments it separates, so Blue keeps its whole
    // reach across the 97 degrees of empty wheel above green while Yellow holds 29 on the side
    // green stands.
    constexpr PigmentWindows makePigmentWindows()
    {
        PigmentWindows windows{};
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            PigmentWindow window = { k_pigmentReachDegrees, k_pigmentReachDegrees };
            for (std::size_t other = 0ull; other != k_pigmentsCount; ++other)
            {
                if (other == pigment)
                    continue;
                int gapAbove = (k_pigmentReferenceDegrees[other] - k_pigmentReferenceDegrees[pigment] + 360) % 360;
                int gapBelow = 360 - gapAbove;
                window.above = (std::min)(window.above, gapAbove / 2);
                window.below = (std::min)(window.below, gapBelow / 2);
            }
            windows[pigment] = window;
        }
        return windows;
    }

    constexpr PigmentWindows k_pigmentWindows = makePigmentWindows();

    // Two distances within a degree of each other count as the same distance, and a hue a degree
    // outside a window stands inside it. A colour between two references - an orange between
    // yellow and red - is at the same remove from both, and a harmony steps by whole degrees, so
    // it drops colours on a window edge exactly; without the margin the float error those offsets
    // carry would settle which pigment took a colour, or whether any pigment took it at all.
    constexpr float k_pigmentHueTie = 1.0f / 360.0f;

    // The most the four may be turned as one: the smallest window side, so the turn leaves every
    // pigment inside its own window whichever way it goes.
    constexpr float makePigmentTurnLimit()
    {
        int limit = k_pigmentReachDegrees;
        for (const PigmentWindow& window : k_pigmentWindows)
        {
            limit = (std::min)(limit, window.below);
            limit = (std::min)(limit, window.above);
        }
        return hueFromDegree(limit);
    }

    constexpr float k_pigmentTurnLimit = makePigmentTurnLimit();

    // What fills a pigment no harmony colour reaches. All three keep the four hues distinct and
    // each one inside its window, so a pigment always carries the colour its name states; they
    // differ in what a theme's icons do when the harmony stands nowhere near a pigment. Set
    // k_pigmentFill and rebuild to compare them.
    enum class PigmentFill
    {
        // The pigment's own reference. Truest to the names, and the same hue in every theme, so in
        // that pigment only the anchor's saturation and luminosity tell two themes' icons apart.
        Reference,
        // The point in the pigment's window standing farthest from every hue already settled. Keeps
        // the four spread and moving with the theme, at the cost of sitting near a window edge.
        WidestGap,
        // Nothing is claimed: the four references turn as one, by the offset that lays the set
        // over as many of the harmony's colours as it can. Widest spacing of the three, and it
        // lands on a harmony colour least often.
        TurnedSet
    };

    constexpr PigmentFill k_pigmentFill = PigmentFill::WidestGap;

    // A hue wrapped into the half open turn a hue is held in.
    [[nodiscard]] float wrappedHue(float value);

    // How far apart two hues stand, going the shorter way round. Half a turn is the most any two
    // can be.
    [[nodiscard]] float hueDistance(float hue, float other);

    // Where a hue stands relative to another, going the shorter way round: negative below it,
    // positive above.
    [[nodiscard]] float hueOffset(float hue, float reference);

    // Whether a hue stands inside a pigment's window, the side it falls on choosing the reach.
    [[nodiscard]] bool insidePigmentWindow(float hue, std::size_t pigment);

    // Hands each pigment the harmony colour nearest its reference, of those standing inside its
    // window, and marks the pigment claimed. The nearest pairing is settled first, so a colour two
    // pigments could take goes to the one it fits better and the other is left for another rule to
    // fill. A colour serves one pigment only, two pigments holding it naming the same hue twice.
    void claimPigmentHues(PigmentHues&, ClaimedPigments&, const PaletteColors&);

    // Moves each unclaimed pigment to the point in its window standing farthest from where every
    // other pigment stands: a claimed pigment at the colour it took, one this has not answered yet
    // at its own reference, which is where it stands unless something crowds it. Each window is
    // walked outward from its reference, so a pigment with room on both sides keeps its reference
    // and the four stay as near their names as the harmony leaves them.
    void spreadPigmentHues(PigmentHues&, const ClaimedPigments&);

    // Turns the whole set by the one offset, no wider than the tightest window, that lays it over
    // as many of the harmony's colours as it can. The turn is looked for among the offsets that
    // would bring some reference onto some colour: between two of those nothing meets, so moving
    // further only takes the set away from whatever it was nearest.
    void turnPigmentHues(PigmentHues&, const PaletteColors&);

    // A palette entry shown as a colour. An entry is a hue and nothing else, so anything that
    // draws one has to be given the other two from somewhere, and this is that somewhere. The pair
    // is the same in every theme, so a swatch shows the hue itself rather than how that hue
    // happens to land in the theme being looked at.
    //
    // Nothing resolved is drawn from these: ThemeColors::pigmentHsl takes the hue of a palette
    // colour and states the other two itself.
    export constexpr float k_paletteDisplaySaturation = 0.75f;
    export constexpr float k_paletteDisplayLuminosity = 0.66f;

    export [[nodiscard]] Hsl paletteDisplayColor(float hue);

    export class ColorHarmony
    {
    public:
        ColorHarmony(float& anchorHue, ColorHarmonyKind);
        virtual ~ColorHarmony() = default;
        static constexpr std::size_t noHue = static_cast<std::size_t>(-1);
    public:
        ColorHarmonyKind kind() const { return m_kind; }
        const std::wstring_view& name() const { return m_name; }
        void anchorChanged();
        std::vector<PaletteMap>& maps() { return m_maps; }
        void addMap(const PaletteMap&);
        void selectMap(PaletteMap&);
        PaletteMap* findMapByHueDegrees(std::array<int, 3ull> hues);
        PaletteMap* selectedMap() const { return m_selectedMap; }
        const PaletteColor& color(std::size_t index) const;
        std::size_t colorsNum() const { return m_colors.size(); }
        bool useColorOnBg() const { return (*m_selectedMap)[0] != noHue; }
        // The four pigments, derived once per anchor and read from there. Where one of the
        // harmony's own colours falls in a pigment's window that colour fills the pigment, a
        // pigment being free to share a hue with the palette; a pigment no harmony colour reaches
        // is answered by k_pigmentFill. No two pigments hold one colour, which would name the same
        // hue twice.
        [[nodiscard]] const PaletteColors& pigmentColors() const { return m_pigmentColors; }
        [[nodiscard]] const PaletteColor& pigmentColor(Pigment pigment) const { return m_pigmentColors[static_cast<std::size_t>(pigment)]; }
    protected:
        PaletteColors& colors() { return m_colors; }
        // Derives the pigments from whatever colours stand in m_colors, so an override of
        // anchorChanged that builds its colours differently ends by calling this.
        void fillPigmentColors();
    protected:
        PaletteColor m_gray{ { 0.0f, 0.0f, 0.5f } };
    protected:
        // The hue every colour here is turned from. Held by reference so that a drag moving it
        // reaches this without anything having to be handed back in.
        float& m_anchor;
        ColorHarmonyKind m_kind;
        const std::wstring_view& m_name;
        PaletteColors m_colors{};
        PaletteColors m_pigmentColors{};
        PaletteMap* m_selectedMap;
        std::vector<PaletteMap> m_maps;
    };

    export class ColorHarmonySelector
    {
    public:
        ColorHarmonySelector(float& anchorHue);
        void anchorChanged();
    public:
        std::size_t count();
        ColorHarmony& harmony(std::size_t);
    private:
        float& m_anchorHue;
        std::array<std::unique_ptr<ColorHarmony>, static_cast<std::size_t>(ColorHarmonyKind::Count)> m_harmonies{};
    };


    //-------------------------------------------------------------------------


    // PaletteColor

    PaletteColor::PaletteColor(Hsl hsl)
        :
        m_hsl{ hsl }
    {
        updateRgb();
    }

    void PaletteColor::updateRgb()
    {
        m_rgb = m_hsl.toColor();
    }

    float wrappedHue(float value)
    {
        float hue = value - std::floor(value);

        // A value a hair below zero floors a whole turn back, and the sum rounds up to a whole
        // turn from there. A whole turn is the hue the wheel starts at, and the range is half open.
        if (hue >= 1.0f)
            hue = 0.0f;
        return hue;
    }

    float hueDistance(float hue, float other)
    {
        float distance = std::fabs(hue - other);
        return distance > 0.5f ? 1.0f - distance : distance;
    }

    float hueOffset(float hue, float reference)
    {
        float offset = wrappedHue(hue - reference);
        return offset > 0.5f ? offset - 1.0f : offset;
    }

    bool insidePigmentWindow(float hue, std::size_t pigment)
    {
        const PigmentWindow& window = k_pigmentWindows[pigment];
        float offset = hueOffset(hue, k_pigmentReferences[pigment]);
        int reach = offset < 0.0f ? window.below : window.above;
        return std::fabs(offset) <= hueFromDegree(reach) + k_pigmentHueTie;
    }

    void claimPigmentHues(PigmentHues& hues, ClaimedPigments& claimed, const PaletteColors& colors)
    {
        // Which colour each pigment took, kept beside the claimed flags because a colour is refused
        // to a second pigment by identity and not by the hue it carries.
        constexpr std::size_t unclaimed = static_cast<std::size_t>(-1);
        std::array<std::size_t, k_pigmentsCount> claimedColor{};
        claimedColor.fill(unclaimed);

        for (std::size_t pairing = 0ull; pairing != k_pigmentsCount; ++pairing)
        {
            std::size_t bestPigment = unclaimed;
            std::size_t bestColor = unclaimed;
            float bestDistance = k_maxFloat;
            for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
            {
                if (claimedColor[pigment] != unclaimed)
                    continue;
                for (std::size_t color = 0ull; color != colors.size(); ++color)
                {
                    if (std::ranges::find(claimedColor, color) != claimedColor.end())
                        continue;
                    if (!insidePigmentWindow(colors[color].hue(), pigment))
                        continue;
                    float distance = hueDistance(colors[color].hue(), k_pigmentReferences[pigment]);
                    // Where two pairings fit equally the earlier pigment keeps the colour, the
                    // pigments being walked in order.
                    if (distance + k_pigmentHueTie >= bestDistance)
                        continue;
                    bestDistance = distance;
                    bestPigment = pigment;
                    bestColor = color;
                }
            }
            if (bestPigment == unclaimed)
                break;
            hues[bestPigment] = colors[bestColor].hue();
            claimedColor[bestPigment] = bestColor;
            claimed[bestPigment] = true;
        }
    }

    void spreadPigmentHues(PigmentHues& hues, const ClaimedPigments& claimed)
    {
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            if (claimed[pigment])
                continue;

            const PigmentWindow& window = k_pigmentWindows[pigment];
            float bestHue = k_pigmentReferences[pigment];
            float bestClearance = -1.0f;
            for (int offset = 0; offset <= (std::max)(window.below, window.above); ++offset)
            {
                for (int direction = -1; direction <= 1; direction += 2)
                {
                    int step = offset * direction;
                    if (step < -window.below || step > window.above)
                        continue;
                    float hue = wrappedHue(k_pigmentReferences[pigment] + hueFromDegree(step));

                    // How much room the candidate has: its distance from the nearest hue another
                    // pigment stands at. A pigment this has not answered yet stands at its
                    // reference, which is where it ends up unless something crowds it, so counting
                    // it holds a pigment with room on both sides at its own reference instead of
                    // pushing it against the window edge furthest from whatever was settled first.
                    float clearance = k_maxFloat;
                    for (std::size_t other = 0ull; other != k_pigmentsCount; ++other)
                    {
                        if (other == pigment)
                            continue;
                        clearance = (std::min)(clearance, hueDistance(hue, hues[other]));
                    }

                    if (clearance <= bestClearance)
                        continue;
                    bestClearance = clearance;
                    bestHue = hue;
                }
            }
            hues[pigment] = bestHue;
        }
    }

    void turnPigmentHues(PigmentHues& hues, const PaletteColors& colors)
    {
        std::vector<float> turns{ 0.0f };
        turns.reserve(colors.size() * k_pigmentsCount + 1ull);
        for (const PaletteColor& color : colors)
            for (float reference : k_pigmentReferences)
            {
                // The shorter way round, so a turn reads as the small move it is rather than as
                // most of a lap the other way.
                float turn = color.hue() - reference;
                turn -= std::round(turn);
                if (std::fabs(turn) <= k_pigmentTurnLimit)
                    turns.push_back(turn);
            }

        // Nearest to no turn at all first, so where two turns fit the harmony equally the set is
        // left as near the references as it can be.
        std::sort(turns.begin(), turns.end(), [](float left, float right){
            return std::fabs(left) < std::fabs(right);
        });

        float bestTurn = 0.0f;
        float bestCost = k_maxFloat;
        for (float turn : turns)
        {
            // How far the harmony's colours stand from the turned set. The lower this is, the more
            // of them a pigment lands on.
            float cost = 0.0f;
            for (const PaletteColor& color : colors)
            {
                float nearest = k_maxFloat;
                for (float reference : k_pigmentReferences)
                    nearest = (std::min)(nearest, hueDistance(color.hue(), wrappedHue(reference + turn)));
                cost += nearest;
            }
            if (cost + k_pigmentHueTie >= bestCost)
                continue;
            bestCost = cost;
            bestTurn = turn;
        }

        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
            hues[pigment] = wrappedHue(k_pigmentReferences[pigment] + bestTurn);
    }

    Hsl paletteDisplayColor(float hue)
    {
        return { hue, k_paletteDisplaySaturation, k_paletteDisplayLuminosity };
    }

    // ColorHarmony

    ColorHarmony::ColorHarmony(float& anchorHue, ColorHarmonyKind kind)
        :
        m_anchor{ anchorHue },
        m_kind{ kind },
        m_name{ k_colorHarmonyTitles[static_cast<std::size_t>(kind)] }
    {
        m_colors.reserve(k_colorHarmonyColorsCount[static_cast<std::size_t>(kind)]);
        switch (m_kind)
        {
        case ColorHarmonyKind::Monochromatic:
            addMap({ 0ull, 0ull, 0ull });
            break;
        case ColorHarmonyKind::TwoAnalogous:
            addMap({ 0ull, 1ull, 1ull });
            addMap({ 0ull, 0ull, 1ull });
            addMap({ 1ull, 0ull, 0ull });
            addMap({ 1ull, 1ull, 0ull });
            addMap({ 1ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Complementary:
            addMap({ 0ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 1ull });
            addMap({ 1ull, 0ull, 0ull });
            addMap({ 1ull, 1ull, 0ull });
            addMap({ 1ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::ThreeAnalogous:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Triadic:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::SplitComplementary:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Count:
            break;
        }
        anchorChanged();
    }

    void ColorHarmony::anchorChanged()
    {
        m_colors.clear();
        m_colors.emplace_back(paletteDisplayColor(m_anchor));
        switch (m_kind)
        {
        case ColorHarmonyKind::Monochromatic:
            break;
        case ColorHarmonyKind::TwoAnalogous:
        {
            Hsl secondColor = paletteDisplayColor(m_anchor);
            secondColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(secondColor);
            break;
        }
        case ColorHarmonyKind::Complementary:
        {
            Hsl secondColor = paletteDisplayColor(m_anchor);
            secondColor.offsetHue(0.5f);
            m_colors.emplace_back(secondColor);
            break;
        }
        case ColorHarmonyKind::ThreeAnalogous:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::Triadic:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(1.0f / 3.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(1.0f / 3.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::SplitComplementary:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(5.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(2.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::Count:
            break;
        }
        fillPigmentColors();
    }

    void ColorHarmony::addMap(const PaletteMap& value)
    {
        m_maps.push_back(value);
        m_selectedMap = &m_maps.front();
    }

    void ColorHarmony::selectMap(PaletteMap& value)
    {
        m_selectedMap = &value;
    }

    PaletteMap* ColorHarmony::findMapByHueDegrees(std::array<int, 3ull> hueDegrees)
    {
        for (PaletteMap& map : m_maps)
            if (map.size() == hueDegrees.size())
                if (color(map[0ull]).hueDegree() == hueDegrees[0ull]
                    && color(map[1ull]).hueDegree() == hueDegrees[1ull]
                    && color(map[2ull]).hueDegree() == hueDegrees[2ull])
                    return &map;
        return nullptr;
    }

    const PaletteColor& ColorHarmony::color(std::size_t index) const
    {
        switch (index)
        {
        case noHue: return m_gray;
        default: return m_colors[index];
        }
    }

    void ColorHarmony::fillPigmentColors()
    {
        // The set starts on the references, and what follows writes over the pigments it settles.
        PigmentHues hues = k_pigmentReferences;
        ClaimedPigments claimed{};

        if constexpr (k_pigmentFill == PigmentFill::TurnedSet)
        {
            turnPigmentHues(hues, m_colors);
        }
        else
        {
            claimPigmentHues(hues, claimed, m_colors);
            if constexpr (k_pigmentFill == PigmentFill::WidestGap)
            {
                spreadPigmentHues(hues, claimed);
            }
        }

        // Only the hue is the harmony's, and only the hue leaves here for anything painted: the
        // resolver states the other two itself. These carry the palette's display pair so that a
        // pigment shown in the editor and a palette swatch beside it read as one set.
        m_pigmentColors.clear();
        for (float hue : hues)
            m_pigmentColors.emplace_back(paletteDisplayColor(hue));
    }

    // ColorHarmonySelector

    ColorHarmonySelector::ColorHarmonySelector(float& anchorHue)
        :
        m_anchorHue{ anchorHue }
    {
        for (ColorHarmonyKind kind = ColorHarmonyKind::First; kind != ColorHarmonyKind::Count; ++reinterpret_cast<std::size_t&>(kind))
            m_harmonies[static_cast<std::size_t>(kind)] = std::make_unique<ColorHarmony>(m_anchorHue, kind);
    }

    void ColorHarmonySelector::anchorChanged()
    {
        for (ColorHarmonyKind kind = ColorHarmonyKind::First; kind != ColorHarmonyKind::Count; ++reinterpret_cast<std::size_t&>(kind))
            m_harmonies[static_cast<std::size_t>(kind)]->anchorChanged();
    }

    std::size_t ColorHarmonySelector::count()
    {
        return m_harmonies.size();
    }

    ColorHarmony& ColorHarmonySelector::harmony(std::size_t index)
    {
        return *m_harmonies[index];
    }
}
