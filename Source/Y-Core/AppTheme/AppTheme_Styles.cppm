export module ClaFi.Core.AppTheme_Styles;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.StdLib;

namespace ClaFi
{
    // The colour rules and the sizes one element wears, stated over its base's. See AppTheme
    export struct ControlStyle
    {
        std::wstring name{};        // what a theme file and an application know the style by
        std::wstring base{};        // the style its chain continues from, none when empty
        std::wstring standsOn{};    // what it is painted on, the mode's bare colour when empty
        OptionalPigment seed{};     // where its surface's hue starts, if not at its standsOn's
        ColorRules rules{};
        RuleSelectors dropped{};    // the inherited rules it leaves out
        StatedMetrics metrics{};    // the sizes it states, over the ones its chain gives it
        [[nodiscard]] bool operator==(const ControlStyle&) const = default;
    };

    export using ControlStyleList = std::vector<ControlStyle>;
    export using StyleChain = std::vector<const ControlStyle*>;

    // The named styles a theme states, each found by name. See AppTheme#themestyles
    export struct ThemeStyles
    {
    public:
        // The style a name stands for, and nullptr where the theme states none.
        [[nodiscard]] const ControlStyle* of(std::wstring_view name) const;
        [[nodiscard]] ControlStyle* of(std::wstring_view name);
        ControlStyle& add(ControlStyle); // appended, replacing a style of the same name
        void remove(std::wstring_view name);
        // A style's chain: the style first, then its base and so on up, ended where a base is
        // stated nowhere or has been walked already.
        [[nodiscard]] StyleChain chainOf(std::wstring_view name) const;
        // A style's rules as the paint path applies them. See AppTheme#themestyles-resolvedrules
        [[nodiscard]] ColorRules resolvedRules(std::wstring_view name) const;
        // A style's sizes as the layout reads them. See AppTheme#themestyles-resolvedmetrics
        [[nodiscard]] ControlMetrics resolvedMetrics(std::wstring_view name) const;
        [[nodiscard]] bool operator==(const ThemeStyles&) const = default;
    public:
        ControlStyleList list{};
    };

    export using ElementMetricsList = std::vector<ControlMetrics>;

    // Every style's sizes as the layout reads them, one entry per element. See AppTheme
    export struct StyleMetrics
    {
    public:
        // An element past the entries - of a style not yet resolved - holds the defaults.
        [[nodiscard]] const ControlMetrics& of(UiElement) const;
        [[nodiscard]] bool operator==(const StyleMetrics&) const = default;
    public:
        ElementMetricsList elements{};
    };


    //-------------------------------------------------------------------------


    // ThemeStyles

    const ControlStyle* ThemeStyles::of(const std::wstring_view name) const
    {
        const auto found = std::ranges::find(list, name, &ControlStyle::name);
        return found == list.end() ? nullptr : &*found;
    }

    ControlStyle* ThemeStyles::of(const std::wstring_view name)
    {
        const auto found = std::ranges::find(list, name, &ControlStyle::name);
        return found == list.end() ? nullptr : &*found;
    }

    ControlStyle& ThemeStyles::add(ControlStyle style)
    {
        remove(style.name);
        return list.emplace_back(std::move(style));
    }

    void ThemeStyles::remove(const std::wstring_view name)
    {
        std::erase_if(list, [name](const ControlStyle& style) {
            return style.name == name;
        });
    }

    StyleChain ThemeStyles::chainOf(const std::wstring_view name) const
    {
        StyleChain result{};
        const ControlStyle* style = of(name);
        while (style)
        {
            if (std::ranges::find(result, style) != result.end())
                break;
            result.push_back(style);
            style = style->base.empty() ? nullptr : of(style->base);
        }
        return result;
    }

    // The chain's resting rules root first, then its state rules root first - a surface is
    // established before a state modifies it. A rule a style further down the chain has dropped
    // is left out.
    ColorRules ThemeStyles::resolvedRules(const std::wstring_view name) const
    {
        const StyleChain chain = chainOf(name);
        auto isDropped = [&chain](const ColorRule& rule, const std::size_t level) {
            for (std::size_t below = 0ull; below != level; ++below)
            {
                for (const RuleSelector& selector : chain[below]->dropped)
                    if (selector.matches(rule))
                        return true;
            }
            return false;
        };
        ColorRules result{};
        for (const bool atRest : { true, false })
        {
            for (std::size_t step = 0ull; step != chain.size(); ++step)
            {
                const std::size_t level = chain.size() - 1ull - step;
                for (const ColorRule& rule : chain[level]->rules)
                {
                    if (rule.atRest() == atRest and !isDropped(rule, level))
                        result.push_back(rule);
                }
            }
        }
        return result;
    }

    // Root first, so a size a style states stands over the one its base states.
    ControlMetrics ThemeStyles::resolvedMetrics(const std::wstring_view name) const
    {
        const StyleChain chain = chainOf(name);
        ControlMetrics result{};
        for (std::size_t step = 0ull; step != chain.size(); ++step)
            chain[chain.size() - 1ull - step]->metrics.applyTo(result);
        return result;
    }

    // StyleMetrics

    const ControlMetrics& StyleMetrics::of(const UiElement element) const
    {
        static const ControlMetrics s_defaults{};
        const std::size_t index = static_cast<std::size_t>(element);
        return index < elements.size() ? elements[index] : s_defaults;
    }
}
