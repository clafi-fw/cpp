export module ClaFi.Documents.Application;

import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;

import ClaFi.Browser.Application;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    export template<typename T>
        concept IsDocumentsBrowser = std::derived_from<T, DocumentsBrowserBase>;

    // A documents browser over the platform an entry point names. See Documents#application
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType>
    class DocumentsApplication
        : public Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>;
        using Base::Base;
    public:
        // Builds the main form over the folder, runs it, and answers what the form answered.
        [[nodiscard]] int run(DocumentsFolder&);
    };


    //-----------------------------------------------------------------------------

    // These are actually should be stated by the end application
    constexpr MinSize k_mainFormMinSize{ 600.0f, 400.0f };
    constexpr PreferredSize k_mainFormPreferredSize{ 900.0f, 600.0f };

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType>
    int DocumentsApplication<PlatformType, GpuBackend, BrowserType>::run(DocumentsFolder& folder)
    {
        // Handed over as the base the browser and its pages ask for: a prop is found by its exact
        // type - see Props::get.
        DocumentsFolder* folderProp = &folder;
        const std::unique_ptr<Form<BrowserType>> form = this->createMainForm(
            this->metrics().primaryWindow,
            this->metrics().primaryWindowShadow,
            UiElement::Dialog,
            k_mainFormMinSize,
            k_mainFormPreferredSize,
            folderProp
        );
        return form->execute();
    }
}
