#include <iostream>
#include <sstream>

#include <gtkmm/filechooserdialog.h>
#include "../util/gtkmm-compat.h"

#include "../plot/plotwidget.h"

#include "imagepropertieswindow.h"
#include "maskedheatmap.h"

ImagePropertiesWindow::ImagePropertiesWindow(PlotWidget& imageWidget,
                                             const std::string& title)
    : Gtk::Window(),
      _plotWidget(imageWidget),
      _applyButton("_Apply", true),
      _exportButton("_Export", true),
      _exportDataButton("Export _data", true),
      _closeButton("_Close", true),

      _colorMapFrame("Color map"),

      _scaleFrame("Scale"),
      _minMaxScaleButton("From min to max"),
      _winsorizedScaleButton("Winsorized min and max"),
      _specifiedScaleButton("Specified:"),
      _scaleMinLabel("Scale minimum:"),
      _scaleMaxLabel("Scale maximum:"),
      _scaleMinEntry(),
      _scaleMaxEntry(),

      _optionsFrame("Options"),
      _normalOptionsButton("Normal scale"),
      _logScaleButton("Logarithmic scale"),

      _filterFrame("Interpolation"),
      _bestFilterButton("Best"),
      _nearestFilterButton("Nearest"),

      _axesFrame("Title & axes"),
      _showXYAxes("Show XY axes"),
      _showColorScale("Show color scale"),
      _showTitleButton("Show title"),
      _showXAxisDescriptionButton("x-axis desc"),
      _showYAxisDescriptionButton("y-axis desc"),
      _showZAxisDescriptionButton("z-axis desc"),
      _manualXAxisDescription("manual"),
      _manualYAxisDescription("manual"),
      _manualZAxisDescription("manual") {
  _maskedHeatMap = dynamic_cast<MaskedHeatMap*>(&_plotWidget.Plot());
  if (!_maskedHeatMap) {
    throw std::invalid_argument(
        "ImagePropertiesWindow called for a plot that is not a heat map");
  }

  set_title(title);

  initColorMapButtons();
  initScaleWidgets();
  initOptionsWidgets();
  initFilterWidgets();
  _framesHBox.pack_start(_filterAndOptionsBox);
  initAxisWidgets();

  gtkmm_set_image_from_icon_name(_applyButton, "gtk-apply");
  _applyButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onApplyClicked));
  _bottomButtonBox.pack_start(_applyButton);

  gtkmm_set_image_from_icon_name(_exportButton, "document-save-as");
  _exportButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onExportClicked));
  _bottomButtonBox.pack_start(_exportButton);

  _exportDataButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onExportDataClicked));
  _bottomButtonBox.pack_start(_exportDataButton);

  gtkmm_set_image_from_icon_name(_closeButton, "window-close");
  _closeButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onCloseClicked));
  _bottomButtonBox.pack_start(_closeButton);

  _topVBox.pack_start(_framesHBox);

  _topVBox.pack_start(_bottomButtonBox);

  add(_topVBox);
  _topVBox.show_all();
}

void ImagePropertiesWindow::initColorMapButtons() {
  _colorMapStore = Gtk::ListStore::create(_colorMapColumns);

  addColorMap("Grayscale", ColorMap::Grayscale);
  addColorMap("Inverted grayscale", ColorMap::Inverted);
  addColorMap("Hot/cold", ColorMap::HotCold);
  addColorMap("Red/blue", ColorMap::RedBlue);
  addColorMap("Black/red", ColorMap::BlackRed);
  addColorMap("Red/Yellow/Blue", ColorMap::RedYellowBlue);
  addColorMap("Fire", ColorMap::Fire);
  addColorMap("Cool", ColorMap::Cool);
  addColorMap("Cubehelix", ColorMap::CubeHelix);
  addColorMap("Cubehelix+", ColorMap::CubeHelixColourful);
  addColorMap("Viridis", ColorMap::Viridis);
  addColorMap("Rainbow", ColorMap::Rainbow);

  _colorMapCombo.set_model(_colorMapStore);
  _colorMapCombo.pack_start(_colorMapColumns.name);

  const ColorMap::Type selectedMap = _maskedHeatMap->GetColorMap();
  const Gtk::ListStore::Children children = _colorMapStore->children();
  for (auto row : children) {
    if (row[_colorMapColumns.colorMap] == selectedMap) {
      _colorMapCombo.set_active(row);
      break;
    }
  }

  _colorMapBox.pack_start(_colorMapCombo, false, false);

  _colorMapFrame.add(_colorMapBox);

  _framesHBox.pack_start(_colorMapFrame);
}

