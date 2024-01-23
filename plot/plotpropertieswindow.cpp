#include <iostream>
#include <sstream>

#include <gtkmm/filechooserdialog.h>

#include "../util/gtkmm-compat.h"

#include "plotpropertieswindow.h"
#include "plotwidget.h"
#include "xyplot.h"

PlotPropertiesWindow::PlotPropertiesWindow(XYPlot& plot,
                                           const std::string& title)
    : Gtk::Window(),
      _plot(plot),
      _titleEntry(),

      _applyButton("_Apply", true),
      _exportButton("_Export", true),
      _closeButton("_Close", true),

      _vRangeFrame("Vertical scale"),
      _minMaxVRangeButton("From min to max"),
      _winsorizedVRangeButton("Winsorized min and max"),
      _specifiedVRangeButton("Specified:"),
      _vRangeMinLabel("Scale minimum:"),
      _vRangeMaxLabel("Scale maximum:"),
      _vRangeMinEntry(),
      _vRangeMaxEntry(),

      _hRangeFrame("Horizontal scale"),
      _automaticHRangeButton("Automatic"),
      _hRangeMinLabel("Scale minimum:"),
      _hRangeMaxLabel("Scale maximum:"),
      _hRangeMinEntry(),
      _hRangeMaxEntry(),

      _xOptionsFrame("X options"),
      _xLogScaleButton("Logarithmic scale"),

      _yOptionsFrame("Y options"),
      _yNormalOptionsButton("Normal scale"),
      _yLogScaleButton("Logarithmic scale"),
      _yZeroSymmetricButton("Symmetric around zero"),

      _axesDescriptionFrame("Axes"),
      _hAxisDescriptionButton("Horizontal description"),
      _vAxisDescriptionButton("Vertical description"),
      _hAxisDescriptionEntry(),
      _vAxisDescriptionEntry(),

      _showAxes("Show axes"),
      _showAxisDescriptionsButton("Show axis descriptions") {
  set_title(title);

  initVRangeWidgets();
  initHRangeWidgets();
  initOptionsWidgets();
  initAxesDescriptionWidgets();

  _titleEntry.set_text(_plot.GetTitle());
  _topVBox.pack_start(_titleEntry);

  _framesHBox.pack_start(_framesRightVBox);

  _applyButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onApplyClicked));
  gtkmm_set_image_from_icon_name(_applyButton, "gtk-ok");
  _bottomButtonBox.pack_start(_applyButton);

  _exportButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onExportClicked));
  gtkmm_set_image_from_icon_name(_exportButton, "document-save");
  _bottomButtonBox.pack_start(_exportButton);

  _closeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onCloseClicked));
  gtkmm_set_image_from_icon_name(_closeButton, "window-close");
  _bottomButtonBox.pack_start(_closeButton);

  _topVBox.pack_start(_framesHBox);

  _showAxes.set_active(_plot.XAxis().Show() || _plot.YAxis().Show());
  _topVBox.pack_start(_showAxes);

  _showAxisDescriptionsButton.set_active(_plot.ShowAxisDescriptions());
  _topVBox.pack_start(_showAxisDescriptionsButton);

  _topVBox.pack_start(_bottomButtonBox);

  add(_topVBox);
  _topVBox.show_all();
}

void PlotPropertiesWindow::initVRangeWidgets() {
  _vRangeFrame.add(_vRangeBox);

  Gtk::RadioButton::Group group;

  _vRangeBox.pack_start(_minMaxVRangeButton);
  _minMaxVRangeButton.set_group(group);
  _minMaxVRangeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onVRangeChanged));

  //_vRangeBox.pack_start(_winsorizedVRangeButton);
  _winsorizedVRangeButton.set_group(group);
  _winsorizedVRangeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onVRangeChanged));
  _vRangeBox.pack_start(_specifiedVRangeButton);

  _specifiedVRangeButton.set_group(group);
  _specifiedVRangeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onVRangeChanged));

  switch (_plot.YAxis().GetRangeDetermination()) {
    default:
    case RangeDetermination::MinMaxRange:
      _minMaxVRangeButton.set_active(true);
      break;
    case RangeDetermination::WinsorizedRange:
      _winsorizedVRangeButton.set_active(true);
      break;
    case RangeDetermination::SpecifiedRange:
      _specifiedVRangeButton.set_active(true);
      break;
  }
  onVRangeChanged();

  updateVMinMaxEntries();

  _vRangeBox.pack_start(_vRangeMinLabel);
  _vRangeBox.pack_start(_vRangeMinEntry);

  _vRangeBox.pack_start(_vRangeMaxLabel);
  _vRangeBox.pack_start(_vRangeMaxEntry);

  _framesHBox.pack_start(_vRangeFrame);
}

