export module ClaFi.Browser.Application;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.Settings;

import ClaFi.App.Application;

export import ClaFi.Core.Dom_StdSerializers;
export import ClaFi.Core.DomEngine;
export import ClaFi.Core.DomEngine_Dt;
export import ClaFi.Core.DomEngine_Document;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Browser
{

    export template<typename T>
        concept IsBrowserControl = std::derived_from<T, BrowserControl>;

    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend, IsBrowserControl ContentType>
        class BrowserApplication : public Application<PlatformType, GpuBackend>
    {
    public:
        using Base = Application<PlatformType, GpuBackend>;
        using Base::Base;
        using Base::config;
        using Base::name;
        using Base::createDialog;
    public:
        // The root config, what an entry keeps of a tab beyond id, title and path, and what a
        // tab's page keeps in the tab's own file.
        template<typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
        BrowserApplication(PlatformType::Params&&, const AppParams&, TRootSchema&& rootSchema,
            TTabEntrySchema&& tabEntrySchema, TTabSchema&& tabSchema);
        // Finalizes while the settings still stand, so the exit save reaches the tab files.
        ~BrowserApplication() override;

        BrowserSettings& settings() { return m_settings; }

        template <typename... Args>
        std::unique_ptr<Form<ContentType>> createMainForm(Args&&... args);
    private:
        BrowserSettings m_settings;
    };


    //-------------------------------------------------------------------------


    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend, IsBrowserControl ContentType>
    template <typename TRootSchema, typename TTabEntrySchema, typename TTabSchema>
    BrowserApplication<PlatformType, GpuBackend, ContentType>::BrowserApplication(
        typename PlatformType::Params&& platformParams,
        const AppParams& appParams,
        TRootSchema&& rootSchema,
        TTabEntrySchema&& tabEntrySchema,
        TTabSchema&& tabSchema)
            :
        Base{
            std::forward<typename PlatformType::Params>(platformParams),
            appParams,
            {
                createBrowserSettingsSchema(std::forward<TTabEntrySchema>(tabEntrySchema)),
                std::forward<TRootSchema>(rootSchema)
            }
        },
        m_settings{ config(), std::forward<TTabSchema>(tabSchema) }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend, IsBrowserControl ContentType>
    BrowserApplication<PlatformType, GpuBackend, ContentType>::~BrowserApplication()
    {
        this->finalize();
    }

    template<IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend, IsBrowserControl ContentType>
    template<typename ...Args>
    std::unique_ptr<Form<ContentType>> BrowserApplication<PlatformType, GpuBackend, ContentType>::createMainForm(Args && ...args)
    {
        std::unique_ptr<Form<ContentType>> form = this->template createDialog<ContentType>(std::forward<Args>(args)...);

        form->content().initialize(m_settings);
        form->setConfigName(AppContext::k_mainFormName);
        form->window().setTitle(name().plainText());
        return form;
    }

}
