#ifndef SIMULATE_WINDOW_H
#define SIMULATE_WINDOW_H

#include <string>

#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

#include "../structures/timefrequencydata.h"

class SimulateDialog : public Gtk::Dialog {
 public:
  SimulateDialog();
  ~SimulateDialog() {}

  TimeFrequencyData Make() const;

 private:
  void onSimulateClicked();
  void onCloseClicked();

  Gtk::Grid _grid;

  Gtk::Label _nTimesLabel;
  Gtk::Entry _nTimesEntry;
  Gtk::Label _nChannelsLabel;
  Gtk::Entry _nChannelsEntry;
  Gtk::Label _bandwidthLabel;
  Gtk::Entry _bandwidthEntry;
  Gtk::Label _polarizationsLabel;
  Gtk::ComboBoxText _polarizationsSelection;
  Gtk::Label _targetLabel;
  Gtk::ComboBoxText _targetSelection;
  Gtk::Label _noiseLabel;
  Gtk::ComboBoxText _noiseSelection;
  Gtk::Label _noiseLevelLabel;
  Gtk::Entry _noiseLevelEntry;
  Gtk::Label _rfiLabel;
  Gtk::ComboBoxText _rfiSelection;

  Gtk::Button* _simulateButton;
};

#endif  // IMAGEPROPERTIESWINDOW_H
