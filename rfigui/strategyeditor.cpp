#include "strategyeditor.h"

StrategyEditor::StrategyEditor() {
  _text.override_font(Pango::FontDescription("monospace"));
  _text.set_wrap_mode(Gtk::WrapMode::WRAP_WORD);
  add(_text);

  _tagKeyword = _text.get_buffer()->create_tag();
  _tagKeyword->property_weight().set_value(Pango::WEIGHT_BOLD);

  _tagComment = _text.get_buffer()->create_tag();
  _tagComment->property_style().set_value(Pango::STYLE_ITALIC);
  Gdk::RGBA blue;
  blue.set_rgba(0.13, 0.0, 0.53);
  _tagComment->property_foreground_rgba().set_value(blue);

  _tagString = _text.get_buffer()->create_tag();
  Gdk::RGBA gold;
  gold.set_rgba(0.68, 0.49, 0.0);
  _tagString->property_foreground_rgba().set_value(gold);

  _tagFunctionName = _text.get_buffer()->create_tag();
  Gdk::RGBA red;
  red.set_rgba(0.65, 0.1, 0.1);
  _tagFunctionName->property_foreground_rgba().set_value(red);

  _tagHighlight = _text.get_buffer()->create_tag();
  Gdk::RGBA highlred;
  highlred.set_rgba(1.0, 0.4, 0.4);
  _tagHighlight->property_background_rgba().set_value(highlred);

  _text.get_buffer()->signal_changed().connect([&]() { updateChanges(); });
}

bool isAlpha(int c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool isAlphaNumeric(int c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') || c == '_';
}

void StrategyEditor::parseWord(ParseInfo& p, Gtk::TextBuffer::iterator start,
                               Gtk::TextBuffer::iterator end) {
  const std::string word(start, end);
  const bool isKeyword =
      word == "and" || word == "break" || word == "do" || word == "else" ||
      word == "elseif" || word == "end" || word == "false" || word == "for" ||
      word == "function" || word == "if" || word == "in" || word == "local" ||
      word == "nil" || word == "not" || word == "or" || word == "repeat" ||
      word == "return" || word == "then" || word == "true" || word == "until" ||
      word == "while";
  if (isKeyword) {
    _text.get_buffer()->apply_tag(_tagKeyword, start, end);
    if (word == "function") p.state = ParseInfo::FunctionStart;
  } else {
    if (p.state == ParseInfo::FunctionStart) {
      _text.get_buffer()->apply_tag(_tagFunctionName, start, end);
      p.state = ParseInfo::Clear;
    }
  }
}

void StrategyEditor::updateHighlighting() {
  const Glib::RefPtr<Gtk::TextBuffer> buffer = _text.get_buffer();
  auto iter = buffer->begin();
  buffer->remove_all_tags(iter, buffer->end());
  enum {
    Clear,
    InWord,
    InComment,
    AfterMinus,
    LongCommentStart,
    InLongComment,
    LongCommentEnd,
    InDQuote,
    InQuote
  } mode = Clear;
  ParseInfo p{ParseInfo::Clear};
  Gtk::TextBuffer::iterator wordStart;
  size_t lineNr = 1;
  Gtk::TextBuffer::iterator lineStart;
  while (iter != buffer->end()) {
    const int c = *iter;
    if (c == '\n') {
      if (lineNr == _highlightLine) {
        _text.get_buffer()->apply_tag(_tagHighlight, lineStart, iter);
      }
      lineStart = iter;
      ++lineStart;
      ++lineNr;
    }

    switch (mode) {
      case Clear:
      case AfterMinus:
        if (isAlpha(c)) {
          mode = InWord;
          wordStart = iter;
        } else if (c == '-') {
          if (mode == AfterMinus) {
            mode = InComment;
          } else {
            mode = AfterMinus;
            wordStart = iter;
          }
        } else if (c == '"') {
          mode = InDQuote;
          wordStart = iter;
        } else if (c == '\'') {
          mode = InQuote;
          wordStart = iter;
        }
        break;
      case InWord:
        if (!isAlphaNumeric(c)) {
          parseWord(p, wordStart, iter);
          mode = Clear;
        }
        break;
      case InComment:
        if (c == '\n') {
          _text.get_buffer()->apply_tag(_tagComment, wordStart, iter);
          mode = Clear;
          p.state = ParseInfo::Clear;
        } else if (c == '[') {
          mode = LongCommentStart;
        }
        break;
      case LongCommentStart:
        if (c == '[')
          mode = InLongComment;
        else
          mode = InComment;
        break;
      case InLongComment:
        if (c == ']') mode = LongCommentEnd;
        break;
      case LongCommentEnd:
        if (c == ']') {
          mode = Clear;
          Gtk::TextBuffer::iterator next = iter;
          ++next;
          _text.get_buffer()->apply_tag(_tagComment, wordStart, next);
        }
        break;
      case InDQuote:
        if (c == '"') {
          Gtk::TextBuffer::iterator next = iter;
          ++next;
          _text.get_buffer()->apply_tag(_tagString, wordStart, next);
          mode = Clear;
          p.state = ParseInfo::Clear;
        }
        break;
      case InQuote:
        if (c == '\'') {
          Gtk::TextBuffer::iterator next = iter;
          ++next;
          _text.get_buffer()->apply_tag(_tagString, wordStart, next);
          mode = Clear;
          p.state = ParseInfo::Clear;
        }
        break;
    }
    ++iter;
  }
}

void StrategyEditor::SetText(const std::string& text) {
  const Glib::RefPtr<Gtk::TextBuffer> buffer = _text.get_buffer();
  buffer->set_text(text);
}

std::string StrategyEditor::GetText() const {
  return std::string(_text.get_buffer()->get_text());
}
