export module ClaFi.Tools.WhatsClip.EncodingPick;

import ClaFi.Tools.WhatsClip.AutoPick;
import ClaFi.Tools.WhatsClip.Encodings;

import ClaFi.Controls.ComboBox;

import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // Auto and the list's encodings, the face on Auto reading the encoding the bytes were found
    // to be in.
    export class EncodingPick : public AutoPicker
    {
    public:
        template<typename... Args>
        EncodingPick(const CreateParams&, const EncodingList&, float fontSize, Args&&...);
    public:
        // The encoding picked by hand, or none while Auto is picked.
        [[nodiscard]] std::optional<EncodingEntry> pickedEncoding() const;
        // The bytes as text: in the encoding picked, or on Auto in the first the list claims
        // them for, whose name goes on the face. Bytes nothing claims are read as UTF-8, that
        // being what a byte string meant to be read is, and the face reads Auto.
        [[nodiscard]] std::wstring decode(std::wstring_view formatName, std::string_view bytes);
    private:
        [[nodiscard]] static Names namesOf(const EncodingList&);
    private:
        const EncodingList& m_encodings;
    };


//-----------------------------------------------------------------------------


    template<typename... Args>
    EncodingPick::EncodingPick(const CreateParams& params, const EncodingList& encodings,
        const float fontSize, Args&&... args)
        :
        AutoPicker{ params, namesOf(encodings), fontSize, std::forward<Args>(args)... },
        m_encodings{ encodings }
    {
    }
}
