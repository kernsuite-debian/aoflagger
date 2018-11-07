#include <iostream>
#include <sstream>

#include <gtkmm/filechooserdialog.h>
#include "../gtkmm-compat.h"

#include "imagepropertieswindow.h"
#include "heatmapwidget.h"

ImagePropertiesWindow::ImagePropertiesWindow(HeatMapWidget &imageWidget, const std::string &title) :
	Gtk::Window(),
	_imageWidget(imageWidget),
	_applyButton("_Apply", true),
	_exportButton("_Export", true),
	_exportDataButton("Export _data", true),
	_closeButton("_Close", true),
	
	_colorMapFrame("Color map"),
	_grayScaleButton("Grayscale"),
	_invGrayScaleButton("Inverted grayscale"),
	_hotColdScaleButton("Hot/cold"),
	_redBlueScaleButton("Red/blue"),
	_blackRedScaleButton("Black/red"),
	_redBlueYellowScaleButton("Red/Yellow/Blue"),
	_fireScaleButton("Fire"),
	_cubeHelixScaleButton("Cubehelix"),
	_cubeHelixColourfulScaleButton("Cubehelix+"),
	_viridisScaleButton("Viridis"),
	
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
	_zeroSymmetricButton("Symmetric around zero"),
	
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
	_manualTitle("manual"),
	_manualXAxisDescription("manual"),
	_manualYAxisDescription("manual"),
	_manualZAxisDescription("manual")
{
	set_title(title);

	initColorMapButtons();
	initScaleWidgets();
	initOptionsWidgets();
	initFilterWidgets();
	_framesHBox.pack_start(_filterAndOptionsBox);
	initAxisWidgets();
	
	gtkmm_set_image_from_icon_name(_applyButton, "gtk-apply");
	_applyButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onApplyClicked));
	_bottomButtonBox.pack_start(_applyButton);

	gtkmm_set_image_from_icon_name(_exportButton, "document-save-as");
	_exportButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onExportClicked));
	_bottomButtonBox.pack_start(_exportButton);

	_exportDataButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onExportDataClicked));
	_bottomButtonBox.pack_start(_exportDataButton);

	gtkmm_set_image_from_icon_name(_closeButton, "window-close");
	_closeButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onCloseClicked));
	_bottomButtonBox.pack_start(_closeButton);

	_topVBox.pack_start(_framesHBox);
	
	_topVBox.pack_start(_bottomButtonBox);

	add(_topVBox);
	_topVBox.show_all();
}

void ImagePropertiesWindow::initColorMapButtons()
{
	Gtk::RadioButton::Group group;

	_grayScaleButton.set_group(group);
	_colorMapBox.pack_start(_grayScaleButton);
	
	_invGrayScaleButton.set_group(group);
	_colorMapBox.pack_start(_invGrayScaleButton);
	
	_hotColdScaleButton.set_group(group);
	_colorMapBox.pack_start(_hotColdScaleButton);
	
	_redBlueScaleButton.set_group(group);
	_colorMapBox.pack_start(_redBlueScaleButton);
	
	_blackRedScaleButton.set_group(group);
	_colorMapBox.pack_start(_blackRedScaleButton);
	
	_redBlueYellowScaleButton.set_group(group);
	_colorMapBox.pack_start(_redBlueYellowScaleButton);
	
	_fireScaleButton.set_group(group);
	_colorMapBox.pack_start(_fireScaleButton);
	
	_cubeHelixScaleButton.set_group(group);
	_colorMapBox.pack_start(_cubeHelixScaleButton);
	
	_cubeHelixColourfulScaleButton.set_group(group);
	_colorMapBox.pack_start(_cubeHelixColourfulScaleButton);
	
	_viridisScaleButton.set_group(group);
	_colorMapBox.pack_start(_viridisScaleButton);
	
	switch(_imageWidget.Plot().GetColorMap())
	{
		default:
		case HeatMapPlot::BWMap: _grayScaleButton.set_active(true); break;
		case HeatMapPlot::InvertedMap: _invGrayScaleButton.set_active(true); break;
		case HeatMapPlot::HotColdMap: _hotColdScaleButton.set_active(true); break;
		case HeatMapPlot::RedBlueMap: _redBlueScaleButton.set_active(true); break;
		case HeatMapPlot::BlackRedMap: _blackRedScaleButton.set_active(true); break;
		case HeatMapPlot::RedYellowBlueMap: _redBlueYellowScaleButton.set_active(true); break;
		case HeatMapPlot::FireMap: _fireScaleButton.set_active(true); break;
		case HeatMapPlot::CubeHelixMap: _cubeHelixScaleButton.set_active(true); break;
		case HeatMapPlot::CubeHelixColourfulMap: _cubeHelixColourfulScaleButton.set_active(true); break;
		case HeatMapPlot::ViridisMap: _viridisScaleButton.set_active(true); break;
	}

	_colorMapFrame.add(_colorMapBox);

	_framesHBox.pack_start(_colorMapFrame);
}

