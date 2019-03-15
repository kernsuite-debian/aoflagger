#ifndef CALIBRATE_BANDPASS_FRAME_H
#define CALIBRATE_BANDPASS_FRAME_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>

#include "../../strategy/actions/calibratebandpassaction.h"

#include "../editstrategywindow.h"

class CalibrateBandpassFrame : public Gtk::Frame {
	public:
		CalibrateBandpassFrame(rfiStrategy::CalibrateBandpassAction& action, EditStrategyWindow& editStrategyWindow)
		: Gtk::Frame("Calibrate (flatten) bandpass"),
		_editStrategyWindow(editStrategyWindow), _action(action),
		_methodLabelButton("Method:"),
		_stepwiseMethodButton("Stepwise\n(fixes regular jumps, like in MWA)"),
		_smoothMethodButton("Smooth\n(Normalizes a smooth bandpass)"),
		_stepsLabel("Steps:"),
		_applyButton("Apply")
		{
			Gtk::RadioButton::Group group;
			_stepwiseMethodButton.set_group(group);
			_smoothMethodButton.set_group(group);
			
			if(_action.GetMethod() == rfiStrategy::CalibrateBandpassAction::StepwiseMethod)
				_stepwiseMethodButton.set_active();
			else
				_smoothMethodButton.set_active();
			
			_box.pack_start(_methodLabelButton);
			_box.pack_start(_stepwiseMethodButton);
			_box.pack_start(_smoothMethodButton);
			
			_box.pack_start(_stepsLabel);
			_stepsEntry.set_text(std::to_string(_action.Steps()));
			_box.pack_start(_stepsEntry);
			
			_buttonBox.pack_start(_applyButton);
			_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &CalibrateBandpassFrame::onApplyClicked));

			_box.pack_start(_buttonBox);

			add(_box);
			_box.show_all();
		}
	private:
		EditStrategyWindow &_editStrategyWindow;
		rfiStrategy::CalibrateBandpassAction &_action;

		Gtk::VBox _box;
		Gtk::ButtonBox _buttonBox;
		Gtk::Label _methodLabelButton;
		Gtk::RadioButton _stepwiseMethodButton, _smoothMethodButton;
		Gtk::Label _stepsLabel;
		Gtk::Entry _stepsEntry;
		Gtk::Button _applyButton;

		void onApplyClicked()
		{
			if(_stepwiseMethodButton.get_active())
				_action.SetMethod(rfiStrategy::CalibrateBandpassAction::StepwiseMethod);
			else
				_action.SetMethod(rfiStrategy::CalibrateBandpassAction::SmoothMethod);
			_action.SetSteps(atoi(_stepsEntry.get_text().c_str()));
			_editStrategyWindow.UpdateAction(&_action);
		}
};

#endif // CALIBRATE_BANDPASS_FRAME_H


