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

        darkModeFloor = 0.11262504f;

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

        rules.elements = {
            ElementRules{
                .name = L"AnyElement"
            },
            ElementRules{
                .name = L"AnyWindow",
                .base = L"AnyElement",
                .rules = {
                    ColorRule{
                        .output = PaintChannel::Stroke,
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.1291775f }     // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::WindowFocused },
                        .output = PaintChannel::Stroke,
                        .effect{
                            { ColorRuleOp::Set, 0.5f },             // S
                            { ColorRuleOp::Offset, 0.09757227f }    // E
                        }
                    },
                    ColorRule{
                        .output = PaintChannel::Shadow,
                        .effect{
                            { ColorRuleOp::Set, 0.20879121f },      // S
                            { ColorRuleOp::Set, 0.23626374f }       // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::WindowFocused },
                        .output = PaintChannel::Shadow,
                        .effect{
                            { ColorRuleOp::Set, 0.6904762f },       // S
                            { ColorRuleOp::Set, 0.24358974f }       // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"AnyNonWindow",
                .base = L"AnyElement",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Current },
                        .output = PaintChannel::Stroke,
                        .effect{
                            { ColorRuleOp::NoChange, 0.0f },        // S
                            { ColorRuleOp::Set, 0.664659f }         // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
                        .andInputs{ RuleInput::Current },
                        .output = PaintChannel::Stroke,
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Set, 1.0f },             // S
                            { ColorRuleOp::Set, 0.543364f }         // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Edit },
                        .output = PaintChannel::Band,
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Set, 1.0f },             // S
                            { ColorRuleOp::Set, 0.544542f }         // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Dialog",
                .base = L"AnyWindow",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::PaletteColor1, 0.252055f }, // H
                            { ColorRuleOp::Set, 0.050285f },        // S
                            { ColorRuleOp::Set, 0.001878f }         // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Page",
                .base = L"AnyNonWindow",
                .standsOn = L"Dialog",
                .rules = {
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
                }
            },
            ElementRules{
                .name = L"Tab",
                .base = L"AnyNonWindow",
                .standsOn = L"Page",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Hot, RuleInput::Current },
                        .output = PaintChannel::Stroke,
                        .effect{
                            { ColorRuleHueOp::NoChange, 0.928767f }, // H
                            { ColorRuleOp::NoChange, 0.490111f },   // S
                            { ColorRuleOp::Offset, 0.10241f }       // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Current },
                        .output = PaintChannel::Band,
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Set, 1.0f },             // S
                            { ColorRuleOp::Set, 0.544542f }         // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Section",
                .base = L"AnyNonWindow",
                .standsOn = L"Page",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.195241f },   // S
                            { ColorRuleOp::Offset, 0.039854f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"SectionHeader",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
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
                }
            },
            ElementRules{
                .name = L"ToolBar",
                .base = L"AnyNonWindow",
                .standsOn = L"Page",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::NoChange, 0.512329f }, // H
                            { ColorRuleOp::NoChange, 0.137752f },   // S
                            { ColorRuleOp::Set, 0.052564f }         // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"DialogTitle",
                .base = L"AnyNonWindow",
                .standsOn = L"Dialog",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.18f },       // S
                            { ColorRuleOp::Set, 0.13f }             // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::WindowFocused },
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
                        .inputs{ RuleInput::WindowFocused },
                        .output = PaintChannel::Text,
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.3f }           // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Menu",
                .base = L"AnyWindow",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::PaletteColor1 },      // H
                            { ColorRuleOp::Set, 0.072888434f },     // S
                            { ColorRuleOp::Set, 0.0011279281f }     // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Tooltip",
                .base = L"AnyWindow",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::PaletteColor3 },      // H
                            { ColorRuleOp::Set, 0.12352588f },      // S
                            { ColorRuleOp::Set, 0.053887f }         // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"Divider",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
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
                }
            },
            ElementRules{
                .name = L"Grid",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .output = PaintChannel::Stroke,
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.120654f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"GridHeader",
                .base = L"AnyNonWindow",
                .standsOn = L"Grid",
                .rules = {
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
                }
            },
            ElementRules{
                .name = L"GridRow",
                .base = L"AnyNonWindow",
                .standsOn = L"Grid",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Selected },
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Offset, 0.190021f },     // S
                            { ColorRuleOp::Offset, 0.08306903f }    // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.073892f }      // E
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
                }
            },
            ElementRules{
                .name = L"AnyButton",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Selected },
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Offset, 0.094499f },     // S
                            { ColorRuleOp::Offset, 0.099311f }      // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
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
                    }
                }
            },
            ElementRules{
                .name = L"Button",
                .base = L"AnyButton",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.0f },        // S
                            { ColorRuleOp::Offset, 0.054944f }      // E
                        }
                    },
                    ColorRule{
                        .output = PaintChannel::Stroke,
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.037839f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"ToolButton",
                .base = L"AnyButton",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Hot, RuleInput::Selected },
                        .output = PaintChannel::Stroke,
                        .effect{
                            {},                                     // S
                            { ColorRuleOp::Offset, 0.037839f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"TextBox",
                .base = L"AnyNonWindow",
                .standsOn = L"Section"
            },
            ElementRules{
                .name = L"SelectedText",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Offset, -0.042085f },    // S
                            { ColorRuleOp::Offset, 0.092057f }      // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Focused },
                        .effect{
                            { ColorRuleOp::Offset, 0.382042f },     // S
                            { ColorRuleOp::Offset, 0.094934f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"FoundText",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .seed = Pigment::Yellow,
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::Set, 0.6f },             // S
                            { ColorRuleOp::Offset, 0.12f }          // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"AnyIndicator",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .inputs{ RuleInput::Selected },
                        .effect{
                            { ColorRuleHueOp::PaletteColor2 },      // H
                            { ColorRuleOp::Set, 1.0f },             // S
                            { ColorRuleOp::Set, 0.544542f }         // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
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
                }
            },
            ElementRules{
                .name = L"SelectionIndicator",
                .base = L"AnyIndicator",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleHueOp::PaletteColor1 },      // H
                            { ColorRuleOp::NoChange, 0.0f },        // S
                            { ColorRuleOp::Offset, 0.155899f }      // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"HoverIndicator",
                .base = L"AnyIndicator"
            },
            ElementRules{
                .name = L"ScrollButton",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.0f },        // S
                            { ColorRuleOp::Offset, 0.03f }          // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
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
                }
            },
            ElementRules{
                .name = L"ScrollThumb",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.0f },        // S
                            { ColorRuleOp::Offset, 0.4f }           // E
                        }
                    },
                    ColorRule{
                        .inputs{ RuleInput::Hot },
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
                }
            },
            ElementRules{
                .name = L"AnyCallout",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::Set, 0.12f },            // S
                            { ColorRuleOp::Set, 0.054f }            // E
                        }
                    },
                    ColorRule{
                        .output = PaintChannel::Band,
                        .effect{
                            { ColorRuleOp::Set, 0.85f },            // S
                            { ColorRuleOp::Set, 0.6f }              // E
                        }
                    }
                }
            },
            ElementRules{
                .name = L"InfoCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Blue
            },
            ElementRules{
                .name = L"QuestionCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Blue
            },
            ElementRules{
                .name = L"WarningCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Yellow
            },
            ElementRules{
                .name = L"ErrorCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Red
            },
            ElementRules{
                .name = L"OkCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Green
            },
            ElementRules{
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
                }
            },
            ElementRules{
                .name = L"Do",
                .base = L"DoDont",
                .seed = Pigment::Green
            },
            ElementRules{
                .name = L"Dont",
                .base = L"DoDont",
                .seed = Pigment::Red
            },
            ElementRules{
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
                }
            }
        };
    }
}
