#include <gtkmm/actiongroup.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/image.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menutoolbutton.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>

#include "../structures/timefrequencydata.h"

class RFIGuiMenu
{
public:
	RFIGuiMenu();
	
	Gtk::MenuBar& Menu() { return _menuBar; }
	Gtk::Toolbar& Toolbar() { return _toolbar; }
	
	// File
	sigc::signal<void> OnActionDirectoryOpen;
	sigc::signal<void> OnActionFileOpen;
	sigc::signal<void> OnActionDirectoryOpenForSpatial;
	sigc::signal<void> OnActionDirectoryOpenForST;
	sigc::signal<void> OnSaveBaseline;
	sigc::signal<void> OnQuit;
	
	// View
	sigc::signal<void> OnImagePropertiesPressed;
	sigc::signal<void> OnTimeGraphButtonPressed;
	sigc::signal<void> OnToggleFlags;
	sigc::signal<void> OnHightlightPressed;	
	sigc::signal<void> OnZoomFit;
	sigc::signal<void> OnZoomIn;
	sigc::signal<void> OnZoomOut;
	sigc::signal<void> OnShowImagePlane;
	sigc::signal<void> OnSetAndShowImagePlane;
	sigc::signal<void> OnAddToImagePlane;
	sigc::signal<void> OnShowStats;

	// Plot
	sigc::signal<void> OnPlotPowerSpectrumComparisonPressed;
	sigc::signal<void> OnPlotPowerTimeComparisonPressed;
	sigc::signal<void> OnPlotTimeScatterComparisonPressed;
	sigc::signal<void> OnPlotDistPressed;
	sigc::signal<void> OnPlotLogLogDistPressed;
	sigc::signal<void> OnPlotComplexPlanePressed;
	sigc::signal<void> OnPlotMeanSpectrumPressed;
	sigc::signal<void> OnPlotSumSpectrumPressed;
	sigc::signal<void> OnPlotPowerSpectrumPressed;
	sigc::signal<void> OnPlotFrequencyScatterPressed;
	sigc::signal<void> OnPlotPowerRMSPressed;
	sigc::signal<void> OnPlotPowerTimePressed;
	sigc::signal<void> OnPlotTimeScatterPressed;
	sigc::signal<void> OnPlotSingularValuesPressed;
	
	// Browse
	sigc::signal<void> OnLoadPrevious;
	sigc::signal<void> OnReloadPressed;
	sigc::signal<void> OnLoadNext;
	sigc::signal<void> OnGoToPressed;
	sigc::signal<void> OnLoadLongestBaselinePressed;
	sigc::signal<void> OnLoadShortestBaselinePressed;
	
	// Simulate
	sigc::signal<void> OnGaussianTestSets;
	sigc::signal<void> OnRayleighTestSets;
	sigc::signal<void> OnZeroTestSets;
	
	sigc::signal<void> OnOpenTestSetA;
	sigc::signal<void> OnOpenTestSetB;
	sigc::signal<void> OnOpenTestSetC;
	sigc::signal<void> OnOpenTestSetD;
	sigc::signal<void> OnOpenTestSetE;
	sigc::signal<void> OnOpenTestSetF;
	sigc::signal<void> OnOpenTestSetG;
	sigc::signal<void> OnOpenTestSetH;
	sigc::signal<void> OnOpenTestSetNoise;
	sigc::signal<void> OnOpenTestSet3Model;
	sigc::signal<void> OnOpenTestSet5Model;
	sigc::signal<void> OnOpenTestSetNoise3Model;
	sigc::signal<void> OnOpenTestSetNoise5Model;
	sigc::signal<void> OnOpenTestSetBStrong;
	sigc::signal<void> OnOpenTestSetBWeak;
	sigc::signal<void> OnOpenTestSetBAligned;
	
	sigc::signal<void> OnOpenTestSetGaussianBroadband;
	sigc::signal<void> OnOpenTestSetSinusoidalBroadband;
	sigc::signal<void> OnOpenTestSetSlewedGaussianBroadband;
	sigc::signal<void> OnOpenTestSetBurstBroadband;
	sigc::signal<void> OnOpenTestSetRFIDistributionLow;
	sigc::signal<void> OnOpenTestSetRFIDistributionMid;
	sigc::signal<void> OnOpenTestSetRFIDistributionHigh;
	
