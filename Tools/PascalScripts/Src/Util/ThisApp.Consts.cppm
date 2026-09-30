export module ThisApp.Consts;

import ClaFi.Documents.Folder;
import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    // What the application is called - its title, its settings folder and its scripts folder.
    export constexpr std::wstring_view k_appName{ L"PascalScripts" };

    // What a script is called and how its files are named where Language.cfg states no extension.
    // The tab config keeps the script under Text, as it stands on screen - see
    // ScriptPage::storeViewState.
    export constexpr Documents::DocumentKind k_scriptKind{
        .extension = L".pas",
        .noun = L"script",
        .nounPlural = L"scripts",
        .newStem = L"New Script",
        .attrName = L"Text"
    };
}
