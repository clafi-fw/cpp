export module Themes_App.DoDontPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.DoDont;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A Do and Don't block, its two lists side by side.
    export class DoDontPreview : public PreviewPageBase
    {
    public:
        explicit DoDontPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{ L"Do and Don't sides" };

        DoDont& m_block{ add<DoDont>() };
    };
}
