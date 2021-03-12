#include <iostream>

#include "../util/gtkmm-compat.h"

#include "msoptionwindow.h"

#include "../imagesets/msimageset.h"

#include "../lua/telescopefile.h"

MSOptionWindow::MSOptionWindow(const std::vector<std::string>& filenames)
    : Gtk::Window(),
      _filenames(filenames),
      _openButton("_Open", true),
      _dataKindFrame("Columns to read"),
      _readingModeFrame("Reading mode"),
      _baselineAveragingFrame("Baseline averaging"),
      _observedDataButton("Observed"),
      _correctedDataButton("Corrected"),
      _modelDataButton("Model"),
      _residualDataButton("Residual"),
      _otherColumnButton("Other:"),
      _baselineAveragingButton("Integrate baselines"),
      _baselineAveragingCombo(),
      _baIncludeAutosButton("Include auto-correlations"),
      _baIncludeFlaggedButton("Include flagged values"),
      _baNoDifference("Use sample values"),
      _baTimeDifference("Use time difference"),
      _baFrequencyDifference("Use frequency difference"),
      _directReadButton("Direct IO"),
      _indirectReadButton("Indirect IO"),
      _memoryReadButton("Memory-mode IO"),
      _intervalButton("Interval"),
      _combineSPWsButton("Combine SPWs") {
  set_title("Options for opening a measurement set");

  initDataTypeButtons();
  initReadingModeButtons();
  initBaselineAveragingButtons();

  gtkmm_set_image_from_icon_name(_openButton, "document-open");
  _openButton.signal_clicked().connect(
      sigc::mem_fun(*this, &MSOptionWindow::onOpen));
  _bottomButtonBox.pack_start(_openButton);

  _intervalBox.pack_start(_intervalButton);
  _intervalStartEntry.set_width_chars(5);
  _intervalStartEntry.set_text("0");
  _intervalBox.pack_start(_intervalStartEntry, true, true, 5);
  _intervalEndEntry.set_width_chars(5);
  _intervalEndEntry.set_text("...");
  _intervalBox.pack_start(_intervalEndEntry, true, true, 5);
  _rightVBox.pack_start(_intervalBox);

  _rightVBox.pack_start(_combineSPWsButton);

  _rightVBox.pack_start(_bottomButtonBox);

  _topHBox.pack_start(_leftVBox);
  _topHBox.pack_start(_rightVBox);

  add(_topHBox);
  show_all();
}

void MSOptionWindow::initDataTypeButtons() {
  Gtk::RadioButton::Group group = _observedDataButton.get_group();
  _correctedDataButton.set_group(group);
  _modelDataButton.set_group(group);
  _residualDataButton.set_group(group);
  _otherColumnButton.set_group(group);

  _dataKindBox.pack_start(_observedDataButton);
  _dataKindBox.pack_start(_correctedDataButton);
  _dataKindBox.pack_start(_modelDataButton);
  _dataKindBox.pack_start(_residualDataButton);

  _otherColumnBox.pack_start(_otherColumnButton);
  _otherColumnBox.pack_start(_otherColumnEntry);
  _dataKindBox.pack_start(_otherColumnBox);

  _dataKindFrame.add(_dataKindBox);

  _leftVBox.pack_start(_dataKindFrame);
}

void MSOptionWindow::initBaselineAveragingButtons() {
  _baselineAveragingBox.pack_start(_baselineAveragingButton);
  _baselineAveragingCombo.append("Average");
  _baselineAveragingCombo.append("Averaged absolute");
  _baselineAveragingCombo.append("Averaged square");
  _baselineAveragingCombo.append("Standard deviation");
  _baselineAveragingCombo.set_active(0);
  _baselineAveragingBox.pack_start(_baselineAveragingCombo);
  _baselineAveragingBox.pack_start(_baIncludeAutosButton);
  _baselineAveragingBox.pack_start(_baIncludeFlaggedButton);

  Gtk::RadioButton::Group group;
  _baNoDifference.set_group(group);
  _baTimeDifference.set_group(group);
  _baFrequencyDifference.set_group(group);
  _baselineAveragingBox.pack_start(_baNoDifference);
  _baselineAveragingBox.pack_start(_baTimeDifference);
  _baselineAveragingBox.pack_start(_baFrequencyDifference);

  _baselineAveragingFrame.add(_baselineAveragingBox);

  _leftVBox.pack_start(_baselineAveragingFrame);
}

void MSOptionWindow::initReadingModeButtons() {
  Gtk::RadioButton::Group group;
  _directReadButton.set_group(group);
  _indirectReadButton.set_group(group);
  _memoryReadButton.set_group(group);
  _directReadButton.set_active(true);

  _readingModeBox.pack_start(_directReadButton);
  _readingModeBox.pack_start(_indirectReadButton);
  _readingModeBox.pack_start(_memoryReadButton);

  _readingModeFrame.add(_readingModeBox);
  _rightVBox.pack_start(_readingModeFrame);
}

void MSOptionWindow::onOpen() {
  MSOptions options;

  if (_indirectReadButton.get_active())
    options.ioMode = IndirectReadMode;
  else if (_memoryReadButton.get_active())
    options.ioMode = MemoryReadMode;
  else
    options.ioMode = DirectReadMode;

  options.combineSPWs = _combineSPWsButton.get_active();
  options.subtractModel = false;

  if (_observedDataButton.get_active())
    options.dataColumnName = "DATA";
  else if (_correctedDataButton.get_active())
    options.dataColumnName = "CORRECTED_DATA";
  else if (_modelDataButton.get_active())
    options.dataColumnName = "MODEL_DATA";
  else if (_residualDataButton.get_active()) {
    options.dataColumnName = "DATA";
    options.subtractModel = true;
  } else if (_otherColumnButton.get_active())
    options.dataColumnName = _otherColumnEntry.get_text();

  if (_intervalButton.get_active()) {
    options.intervalStart = atoi(_intervalStartEntry.get_text().c_str());
    std::string endStr = _intervalEndEntry.get_text();
    if (!endStr.empty() && endStr != "...")
      options.intervalEnd = atoi(_intervalEndEntry.get_text().c_str());
  }

  options.baselineIntegration.enable = _baselineAveragingButton.get_active();
  options.baselineIntegration.withAutos = _baIncludeAutosButton.get_active();
  options.baselineIntegration.withFlagged =
      _baIncludeFlaggedButton.get_active();
  options.baselineIntegration.mode = BaselineIntegration::Average;
  std::string text = _baselineAveragingCombo.get_active_text();
  if (text == "Average")
    options.baselineIntegration.mode = BaselineIntegration::Average;
  else if (text == "Averaged absolute")
    options.baselineIntegration.mode = BaselineIntegration::AverageAbs;
  else if (text == "Averaged square")
    options.baselineIntegration.mode = BaselineIntegration::Squared;
  else if (text == "Standard deviation")
    options.baselineIntegration.mode = BaselineIntegration::Stddev;
  if (_baTimeDifference.get_active())
    options.baselineIntegration.differencing =
        BaselineIntegration::TimeDifference;
  else if (_baFrequencyDifference.get_active())
    options.baselineIntegration.differencing =
        BaselineIntegration::FrequencyDifference;
  else
    options.baselineIntegration.differencing =
        BaselineIntegration::NoDifference;

  _signalOpenMS(_filenames, options);

  hide();
}
