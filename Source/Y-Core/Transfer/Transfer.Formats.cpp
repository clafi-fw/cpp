module ClaFi.Core.Transfer.Formats;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    Format Format::Of(const StandardFormat standardFormat)
    {
        return Format{ standardFormat };
    }

    Format Format::Custom(std::wstring name)
    {
        return Format{ std::move(name) };
    }

    Format::Kind Format::kind() const
    {
        return static_cast<Kind>(m_identity.index());
    }

    StandardFormat Format::standard() const
    {
        const StandardFormat* held = std::get_if<StandardFormat>(&m_identity);
        if (!held)
            noReach("Transfer::Format::standard asked of a format that is not a standard one");

        return *held;
    }

    const std::wstring& Format::name() const
    {
        const std::wstring* held = std::get_if<std::wstring>(&m_identity);
        if (!held)
            noReach("Transfer::Format::name asked of a format that carries no name");

        return *held;
    }

    Format::Format(Identity identity)
        :
        m_identity{ std::move(identity) }
    {
    }

    Format PlainText::format()
    {
        return Format::Of(StandardFormat::Text);
    }

    Format Picture::format()
    {
        return Format::Of(StandardFormat::Picture);
    }
}