void PlotPropertiesWindow::initHRangeWidgets() {
  _hRangeFrame.add(_hRangeBox);

  const Gtk::RadioButton::Group group;

  _hRangeBox.pack_start(_automaticHRangeButton);
  _automaticHRangeButton.set_active(_plot.XAxis().GetRangeDetermination() !=
                                    RangeDetermination::SpecifiedRange);
  _automaticHRangeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotPropertiesWindow::onHRangeChanged));

  onHRangeChanged();

  updateHMinMaxEntries();

  _hRangeBox.pack_start(_hRangeMinLabel);
  _hRangeBox.pack_start(_hRangeMinEntry);

  _hRangeBox.pack_start(_hRangeMaxLabel);
  _hRangeBox.pack_start(_hRangeMaxEntry);

  _framesHBox.pack_start(_hRangeFrame);
}

void PlotPropertiesWindow::initOptionsWidgets() {
  Gtk::RadioButton::Group group;

  _xOptionsBox.pack_start(_xLogScaleButton);
  _xLogScaleButton.set_active(_plot.XAxis().Logarithmic());
  _xOptionsFrame.add(_xOptionsBox);
  _framesRightVBox.pack_start(_xOptionsFrame);

  _yOptionsBox.pack_start(_yNormalOptionsButton);
  _yNormalOptionsButton.set_group(group);

  _yOptionsBox.pack_start(_yLogScaleButton);
  _yLogScaleButton.set_group(group);

  _yOptionsBox.pack_start(_yZeroSymmetricButton);
  _yZeroSymmetricButton.set_group(group);

  if (_plot.YAxis().Logarithmic())
    _yLogScaleButton.set_active(true);
  else
    _yNormalOptionsButton.set_active(true);

  _yOptionsFrame.add(_yOptionsBox);

  _framesRightVBox.pack_start(_yOptionsFrame);
}

void PlotPropertiesWindow::initAxesDescriptionWidgets() {
  _axesDescriptionBox.pack_start(_hAxisDescriptionButton);
  _axesDescriptionBox.pack_start(_hAxisDescriptionEntry);
  _axesDescriptionBox.pack_start(_vAxisDescriptionButton);
  _axesDescriptionBox.pack_start(_vAxisDescriptionEntry);

  _axesDescriptionFrame.add(_axesDescriptionBox);

  _framesRightVBox.pack_start(_axesDescriptionFrame);
}

void PlotPropertiesWindow::updateHMinMaxEntries() {
  auto range = _plot.RangeX(false);
  std::stringstream minStr;
  minStr << range.first;
  _hRangeMinEntry.set_text(minStr.str());

  std::stringstream maxStr;
  maxStr << range.second;
  _hRangeMaxEntry.set_text(maxStr.str());
}

void PlotPropertiesWindow::updateVMinMaxEntries() {
  auto range = _plot.RangeY(false);
  std::stringstream minStr;
  minStr << range.first;
  _vRangeMinEntry.set_text(minStr.str());

  std::stringstream maxStr;
  maxStr << range.second;
  _vRangeMaxEntry.set_text(maxStr.str());
}

