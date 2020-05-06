#include "rfiguiwindow.h"

#include <gtkmm/aboutdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>

#include "../msio/baselinematrixloader.h"

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"

#include "../strategy/actions/strategy.h"

#include "../strategy/control/artifactset.h"
#include "../strategy/control/defaultstrategy.h"

#include "../strategy/imagesets/msimageset.h"

#include "../strategy/algorithms/baselineselector.h"
#include "../strategy/algorithms/morphology.h"
#include "../strategy/algorithms/fringetestcreater.h"
#include "../strategy/algorithms/polarizationstatistics.h"
#include "../strategy/algorithms/thresholdtools.h"
#include "../strategy/algorithms/timefrequencystatistics.h"
#include "../strategy/algorithms/vertevd.h"

#include "../strategy/plots/antennaflagcountplot.h"
#include "../strategy/plots/frequencyflagcountplot.h"
#include "../strategy/plots/frequencypowerplot.h"
#include "../strategy/plots/iterationsplot.h"
#include "../strategy/plots/timeflagcountplot.h"

#include "controllers/imagecomparisoncontroller.h"
#include "controllers/rfiguicontroller.h"

#include "complexplaneplotwindow.h"
#include "editstrategywindow.h"
#include "gotowindow.h"
#include "highlightwindow.h"
#include "histogramwindow.h"
#include "imageplanewindow.h"
#include "imagepropertieswindow.h"
#include "msoptionwindow.h"
#include "numinputdialog.h"
#include "plotwindow.h"
#include "progresswindow.h"
#include "rfiguimenu.h"

#include "../imaging/model.h"
#include "../imaging/observatorium.h"

#include "../quality/histogramcollection.h"

#include "../version.h"

#include <iostream>

