export module ClaFi.Documents.Application;

import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;

import ClaFi.Browser.Application;

import ClaFi.App.Application;

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
    export template<typename T>
        concept IsDocumentsFolder = std::derived_from<T, DocumentsFolder>;

    // A documents browser over the platform an entry point names, owning the folder its pages
    // work on. The name and the config schemas are the application's. See Documents#application
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType, IsDocumentsFolder FolderType>
    class DocumentsApplication
        : public Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, BrowserType>;
    public:
        // The root config, what an entry keeps of a tab beyond what the browser puts there, and
        // what a tab's page keeps - the document it is editing among it.
        template<typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
        DocumentsApplication(PlatformType::Params&&, const AppParams&,
            const std::filesystem::path& directory, TRootSchema&&, TTabEntrySchema&&, TTabSchema&&);
    public:
        [[nodiscard]] FolderType& folder() { return m_folder; }
        // Builds the main form over the folder, runs it, and answers what the form answered.
        [[nodiscard]] int run();
    private:
        // Built after the platform, which its watch stands on, and outliving every window: the
        // browser and its pages hold it by reference.
        FolderType m_folder;
    };


    //-----------------------------------------------------------------------------


    constexpr MinSize k_mainFormMinSize{ 900.0f, 600.0f };

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType, IsDocumentsFolder FolderType>
    template<typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
    DocumentsApplication<PlatformType, GpuBackend, BrowserType, FolderType>::DocumentsApplication(
        typename PlatformType::Params&& platformParams,
        const AppParams& appParams,
        const std::filesystem::path& directory,
        TRootSchema&& rootSchema,
        TTabEntrySchema&& tabEntrySchema,
        TTabSchema&& tabSchema)
        :
        Base{
            std::move(platformParams),
            appParams,
            std::forward<TRootSchema>(rootSchema),
            std::forward<TTabEntrySchema>(tabEntrySchema),
            std::forward<TTabSchema>(tabSchema)
        },
        m_folder{ directory }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend,
        IsDocumentsBrowser BrowserType, IsDocumentsFolder FolderType>
    int DocumentsApplication<PlatformType, GpuBackend, BrowserType, FolderType>::run()
    {
        // Handed over as the base the browser and its pages ask for: a prop is found by its exact
        // type - see Props::get.
        DocumentsFolder* folder = &m_folder;
        const std::unique_ptr<Form<BrowserType>> form = this->createMainForm(
            this->metrics().primaryWindow,
            this->metrics().primaryWindowShadow,
            UiElement::Dialog,
            k_mainFormMinSize,
            folder
        );
        return form->execute();
    }
}
