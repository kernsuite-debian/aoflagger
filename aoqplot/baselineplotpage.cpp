#include <limits>

#include "baselineplotpage.h"

#include "controllers/baselinepagecontroller.h"

BaselinePlotPage::BaselinePlotPage(BaselinePageController* controller)
    : GrayScalePlotPage(controller), _controller(controller) {
  grayScaleWidget().OnMouseMovedEvent().connect(
      sigc::mem_fun(*this, &BaselinePlotPage::onMouseMoved));
}

BaselinePlotPage::~BaselinePlotPage() {}

void BaselinePlotPage::onMouseMoved(double x, double y) {
  const MaskedHeatMap& map =
      static_cast<MaskedHeatMap&>(grayScaleWidget().Plot());
  size_t image_x;
  size_t image_y;
  if (map.UnitToImage(x, y, image_x, image_y)) {
    const std::string antenna1Name = _controller->AntennaName(x);
    const std::string antenna2Name = _controller->AntennaName(y);
    const QualityTablesFormatter::StatisticKind kind =
        getSelectedStatisticKind();
    const std::string& kindName = QualityTablesFormatter::KindToName(kind);

    std::stringstream text;
    const size_t stride =
        static_cast<HeatMap&>(grayScaleWidget().Plot()).Image().Stride();
    text << "Correlation " << antenna1Name << " (" << image_x << ") x "
         << antenna2Name << " (" << image_y << "), " << kindName << " = "
         << static_cast<HeatMap&>(grayScaleWidget().Plot())
                .Image()
                .Data()[image_y * stride + image_x];
    _signalStatusChange(text.str());
  }
}