RFIGuiWindow::RFIGuiWindow(RFIGuiController* controller) : 
	_controller(controller),
	_mainVBox(Gtk::ORIENTATION_VERTICAL),
	_timeFrequencyWidget(&_controller->TFController().Plot()),
	_plotWindow(new PlotWindow(_controller->PlotManager())),
	_menu(new RFIGuiMenu()),
	_strategy(new rfiStrategy::Strategy()),
	_gaussianTestSets(true)
{
	_controller->AttachWindow(this);
	_controller->AttachStrategyControl(this);
	
	_menu->SetOriginalFlagsActive(_controller->AreOriginalFlagsShown());
	_menu->SetAlternativeFlagsActive(_controller->AreAlternativeFlagsShown());
	
	// File
	_menu->OnActionDirectoryOpen.connect([&]() { onActionDirectoryOpen(); });
	_menu->OnActionFileOpen.connect([&]() { onActionFileOpen(); });
	_menu->OnActionDirectoryOpenForSpatial.connect([&]() { onActionDirectoryOpenForSpatial(); });
	_menu->OnActionDirectoryOpenForST.connect([&]() { onActionDirectoryOpenForST(); });
	_menu->OnSaveBaseline.connect([&]() { onSaveBaseline(); });
	_menu->OnQuit.connect([&]() { onQuit(); });
	
	// View
	_menu->OnImagePropertiesPressed.connect([&]() { onImagePropertiesPressed(); });
	_menu->OnTimeGraphButtonPressed.connect([&]() { onTimeGraphButtonPressed(); });
	_menu->OnToggleFlags.connect([&]() { onToggleFlags(); });
	_menu->OnHightlightPressed.connect([&]() { onHightlightPressed(); });
	_menu->OnZoomFit.connect([&]() { onZoomFit(); });
	_menu->OnZoomIn.connect([&]() { onZoomIn(); });
	_menu->OnZoomOut.connect([&]() { onZoomOut(); });
	_menu->OnShowImagePlane.connect([&]() { onShowImagePlane(); });
	_menu->OnSetAndShowImagePlane.connect([&]() { onSetAndShowImagePlane(); });
	_menu->OnAddToImagePlane.connect([&]() { onAddToImagePlane(); });
	_menu->OnShowStats.connect([&]() { onShowStats(); });
	
	// Plot
	_menu->OnPlotPowerSpectrumComparisonPressed.connect([&]() { onImagePropertiesPressed(); });
	_menu->OnPlotPowerTimeComparisonPressed.connect([&]() { onPlotPowerTimeComparisonPressed(); });
	_menu->OnPlotTimeScatterComparisonPressed.connect([&]() { onPlotTimeScatterComparisonPressed(); });
	_menu->OnPlotDistPressed.connect([&]() { onPlotDistPressed(); });
	_menu->OnPlotLogLogDistPressed.connect([&]() { onPlotLogLogDistPressed(); });
	_menu->OnPlotComplexPlanePressed.connect([&]() { onPlotComplexPlanePressed(); });
	_menu->OnPlotMeanSpectrumPressed.connect([&]() { onPlotMeanSpectrumPressed(); });
	_menu->OnPlotSumSpectrumPressed.connect([&]() { onPlotSumSpectrumPressed(); });
	_menu->OnPlotPowerSpectrumPressed.connect([&]() { onPlotPowerSpectrumPressed(); });
	_menu->OnPlotFrequencyScatterPressed.connect([&]() { onPlotFrequencyScatterPressed(); });
	_menu->OnPlotPowerRMSPressed.connect([&]() { onPlotPowerRMSPressed(); });
	_menu->OnPlotPowerTimePressed.connect([&]() { onPlotPowerTimePressed(); });
	_menu->OnPlotTimeScatterPressed.connect([&]() { onPlotTimeScatterPressed(); });
	_menu->OnPlotSingularValuesPressed.connect([&]() { onPlotSingularValuesPressed(); });
	
	// Browse
	_menu->OnLoadPrevious.connect([&]() { onLoadPrevious(); });
	_menu->OnReloadPressed.connect([&]() { onReloadPressed(); });
	_menu->OnLoadNext.connect([&]() { onLoadNext(); });
	_menu->OnGoToPressed.connect([&]() { onGoToPressed(); });
	_menu->OnLoadLongestBaselinePressed.connect([&]() { onLoadLongestBaselinePressed(); });
	_menu->OnLoadShortestBaselinePressed.connect([&]() { onLoadShortestBaselinePressed(); });
	
	// Simulate
	_menu->OnGaussianTestSets.connect([&]() { onGaussianTestSets(); });
	_menu->OnRayleighTestSets.connect([&]() { onRayleighTestSets(); });
	_menu->OnZeroTestSets.connect([&]() { onZeroTestSets(); });
	
	_menu->OnOpenTestSetA.connect([&]() { onOpenTestSetA(); });
	_menu->OnOpenTestSetB.connect([&]() { onOpenTestSetB(); });
	_menu->OnOpenTestSetC.connect([&]() { onOpenTestSetC(); });
	_menu->OnOpenTestSetD.connect([&]() { onOpenTestSetD(); });
	_menu->OnOpenTestSetE.connect([&]() { onOpenTestSetE(); });
	_menu->OnOpenTestSetF.connect([&]() { onOpenTestSetF(); });
	_menu->OnOpenTestSetG.connect([&]() { onOpenTestSetG(); });
	_menu->OnOpenTestSetH.connect([&]() { onOpenTestSetH(); });
	_menu->OnOpenTestSetNoise.connect([&]() { onOpenTestSetNoise(); });
	_menu->OnOpenTestSet3Model.connect([&]() { onOpenTestSet3Model(); });
	_menu->OnOpenTestSet5Model.connect([&]() { onOpenTestSet5Model(); });
	_menu->OnOpenTestSetNoise3Model.connect([&]() { onOpenTestSetNoise3Model(); });
	_menu->OnOpenTestSetNoise5Model.connect([&]() { onOpenTestSetNoise5Model(); });
	_menu->OnOpenTestSetBStrong.connect([&]() { onOpenTestSetBStrong(); });
	_menu->OnOpenTestSetBWeak.connect([&]() { onOpenTestSetBWeak(); });
	_menu->OnOpenTestSetBAligned.connect([&]() { onOpenTestSetBAligned(); });
	
	_menu->OnOpenTestSetGaussianBroadband.connect([&]() { onOpenTestSetGaussianBroadband(); });
	_menu->OnOpenTestSetSinusoidalBroadband.connect([&]() { onOpenTestSetSinusoidalBroadband(); });
	_menu->OnOpenTestSetSlewedGaussianBroadband.connect([&]() { onOpenTestSetSlewedGaussianBroadband(); });
	_menu->OnOpenTestSetBurstBroadband.connect([&]() { onOpenTestSetBurstBroadband(); });
	_menu->OnOpenTestSetRFIDistributionLow.connect([&]() { onOpenTestSetRFIDistributionLow(); });
	_menu->OnOpenTestSetRFIDistributionMid.connect([&]() { onOpenTestSetRFIDistributionMid(); });
	_menu->OnOpenTestSetRFIDistributionHigh.connect([&]() { onOpenTestSetRFIDistributionHigh(); });
	
	_menu->OnAddStaticFringe.connect([&]() { onAddStaticFringe(); });
	_menu->OnAdd1SigmaFringe.connect([&]() { onAdd1SigmaFringe(); });
	_menu->OnSetToOne.connect([&]() { onSetToOne(); });
	_menu->OnSetToI.connect([&]() { onSetToI(); });
	_menu->OnSetToOnePlusI.connect([&]() { onSetToOnePlusI(); });
	_menu->OnAddCorrelatorFault.connect([&]() { onAddCorrelatorFault(); });
	_menu->OnMultiplyData.connect([&]() { onMultiplyData(); });
	
	_menu->OnSimulateCorrelation.connect([&]() { onSimulateCorrelation(); });
	_menu->OnSimulateSourceSetA.connect([&]() { onSimulateSourceSetA(); });
	_menu->OnSimulateSourceSetB.connect([&]() { onSimulateSourceSetB(); });
	_menu->OnSimulateSourceSetC.connect([&]() { onSimulateSourceSetC(); });
	_menu->OnSimulateSourceSetD.connect([&]() { onSimulateSourceSetD(); });
	_menu->OnSimulateOffAxisSource.connect([&]() { onSimulateOffAxisSource(); });
	_menu->OnSimulateOnAxisSource.connect([&]() { onSimulateOnAxisSource(); });
	
	// Data
	_menu->OnVisualizedToOriginalPressed.connect([&]() { onVisualizedToOriginalPressed(); });
	_menu->OnKeepRealPressed.connect([&]() { onKeepRealPressed(); });
	_menu->OnKeepImaginaryPressed.connect([&]() { onKeepImaginaryPressed(); });
	_menu->OnKeepPhasePressed.connect([&]() { onKeepPhasePressed(); });
	_menu->OnUnrollPhaseButtonPressed.connect([&]() { onUnrollPhaseButtonPressed(); });
	
	_menu->OnKeepStokesIPressed.connect([&]() { onKeepStokesIPressed(); });
	_menu->OnKeepStokesQPressed.connect([&]() { onKeepStokesQPressed(); });
	_menu->OnKeepStokesUPressed.connect([&]() { onKeepStokesUPressed(); });
	_menu->OnKeepStokesVPressed.connect([&]() { onKeepStokesVPressed(); });
	_menu->OnKeepRRPressed.connect([&]() { onKeepRRPressed(); });
	_menu->OnKeepRLPressed.connect([&]() { onKeepRLPressed(); });
	_menu->OnKeepLRPressed.connect([&]() { onKeepLRPressed(); });
	_menu->OnKeepLLPressed.connect([&]() { onKeepLLPressed(); });
	_menu->OnKeepXXPressed.connect([&]() { onKeepXXPressed(); });
	_menu->OnKeepXYPressed.connect([&]() { onKeepXYPressed(); });
	_menu->OnKeepYXPressed.connect([&]() { onKeepYXPressed(); });
	_menu->OnKeepYYPressed.connect([&]() { onKeepYYPressed(); });
	
	_menu->OnStoreData.connect([&]() { onStoreData(); });
	_menu->OnRecallData.connect([&]() { onRecallData(); });
	_menu->OnSubtractDataFromMem.connect([&]() { onSubtractDataFromMem(); });
	_menu->OnClearOriginalFlagsPressed.connect([&]() { onClearOriginalFlagsPressed(); });
	_menu->OnClearAltFlagsPressed.connect([&]() { onClearAltFlagsPressed(); });
	
	// Actions
	_menu->OnEditStrategyPressed.connect([&]() { onEditStrategyPressed(); });
	_menu->OnExecuteStrategyPressed.connect([&]() { onExecuteStrategyPressed(); });
	_menu->OnExecutePythonStrategy.connect([&]() { onExecutePythonStrategy(); });
	_menu->OnExecuteLuaStrategy.connect([&]() { onExecuteLuaStrategy(); });
	
	_menu->OnSegment.connect([&]() { onSegment(); });
	_menu->OnCluster.connect([&]() { onCluster(); });
	_menu->OnClassify.connect([&]() { onClassify(); });
	_menu->OnRemoveSmallSegments.connect([&]() { onRemoveSmallSegments(); });
	
	_menu->OnInterpolateFlagged.connect([&]() { _controller->InterpolateFlagged(); });
	_menu->OnVertEVD.connect([&]() { onVertEVD(); });
	_menu->OnApplyTimeProfile.connect([&]() { onApplyTimeProfile(); });
	_menu->OnApplyVertProfile.connect([&]() { onApplyVertProfile(); });
	
	_menu->OnUseTimeProfile.connect([&](bool invert) { onUseTimeProfile(invert); });
	_menu->OnUseVertProfile.connect([&](bool invert) { onUseVertProfile(invert); });
	
	// Help
	_menu->OnHelpAbout.connect([&]() { onHelpAbout(); });
	
	// Toolbar signals (some are already covered)
	_menu->OnTogglePolarizations.connect([&]() { onTogglePolarizations(); });
	_menu->OnToggleImage.connect([&]() { onToggleImage(); });
	_menu->OnSelectImage.connect([&]() { onSelectImage(); });
	
	_mainVBox.pack_start(_menu->Menu(), Gtk::PACK_SHRINK);
	_mainVBox.pack_start(_menu->Toolbar(), Gtk::PACK_SHRINK);
	
	_mainVBox.pack_start(_timeFrequencyWidget, Gtk::PACK_EXPAND_WIDGET);
	_timeFrequencyWidget.OnMouseMovedEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFWidgetMouseMoved));
	_timeFrequencyWidget.OnMouseLeaveEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::setSetNameInStatusBar));
	_timeFrequencyWidget.OnButtonReleasedEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFWidgetButtonReleased));
	_timeFrequencyWidget.OnScrollEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFScroll));
	_timeFrequencyWidget.Plot().OnZoomChanged().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFZoomChanged));
	_timeFrequencyWidget.Plot().SetShowXAxisDescription(false);
	_timeFrequencyWidget.Plot().SetShowYAxisDescription(false);
	_timeFrequencyWidget.Plot().SetShowZAxisDescription(false);
	
	_controller->TFController().VisualizationListChange().connect(sigc::mem_fun(*this, &RFIGuiWindow::updateTFVisualizationMenu));

	_mainVBox.pack_end(_statusbar, Gtk::PACK_SHRINK);
	_statusbar.push("Ready. For suggestions, contact offringa@gmail.com .");

	add(_mainVBox);
	_mainVBox.show_all();

	set_default_size(800,600);
	set_default_icon_name("aoflagger");

	rfiStrategy::DefaultStrategy::StrategySetup setup =
		rfiStrategy::DefaultStrategy::DetermineSetup(
			rfiStrategy::DefaultStrategy::GENERIC_TELESCOPE,
			rfiStrategy::DefaultStrategy::FLAG_NONE, 0.0, 0.0, 0.0);
	rfiStrategy::DefaultStrategy::LoadSingleStrategy(*_strategy, setup);
	_imagePlaneWindow.reset(new ImagePlaneWindow());
	
	onTFZoomChanged();
	
	_controller->SignalStateChange().connect(
		sigc::mem_fun(*this, &RFIGuiWindow::onControllerStateChange));
	
	updateTFVisualizationMenu();
}

