module ClaFi.Core.System.WebFetch;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // WebFetchEvent

    WebFetchEvent::WebFetchEvent(WebFetch& sender)
        :
        EventOf<WebFetch>{ sender }
    {
    }

    // WebFetch - portable members. The constructor and destructor live in the platform
    // implementation unit, where Impl is a complete type.

    void WebFetch::reportDone(const int status, std::string&& body)
    {
        {
            std::lock_guard<std::mutex> lock{ m_pendingMutex };
            m_pendingStatus = status;
            m_pendingBody = std::move(body);
        }
        m_doneTimer.start(MilliSeconds{ 0u });
    }

    void WebFetch::deliver()
    {
        {
            std::lock_guard<std::mutex> lock{ m_pendingMutex };
            m_status = m_pendingStatus;
            m_body = std::move(m_pendingBody);
        }
        m_done = true;
        emitEvent<WebFetchEvent>(*this);
    }
}
