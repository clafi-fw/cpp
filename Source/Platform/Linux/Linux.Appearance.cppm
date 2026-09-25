export module ClaFi.Platform.Linux.Appearance;

import ClaFi.Core.System.UiTypes;

namespace ClaFi::PlatformImplementation::Linux
{
    // Who is told that the desktop's colour mode has moved - one listener, set once. See Platform
    export using AppearanceHandler = void (*)();

    // The mode the desktop asks applications to be drawn in. See Platform
    export [[nodiscard]] ColorMode desktopColorMode();

    // Names who is told when the answer desktopColorMode gives has moved.
    export void setAppearanceHandler(AppearanceHandler);

    // The session bus socket the desktop answers on while a connection stands, and -1 otherwise.
    export [[nodiscard]] int appearanceFd();

    // Reads what the bus has sent, telling the handler where the mode has moved.
    export void dispatchAppearance();
}
