#include "timefrequencyplotpage.h"

#include "controllers/tfpagecontroller.h"

TimeFrequencyPlotPage::TimeFrequencyPlotPage(TFPageController* controller)
    : GrayScalePlotPage(controller) {
  grayScaleWidget().OnMouseMovedEvent().connect(
      sigc::mem_fun(*this, &TimeFrequencyPlotPage::onMouseMoved));
}

void TimeFrequencyPlotPage::onMouseMoved(size_t x, size_t y) {
  std::stringstream text;

  const QualityTablesFormatter::StatisticKind kind = getSelectedStatisticKind();
  const std::string& kindName = QualityTablesFormatter::KindToName(kind);

  const MaskedHeatMap& map =
      static_cast<MaskedHeatMap&>(grayScaleWidget().Plot());
  text << kindName << " = " << map.GetImage2D()->Value(x, y) << " (" << x
       << ", " << y << ")";
  _signalStatusChange(text.str());
}
