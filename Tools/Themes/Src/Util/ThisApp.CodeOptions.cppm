export module ThisApp.CodeOptions;

import ThisApp.ThemeToCppCode;

import ClaFi.Core.Foundation;
import ClaFi.Dom;

namespace ThisApp::CodeOptions
{
    using namespace ClaFi;

    // How much of a theme a code page states, and how the C++ page states it. One answer serves
    // every page and every theme tab, so it belongs to the application rather than to a page -
    // and so does every command that moves it.
    export [[nodiscard]] CodeContent content();
    export [[nodiscard]] CodeScope scope();

    // Takes both from the root config section, which is where a click leaves them.
    export void restore(const Dom::Section& appConfig);

    // One command per choice. An action carries no notion of a group, so what makes these
    // exclusive is the state each answers with: the one naming the answer in force reports
    // itself selected, and moving the answer asks every one of them again. A presenter takes
    // its words from the action, so the words are stated here and nowhere else.
    export extern Action onlyDifferences;
    export extern Action full;
    export extern Action asClassDeclarations;
    export extern Action asClassMethod;
    export extern Action asOutsideClass;
}
