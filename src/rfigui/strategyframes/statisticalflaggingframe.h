#ifndef STATISTICALFLAGGINGFRAME_H
#define STATISTICALFLAGGINGFRAME_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>

#include "../../strategy/actions/morphologicalflagaction.h"

#include "../editstrategywindow.h"

class MorphologicalFlaggingFrame : public Gtk::Frame {
	public:
		MorphologicalFlaggingFrame(rfiStrategy::MorphologicalFlagAction &action, EditStrategyWindow &editStrategyWindow)
		: Gtk::Frame("Statistical flagging"),
		_editStrategyWindow(editStrategyWindow), _action(action),
		_dilluteTimeSizeLabel("Dillution time size:"),
		_dilluteTimeSizeScale(Gtk::ORIENTATION_HORIZONTAL),
		_dilluteFrequencySizeLabel("Dillution frequency size:"),
		_dilluteFrequencySizeScale(Gtk::ORIENTATION_HORIZONTAL),
		_minTimesAvailableRatioLabel("Minimum time ratio:"),
		_minTimesAvailableRatioScale(Gtk::ORIENTATION_HORIZONTAL),
		_minFreqAvailableRatioLabel("Minimum frequency ratio:"),
		_minFreqAvailableRatioScale(Gtk::ORIENTATION_HORIZONTAL),
		_minTFAvailableRatioLabel("Minimum total available ratio:"),
		_minTFAvailableRatioScale(Gtk::ORIENTATION_HORIZONTAL),
		_minTimeRatioLabel("SIR-operator time extension:"),
		_minTimeRatioScale(Gtk::ORIENTATION_HORIZONTAL),
		_minFreqRatioLabel("SIR-operator frequency extension:"),
		_minFreqRatioScale(Gtk::ORIENTATION_HORIZONTAL),
		_excludeOriginalFlags("Exclude original flags"),
		_applyButton("Apply")
		{
			_box.pack_start(_dilluteTimeSizeLabel);

			_dilluteTimeSizeScale.set_range(0, 100);
			_dilluteTimeSizeScale.set_value(_action.EnlargeTimeSize());
			_dilluteTimeSizeScale.set_increments(1, 10);
			_box.pack_start(_dilluteTimeSizeScale);

			_box.pack_start(_dilluteFrequencySizeLabel);

			_dilluteFrequencySizeScale.set_range(0, 100);
			_dilluteFrequencySizeScale.set_value(_action.EnlargeFrequencySize());
			_dilluteFrequencySizeScale.set_increments(1, 10);
			_box.pack_start(_dilluteFrequencySizeScale);

			_box.pack_start(_minTimesAvailableRatioLabel);

			_minTimesAvailableRatioScale.set_range(0, 100);
			_minTimesAvailableRatioScale.set_value(_action.MinAvailableTimesRatio()*100.0);
			_minTimesAvailableRatioScale.set_increments(.25, 5);
			_box.pack_start(_minTimesAvailableRatioScale);

			_box.pack_start(_minFreqAvailableRatioLabel);

			_minFreqAvailableRatioScale.set_range(0, 100);
			_minFreqAvailableRatioScale.set_value(_action.MinAvailableFrequenciesRatio()*100.0);
			_minFreqAvailableRatioScale.set_increments(.25, 5);
			_box.pack_start(_minFreqAvailableRatioScale);

			_box.pack_start(_minTFAvailableRatioLabel);

			_minTFAvailableRatioScale.set_range(0, 100);
			_minTFAvailableRatioScale.set_value(_action.MinAvailableTFRatio()*100.0);
			_minTFAvailableRatioScale.set_increments(.25, 5);
			_box.pack_start(_minTFAvailableRatioScale);

			_box.pack_start(_minTimeRatioLabel);

			_minTimeRatioScale.set_range(0, 100);
			_minTimeRatioScale.set_value(_action.MinimumGoodTimeRatio()*100.0);
			_minTimeRatioScale.set_increments(.25, 5);
			_box.pack_start(_minTimeRatioScale);

			_box.pack_start(_minFreqRatioLabel);

			_minFreqRatioScale.set_range(0, 100);
			_minFreqRatioScale.set_value(_action.MinimumGoodFrequencyRatio()*100.0);
			_minFreqRatioScale.set_increments(.25, 5);
			_box.pack_start(_minFreqRatioScale);
			
			_excludeOriginalFlags.set_active(_action.ExcludeOriginalFlags());
			_box.pack_start(_excludeOriginalFlags);

			_buttonBox.pack_start(_applyButton);
			_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &MorphologicalFlaggingFrame::onApplyClicked));

			_box.pack_start(_buttonBox);

			add(_box);
			_box.show_all();
		}
	private:
		EditStrategyWindow &_editStrategyWindow;
		rfiStrategy::MorphologicalFlagAction &_action;

		Gtk::VBox _box;
		Gtk::ButtonBox _buttonBox;
		Gtk::Label _dilluteTimeSizeLabel;
		Gtk::Scale _dilluteTimeSizeScale;
		Gtk::Label _dilluteFrequencySizeLabel;
		Gtk::Scale _dilluteFrequencySizeScale;
		
		Gtk::Label _minTimesAvailableRatioLabel;
		Gtk::Scale _minTimesAvailableRatioScale;
		Gtk::Label _minFreqAvailableRatioLabel;
		Gtk::Scale _minFreqAvailableRatioScale;
		Gtk::Label _minTFAvailableRatioLabel;
		Gtk::Scale _minTFAvailableRatioScale;
		
		Gtk::Label _minTimeRatioLabel;
		Gtk::Scale _minTimeRatioScale;
		Gtk::Label _minFreqRatioLabel;
		Gtk::Scale _minFreqRatioScale;
		
		Gtk::CheckButton _excludeOriginalFlags;
		
		Gtk::Button _applyButton;

		void onApplyClicked()
		{
			_action.SetEnlargeTimeSize((size_t) _dilluteTimeSizeScale.get_value());
			_action.SetEnlargeFrequencySize((size_t) _dilluteFrequencySizeScale.get_value());
			_action.SetMinAvailableTimesRatio(_minTimesAvailableRatioScale.get_value()/100.0);
			_action.SetMinAvailableFrequenciesRatio(_minFreqAvailableRatioScale.get_value()/100.0);
			_action.SetMinAvailableTFRatio(_minTFAvailableRatioScale.get_value()/100.0);
			_action.SetMinimumGoodTimeRatio(_minTimeRatioScale.get_value()/100.0);
			_action.SetMinimumGoodFrequencyRatio(_minFreqRatioScale.get_value()/100.0);
			_action.SetExcludeOriginalFlags(_excludeOriginalFlags.get_active());
			_editStrategyWindow.UpdateAction(&_action);
		}
};

#endif // STATISTICALFLAGGINGFRAME_H
