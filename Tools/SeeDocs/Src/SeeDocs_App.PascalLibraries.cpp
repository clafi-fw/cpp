module SeeDocs_App.PascalLibraries;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        using ClassNames = std::span<const std::wstring_view>;

        constexpr std::wstring_view k_controlRoot = L"TControl";
        constexpr std::wstring_view k_fireMonkeyPrefix = L"FMX.";

        // Every class of the VCL and the LCL that descends from TControl. See README
        constexpr auto k_vclControls = std::to_array<std::wstring_view>({
            L"TAbstractGroupedEdit", L"TActionMainMenuBar", L"TActionToolBar", L"TActiveForm",
            L"TActivityIndicator", L"TAnimate", L"TArrow", L"TBaseNavigator", L"TBasePickerControl",
            L"TBevel", L"TBindNavigator", L"TBitBtn", L"TBoundLabel", L"TBoundLabelEx", L"TButton",
            L"TButtonCellEditor", L"TButtonControl", L"TButtonedEdit", L"TButtonGroup",
            L"TButtonPanel", L"TCalcEdit", L"TCalculatorForm", L"TCalculatorPanel", L"TCalendar",
            L"TCalendarPicker", L"TCalendarPopupForm", L"TCalendarView", L"TCard", L"TCardPanel",
            L"TCategoryButtons", L"TCategoryPanel", L"TCategoryPanelGroup",
            L"TCategoryPanelSurface", L"TCDButton", L"TCDButtonControl", L"TCDCheckBox",
            L"TCDCheckListBox", L"TCDComboBox", L"TCDControl", L"TCDCustomTabControl", L"TCDEdit",
            L"TCDGroupBox", L"TCDListBox", L"TCDListView", L"TCDPageControl", L"TCDPanel",
            L"TCDPositionedControl", L"TCDProgressBar", L"TCDRadioButton", L"TCDScrollableControl",
            L"TCDScrollBar", L"TCDSpinEdit", L"TCDStaticText", L"TCDStatusBar", L"TCDTabControl",
            L"TCDTabSheet", L"TCDToggleBox", L"TCDToolBar", L"TCDTrackBar", L"TCheckBox",
            L"TCheckComboBox", L"TCheckGroup", L"TCheckListBox", L"TColorBox", L"TColorButton",
            L"TColorGrid", L"TColorListBox", L"TComboBox", L"TComboBoxEx", L"TComboButton",
            L"TCommonCalendar", L"TCompositeCellEditor", L"TControl", L"TControlBar",
            L"TControlList", L"TControlListButton", L"TControlListCheckBox", L"TControlListControl",
            L"TControlListItemPanel", L"TControlListRadioButton", L"TCoolBar",
            L"TCustomAbstractGroupedEdit", L"TCustomActionBar", L"TCustomActionCombo",
            L"TCustomActionControl", L"TCustomActionControlBar", L"TCustomActionDockBar",
            L"TCustomActionMainMenuBar", L"TCustomActionMenuBar", L"TCustomActionPopupMenu",
            L"TCustomActionToolBar", L"TCustomActiveForm", L"TCustomActivityIndicator",
            L"TCustomAddRemoveItem", L"TCustomBindNavigator", L"TCustomBitBtn", L"TCustomButton",
            L"TCustomButtonControl", L"TCustomButtonedEdit", L"TCustomButtonPanel",
            L"TCustomCalendar", L"TCustomCalendarPicker", L"TCustomCalendarView",
            L"TCustomCardPanel", L"TCustomCategoryPanel", L"TCustomCategoryPanelGroup",
            L"TCustomCheckBox", L"TCustomCheckCombo", L"TCustomCheckGroup", L"TCustomCheckListBox",
            L"TCustomColorBox", L"TCustomColorListBox", L"TCustomCombo", L"TCustomComboBox",
            L"TCustomComboBoxEx", L"TCustomComboControl", L"TCustomControl", L"TCustomControlBar",
            L"TCustomControlFilterEdit", L"TCustomControlList", L"TCustomCoolBar",
            L"TCustomCopyToClipboardDialog", L"TCustomDatePicker", L"TCustomDBComboBox",
            L"TCustomDBGrid", L"TCustomDBListBox", L"TCustomDBLookupComboBox",
            L"TCustomDesignControl", L"TCustomDockForm", L"TCustomDrawGrid",
            L"TCustomDropDownButton", L"TCustomEdgeBrowser", L"TCustomEdit", L"TCustomEditButton",
            L"TCustomFileListBox", L"TCustomFilterComboBox", L"TCustomFloatSpinEdit",
            L"TCustomFlowPanel", L"TCustomForm", L"TCustomFrame", L"TCustomGestureListView",
            L"TCustomGesturePreview", L"TCustomGestureRecorder", L"TCustomGrid",
            L"TCustomGridPanel", L"TCustomGroupBox", L"TCustomHeaderControl", L"TCustomHintWindow",
            L"TCustomHotKey", L"TCustomImage", L"TCustomizeActionToolBar", L"TCustomLabel",
            L"TCustomLabeledEdit", L"TCustomLinkLabel", L"TCustomListBox", L"TCustomListControl",
            L"TCustomListView", L"TCustomListViewEditor", L"TCustomMaskEdit",
            L"TCustomMDIMenuButton", L"TCustomMemo", L"TCustomMenuButton", L"TCustomMenuExpandBtn",
            L"TCustomMenuItem", L"TCustomMultiSelectListControl", L"TCustomNumberBox",
            L"TCustomOutline", L"TCustomPage", L"TCustomPairSplitter", L"TCustomPanel",
            L"TCustomPanningWindow", L"TCustomProgressBar", L"TCustomRadioGroup",
            L"TCustomRelativePanel", L"TCustomRibbon", L"TCustomRibbonComboBox",
            L"TCustomRibbonGalleryBar", L"TCustomRibbonGalleryPopupBar", L"TCustomRibbonGroup",
            L"TCustomRibbonPage", L"TCustomRibbonQuickAccessPopupToolbar",
            L"TCustomRibbonQuickAccessToolbar", L"TCustomRichEdit", L"TCustomRubberBand",
            L"TCustomScrollBar", L"TCustomShape", L"TCustomShellListView", L"TCustomShellTreeView",
            L"TCustomSpeedButton", L"TCustomSpinEdit", L"TCustomSplitter", L"TCustomSplitView",
            L"TCustomStackPanel", L"TCustomStaticText", L"TCustomStatusBar", L"TCustomStringGrid",
            L"TCustomTabControl", L"TCustomTimePicker", L"TCustomToggleSwitch",
            L"TCustomToolScrollBtn", L"TCustomTouchKeyboard", L"TCustomTrackBar",
            L"TCustomTransparentControl", L"TCustomTreeView", L"TCustomUpDown",
            L"TCustomUtilityButton", L"TCustomVirtualImage", L"TDateEdit", L"TDatePicker",
            L"TDateTimePicker", L"TDBBoundLabel", L"TDBCalendar", L"TDBCheckBox", L"TDBComboBox",
            L"TDBCtrlGrid", L"TDBCtrlPanel", L"TDBCustomNavigator", L"TDBDateEdit", L"TDBEdit",
            L"TDBGrid", L"TDBGridInplaceEdit", L"TDBGroupBox", L"TDBImage", L"TDBLabeledEdit",
            L"TDBListBox", L"TDBLookupCombo", L"TDBLookupComboBox", L"TDBLookupControl",
            L"TDBLookupList", L"TDBLookupListBox", L"TDBMemo", L"TDBNavButton",
            L"TDBNavFocusableButton", L"TDBNavigator", L"TDBOleControl", L"TDBRadioGroup",
            L"TDBRichEdit", L"TDBText", L"TDirectoryEdit", L"TDirectoryListBox",
            L"TDirectoryOutline", L"TDockTabSet", L"TDrawGrid", L"TDriveComboBox", L"TEdgeBrowser",
            L"TEdit", L"TEditSpeedButton", L"TFileListBox", L"TFileNameEdit", L"TFilterComboBox",
            L"TFindDialogForm", L"TFloatSpinEdit", L"TFlowPanel", L"TForm", L"TFrame", L"TGauge",
            L"TGestureListView", L"TGesturePreview", L"TGestureRecorder", L"TGlyph",
            L"TGraphicControl", L"TGridPanel", L"TGroupBox", L"THeader", L"THeaderControl",
            L"THelpButton", L"THintWindow", L"THintWindowRendered", L"THotKey", L"TImage",
            L"TIndustrialBase", L"TInplaceEdit", L"TInplaceEditList", L"TLabel", L"TLabeledEdit",
            L"TLazarusFileDialogForm", L"TLazDockForm", L"TLazDockPage", L"TLazDockPages",
            L"TLazDockSplitter", L"TLazMessageDialog", L"TLCLTaskDialog", L"TLinkLabel",
            L"TListBox", L"TListView", L"TLoginDialog", L"TMaskEdit", L"TMediaPlayer", L"TMemo",
            L"TMenuResizeActionControl", L"TMonthCalendar", L"TNavButton", L"TNotebook",
            L"TNoteBookStringsTabControl", L"TNotifierForm", L"TNotifierXButton", L"TNumberBox",
            L"TOffice2007Button", L"TOleContainer", L"TOleControl", L"TOutline", L"TPage",
            L"TPageControl", L"TPageScroller", L"TPaintBox", L"TPairSplitter", L"TPairSplitterSide",
            L"TPanel", L"TPanelBitBtn", L"TPickerPopupControl", L"TPickListCellEditor",
            L"TPopupCalendarView", L"TPopupDataList", L"TPopupGrid", L"TPreviewFileControl",
            L"TProgressBar", L"TPropertyPage", L"TRadioButton", L"TRadioGroup", L"TReflectorWindow",
            L"TRelativePanel", L"TReplaceDialogForm", L"TRibbon", L"TRibbonActionPopupMenu",
            L"TRibbonAddRemoveItem", L"TRibbonApplicationButton", L"TRibbonApplicationMenuBar",
            L"TRibbonApplicationMenuButton", L"TRibbonApplicationMenuItem",
            L"TRibbonApplicationPopupMenu", L"TRibbonBaseButtonControl", L"TRibbonCheckBoxControl",
            L"TRibbonComboBox", L"TRibbonComboBoxItem", L"TRibbonComboControl",
            L"TRibbonCustomControl", L"TRibbonCustomizeItem", L"TRibbonCustomStatusControl",
            L"TRibbonDropDownButton", L"TRibbonGallery", L"TRibbonGalleryBar",
            L"TRibbonGalleryControl", L"TRibbonGalleryControlMenuItem", L"TRibbonGalleryItem",
            L"TRibbonGalleryMenuItem", L"TRibbonGalleryPopupBar", L"TRibbonGallerySplitButton",
            L"TRibbonGripItem", L"TRibbonGroup", L"TRibbonGroupCollapsedControl",
            L"TRibbonGroupDockForm", L"TRibbonGroupDropDown", L"TRibbonGroupScrollButton",
            L"TRibbonLargeButtonControl", L"TRibbonLargeSplitButton", L"TRibbonMainMenuItem",
            L"TRibbonMainMenuSeparator", L"TRibbonMenuItem", L"TRibbonMenuSeparatorItem",
            L"TRibbonMenuSmallButtonControl", L"TRibbonOptionButtonControl", L"TRibbonPage",
            L"TRibbonPopupGalleryBar", L"TRibbonQATCustomizeMenu", L"TRibbonQATScrollButton",
            L"TRibbonQuickAccessPopupToolbar", L"TRibbonQuickAccessToolbar",
            L"TRibbonRadioButtonControl", L"TRibbonRecentButtonControl", L"TRibbonRecentDocuments",
            L"TRibbonRichContentItem", L"TRibbonRichContentMainMenuItem", L"TRibbonSeparator",
            L"TRibbonSmallButtonControl", L"TRibbonSmallSplitButton", L"TRibbonSpinButton",
            L"TRibbonSpinEdit", L"TRibbonSplitMenuItem", L"TRibbonStyleMainPopupMenu",
            L"TRibbonStyleMenuButton", L"TRibbonStylePopupMenu", L"TRibbonSystemButton",
            L"TRibbonTextControl", L"TRibbonToolScrollButton", L"TRichEdit", L"TRubberBand",
            L"TScrollBar", L"TScrollBox", L"TScroller", L"TScrollingWinControl", L"TScrollWindow",
            L"TSearchBox", L"TShape", L"TShellListView", L"TShellTreeView", L"TSpeedButton",
            L"TSpinButton", L"TSpinEdit", L"TSplitter", L"TSplitView", L"TStackPanel",
            L"TStandardAddRemoveItem", L"TStandardButtonControl", L"TStandardCustomizePopup",
            L"TStandardDropDownButton", L"TStandardMenuButton", L"TStandardMenuExpandBtn",
            L"TStandardMenuItem", L"TStandardMenuPopup", L"TStandardToolScrollBtn", L"TStaticText",
            L"TStatusBar", L"TStringCellEditor", L"TStringGrid", L"TTabControl", L"TTabDockPanel",
            L"TTabSet", L"TTabSheet", L"TTimeEdit", L"TTimePicker", L"TTimePopupForm",
            L"TTimerSpeedButton", L"TToggleBox", L"TToggleSwitch", L"TToolBar", L"TToolButton",
            L"TToolDockForm", L"TToolWindow", L"TTouchKeyboard", L"TTrackBar", L"TTreeView",
            L"TUpDown", L"TValueListEditor", L"TVirtualImage", L"TWinControl",
            L"TXPStyleAddRemoveItem", L"TXPStylePopupMenu", L"TXToolDockForm"
        });

        // Every class of FMX that descends from TControl. See README
        constexpr auto k_fireMonkeyControls = std::to_array<std::wstring_view>({
            L"TActiveMaskedImage", L"TActiveOpacityObject", L"TActiveStyleObject",
            L"TActiveStyleTextObject", L"TAdapterListView", L"TAlphaTrackBar", L"TAniIndicator",
            L"TAppearanceListView", L"TArc", L"TArcDial", L"TBannerAd", L"TBitmapSwitchObject",
            L"TBitmapTrackBar", L"TBufferedLayout", L"TButton", L"TButtonStyleObject",
            L"TButtonStyleTextObject", L"TBWTrackBar", L"TCalendar", L"TCalloutPanel",
            L"TCalloutRectangle", L"TCaretRectangle", L"TCellText", L"TCheckBox", L"TCheckCell",
            L"TCheckColumn", L"TCheckStyleObject", L"TCircle", L"TClearEditButton",
            L"TClearingEdit", L"TColorBox", L"TColorButton", L"TColorComboBox", L"TColorListBox",
            L"TColorPanel", L"TColorPicker", L"TColorQuad", L"TColumn", L"TComboBox",
            L"TComboColorBox", L"TComboEdit", L"TComboEditBase", L"TComboEditListBox",
            L"TComboListBox", L"TComboTrackBar", L"TContent", L"TContentEdit", L"TContextMenuItem",
            L"TControl", L"TCornerButton", L"TCurrencyCell", L"TCurrencyColumn", L"TCustomBannerAd",
            L"TCustomBufferedLayout", L"TCustomButton", L"TCustomCalendar", L"TCustomColorComboBox",
            L"TCustomComboBox", L"TCustomComboEdit", L"TCustomCornerButton", L"TCustomDateEdit",
            L"TCustomDateTimeEdit", L"TCustomEdit", L"TCustomEditBox", L"TCustomGrid",
            L"TCustomListBox", L"TCustomListView", L"TCustomMagnifierGlass", L"TCustomMapView",
            L"TCustomMemo", L"TCustomMultiView", L"TCustomNumberColumn", L"TCustomPath",
            L"TCustomPopover", L"TCustomPresentedFramedScrollBox",
            L"TCustomPresentedFramedVertScrollBox", L"TCustomPresentedHorzScrollBox",
            L"TCustomPresentedScrollBox", L"TCustomPresentedVertScrollBox", L"TCustomScrollBox",
            L"TCustomStyleObject", L"TCustomSwitch", L"TCustomSwitchObject", L"TCustomTimeEdit",
            L"TCustomTouchInterceptingLayout", L"TCustomTrack", L"TCustomTreeView",
            L"TCustomWebBrowser", L"TDateCell", L"TDateColumn", L"TDateEdit", L"TDateTimeColumn",
            L"TDateTimeColumnBase", L"TDayItem", L"TDefaultEditor", L"TDropDownEditButton",
            L"TDropTarget", L"TEdgeWebBrowser", L"TEdit", L"TEditButton", L"TEllipse",
            L"TEllipsesEditButton", L"TExpander", L"TExpanderButton", L"TFixedStyledComboEdit",
            L"TFixedStyledEdit", L"TFixedStyledEditBox", L"TFixedStyledMemo",
            L"TFixedStyledNumberBox", L"TFixedStyledSearchBox", L"TFixedStyledSpinBox",
            L"TFloatColumn", L"TFlowLayout", L"TFlowLayoutBreak", L"TFrame", L"TFramedScrollBox",
            L"TFramedVertScrollBox", L"TGlyph", L"TGlyphColumn", L"TGradientEdit", L"TGrid",
            L"TGridHeaderItem", L"TGridLayout", L"TGridPanelLayout", L"TGroupBox", L"THeader",
            L"THeaderItem", L"THorzScrollBox", L"THueTrackBar", L"TImage", L"TImageCell",
            L"TImageColumn", L"TImageControl", L"TImageViewer", L"TIntegerColumn", L"TLabel",
            L"TLastItem", L"TLayout", L"TLine", L"TListBox", L"TListBoxGroupFooter",
            L"TListBoxGroupHeader", L"TListBoxHeader", L"TListBoxItem", L"TListBoxSeparatorItem",
            L"TListView", L"TListViewBase", L"TMagnifierGlass", L"TMapView", L"TMaskedImage",
            L"TMediaPlayerControl", L"TMemo", L"TMenuBar", L"TMenuItem",
            L"TMetropolisUIListBoxItem", L"TMultiView", L"TNativeStyledControl", L"TNumberBox",
            L"TNumberCell", L"TOLEFrameworkDelegate", L"TPaintBox", L"TPanel",
            L"TPasswordEditButton", L"TPath", L"TPathLabel", L"TPie", L"TPlatformEdit",
            L"TPlotGrid", L"TPopup", L"TPopupBox", L"TPopupCell", L"TPopupColumn",
            L"TPopupDateTimePicker", L"TPopupListPicker", L"TPopupPicker", L"TPresentedControl",
            L"TPresentedFramedScrollBox", L"TPresentedFramedVertScrollBox",
            L"TPresentedHorzScrollBox", L"TPresentedListView", L"TPresentedScrollBox",
            L"TPresentedTextControl", L"TPresentedVertScrollBox", L"TProgressBar", L"TProgressCell",
            L"TProgressColumn", L"TProxyMenuItem", L"TRadioButton", L"TRectangle", L"TRoundRect",
            L"TScaledLayout", L"TScrollBar", L"TScrollBox", L"TScrollContent", L"TSearchBox",
            L"TSearchEditButton", L"TSelection", L"TSelectionPoint", L"TSeparatorLine", L"TShape",
            L"TSizeGrip", L"TSmallScrollBar", L"TSpeedButton", L"TSpinBox", L"TSpinEditButton",
            L"TSplitter", L"TStatusBar", L"TStringColumn", L"TStringGrid", L"TStyleContainer",
            L"TStyledCalendar", L"TStyledComboEdit", L"TStyledComboEditBase", L"TStyledControl",
            L"TStyledCustomScrollBox", L"TStyledEdit", L"TStyledEditBox", L"TStyledEditContent",
            L"TStyledGrid", L"TStyledMemo", L"TStyledNumberBox", L"TStyledPresentation",
            L"TStyledSearchBox", L"TStyledSpinBox", L"TStyledSwitch",
            L"TStyledTouchInterceptingLayout", L"TStyleObject", L"TStyleTextObject", L"TSubImage",
            L"TSwitch", L"TSwitchObject", L"TSwitchTextObject", L"TSystemButtonObject",
            L"TTabControl", L"TTabItem", L"TTabStyleObject", L"TTabStyleTextObject", L"TText",
            L"TTextControl", L"TThumb", L"TTimeCell", L"TTimeColumn", L"TTimeEdit",
            L"TTintedButtonStyleObject", L"TTintedStyleObject", L"TToolBar",
            L"TTouchInterceptingLayout", L"TTrack", L"TTrackBar", L"TTreeView", L"TTreeViewItem",
            L"TVertScrollBox", L"TViewport3D", L"TWebBrowser", L"TWinControl",
            L"TWindowsWebBrowserOLEFrameworkDelegate", L"TWinNativeStyledControl"
        });

        // Whether two names are one, the way the language reads them: in any case.
        [[nodiscard]] bool sameName(const std::wstring_view left, const std::wstring_view right)
        {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i != left.size(); ++i)
            {
                if (std::towlower(left[i]) != std::towlower(right[i]))
                    return false;
            }
            return true;
        }
    }

    bool isFireMonkeyUnit(const std::wstring_view unit)
    {
        return unit.size() > k_fireMonkeyPrefix.size()
            && sameName(unit.substr(0, k_fireMonkeyPrefix.size()), k_fireMonkeyPrefix);
    }

    bool isControlRoot(const std::wstring_view name)
    {
        return sameName(name, k_controlRoot);
    }

    bool isLibraryControl(const std::wstring_view name, const ClassLibrary library)
    {
        const ClassNames controls = library == ClassLibrary::FireMonkey
            ? ClassNames{ k_fireMonkeyControls }
            : ClassNames{ k_vclControls };
        return std::ranges::any_of(controls, [name](const std::wstring_view listed){
            return sameName(listed, name);
        });
    }
}
