#include "opendialog.h"
#include "rfiguiwindow.h"

OpenDialog::OpenDialog(RFIGuiWindow& rfiGuiWindow)
    : _rfiGuiWindow(rfiGuiWindow),
      _selectButton("Select"),
      _unselectButton("Unselect"),
      // File tab elements
      _fileScroller(),
      _fileList(1, false, Gtk::SELECTION_SINGLE),
      // Options tab elements
      _dataColumnLabel("Data column: "),
      _otherColumnLabel("Other column: "),
      _combineSPWsCB("Combine SPWs"),
      _concatenateFrequency("Spectrally concatenate multiple measurement sets"),
      _readingModeLabel("Reading mode: "),
      _readingModeExplanation(
          "Set how the measurement set is read: in direct mode, the "
          "information for one baseline is gathered by scanning through the "
          "entire baseline "
          "each time a baseline is requested. It is the recommended mode "
          "for interactive experimentation. The indirect and memory modes "
          "reorder the baselines "
          "in a temporary file or memory, respectively, which takes time at "
          "the start, but allows "
          "faster browsing through the baselines."),
      _baselineAveragingCB("Average baselines together"),
      _baselineAveragingCombo(),
      _baIncludeAutosCB("Include auto-correlations"),
      _baIncludeFlaggedCB("Include flagged values"),
      _baNoDifference("Use sample values"),
      _baTimeDifference("Use time difference"),
      _baFrequencyDifference("Use frequency difference"),
      _intervalCB("Interval"),
      _intervalExplanation(
          "Limit the timeslots that are read. If only an interval start is "
          "entered, the data set is read to the end.") {
  get_content_area()->pack_start(_notebook);

  _notebook.append_page(_fileTab, "Files");
  _fileChooser.signal_file_activated().connect([&]() { onSelectFile(); });
  _fileChooser.set_size_request(800, 400);
  _fileTab.pack_start(_fileChooser);

  _unselectButton.set_hexpand(false);
  _unselectButton.set_vexpand(false);
  _unselectButton.set_sensitive(false);
  _unselectButton.signal_clicked().connect([&]() { onUnselectFile(); });
  _fileButtonBox.pack_start(_unselectButton);

  _selectButton.set_hexpand(false);
  _selectButton.set_vexpand(false);
  _selectButton.signal_clicked().connect([&]() { onSelectFile(); });
  _fileButtonBox.pack_end(_selectButton);

  _fileButtonBox.set_hexpand(false);
  _fileButtonBox.set_vexpand(false);
  _fileTab.pack_start(_fileButtonBox, false, false);

  _fileList.set_column_title(0, "Selected file(s)");
  _fileList.set_size_request(0, 100);
  _fileList.get_selection()->signal_changed().connect(
      [&]() { onFileListSelectionChanged(); });

  _fileScroller.add(_fileList);

  _fileTab.pack_start(_fileScroller);

  _notebook.append_page(_optionsTab, "Options");
  _optionsTab.set_hexpand(true);
  _optionsTab.set_halign(Gtk::ALIGN_FILL);
  _optionsTab.set_column_spacing(15);
  _optionsTab.set_row_spacing(5);
  _optionsTab.set_margin_top(10);
  _optionsTab.set_margin_left(10);
  _optionsTab.set_margin_right(10);

  size_t gridRow = 0;
  initDataColumnButtons(gridRow);
  initReadingModeButtons(gridRow);
  initBaselineAveragingButtons(gridRow);

  _optionsTab.attach(_intervalCB, 0, gridRow, 1, 1);
  auto onChange = [&]() {
    const bool enabled = _intervalCB.get_active();
    _intervalStartEntry.set_sensitive(enabled);
    _intervalEndEntry.set_sensitive(enabled);
  };
  _intervalCB.signal_clicked().connect(onChange);
  onChange();
  _intervalStartEntry.set_width_chars(5);
  _intervalStartEntry.set_text("0");
  _intervalBox.pack_start(_intervalStartEntry, false, false, 5);
  _intervalEndEntry.set_width_chars(5);
  _intervalEndEntry.set_text("...");
  _intervalBox.pack_start(_intervalEndEntry, false, false, 5);
  _optionsTab.attach(_intervalBox, 1, gridRow, 1, 1);
  ++gridRow;
  _intervalExplanation.set_max_width_chars(80);
  _intervalExplanation.set_line_wrap(true);
  _optionsTab.attach(_intervalExplanation, 0, gridRow, 2, 1);
  ++gridRow;

  _optionsTab.attach(_bottomButtonBox, 0, gridRow, 2, 1);
  ++gridRow;

  get_content_area()->show_all();

  add_button("Cancel", Gtk::RESPONSE_CANCEL);
  _openButton = add_button("Open", Gtk::RESPONSE_OK);
  _openButton->set_sensitive(false);
}

