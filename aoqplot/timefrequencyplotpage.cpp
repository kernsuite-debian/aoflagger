#include "timefrequencyplotpage.h"

#include "controllers/tfpagecontroller.h"

TimeFrequencyPlotPage::TimeFrequencyPlotPage(TFPageController* controller)
    : GrayScalePlotPage(controller) {
  grayScaleWidget().OnMouseMovedEvent().connect(
      sigc::mem_fun(*this, &TimeFrequencyPlotPage::onMouseMoved));
}

void TimeFrequencyPlotPage::onMouseMoved(double x, double y) {
  std::stringstream text;

  const QualityTablesFormatter::StatisticKind kind = getSelectedStatisticKind();
  const std::string& kindName = QualityTablesFormatter::KindToName(kind);

  const MaskedHeatMap& map =
      static_cast<MaskedHeatMap&>(grayScaleWidget().Plot());
  size_t image_x;
  size_t image_y;
  if (map.UnitToImage(x, y, image_x, image_y)) {
    text << kindName << " = " << map.GetImage2D()->Value(image_x, image_y)
         << " (" << image_x << ", " << image_y << ")";
  }
  _signalStatusChange(text.str());
}