void ImagePropertiesWindow::initScaleWidgets()
{
	_scaleFrame.add(_scaleBox);
	
	Gtk::RadioButton::Group group;
	
	_scaleBox.pack_start(_minMaxScaleButton);
	_minMaxScaleButton.set_group(group);
	_minMaxScaleButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));

	_scaleBox.pack_start(_winsorizedScaleButton);
	_winsorizedScaleButton.set_group(group);
	_winsorizedScaleButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));
	_scaleBox.pack_start(_specifiedScaleButton);
	
	_specifiedScaleButton.set_group(group);
	_specifiedScaleButton.signal_clicked().connect(sigc::mem_fun(*this, &ImagePropertiesWindow::onScaleChanged));
	
	switch(_imageWidget.Plot().Range())
	{
		default:
		case HeatMapPlot::MinMax: _minMaxScaleButton.set_active(true); break;
		case HeatMapPlot::Winsorized: _winsorizedScaleButton.set_active(true); break;
		case HeatMapPlot::Specified: _specifiedScaleButton.set_active(true); break;
	}
	onScaleChanged();

	updateMinMaxEntries();

	_scaleBox.pack_start(_scaleMinLabel);
	_scaleBox.pack_start(_scaleMinEntry);
	
	_scaleBox.pack_start(_scaleMaxLabel);
	_scaleBox.pack_start(_scaleMaxEntry);
	
	_framesHBox.pack_start(_scaleFrame);
}

void ImagePropertiesWindow::initOptionsWidgets()
{
	Gtk::RadioButton::Group group;
	
	_optionsBox.pack_start(_normalOptionsButton);
	_normalOptionsButton.set_group(group);

	_optionsBox.pack_start(_logScaleButton);
	_logScaleButton.set_group(group);

	_optionsBox.pack_start(_zeroSymmetricButton);
	_zeroSymmetricButton.set_group(group);
	
	switch(_imageWidget.Plot().ScaleOption())
	{
		default:
		case HeatMapPlot::NormalScale: _normalOptionsButton.set_active(true); break;
		case HeatMapPlot::LogScale: _logScaleButton.set_active(true); break;
		case HeatMapPlot::ZeroSymmetricScale: _zeroSymmetricButton.set_active(true); break;
	}

	_optionsFrame.add(_optionsBox);
	
	_filterAndOptionsBox.pack_start(_optionsFrame);
}

void ImagePropertiesWindow::initFilterWidgets()
{
	Gtk::RadioButton::Group group;
	
	_filterBox.pack_start(_bestFilterButton);
	_bestFilterButton.set_group(group);

	_filterBox.pack_start(_nearestFilterButton);
	_nearestFilterButton.set_group(group);

	switch(_imageWidget.Plot().CairoFilter())
	{
		default:
		case Cairo::FILTER_BEST:    _bestFilterButton.set_active(true); break;
		case Cairo::FILTER_NEAREST: _nearestFilterButton.set_active(true); break;
	}

	_filterFrame.add(_filterBox);
	
	_filterAndOptionsBox.pack_start(_filterFrame);
}

