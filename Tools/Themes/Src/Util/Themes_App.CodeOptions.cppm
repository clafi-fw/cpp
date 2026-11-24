export module Themes_App.CodeOptions;

import Themes_App.ThemeToCppCode;

import ClaFi.Core.Foundation;
import ClaFi.Dom;

namespace Themes_App::CodeOptions
{
    using namespace ClaFi;

    export [[nodiscard]] CodeContent content();

    export void restore(const Dom::Section& appConfig);

    // One command per choice. An action carries no notion of a group, so what makes these
    // exclusive is the state each answers with: the one naming the answer in force reports
    // itself selected, and moving the answer asks every one of them again. A presenter takes
    // its words from the action, so the words are stated here and nowhere else.
    export extern Action onlyDifferences;
    export extern Action full;
}
