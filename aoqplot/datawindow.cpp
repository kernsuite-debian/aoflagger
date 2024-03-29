#include "datawindow.h"

#include "../plot/xyplot.h"

#include <sstream>
#include <iomanip>

void DataWindow::SetData(const XYPlot& plot) {
  _plot = &plot;
  int selectedIndex = _comboBox.get_active_row_number();
  if (selectedIndex < 0) selectedIndex = 0;
  _comboListStore->clear();
  for (size_t i = 0; i < plot.PointSetCount(); ++i) {
    std::stringstream str;
    str << (i + 1) << ". " << plot.GetPointSet(i).Label();
    const Gtk::TreeModel::Row row = *_comboListStore->append();
    row[_comboColumnRecord._comboListNameColumn] = str.str();
  }
  if (selectedIndex < (int)plot.PointSetCount())
    _comboBox.set_active(selectedIndex);
  else if (plot.PointSetCount() > 0)
    _comboBox.set_active(0);
  onComboChange();
}

void DataWindow::onComboChange() {
  const int active = _comboBox.get_active_row_number();
  if (active >= 0)
    loadData(active);
  else
    loadData(_plot->PointSetCount());
}

void DataWindow::loadData(size_t plotSetIndex) {
  std::stringstream _dataStream;
  _dataStream << std::setprecision(14);
  if (_plot->PointSetCount() > plotSetIndex) {
    const XYPointSet& pointSet = _plot->GetPointSet(plotSetIndex);
    const size_t valueCount = pointSet.Size();
    for (size_t i = 0; i < valueCount; ++i) {
      const double x = pointSet.GetX(i), y = pointSet.GetY(i);
      if (_plot->XAxis().Type() == AxisType::kText) {
        const std::string& label = _plot->XAxis().TickLabels()[i].second;
        _dataStream << i << '\t' << label << '\t' << y << '\n';
      } else {
        _dataStream << i << '\t' << x << '\t' << y << '\n';
      }
    }
  }
  SetData(_dataStream.str());
}
