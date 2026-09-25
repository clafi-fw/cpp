module ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Events;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Types;

namespace ClaFi
{
    // FormContext

    FormContext::FormContext(const Platform& platform, const AppTheme& theme,
        const BakedColors& bakedColors, Graphics::Canvas& canvas, const Scaler& scaler)
        :
        m_platform{ platform },
        m_theme{ &theme },
        m_bakedColors{ &bakedColors },
        m_scaler{ &scaler },
        m_canvas{ canvas }
    {
    }

    FormContext::FormContext(const FormContext& other)
        :
        m_platform{ other.m_platform },
        m_theme{ other.m_theme },
        m_bakedColors{ other.m_bakedColors },
        m_scaler{ other.m_scaler },
        m_canvas{ other.m_canvas }
    {
    }

    Transfer::Clipboard& FormContext::clipboard() const
    {
        return m_platform.clipboard();
    }

    // ControlEventBase

    ControlEventBase::ControlEventBase(FormContext& formContext)
        :
        m_formContext{ formContext }
    {
    }

    // ControlEventBaseC

    ControlEventBaseC::ControlEventBaseC(const FormContext& formContext)
        :
        m_formContext{ formContext }
    {
    }

    // Platform

    Platform::Platform(Transfer::Clipboard& clipboard)
        :
        m_clipboard{ clipboard }
    {
    }

    EventDispatcher& Platform::events()
    {
        static EventDispatcher dispatcher{};
        return dispatcher;
    }

    // ScopedWaitCursor

    ScopedWaitCursor::ScopedWaitCursor()
        :
        m_previous{ Platform::cursor() }
    {
        Platform::setCursor(CursorShape::Wait);
    }

    ScopedWaitCursor::~ScopedWaitCursor()
    {
        Platform::setCursor(m_previous);
    }

}
