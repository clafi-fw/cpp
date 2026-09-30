export module ClaFi.Controls.HistoryButton;

export import ClaFi.Controls.SplitButton;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Which way a history button walks.
    export enum class HistoryDirection
    {
        Undo,
        Redo
    };

    // A split button on Undo or Redo whose strip lists the steps to take. See Controls
    export class HistoryButton : public SplitButton
    {
    public:
        template<typename... Args>
        HistoryButton(const CreateParams&, HistoryDirection, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"HistoryButton"; }
    protected:
        // The steps of the history the action's subject answers with; the run picked is taken.
        void dropdown(DropdownEvent&) override;
    private:
        [[nodiscard]] static Action& actionOf(HistoryDirection);
    private:
        HistoryDirection m_direction;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    HistoryButton::HistoryButton(const CreateParams& params, const HistoryDirection direction,
        Args&&... args)
        :
        SplitButton{ params, actionOf(direction), std::forward<Args>(args)... },
        m_direction{ direction }
    {
    }
}
