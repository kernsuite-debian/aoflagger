#ifndef THRESHOLDFRAME_H
#define THRESHOLDFRAME_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>

#include "../../strategy/actions/sumthresholdaction.h"

#include "../editstrategywindow.h"

class SumThresholdFrame : public Gtk::Frame {
	public:
		SumThresholdFrame(rfiStrategy::SumThresholdAction &action, EditStrategyWindow &editStrategyWindow)
		: Gtk::Frame("SumThreshold"),
		_editStrategyWindow(editStrategyWindow), _action(action),
		_timeDirectionButton("In time direction (sensitive to spectral lines)"),
		_timeSensitivityLabel("Time-direction base sensitivity: (low = sensitive)"),
		_timeSensitivityScale(Gtk::ORIENTATION_HORIZONTAL),
		_frequencyDirectionButton("In frequency direction (sensitive to temporal RFI)"),
		_frequencySensitivityLabel("Frequency-direction base sensitivity: (low = sensitive)"),
		_frequencySensitivityScale(Gtk::ORIENTATION_HORIZONTAL),
		_applyButton("Apply")
		{
			_timeDirectionButton.set_active(_action.TimeDirectionFlagging());
			_box.pack_start(_timeDirectionButton);

			_box.pack_start(_timeSensitivityLabel);
			_box.pack_start(_timeSensitivityScale);
			_timeSensitivityScale.set_range(0, 10);
			_timeSensitivityScale.set_increments(0.1, 1.0);
			_timeSensitivityScale.set_value(_action.TimeDirectionSensitivity());

			_frequencyDirectionButton.set_active(_action.FrequencyDirectionFlagging());
			_box.pack_start(_frequencyDirectionButton);

			_box.pack_start(_frequencySensitivityLabel);
			_box.pack_start(_frequencySensitivityScale);
			_frequencySensitivityScale.set_range(0, 10);
			_frequencySensitivityScale.set_increments(0.1, 1.0);
			_frequencySensitivityScale.set_value(_action.FrequencyDirectionSensitivity());

			_buttonBox.pack_start(_applyButton);
			_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &SumThresholdFrame::onApplyClicked));

			_box.pack_end(_buttonBox);

			add(_box);
			_box.show_all();
		}
	private:
		EditStrategyWindow &_editStrategyWindow;
		rfiStrategy::SumThresholdAction &_action;

		Gtk::VBox _box;
		Gtk::ButtonBox _buttonBox;
		Gtk::CheckButton _timeDirectionButton;
		Gtk::Label _timeSensitivityLabel;
		Gtk::Scale _timeSensitivityScale;
		Gtk::CheckButton _frequencyDirectionButton;
		Gtk::Label _frequencySensitivityLabel;
		Gtk::Scale _frequencySensitivityScale;
		Gtk::Button _applyButton;

		void onApplyClicked()
		{
			_action.SetTimeDirectionFlagging(_timeDirectionButton.get_active());
			_action.SetTimeDirectionSensitivity(_timeSensitivityScale.get_value());
			_action.SetFrequencyDirectionFlagging(_frequencyDirectionButton.get_active());
			_action.SetFrequencyDirectionSensitivity(_frequencySensitivityScale.get_value());
			_editStrategyWindow.UpdateAction(&_action);
		}
};

#endif // THRESHOLDFRAME_H