void ImagePropertiesWindow::initScaleWidgets() {
  _scaleFrame.add(_scaleBox);

  Gtk::RadioButton::Group group;

  _scaleBox.pack_start(_minMaxScaleButton);
  _minMaxScaleButton.set_group(group);
  _minMaxScaleButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));

  _scaleBox.pack_start(_winsorizedScaleButton);
  _winsorizedScaleButton.set_group(group);
  _winsorizedScaleButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));
  _scaleBox.pack_start(_specifiedScaleButton);

  _specifiedScaleButton.set_group(group);
  _specifiedScaleButton.signal_clicked().connect(
      sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));

  switch (_maskedHeatMap->ZRange().minimum) {
    default:
    case RangeLimit::Extreme:
      _minMaxScaleButton.set_active(true);
      break;
    case RangeLimit::Winsorized:
      _winsorizedScaleButton.set_active(true);
      break;
    case RangeLimit::Specified:
      _specifiedScaleButton.set_active(true);
      break;
  }
  onScaleChanged();

  updateMinMaxEntries();

  _scaleBox.pack_start(_scaleMinLabel);
  _scaleBox.pack_start(_scaleMinEntry);

  _scaleBox.pack_start(_scaleMaxLabel);
  _scaleBox.pack_start(_scaleMaxEntry);

  _framesHBox.pack_start(_scaleFrame);
}

void ImagePropertiesWindow::initOptionsWidgets() {
  Gtk::RadioButton::Group group;

  _optionsBox.pack_start(_normalOptionsButton);
  _normalOptionsButton.set_group(group);

  _optionsBox.pack_start(_logScaleButton);
  _logScaleButton.set_group(group);

  if (_maskedHeatMap->LogZScale()) {
    _logScaleButton.set_active(true);
  } else {
    _normalOptionsButton.set_active(true);
  }

  _optionsFrame.add(_optionsBox);

  _filterAndOptionsBox.pack_start(_optionsFrame);
}

void ImagePropertiesWindow::initFilterWidgets() {
  Gtk::RadioButton::Group group;

  _filterBox.pack_start(_bestFilterButton);
  _bestFilterButton.set_group(group);

  _filterBox.pack_start(_nearestFilterButton);
  _nearestFilterButton.set_group(group);

  switch (_maskedHeatMap->CairoFilter()) {
    default:
    case Cairo::FILTER_BEST:
      _bestFilterButton.set_active(true);
      break;
    case Cairo::FILTER_NEAREST:
      _nearestFilterButton.set_active(true);
      break;
  }

  _filterFrame.add(_filterBox);

  _filterAndOptionsBox.pack_start(_filterFrame);
}

