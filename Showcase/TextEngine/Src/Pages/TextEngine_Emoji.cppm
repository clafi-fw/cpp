export module ClaFi.Showcase.TextEngine.Emoji;

import ClaFi.Core.TextEngine.Text;

namespace ClaFi::Showcase::TextEngine
{
    // A page of emoji, set in one font family after another.
    //
    // Emoji ask the text path questions nothing else in the showcase asks. A character above the
    // basic plane is two wchar_t on Windows and one on Linux, so a caret stepping by units crosses
    // it differently on each. A family named for its emoji holds glyphs no text family has, so what
    // draws is whatever the platform reached for rather than the family the run states. A picture
    // that is one emoji can be seven code points joined by zero-width joiners, which the shaper
    // either ligates or draws as its parts. And an emoji face carries its colour in layers or in
    // bitmap strikes, neither of which a run drawn with one brush can put on the screen.
    //
    // WHAT THE PAGE ANSWERS WITH IS THE STATE OF THE MACHINE. Families for both platforms are
    // named, so a row naming one this machine does not have is part of the reading: it shows what a
    // name nothing matches falls to. The counts beside a sequence are taken from the literal
    // itself, so they are this build's numbers rather than a claim about wchar_t.
    //
    // Editable, like the syntax page. The sequences are here to be walked through with the arrow
    // keys, and the caret readout in the corner is what the walk is read from.
    export [[nodiscard]] Text buildEmojiShowcase();
}
