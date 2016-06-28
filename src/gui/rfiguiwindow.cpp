#include "rfiguiwindow.h"

#include <gtkmm/aboutdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/icontheme.h>

#include "../msio/baselinematrixloader.h"

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"
#include "../structures/spatialmatrixmetadata.h"

#include "../strategy/actions/strategy.h"

#include "../strategy/control/artifactset.h"
#include "../strategy/control/defaultstrategy.h"

#include "../strategy/imagesets/msimageset.h"
#include "../strategy/imagesets/spatialmsimageset.h"
#include "../strategy/imagesets/spatialtimeimageset.h"

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

#include "../imaging/model.h"
#include "../imaging/observatorium.h"

#include "../quality/histogramcollection.h"

#include "../version.h"

#include <iostream>

RFIGuiWindow::RFIGuiWindow() : 
	_controller(new RFIGuiController(*this, this)),
	_mainVBox(Gtk::ORIENTATION_VERTICAL),
	_imagePlaneWindow(0), _histogramWindow(0), _optionWindow(0), _editStrategyWindow(0),
	_gotoWindow(0), _progressWindow(0), _highlightWindow(0), _plotComplexPlaneWindow(0),
	_imagePropertiesWindow(0),
	_imageSet(0),
	_imageSetIndex(0),
	_gaussianTestSets(true),
	_spatialMetaData(0),
	_plotWindow(new PlotWindow(_controller->PlotManager()))
{
	createToolbar();

	_mainVBox.pack_start(_timeFrequencyWidget, Gtk::PACK_EXPAND_WIDGET);
	_timeFrequencyWidget.OnMouseMovedEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFWidgetMouseMoved));
	_timeFrequencyWidget.OnMouseLeaveEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::setSetNameInStatusBar));
	_timeFrequencyWidget.OnButtonReleasedEvent().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFWidgetButtonReleased));
	_timeFrequencyWidget.OnZoomChanged().connect(sigc::mem_fun(*this, &RFIGuiWindow::onTFZoomChanged));
	_timeFrequencyWidget.SetShowXAxisDescription(false);
	_timeFrequencyWidget.SetShowYAxisDescription(false);
	_timeFrequencyWidget.SetShowZAxisDescription(false);
	_timeFrequencyWidget.show();

	_mainVBox.pack_end(_statusbar, Gtk::PACK_SHRINK);
	_statusbar.push("Ready. For suggestions, contact offringa@gmail.com .");
	_statusbar.show();

	add(_mainVBox);
	_mainVBox.show();

	set_default_size(800,600);
	set_default_icon_name("aoflagger");

	_strategy = rfiStrategy::DefaultStrategy::CreateStrategy(
		rfiStrategy::DefaultStrategy::GENERIC_TELESCOPE,
		rfiStrategy::DefaultStrategy::FLAG_GUI_FRIENDLY);
	_imagePlaneWindow = new ImagePlaneWindow();
	
	onTFZoomChanged();
	
	_controller->SignalStateChange().connect(
		sigc::mem_fun(*this, &RFIGuiWindow::onControllerStateChange));
}

RFIGuiWindow::~RFIGuiWindow()
{
	boost::mutex::scoped_lock lock(_ioMutex);
	while(!_actionGroup->get_actions().empty())
		_actionGroup->remove(*_actionGroup->get_actions().begin());
	
	delete _imagePlaneWindow;
	delete _plotWindow;
	if(_histogramWindow != 0)
		delete _histogramWindow;
	if(_optionWindow != 0)
		delete _optionWindow;
	if(_editStrategyWindow != 0)
		delete _editStrategyWindow;
	if(_gotoWindow != 0)
		delete _gotoWindow;
	if(_progressWindow != 0)
		delete _progressWindow;
	if(_highlightWindow != 0)
		delete _highlightWindow;
	if(_imagePropertiesWindow != 0)
		delete _imagePropertiesWindow;
	
	// The rfistrategy needs the lock to clean up
	lock.unlock();
	
	delete _strategy;
	if(HasImageSet())
	{
		delete _imageSetIndex;
		delete _imageSet;
	}
	if(_spatialMetaData != 0)
		delete _spatialMetaData;
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
		OpenPath(dialog.get_filename());
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
		boost::mutex::scoped_lock lock(_ioMutex);
		rfiStrategy::SpatialMSImageSet *imageSet = new rfiStrategy::SpatialMSImageSet(dialog.get_filename());
		imageSet->Initialize();
		lock.unlock();
		SetImageSet(imageSet, true);
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
		boost::mutex::scoped_lock lock(_ioMutex);
		rfiStrategy::SpatialTimeImageSet *imageSet = new rfiStrategy::SpatialTimeImageSet(dialog.get_filename());
		imageSet->Initialize();
		lock.unlock();
		SetImageSet(imageSet, true);
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
		OpenPath(dialog.get_filename());
	}
}

void RFIGuiWindow::OpenPath(const std::string &path)
{
	if(_optionWindow != 0)
		delete _optionWindow;
	if(rfiStrategy::ImageSet::IsMSFile(path))
	{
		_optionWindow = new MSOptionWindow(*_controller, path);
		_optionWindow->present();
	}
	else {
		boost::mutex::scoped_lock lock(_ioMutex);
		rfiStrategy::ImageSet *imageSet = rfiStrategy::ImageSet::Create(path, DirectReadMode);
		imageSet->Initialize();
		lock.unlock();
		
		rfiStrategy::DefaultStrategy::TelescopeId telescopeId;
		unsigned flags;
		double frequency, timeResolution, frequencyResolution;
		rfiStrategy::DefaultStrategy::DetermineSettings(*imageSet, telescopeId, flags, frequency, timeResolution, frequencyResolution);
		Strategy().RemoveAll();
		rfiStrategy::DefaultStrategy::LoadStrategy(
			Strategy(),
			telescopeId,
			flags | rfiStrategy::DefaultStrategy::FLAG_GUI_FRIENDLY,
			frequency,
			timeResolution,
			frequencyResolution
		);
		NotifyChange();
		
		SetImageSet(imageSet, true);
	}
}

void RFIGuiWindow::onToggleFlags()
{
	_controller->SetShowOriginalFlags(_originalFlagsButton->get_active());
	_controller->SetShowAlternativeFlags(_altFlagsButton->get_active());
}

void RFIGuiWindow::loadCurrentTFData()
{
	if(_imageSet != 0) {
		try {
			boost::mutex::scoped_lock lock(_ioMutex);
			_imageSet->AddReadRequest(*_imageSetIndex);
			_imageSet->PerformReadRequests();
			rfiStrategy::BaselineData *baseline = _imageSet->GetNextRequested();
			lock.unlock();
			
			_timeFrequencyWidget.SetNewData(baseline->Data(), baseline->MetaData());
			delete baseline;
			if(_spatialMetaData != 0)
			{
				delete _spatialMetaData;
				_spatialMetaData = 0;
			}
			if(dynamic_cast<rfiStrategy::SpatialMSImageSet*>(_imageSet) != 0)
			{
				_spatialMetaData = new SpatialMatrixMetaData(static_cast<rfiStrategy::SpatialMSImageSet*>(_imageSet)->SpatialMetaData(*_imageSetIndex));
			}
			// Disable forward/back buttons when only one baseline is available
			rfiStrategy::ImageSetIndex* firstIndex = _imageSet->StartIndex();
			firstIndex->Next();
			bool multipleBaselines = firstIndex->IsValid();
			delete firstIndex;
			_previousButton->set_sensitive(multipleBaselines);
			_reloadButton->set_sensitive(true);
			_nextButton->set_sensitive(multipleBaselines);
			
			// We store these seperate, as they might access the measurement set. This is
			// not only faster (the names are used in the onMouse.. events) but also less dangerous,
			// since the set can be simultaneously accessed by another thread. (thus the io mutex should
			// be locked before calling below statements).
			lock.lock();
			_imageSetName = _imageSet->Name();
			_imageSetIndexDescription = _imageSetIndex->Description();
			lock.unlock();
			
			_timeFrequencyWidget.SetTitleText(_imageSetIndexDescription);
			_timeFrequencyWidget.Update();
			
			setSetNameInStatusBar();
		} catch(std::exception &e)
		{
			AOLogger::Error << e.what() << '\n';
			showError(e.what());
		}
	}
}

