#include "strategywizardwindow.h"
#include "interfaces.h"

#include "controllers/rfiguicontroller.h"

#include "../strategy/actions/strategy.h"

#include "../gtkmm-compat.h"

StrategyWizardWindow::StrategyWizardWindow(class RFIGuiController& guiController, class StrategyController& controller) : Window(),
	_strategyController(controller),
	_page(0),
	_telescopeLabel("Telescope:"),
	_telescopeCombo(),
	_finishButton("_Finish", true),
	_nextButton("_Next", true),
	_previousButton("_Previous", true),
	_transientsButton("Transients"), _highTimeResolutionButton("High time resolution"),
	_smallBandwidthButton("Small bandwidth"), _normBandwidthButton("Normal bandwidth"), _largeBandwidthButton("Large bandwidth"),
	_robustConvergenceButton("More iterations (more robust)"),
	_normConvergenceButton("Default number of iterations"),
	_fastConvergenceButton("Fewer iterations (faster)"),
	_insensitiveButton("Insensitive"),
	_normalSensitivityButton("Normal sensitivity"), _sensitiveButton("Sensitive"),
	_useOriginalFlagsButton("Use existing flags"),
	_autoCorrelationButton("Auto-correlation"),
	
	_setupGrid(),
	_iterationCountLabel("Iterations"),
	_sumThresholdLevelLabel("SumThreshold level"),
	_verticalSmoothingLabel("Vertical smoothing"),
	_changeResVerticallyCB("Change frequency resolution"),
	_calPassbandCB("Normalize passband"),
	_channelSelectionCB("Channel selection"),
	_onStokesIQCB("On Stokes I and Q only"),
	_includeStatisticsCB("Collect statistics"),
	_hasBaselinesCB("Observation has multiple baselines")
{
	set_default_size(500, 250);
	
	initializeTelescopePage(guiController);
	
	initializeOptionPage();
	
	initializeSetupPage();
	
	gtkmm_set_image_from_icon_name(_previousButton, "go-previous");
	_previousButton.signal_clicked().connect(
		sigc::mem_fun(*this, &StrategyWizardWindow::onPreviousClicked));
	_buttonBox.pack_start(_previousButton, true, false);
	gtkmm_set_image_from_icon_name(_nextButton, "go-next");
	_nextButton.signal_clicked().connect(
		sigc::mem_fun(*this, &StrategyWizardWindow::onNextClicked));
	_buttonBox.pack_start(_nextButton, true, false);
	gtkmm_set_image_from_icon_name(_finishButton, "gtk-ok");
	_finishButton.signal_clicked().connect(
		sigc::mem_fun(*this, &StrategyWizardWindow::onFinishClicked));
	_buttonBox.pack_end(_finishButton, true, false);
	_finishButton.set_sensitive(false);
	_mainBox.pack_end(_buttonBox, false, false);
	
	add(_mainBox);
	_buttonBox.show_all();
	_mainBox.show();
	
	updateSensitivities();
}

void StrategyWizardWindow::initializeTelescopePage(class RFIGuiController& guiController)
{
	_telescopeSubBox.pack_start(_telescopeLabel);
	_telescopeList = Gtk::ListStore::create(_telescopeListColumns);
	addTelescope("Generic", rfiStrategy::DefaultStrategy::GENERIC_TELESCOPE);
	addTelescope("AARTFAAC", rfiStrategy::DefaultStrategy::AARTFAAC_TELESCOPE);
	addTelescope("Arecibo (305 m single dish, Puerto Rico)", rfiStrategy::DefaultStrategy::ARECIBO_TELESCOPE);
	addTelescope("Bighorns (low-frequency wide-band EoR instrument, Curtin uni, Australia)", rfiStrategy::DefaultStrategy::BIGHORNS_TELESCOPE);
	addTelescope("JVLA (Jansky Very Large Array, New Mexico)", rfiStrategy::DefaultStrategy::JVLA_TELESCOPE);
	addTelescope("LOFAR (Low-Frequency Array, Europe)", rfiStrategy::DefaultStrategy::LOFAR_TELESCOPE);
	addTelescope("MWA (Murchison Widefield Array, Australia)", rfiStrategy::DefaultStrategy::MWA_TELESCOPE);
	addTelescope("Parkes (single dish, Australia)",    rfiStrategy::DefaultStrategy::PARKES_TELESCOPE);
	addTelescope("WSRT (Westerbork Synthesis Radio Telescope, Netherlands)", rfiStrategy::DefaultStrategy::WSRT_TELESCOPE);
	
	_telescopeCombo.set_model(_telescopeList);
	_telescopeCombo.pack_start(_telescopeListColumns.name, false);
	_telescopeCombo.signal_changed().connect(sigc::mem_fun(*this, &StrategyWizardWindow::updateSensitivities));
	_telescopeSubBox.pack_start(_telescopeCombo, false, false);
	
	if(guiController.HasImageSet())
	{
		rfiStrategy::ImageSet& set = guiController.GetImageSet();
		rfiStrategy::DefaultStrategy::TelescopeId telescope;
		unsigned flags;
		double frequency, timeRes, freqRes;
		rfiStrategy::DefaultStrategy::DetermineSettings(set, telescope, flags, frequency, timeRes, freqRes);
		Gtk::TreeModel::Children rows = _telescopeList->children();
		for(Gtk::TreeModel::iterator row=rows.begin(); row!=rows.end(); ++row)
		{
			if((*row)[_telescopeListColumns.val] == telescope)
			{
				_telescopeCombo.set_active(row);
				break;
			}
		}
	}
	
	_telescopeBox.pack_start(_telescopeSubBox, false, false);
	_mainBox.pack_start(_telescopeBox);
	_telescopeBox.show_all();
}

