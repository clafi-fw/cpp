module ClaFiApps.ArticleTheme;

import ClaFiApps.StarterTheme;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFiApps
{
    using namespace ClaFi;

    const AppTheme& articleTheme()
    {
        static const AppTheme theme = []() {
            AppTheme result = starterTheme();
            result.styles.list.insert(result.styles.list.end(), {
                ControlStyle{
                    .name = L"DoDont",
                    .base = L"AnyNonWindow",
                    .standsOn = L"Section",
                    .rules = {
                        ColorRule{
                            .effect{
                                { ColorRuleOp::Set, 0.4f },             // S
                                { ColorRuleOp::Offset, 0.045f }         // E
                            }
                        },
                        ColorRule{
                            .output = PaintChannel::Stroke,
                            .effect{
                                { ColorRuleOp::Set, 0.8f },             // S
                                { ColorRuleOp::Offset, 0.35f }          // E
                            }
                        }
                    },
                    .metrics = {
                        .radius = 6.0f,
                        .padding = Padding{ 12.0f, 10.0f },
                        .spacing = Spacing{ 12.0f, 4.0f }
                    }
                },
                ControlStyle{
                    .name = L"Do",
                    .base = L"DoDont",
                    .seed = Pigment::Green
                },
                ControlStyle{
                    .name = L"Dont",
                    .base = L"DoDont",
                    .seed = Pigment::Red
                },
                ControlStyle{
                    .name = L"CodeBlock",
                    .base = L"AnyNonWindow",
                    .standsOn = L"Section",
                    .rules = {
                        ColorRule{
                            .effect{
                                {},                                     // S
                                { ColorRuleOp::Offset, 0.035f }         // E
                            }
                        },
                        ColorRule{
                            .output = PaintChannel::Band,
                            .effect{
                                {},                                     // S
                                { ColorRuleOp::Offset, 0.12f }          // E
                            }
                        }
                    },
                    .metrics = {
                        .radius = 6.0f,
                        .padding = Padding{ 12.0f, 10.0f }
                    }
                }
            });
            return result;
        }();
        return theme;
    }
}