RFIGuiWindow::~RFIGuiWindow()
{
	_imagePlaneWindow.reset();
	_plotWindow.reset();
	_histogramWindow.reset();
	_optionWindow.reset();
	_editStrategyWindow.reset();
	_gotoWindow.reset();
	_progressWindow.reset();
	_highlightWindow.reset();
	_imagePropertiesWindow.reset();
	
	_strategy.reset();
}

void RFIGuiWindow::onActionDirectoryOpen()
{
  Gtk::FileChooserDialog dialog("Select a measurement set",
          Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  int result = dialog.run();

  if(result == Gtk::RESPONSE_OK)
	{
		OpenPaths(std::vector<std::string>{dialog.get_filename()});
	}
}

void RFIGuiWindow::onActionDirectoryOpenForSpatial()
{
  Gtk::FileChooserDialog dialog("Select a measurement set",
          Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  int result = dialog.run();

  if(result == Gtk::RESPONSE_OK)
	{
		_controller->LoadSpatial(dialog.get_filename());
	}
}

void RFIGuiWindow::onActionDirectoryOpenForST()
{
  Gtk::FileChooserDialog dialog("Select a measurement set",
          Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  int result = dialog.run();

  if(result == Gtk::RESPONSE_OK)
	{
		_controller->LoadSpatialTime(dialog.get_filename());
	}
}

void RFIGuiWindow::onSaveBaseline()
{
  Gtk::FileChooserDialog dialog("Select baseline file", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Save", Gtk::RESPONSE_ACCEPT);

  auto filter_text = Gtk::FileFilter::create();
  filter_text->set_name("Baseline files (*.rfibl)");
  filter_text->add_mime_type("application/rfibl");
  dialog.add_filter(filter_text);
	dialog.set_do_overwrite_confirmation(true);
	
	int result = dialog.run();

  if(result == Gtk::RESPONSE_ACCEPT)
	{
		_controller->SaveBaseline(dialog.get_filename());
	}
}

void RFIGuiWindow::onActionFileOpen()
{
  Gtk::FileChooserDialog dialog("Select a measurement set");
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  int result = dialog.run();

  if(result == Gtk::RESPONSE_OK)
	{
		OpenPaths(std::vector<std::string>{dialog.get_filename()});
	}
}

void RFIGuiWindow::OpenPaths(const std::vector<std::string>& paths)
{
	_optionWindow.reset();
	if(paths.size()>1 || rfiStrategy::ImageSet::IsMSFile(paths.front()))
	{
		_optionWindow.reset(new MSOptionWindow(*_controller, paths));
		_optionWindow->present();
	}
	else {
		_controller->LoadPaths(paths);
	}
}

TimeFrequencyData RFIGuiWindow::GetActiveData() const
{
	return _controller->TFController().GetActiveData();
}
const TimeFrequencyData& RFIGuiWindow::GetOriginalData() const
{
	return _controller->TFController().OriginalData();
}
class ThresholdConfig &RFIGuiWindow::HighlightConfig()
{
	return _controller->TFController().Plot().HighlightConfig();
}
void RFIGuiWindow::SetHighlighting(bool newValue)
{
	_controller->TFController().Plot().SetHighlighting(newValue);
}
TimeFrequencyMetaDataCPtr RFIGuiWindow::SelectedMetaData()
{
	return _controller->TFController().Plot().GetSelectedMetaData();
}

void RFIGuiWindow::onToggleFlags()
{
	_controller->SetShowOriginalFlags(_menu->OriginalFlagsActive());
	_controller->SetShowAlternativeFlags(_menu->AlternativeFlagsActive());
}

void RFIGuiWindow::onTogglePolarizations()
{
	_controller->SetShowPP(_menu->ShowPPActive());
	_controller->SetShowPQ(_menu->ShowPQActive());
	_controller->SetShowQP(_menu->ShowQPActive());
	_controller->SetShowQQ(_menu->ShowQQActive());
}

void RFIGuiWindow::setSetNameInStatusBar()
{
  if(_controller->HasImageSet()) {
		_statusbar.pop();
		_statusbar.push(_imageSetName + ": " + _imageSetIndexDescription);
  }
}
		
void RFIGuiWindow::onLoadPrevious()
{
	if(_controller->HasImageSet()) {
		std::unique_lock<std::mutex> lock(_controller->IOMutex());
		_controller->GetImageSetIndex().Previous();
		lock.unlock();
		_controller->LoadCurrentTFData();
	}
}

void RFIGuiWindow::onLoadNext()
{
	if(_controller->HasImageSet()) {
		std::unique_lock<std::mutex> lock(_controller->IOMutex());
		_controller->GetImageSetIndex().Next();
		lock.unlock();
		_controller->LoadCurrentTFData();
	}
}

void RFIGuiWindow::onEditStrategyPressed()
{
	_editStrategyWindow.reset(new EditStrategyWindow(*_controller, *this));
	_editStrategyWindow->show();
}

void RFIGuiWindow::onExecuteStrategyPressed()
{
	_progressWindow.reset(new ProgressWindow(*this));
	_progressWindow->show();

	rfiStrategy::ArtifactSet artifacts(&_controller->IOMutex());

	artifacts.SetAntennaFlagCountPlot(std::unique_ptr<AntennaFlagCountPlot>(new AntennaFlagCountPlot()));
	artifacts.SetFrequencyFlagCountPlot(std::unique_ptr<FrequencyFlagCountPlot>(new FrequencyFlagCountPlot()));
	artifacts.SetFrequencyPowerPlot(std::unique_ptr<FrequencyPowerPlot>(new FrequencyPowerPlot()));
	artifacts.SetTimeFlagCountPlot(std::unique_ptr<TimeFlagCountPlot>(new TimeFlagCountPlot()));
	artifacts.SetIterationsPlot(std::unique_ptr<IterationsPlot>(new IterationsPlot()));
	
	artifacts.SetPolarizationStatistics(std::unique_ptr<PolarizationStatistics>(new PolarizationStatistics()));
	artifacts.SetBaselineSelectionInfo(std::unique_ptr<rfiStrategy::BaselineSelector>(new rfiStrategy::BaselineSelector()));
	artifacts.SetImager(_imagePlaneWindow->GetImager());

	if(HasImage())
	{
		artifacts.SetOriginalData(GetOriginalData());
		artifacts.SetContaminatedData(GetActiveData());
		TimeFrequencyData zero(GetOriginalData());
		zero.SetImagesToZero();
		artifacts.SetRevisedData(zero);
	}
	if(_timeFrequencyWidget.Plot().GetFullMetaData() != nullptr)
		artifacts.SetMetaData(_timeFrequencyWidget.Plot().GetFullMetaData());
	if(_controller->HasImageSet())
	{
		artifacts.SetImageSet(_controller->GetImageSet().Clone());
		artifacts.SetImageSetIndex(_controller->GetImageSetIndex().Clone());
	}
	artifacts.SetCanVisualize(true);
	_strategy->InitializeAll();
	try {
		_strategy->StartPerformThread(artifacts, static_cast<ProgressWindow&>(*_progressWindow));
	}  catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onExecuteStrategyFinished()
{
	std::unique_ptr<rfiStrategy::ArtifactSet> artifacts = _strategy->JoinThread();
	_controller->TFController().ClearAllButOriginal();
	if(artifacts)
	{
		bool update = false;
		
		auto vis = artifacts->Visualizations();
		// Sort the visualizations on their sorting index
		std::sort(vis.begin(), vis.end(),
							[](const std::tuple<std::string, TimeFrequencyData, size_t>& a,
								 const std::tuple<std::string, TimeFrequencyData, size_t>& b)
			{ return std::get<2>(a) < std::get<2>(b); });
		for(const auto& v : vis)
		{
			_controller->TFController().AddVisualization(std::get<0>(v), std::get<1>(v));
			update = true;
		}
		
		if(!artifacts->ContaminatedData().IsEmpty())
		{
			size_t index = 
				_controller->TFController().AddVisualization("Estimated RFI", artifacts->ContaminatedData());
			_controller->TFController().SetVisualization(index);
			update = true;
		}
		
		updateTFVisualizationMenu();
		
		if(update)
			_timeFrequencyWidget.Update();
		
		_imagePlaneWindow->Update();
		
		if(artifacts->AntennaFlagCountPlot().HasData())
			artifacts->AntennaFlagCountPlot().MakePlot();
		if(artifacts->FrequencyFlagCountPlot().HasData())
			artifacts->FrequencyFlagCountPlot().MakePlot();
		if(artifacts->FrequencyPowerPlot().HasData())
			artifacts->FrequencyPowerPlot().MakePlot();
		if(artifacts->TimeFlagCountPlot().HasData())
			artifacts->TimeFlagCountPlot().MakePlot();
		if(artifacts->PolarizationStatistics().HasData())
			artifacts->PolarizationStatistics().Report();
		if(artifacts->IterationsPlot().HasData())
			artifacts->IterationsPlot().MakePlot();
	}
	if(_menu->CloseExecuteFrame())
	{
		_progressWindow.reset();
	}
}

void RFIGuiWindow::UpdateImageSetIndex()
{
	if(_controller->HasImageSet())
	{
		_imageSetIndexDescription = _controller->GetImageSetIndex().Description();
		_controller->LoadCurrentTFData();
	}
}

void RFIGuiWindow::openTestSet(unsigned index)
{
	_controller->OpenTestSet(index, _gaussianTestSets);
}

void RFIGuiWindow::onClearOriginalFlagsPressed()
{
	TimeFrequencyData data = _controller->TFController().GetVisualizationData(0);
	data.SetMasksToValue<false>();
	_controller->TFController().SetVisualizationData(0, std::move(data));
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onClearAltFlagsPressed()
{
	TimeFrequencyData data(_controller->TFController().AltMaskData());
	data.SetMasksToValue<false>();
	_controller->TFController().SetAltMaskData(data);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onVisualizedToOriginalPressed()
{
	if(HasImage())
	{
		TimeFrequencyData data(_controller->TFController().VisualizedData());
		_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetFullMetaData());
	}
	// TODO
	/*if(_originalImageButton->get_active())
		_timeFrequencyWidget.Update();
	else
		_originalImageButton->set_active();
	*/
}

void RFIGuiWindow::onHightlightPressed()
{
	_highlightWindow.reset(new HighlightWindow(*this));
	_highlightWindow->show();
}

void RFIGuiWindow::onAddStaticFringe()
{
	try {
		if(HasImage())
		{
			TimeFrequencyMetaDataCPtr metaData = SelectedMetaData();
			TimeFrequencyData data(GetActiveData());
			FringeTestCreater::AddStaticFringe(data, metaData, 1.0L);
			_controller->TFController().SetNewData(data, metaData);
			_timeFrequencyWidget.Update();
		}
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onAdd1SigmaFringe()
{
	try {
		if(HasImage())
		{
			TimeFrequencyMetaDataCPtr metaData = SelectedMetaData();
			num_t mean, stddev;
			TimeFrequencyData data(GetActiveData());
			ThresholdTools::MeanAndStdDev(data.GetRealPart().get(), data.GetSingleMask().get(), mean, stddev);
			FringeTestCreater::AddStaticFringe(data, metaData, stddev);
			_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		}
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onSetToOne()
{
	try {
		TimeFrequencyData data(GetActiveData());
		std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::MakePtr(*images[0]),
			imaginary = Image2D::MakePtr(*images[1]);
		real->SetAll(1.0);
		imaginary->SetAll(0.0);
		TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
		newData.SetMask(data);
		_controller->TFController().SetNewData(newData, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onSetToI()
{
	try {
		TimeFrequencyData data(GetActiveData());
		std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::MakePtr(*images[0]),
			imaginary = Image2D::MakePtr(*images[0]);
		real->SetAll(0.0);
		imaginary->SetAll(1.0);
		TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
		newData.SetMask(data);
		_controller->TFController().SetNewData(newData, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onSetToOnePlusI()
{
	try {
		TimeFrequencyData data(GetActiveData());
		std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::MakePtr(*images[0]),
			imaginary = Image2D::MakePtr(*images[0]);
		real->SetAll(1.0);
		imaginary->SetAll(1.0);
		TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
		newData.SetMask(data);
		_controller->TFController().SetNewData(newData, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onAddCorrelatorFault()
{
	TimeFrequencyData data(GetActiveData());
	size_t
		startIndex = data.ImageWidth()*1/4,
		endIndex = data.ImageWidth()*2/4;
	for(size_t i=0; i!=data.ImageCount(); ++i)
	{
		Image2DPtr image(new Image2D(*data.GetImage(i)));
		num_t addValue = 10.0 * image->GetStdDev();
		for(size_t y=0; y!=image->Height(); ++y)
		{
			for(size_t x=startIndex; x!=endIndex; ++x)
				image->AddValue(x, y, addValue);
		}
		
		data.SetImage(i, std::move(image));
	}
	for(size_t i=0; i!=data.MaskCount(); ++i)
	{
		Mask2DPtr mask(new Mask2D(*data.GetMask(i)));
		for(size_t y=0; y!=mask->Height(); ++y)
		{
			for(size_t x=startIndex; x!=endIndex; ++x)
				mask->SetValue(x, y, true);
		}
		data.SetMask(i, std::move(mask));
	}
	_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onShowStats()
{
	if(_timeFrequencyWidget.Plot().HasImage())
	{
		TimeFrequencyData activeData = GetActiveData();
		TimeFrequencyStatistics statistics(activeData);
		std::stringstream s;
		s << "Percentage flagged: " << TimeFrequencyStatistics::FormatRatio(statistics.GetFlaggedRatio()) << "\n";
			
		Mask2DCPtr
			original = _controller->TFController().Plot().OriginalMask(),
			alternative = _controller->TFController().Plot().AlternativeMask();
		Mask2DPtr
			intersect;
		if(original != 0 && alternative != 0)
		{
			intersect = Mask2D::MakePtr(*original);
			intersect->Intersect(*alternative);
			
			unsigned intCount = intersect->GetCount<true>();
			if(intCount != 0)
			{
				if(*original != *alternative)
				{
					s << "Overlap between original and alternative: " << TimeFrequencyStatistics::FormatRatio((double) intCount / ((double) (original->Width() * original->Height()))) << "\n"
					<< "(relative to alternative flags: " << TimeFrequencyStatistics::FormatRatio((double) intCount / ((double) (alternative->GetCount<true>()))) << ")\n";
					
				}
			}
		}
		
		Image2DCPtr powerImg = activeData.GetSingleImage();
		Mask2DCPtr mask = activeData.GetSingleMask();
		double power = 0.0;
		for(unsigned y=0;y<powerImg->Height();++y)
		{
			for(unsigned x=0;x<powerImg->Width();++x)
			{
				if(!mask->Value(x, y) && std::isfinite(powerImg->Value(x, y)))
				{
					power += powerImg->Value(x, y);
				}
			}
		}
		s << "Total unflagged power: " << power << "\n";
		Gtk::MessageDialog dialog(*this, s.str(), false, Gtk::MESSAGE_INFO);
		dialog.run();
	}
}

void RFIGuiWindow::onPlotDistPressed()
{
	_controller->PlotDist();
}

void RFIGuiWindow::onPlotLogLogDistPressed()
{
	_controller->PlotLogLogDist();
}

void RFIGuiWindow::onPlotComplexPlanePressed()
{
	if(HasImage()) {
		_plotComplexPlaneWindow.reset(new ComplexPlanePlotWindow(*this, _controller->PlotManager()));
		_plotComplexPlaneWindow->show();
	}
}

void RFIGuiWindow::onPlotPowerSpectrumPressed()
{
	_controller->PlotPowerSpectrum();
}

void RFIGuiWindow::onPlotPowerSpectrumComparisonPressed()
{
	_controller->PlotPowerSpectrumComparison();
}

void RFIGuiWindow::onPlotFrequencyScatterPressed()
{
	_controller->PlotFrequencyScatter();
}

void RFIGuiWindow::onPlotPowerRMSPressed()
{
	_controller->PlotPowerRMS();
}

void RFIGuiWindow::onPlotPowerTimePressed()
{
	_controller->PlotPowerTime();
}

void RFIGuiWindow::onPlotPowerTimeComparisonPressed()
{
	_controller->PlotPowerTimeComparison();
}

void RFIGuiWindow::onPlotTimeScatterPressed()
{
	_controller->PlotTimeScatter();
}

void RFIGuiWindow::onPlotTimeScatterComparisonPressed()
{
	_controller->PlotTimeScatterComparison();
}

void RFIGuiWindow::onPlotSingularValuesPressed()
{
	_controller->PlotSingularValues();
}

void RFIGuiWindow::ShowHistogram(HistogramCollection& histogramCollection)
{
	if(_histogramWindow == nullptr)
		_histogramWindow.reset(new HistogramWindow(histogramCollection));
	else
		_histogramWindow->SetStatistics(histogramCollection);
	_histogramWindow->show();
}

void RFIGuiWindow::onImagePropertiesPressed()
{
	_imagePropertiesWindow.reset(new ImagePropertiesWindow(_timeFrequencyWidget, "Time-frequency plotting options"));
	_imagePropertiesWindow->show();
}

void RFIGuiWindow::onPlotMeanSpectrumPressed()
{
	_controller->PlotMeanSpectrum();
}

void RFIGuiWindow::onPlotSumSpectrumPressed()
{
	_controller->PlotSumSpectrum();
}

void RFIGuiWindow::keepPhasePart(enum TimeFrequencyData::ComplexRepresentation phaseRepresentation)
{
	if(HasImage())
	{
		try {
			_controller->TFController().SetNewData(
				_controller->TFController().GetActiveData().Make(phaseRepresentation), _controller->TFController().Plot().GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		} catch(std::exception &e)
		{
			std::stringstream errstr;
			errstr
				<< "The data that was currently in memory could not be converted to the requested "
				   "type. The error given by the converter was:\n"
				<< e.what()
				<< "\n\n"
				<< "Note that if the original data should be convertable to this type, but "
				   "you have already used one of the 'Keep ..' buttons, you first need to reload "
					 "the full data with Goto -> Load.\n\n"
					 "(alternatively, if loading takes a lot of time, you can use the Store and Recall"
					 " options in the Data menu)";
			showError(errstr.str());
		}
	}
}

void RFIGuiWindow::updatePolarizations()
{
	_controller->CheckPolarizations();
	bool pp, pq, qp, qq;
	_controller->GetAvailablePolarizations(pp, pq, qp, qq);
	_menu->SetShowPPSensitive(pp);
	_menu->SetShowPQSensitive(pq);
	_menu->SetShowQPSensitive(qp);
	_menu->SetShowQQSensitive(qq);
}

void RFIGuiWindow::keepPolarisation(PolarizationEnum polarisation)
{
	if(HasImage())
	{
		try {
			_controller->TFController().SetNewData(
				_controller->TFController().GetActiveData().Make(polarisation), _controller->TFController().Plot().GetSelectedMetaData());
			updatePolarizations();
			_timeFrequencyWidget.Update();
		} catch(std::exception &e)
		{
			std::stringstream errstr;
			errstr
				<< "The data that was currently in memory could not be converted to the requested "
				   "polarization. The error given by the converter was:\n"
				<< e.what()
				<< "\n\n"
				<< "Note that if the original data should be convertable to this polarization, but "
				   "you have already used one of the 'Keep ..' buttons, you first need to reload "
					 "the full data with Goto -> Load.\n\n"
					 "(alternatively, if loading takes a lot of time, you can use the Store and Recall"
					 " options in the Data menu)";
			showError(errstr.str());
		}
	}
}

void RFIGuiWindow::onGoToPressed()
{
	if(_controller->HasImageSet())
	{
		rfiStrategy::IndexableSet *msSet =
			dynamic_cast<rfiStrategy::IndexableSet*>(&_controller->GetImageSet());
		if(msSet != nullptr)
		{
			_gotoWindow.reset(new GoToWindow(*this));
			_gotoWindow->show();
		} else {
			showError("Can not goto in this image set; format does not support goto");
		}
	}
}

void RFIGuiWindow::onReloadPressed()
{
	if(_controller->HasImageSet())
	{
		_controller->LoadCurrentTFData();
	}
}

void RFIGuiWindow::onLoadLongestBaselinePressed()
{
	if(_controller->HasImageSet())
	{
		rfiStrategy::IndexableSet *msSet =
			dynamic_cast<rfiStrategy::IndexableSet*>(&_controller->GetImageSet());
		if(msSet != nullptr)
		{
			double longestSq = 0.0;
			size_t longestA1=0, longestA2=0;
			size_t antCount = msSet->AntennaCount();
			std::vector<AntennaInfo> antennas(antCount);
			for(size_t a=0; a!=antCount; a++)
				antennas[a] = msSet->GetAntennaInfo(a);
			
			std::unique_ptr<rfiStrategy::ImageSetIndex> index(msSet->StartIndex());
			size_t band = msSet->GetBand(_controller->GetImageSetIndex());
			size_t sequenceId = msSet->GetSequenceId(_controller->GetImageSetIndex());
			while(index->IsValid())
			{
				if(sequenceId == msSet->GetSequenceId(*index) && band == msSet->GetBand(*index))
				{
					size_t a1 = msSet->GetAntenna1(*index);
					size_t a2 = msSet->GetAntenna2(*index);
					const AntennaInfo &ant1 = antennas[a1], &ant2 = antennas[a2];
					double distSq = ant1.position.DistanceSquared(ant2.position);
					if(distSq > longestSq)
					{
						longestSq = distSq;
						longestA1 = a1;
						longestA2 = a2;
					}
				}
				index->Next();
			}
			_controller->SetImageSetIndex(msSet->Index(longestA1, longestA2, band, sequenceId));
		}
	}
}

void RFIGuiWindow::onLoadShortestBaselinePressed()
{
	if(_controller->HasImageSet())
	{
		rfiStrategy::IndexableSet *msSet =
			dynamic_cast<rfiStrategy::IndexableSet*>(&_controller->GetImageSet());
		if(msSet != nullptr)
		{
			double smallestSq = 1e26;
			size_t smallestA1=0, smallestA2=0;
			size_t antCount = msSet->AntennaCount();
			std::vector<AntennaInfo> antennas(antCount);
			for(size_t a=0; a!=antCount; a++)
				antennas[a] = msSet->GetAntennaInfo(a);
			
			std::unique_ptr<rfiStrategy::ImageSetIndex> index(msSet->StartIndex());
			size_t band = msSet->GetBand(_controller->GetImageSetIndex());
			size_t sequenceId = msSet->GetSequenceId(_controller->GetImageSetIndex());
			while(index->IsValid())
			{
				if(sequenceId == msSet->GetSequenceId(*index) && band == msSet->GetBand(*index))
				{
					size_t a1 = msSet->GetAntenna1(*index);
					size_t a2 = msSet->GetAntenna2(*index);
					const AntennaInfo &ant1 = antennas[a1], &ant2 = antennas[a2];
					double distSq = ant1.position.DistanceSquared(ant2.position);
					if(distSq < smallestSq && a1 != a2)
					{
						smallestSq = distSq;
						smallestA1 = a1;
						smallestA2 = a2;
					}
				}
				index->Next();
			}
			_controller->SetImageSetIndex(msSet->Index(smallestA1, smallestA2, band, sequenceId));
		}
	}
}

void RFIGuiWindow::onTFWidgetMouseMoved(size_t x, size_t y)
{
	Image2DCPtr image = _timeFrequencyWidget.Plot().Image();
	num_t v = image->Value(x, y);
	_statusbar.pop();
	std::stringstream s;
		s << "x=" << x << ",y=" << y << ",value=" << v;
	TimeFrequencyMetaDataCPtr metaData =_timeFrequencyWidget.Plot().GetFullMetaData();
	if(metaData != 0)
	{
		if(metaData->HasObservationTimes() && metaData->HasBand())
		{
			const std::vector<double> &times = metaData->ObservationTimes();
			s << " (t=" << Date::AipsMJDToString(times[x]) <<
			", f=" << Frequency::ToString(metaData->Band().channels[y].frequencyHz);
		}
		
		if(metaData->HasUVW())
		{
			UVW uvw = metaData->UVW()[x];
			s << ", uvw=" << uvw.u << "," << uvw.v << "," << uvw.w;
		}
		s << ')';
	}
	_statusbar.push(s.str(), 0);
}

void RFIGuiWindow::onShowImagePlane()
{
	_imagePlaneWindow->show();
}

void RFIGuiWindow::onSetAndShowImagePlane()
{
	_imagePlaneWindow->GetImager()->Empty();
	onAddToImagePlane();
	_imagePlaneWindow->show();
	_imagePlaneWindow->GetImager()->ApplyWeightsToUV();
	_imagePlaneWindow->Update();
}

void RFIGuiWindow::onAddToImagePlane()
{
	try {
		if(_timeFrequencyWidget.Plot().GetFullMetaData() != 0 && _timeFrequencyWidget.Plot().GetFullMetaData()->HasUVW())
		{
			TimeFrequencyData activeData = GetActiveData();
			if(activeData.PolarizationCount() != 1)
			{
				activeData = activeData.Make(Polarization::StokesI);
			}
			_imagePlaneWindow->AddData(activeData, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		}
		else
			showError("No meta data found.");
	}  catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onMultiplyData()
{
	TimeFrequencyData data(GetActiveData());
	data.MultiplyImages(2.0L);
	_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSegment()
{
	_segmentedImage = SegmentedImage::CreateUnsetPtr(GetOriginalData().ImageWidth(),  GetOriginalData().ImageHeight());
	Morphology morphology;
	morphology.SegmentByLengthRatio(GetActiveData().GetSingleMask().get(), _segmentedImage);
	_timeFrequencyWidget.Plot().SetSegmentedImage(_segmentedImage);
	Update();
}

void RFIGuiWindow::onCluster()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.Cluster(_segmentedImage);
	_timeFrequencyWidget.Plot().SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onClassify()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.Classify(_segmentedImage);
		_timeFrequencyWidget.Plot().SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onRemoveSmallSegments()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.RemoveSmallSegments(_segmentedImage, 4);
	_timeFrequencyWidget.Plot().SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onTimeGraphButtonPressed()
{
	if(_menu->TimeGraphActive())
	{
		//_mainVBox.remove(_timeFrequencyWidget);
		//_mainVBox.pack_start(_panedArea, Gtk::PACK_EXPAND_WIDGET);
		//_panedArea.add1(_timeFrequencyWidget);
		//_panedArea.add2(_plotFrame);
		_mainVBox.pack_start(_plotFrame, true, true);
		_mainVBox.show_all();
		//_panedArea.set_position(_panedArea.get_height()/2);
		//std::cout << "size of _panedArea: " << _panedArea.get_width() <<',' << _panedArea.get_height() << '\n';
		//std::cout << "size of tf: " << _timeFrequencyWidget.get_height() << '\n';
		//std::cout << "plotframe: " << _plotFrame.get_height() << '\n';
	} else {
		//_mainVBox.remove(_panedArea);
		//_panedArea.remove(_timeFrequencyWidget);
		//_panedArea.remove(_plotFrame);
		//_mainVBox.pack_start(_timeFrequencyWidget);
		_mainVBox.remove(_plotFrame);

		_timeFrequencyWidget.show();
	}
}

void RFIGuiWindow::onTFWidgetButtonReleased(size_t x, size_t y)
{
	if(HasImage())
	{
		if(_plotFrame.get_visible())
		{
			_plotFrame.SetTimeFrequencyData(GetActiveData());
			_plotFrame.SetSelectedSample(x, y);
		
			_plotFrame.Update();
		}
	}
}

void RFIGuiWindow::onTFScroll(size_t x, size_t y, int direction)
{
	if(direction < 0)
	{
		_timeFrequencyWidget.Plot().ZoomInOn(x, y);
		_timeFrequencyWidget.Update();
	}
	else if(direction > 0) {
		_timeFrequencyWidget.Plot().ZoomOut();
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::onUnrollPhaseButtonPressed()
{
	if(HasImage())
	{
		TimeFrequencyData data = GetActiveData().Make(TimeFrequencyData::PhasePart);
		for(unsigned i=0;i<data.ImageCount();++i)
		{
			Image2DPtr image = Image2D::MakePtr(*data.GetImage(i));
			ThresholdTools::UnrollPhase(image.get());
			data.SetImage(i, image);
		}
		_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::showError(const std::string &description)
{
	Gtk::MessageDialog dialog(*this, description, false, Gtk::MESSAGE_ERROR);
	dialog.run();
}

DefaultModels::SetLocation RFIGuiWindow::getSetLocation(bool empty)
{
	if(empty)
		return DefaultModels::EmptySet;
	if(_menu->SimulateNCPActive())
		return DefaultModels::NCPSet;
	else if(_menu->SimulateB1834Active())
		return DefaultModels::B1834Set;
	else
		return DefaultModels::EmptySet;
}

void RFIGuiWindow::loadDefaultModel(DefaultModels::Distortion distortion, bool withNoise, bool empty)
{
	unsigned channelCount;
	if(_menu->Simulate16ChActive())
		channelCount = 16;
	else if(_menu->Simulate64ChActive())
		channelCount = 64;
	else
		channelCount = 256;
	double bandwidth;
	if(_menu->SimFixBandwidthActive())
		bandwidth = 16.0 * 2500000.0;
	else
		bandwidth = (double) channelCount / 16.0 * 2500000.0;
	std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> pair = DefaultModels::LoadSet(getSetLocation(empty), distortion, withNoise ? 1.0 : 0.0, channelCount, bandwidth);
	TimeFrequencyData data = pair.first;
	TimeFrequencyMetaDataCPtr metaData = pair.second;
	
	_controller->TFController().SetNewData(data, metaData);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onVertEVD()
{
	if(HasImage())
	{
		try {
			TimeFrequencyData data = GetActiveData();
			TimeFrequencyData old(data);
			VertEVD::Perform(data, true);
			_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		} catch(std::exception &e)
		{
			showError(e.what());
		}
	}
}

void RFIGuiWindow::onApplyTimeProfile()
{
	if(HasImage())
	{
		TimeFrequencyData data = GetActiveData();
		if(_horProfile.size() != data.ImageWidth())
		{
			_horProfile.clear();
			for(unsigned i=0;i<data.ImageWidth();++i)
				_horProfile.push_back(1.0);
		}
		
		Image2DCPtr weights = data.GetSingleImage();
		for(unsigned i=0;i<data.ImageCount();++i)
		{
			Image2DCPtr input = data.GetImage(i);
			Image2DPtr output = Image2D::CreateUnsetImagePtr(input->Width(), input->Height());
			for(unsigned x=0;x<weights->Width();++x)
			{
				num_t timeAvg = 0.0;
				for(unsigned y=0;y<weights->Height();++y)
				{
					if(std::isfinite(weights->Value(x, y)))
						timeAvg += weights->Value(x, y);
				}
				timeAvg /= (num_t) weights->Height();
				_horProfile[x] = timeAvg;
				for(unsigned y=0;y<input->Height();++y)
				{
					output->SetValue(x, y, input->Value(x, y) * timeAvg);
				}
			}
			data.SetImage(i, output);
		}
		_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::onApplyVertProfile()
{
	if(HasImage())
	{
		TimeFrequencyData data = GetActiveData();
		if(_vertProfile.size() != data.ImageHeight())
		{
			_vertProfile.clear();
			for(unsigned i=0;i<data.ImageHeight();++i)
				_vertProfile.push_back(1.0);
		}

		Image2DCPtr weights = data.GetSingleImage();
		for(unsigned i=0;i<data.ImageCount();++i)
		{
			Image2DCPtr input = data.GetImage(i);
			Image2DPtr output = Image2D::CreateUnsetImagePtr(input->Width(), input->Height());
			for(unsigned y=0;y<weights->Height();++y)
			{
				num_t vertAvg = 0.0;
				for(unsigned x=0;x<weights->Width();++x)
				{
					if(std::isfinite(weights->Value(x, y)))
						vertAvg += weights->Value(x, y);
				}
				vertAvg /= (num_t) weights->Width();
				_vertProfile[y] = vertAvg;
				for(unsigned x=0;x<input->Width();++x)
				{
					output->SetValue(x, y, input->Value(x, y) * vertAvg);
				}
			}
			data.SetImage(i, output);
		}
		_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::onUseTimeProfile(bool inverse)
{
	if(HasImage())
	{
		TimeFrequencyData data = GetActiveData();
		if(_horProfile.size()==data.ImageWidth())
		{
			for(unsigned i=0;i<data.ImageCount();++i)
			{
				Image2DCPtr input = data.GetImage(i);
				Image2DPtr output = Image2D::CreateUnsetImagePtr(input->Width(), input->Height());
				for(unsigned x=0;x<input->Width();++x)
				{
					for(unsigned y=0;y<input->Height();++y)
					{
						if(inverse)
						{
							if(_horProfile[x] != 0.0)
								output->SetValue(x, y, input->Value(x, y) / _horProfile[x]);
							else
								output->SetValue(x, y, 0.0);
						} else {
								output->SetValue(x, y, input->Value(x, y) * _horProfile[x]);
						}
					}
				}
				data.SetImage(i, output);
			}
			_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		}
	}
}

void RFIGuiWindow::onUseVertProfile(bool inverse)
{
	if(HasImage())
	{
		TimeFrequencyData data = GetActiveData();
		if(_vertProfile.size()==data.ImageHeight())
		{
			TimeFrequencyData data = GetActiveData();
			for(unsigned i=0;i<data.ImageCount();++i)
			{
				Image2DCPtr input = data.GetImage(i);
				Image2DPtr output = Image2D::CreateUnsetImagePtr(input->Width(), input->Height());
				for(unsigned x=0;x<input->Width();++x)
				{
					for(unsigned y=0;y<input->Height();++y)
					{
						if(inverse)
						{
							if(_vertProfile[y] != 0.0)
								output->SetValue(x, y, input->Value(x, y) / _vertProfile[y]);
							else
								output->SetValue(x, y, 0.0);
						} else {
								output->SetValue(x, y, input->Value(x, y) * _vertProfile[y]);
						}
					}
				}
				data.SetImage(i, output);
			}
			_controller->TFController().SetNewData(data, _timeFrequencyWidget.Plot().GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		}
	}
}

void RFIGuiWindow::onStoreData()
{
	if(HasImage())
	{
		_storedData = _controller->TFController().GetActiveData();
		_storedMetaData = _controller->TFController().Plot().GetSelectedMetaData();
	}
}

void RFIGuiWindow::onRecallData()
{
	_controller->TFController().SetNewData(_storedData, _storedMetaData);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSubtractDataFromMem()
{
	if(HasImage())
	{
		TimeFrequencyData diffData = TimeFrequencyData::MakeFromDiff(_storedData, _controller->TFController().GetActiveData());
		_controller->TFController().SetNewData(diffData, _storedMetaData);
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::SetStrategy(std::unique_ptr<rfiStrategy::Strategy> newStrategy)
{
	_strategy = std::move(newStrategy);
}

void RFIGuiWindow::onControllerStateChange()
{
	_menu->BlockVisualizationSignals();
	
	_menu->SetOriginalFlagsActive(_controller->AreOriginalFlagsShown());
	_timeFrequencyWidget.Plot().SetShowOriginalMask(_controller->AreOriginalFlagsShown());
	
	_menu->SetAlternativeFlagsActive(_controller->AreAlternativeFlagsShown());
	_timeFrequencyWidget.Plot().SetShowAlternativeMask(_controller->AreAlternativeFlagsShown());
	
	_menu->SetShowPPActive(_controller->IsPPShown());
	_menu->SetShowPQActive(_controller->IsPQShown());
	_menu->SetShowQPActive(_controller->IsQPShown());
	_menu->SetShowQQActive(_controller->IsQQShown());
	
	_controller->TFController().SetVisualizedPolarization(_controller->IsPPShown(), _controller->IsPQShown(), _controller->IsQPShown(), _controller->IsQQShown());
	
	_menu->UnblockVisualizationSignals();
	
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomFit() {
	_timeFrequencyWidget.Plot().ZoomFit();
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomIn()
{ 
	if(_timeFrequencyWidget.IsMouseInImage())
		_timeFrequencyWidget.Plot().ZoomInOn(_timeFrequencyWidget.MouseX(), _timeFrequencyWidget.MouseY());
	else
		_timeFrequencyWidget.Plot().ZoomIn();
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomOut() {
	_timeFrequencyWidget.Plot().ZoomOut(); 
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onTFZoomChanged()
{
	bool s = !_timeFrequencyWidget.Plot().IsZoomedOut();
	bool i = _timeFrequencyWidget.Plot().HasImage();
	_menu->SetZoomToFitSensitive(s && i);
	_menu->SetZoomOutSensitive(s && i);
	_menu->SetZoomInSensitive(i);
}

void RFIGuiWindow::onHelpAbout()
{
	Gtk::AboutDialog aboutDialog;
	
	std::vector<Glib::ustring> authors;
	authors.push_back("André Offringa <offringa@gmail.com>");
	aboutDialog.set_authors(authors);
	
	aboutDialog.set_copyright("Copyright 2008 - 2018 A. R. Offringa");
	aboutDialog.set_license_type(Gtk::LICENSE_GPL_3_0);
	aboutDialog.set_logo_icon_name("aoflagger");
	aboutDialog.set_program_name("AOFlagger's RFI Gui");
	aboutDialog.set_version("AOFlagger " AOFLAGGER_VERSION_STR " (" AOFLAGGER_VERSION_DATE_STR ") ");
	aboutDialog.set_website("http://aoflagger.sourceforge.net/");
	
	aboutDialog.run();
}

void RFIGuiWindow::onExecutePythonStrategy()
{
	try {
		_controller->ExecutePythonStrategy();
	}
	catch(std::exception& e) {
		showError(e.what());
	}
}

void RFIGuiWindow::onExecuteLuaStrategy()
{
	try {
		_controller->ExecuteLuaStrategy();
	}
	catch(std::exception& e) {
		showError(e.what());
	}
}

void RFIGuiWindow::SetBaselineInfo(bool multipleBaselines, const std::string& name, const std::string& description)
{
	_menu->SetPreviousSensitive(multipleBaselines);
	_menu->SetReloadSensitive(true);
	_menu->SetNextSensitive(multipleBaselines);
	_imageSetName = name;
	_imageSetIndexDescription = description;
	setSetNameInStatusBar();
	updatePolarizations();
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSelectImage()
{
	size_t index = getActiveTFVisualization();
	_controller->TFController().SetVisualization(index);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::updateTFVisualizationMenu()
{
	Gtk::Menu& menu = _menu->VisualizationMenu();
	std::vector<Gtk::Widget*> children = menu.get_children();
	for(Gtk::Widget* child : children)
		menu.remove(*child);
	
	_tfVisualizationMenuItems.clear();
	Gtk::RadioButtonGroup group;
	for(size_t i=0; i!=_controller->TFController().VisualizationCount(); ++i)
	{
		std::string label = _controller->TFController().GetVisualizationLabel(i);
		_tfVisualizationMenuItems.emplace_back(std::unique_ptr<Gtk::RadioMenuItem>(new Gtk::RadioMenuItem(group, label)));
		Gtk::RadioMenuItem& item = *_tfVisualizationMenuItems.back();
		item.signal_activate().connect(sigc::mem_fun(*this, &RFIGuiWindow::onSelectImage));
		menu.add(item);
	}
	
	_menu->SetSelectVisualizationSensitive(_tfVisualizationMenuItems.size() > 1);
	
	_tfVisualizationMenuItems.front()->activate();
	menu.show_all_children();
}

void RFIGuiWindow::onToggleImage()
{
	size_t index = getActiveTFVisualization();
	++index;
	if(index == _tfVisualizationMenuItems.size())
		index = 0;
	_tfVisualizationMenuItems[index]->set_active(true);
}

size_t RFIGuiWindow::getActiveTFVisualization()
{
	for(size_t index=0; index!=_tfVisualizationMenuItems.size(); ++index)
	{
		if(_tfVisualizationMenuItems[index]->get_active())
			return index;
	}
	return 0;
}