void StrategyWizardWindow::initializeOptionPage()
{
	_optionsLeftBox.pack_start(_transientsButton, true, true);
	_optionsLeftBox.pack_start(_highTimeResolutionButton, true, true);
	
	Gtk::RadioButton::Group bandwidthGroup;
	_optionsRightBox.pack_start(_smallBandwidthButton, true, true);
	_smallBandwidthButton.set_group(bandwidthGroup);
	_optionsRightBox.pack_start(_normBandwidthButton, true, true);
	_normBandwidthButton.set_group(bandwidthGroup);
	_normBandwidthButton.set_active(true);
	_optionsRightBox.pack_start(_largeBandwidthButton, true, true);
	_largeBandwidthButton.set_group(bandwidthGroup);
	
	Gtk::RadioButton::Group convergenceGroup;
	_optionsLeftBox.pack_start(_robustConvergenceButton, true, true);
	_robustConvergenceButton.set_group(convergenceGroup);
	_optionsLeftBox.pack_start(_normConvergenceButton, true, true);
	_normConvergenceButton.set_group(convergenceGroup);
	_normConvergenceButton.set_active(true);
	_optionsLeftBox.pack_start(_fastConvergenceButton, true, true);
	_fastConvergenceButton.set_group(convergenceGroup);
		
	Gtk::RadioButton::Group sensitivityGroup;
	_optionsRightBox.pack_start(_insensitiveButton, true, true);
	_insensitiveButton.set_group(sensitivityGroup);
	_optionsRightBox.pack_start(_normalSensitivityButton, true, true);
	_normalSensitivityButton.set_group(sensitivityGroup);
	_normalSensitivityButton.set_active(true);
	_optionsRightBox.pack_start(_sensitiveButton, true, true);
	_sensitiveButton.set_group(sensitivityGroup);
	
	_optionsLeftBox.pack_start(_useOriginalFlagsButton, true, true);
	_optionsLeftBox.pack_start(_autoCorrelationButton, true, true);
	_optionsBox.pack_start(_optionsLeftBox);
	_optionsBox.pack_start(_optionsRightBox);
	
	_optionsBox.show_all_children();
	_mainBox.pack_start(_optionsBox);
}

void StrategyWizardWindow::initializeSetupPage()
{
	_setupGrid.attach(_iterationCountLabel, 0, 0, 1, 1);
	_iterationCountLabel.set_hexpand(true);
	_setupGrid.attach(_iterationCountEntry, 1, 0, 1, 1);
	_iterationCountEntry.set_hexpand(true);
	
	_setupGrid.attach(_sumThresholdLevelLabel, 0, 1, 1, 1);
	_sumThresholdLevelLabel.set_hexpand(true);
	_setupGrid.attach(_sumThresholdLevelEntry, 1, 1, 1, 1);
	_sumThresholdLevelEntry.set_hexpand(true);
	
	_setupGrid.attach(_verticalSmoothingLabel, 0, 2, 1, 1);
	_verticalSmoothingLabel.set_hexpand(true);
	_setupGrid.attach(_verticalSmoothingEntry, 1, 2, 1, 1);
	_verticalSmoothingEntry.set_hexpand(true);
	
	_setupGrid.attach(_changeResVerticallyCB, 0, 3, 2, 1);
	_changeResVerticallyCB.set_hexpand(true);
	
	_setupGrid.attach(_calPassbandCB, 0, 4, 2, 1);
	_calPassbandCB.set_hexpand(true);

	_setupGrid.attach(_channelSelectionCB, 0, 5, 2, 1);
	_channelSelectionCB.set_hexpand(true);

	_setupGrid.attach(_onStokesIQCB, 0, 6, 2, 1);
	_onStokesIQCB.set_hexpand(true);

	_setupGrid.attach(_includeStatisticsCB, 0, 7, 2, 1);
	_includeStatisticsCB.set_hexpand(true);

	_setupGrid.attach(_hasBaselinesCB, 0, 8, 2, 1);
	_hasBaselinesCB.set_hexpand(true);
	
	_strategySetupBox.pack_start(_setupGrid, true, true);
	
	_strategySetupBox.show_all_children();
	_mainBox.pack_start(_strategySetupBox);
}