void OpenDialog::initDataColumnButtons(size_t& gridRow) {
  _optionsTab.attach(_dataColumnLabel, 0, gridRow, 1, 1);
  _dataColumnCombo.append("DATA");
  _dataColumnCombo.append("CORRECTED_DATA");
  _dataColumnCombo.append("MODEL_DATA");
  _dataColumnCombo.append("other...");
  _dataColumnCombo.signal_changed().connect([&]() {
    const bool isOther = _dataColumnCombo.get_active_row_number() == 3;
    _otherColumnLabel.set_sensitive(isOther);
    _otherColumnEntry.set_sensitive(isOther);
  });
  _dataColumnCombo.set_active(0);
  _dataColumnCombo.set_hexpand(true);
  _dataColumnCombo.set_halign(Gtk::ALIGN_FILL);
  _optionsTab.attach(_dataColumnCombo, 1, gridRow, 1, 1);

  _otherColumnLabel.set_halign(Gtk::ALIGN_END);
  _otherColumnGrid.attach(_otherColumnLabel, 0, 0, 1, 1);
  _otherColumnEntry.set_hexpand(true);
  _otherColumnEntry.set_halign(Gtk::ALIGN_FILL);
  _otherColumnGrid.attach(_otherColumnEntry, 1, 0, 1, 1);
  _otherColumnGrid.set_margin_bottom(10);
  _optionsTab.attach(_otherColumnGrid, 1, gridRow + 1, 1, 1);
  gridRow += 2;
  _optionsTab.attach(_combineSPWsCB, 0, gridRow, 2, 1);
  _combineSPWsCB.signal_toggled().connect([&] {
    // The Combine SPWs and Concatenate Frequency are mutually exclusive.
    _concatenateFrequency.set_sensitive(!_combineSPWsCB.get_active());
  });
  ++gridRow;
  _optionsTab.attach(_concatenateFrequency, 0, gridRow, 2, 1);
  _concatenateFrequency.signal_toggled().connect([&] {
    // The Combine SPWs and Concatenate Frequency are mutually exclusive.
    _combineSPWsCB.set_sensitive(!_concatenateFrequency.get_active());
  });
  ++gridRow;
  _optionsTab.attach(_columnSeparator, 0, gridRow, 4, 1);
  ++gridRow;
}

void OpenDialog::initReadingModeButtons(size_t& gridRow) {
  _readingModeLabel.set_hexpand(false);
  _optionsTab.attach(_readingModeLabel, 0, gridRow, 1, 1);
  _optionsTab.attach(_readingModeCombo, 1, gridRow, 1, 1);
  ++gridRow;
  _readingModeCombo.append("Direct");
  _readingModeCombo.append("Indirect");
  _readingModeCombo.append("Memory");
  _readingModeCombo.set_active(0);
  _readingModeExplanation.set_line_wrap(true);
  _readingModeExplanation.set_max_width_chars(80);
  _readingModeExplanation.set_margin_bottom(10);
  _optionsTab.attach(_readingModeExplanation, 0, gridRow, 2, 1);
  ++gridRow;
  _optionsTab.attach(_readingSeparator, 0, gridRow, 2, 1);
  ++gridRow;
}

