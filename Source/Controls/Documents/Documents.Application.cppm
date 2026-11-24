export module ClaFi.Documents.Application;

import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;

import ClaFi.Browser.Application;

import ClaFi.App.WindowApplication;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    export template<typename T>
        concept IsDocumentsBrowser = std::derived_from<T, DocumentsBrowserBase>;

    // A documents browser over the platform an entry point names. See Documents
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType>
    class DocumentsApplication
        : public Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>;
    public:
        template<typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
        DocumentsApplication(typename PlatformType::Params&&, const ApplicationProperties&,
            TRootSchema&& rootSchema, TTabEntrySchema&& tabEntrySchema, TTabSchema&& tabSchema);
        // Builds the main form over the folder, runs it, and answers what the form answered.
        [[nodiscard]] int run(DocumentsFolder&, MinSize, PreferredSize);
    };


    //-----------------------------------------------------------------------------


    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType>
    template<typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
    DocumentsApplication<PlatformType, GpuBackend, BrowserType>::DocumentsApplication(
        typename PlatformType::Params&& platformParams, const ApplicationProperties& properties,
        TRootSchema&& rootSchema, TTabEntrySchema&& tabEntrySchema, TTabSchema&& tabSchema)
        :
        Base{
            std::move(platformParams),
            properties,
            Dom::Dt::Section{
                DocumentsBrowserBase::storageSettingsSchema(),
                std::forward<TRootSchema>(rootSchema)
            },
            std::forward<TTabEntrySchema>(tabEntrySchema),
            std::forward<TTabSchema>(tabSchema)
        }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType>
    int DocumentsApplication<PlatformType, GpuBackend, BrowserType>::run(DocumentsFolder& folder,
        const MinSize minSize, const PreferredSize preferredSize)
    {
        // Handed over as the base the browser and its pages ask for: a prop is found by its exact
        // type - see Props::get.
        DocumentsFolder* folderProp = &folder;
        const std::unique_ptr<Form<BrowserType>> form = this->createMainForm(
            UiElement::Dialog,
            minSize,
            preferredSize,
            folderProp
        );
        return form->execute();
    }
}