void StrategyWizardWindow::updateSetupPageSelection(const rfiStrategy::DefaultStrategy::StrategySetup& setup)
{
	_iterationCountEntry.set_text(std::to_string(setup.iterationCount));
	_sumThresholdLevelEntry.set_text(std::to_string(setup.sumThresholdSensitivity));
	_verticalSmoothingEntry.set_text(std::to_string(setup.verticalSmoothing));
	
	_changeResVerticallyCB.set_active(setup.changeResVertically);
	_calPassbandCB.set_active(setup.calPassband);
	_channelSelectionCB.set_active(setup.channelSelection);

	_onStokesIQCB.set_active(setup.onStokesIQ);
	_includeStatisticsCB.set_active(setup.includeStatistics);
	_hasBaselinesCB.set_active(setup.hasBaselines);
}

rfiStrategy::DefaultStrategy::StrategySetup StrategyWizardWindow::getSetupPageSelection() const
{
	rfiStrategy::DefaultStrategy::StrategySetup setup = getSetupFromOptions();
	
	setup.iterationCount = std::stoi(_iterationCountEntry.get_text());
	setup.sumThresholdSensitivity = std::stod(_sumThresholdLevelEntry.get_text());
	setup.verticalSmoothing = std::stod(_verticalSmoothingEntry.get_text());
	
	setup.changeResVertically = _changeResVerticallyCB.get_active();
	setup.calPassband = _calPassbandCB.get_active();
	setup.channelSelection = _channelSelectionCB.get_active();
	setup.onStokesIQ = _onStokesIQCB.get_active();
	setup.includeStatistics = _includeStatisticsCB.get_active();
	setup.hasBaselines = _hasBaselinesCB.get_active();
	
	return setup;
}

void StrategyWizardWindow::addTelescope(const Glib::ustring& name, int val)
{
	Gtk::TreeModel::iterator row = _telescopeList->append();
	(*row)[_telescopeListColumns.name] = name;
	(*row)[_telescopeListColumns.val] = val;
}

void StrategyWizardWindow::onPreviousClicked()
{
	if(_page > 0)
	{
		--_page;
		updatePage();
	}
}

void StrategyWizardWindow::onNextClicked()
{
	if(_page < 2)
	{
		++_page;
		updatePage();
	}
}

rfiStrategy::DefaultStrategy::StrategySetup StrategyWizardWindow::getSetupFromOptions() const
{
	const enum rfiStrategy::DefaultStrategy::TelescopeId telescopeId =
		(enum rfiStrategy::DefaultStrategy::TelescopeId) (int) ((*_telescopeCombo.get_active())[_telescopeListColumns.val]);
		
	int flags = rfiStrategy::DefaultStrategy::FLAG_NONE;
	if(_largeBandwidthButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_LARGE_BANDWIDTH;
	if(_smallBandwidthButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_SMALL_BANDWIDTH;
	if(_transientsButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_TRANSIENTS;
	if(_highTimeResolutionButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_HIGH_TIME_RESOLUTION;
	if(_robustConvergenceButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_ROBUST;
	if(_fastConvergenceButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_FAST;
	if(_insensitiveButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_INSENSITIVE;
	if(_sensitiveButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_SENSITIVE;
	if(_useOriginalFlagsButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_USE_ORIGINAL_FLAGS;
	if(_autoCorrelationButton.get_active())
		flags |= rfiStrategy::DefaultStrategy::FLAG_AUTO_CORRELATION;
	return rfiStrategy::DefaultStrategy::DetermineSetup(telescopeId, flags, 0.0, 0.0, 0.0);
}

void StrategyWizardWindow::onFinishClicked()
{
	std::unique_ptr<rfiStrategy::Strategy> strategy(new rfiStrategy::Strategy());
	rfiStrategy::DefaultStrategy::LoadSingleStrategy(*strategy, getSetupPageSelection());
		
	_strategyController.SetStrategy(std::move(strategy));
	_strategyController.NotifyChange();
	
	hide();
	_page = 0;
	updatePage();
}

void StrategyWizardWindow::updateSensitivities()
{
	bool hasTelescope = (_telescopeCombo.get_active_row_number() != -1);
	_previousButton.set_sensitive(_page!=0);
	_nextButton.set_sensitive(_page!=2 && hasTelescope);
	_finishButton.set_sensitive(_page==2);
}

void StrategyWizardWindow::updatePage()
{
	_telescopeBox.hide();
	_optionsBox.hide();
	_strategySetupBox.hide();
	switch(_page) {
	case 0:
		_telescopeBox.show();
		break;
	case 1:
		_optionsBox.show();
		break;
	case 2:
		updateSetupPageSelection(getSetupFromOptions());
		_strategySetupBox.show();
		break;
	}
	updateSensitivities();
}