void RFIGuiWindow::setSetNameInStatusBar()
{
  if(HasImageSet()) {
		_statusbar.pop();
		_statusbar.push(_imageSetName + ": " + _imageSetIndexDescription);
  }
}
		
void RFIGuiWindow::onLoadPrevious()
{
	if(_imageSet != 0) {
		boost::mutex::scoped_lock lock(_ioMutex);
		_imageSetIndex->Previous();
		lock.unlock();
		loadCurrentTFData();
	}
}

void RFIGuiWindow::onLoadNext()
{
	if(_imageSet != 0) {
		boost::mutex::scoped_lock lock(_ioMutex);
		_imageSetIndex->Next();
		lock.unlock();
		loadCurrentTFData();
	}
}

void RFIGuiWindow::onEditStrategyPressed()
{
	if(_editStrategyWindow != 0)
		delete _editStrategyWindow;
	_editStrategyWindow = new EditStrategyWindow(*this);
	_editStrategyWindow->show();
}

void RFIGuiWindow::onExecuteStrategyPressed()
{
	if(_progressWindow != 0)
		delete _progressWindow;

	ProgressWindow *window = new ProgressWindow(*this);
	_progressWindow = window;
	_progressWindow->show();

	rfiStrategy::ArtifactSet artifacts(&_ioMutex);

	artifacts.SetAntennaFlagCountPlot(new AntennaFlagCountPlot());
	artifacts.SetFrequencyFlagCountPlot(new FrequencyFlagCountPlot());
	artifacts.SetFrequencyPowerPlot(new FrequencyPowerPlot());
	artifacts.SetTimeFlagCountPlot(new TimeFlagCountPlot());
	artifacts.SetIterationsPlot(new IterationsPlot());
	
	artifacts.SetPolarizationStatistics(new PolarizationStatistics());
	artifacts.SetBaselineSelectionInfo(new rfiStrategy::BaselineSelector());
	artifacts.SetImager(_imagePlaneWindow->GetImager());

	if(HasImage())
	{
		artifacts.SetOriginalData(GetOriginalData());
		artifacts.SetContaminatedData(GetContaminatedData());
		TimeFrequencyData *zero = new TimeFrequencyData(GetOriginalData());
		zero->SetImagesToZero();
		artifacts.SetRevisedData(*zero);
		delete zero;
	}
	if(_timeFrequencyWidget.GetFullMetaData() != 0)
			artifacts.SetMetaData(_timeFrequencyWidget.GetFullMetaData());
	if(HasImageSet())
	{
		artifacts.SetImageSet(_imageSet);
		artifacts.SetImageSetIndex(_imageSetIndex);
	}
	_strategy->InitializeAll();
	try {
		_strategy->StartPerformThread(artifacts, *window);
	}  catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onExecuteStrategyFinished()
{
	rfiStrategy::ArtifactSet *artifacts = _strategy->JoinThread();
	if(artifacts != 0)
	{
		bool update = false;
		if(!artifacts->RevisedData().IsEmpty())
		{
			std::cout << "Updating revised data..." << std::endl;
			_timeFrequencyWidget.SetRevisedData(artifacts->RevisedData());
			update = true;
		}

		if(!artifacts->ContaminatedData().IsEmpty())
		{
			std::cout << "Updating contaminated data..." << std::endl;
			_timeFrequencyWidget.SetContaminatedData(artifacts->ContaminatedData());
			update = true;
		}
		
		if(update)
			_timeFrequencyWidget.Update();
		
		_imagePlaneWindow->Update();
		
		if(artifacts->AntennaFlagCountPlot()->HasData())
			artifacts->AntennaFlagCountPlot()->MakePlot();
		if(artifacts->FrequencyFlagCountPlot()->HasData())
			artifacts->FrequencyFlagCountPlot()->MakePlot();
		if(artifacts->FrequencyPowerPlot()->HasData())
			artifacts->FrequencyPowerPlot()->MakePlot();
		if(artifacts->TimeFlagCountPlot()->HasData())
			artifacts->TimeFlagCountPlot()->MakePlot();
		if(artifacts->PolarizationStatistics()->HasData())
			artifacts->PolarizationStatistics()->Report();
		if(artifacts->IterationsPlot()->HasData())
			artifacts->IterationsPlot()->MakePlot();

		delete artifacts->AntennaFlagCountPlot();
		delete artifacts->FrequencyFlagCountPlot();
		delete artifacts->FrequencyPowerPlot();
		delete artifacts->TimeFlagCountPlot();
		delete artifacts->PolarizationStatistics();
		delete artifacts->BaselineSelectionInfo();
		delete artifacts->IterationsPlot();
		delete artifacts;
	}
}

void RFIGuiWindow::onToggleImage()
{
	ImageComparisonWidget::TFImage image = ImageComparisonWidget::TFOriginalImage;
	if(_backgroundImageButton->get_active())
		image = ImageComparisonWidget::TFRevisedImage;
	else if(_diffImageButton->get_active())
		image = ImageComparisonWidget::TFContaminatedImage;
	_timeFrequencyWidget.SetVisualizedImage(image);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::SetImageSet(rfiStrategy::ImageSet *newImageSet, bool loadBaseline)
{
	if(_imageSet != 0) {
		delete _imageSet;
		delete _imageSetIndex;
	}
	_imageSet = newImageSet;
	_imageSetIndex = _imageSet->StartIndex();
	
	if(loadBaseline)
	{
		if(dynamic_cast<rfiStrategy::MSImageSet*>(newImageSet) != 0)
		{
			onGoToPressed();
		} else {
			loadCurrentTFData();
		}
	}
}

void RFIGuiWindow::SetImageSetIndex(rfiStrategy::ImageSetIndex *newImageSetIndex)
{
	if(HasImageSet())
	{
		delete _imageSetIndex;
		_imageSetIndex = newImageSetIndex;
		_imageSetIndexDescription = _imageSetIndex->Description();
		loadCurrentTFData();
	} else {
		delete newImageSetIndex;
	}
}

void RFIGuiWindow::openTestSet(unsigned index)
{
	_controller->OpenTestSet(index, _gaussianTestSets);
}

void RFIGuiWindow::createToolbar()
{
	Glib::RefPtr<Gtk::Action> action;
	
	_actionGroup = Gtk::ActionGroup::create();
	_actionGroup->add( Gtk::Action::create("MenuFile", "_File") );
	_actionGroup->add( Gtk::Action::create("MenuBrowse", "_Browse") );
	_actionGroup->add( Gtk::Action::create("MenuView", "_View") );
	_actionGroup->add( Gtk::Action::create("MenuPlot", "_Plot") );
	_actionGroup->add( Gtk::Action::create("MenuSimulate", "_Simulate") );
	_actionGroup->add( Gtk::Action::create("MenuPlotFlagComparison", "_Compare flags") );
	_actionGroup->add( Gtk::Action::create("MenuActions", "_Actions") );
	_actionGroup->add( Gtk::Action::create("MenuData", "_Data") );
	_actionGroup->add( Gtk::Action::create("MenuHelp", "_Help") );
	
	action = Gtk::Action::create("OpenFile", "Open _file");
	action->set_icon_name("document-open");
	_actionGroup->add(action, Gtk::AccelKey("<control>O"),
		sigc::mem_fun(*this, &RFIGuiWindow::onActionFileOpen) );
	action = Gtk::Action::create("OpenDirectory", "Open _directory");
	action->set_icon_name("folder");
	action->set_tooltip("Open a directory. This action should be used to open a measurement set. For opening files (e.g. sdfits files), select 'Open file' instead.");
	_actionGroup->add(action, Gtk::AccelKey("<control>D"),
		sigc::mem_fun(*this, &RFIGuiWindow::onActionDirectoryOpen) );
	_actionGroup->add( Gtk::Action::create("OpenDirectorySpatial", "Open _directory as spatial"),
  sigc::mem_fun(*this, &RFIGuiWindow::onActionDirectoryOpenForSpatial) );
	_actionGroup->add( Gtk::Action::create("OpenDirectoryST", "Open _directory as spatial/time"),
  sigc::mem_fun(*this, &RFIGuiWindow::onActionDirectoryOpenForST) );
	_actionGroup->add( Gtk::Action::create("OpenTestSet", "Open _testset") );

	Gtk::RadioButtonGroup testSetGroup;
	_gaussianTestSetsButton = Gtk::RadioAction::create(testSetGroup, "GaussianTestSets", "Gaussian");
	_gaussianTestSetsButton->set_active(true);
	_rayleighTestSetsButton = Gtk::RadioAction::create(testSetGroup, "RayleighTestSets", "Rayleigh");
	_zeroTestSetsButton = Gtk::RadioAction::create(testSetGroup, "ZeroTestSets", "Zero");
	_actionGroup->add(_gaussianTestSetsButton, sigc::mem_fun(*this, &RFIGuiWindow::onGaussianTestSets) );
	_actionGroup->add(_rayleighTestSetsButton, sigc::mem_fun(*this, &RFIGuiWindow::onRayleighTestSets) );
	_actionGroup->add(_zeroTestSetsButton, sigc::mem_fun(*this, &RFIGuiWindow::onZeroTestSets) );
	
	_actionGroup->add( Gtk::Action::create("OpenTestSetA", "Test set A"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetA) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetB", "Test set B"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetB) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetC", "Test set C"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetC) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetD", "Test set D"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetD) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetE", "Test set E"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetE) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetF", "Test set F"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetF) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetG", "Test set G"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetG) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetH", "Test set H"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetH) );
	_actionGroup->add( Gtk::Action::create("OpenTestSetNoise", "Noise"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetNoise));
	_actionGroup->add( Gtk::Action::create("OpenTestSetModel3", "3-source model"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSet3Model));
	_actionGroup->add( Gtk::Action::create("OpenTestSetModel5", "5-source model"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSet5Model));
	_actionGroup->add( Gtk::Action::create("OpenTestSetNoiseModel3", "3-source model with noise"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetNoise3Model));
	_actionGroup->add( Gtk::Action::create("OpenTestSetNoiseModel5", "5-source model with noise"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetNoise5Model));
	_actionGroup->add( Gtk::Action::create("OpenTestSetBStrong", "Test set B (strong RFI)"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetBStrong));
	_actionGroup->add( Gtk::Action::create("OpenTestSetBWeak", "Test set B (weak RFI)"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetBWeak));
	_actionGroup->add( Gtk::Action::create("OpenTestSetBAligned", "Test set B (aligned)"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetBAligned));
	_actionGroup->add( Gtk::Action::create("OpenTestSetGaussianBroadband", "Gaussian broadband"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetGaussianBroadband));
	_actionGroup->add( Gtk::Action::create("OpenTestSetSinusoidalBroadband", "Sinusoidal broadband"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetSinusoidalBroadband));
	_actionGroup->add( Gtk::Action::create("OpenTestSetSlewedGaussianBroadband", "Slewed Gaussian"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetSlewedGaussianBroadband));
	_actionGroup->add( Gtk::Action::create("OpenTestSetBurstBroadband", "Burst"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetBurstBroadband));
	_actionGroup->add( Gtk::Action::create("OpenTestSetRFIDistributionLow", "Slope -2 dist low"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetRFIDistributionLow));
	_actionGroup->add( Gtk::Action::create("OpenTestSetRFIDistributionMid", "Slope -2 dist mid"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetRFIDistributionMid));
	_actionGroup->add( Gtk::Action::create("OpenTestSetRFIDistributionHigh", "Slope -2 dist high"),
	sigc::mem_fun(*this, &RFIGuiWindow::onOpenTestSetRFIDistributionHigh));
	_actionGroup->add( Gtk::Action::create("AddTestModification", "Test modify") );
	_actionGroup->add( Gtk::Action::create("AddStaticFringe", "Static fringe"),
	sigc::mem_fun(*this, &RFIGuiWindow::onAddStaticFringe) );
	_actionGroup->add( Gtk::Action::create("Add1SigmaStaticFringe", "Static 1 sigma fringe"),
	sigc::mem_fun(*this, &RFIGuiWindow::onAdd1SigmaFringe) );
	_actionGroup->add( Gtk::Action::create("SetToOne", "Set to 1"),
	sigc::mem_fun(*this, &RFIGuiWindow::onSetToOne) );
	_actionGroup->add( Gtk::Action::create("SetToI", "Set to i"),
	sigc::mem_fun(*this, &RFIGuiWindow::onSetToI) );
	_actionGroup->add( Gtk::Action::create("SetToOnePlusI", "Set to 1+i"),
	sigc::mem_fun(*this, &RFIGuiWindow::onSetToOnePlusI) );
	_actionGroup->add( Gtk::Action::create("MultiplyData", "Multiply data..."),
	sigc::mem_fun(*this, &RFIGuiWindow::onMultiplyData) );
	action = Gtk::Action::create("Quit", "_Quit");
	action->set_icon_name("application-exit");
	_actionGroup->add(action, Gtk::AccelKey("<control>Q"),
		sigc::mem_fun(*this, &RFIGuiWindow::onQuit) );

	_actionGroup->add( Gtk::Action::create("ImageProperties", "Plot properties..."),
		Gtk::AccelKey("<control>P"),
  	sigc::mem_fun(*this, &RFIGuiWindow::onImagePropertiesPressed) );
	_timeGraphButton = Gtk::ToggleAction::create("TimeGraph", "Time graph");
	_timeGraphButton->set_active(false); 
	_actionGroup->add(_timeGraphButton, sigc::mem_fun(*this, &RFIGuiWindow::onTimeGraphButtonPressed) );
	
	_actionGroup->add( Gtk::Action::create("PlotDist", "Plot _distribution"),
  sigc::mem_fun(*this, &RFIGuiWindow::onPlotDistPressed) );
	_actionGroup->add( Gtk::Action::create("PlotLogLogDist", "Plot _log-log dist"),
  sigc::mem_fun(*this, &RFIGuiWindow::onPlotLogLogDistPressed) );
	_actionGroup->add( Gtk::Action::create("PlotComplexPlane", "Plot _complex plane"),
		Gtk::AccelKey("<alt>C"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotComplexPlanePressed) );
	_actionGroup->add( Gtk::Action::create("PlotMeanSpectrum", "Plot _mean spectrum"),
		Gtk::AccelKey("<alt>M"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotMeanSpectrumPressed) );
	_actionGroup->add( Gtk::Action::create("PlotSumSpectrum", "Plot s_um spectrum"),
		Gtk::AccelKey("<alt>U"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotSumSpectrumPressed) );
	_actionGroup->add( Gtk::Action::create("PlotPowerSpectrum", "Plot _power spectrum"),
		Gtk::AccelKey("<alt>W"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerSpectrumPressed) );
	_actionGroup->add( Gtk::Action::create("PlotPowerSpectrumComparison", "Power _spectrum"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerSpectrumComparisonPressed) );
	_actionGroup->add( Gtk::Action::create("PlotRMSSpectrum", "Plot _rms spectrum"),
		Gtk::AccelKey("<alt>R"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerRMSPressed) );
	_actionGroup->add( Gtk::Action::create("PlotSNRSpectrum", "Plot spectrum snr"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerSNRPressed) );
	_actionGroup->add( Gtk::Action::create("PlotPowerTime", "Plot power vs _time"),
		Gtk::AccelKey("<alt>T"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerTimePressed) );
	_actionGroup->add( Gtk::Action::create("PlotPowerTimeComparison", "Po_wer vs time"),
  sigc::mem_fun(*this, &RFIGuiWindow::onPlotPowerTimeComparisonPressed) );
	_actionGroup->add( Gtk::Action::create("PlotTimeScatter", "Plot t_ime scatter"),
 		Gtk::AccelKey("<alt>I"),
		sigc::mem_fun(*this, &RFIGuiWindow::onPlotTimeScatterPressed) );
	_actionGroup->add( Gtk::Action::create("PlotTimeScatterComparison", "Time _scatter"),
  sigc::mem_fun(*this, &RFIGuiWindow::onPlotTimeScatterComparisonPressed) );
	_actionGroup->add( Gtk::Action::create("PlotSingularValues", "Plot _singular values"),
  sigc::mem_fun(*this, &RFIGuiWindow::onPlotSingularValuesPressed) );
	_zoomToFitButton = Gtk::Action::create("ZoomFit", "Zoom _fit");
	_zoomToFitButton->set_icon_name("zoom-fit-best");
	_actionGroup->add(_zoomToFitButton, Gtk::AccelKey("<control>0"),
		sigc::mem_fun(*this, &RFIGuiWindow::onZoomFit) );
	_zoomInButton = Gtk::Action::create("ZoomIn", "Zoom in");
	_zoomInButton->set_icon_name("zoom-in");
	_actionGroup->add(_zoomInButton, Gtk::AccelKey("<control>equal"),
		sigc::mem_fun(*this, &RFIGuiWindow::onZoomIn) );
	_zoomOutButton = Gtk::Action::create("ZoomOut", "Zoom out");
	_zoomOutButton->set_icon_name("zoom-out");
	_actionGroup->add(_zoomOutButton, Gtk::AccelKey("<control>minus"),
	sigc::mem_fun(*this, &RFIGuiWindow::onZoomOut) );
	_actionGroup->add( Gtk::Action::create("ShowImagePlane", "_Show image plane"),
		Gtk::AccelKey("<control>I"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowImagePlane) );
	_actionGroup->add( Gtk::Action::create("SetAndShowImagePlane", "S_et & show image plane"),
		Gtk::AccelKey("<control><shift>I"),
		sigc::mem_fun(*this, &RFIGuiWindow::onSetAndShowImagePlane) );
	_actionGroup->add( Gtk::Action::create("AddToImagePlane", "Add to _image plane"),
  sigc::mem_fun(*this, &RFIGuiWindow::onAddToImagePlane) );
	
	Gtk::RadioButtonGroup setGroup;
	_ncpSetButton = Gtk::RadioAction::create(setGroup, "NCPSet", "Use NCP set");
	_b1834SetButton = Gtk::RadioAction::create(setGroup, "B1834Set", "Use B1834 set");
	_emptySetButton = Gtk::RadioAction::create(setGroup, "EmptySet", "Use empty set");
	_ncpSetButton->set_active(true); 
	_actionGroup->add(_ncpSetButton);
	_actionGroup->add(_b1834SetButton);
	_actionGroup->add(_emptySetButton);
	
	Gtk::RadioButtonGroup chGroup;
	_sim16ChannelsButton = Gtk::RadioAction::create(chGroup, "Sim16Channels", "16 channels");
	_sim64ChannelsButton = Gtk::RadioAction::create(chGroup, "Sim64Channels", "64 channels");
	_sim256ChannelsButton = Gtk::RadioAction::create(chGroup, "Sim256Channels", "256 channels");
	_sim64ChannelsButton->set_active(true); 
	_actionGroup->add(_sim16ChannelsButton);
	_actionGroup->add(_sim64ChannelsButton);
	_actionGroup->add(_sim256ChannelsButton);
	
	_simFixBandwidthButton = Gtk::ToggleAction::create("SimFixBandwidth", "Fix bandwidth");
	_simFixBandwidthButton->set_active(false); 
	_actionGroup->add(_simFixBandwidthButton);
	
	_actionGroup->add( Gtk::Action::create("SimulateCorrelation", "Simulate correlation"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateCorrelation) );
	_actionGroup->add( Gtk::Action::create("SimulateSourceSetA", "Simulate source set A"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateSourceSetA) );
	_actionGroup->add( Gtk::Action::create("SimulateSourceSetB", "Simulate source set B"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateSourceSetB) );
	_actionGroup->add( Gtk::Action::create("SimulateSourceSetC", "Simulate source set C"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateSourceSetC) );
	_actionGroup->add( Gtk::Action::create("SimulateSourceSetD", "Simulate source set D"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateSourceSetD) );
	_actionGroup->add( Gtk::Action::create("SimulateOffAxisSource", "Simulate off-axis source"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateOffAxisSource) );
	_actionGroup->add( Gtk::Action::create("SimulateOnAxisSource", "Simulate on-axis source"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSimulateOnAxisSource) );

	_actionGroup->add( Gtk::Action::create("EditStrategy", "_Edit strategy"),
		Gtk::AccelKey("F8"),
  sigc::mem_fun(*this, &RFIGuiWindow::onEditStrategyPressed) );
	action = Gtk::Action::create("ExecuteStrategy", "E_xecute strategy");
	action->set_tooltip("Run the currently loaded strategy. Normally this will not write back the results to the opened set. The flagging results are displayed in the plot as yellow ('alternative') flag mask.");
	action->set_icon_name("system-run");
	_actionGroup->add(action, Gtk::AccelKey("F9"),
			sigc::mem_fun(*this, &RFIGuiWindow::onExecuteStrategyPressed));
	_actionGroup->add(Gtk::Action::create("ShowStats", "Show _stats"),
		Gtk::AccelKey("F2"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowStats) );
	_previousButton = Gtk::Action::create("Previous", "Previous");
	_previousButton->set_icon_name("go-previous");
	_previousButton->set_tooltip("Load and display the previous baseline. Normally, this steps from the baseline between antennas (i) and (j) to (i) and (j-1).");
	_previousButton->set_sensitive(false);
	
	_actionGroup->add(_previousButton,
		Gtk::AccelKey("F6"),
		sigc::mem_fun(*this, &RFIGuiWindow::onLoadPrevious) );
	_nextButton = Gtk::Action::create("Next", "Next");
	_nextButton->set_icon_name("go-next");
	_nextButton->set_tooltip("Load and display the next baseline. Normally, this steps from the baseline between antennas (i) and (j) to (i) and (j+1).");
	_nextButton->set_sensitive(false);
	_actionGroup->add(_nextButton,
		Gtk::AccelKey("F7"),
		sigc::mem_fun(*this, &RFIGuiWindow::onLoadNext) );
	_reloadButton = Gtk::Action::create("Reload", "_Reload");
	_reloadButton->set_icon_name("view-refresh");
	_reloadButton->set_tooltip("Reload the currently displayed baseline. This will reset the purple flags to the measurement set flags, and clear the yellow flags.");
	_reloadButton->set_sensitive(false);
	_actionGroup->add(_reloadButton, Gtk::AccelKey("F5"),
  sigc::mem_fun(*this, &RFIGuiWindow::onReloadPressed) );
	_actionGroup->add( Gtk::Action::create("GoTo", "_Go to..."),
		Gtk::AccelKey("<control>G"),
  sigc::mem_fun(*this, &RFIGuiWindow::onGoToPressed) );
	_actionGroup->add( Gtk::Action::create("LoadLongestBaseline", "Longest baseline"),
		sigc::mem_fun(*this, &RFIGuiWindow::onLoadLongestBaselinePressed) );
	_actionGroup->add( Gtk::Action::create("LoadShortestBaseline", "Shortest baseline"),
		sigc::mem_fun(*this, &RFIGuiWindow::onLoadShortestBaselinePressed) );
	
  _originalFlagsButton = Gtk::ToggleAction::create("OriginalFlags", "Or flags");
	_originalFlagsButton->set_active(true);
	_originalFlagsButton->set_icon_name("showoriginalflags");
	_originalFlagsButton->set_tooltip("Display the first flag mask on top of the visibilities. These flags are displayed in purple and indicate the flags as they originally were stored in the measurement set.");
	_actionGroup->add(_originalFlagsButton,
			Gtk::AccelKey("F3"),
			sigc::mem_fun(*this, &RFIGuiWindow::onToggleFlags));
  _altFlagsButton = Gtk::ToggleAction::create("AlternativeFlags", "Alt flags");
	_altFlagsButton->set_active(true); 
	_altFlagsButton->set_icon_name("showalternativeflags");
	_altFlagsButton->set_tooltip("Display the second flag mask on top of the visibilities. These flags are displayed in yellow and indicate flags found by running the strategy.");
	_actionGroup->add(_altFlagsButton,
			Gtk::AccelKey("F4"),
			sigc::mem_fun(*this, &RFIGuiWindow::onToggleFlags));
	_actionGroup->add( Gtk::Action::create("ClearAltFlags", "Clear alt flags"),
  sigc::mem_fun(*this, &RFIGuiWindow::onClearAltFlagsPressed) );

	Gtk::RadioButtonGroup imageGroup;
	_originalImageButton = Gtk::RadioAction::create(imageGroup, "ImageOriginal", "Original");
	_originalImageButton->set_active(true);
	_originalImageButton->set_icon_name("showoriginalvisibilities");
	_originalImageButton->set_tooltip("Display the original visibilities (before any processing)");
	_backgroundImageButton = Gtk::RadioAction::create(imageGroup, "ImageBackground", "Background");
	_backgroundImageButton->set_icon_name("showsmoothedvisibilities");
	_backgroundImageButton->set_tooltip("Display the smoothed visibilities (only available if strategy has run and has created smoothed visibilities)");
	_diffImageButton = Gtk::RadioAction::create(imageGroup, "ImageDiff", "Difference");
	_diffImageButton->set_icon_name("showresidualvisibilities");
	_diffImageButton->set_tooltip("Display the residual visibilities (only available if strategy has run and has created residual visibilities)");
	_actionGroup->add(_originalImageButton,
		Gtk::AccelKey("<control>1"),
		sigc::mem_fun(*this, &RFIGuiWindow::onToggleImage) );
	_actionGroup->add(_backgroundImageButton,
		Gtk::AccelKey("<control>2"),
		sigc::mem_fun(*this, &RFIGuiWindow::onToggleImage) );
	_actionGroup->add(_diffImageButton,
		Gtk::AccelKey("<control>3"),
		sigc::mem_fun(*this, &RFIGuiWindow::onToggleImage) );

	_actionGroup->add( Gtk::Action::create("DiffToOriginal", "Diff->Original"),
  sigc::mem_fun(*this, &RFIGuiWindow::onDifferenceToOriginalPressed) );
	_actionGroup->add( Gtk::Action::create("BackToOriginal", "Background->Original"),
  sigc::mem_fun(*this, &RFIGuiWindow::onBackgroundToOriginalPressed) );

	_actionGroup->add( Gtk::Action::create("ShowReal", "Keep _real part"),
		Gtk::AccelKey("<control>,"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowRealPressed) );
	_actionGroup->add( Gtk::Action::create("ShowImaginary", "Keep _imaginary part"),
		Gtk::AccelKey("<control>."),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowImaginaryPressed) );
	_actionGroup->add( Gtk::Action::create("ShowPhase", "Keep _phase part"),
		Gtk::AccelKey("<control>1"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowPhasePressed) );
	_actionGroup->add( Gtk::Action::create("ShowStokesI", "Keep _stokesI part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowStokesIPressed) );
	_actionGroup->add( Gtk::Action::create("ShowStokesQ", "Keep stokes_Q part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowStokesQPressed) );
	_actionGroup->add( Gtk::Action::create("ShowStokesU", "Keep stokes_U part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowStokesUPressed) );
	_actionGroup->add( Gtk::Action::create("ShowStokesV", "Keep stokes_V part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowStokesVPressed) );
	_actionGroup->add( Gtk::Action::create("ShowAutoPol", "Keep xx+yy part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowAutoDipolePressed) );
	_actionGroup->add( Gtk::Action::create("ShowCrossPol", "Keep xy+yx part"),
  sigc::mem_fun(*this, &RFIGuiWindow::onShowCrossDipolePressed) );
	_actionGroup->add( Gtk::Action::create("ShowXX", "Keep _xx part"),
		Gtk::AccelKey("<control>X"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowXXPressed) );
	_actionGroup->add( Gtk::Action::create("ShowXY", "Keep xy part"),
		Gtk::AccelKey("<control><alt>X"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowXYPressed) );
	_actionGroup->add( Gtk::Action::create("ShowYX", "Keep yx part"),
		Gtk::AccelKey("<control><alt>Y"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowYXPressed) );
	_actionGroup->add( Gtk::Action::create("ShowYY", "Keep _yy part"),
		Gtk::AccelKey("<control>Y"),
		sigc::mem_fun(*this, &RFIGuiWindow::onShowYYPressed) );
	_actionGroup->add( Gtk::Action::create("UnrollPhase", "_Unroll phase"),
	sigc::mem_fun(*this, &RFIGuiWindow::onUnrollPhaseButtonPressed) );

	_actionGroup->add( Gtk::Action::create("Segment", "Segment"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSegment) );
	_actionGroup->add( Gtk::Action::create("Cluster", "Cluster"),
  sigc::mem_fun(*this, &RFIGuiWindow::onCluster) );
	_actionGroup->add( Gtk::Action::create("Classify", "Classify"),
  sigc::mem_fun(*this, &RFIGuiWindow::onClassify) );
	_actionGroup->add( Gtk::Action::create("RemoveSmallSegments", "Remove small segments"),
  sigc::mem_fun(*this, &RFIGuiWindow::onRemoveSmallSegments) );
	_actionGroup->add( Gtk::Action::create("StoreData", "Store"),
		Gtk::AccelKey("<control>M"),
  sigc::mem_fun(*this, &RFIGuiWindow::onStoreData) );
	_actionGroup->add( Gtk::Action::create("RecallData", "Recall"),
		Gtk::AccelKey("<control>R"),
		sigc::mem_fun(*this, &RFIGuiWindow::onRecallData) );
	_actionGroup->add( Gtk::Action::create("SubtractDataFromMem", "Subtract from mem"),
  sigc::mem_fun(*this, &RFIGuiWindow::onSubtractDataFromMem) );

	_actionGroup->add( Gtk::Action::create("Highlight", "Highlight"),
  sigc::mem_fun(*this, &RFIGuiWindow::onHightlightPressed) );
	_actionGroup->add( Gtk::Action::create("VertEVD", "Vert EVD"),
  sigc::mem_fun(*this, &RFIGuiWindow::onVertEVD) );
	_actionGroup->add( Gtk::Action::create("ApplyTimeProfile", "Apply time profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onApplyTimeProfile) );
	_actionGroup->add( Gtk::Action::create("ApplyVertProfile", "Apply vert profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onApplyVertProfile) );
	_actionGroup->add( Gtk::Action::create("RestoreTimeProfile", "Restore time profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onRestoreTimeProfile) );
	_actionGroup->add( Gtk::Action::create("RestoreVertProfile", "Restore vert profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onRestoreVertProfile) );
	_actionGroup->add( Gtk::Action::create("ReapplyTimeProfile", "Reapply time profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onReapplyTimeProfile) );
	_actionGroup->add( Gtk::Action::create("ReapplyVertProfile", "Reapply vert profile"),
  sigc::mem_fun(*this, &RFIGuiWindow::onReapplyVertProfile) );

	action = Gtk::Action::create("About", "_About");
	action->set_icon_name("aoflagger");
	_actionGroup->add(action, sigc::mem_fun(*this, &RFIGuiWindow::onHelpAbout));

	Glib::RefPtr<Gtk::UIManager> uiManager =
		Gtk::UIManager::create();
	uiManager->insert_action_group(_actionGroup);
	add_accel_group(uiManager->get_accel_group());

	Glib::ustring ui_info =
    "<ui>"
    "  <menubar name='MenuBar'>"
    "    <menu action='MenuFile'>"
    "      <menuitem action='OpenFile'/>"
    "      <menuitem action='OpenDirectory'/>"
    "      <menuitem action='OpenDirectorySpatial'/>"
    "      <menuitem action='OpenDirectoryST'/>"
    "      <menu action='OpenTestSet'>"
		"        <menuitem action='GaussianTestSets'/>"
		"        <menuitem action='RayleighTestSets'/>"
		"        <menuitem action='ZeroTestSets'/>"
    "        <separator/>"
		"        <menuitem action='OpenTestSetA'/>"
		"        <menuitem action='OpenTestSetB'/>"
		"        <menuitem action='OpenTestSetC'/>"
		"        <menuitem action='OpenTestSetD'/>"
		"        <menuitem action='OpenTestSetE'/>"
		"        <menuitem action='OpenTestSetF'/>"
		"        <menuitem action='OpenTestSetG'/>"
		"        <menuitem action='OpenTestSetH'/>"
		"        <menuitem action='OpenTestSetNoise'/>"
		"        <menuitem action='OpenTestSetModel3'/>"
		"        <menuitem action='OpenTestSetModel5'/>"
		"        <menuitem action='OpenTestSetNoiseModel3'/>"
		"        <menuitem action='OpenTestSetNoiseModel5'/>"
		"        <menuitem action='OpenTestSetBStrong'/>"
		"        <menuitem action='OpenTestSetBWeak'/>"
		"        <menuitem action='OpenTestSetBAligned'/>"
		"        <menuitem action='OpenTestSetGaussianBroadband'/>"
		"        <menuitem action='OpenTestSetSinusoidalBroadband'/>"
		"        <menuitem action='OpenTestSetSlewedGaussianBroadband'/>"
		"        <menuitem action='OpenTestSetBurstBroadband'/>"
		"        <menuitem action='OpenTestSetRFIDistributionLow'/>"
		"        <menuitem action='OpenTestSetRFIDistributionMid'/>"
		"        <menuitem action='OpenTestSetRFIDistributionHigh'/>"
		"      </menu>"
		"      <menu action='AddTestModification'>"
		"        <menuitem action='AddStaticFringe'/>"
		"        <menuitem action='Add1SigmaStaticFringe'/>"
		"        <menuitem action='SetToOne'/>"
		"        <menuitem action='SetToI'/>"
		"        <menuitem action='SetToOnePlusI'/>"
		"        <menuitem action='MultiplyData'/>"
		"      </menu>"
    "      <menuitem action='Quit'/>"
    "    </menu>"
	  "    <menu action='MenuView'>"
    "      <menuitem action='ImageProperties'/>"
    "      <menuitem action='TimeGraph'/>"
    "      <separator/>"
    "      <menuitem action='OriginalFlags'/>"
    "      <menuitem action='AlternativeFlags'/>"
    "      <menuitem action='Highlight'/>"
    "      <separator/>"
    "      <menuitem action='ZoomFit'/>"
    "      <menuitem action='ZoomIn'/>"
    "      <menuitem action='ZoomOut'/>"
    "      <separator/>"
    "      <menuitem action='ShowImagePlane'/>"
    "      <menuitem action='SetAndShowImagePlane'/>"
    "      <menuitem action='AddToImagePlane'/>"
    "      <separator/>"
    "      <menuitem action='ShowStats'/>"
	  "    </menu>"
	  "    <menu action='MenuPlot'>"
    "      <menu action='MenuPlotFlagComparison'>"
    "        <menuitem action='PlotPowerSpectrumComparison'/>"
    "        <menuitem action='PlotPowerTimeComparison'/>"
    "        <menuitem action='PlotTimeScatterComparison'/>"
		"      </menu>"
    "      <separator/>"
    "      <menuitem action='PlotDist'/>"
    "      <menuitem action='PlotLogLogDist'/>"
    "      <menuitem action='PlotComplexPlane'/>"
    "      <menuitem action='PlotMeanSpectrum'/>"
    "      <menuitem action='PlotSumSpectrum'/>"
    "      <menuitem action='PlotPowerSpectrum'/>"
    "      <menuitem action='PlotRMSSpectrum'/>"
    "      <menuitem action='PlotSNRSpectrum'/>"
    "      <menuitem action='PlotPowerTime'/>"
    "      <menuitem action='PlotTimeScatter'/>"
    "      <menuitem action='PlotSingularValues'/>"
	  "    </menu>"
    "    <menu action='MenuBrowse'>"
    "      <menuitem action='Previous'/>"
    "      <menuitem action='Reload'/>"
    "      <menuitem action='Next'/>"
    "      <separator/>"
    "      <menuitem action='GoTo'/>"
    "      <separator/>"
    "      <menuitem action='LoadLongestBaseline'/>"
    "      <menuitem action='LoadShortestBaseline'/>"
    "    </menu>"
	  "    <menu action='MenuSimulate'>"
    "      <menuitem action='NCPSet'/>"
    "      <menuitem action='B1834Set'/>"
    "      <menuitem action='EmptySet'/>"
    "      <separator/>"
    "      <menuitem action='Sim16Channels'/>"
    "      <menuitem action='Sim64Channels'/>"
    "      <menuitem action='Sim256Channels'/>"
    "      <menuitem action='SimFixBandwidth'/>"
    "      <separator/>"
    "      <menuitem action='SimulateCorrelation'/>"
    "      <menuitem action='SimulateSourceSetA'/>"
    "      <menuitem action='SimulateSourceSetB'/>"
    "      <menuitem action='SimulateSourceSetC'/>"
    "      <menuitem action='SimulateSourceSetD'/>"
    "      <menuitem action='SimulateOffAxisSource'/>"
    "      <menuitem action='SimulateOnAxisSource'/>"
	  "    </menu>"
	  "    <menu action='MenuData'>"
    "      <menuitem action='DiffToOriginal'/>"
    "      <menuitem action='BackToOriginal'/>"
    "      <separator/>"
    "      <menuitem action='ShowReal'/>"
    "      <menuitem action='ShowImaginary'/>"
    "      <menuitem action='ShowPhase'/>"
    "      <separator/>"
    "      <menuitem action='ShowStokesI'/>"
    "      <menuitem action='ShowStokesQ'/>"
    "      <menuitem action='ShowStokesU'/>"
    "      <menuitem action='ShowStokesV'/>"
    "      <separator/>"
    "      <menuitem action='ShowXX'/>"
    "      <menuitem action='ShowXY'/>"
    "      <menuitem action='ShowYX'/>"
    "      <menuitem action='ShowYY'/>"
    "      <menuitem action='ShowAutoPol'/>"
    "      <menuitem action='ShowCrossPol'/>"
    "      <menuitem action='UnrollPhase'/>"
    "      <separator/>"
    "      <menuitem action='StoreData'/>"
    "      <menuitem action='RecallData'/>"
    "      <menuitem action='SubtractDataFromMem'/>"
    "      <menuitem action='ClearAltFlags'/>"
	  "    </menu>"
	  "    <menu action='MenuActions'>"
    "      <menuitem action='EditStrategy'/>"
    "      <menuitem action='ExecuteStrategy'/>"
    "      <separator/>"
    "      <menuitem action='Segment'/>"
    "      <menuitem action='Cluster'/>"
    "      <menuitem action='Classify'/>"
    "      <menuitem action='RemoveSmallSegments'/>"
    "      <separator/>"
    "      <menuitem action='VertEVD'/>"
    "      <menuitem action='ApplyTimeProfile'/>"
    "      <menuitem action='ApplyVertProfile'/>"
    "      <menuitem action='RestoreTimeProfile'/>"
    "      <menuitem action='RestoreVertProfile'/>"
    "      <menuitem action='ReapplyTimeProfile'/>"
    "      <menuitem action='ReapplyVertProfile'/>"
	  "    </menu>"
	  "    <menu action='MenuHelp'>"
    "      <menuitem action='About'/>"
	  "    </menu>"
    "  </menubar>"
    "  <toolbar  name='ToolBar'>"
    "    <toolitem action='OpenDirectory'/>"
    "    <separator/>"
    "    <toolitem action='ExecuteStrategy'/>"
    "    <toolitem action='OriginalFlags'/>"
    "    <toolitem action='AlternativeFlags'/>"
    "    <separator/>"
    "    <toolitem action='ZoomFit'/>"
    "    <toolitem action='ZoomIn'/>"
    "    <toolitem action='ZoomOut'/>"
    "    <separator/>"
    "    <toolitem action='Previous'/>"
    "    <toolitem action='Reload'/>"
    "    <toolitem action='Next'/>"
    "    <separator/>"
    "    <toolitem action='ImageOriginal'/>"
    "    <toolitem action='ImageBackground'/>"
    "    <toolitem action='ImageDiff'/>"
    "  </toolbar>"
    "</ui>";

	uiManager->add_ui_from_string(ui_info);
	Gtk::Widget* pMenubar = uiManager->get_widget("/MenuBar");
	_mainVBox.pack_start(*pMenubar, Gtk::PACK_SHRINK);
	Gtk::Toolbar* pToolbar = static_cast<Gtk::Toolbar *>(uiManager->get_widget("/ToolBar"));
	if(Gtk::IconTheme::get_default()->has_icon("aoflagger"))
	{
		pToolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
		pToolbar->set_icon_size(Gtk::ICON_SIZE_LARGE_TOOLBAR);
	}
	else {
		pToolbar->set_toolbar_style(Gtk::TOOLBAR_TEXT);
		pToolbar->set_icon_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
	}
	_mainVBox.pack_start(*pToolbar, Gtk::PACK_SHRINK);
	pMenubar->show();
}

void RFIGuiWindow::onClearAltFlagsPressed()
{
	TimeFrequencyData data = GetActiveData();
	_timeFrequencyWidget.SetAlternativeMask(
		Mask2D::CreateSetMaskPtr<false>(data.ImageWidth(), data.ImageHeight())
	);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onDifferenceToOriginalPressed()
{
	if(HasImage())
	{
		TimeFrequencyData data(_timeFrequencyWidget.ContaminatedData());
		_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetFullMetaData());
	}
	if(_originalImageButton->get_active())
		_timeFrequencyWidget.Update();
	else
		_originalImageButton->set_active();
}

void RFIGuiWindow::onBackgroundToOriginalPressed()
{
	if(HasImage())
	{
		TimeFrequencyData data(_timeFrequencyWidget.RevisedData());
		_timeFrequencyWidget.ClearBackground();
		_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetFullMetaData());
	}
	if(_originalImageButton->get_active())
		_timeFrequencyWidget.Update();
	else
		_originalImageButton->set_active();
}

void RFIGuiWindow::onHightlightPressed()
{
	if(_highlightWindow != 0)
		delete _highlightWindow;
	_highlightWindow = new HighlightWindow(*this);
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
			_timeFrequencyWidget.SetNewData(data, metaData);
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
			ThresholdTools::MeanAndStdDev(data.GetRealPart(), data.GetSingleMask(), mean, stddev);
			FringeTestCreater::AddStaticFringe(data, metaData, stddev);
			_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
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
		std::pair<Image2DCPtr, Image2DCPtr> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::CreateCopy(images.first),
			imaginary = Image2D::CreateCopy(images.first);
		real->SetAll(1.0);
		imaginary->SetAll(0.0);
		TimeFrequencyData newData(data.Polarisation(), real, imaginary);
		newData.SetMask(data);
		_timeFrequencyWidget.SetNewData(newData, _timeFrequencyWidget.GetSelectedMetaData());
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
		std::pair<Image2DCPtr, Image2DCPtr> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::CreateCopy(images.first),
			imaginary = Image2D::CreateCopy(images.first);
		real->SetAll(0.0);
		imaginary->SetAll(1.0);
		TimeFrequencyData newData(data.Polarisation(), real, imaginary);
		newData.SetMask(data);
		_timeFrequencyWidget.SetNewData(newData, _timeFrequencyWidget.GetSelectedMetaData());
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
		std::pair<Image2DCPtr, Image2DCPtr> images = data.GetSingleComplexImage();
		Image2DPtr
			real = Image2D::CreateCopy(images.first),
			imaginary = Image2D::CreateCopy(images.first);
		real->SetAll(1.0);
		imaginary->SetAll(1.0);
		TimeFrequencyData newData(data.Polarisation(), real, imaginary);
		newData.SetMask(data);
		_timeFrequencyWidget.SetNewData(newData, _timeFrequencyWidget.GetSelectedMetaData());
		_timeFrequencyWidget.Update();
	} catch(std::exception &e)
	{
		showError(e.what());
	}
}

void RFIGuiWindow::onShowStats()
{
	if(_timeFrequencyWidget.HasImage())
	{
		TimeFrequencyData activeData = GetActiveData();
		TimeFrequencyStatistics statistics(activeData);
		std::stringstream s;
		s << "Percentage flagged: " << TimeFrequencyStatistics::FormatRatio(statistics.GetFlaggedRatio()) << "\n";
			
		Mask2DCPtr
			original = _timeFrequencyWidget.OriginalMask(),
			alternative = _timeFrequencyWidget.AlternativeMask();
		Mask2DPtr
			intersect;
		if(original != 0 && alternative != 0)
		{
			intersect = Mask2D::CreateCopy(original);
			intersect->Intersect(alternative);
			
			unsigned intCount = intersect->GetCount<true>();
			if(intCount != 0)
			{
				if(!original->Equals(alternative))
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
		if(_plotComplexPlaneWindow != 0)
			delete _plotComplexPlaneWindow;
		_plotComplexPlaneWindow = new ComplexPlanePlotWindow(*this, _controller->PlotManager());
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

void RFIGuiWindow::onPlotPowerRMSPressed()
{
	_controller->PlotPowerRMS();
}

void RFIGuiWindow::onPlotPowerSNRPressed()
{
	_controller->PlotPowerSNR();
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
	if(_histogramWindow == 0)
		_histogramWindow = new HistogramWindow(histogramCollection);
	else
		_histogramWindow->SetStatistics(histogramCollection);
	_histogramWindow->show();
}

void RFIGuiWindow::onImagePropertiesPressed()
{
	if(_imagePropertiesWindow != 0)
		delete _imagePropertiesWindow;
	_imagePropertiesWindow = new ImagePropertiesWindow(_timeFrequencyWidget, "Time-frequency plotting options");
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

void RFIGuiWindow::showPhasePart(enum TimeFrequencyData::PhaseRepresentation phaseRepresentation)
{
	if(HasImage())
	{
		try {
			TimeFrequencyData *newPart =  _timeFrequencyWidget.GetActiveData().CreateTFData(phaseRepresentation);
			_timeFrequencyWidget.SetNewData(*newPart, _timeFrequencyWidget.GetSelectedMetaData());
			delete newPart;
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

void RFIGuiWindow::showPolarisation(enum PolarisationType polarisation)
{
	if(HasImage())
	{
		try {
			TimeFrequencyData *newData =
				_timeFrequencyWidget.GetActiveData().CreateTFData(polarisation);
			_timeFrequencyWidget.SetNewData(*newData, _timeFrequencyWidget.GetSelectedMetaData());
			delete newData;
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
	if(HasImageSet())
	{
		rfiStrategy::MSImageSet *msSet = dynamic_cast<rfiStrategy::MSImageSet*>(_imageSet);
		if(msSet != 0)
		{
			if(_gotoWindow != 0)
				delete _gotoWindow;
			_gotoWindow = new GoToWindow(*this);
			_gotoWindow->show();
		} else {
			showError("Can not goto in this image set; format does not support goto");
		}
	}
}

void RFIGuiWindow::onReloadPressed()
{
	if(HasImageSet())
	{
		loadCurrentTFData();
	}
}

void RFIGuiWindow::onLoadLongestBaselinePressed()
{
	if(HasImageSet())
	{
		rfiStrategy::MSImageSet *msSet = dynamic_cast<rfiStrategy::MSImageSet*>(_imageSet);
		if(msSet != 0)
		{
			double longestSq = 0.0;
			size_t longestA1=0, longestA2=0;
			size_t antCount = msSet->AntennaCount();
			std::vector<AntennaInfo> antennas(antCount);
			for(size_t a=0; a!=antCount; a++)
				antennas[a] = msSet->GetAntennaInfo(a);
			
			rfiStrategy::ImageSetIndex *index = msSet->StartIndex();
			size_t band = msSet->GetBand(*_imageSetIndex);
			size_t sequenceId = msSet->GetSequenceId(*_imageSetIndex);
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
			delete index;
			rfiStrategy::MSImageSetIndex *newIndex = msSet->Index(longestA1, longestA2, band, sequenceId);
			SetImageSetIndex(newIndex);
		}
	}
}

void RFIGuiWindow::onLoadShortestBaselinePressed()
{
	if(HasImageSet())
	{
		rfiStrategy::MSImageSet *msSet = dynamic_cast<rfiStrategy::MSImageSet*>(_imageSet);
		if(msSet != 0)
		{
			double smallestSq = 1e26;
			size_t smallestA1=0, smallestA2=0;
			size_t antCount = msSet->AntennaCount();
			std::vector<AntennaInfo> antennas(antCount);
			for(size_t a=0; a!=antCount; a++)
				antennas[a] = msSet->GetAntennaInfo(a);
			
			rfiStrategy::ImageSetIndex *index = msSet->StartIndex();
			size_t band = msSet->GetBand(*_imageSetIndex);
			size_t sequenceId = msSet->GetSequenceId(*_imageSetIndex);
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
			delete index;
			rfiStrategy::MSImageSetIndex *newIndex = msSet->Index(smallestA1, smallestA2, band, sequenceId);
			SetImageSetIndex(newIndex);
		}
	}
}

void RFIGuiWindow::onTFWidgetMouseMoved(size_t x, size_t y)
{
	Image2DCPtr image = _timeFrequencyWidget.Image();
	num_t v = image->Value(x, y);
	_statusbar.pop();
	std::stringstream s;
		s << "x=" << x << ",y=" << y << ",value=" << v;
	TimeFrequencyMetaDataCPtr metaData =_timeFrequencyWidget.GetFullMetaData();
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
		if(_timeFrequencyWidget.GetFullMetaData() != 0 && _timeFrequencyWidget.GetFullMetaData()->HasUVW())
		{
			TimeFrequencyData activeData = GetActiveData();
			if(activeData.PolarisationCount() != 1)
			{
				TimeFrequencyData *singlePolarization = activeData.CreateTFData(StokesIPolarisation);
				activeData = *singlePolarization;
				delete singlePolarization;
			}
			_imagePlaneWindow->AddData(activeData, _timeFrequencyWidget.GetSelectedMetaData());
		}
		else if(_spatialMetaData != 0)
			_imagePlaneWindow->AddData(GetActiveData(), _spatialMetaData);
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
	_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSegment()
{
	_segmentedImage = SegmentedImage::CreateUnsetPtr(GetOriginalData().ImageWidth(),  GetOriginalData().ImageHeight());
	Morphology morphology;
	morphology.SegmentByLengthRatio(GetActiveData().GetSingleMask(), _segmentedImage);
	_timeFrequencyWidget.SetSegmentedImage(_segmentedImage);
	Update();
}

void RFIGuiWindow::onCluster()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.Cluster(_segmentedImage);
	_timeFrequencyWidget.SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onClassify()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.Classify(_segmentedImage);
		_timeFrequencyWidget.SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onRemoveSmallSegments()
{
	if(_segmentedImage != 0)
	{
		Morphology morphology;
		morphology.RemoveSmallSegments(_segmentedImage, 4);
	_timeFrequencyWidget.SetSegmentedImage(_segmentedImage);
		Update();
	}
}

void RFIGuiWindow::onTimeGraphButtonPressed()
{
	if(_timeGraphButton->get_active())
	{
		_mainVBox.remove(_timeFrequencyWidget);
		_mainVBox.pack_start(_panedArea);
		_panedArea.pack1(_timeFrequencyWidget, true, true);
		_panedArea.pack2(_plotFrame, true, true);

		_panedArea.show();
		_timeFrequencyWidget.show();
		_plotFrame.show();
	} else {
		_mainVBox.remove(_panedArea);
		_panedArea.remove(_timeFrequencyWidget);
		_panedArea.remove(_plotFrame);

		_mainVBox.pack_start(_timeFrequencyWidget);
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

void RFIGuiWindow::onUnrollPhaseButtonPressed()
{
	if(HasImage())
	{
		TimeFrequencyData *data =
			GetActiveData().CreateTFData(TimeFrequencyData::PhasePart);
		for(unsigned i=0;i<data->ImageCount();++i)
		{
			Image2DPtr image = Image2D::CreateCopy(data->GetImage(i));
			ThresholdTools::UnrollPhase(image);
			data->SetImage(i, image);
		}
		_timeFrequencyWidget.SetNewData(*data, _timeFrequencyWidget.GetSelectedMetaData());
		_timeFrequencyWidget.Update();
		delete data;
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
	if(_ncpSetButton->get_active())
		return DefaultModels::NCPSet;
	else if(_b1834SetButton->get_active())
		return DefaultModels::B1834Set;
	else
		return DefaultModels::EmptySet;
}

void RFIGuiWindow::loadDefaultModel(DefaultModels::Distortion distortion, bool withNoise, bool empty)
{
	unsigned channelCount;
	if(_sim16ChannelsButton->get_active())
		channelCount = 16;
	else if(_sim64ChannelsButton->get_active())
		channelCount = 64;
	else
		channelCount = 256;
	double bandwidth;
	if(_simFixBandwidthButton->get_active())
		bandwidth = 16.0 * 2500000.0;
	else
		bandwidth = (double) channelCount / 16.0 * 2500000.0;
	std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> pair = DefaultModels::LoadSet(getSetLocation(empty), distortion, withNoise ? 1.0 : 0.0, channelCount, bandwidth);
	TimeFrequencyData data = pair.first;
	TimeFrequencyMetaDataCPtr metaData = pair.second;
	
	_timeFrequencyWidget.SetNewData(data, metaData);
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
			TimeFrequencyData *diff = TimeFrequencyData::CreateTFDataFromDiff(old, data);
			_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
			_timeFrequencyWidget.SetRevisedData(*diff);
			delete diff;
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
		_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
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
		_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
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
			_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
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
			_timeFrequencyWidget.SetNewData(data, _timeFrequencyWidget.GetSelectedMetaData());
			_timeFrequencyWidget.Update();
		}
	}
}

void RFIGuiWindow::onStoreData()
{
	if(HasImage())
	{
		_storedData = _timeFrequencyWidget.GetActiveData();
		_storedMetaData = _timeFrequencyWidget.GetSelectedMetaData();
	}
}

void RFIGuiWindow::onRecallData()
{
	_timeFrequencyWidget.SetNewData(_storedData, _storedMetaData);
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSubtractDataFromMem()
{
	if(HasImage())
	{
		TimeFrequencyData activeData = _timeFrequencyWidget.GetActiveData();
		TimeFrequencyData *diffData = TimeFrequencyData::CreateTFDataFromDiff(_storedData, activeData);
		_timeFrequencyWidget.SetNewData(*diffData, _storedMetaData);
		delete diffData;
		_timeFrequencyWidget.Update();
	}
}

void RFIGuiWindow::SetStrategy(rfiStrategy::Strategy* newStrategy)
{
	delete _strategy;
	_strategy = newStrategy;
}

void RFIGuiWindow::onControllerStateChange()
{
	_originalFlagsButton->set_active(_controller->AreOriginalFlagsShown());
	_timeFrequencyWidget.SetShowOriginalMask(_controller->AreOriginalFlagsShown());
	
	_altFlagsButton->set_active(_controller->AreAlternativeFlagsShown());
	_timeFrequencyWidget.SetShowAlternativeMask(_controller->AreAlternativeFlagsShown());
	
	_timeFrequencyWidget.Update();
}

void RFIGuiWindow::onTFZoomChanged()
{
	bool s = !_timeFrequencyWidget.IsZoomedOut();
	bool i = _timeFrequencyWidget.HasImage();
	_zoomToFitButton->set_sensitive(s && i);
	_zoomOutButton->set_sensitive(s && i);
	_zoomInButton->set_sensitive(i);
}

void RFIGuiWindow::onHelpAbout()
{
	Gtk::AboutDialog aboutDialog;
	
	std::vector<Glib::ustring> authors;
	authors.push_back("André Offringa <offringa@gmail.com>");
	aboutDialog.set_authors(authors);
	
	aboutDialog.set_copyright("Copyright 2008 - 2015 A. R. Offringa");
	aboutDialog.set_license_type(Gtk::LICENSE_GPL_3_0);
	aboutDialog.set_logo_icon_name("aoflagger");
	aboutDialog.set_program_name("AOFlagger's RFI Gui");
	aboutDialog.set_version("AOFlagger " AOFLAGGER_VERSION_STR " (" AOFLAGGER_VERSION_DATE_STR ") ");
	aboutDialog.set_website("http://aoflagger.sourceforge.net/");
	
	aboutDialog.run();
}
