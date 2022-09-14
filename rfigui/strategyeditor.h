#ifndef STRATEGY_EDITOR_H
#define STRATEGY_EDITOR_H

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

class StrategyEditor : public Gtk::ScrolledWindow {
 public:
  StrategyEditor();

  void SetText(const std::string& text);
  std::string GetText() const;

  void HighlightLine(size_t index) {
    _highlightLine = index;
    updateHighlighting();
  }

  void ResetChangedStatus() { _isChanged = false; }

  bool IsChanged() { return _isChanged; }

 private:
  struct ParseInfo {
    enum { Clear, FunctionStart } state;
  };
  Gtk::TextView _text;
  Glib::RefPtr<Gtk::TextBuffer::Tag> _tagKeyword, _tagComment, _tagString,
      _tagFunctionName, _tagHighlight;

  void parseWord(ParseInfo& p, Gtk::TextBuffer::iterator start,
                 Gtk::TextBuffer::iterator end);
  void updateChanges() {
    _highlightLine = 0;
    updateHighlighting();
    _isChanged = true;
  }
  void updateHighlighting();
  std::size_t _highlightLine;
  bool _isChanged;
};

#endif
