#include "opendialog.h"
#include "rfiguiwindow.h"

OpenDialog::OpenDialog(RFIGuiWindow& rfiGuiWindow)
    : _selectButton("Select"),
      _unselectButton("Unselect"),
      _fileList(1, false, Gtk::SELECTION_SINGLE),
      _rfiGuiWindow(rfiGuiWindow) {
  get_content_area()->pack_start(_notebook);
  _notebook.append_page(_fileTab, "Files");
  _notebook.append_page(_optionsTab, "Options");

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
  _fileTab.pack_start(_fileList);

  get_content_area()->show_all();

  add_button("Cancel", Gtk::RESPONSE_CANCEL);
  _openButton = add_button("Open", Gtk::RESPONSE_OK);
  _openButton->set_sensitive(false);
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
  bool hasSelection = _fileList.get_selection()->count_selected_rows() != 0;
  _unselectButton.set_sensitive(hasSelection);
}
