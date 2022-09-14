
#ifndef OPEN_DIALOG_H
#define OPEN_DIALOG_H

#include <string>

#include "../imagesets/msoptions.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/frame.h>
#include <gtkmm/grid.h>
#include <gtkmm/listviewtext.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>

class OpenDialog : public Gtk::Dialog {
 public:
  explicit OpenDialog(class RFIGuiWindow& rfiGuiWindow);
  const std::vector<std::string>& Selection() const { return _selection; }
  void SetSelection(const std::vector<std::string>& selection);
  MSOptions GetOptions() const;
  void ActivateOptionsTab() { _notebook.set_current_page(1); }

 private:
  void onSelectFile();
  void onUnselectFile();
  void onFileListSelectionChanged();
  void onOpen();

  void initDataColumnButtons(size_t& gridRow);
  void initReadingModeButtons(size_t& gridRow);
  void initBaselineAveragingButtons(size_t& gridRow);

  RFIGuiWindow& _rfiGuiWindow;
  Gtk::Notebook _notebook;
  Gtk::VBox _fileTab;
  Gtk::Grid _optionsTab;

  // File tab elements
  Gtk::FileChooserWidget _fileChooser;
  Gtk::HBox _fileButtonBox;
  Gtk::Button _selectButton;
  Gtk::Button _unselectButton;
  Gtk::ScrolledWindow _fileScroller;
  Gtk::ListViewText _fileList;
  Gtk::Button* _openButton;
  std::vector<std::string> _selection;

  // Options tab elements
  Gtk::HBox _intervalBox;
  Gtk::Label _dataColumnLabel;
  Gtk::ComboBoxText _dataColumnCombo;
  Gtk::Grid _otherColumnGrid;
  Gtk::Label _otherColumnLabel;
  Gtk::Entry _otherColumnEntry;
  Gtk::CheckButton _combineSPWsCB;
  Gtk::CheckButton _concatenateFrequency;
  Gtk::Separator _columnSeparator;
  Gtk::Label _readingModeLabel;
  Gtk::Label _readingModeExplanation;
  Gtk::ComboBoxText _readingModeCombo;
  Gtk::Separator _readingSeparator;
  Gtk::CheckButton _baselineAveragingCB;
  Gtk::ComboBoxText _baselineAveragingCombo;
  Gtk::CheckButton _baIncludeAutosCB, _baIncludeFlaggedCB;
  Gtk::RadioButton _baNoDifference, _baTimeDifference, _baFrequencyDifference;
  Gtk::Separator _baselineAveragingSeparator;
  Gtk::CheckButton _intervalCB;
  Gtk::Entry _intervalStartEntry, _intervalEndEntry;
  Gtk::Label _intervalExplanation;

  // Other
  Gtk::ButtonBox _bottomButtonBox;
};

#endif