	sigc::signal<void> OnAddStaticFringe;
	sigc::signal<void> OnAdd1SigmaFringe;
	sigc::signal<void> OnSetToOne;
	sigc::signal<void> OnSetToI;
	sigc::signal<void> OnSetToOnePlusI;
	sigc::signal<void> OnAddCorrelatorFault;
	sigc::signal<void> OnMultiplyData;
	
	sigc::signal<void> OnSimulateCorrelation;
	sigc::signal<void> OnSimulateSourceSetA;
	sigc::signal<void> OnSimulateSourceSetB;
	sigc::signal<void> OnSimulateSourceSetC;
	sigc::signal<void> OnSimulateSourceSetD;
	sigc::signal<void> OnSimulateOffAxisSource;
	sigc::signal<void> OnSimulateOnAxisSource;
	
	// Data
	sigc::signal<void> OnVisualizedToOriginalPressed;
	sigc::signal<void> OnKeepRealPressed;
	sigc::signal<void> OnKeepImaginaryPressed;
	sigc::signal<void> OnKeepPhasePressed;
	sigc::signal<void> OnUnrollPhaseButtonPressed;
	
	sigc::signal<void> OnKeepStokesIPressed;
	sigc::signal<void> OnKeepStokesQPressed;
	sigc::signal<void> OnKeepStokesUPressed;
	sigc::signal<void> OnKeepStokesVPressed;
	sigc::signal<void> OnKeepRRPressed;
	sigc::signal<void> OnKeepRLPressed;
	sigc::signal<void> OnKeepLRPressed;
	sigc::signal<void> OnKeepLLPressed;
	sigc::signal<void> OnKeepXXPressed;
	sigc::signal<void> OnKeepXYPressed;
	sigc::signal<void> OnKeepYXPressed;
	sigc::signal<void> OnKeepYYPressed;
	
	sigc::signal<void> OnStoreData;
	sigc::signal<void> OnRecallData;
	sigc::signal<void> OnSubtractDataFromMem;
	sigc::signal<void> OnClearOriginalFlagsPressed;
	sigc::signal<void> OnClearAltFlagsPressed;
	
	// Actions
	sigc::signal<void> OnEditStrategyPressed;
	sigc::signal<void> OnExecuteStrategyPressed;
	sigc::signal<void> OnExecutePythonStrategy;
	sigc::signal<void> OnExecuteLuaStrategy;
	
	sigc::signal<void> OnSegment;
	sigc::signal<void> OnCluster;
	sigc::signal<void> OnClassify;
	sigc::signal<void> OnRemoveSmallSegments;
	
	sigc::signal<void> OnInterpolateFlagged;
	sigc::signal<void> OnVertEVD;
	sigc::signal<void> OnApplyTimeProfile;
	sigc::signal<void> OnApplyVertProfile;
	
	sigc::signal<void, bool> OnUseTimeProfile;
	sigc::signal<void, bool> OnUseVertProfile;
	
	// Help
	sigc::signal<void> OnHelpAbout;
	
	// Toolbar signals (some are already covered)
	sigc::signal<void> OnTogglePolarizations;
	sigc::signal<void> OnToggleImage;
	sigc::signal<void> OnSelectImage;
	
	sigc::signal<void, unsigned> openTestSet;
	
	bool OriginalFlagsActive() const { return _tbOriginalFlags.get_active(); }
	bool AlternativeFlagsActive() const { return _tbAlternativeFlags.get_active(); }
	
	void SetOriginalFlagsActive(bool originalFlags) {
		_tbOriginalFlags.set_active(originalFlags);
	}
	void SetAlternativeFlagsActive(bool alternativeFlags) {
		_tbAlternativeFlags.set_active(alternativeFlags);
	}
	
