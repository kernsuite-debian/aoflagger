#include "imagecomparisoncontroller.h"

ImageComparisonController::ImageComparisonController() :
	_showPP(true),
	_showPQ(false),
	_showQP(false),
	_showQQ(true),
	_visualizedImage(0)
{
	_dataList.emplace_back("No data", TimeFrequencyData());
}

TimeFrequencyData ImageComparisonController::GetActiveData() const
{
	TimeFrequencyData data(_dataList[_visualizedImage].data);
	getActiveMask(data);
	if(_plot.StartHorizontal() != 0.0 ||
		_plot.EndHorizontal() != 1.0 ||
		_plot.StartVertical() != 0.0 ||
		_plot.EndVertical() != 1.0)
	{
		data.Trim(round(_plot.StartHorizontal() * data.ImageWidth()),
							round(_plot.StartVertical() * data.ImageHeight()),
							round(_plot.EndHorizontal() * data.ImageWidth()),
							round(_plot.EndVertical() * data.ImageHeight()));
	}
	return data;
}

void ImageComparisonController::SetNewData(const TimeFrequencyData &data, TimeFrequencyMetaDataCPtr metaData)
{
	_plot.Clear();
	
	_dataList.clear();
	_dataList.emplace_back("Original data", data);
	_visualizedImage = 0;
	updateVisualizedImage();
	
	_plot.SetOriginalMask(data.GetSingleMask());
	_plot.SetMetaData(metaData);
	_plot.ZoomFit();
	
	_visualizationListChange.emit();
}

void ImageComparisonController::getFirstAvailablePolarization(bool& pp, bool& pq, bool& qp, bool& qq) const
{
	const TimeFrequencyData& selectedData = _dataList[_visualizedImage].data;
	if(selectedData.IsEmpty()) {
		pp = true; pq = false; qp = false; qq = true;
	}
	else {
		bool
			hasXX = selectedData.HasPolarization(Polarization::XX),
			hasXY = selectedData.HasPolarization(Polarization::XY),
			hasYX = selectedData.HasPolarization(Polarization::YX),
			hasYY = selectedData.HasPolarization(Polarization::YY),
			hasRR = selectedData.HasPolarization(Polarization::RR),
			hasRL = selectedData.HasPolarization(Polarization::RL),
			hasLR = selectedData.HasPolarization(Polarization::LR),
			hasLL = selectedData.HasPolarization(Polarization::LL),
			hasI = selectedData.HasPolarization(Polarization::StokesI),
			hasQ = selectedData.HasPolarization(Polarization::StokesQ),
			hasU = selectedData.HasPolarization(Polarization::StokesU),
			hasV = selectedData.HasPolarization(Polarization::StokesV);
		if(hasXX || hasRR || hasI) {
			pp = true; pq = false; qp = false; qq = false;
		}
		else if(hasYY || hasLL || hasV) {
			pp = false; pq = false; qp = false; qq = true;
		}
		else if(hasXY || hasRL || hasQ) {
			pp = false; pq = true; qp = false; qq = false;
		}
		else if(hasYX || hasLR || hasU) {
			pp = false; pq = false; qp = true; qq = false;
		}
		else {
			pp = true; pq = false; qp = false; qq = true;
		}
	}
}