void OpenDialog::initBaselineAveragingButtons(size_t& gridRow) {
  _optionsTab.attach(_baselineAveragingCB, 0, gridRow, 2, 1);
  auto onChange = [&]() {
    const bool enabled = _baselineAveragingCB.get_active();
    _baselineAveragingCombo.set_sensitive(enabled);
    _baIncludeAutosCB.set_sensitive(enabled);
    _baIncludeFlaggedCB.set_sensitive(enabled);
    _baNoDifference.set_sensitive(enabled);
    _baTimeDifference.set_sensitive(enabled);
    _baFrequencyDifference.set_sensitive(enabled);
  };
  _baselineAveragingCB.signal_clicked().connect(onChange);
  onChange();
  ++gridRow;
  _baselineAveragingCombo.append("Normal average");
  _baselineAveragingCombo.append("Average of absolute values");
  _baselineAveragingCombo.append("Average of squared values");
  _baselineAveragingCombo.append("Standard deviation");
  _baselineAveragingCombo.set_active(0);
  _optionsTab.attach(_baselineAveragingCombo, 1, gridRow, 1, 1);
  ++gridRow;
  _optionsTab.attach(_baIncludeAutosCB, 1, gridRow, 1, 1);
  ++gridRow;
  _optionsTab.attach(_baIncludeFlaggedCB, 1, gridRow, 1, 1);
  ++gridRow;

  Gtk::RadioButton::Group group;
  _baNoDifference.set_group(group);
  _baTimeDifference.set_group(group);
  _baFrequencyDifference.set_group(group);
  _optionsTab.attach(_baNoDifference, 1, gridRow, 1, 1);
  ++gridRow;
  _optionsTab.attach(_baTimeDifference, 1, gridRow, 1, 1);
  ++gridRow;
  _optionsTab.attach(_baFrequencyDifference, 1, gridRow, 1, 1);
  ++gridRow;
  _optionsTab.attach(_baselineAveragingSeparator, 0, gridRow, 2, 1);
  ++gridRow;
}

void OpenDialog::SetSelection(const std::vector<std::string>& selection) {
  _selection = selection;
  _fileList.clear_items();
  for (const std::string& f : _selection) _fileList.append(f);
  _openButton->set_sensitive(!_selection.empty());
}

void OpenDialog::onSelectFile() {
  _selection.push_back(_fileChooser.get_file()->get_path());
  _fileList.append(_selection.back());
  _openButton->set_sensitive(true);
}

void OpenDialog::onUnselectFile() {
  const std::vector<int> unselectList = _fileList.get_selected();
  for (std::vector<int>::const_reverse_iterator i = unselectList.rbegin();
       i != unselectList.rend(); ++i) {
    _selection.erase(_selection.begin() + *i);
  }
  _fileList.clear_items();
  for (const std::string& f : _selection) _fileList.append(f);
  _openButton->set_sensitive(!_selection.empty());
}

void OpenDialog::onFileListSelectionChanged() {
  const bool hasSelection =
      _fileList.get_selection()->count_selected_rows() != 0;
  _unselectButton.set_sensitive(hasSelection);
}

MSOptions OpenDialog::GetOptions() const {
  MSOptions options;
  switch (_readingModeCombo.get_active_row_number()) {
    case 0:
      options.ioMode = DirectReadMode;
      break;
    case 1:
      options.ioMode = ReorderingReadMode;
      break;
    case 2:
      options.ioMode = MemoryReadMode;
      break;
  }
  options.combineSPWs = _combineSPWsCB.get_active();
  options.concatenateFrequency = _concatenateFrequency.get_active();

  options.dataColumnName = _dataColumnCombo.get_active_text();
  if (options.dataColumnName == "other...")
    options.dataColumnName = _otherColumnEntry.get_text();

  if (_intervalCB.get_active()) {
    options.intervalStart = atoi(_intervalStartEntry.get_text().c_str());
    const std::string endStr = _intervalEndEntry.get_text();
    if (!endStr.empty() && endStr != "...")
      options.intervalEnd = atoi(_intervalEndEntry.get_text().c_str());
  }

  options.baselineIntegration.enable = _baselineAveragingCB.get_active();
  options.baselineIntegration.withAutos = _baIncludeAutosCB.get_active();
  options.baselineIntegration.withFlagged = _baIncludeFlaggedCB.get_active();
  options.baselineIntegration.mode = BaselineIntegration::Average;
  switch (_baselineAveragingCombo.get_active_row_number()) {
    case 0:
      options.baselineIntegration.mode = BaselineIntegration::Average;
      break;
    case 1:
      options.baselineIntegration.mode = BaselineIntegration::AverageAbs;
      break;
    case 2:
      options.baselineIntegration.mode = BaselineIntegration::Squared;
      break;
    case 3:
      options.baselineIntegration.mode = BaselineIntegration::Stddev;
      break;
  }
  if (_baTimeDifference.get_active())
    options.baselineIntegration.differencing =
        BaselineIntegration::TimeDifference;
  else if (_baFrequencyDifference.get_active())
    options.baselineIntegration.differencing =
        BaselineIntegration::FrequencyDifference;
  else
    options.baselineIntegration.differencing =
        BaselineIntegration::NoDifference;
  return options;
}