	bool ShowPPActive() const { return _tbDisplayPP.get_active(); }
	bool ShowPQActive() const { return _tbDisplayPQ.get_active(); }
	bool ShowQPActive() const { return _tbDisplayQP.get_active(); }
	bool ShowQQActive() const { return _tbDisplayQQ.get_active(); }
	
	void SetShowPPActive(bool active) { _tbDisplayPP.set_active(active); }
	void SetShowPQActive(bool active) { _tbDisplayPQ.set_active(active); }
	void SetShowQPActive(bool active) { _tbDisplayQP.set_active(active); }
	void SetShowQQActive(bool active) { _tbDisplayQQ.set_active(active); }
	
	void SetShowPPSensitive(bool sensitive) { _tbDisplayPP.set_sensitive(sensitive); }
	void SetShowPQSensitive(bool sensitive) { _tbDisplayPQ.set_sensitive(sensitive); }
	void SetShowQPSensitive(bool sensitive) { _tbDisplayQP.set_sensitive(sensitive); }
	void SetShowQQSensitive(bool sensitive) { _tbDisplayQQ.set_sensitive(sensitive); }
	
	bool CloseExecuteFrame() const { return _miActionsCloseExecuteFrame.get_active(); }
	
	bool TimeGraphActive() const { return _miViewTimeGraph.get_active(); }
	
	bool SimulateNCPActive() const { return _miSimNCP.get_active(); }
	bool SimulateB1834Active() const { return _miSimB1834.get_active(); }
	
	bool Simulate16ChActive() const { return _miSim16channels.get_active(); }
	bool Simulate64ChActive() const { return _miSim64channels.get_active(); }
	bool SimFixBandwidthActive() const { return _miSimFixBandwidth.get_active(); }
	
	void SetZoomToFitSensitive(bool sensitive) { _tbZoomFit.set_sensitive(sensitive); }
	void SetZoomOutSensitive(bool sensitive) { _tbZoomOut.set_sensitive(sensitive); }
	void SetZoomInSensitive(bool sensitive) { _tbZoomIn.set_sensitive(sensitive); }
	
	void SetPreviousSensitive(bool sensitive) { _tbPrevious.set_sensitive(sensitive); }
	void SetNextSensitive(bool sensitive) { _tbNext.set_sensitive(sensitive); }
	void SetReloadSensitive(bool sensitive) { _tbReload.set_sensitive(sensitive); }
	
	void SetSelectVisualizationSensitive(bool sensitive) { _tbSelectVisualization.set_sensitive(sensitive); }
	
	Gtk::Menu& VisualizationMenu() { return _tfVisualizationMenu; }
	
	void BlockVisualizationSignals() { _blockVisualizationSignals = true; }
	void UnblockVisualizationSignals() { _blockVisualizationSignals = false; }
	
private:
	struct ImgMenuItem {
		Gtk::ImageMenuItem item;
		Gtk::HBox box;
		Gtk::AccelLabel label;
		Gtk::Image image;
	};
	
	void topMenu(Gtk::Menu& menu, Gtk::MenuItem& item, const char* label)
	{
		item.set_submenu(menu);
		item.set_label(label);
		item.set_use_underline(true);
		_menuBar.append(item);
	}
	
	void addItem(Gtk::Menu& menu, Gtk::SeparatorMenuItem& sep)
	{
		menu.append(sep);
	}
	
	void addItem(Gtk::Menu& menu, Gtk::MenuItem& item, const char* label)
	{
		item.set_label(label);
		item.set_use_underline(true);
		menu.append(item);
	}
	
	template<typename SigType>
	void addItem(Gtk::Menu& menu, Gtk::MenuItem& item, const SigType& sig, const char* label)
	{
		item.set_label(label);
		item.set_use_underline(true);
		item.signal_activate().connect(sig);
		menu.append(item);
	}
	
	template<typename SigType>
	void addItem(Gtk::Menu& menu, ImgMenuItem& item, const SigType& sig, const char* label, const char* icon)
	{
		
		//item.box.set_halign(Gtk::ALIGN_START);
		item.image.set_from_icon_name(icon, Gtk::BuiltinIconSize::ICON_SIZE_MENU);
		//item.box.pack_start(item.image);
		//item.label.set_label(label);
		//item.box.pack_start(item.label);
		item.item.set_label(label);
		item.item.set_use_underline(true);
		item.item.set_image(item.image);
		item.item.signal_activate().connect(sig);
		//item.item.add(item.box);
		menu.append(item.item);
	}
	
