export module Themes_App.PreviewPageBase;

import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A page of the preview panel, about fish so that it is never taken for the app's own.
    export class PreviewPageBase : public StackPanel
    {
    public:
        template<typename... Args>
        explicit PreviewPageBase(const CreateParams&, Args&&...);
    public:
        // What the page shows of a theme, in words - the second line of the panel's title.
        [[nodiscard]] virtual std::wstring_view whatItShows() const = 0;
    };


    //-------------------------------------------------------------------------


    // A page's own padding or spacing, given after these, stands in their place.
    template<typename... Args>
    PreviewPageBase::PreviewPageBase(const CreateParams& params, Args&&... args)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            Padding{ 12.0f, 8.0f },
            Spacing{ 8.0f },
            std::forward<Args>(args)...
        }
    {
    }
}