void ImageComparisonController::TryVisualizePolarizations(bool& pp, bool& pq, bool& qp, bool& qq) const
{
	const TimeFrequencyData& selectedData = _dataList[_visualizedImage].data;
	if(!selectedData.IsEmpty())
	{
		bool
			hasXX = selectedData.HasPolarization(Polarization::XX),
			hasXY = selectedData.HasPolarization(Polarization::XY),
			hasYX = selectedData.HasPolarization(Polarization::YX),
			hasYY = selectedData.HasPolarization(Polarization::YY),
			hasRR = selectedData.HasPolarization(Polarization::RR),
			hasRL = selectedData.HasPolarization(Polarization::RL),
			hasLR = selectedData.HasPolarization(Polarization::LR),
			hasLL = selectedData.HasPolarization(Polarization::LL),
			hasI = selectedData.HasPolarization(Polarization::StokesI),
			hasQ = selectedData.HasPolarization(Polarization::StokesQ),
			hasU = selectedData.HasPolarization(Polarization::StokesU),
			hasV = selectedData.HasPolarization(Polarization::StokesV);
		if(pp && qq)
		{
			pq = false; qp = false;
			if((hasXX && !hasYY) || (hasRR && !hasLL))
				qq = false;
			else if((hasYY && !hasXX) || (hasLL && !hasRR))
				pp = false;
			else if(!hasXX && !hasYY && !hasRR && !hasLL && !hasI)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
		else if(pq && qp)
		{
			pp = false; qq = false;
			if((hasXY && !hasYX) || (hasRL && !hasLR) || hasQ)
				qp = false;
			else if((hasYX && !hasXY) || (hasLR && !hasRL) || hasU)
				pq = false;
			else if(!hasXY && !hasYX && !hasRL && !hasLR && !hasU)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
		else if(pp)
		{
			if(!hasXX && !hasRR && !hasI)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
		else if(pq)
		{
			if(!hasXY && !hasRL && !hasQ)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
		else if(qp)
		{
			if(!hasYX && !hasLR && !hasU)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
		else if(qq)
		{
			if(!hasYY && !hasLL && !hasV)
				getFirstAvailablePolarization(pp, pq, qp, qq);
		}
	}
}

Mask2DCPtr ImageComparisonController::getSelectedPolarizationMask(const TimeFrequencyData& data) const
{
	if(data.MaskCount() <= 1)
		return data.GetSingleMask();
	else {
		if(data.MaskCount() == 4)
		{
			if(_showPP && _showQQ)
			{
				Mask2DPtr mask = Mask2D::MakePtr(*data.GetMask(0));
				mask->Join(*data.GetMask(3));
				return mask;
			}
			else if(_showPQ && _showQP)
			{
				Mask2DPtr mask = Mask2D::MakePtr(*data.GetMask(1));
				mask->Join(*data.GetMask(2));
				return mask;
			}
			else if(_showPP)
				return data.GetMask(0);
			else if(_showPQ)
				return data.GetMask(1);
			else if(_showQP)
				return data.GetMask(2);
			else //if(_showQQ)
				return data.GetMask(3);
		}
		else // data->MaskCount() == 2
		{
			if(_showPP && _showQQ)
			{
				Mask2DPtr mask = Mask2D::MakePtr(*data.GetMask(0));
				mask->Join(*data.GetMask(1));
				return mask;
			}
			else if(_showPP)
				return data.GetMask(0);
			else //if(_showQQ)
				return data.GetMask(1);
		}
	}
}

void ImageComparisonController::updateVisualizedImageAndMask()
{
	TimeFrequencyData* data;
	if(_visualizedImage == 0)
		data = &_dataList.back().data;
	else
		data = &_dataList[_visualizedImage].data;
	_plot.SetAlternativeMask(getSelectedPolarizationMask(*data));
	_plot.SetOriginalMask(getSelectedPolarizationMask(_dataList.front().data));
	updateVisualizedImage();
}

void ImageComparisonController::updateVisualizedImage()
{
  Image2DCPtr image;
	const TimeFrequencyData& selectedData = _dataList[_visualizedImage].data;
	if(!selectedData.IsEmpty())
	{
		if(_showPP && _showQQ)
		{
			if((selectedData.HasPolarization(Polarization::XX) && selectedData.HasPolarization(Polarization::YY)) ||
				(selectedData.HasPolarization(Polarization::RR) && selectedData.HasPolarization(Polarization::LL)) ||
				(selectedData.HasPolarization(Polarization::StokesI)) )
			image = selectedData.Make(Polarization::StokesI).GetSingleImage();
		}
		else if(_showQP && _showPQ)
		{
			if(selectedData.HasPolarization(Polarization::XY) && selectedData.HasPolarization(Polarization::YX))
				image = selectedData.Make(Polarization::StokesU).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::RL) && selectedData.HasPolarization(Polarization::LR))
				image = selectedData.Make(Polarization::StokesQ).GetSingleImage();
		}
		else if(_showPP)
		{
			if(selectedData.HasPolarization(Polarization::XX))
				image = selectedData.Make(Polarization::XX).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::RR))
				image = selectedData.Make(Polarization::RR).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::StokesI))
				image = selectedData.Make(Polarization::StokesI).GetSingleImage();
		}
		else if(_showPQ)
		{
			if(selectedData.HasPolarization(Polarization::XY))
				image = selectedData.Make(Polarization::XY).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::RL))
				image = selectedData.Make(Polarization::RL).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::StokesQ))
				image = selectedData.Make(Polarization::StokesQ).GetSingleImage();
		}
		else if(_showQP)
		{
			if(selectedData.HasPolarization(Polarization::YX))
				image = selectedData.Make(Polarization::YX).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::LR))
				image = selectedData.Make(Polarization::LR).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::StokesU))
				image = selectedData.Make(Polarization::StokesU).GetSingleImage();
		}
		else if(_showQQ)
		{
			if(selectedData.HasPolarization(Polarization::YY))
				image = selectedData.Make(Polarization::YY).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::LL))
				image = selectedData.Make(Polarization::LL).GetSingleImage();
			else if(selectedData.HasPolarization(Polarization::StokesV))
				image = selectedData.Make(Polarization::StokesV).GetSingleImage();
		}
	}
	if(image == nullptr)
		_plot.Clear();
	else
		_plot.SetImage(image);
}

void ImageComparisonController::ClearAllButOriginal()
{
	if(_dataList.size() > 1)
	{
		_dataList.erase(_dataList.begin()+1, _dataList.end());
		SetVisualization(0);
		_visualizationListChange.emit();
	}
}