	template<typename SigType>
	void addTool(Gtk::ToolButton& tool, const SigType& sig, const char* label, const char* tooltip, const char* icon)
	{
		tool.set_label(label);
		tool.set_tooltip_text(tooltip);
		tool.set_icon_name(icon);
		tool.signal_clicked().connect(sig);
		_toolbar.append(tool);
	}
	
	void tooltip(ImgMenuItem& item, const char* tooltipStr) { }
	
	void makeFileMenu();
	void makeViewMenu();
	void makePlotMenu();
	void makeBrowseMenu();
	void makeSimulateMenu();
	void makeDataMenu();
	void makeActionsMenu();
	void makeHelpMenu();
	void makeToolbarActions();
	
	Gtk::MenuBar _menuBar;
	Gtk::Toolbar _toolbar;
	
	Gtk::Menu
		_menuFile, _menuView, _menuPlot, _menuBrowse,
		_menuSimulate, _menuData, _menuActions, _menuHelp;
	Gtk::MenuItem
		_miFile, _miView, _miPlot, _miBrowse,
		_miSimulate, _miData, _miActions, _miHelp;
		
	// File menu
	ImgMenuItem _miFileOpenDir, _miFileOpen;
	Gtk::MenuItem _miFileOpenSpatial, _miFileOpenST, _miFileSaveBaseline;
	ImgMenuItem _miFileQuit;
	
	// View menu
	Gtk::MenuItem _miViewProperties;
	Gtk::CheckMenuItem _miViewTimeGraph;
	ImgMenuItem _miViewOriginalFlags, _miViewAlternativeFlags;
	Gtk::MenuItem _miViewHighlight;
	Gtk::SeparatorMenuItem _miViewSep1, _miViewSep2, _miViewSep3, _miViewSep4;
	ImgMenuItem _miViewZoomFit, _miViewZoomIn, _miViewZoomOut;
	Gtk::MenuItem _miViewImagePlane, _miViewSetImagePlane, _miViewAddToImagePlane;
	Gtk::MenuItem _miViewStats;
	
	// Plot menu
	Gtk::MenuItem _miFlagComparison;
	Gtk::Menu _menuFlagComparison;
	Gtk::MenuItem _miPlotComparisonPowerSpectrum, _miPlotComparisonPowerTime, _miPlotComparisonTimeScatter;
	Gtk::MenuItem _miPlotDistribution, _miPlotLogLogDistribution, _miPlotComplexPlane, _miPlotMeanSpectrum;
	Gtk::MenuItem _miPlotSumSpectrum, _miPlotPowerSpectrum, _miPlotFrequencyScatter, _miPlotRMSSpectrum, _miPlotPowerTime;
	Gtk::MenuItem _miPlotTimeScatter, _miPlotSingularValues;
	
	// Browse menu
	Gtk::SeparatorMenuItem _miBrowseSep1, _miBrowseSep2;
	ImgMenuItem _miBrowsePrevious, _miBrowseReload, _miBrowseNext;
	Gtk::MenuItem _miBrowseGoto, _miBrowseLongestBaseline, _miBrowseShortestBaseline;
	
	// Simulate menu
	Gtk::Menu _menuTestSets;
	Gtk::SeparatorMenuItem _miTestSep1;
	Gtk::MenuItem _miTestSets;
	Gtk::RadioButtonGroup _testSetGroup;
	Gtk::RadioMenuItem _miTestGaussian, _miTestRayleigh, _miTestZero;
	Gtk::MenuItem _miTestA, _miTestB, _miTestC, _miTestD, _miTestE;
	Gtk::MenuItem _miTestF, _miTestG, _miTestH, _miTestNoise, _miTestModel3;
	Gtk::MenuItem _miTestModel5, _miTestNoiseModel3, _miTestNoiseModel5, _miTestBStrong;
	Gtk::MenuItem _miTestBWeak, _miTestBAligned, _miTestGaussianBroadband, _miTestSenusoidalBroadband;
	Gtk::MenuItem _miTestSlewedGaussianBroadband, _miTestBurstBroadband;
	Gtk::MenuItem _miTestRFIDistLow, _miTestRFIDistMid, _miTestRFIDistHigh;
	
