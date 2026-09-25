export module ClaFi.Tools.WhatsClip.AutoPick;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.TextItems;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // A combobox of Auto and a list of names, whose face on Auto reads the name found for it
    // with a muted (Auto) after it. The list's Auto item keeps its word: it is what the user
    // picks to have the answer asked rather than stated. What is found, and from what, is the
    // derived pick's affair.
    export class AutoPicker : public Controls::ComboBox
    {
    public:
        using Names = std::vector<std::wstring_view>;
    public:
        template<typename... Args>
        AutoPicker(const CreateParams&, const Names&, float fontSize, Args&&...);
    public:
        // The list index picked by hand, or none while Auto is picked.
        [[nodiscard]] std::optional<std::size_t> pickedIndex() const;
    protected:
        // What the face reads while Auto is picked - empty where nothing was found. Invalidates:
        // dropped by an align pass in progress, whose paint reads the face anyway, and needed
        // when the question came from outside one. See Controls#detectlanguageevent
        void setFoundName(std::wstring_view);
        void getMainText(GetTextEvent&) const override;
    private:
        [[nodiscard]] bool isAuto() const;
        // Auto, then the names, every item at the size given.
        [[nodiscard]] static Controls::TextItems itemsFor(const Names&, float fontSize);
        [[nodiscard]] static Text itemText(std::wstring_view name, float fontSize);
        static constexpr std::size_t k_autoItem = 0;
        // What the face says after a name found rather than picked.
        static constexpr std::wstring_view k_autoNote = L" (Auto)";
    private:
        float m_fontSize;
        std::wstring m_foundName{};
    };


//-----------------------------------------------------------------------------


    template<typename... Args>
    AutoPicker::AutoPicker(const CreateParams& params, const Names& names, const float fontSize,
        Args&&... args)
        :
        ComboBox{
            params,
            itemsFor(names, fontSize),
            ItemIndex{ k_autoItem },
            std::forward<Args>(args)...
        },
        m_fontSize{ fontSize }
    {
    }
}