void PlotPropertiesWindow::onApplyClicked() {
  _plot.SetTitle(_titleEntry.get_text());

  if (_minMaxVRangeButton.get_active()) {
    _plot.YAxis().SetRangeDetermination(RangeDetermination::MinMaxRange);
  } else if (_winsorizedVRangeButton.get_active()) {
    _plot.YAxis().SetRangeDetermination(RangeDetermination::WinsorizedRange);
  } else if (_specifiedVRangeButton.get_active()) {
    _plot.YAxis().SetRangeDetermination(RangeDetermination::SpecifiedRange);
    _plot.YAxis().SetMin(atof(_vRangeMinEntry.get_text().c_str()));
    _plot.YAxis().SetMax(atof(_vRangeMaxEntry.get_text().c_str()));
  }

  if (_automaticHRangeButton.get_active()) {
    _plot.XAxis().SetRangeDetermination(RangeDetermination::MinMaxRange);
  } else {
    _plot.XAxis().SetRangeDetermination(RangeDetermination::SpecifiedRange);
    _plot.XAxis().SetMin(atof(_hRangeMinEntry.get_text().c_str()));
    _plot.XAxis().SetMax(atof(_hRangeMaxEntry.get_text().c_str()));
  }

  _plot.XAxis().SetLogarithmic(_xLogScaleButton.get_active());

  if (_yNormalOptionsButton.get_active())
    _plot.YAxis().SetLogarithmic(false);
  else if (_yLogScaleButton.get_active())
    _plot.YAxis().SetLogarithmic(true);

  if (_hAxisDescriptionButton.get_active())
    _plot.XAxis().SetCustomDescription(
        _hAxisDescriptionEntry.get_text().c_str());
  else
    _plot.XAxis().SetAutomaticDescription();

  if (_vAxisDescriptionButton.get_active())
    _plot.YAxis().SetCustomDescription(
        _vAxisDescriptionEntry.get_text().c_str());
  else
    _plot.YAxis().SetAutomaticDescription();

  _plot.XAxis().SetShow(_showAxes.get_active());
  _plot.YAxis().SetShow(_showAxes.get_active());
  _plot.SetShowAxisDescriptions(_showAxisDescriptionsButton.get_active());

  if (OnChangesApplied) OnChangesApplied();

  updateHMinMaxEntries();
  updateVMinMaxEntries();
}

void PlotPropertiesWindow::onCloseClicked() { hide(); }

void PlotPropertiesWindow::onExportClicked() {
  Gtk::FileChooserDialog dialog("Specify image filename",
                                Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.set_transient_for(*this);

  // Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Save", Gtk::RESPONSE_OK);

  const Glib::RefPtr<Gtk::FileFilter> pdfFilter = Gtk::FileFilter::create();
  const std::string pdfName = "Portable Document Format (*.pdf)";
  pdfFilter->set_name(pdfName);
  pdfFilter->add_pattern("*.pdf");
  pdfFilter->add_mime_type("application/pdf");
  dialog.add_filter(pdfFilter);

  const Glib::RefPtr<Gtk::FileFilter> svgFilter = Gtk::FileFilter::create();
  const std::string svgName = "Scalable Vector Graphics (*.svg)";
  svgFilter->set_name(svgName);
  svgFilter->add_pattern("*.svg");
  svgFilter->add_mime_type("image/svg+xml");
  dialog.add_filter(svgFilter);

  const Glib::RefPtr<Gtk::FileFilter> pngFilter = Gtk::FileFilter::create();
  const std::string pngName = "Portable Network Graphics (*.png)";
  pngFilter->set_name(pngName);
  pngFilter->add_pattern("*.png");
  pngFilter->add_mime_type("image/png");
  dialog.add_filter(pngFilter);

  const int result = dialog.run();

  if (result == Gtk::RESPONSE_OK) {
    const Glib::RefPtr<Gtk::FileFilter> filter = dialog.get_filter();
    if (filter->get_name() == pdfName)
      _plot.SavePdf(dialog.get_filename(), _plot.Width(), _plot.Height());
    else if (filter->get_name() == svgName)
      _plot.SaveSvg(dialog.get_filename(), _plot.Width(), _plot.Height());
    else
      _plot.SavePng(dialog.get_filename(), _plot.Width(), _plot.Height());
  }
}
