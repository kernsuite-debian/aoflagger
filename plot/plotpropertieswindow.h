#ifndef PLOTPROPERTIESWINDOW_H
#define PLOTPROPERTIESWINDOW_H

#include <string>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/window.h>

#include <functional>

class XYPlot;

class PlotPropertiesWindow : public Gtk::Window {
 public:
  PlotPropertiesWindow(XYPlot& plot, const std::string& title);
  ~PlotPropertiesWindow() {}

  std::function<void()> OnChangesApplied;

 private:
  void onApplyClicked();
  void onCloseClicked();
  void onExportClicked();

  void onVRangeChanged() {
    _vRangeMinEntry.set_sensitive(_specifiedVRangeButton.get_active());
    _vRangeMaxEntry.set_sensitive(_specifiedVRangeButton.get_active());
  }

  void onHRangeChanged() {
    _hRangeMinEntry.set_sensitive(!_automaticHRangeButton.get_active());
    _hRangeMaxEntry.set_sensitive(!_automaticHRangeButton.get_active());
  }

  void initVRangeWidgets();
  void initHRangeWidgets();
  void initOptionsWidgets();
  void initAxesDescriptionWidgets();

  void updateHMinMaxEntries();
  void updateVMinMaxEntries();

  XYPlot& _plot;

  Gtk::ButtonBox _bottomButtonBox;
  Gtk::VBox _topVBox;
  Gtk::HBox _framesHBox;
  Gtk::VBox _framesRightVBox;

  Gtk::Entry _titleEntry;

  Gtk::Button _applyButton, _exportButton, _closeButton;

  Gtk::Frame _vRangeFrame;
  Gtk::VBox _vRangeBox;
  Gtk::RadioButton _minMaxVRangeButton, _winsorizedVRangeButton,
      _specifiedVRangeButton;
  Gtk::Label _vRangeMinLabel, _vRangeMaxLabel;
  Gtk::Entry _vRangeMinEntry, _vRangeMaxEntry;

  Gtk::Frame _hRangeFrame;
  Gtk::VBox _hRangeBox;
  Gtk::CheckButton _automaticHRangeButton;
  Gtk::Label _hRangeMinLabel, _hRangeMaxLabel;
  Gtk::Entry _hRangeMinEntry, _hRangeMaxEntry;

  Gtk::Frame _xOptionsFrame;
  Gtk::VBox _xOptionsBox;
  Gtk::CheckButton _xLogScaleButton;

  Gtk::Frame _yOptionsFrame;
  Gtk::VBox _yOptionsBox;
  Gtk::RadioButton _yNormalOptionsButton, _yLogScaleButton,
      _yZeroSymmetricButton;

  Gtk::Frame _axesDescriptionFrame;
  Gtk::VBox _axesDescriptionBox;
  Gtk::CheckButton _hAxisDescriptionButton, _vAxisDescriptionButton;
  Gtk::Entry _hAxisDescriptionEntry, _vAxisDescriptionEntry;

  Gtk::CheckButton _showAxes, _showAxisDescriptionsButton;
};

#endif  // PLOTPROPERTIESWINDOW_H
