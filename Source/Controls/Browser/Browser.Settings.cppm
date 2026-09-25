export module ClaFi.Browser.Settings;

import ClaFi.Browser.Consts;

import ClaFi.Dom.Formats.ClaFi;
import ClaFi.Diagnostic.Log;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Browser
{
    using namespace Dom::Dt;

    // What every entry keeps of a tab; an application adds to it what its own tab shows unopened.
    export const Section k_tabEntrySchema
    {
        Value{ ConfigNames::id, L"" },
        Value{ ConfigNames::title, ConfigNames::untitledPage },
        Value{ ConfigNames::path, L"" }
    };

    // The browser's part of the application config - the selected tab and an entry per open tab.
    export template <typename TTabEntrySchema>
    Section createBrowserSettingsSchema(TTabEntrySchema&& tabEntrySchema);

    export using TabEntries = Dom::Sequence<Dom::Section>;

    // The tab entries in the application config, and a file per shown page. See Browser
    export class BrowserSettings
    {
    public:
        BrowserSettings(Dom::DocumentBase& appConfig, Dom::Dt::Section&& tabSchema);
        [[nodiscard]] Dom::Section& appConfig() const { return m_appConfig; }
        [[nodiscard]] TabEntries& openTabs() const { return m_openTabs; }
        [[nodiscard]] Dom::Value<std::wstring>& selectedTab() const { return m_selectedTab; }
        // The entry of a listed tab.
        [[nodiscard]] Dom::Section& tabEntry(std::wstring_view tabId) const;
        // What a tab's page keeps, in the tab's own file - read the first time it is asked for.
        [[nodiscard]] Dom::Section& tabConfig(std::wstring_view tabId);
        // Lists a tab, answering its entry.
        Dom::Section& addTabEntry(std::wstring_view tabId);
        // Unlists a tab. Its file, where it has one, goes with the next save.
        void deleteTabEntry(std::wstring_view tabId);
    private:
        using TabDocument = Dom::Document<Dom::FileFormat::ClaFi>;
        using TabDocuments = std::map<std::wstring, std::unique_ptr<TabDocument>, std::less<>>;
    private:
        // Writes the file of every page that has one and removes every other file from the folder.
        void save();
        [[nodiscard]] std::filesystem::path tabsFolder() const;
        [[nodiscard]] std::filesystem::path tabFilePath(std::wstring_view tabId) const;
    private:
        Dom::DocumentBase& m_appConfig;
        Dom::Dt::Section m_tabSchema;
        TabEntries& m_openTabs{ (m_appConfig / ConfigNames::openTabs).as<TabEntries>() };
        Dom::Value<std::wstring>& m_selectedTab{
            m_appConfig.child(ConfigNames::selectedTab)->as<Dom::Value<std::wstring>>() };
        TabDocuments m_tabDocuments{};
        ScopedEventConnection m_appConfigSaved{};
    };

    //-------------------------------------------------------------------------


    template <typename TTabEntrySchema>
    Section createBrowserSettingsSchema(TTabEntrySchema&& tabEntrySchema)
    {
        return {
            Value{ ConfigNames::selectedTab, L"" },
            Sequence
            {
                ConfigNames::openTabs,
                Section
                {
                    k_tabEntrySchema,
                    std::forward<TTabEntrySchema>(tabEntrySchema)
                }
            }
        };
    }

    // BrowserSettings

    BrowserSettings::BrowserSettings(Dom::DocumentBase& appConfig, Dom::Dt::Section&& tabSchema)
        :
        m_appConfig{ appConfig },
        m_tabSchema{ std::move(tabSchema) }
    {
        m_appConfigSaved = m_appConfig.connectEvent([this](Dom::SaveEvent&) {
            save();
        });
    }

    Dom::Section& BrowserSettings::tabEntry(std::wstring_view tabId) const
    {
        Dom::Section* entry = m_openTabs.childWhere(ConfigNames::id, tabId);
        if (!entry)
            unreachable("BrowserSettings: no tab is listed under this id");
        return *entry;
    }

    Dom::Section& BrowserSettings::tabConfig(std::wstring_view tabId)
    {
        const TabDocuments::iterator found = m_tabDocuments.find(tabId);
        if (found != m_tabDocuments.end())
            return *found->second;
        auto document = std::make_unique<TabDocument>(
            tabFilePath(tabId), Dom::AutoSave::No, m_tabSchema, Dom::WriteDefaults::No);
        TabDocument& result = *document;
        m_tabDocuments.insert_or_assign(std::wstring{ tabId }, std::move(document));
        const bool read = result.load();
        diagnosticLog(std::format(
            L"tab config {} {}", read ? L"read" : L"absent", result.path().wstring()));
        return result;
    }

    Dom::Section& BrowserSettings::addTabEntry(std::wstring_view tabId)
    {
        Dom::Section& entry = m_openTabs.add();
        (entry / ConfigNames::id).set(tabId);
        return entry;
    }

    void BrowserSettings::deleteTabEntry(std::wstring_view tabId)
    {
        if (Dom::Section* entry = m_openTabs.childWhere(ConfigNames::id, tabId))
            entry->deleteSelf();
        const TabDocuments::iterator document = m_tabDocuments.find(tabId);
        if (document != m_tabDocuments.end())
            m_tabDocuments.erase(document);
    }

    void BrowserSettings::save()
    {
        for (const TabDocuments::value_type& tabDocument : m_tabDocuments)
        {
            tabDocument.second->save();
            diagnosticLog(std::format(
                L"tab config written {}", tabDocument.second->path().wstring()));
        }

        // THE FOLDER IS THE BROWSER'S. A file in it under an id the entries do not carry is a
        // closed tab's, and this is where it goes: the disk changes when the entries are written,
        // and a run that ends without writing leaves the files the last written entries name.
        std::error_code error{};
        const std::filesystem::directory_iterator files{ tabsFolder(), error };
        for (const std::filesystem::directory_entry& entry : files)
        {
            if (m_openTabs.childWhere(ConfigNames::id, entry.path().stem().wstring()))
                continue;
            std::filesystem::remove(entry.path(), error);
            diagnosticLog(std::format(L"tab config removed {}", entry.path().wstring()));
        }
    }

    std::filesystem::path BrowserSettings::tabsFolder() const
    {
        return m_appConfig.path().parent_path() / ConfigNames::openTabs;
    }

    // Named by the id, with the extension the application config file has.
    std::filesystem::path BrowserSettings::tabFilePath(std::wstring_view tabId) const
    {
        std::filesystem::path result = tabsFolder() / tabId;
        result += m_appConfig.path().extension();
        return result;
    }
}
