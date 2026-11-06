// The framework's own theme, written whole by the Themes app's Write to source.
// An edit made here is lost at the next write.
module ClaFi.Core.AppTheme_Theme;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    AppTheme::AppTheme()
    {
        colors.anchorHue = 0.666534f;

        colors.harmonyKind = ColorHarmonyKind::Complementary;

        colors.paletteHues = {
            0.666534f,
            0.666534f,
            0.16653395f
        };

        colors.darkModeFloor = 0.11262504f;

        colors.accent = ColorEffect{
            { ColorRuleHueOp::PaletteColor2 },          // H
            { ColorRuleOp::Set, 1.0f },                 // S
            { ColorRuleOp::Set, 0.544542f }             // E
        };

        colors.spot = ColorEffect{
            { ColorRuleHueOp::PaletteColor3, 0.805479f }, // H
            { ColorRuleOp::Set, 1.0f },                 // S
            { ColorRuleOp::Set, 0.648933f }             // E
        };

        styles.list = {
            ControlStyle{
                .name = L"AnyElement"
            },
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 6.0f,
                    .padding = Padding{ 4.0f, 4.0f },
                    .shadowBlur = 12.0f,
                    .shadowOffset = FloatPoint{ 0.0f, 3.0f },
                    .shadowOpacity = 0.35f
                }
            },
            ControlStyle{
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
            ControlStyle{
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
                },
                .metrics = {
                    .radius = 8.0f,
                    .shadowBlur = 24.0f,
                    .shadowOffset = FloatPoint{ 0.0f, 6.0f },
                    .shadowOpacity = 0.4f
                }
            },
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 1.0f, 1.0f }
                }
            },
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Thin,
                    .band = 1.66f,
                    .radius = 4.0f,
                    .padding = Padding{ 8.0f, 4.0f },
                    .spacing = Spacing{ 4.0f, 4.0f }
                }
            },
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
                .name = L"Expander",
                .base = L"Section",
                .standsOn = L"Page",
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 1.0f, 1.0f }
                }
            },
            ControlStyle{
                .name = L"Field",
                .base = L"Section",
                .standsOn = L"Page",
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 8.0f, 4.0f }
                }
            },
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
                .name = L"Dropdown",
                .base = L"AnyWindow",
                .rules = {
                    ColorRule{
                        .effect{
                            { ColorRuleOp::NoChange, 0.195241f },   // S
                            { ColorRuleOp::Offset, 0.039854f }      // E
                        }
                    }
                }
            },
            ControlStyle{
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
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 1.0f, 1.0f }
                }
            },
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
                .name = L"GridCell",
                .base = L"AnyNonWindow",
                .standsOn = L"Grid",
                .metrics = {
                    .border = Thickness::Hairline,
                    .radius = 4.0f,
                    .padding = Padding{ 5.0f, 3.0f },
                    .spacing = Spacing{ 4.0f, 4.0f },
                    .depth = 1.0f
                }
            },
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 8.0f, 4.0f },
                    .spacing = Spacing{ 4.0f, 4.0f },
                    .minSize = MinSize{ 40.0f, 24.0f },
                    .depth = 1.0f
                }
            },
            ControlStyle{
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
            ControlStyle{
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
                },
                .metrics = {
                    .growIn = 0.83f
                }
            },
            ControlStyle{
                .name = L"ListItem",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .metrics = {
                    .border = Thickness::Thin,
                    .radius = 4.0f,
                    .padding = Padding{ 5.0f, 3.0f },
                    .spacing = Spacing{ 4.0f, 4.0f },
                    .depth = 1.0f
                }
            },
            ControlStyle{
                .name = L"TextBox",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .metrics = {
                    .band = 1.66f
                }
            },
            ControlStyle{
                .name = L"Slider",
                .base = L"AnyNonWindow",
                .standsOn = L"Section",
                .metrics = {
                    .band = 1.66f,
                    .radius = 4.0f
                }
            },
            ControlStyle{
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
            ControlStyle{
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
            ControlStyle{
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
                },
                .metrics = {
                    .border = Thickness::Hairline,
                    .radius = 3.0f,
                    .padding = Padding{ 2.33f, 2.33f },
                    .minSize = MinSize{ 16.0f, 16.0f },
                    .depth = 3.0f
                }
            },
            ControlStyle{
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
            ControlStyle{
                .name = L"HoverIndicator",
                .base = L"AnyIndicator"
            },
            ControlStyle{
                .name = L"ScrollBar",
                .base = L"AnyNonWindow",
                .standsOn = L"Section"
            },
            ControlStyle{
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
                },
                .metrics = {
                    .radius = 3.0f,
                    .minSize = MinSize{ 17.0f, 17.0f }
                }
            },
            ControlStyle{
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
                },
                .metrics = {
                    .radius = 5.5f
                }
            },
            ControlStyle{
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
                },
                .metrics = {
                    .band = 4.0f,
                    .radius = 4.0f,
                    .padding = Padding{ 16.0f, 12.0f },
                    .spacing = Spacing{ 12.0f, 12.0f }
                }
            },
            ControlStyle{
                .name = L"InfoCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Blue
            },
            ControlStyle{
                .name = L"QuestionCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Blue
            },
            ControlStyle{
                .name = L"WarningCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Yellow
            },
            ControlStyle{
                .name = L"ErrorCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Red
            },
            ControlStyle{
                .name = L"OkCallout",
                .base = L"AnyCallout",
                .seed = Pigment::Green
            },
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
        };
    }
}