void ImagePropertiesWindow::initAxisWidgets() {
  const MaskedHeatMap& maskedHeatMap =
      static_cast<const MaskedHeatMap&>(_plotWidget.Plot());
  _showXYAxes.set_active(maskedHeatMap.ShowXAxis() ||
                         maskedHeatMap.ShowYAxis());
  _axesGeneralBox.pack_start(_showXYAxes);

  _showColorScale.set_active(maskedHeatMap.ShowColorScale());
  _axesGeneralBox.pack_start(_showColorScale);

  _axesHBox.pack_start(_axesGeneralBox);

  _showTitleButton.set_active(maskedHeatMap.ShowTitle());
  _titleBox.pack_start(_showTitleButton);
  _titleEntry.set_text(maskedHeatMap.TitleText());
  _titleBox.pack_start(_titleEntry);

  _axesVisibilityBox.pack_start(_titleBox);

  _showXAxisDescriptionButton.set_active(maskedHeatMap.ShowXAxisDescription());
  _xAxisBox.pack_start(_showXAxisDescriptionButton);

  _manualXAxisDescription.set_active(maskedHeatMap.ManualXAxisDescription());
  _xAxisBox.pack_start(_manualXAxisDescription);

  _xAxisDescriptionEntry.set_text(maskedHeatMap.XAxisDescription());
  _xAxisBox.pack_start(_xAxisDescriptionEntry);

  _axesVisibilityBox.pack_start(_xAxisBox);

  _showYAxisDescriptionButton.set_active(maskedHeatMap.ShowYAxisDescription());
  _yAxisBox.pack_start(_showYAxisDescriptionButton);

  _manualYAxisDescription.set_active(maskedHeatMap.ManualYAxisDescription());
  _yAxisBox.pack_start(_manualYAxisDescription);

  _yAxisDescriptionEntry.set_text(maskedHeatMap.YAxisDescription());
  _yAxisBox.pack_start(_yAxisDescriptionEntry);

  _axesVisibilityBox.pack_start(_yAxisBox);

  _showZAxisDescriptionButton.set_active(maskedHeatMap.ShowZAxisDescription());
  _zAxisBox.pack_start(_showZAxisDescriptionButton);

  _manualZAxisDescription.set_active(maskedHeatMap.ManualZAxisDescription());
  _zAxisBox.pack_start(_manualZAxisDescription);

  _zAxisDescriptionEntry.set_text(maskedHeatMap.ZAxisDescription());
  _zAxisBox.pack_start(_zAxisDescriptionEntry);

  _axesVisibilityBox.pack_start(_zAxisBox);

  _axesHBox.pack_start(_axesVisibilityBox);

  _axesFrame.add(_axesHBox);
  _topVBox.pack_start(_axesFrame);
}

void ImagePropertiesWindow::updateMinMaxEntries() {
  std::stringstream minStr;
  minStr << _maskedHeatMap->Min();
  _scaleMinEntry.set_text(minStr.str());

  std::stringstream maxStr;
  maxStr << _maskedHeatMap->Max();
  _scaleMaxEntry.set_text(maxStr.str());
}

void ImagePropertiesWindow::onApplyClicked() {
  MaskedHeatMap& maskedHeatMap =
      static_cast<MaskedHeatMap&>(_plotWidget.Plot());
  maskedHeatMap.SetColorMap(
      (*_colorMapCombo.get_active())[_colorMapColumns.colorMap]);

  if (_minMaxScaleButton.get_active()) {
    maskedHeatMap.SetZRange(FullRange());
  } else if (_winsorizedScaleButton.get_active()) {
    maskedHeatMap.SetZRange(WinsorizedRange());
  } else if (_specifiedScaleButton.get_active()) {
    RangeConfiguration range;
    range.minimum = RangeLimit::Specified;
    range.maximum = RangeLimit::Specified;
    range.specified_min = atof(_scaleMinEntry.get_text().c_str());
    range.specified_max = atof(_scaleMaxEntry.get_text().c_str());
    maskedHeatMap.SetZRange(range);
  }

  maskedHeatMap.SetLogZScale(_logScaleButton.get_active());

  if (_bestFilterButton.get_active())
    maskedHeatMap.SetCairoFilter(Cairo::FILTER_BEST);
  else if (_nearestFilterButton.get_active())
    maskedHeatMap.SetCairoFilter(Cairo::FILTER_NEAREST);

  maskedHeatMap.SetShowTitle(_showTitleButton.get_active());
  maskedHeatMap.SetTitleText(_titleEntry.get_text());
  maskedHeatMap.SetShowXAxis(_showXYAxes.get_active());
  maskedHeatMap.SetShowYAxis(_showXYAxes.get_active());
  maskedHeatMap.SetShowXAxisDescription(
      _showXAxisDescriptionButton.get_active());
  maskedHeatMap.SetShowYAxisDescription(
      _showYAxisDescriptionButton.get_active());
  maskedHeatMap.SetShowZAxisDescription(
      _showZAxisDescriptionButton.get_active());

  maskedHeatMap.SetManualXAxisDescription(_manualXAxisDescription.get_active());
  if (_manualXAxisDescription.get_active())
    maskedHeatMap.SetXAxisDescription(_xAxisDescriptionEntry.get_text());

  maskedHeatMap.SetManualYAxisDescription(_manualYAxisDescription.get_active());
  if (_manualYAxisDescription.get_active())
    maskedHeatMap.SetYAxisDescription(_yAxisDescriptionEntry.get_text());

  maskedHeatMap.SetManualZAxisDescription(_manualZAxisDescription.get_active());
  if (_manualZAxisDescription.get_active())
    maskedHeatMap.SetZAxisDescription(_zAxisDescriptionEntry.get_text());

  maskedHeatMap.SetShowColorScale(_showColorScale.get_active());

  _plotWidget.Update();

  updateMinMaxEntries();
}

