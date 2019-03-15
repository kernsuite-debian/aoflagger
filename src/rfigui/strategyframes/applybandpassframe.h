#ifndef APPLY_PASSBAND_FRAME_H
#define APPLY_PASSBAND_FRAME_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/filechooserdialog.h>

#include "../../strategy/actions/applybandpassaction.h"

#include "../editstrategywindow.h"

class ApplyBandpassFrame : public Gtk::Frame {
	public:
		ApplyBandpassFrame(rfiStrategy::ApplyBandpassAction& action, EditStrategyWindow& editStrategyWindow)
		: Gtk::Frame("Apply bandpass"),
		_editStrategyWindow(editStrategyWindow), _action(action),
		_filenameLabel("Filename:"),
		_filenameBrowseButton(".."),
		_applyButton("Apply")
		{
			_box.pack_start(_filenameLabel, false, false);

			_box.pack_start(_filenameEntry, false, false);
			_filenameEntry.set_text(_action.Filename());
			
			_box.pack_start(_filenameBrowseButton, false, false);
			_filenameBrowseButton.signal_clicked().connect(sigc::mem_fun(*this, &ApplyBandpassFrame::onBrowseClicked));

			_buttonBox.pack_start(_applyButton);
			_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &ApplyBandpassFrame::onApplyClicked));

			_box.pack_start(_buttonBox);

			add(_box);
			_box.show_all();
		}
	private:
		EditStrategyWindow &_editStrategyWindow;
		rfiStrategy::ApplyBandpassAction &_action;

		Gtk::VBox _box;
		Gtk::ButtonBox _buttonBox;
		Gtk::Label _filenameLabel;
		Gtk::Entry _filenameEntry;
		Gtk::Button _filenameBrowseButton;
		Gtk::Button _applyButton;

		void onBrowseClicked()
		{
			Gtk::FileChooserDialog dialog(_editStrategyWindow, "Select bandpass file");
			//Add response buttons the the dialog:
			dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
			dialog.add_button("_Open", Gtk::RESPONSE_OK);
			
			Glib::RefPtr<Gtk::FileFilter> txtFilter = Gtk::FileFilter::create();
			txtFilter->set_name("Plain text file (*.txt)");
			txtFilter->add_pattern("*.txt");
			txtFilter->add_mime_type("text/plain");
			dialog.add_filter(txtFilter);

			int result = dialog.run();

			if(result == Gtk::RESPONSE_OK)
			{
				_filenameEntry.set_text(dialog.get_filename());
			}
		}
		
		void onApplyClicked()
		{
			_action.SetFilename(_filenameEntry.get_text());
			_editStrategyWindow.UpdateAction(&_action);
		}
};

#endif // APPLY_PASSBAND_FRAME_H