void ImagePropertiesWindow::initAxisWidgets()
{
	_showXYAxes.set_active(_imageWidget.Plot().ShowXYAxes());
	_axesGeneralBox.pack_start(_showXYAxes);

	_showColorScale.set_active(_imageWidget.Plot().ShowColorScale());
	_axesGeneralBox.pack_start(_showColorScale);
	
	_axesHBox.pack_start(_axesGeneralBox);
	
	_showTitleButton.set_active(_imageWidget.Plot().ShowTitle());
	_titleBox.pack_start(_showTitleButton);
	_manualTitle.set_active(_imageWidget.Plot().ManualTitle());
	_titleBox.pack_start(_manualTitle);
	_titleEntry.set_text(_imageWidget.Plot().ManualTitleText());
	_titleBox.pack_start(_titleEntry);
	
	_axesVisibilityBox.pack_start(_titleBox);
	
	_showXAxisDescriptionButton.set_active(_imageWidget.Plot().ShowXAxisDescription());
	_xAxisBox.pack_start(_showXAxisDescriptionButton);
	
	_manualXAxisDescription.set_active(_imageWidget.Plot().ManualXAxisDescription());
	_xAxisBox.pack_start(_manualXAxisDescription);
	
	_xAxisDescriptionEntry.set_text(_imageWidget.Plot().XAxisDescription());
	_xAxisBox.pack_start(_xAxisDescriptionEntry);
	
	_axesVisibilityBox.pack_start(_xAxisBox);
	
	_showYAxisDescriptionButton.set_active(_imageWidget.Plot().ShowYAxisDescription());
	_yAxisBox.pack_start(_showYAxisDescriptionButton);
	
	_manualYAxisDescription.set_active(_imageWidget.Plot().ManualYAxisDescription());
	_yAxisBox.pack_start(_manualYAxisDescription);
	
	_yAxisDescriptionEntry.set_text(_imageWidget.Plot().YAxisDescription());
	_yAxisBox.pack_start(_yAxisDescriptionEntry);
	
	_axesVisibilityBox.pack_start(_yAxisBox);
	
	_showZAxisDescriptionButton.set_active(_imageWidget.Plot().ShowZAxisDescription());
	_zAxisBox.pack_start(_showZAxisDescriptionButton);
	
	_manualZAxisDescription.set_active(_imageWidget.Plot().ManualZAxisDescription());
	_zAxisBox.pack_start(_manualZAxisDescription);
	
	_zAxisDescriptionEntry.set_text(_imageWidget.Plot().ZAxisDescription());
	_zAxisBox.pack_start(_zAxisDescriptionEntry);
	
	_axesVisibilityBox.pack_start(_zAxisBox);
	
	_axesHBox.pack_start(_axesVisibilityBox);
	
	_axesFrame.add(_axesHBox);
	_topVBox.pack_start(_axesFrame);
}

void ImagePropertiesWindow::updateMinMaxEntries()
{
	std::stringstream minStr;
	minStr << _imageWidget.Plot().Min();
	_scaleMinEntry.set_text(minStr.str());
	
	std::stringstream maxStr;
	maxStr << _imageWidget.Plot().Max();
	_scaleMaxEntry.set_text(maxStr.str());
}

void ImagePropertiesWindow::onApplyClicked()
{
	if(_grayScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::BWMap);
	else if(_invGrayScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::InvertedMap);
	else if(_hotColdScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::HotColdMap);
	else if(_redBlueScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::RedBlueMap);
	else if(_blackRedScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::BlackRedMap);
	else if(_redBlueYellowScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::RedYellowBlueMap);
	else if(_fireScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::FireMap);
	else if(_cubeHelixScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::CubeHelixMap);
	else if(_cubeHelixColourfulScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::CubeHelixColourfulMap);
	else if(_viridisScaleButton.get_active())
		_imageWidget.Plot().SetColorMap(HeatMapPlot::ViridisMap);
	
	if(_minMaxScaleButton.get_active())
		_imageWidget.Plot().SetRange(HeatMapPlot::MinMax);
	else if(_winsorizedScaleButton.get_active())
		_imageWidget.Plot().SetRange(HeatMapPlot::Winsorized);
	else if(_specifiedScaleButton.get_active())
	{
		_imageWidget.Plot().SetRange(HeatMapPlot::Specified);
		_imageWidget.Plot().SetMin(atof(_scaleMinEntry.get_text().c_str()));
		_imageWidget.Plot().SetMax(atof(_scaleMaxEntry.get_text().c_str()));
	}
	
	if(_normalOptionsButton.get_active())
		_imageWidget.Plot().SetScaleOption(HeatMapPlot::NormalScale);
	else if(_logScaleButton.get_active())
		_imageWidget.Plot().SetScaleOption(HeatMapPlot::LogScale);
	else if(_zeroSymmetricButton.get_active())
		_imageWidget.Plot().SetScaleOption(HeatMapPlot::ZeroSymmetricScale);
	
	if(_bestFilterButton.get_active())
		_imageWidget.Plot().SetCairoFilter(Cairo::FILTER_BEST);
	else if(_nearestFilterButton.get_active())
		_imageWidget.Plot().SetCairoFilter(Cairo::FILTER_NEAREST);
	
	_imageWidget.Plot().SetShowTitle(_showTitleButton.get_active());
	_imageWidget.Plot().SetManualTitle(_manualTitle.get_active());
	if(_manualTitle.get_active())
		_imageWidget.Plot().SetManualTitleText(_titleEntry.get_text());
	
	_imageWidget.Plot().SetShowXYAxes(_showXYAxes.get_active());
	_imageWidget.Plot().SetShowXAxisDescription(_showXAxisDescriptionButton.get_active());
	_imageWidget.Plot().SetShowYAxisDescription(_showYAxisDescriptionButton.get_active());
	_imageWidget.Plot().SetShowZAxisDescription(_showZAxisDescriptionButton.get_active());
	
	_imageWidget.Plot().SetManualXAxisDescription(_manualXAxisDescription.get_active());
	if(_manualXAxisDescription.get_active())
		_imageWidget.Plot().SetXAxisDescription(_xAxisDescriptionEntry.get_text());
	
	_imageWidget.Plot().SetManualYAxisDescription(_manualYAxisDescription.get_active());
	if(_manualYAxisDescription.get_active())
		_imageWidget.Plot().SetYAxisDescription(_yAxisDescriptionEntry.get_text());
	
	_imageWidget.Plot().SetManualZAxisDescription(_manualZAxisDescription.get_active());
	if(_manualZAxisDescription.get_active())
		_imageWidget.Plot().SetZAxisDescription(_zAxisDescriptionEntry.get_text());
	
	_imageWidget.Plot().SetShowColorScale(_showColorScale.get_active());
	
	_imageWidget.Update();
	
	updateMinMaxEntries();
}