void ImagePropertiesWindow::onCloseClicked() { hide(); }

void ImagePropertiesWindow::onExportClicked() {
  if (_maskedHeatMap->HasImage()) {
    Gtk::FileChooserDialog dialog("Specify image filename",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    // Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);

    const Glib::RefPtr<Gtk::FileFilter> pdfFilter = Gtk::FileFilter::create();
    const std::string pdfName = "Portable Document Format (*.pdf)";
    pdfFilter->set_name(pdfName);
    pdfFilter->add_pattern("*.pdf");
    pdfFilter->add_mime_type("application/pdf");
    dialog.add_filter(pdfFilter);

    const Glib::RefPtr<Gtk::FileFilter> svgFilter = Gtk::FileFilter::create();
    const std::string svgName = "Scalable Vector Graphics (*.svg)";
    svgFilter->set_name(svgName);
    svgFilter->add_pattern("*.svg");
    svgFilter->add_mime_type("image/svg+xml");
    dialog.add_filter(svgFilter);

    const Glib::RefPtr<Gtk::FileFilter> pngFilter = Gtk::FileFilter::create();
    const std::string pngName = "Portable Network Graphics (*.png)";
    pngFilter->set_name(pngName);
    pngFilter->add_pattern("*.png");
    pngFilter->add_mime_type("image/png");
    dialog.add_filter(pngFilter);

    const int result = dialog.run();

    if (result == Gtk::RESPONSE_OK) {
      const Glib::RefPtr<const Gtk::FileFilter> filter = dialog.get_filter();
      if (filter->get_name() == pdfName)
        _plotWidget.SavePdf(dialog.get_filename());
      else if (filter->get_name() == svgName)
        _plotWidget.SaveSvg(dialog.get_filename());
      else
        _plotWidget.SavePng(dialog.get_filename());
    }
  }
}

void ImagePropertiesWindow::onExportDataClicked() {
  if (_maskedHeatMap->HasImage()) {
    Gtk::FileChooserDialog dialog("Specify data filename",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    // Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);

    const Glib::RefPtr<Gtk::FileFilter> pdfFilter = Gtk::FileFilter::create();
    const std::string pdfName =
        "Text format: width; height; data1; data2... (*.txt)";
    pdfFilter->set_name(pdfName);
    pdfFilter->add_pattern("*.txt");
    pdfFilter->add_mime_type("text/plain");
    dialog.add_filter(pdfFilter);
    const int result = dialog.run();

    if (result == Gtk::RESPONSE_OK) {
      const MaskedHeatMap& maskedHeatMap =
          static_cast<MaskedHeatMap&>(_plotWidget.Plot());
      maskedHeatMap.SaveText(dialog.get_filename());
    }
  }
}
