
#ifndef OPEN_DIALOG_H
#define OPEN_DIALOG_H

#include <string>

#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/listviewtext.h>
#include <gtkmm/notebook.h>

class OpenDialog : public Gtk::Dialog {
 public:
  explicit OpenDialog(class RFIGuiWindow& rfiGuiWindow);
  const std::vector<std::string>& Selection() const { return _selection; }

 private:
  void onSelectFile();
  void onUnselectFile();
  void onFileListSelectionChanged();

  Gtk::Notebook _notebook;
  Gtk::VBox _fileTab, _optionsTab;
  Gtk::FileChooserWidget _fileChooser;
  Gtk::HBox _fileButtonBox;
  Gtk::Button _selectButton;
  Gtk::Button _unselectButton;
  Gtk::ListViewText _fileList;
  Gtk::Button* _openButton;
  std::vector<std::string> _selection;

  RFIGuiWindow& _rfiGuiWindow;
};

#endif