void ImagePropertiesWindow::onCloseClicked()
{
	hide();
}

void ImagePropertiesWindow::onExportClicked()
{
	if(_imageWidget.Plot().HasImage())
	{
		Gtk::FileChooserDialog dialog("Specify image filename", Gtk::FILE_CHOOSER_ACTION_SAVE);
		dialog.set_transient_for(*this);

		//Add response buttons the the dialog:
		dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
		dialog.add_button("_Save", Gtk::RESPONSE_OK);

		Glib::RefPtr<Gtk::FileFilter> pdfFilter = Gtk::FileFilter::create();
		std::string pdfName = "Portable Document Format (*.pdf)";
		pdfFilter->set_name(pdfName);
		pdfFilter->add_pattern("*.pdf");
		pdfFilter->add_mime_type("application/pdf");
		dialog.add_filter(pdfFilter);

		Glib::RefPtr<Gtk::FileFilter> svgFilter = Gtk::FileFilter::create();
		std::string svgName = "Scalable Vector Graphics (*.svg)";
		svgFilter->set_name(svgName);
		svgFilter->add_pattern("*.svg");
		svgFilter->add_mime_type("image/svg+xml");
		dialog.add_filter(svgFilter);

		Glib::RefPtr<Gtk::FileFilter> pngFilter = Gtk::FileFilter::create();
		std::string pngName = "Portable Network Graphics (*.png)";
		pngFilter->set_name(pngName);
		pngFilter->add_pattern("*.png");
		pngFilter->add_mime_type("image/png");
		dialog.add_filter(pngFilter);

		int result = dialog.run();

		if(result == Gtk::RESPONSE_OK)
		{
			Glib::RefPtr<const Gtk::FileFilter> filter = dialog.get_filter();
			if(filter->get_name() == pdfName)
				_imageWidget.SavePdf(dialog.get_filename());
			else if(filter->get_name() == svgName)
				_imageWidget.SaveSvg(dialog.get_filename());
			else
				_imageWidget.SavePng(dialog.get_filename());
		}
	}
}

void ImagePropertiesWindow::onExportDataClicked()
{
	if(_imageWidget.Plot().HasImage())
	{
		Gtk::FileChooserDialog dialog("Specify data filename", Gtk::FILE_CHOOSER_ACTION_SAVE);
		dialog.set_transient_for(*this);

		//Add response buttons the the dialog:
		dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
		dialog.add_button("_Save", Gtk::RESPONSE_OK);

		Glib::RefPtr<Gtk::FileFilter> pdfFilter = Gtk::FileFilter::create();
		std::string pdfName = "Text format: width; height; data1; data2... (*.txt)";
		pdfFilter->set_name(pdfName);
		pdfFilter->add_pattern("*.txt");
		pdfFilter->add_mime_type("text/plain");
		dialog.add_filter(pdfFilter);
		int result = dialog.run();

		if(result == Gtk::RESPONSE_OK)
		{
			_imageWidget.Plot().SaveText(dialog.get_filename());
		}
	}
}

