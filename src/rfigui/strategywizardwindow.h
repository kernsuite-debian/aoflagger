#ifndef STRATEGY_WIZARD_WINDOW_H
#define STRATEGY_WIZARD_WINDOW_H

#include "../strategy/control/defaultstrategy.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/grid.h>
#include <gtkmm/liststore.h>
#include <gtkmm/notebook.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/window.h>

class StrategyWizardWindow : public Gtk::Window
{
public:
	explicit StrategyWizardWindow(class RFIGuiController& guiController, class StrategyController& controller);
private:
	class StrategyController &_strategyController;
	
	int _page;
	
	Gtk::VBox _mainBox, _telescopeBox, _optionsLeftBox, _optionsRightBox, _strategySetupBox;
	Gtk::HBox _telescopeSubBox, _optionsBox;
	Gtk::Label _telescopeLabel;
	Gtk::ComboBox _telescopeCombo;
	Glib::RefPtr<Gtk::ListStore> _telescopeList;
	Gtk::ButtonBox _buttonBox;
	Gtk::Button _finishButton, _nextButton, _previousButton;
	Gtk::CheckButton _transientsButton, _highTimeResolutionButton;
	Gtk::RadioButton _lowFreqRadioButton, _normFreqRadioButton, _highFreqRadioButton;
	Gtk::RadioButton _smallBandwidthButton, _normBandwidthButton, _largeBandwidthButton;
	Gtk::RadioButton _robustConvergenceButton, _normConvergenceButton, _fastConvergenceButton;
	Gtk::RadioButton _insensitiveButton, _normalSensitivityButton, _sensitiveButton;
	Gtk::CheckButton _useOriginalFlagsButton, _autoCorrelationButton;
	
	// Strategy setup page
	Gtk::Grid _setupGrid;
	Gtk::Label _iterationCountLabel, _sumThresholdLevelLabel, _verticalSmoothingLabel;
	Gtk::Entry _iterationCountEntry, _sumThresholdLevelEntry, _verticalSmoothingEntry;
	Gtk::CheckButton
		_changeResVerticallyCB, _calPassbandCB, _channelSelectionCB,
		_onStokesIQCB, _includeStatisticsCB, _hasBaselinesCB;
	
	void initializeTelescopePage(class RFIGuiController& guiController);
	void initializeOptionPage();
	void initializeSetupPage();
	void onNextClicked();
	void onPreviousClicked();
	void onFinishClicked();
	void updateSensitivities();
	void updatePage();
	void addTelescope(const Glib::ustring &name, int val);
	rfiStrategy::DefaultStrategy::StrategySetup getSetupFromOptions() const;
	void updateSetupPageSelection(const rfiStrategy::DefaultStrategy::StrategySetup& setup);
	rfiStrategy::DefaultStrategy::StrategySetup getSetupPageSelection() const;

	struct ModelColumns : public Gtk::TreeModelColumnRecord
	{
		ModelColumns() { add(name); add(val); }
		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> val;
	} _telescopeListColumns;
};

#endif
