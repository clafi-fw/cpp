// The framework's own theme, written whole by the Themes app's Write to source.
// An edit made here is lost at the next write.
module ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.AppTheme_Palette;

import ClaFi.StdLib;

namespace ClaFi
{
    ThemeColors::ThemeColors()
    {
        anchorHue = 0.666534f;

        harmonyKind = ColorHarmonyKind::Complementary;

        paletteHues = {
            0.666534f,
            0.666534f,
            0.16653395f
        };

        darkModeFloor = 0.098735f;

        accent = ColorEffect{
            { ColorRuleHueOp::PaletteColor2 },          // H
            { ColorRuleOp::Set, 1.0f },                 // S
            { ColorRuleOp::Set, 0.544542f }             // E
        };

        spot = ColorEffect{
            { ColorRuleHueOp::PaletteColor3, 0.805479f }, // H
            { ColorRuleOp::Set, 1.0f },                 // S
            { ColorRuleOp::Set, 0.648933f }             // E
        };

        rules.focusRing = {
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::Keyboard },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 1.0f }              // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Current },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 1.0f }              // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .andInputs{ RuleInput::Current, RuleInput::Mouse },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::WindowFocused, RuleInput::Keyboard },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            }
        };

        rules.of(UiElement::Dialog) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::PaletteColor1, 0.252055f }, // H
                    { ColorRuleOp::Set, 0.050285f },        // S
                    { ColorRuleOp::Set, 0.001878f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.120151f }      // E
                }
            },
            ColorRule{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleHueOp::PaletteColor1 },      // H
                    { ColorRuleOp::Set, 0.692649f },        // S
                    { ColorRuleOp::Set, 0.238022f }         // E
                }
            }
        };

        rules.of(UiElement::Page) = {
            ColorRule{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.044491f }      // E
                }
            }
        };

        rules.of(UiElement::Tab) = {
            ColorRule{
                .inputs{ RuleInput::Hovered, RuleInput::Selected },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.928767f }, // H
                    { ColorRuleOp::NoChange, 0.490111f },   // S
                    { ColorRuleOp::Offset, 0.25f }          // E
                }
            }
        };

        rules.of(UiElement::Section) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.195241f },   // S
                    { ColorRuleOp::Offset, 0.039854f }      // E
                }
            }
        };

        rules.of(UiElement::SectionHeader) = {
            ColorRule{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.111111f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.897275f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        rules.of(UiElement::ToolBar) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::NoChange, 0.512329f }, // H
                    { ColorRuleOp::NoChange, 0.137752f },   // S
                    { ColorRuleOp::Set, 0.052564f }         // E
                }
            }
        };

        rules.of(UiElement::DialogTitle) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.18f },       // S
                    { ColorRuleOp::Set, 0.13f }             // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleOp::NoChange, 0.501121f },   // S
                    { ColorRuleOp::Set, 0.08f }             // E
                }
            },
            ColorRule{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.131507f }, // H
                    { ColorRuleOp::NoChange, 1.0f },        // S
                    { ColorRuleOp::Offset, -0.3f }          // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Selected },
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.3f }           // E
                }
            }
        };

        rules.of(UiElement::Menu) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::PaletteColor1 },      // H
                    { ColorRuleOp::Set, 0.072888434f },     // S
                    { ColorRuleOp::Set, 0.0011279281f }     // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.5f }              // E
                }
            },
            ColorRule{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleOp::Set, 0.68912506f },      // S
                    { ColorRuleOp::Set, 0.23544617f }       // E
                }
            }
        };

        rules.of(UiElement::Tooltip) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::PaletteColor3 },      // H
                    { ColorRuleOp::Set, 0.12352588f },      // S
                    { ColorRuleOp::Set, 0.053887f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.278328f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleHueOp::PaletteColor3 },      // H
                    { ColorRuleOp::Set, 0.69047594f },      // S
                    { ColorRuleOp::Set, 0.23544617f }       // E
                }
            }
        };

        rules.of(UiElement::Divider) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.191474f },   // S
                    { ColorRuleOp::Offset, 0.109524f }      // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        rules.of(UiElement::Grid) = {
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.120654f }      // E
                }
            }
        };

        rules.of(UiElement::GridHeader) = {
            ColorRule{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.038054f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.897275f }         // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        rules.of(UiElement::GridRow) = {
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.190021f },     // S
                    { ColorRuleOp::Offset, 0.08306903f }    // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .andInputs{ RuleInput::Mouse },
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.073892f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::Keyboard },
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.080827f }      // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        rules.of(UiElement::Button) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.054944f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.094499f },     // S
                    { ColorRuleOp::Offset, 0.099311f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .andInputs{ RuleInput::Mouse },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::Keyboard },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.037839f }      // E
                }
            }
        };

        rules.of(UiElement::ToolButton) = {
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.094499f },     // S
                    { ColorRuleOp::Offset, 0.099311f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .andInputs{ RuleInput::Mouse },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::Keyboard },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered, RuleInput::Selected },
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.037839f }      // E
                }
            }
        };

        rules.of(UiElement::SelectedText) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, -0.042085f },    // S
                    { ColorRuleOp::Offset, 0.092057f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::WindowFocused },
                .effect{
                    { ColorRuleOp::Offset, 0.382042f },     // S
                    { ColorRuleOp::Offset, 0.094934f }      // E
                }
            }
        };

        rules.of(UiElement::FoundText) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::Set, 0.6f },             // S
                    { ColorRuleOp::Offset, 0.12f }          // E
                }
            }
        };

        rules.of(UiElement::SelectionIndicator) = {
            ColorRule{
                .effect{
                    { ColorRuleHueOp::PaletteColor1 },      // H
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.155899f }      // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.125f },        // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            }
        };

        rules.of(UiElement::HoverIndicator) = {
            ColorRule{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.125f },        // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            }
        };

        rules.of(UiElement::ScrollButton) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.03f }          // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.0f },          // S
                    { ColorRuleOp::Offset, 0.3f }           // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            }
        };

        rules.of(UiElement::ScrollThumb) = {
            ColorRule{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.4f }           // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.0f },          // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            }
        };

        rules.of(UiElement::Testee) = {
            ColorRule{
                .inputs{ RuleInput::Keyboard },
                .effect{
                    { ColorRuleHueOp::PaletteColor3 },      // H
                    { ColorRuleOp::Set, 0.5f },             // S
                    { ColorRuleOp::Set, 0.5f }              // E
                }
            }
        };

        rules.of(UiElement::Bestee) = {
            ColorRule{
                .inputs{ RuleInput::Mouse },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 0.5f },             // S
                    { ColorRuleOp::Set, 0.5f }              // E
                }
            }
        };
    }
}
