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
  }
  void updateHighlighting();
  std::size_t _highlightLine;
};

#endif
