#include "plotwindow.h"

#include "../plot/plotmanager.h"
#include "../plot/plotpropertieswindow.h"

PlotWindow::PlotWindow(PlotManager& plotManager)
    : _plotManager(plotManager),
      _clearButton("_Clear"),
      _editButton("_Edit"),
      _plotPropertiesWindow(nullptr) {
  Gtk::ToolButton();
  plotManager.OnUpdate() = std::bind(&PlotWindow::handleUpdate, this);

  _hBox.pack_start(_plotWidget, Gtk::PACK_EXPAND_WIDGET);

  _plotListStore = Gtk::ListStore::create(_plotListColumns);
  _plotListView.set_model(_plotListStore);
  _plotListView.append_column("Plot title", _plotListColumns._name);
  _plotListView.get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &PlotWindow::onSelectedPlotChange));

  _clearButton.set_tooltip_text("Clear the list of plots");
  _clearButton.set_icon_name("edit-clear");
  _clearButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotWindow::onClearPlotsPressed));
  _toolbar.append(_clearButton);
  _editButton.set_tooltip_text("Edit the properties of the selected plot");
  _editButton.set_icon_name("document-properties");
  _editButton.signal_clicked().connect(
      sigc::mem_fun(*this, &PlotWindow::onEditPlottingPropertiesPressed));
  _toolbar.append(_editButton);
  _toolbar.set_icon_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
  _toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);
  _sideBox.pack_start(_toolbar, Gtk::PACK_SHRINK);
  _sideBox.pack_start(_plotListView);

  _hBox.pack_end(_sideBox, false, false, 3);

  add(_hBox);
  _hBox.show_all();
}

PlotWindow::~PlotWindow() { delete _plotPropertiesWindow; }

void PlotWindow::handleUpdate() {
  updatePlotList();

  const std::vector<std::unique_ptr<XYPlot>>& plots = _plotManager.Items();
  if (!plots.empty()) {
    XYPlot& lastPlot = **plots.rbegin();
    const size_t index = plots.size() - 1;
    _plotWidget.SetPlot(lastPlot);
    for (Gtk::TreeNodeChildren::iterator i = _plotListStore->children().begin();
         i != _plotListStore->children().end(); ++i) {
      if ((*i)[_plotListColumns._index] == index) {
        _plotListView.get_selection()->select(i);
        break;
      }
    }
  } else {
    _plotWidget.Clear();
    delete _plotPropertiesWindow;
    _plotPropertiesWindow = nullptr;
  }
  show();
  raise();
}

void PlotWindow::updatePlotList() {
  const std::vector<std::unique_ptr<XYPlot>>& plots = _plotManager.Items();

  _plotListView.get_selection()->unselect_all();
  _plotListStore->clear();
  for (size_t index = 0; index != plots.size(); ++index) {
    const XYPlot& plot = *plots[index];
    const Gtk::TreeModel::Row row = *_plotListStore->append();
    row[_plotListColumns._index] = index;
    row[_plotListColumns._name] = plot.GetTitle();
  }
  onSelectedPlotChange();
}

void PlotWindow::onSelectedPlotChange() {
  const Gtk::TreeModel::iterator iter =
      _plotListView.get_selection()->get_selected();
  if (iter)  // If anything is selected
  {
    const Gtk::TreeModel::Row row = *iter;
    const size_t index = row[_plotListColumns._index];
    XYPlot& plot = *_plotManager.Items()[index];
    _plotWidget.SetPlot(plot);
  }
}

void PlotWindow::onClearPlotsPressed() {
  _plotListView.get_selection()->unselect_all();
  _plotManager.Clear();
}

void PlotWindow::onEditPlottingPropertiesPressed() {
  delete _plotPropertiesWindow;
  const Gtk::TreeModel::iterator iter =
      _plotListView.get_selection()->get_selected();
  if (iter) {
    XYPlot& plot = *_plotManager.Items()[(*iter)[_plotListColumns._index]];
    _plotPropertiesWindow = new PlotPropertiesWindow(plot, "Plot properties");
    _plotPropertiesWindow->OnChangesApplied =
        std::bind(&PlotWindow::onPlotPropertiesChanged, this);
    _plotPropertiesWindow->show();
    _plotPropertiesWindow->raise();
  }
}

void PlotWindow::onPlotPropertiesChanged() { _plotWidget.Update(); }
