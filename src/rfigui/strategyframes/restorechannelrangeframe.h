#ifndef RESTORECHANNELRANGEFRAME_H
#define RESTORECHANNELRANGEFRAME_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>

#include "../../strategy/actions/restorechannelrangeaction.h"

#include "../editstrategywindow.h"

class RestoreChannelRangeFrame : public Gtk::Frame {
	public:
		RestoreChannelRangeFrame(rfiStrategy::RestoreChannelRangeAction &action, EditStrategyWindow &editStrategyWindow)
		: Gtk::Frame("Restore channel range"),
		_editStrategyWindow(editStrategyWindow), _action(action),
		_startFrequencyLabel("Start frequency (MHz):"),
		_endFrequencyLabel("End frequency (MHz):"),
		_applyButton("Apply")
		{
			_box.pack_start(_startFrequencyLabel);

			_box.pack_start(_startFrequencyEntry);
			std::stringstream s1;
			s1 << _action.StartFrequencyMHz();
			_startFrequencyEntry.set_text(s1.str());

			_box.pack_start(_endFrequencyLabel);

			_box.pack_start(_endFrequencyEntry);
			std::stringstream s2;
			s2 << _action.EndFrequencyMHz();
			_endFrequencyEntry.set_text(s2.str());

			_buttonBox.pack_start(_applyButton);
			_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &RestoreChannelRangeFrame::onApplyClicked));

			_box.pack_start(_buttonBox);

			add(_box);
			_box.show_all();
		}
	private:
		EditStrategyWindow &_editStrategyWindow;
		rfiStrategy::RestoreChannelRangeAction &_action;

		Gtk::VBox _box;
		Gtk::ButtonBox _buttonBox;
		Gtk::Label _startFrequencyLabel;
		Gtk::Entry _startFrequencyEntry;
		Gtk::Label _endFrequencyLabel;
		Gtk::Entry _endFrequencyEntry;
		Gtk::Button _applyButton;

		void onApplyClicked()
		{
			_action.SetStartFrequencyMHz(atof(_startFrequencyEntry.get_text().c_str()));
			_action.SetEndFrequencyMHz(atof(_endFrequencyEntry.get_text().c_str()));
			_editStrategyWindow.UpdateAction(&_action);
		}
};

#endif // RESTORECHANNELRANGEFRAME_H

