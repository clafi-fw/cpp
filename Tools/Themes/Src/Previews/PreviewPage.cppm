export module Themes_App.PreviewPage;

import ClaFi.StdLib;

namespace Themes_App
{
    // A page of the preview panel, shared by every node of the Design tree that names it.
    export enum class PreviewPage
    {
        Overview,       // a passage and a few controls, for a node with no control of its own
        Windows,        // a dialog, a menu and a tooltip
        Surfaces,       // a tool bar, a heading and two sections
        Grid,           // a grid with one group open and one shut
        Inputs,         // buttons, tool buttons, a slider and a text box
        Selection,      // check boxes, radio buttons, tiles and a search through a passage
        Tabs,           // tabs across the top and down the left
        Scrolling,      // a box that scrolls both ways
        Callouts,       // a callout of each kind
        DoDont,         // a Do and Don't block
        CodeBlock,      // a block of code
        TestSubjects,   // Testee and Bestee
        Count
    };

    export constexpr std::size_t k_previewPageCount{ static_cast<std::size_t>(PreviewPage::Count) };

    // What the preview panel shows for a node of the Design tree.
    export struct NodePreview
    {
        PreviewPage page{ PreviewPage::Overview };
        bool editsSizes{ false }; // a set of sizes, or a folder holding nothing else
    };
}
