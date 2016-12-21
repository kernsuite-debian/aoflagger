#include "imagecomparisonwidget.h"

#include "../strategy/algorithms/thresholdconfig.h"
#include "../strategy/algorithms/thresholdtools.h"

ImageComparisonWidget::ImageComparisonWidget() :
	_visualizedImage(TFOriginalImage),
	_showPP(true),
	_showPQ(false),
	_showQP(false),
	_showQQ(true)
{
}

ImageComparisonWidget::~ImageComparisonWidget()
{
}

void ImageComparisonWidget::SetNewData(const TimeFrequencyData &data, TimeFrequencyMetaDataCPtr metaData)
{
	Clear();
	
	_original = data;
	_revised = _original;
	_revised.SetImagesToZero();
	_contaminated = _original;
	updateVisualizedImage();
	
	SetOriginalMask(data.GetSingleMask());
	SetMetaData(metaData);
	ZoomFit();
}

void ImageComparisonWidget::updateVisualizedImage()
{
  Image2DCPtr image;
	const TimeFrequencyData* selectedData;
  switch(_visualizedImage)
	{
	default:
	case TFOriginalImage:
		selectedData = &_original;
		break;
	case TFRevisedImage:
		selectedData = &_revised;
		break;
	case TFContaminatedImage:
		selectedData = &_contaminated;
		break;
	}
	if(!selectedData->IsEmpty())
	{
		if(_showPP && _showQQ)
		{
			if((selectedData->HasPolarisation(Polarization::XX) && selectedData->HasPolarisation(Polarization::YY)) ||
				(selectedData->HasPolarisation(Polarization::RR) && selectedData->HasPolarisation(Polarization::LL)))
			image = selectedData->Make(Polarization::StokesI).GetSingleImage();
		}
		else if(_showQP && _showPQ)
		{
			if(selectedData->HasPolarisation(Polarization::XY) && selectedData->HasPolarisation(Polarization::YX))
				image = selectedData->Make(Polarization::StokesU).GetSingleImage();
			else if(selectedData->HasPolarisation(Polarization::RL) && selectedData->HasPolarisation(Polarization::LR))
				image = selectedData->Make(Polarization::StokesQ).GetSingleImage();
		}
		else if(_showPP)
		{
			if(selectedData->HasPolarisation(Polarization::XX))
				image = selectedData->Make(Polarization::XX).GetSingleImage();
			else if(selectedData->HasPolarisation(Polarization::RR))
				image = selectedData->Make(Polarization::RR).GetSingleImage();
		}
		else if(_showPQ)
		{
			if(selectedData->HasPolarisation(Polarization::XY))
				image = selectedData->Make(Polarization::XY).GetSingleImage();
			else if(selectedData->HasPolarisation(Polarization::RL))
				image = selectedData->Make(Polarization::RL).GetSingleImage();
		}
		else if(_showQP)
		{
			if(selectedData->HasPolarisation(Polarization::YX))
				image = selectedData->Make(Polarization::YX).GetSingleImage();
			else if(selectedData->HasPolarisation(Polarization::LR))
				image = selectedData->Make(Polarization::LR).GetSingleImage();
		}
		else if(_showQQ)
		{
			if(selectedData->HasPolarisation(Polarization::YY))
				image = selectedData->Make(Polarization::YY).GetSingleImage();
			else if(selectedData->HasPolarisation(Polarization::LL))
				image = selectedData->Make(Polarization::LL).GetSingleImage();
		}
	}
	if(image == 0)
		Clear();
	else
		ImageWidget::SetImage(image);
} 

void ImageComparisonWidget::ClearBackground()
{
	_revised.SetImagesToZero();
}
