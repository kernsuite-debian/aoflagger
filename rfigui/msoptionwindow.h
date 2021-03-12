#ifndef MSOPTIONWINDOW_H
#define MSOPTIONWINDOW_H

#include "../imagesets/msoptions.h"

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/frame.h>

#include <string>
#include <vector>

class MSOptionWindow : public Gtk::Window {
 public:
  MSOptionWindow(const std::vector<std::string>& filenames);
  void onOpen();

  sigc::signal<void, const std::vector<std::string>& /*filenames*/,
                    const MSOptions& /*options*/>&
  SignalOpenMS() {
    return _signalOpenMS;
  }

 private:
  void initDataTypeButtons();
  void initReadingModeButtons();
  void initBaselineAveragingButtons();

  const std::vector<std::string> _filenames;

  sigc::signal<void, const std::vector<std::string>& /*filenames*/,
                    const MSOptions& /*options*/>
      _signalOpenMS;

  Gtk::ButtonBox _bottomButtonBox;
  Gtk::VBox _leftVBox, _rightVBox;
  Gtk::HBox _topHBox;
  Gtk::Button _openButton;
  Gtk::Frame _dataKindFrame, _readingModeFrame, _baselineAveragingFrame;
  Gtk::VBox _dataKindBox, _baselineAveragingBox, _readingModeBox;
  Gtk::HBox _otherColumnBox, _intervalBox;
  Gtk::RadioButton _observedDataButton, _correctedDataButton, _modelDataButton,
      _residualDataButton, _otherColumnButton;
  Gtk::Entry _otherColumnEntry, _intervalStartEntry, _intervalEndEntry;
  Gtk::CheckButton _baselineAveragingButton;
  Gtk::ComboBoxText _baselineAveragingCombo;
  Gtk::CheckButton _baIncludeAutosButton, _baIncludeFlaggedButton;
  Gtk::RadioButton _baNoDifference, _baTimeDifference, _baFrequencyDifference;
  Gtk::RadioButton _directReadButton, _indirectReadButton, _memoryReadButton;
  Gtk::CheckButton _intervalButton, _combineSPWsButton;
};

#endif
