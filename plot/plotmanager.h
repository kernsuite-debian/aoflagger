#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <functional>
#include <set>
#include <vector>

#include "plot2d.h"

class PlotManager {
 public:
  ~PlotManager() { clear(); }
  Plot2D &NewPlot2D(const std::string &plotTitle) {
    std::string title = plotTitle;
    if (_plotTitles.find(title) != _plotTitles.end()) {
      char addChar = 'B';
      std::string tryTitle;
      do {
        tryTitle = title + " (" + addChar + ')';
        ++addChar;
      } while (_plotTitles.find(tryTitle) != _plotTitles.end() &&
               addChar <= 'Z');
      if (addChar > 'Z') tryTitle = title + " (..)";
      title = tryTitle;
    }
    std::unique_ptr<Plot2D> plot(new Plot2D());
    plot->SetTitle(title);
    _plotTitles.insert(title);
    _items.push_back(std::move(plot));
    return *_items.back();
  }

  void Update() { _onUpdate(); }

  std::function<void()> &OnUpdate() { return _onUpdate; }

  const std::vector<std::unique_ptr<Plot2D>> &Items() const { return _items; }

  void Clear() {
    clear();
    Update();
  }

 private:
  void clear() { _items.clear(); }
  std::vector<std::unique_ptr<Plot2D>> _items;
  std::set<std::string> _plotTitles;

  std::function<void()> _onUpdate;
};

#endif