	Gtk::Menu _menuModify;
	Gtk::MenuItem _miSimulateModify;
	Gtk::MenuItem _miModifyStaticFringe, _miModify1SigmaStaticFringe;
	Gtk::MenuItem _miModifyToOne, _miModifyToI, _miModifyToOnePlusI;
	Gtk::MenuItem _miModifyCorrelatorFault, _miModifyMultiply;
	
	Gtk::SeparatorMenuItem _miSimSep1, _miSimSep2;
	Gtk::RadioButtonGroup _simSetGroup, _simChGroup;
	Gtk::RadioMenuItem _miSimNCP, _miSimB1834, _miSimEmpty;
	Gtk::RadioMenuItem _miSim16channels, _miSim64channels, _miSim256channels;
	Gtk::CheckMenuItem _miSimFixBandwidth;
	Gtk::MenuItem _miSimCorrelation;
	Gtk::MenuItem _miSimSourceSetA, _miSimSourceSetB, _miSimSourceSetC, _miSimSourceSetD;
	Gtk::MenuItem _miSimSourceOffAxis, _miSimSourceOnAxis;
	
	// Data menu
	Gtk::MenuItem _miDataToOriginal;
	Gtk::SeparatorMenuItem _miDataSep1, _miDataSep2, _miDataSep3, _miDataSep4, _miDataSep5;
	Gtk::MenuItem _miDataReal, _miDataImaginary, _miDataPhase, _miDataUnrollPhase;
	Gtk::MenuItem _miDataStokesI, _miDataStokesQ, _miDataStokesU, _miDataStokesV;
	Gtk::MenuItem _miDataRR, _miDataRL, _miDataLR, _miDataLL;
	Gtk::MenuItem _miDataXX, _miDataXY, _miDataYX, _miDataYY;
	Gtk::MenuItem _miDataStore, _miDataRecall, _miDataSubtract;
	Gtk::MenuItem _miDataClearOriginalFlags, _miDataClearAltFlags;
	
	// Actions menu
	Gtk::MenuItem _miEditStrategy;
	Gtk::SeparatorMenuItem _miActionsSep1, _miActionsSep2, _miActionsSep3;
	ImgMenuItem _miActionsExecuteStrategy;
	Gtk::CheckMenuItem _miActionsCloseExecuteFrame;
	Gtk::MenuItem _miActionsExecutePythonStrategy, _miActionsExecuteLuaStrategy;
	Gtk::MenuItem _miActionsSegment, _miActionsCluster, _miActionsClassify, _miActionsRemoveSmallSegments, _miActionsInterpolateFlagged;
	Gtk::MenuItem _miActionsVertEVD, _miActionsApplyTimeProfile, _miActionsApplyVertProfile;
	Gtk::MenuItem _miActionsRestoreTimeProfile, _miActionsRestoreVertProfile, _miActionsReapplyTimeProfile, _miActionsReapplyVertProfile;
	
	// Help menu
	ImgMenuItem _miHelpAbout;
	
	// Toolbar
	Gtk::SeparatorToolItem _tbSep1, _tbSep2, _tbSep3;
	Gtk::ToolButton _tbOpenDirectory, _tbExecuteStrategy;
	Gtk::ToggleToolButton _tbOriginalFlags, _tbAlternativeFlags;
	Gtk::ToolButton _tbPrevious, _tbReload, _tbNext;
	Gtk::ToolButton _tbZoomFit, _tbZoomIn, _tbZoomOut;
	Gtk::ToggleToolButton _tbDisplayPP, _tbDisplayPQ, _tbDisplayQP, _tbDisplayQQ;
	Gtk::MenuToolButton _tbSelectVisualization;
	
	bool _blockVisualizationSignals;
	Gtk::Menu _tfVisualizationMenu;
};

