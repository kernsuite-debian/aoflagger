#include "timefrequencydata.h"
#include "stokesimager.h"

#include "../util/ffttools.h"

Image2DCPtr TimeFrequencyData::GetAbsoluteFromComplex(const Image2DCPtr &real, const Image2DCPtr &imag) const
{
	return Image2DPtr(FFTTools::CreateAbsoluteImage(*real, *imag));
}
			
Image2DCPtr TimeFrequencyData::GetSum(const Image2DCPtr &left, const Image2DCPtr &right) const
{
	return StokesImager::CreateSum(left, right);
}

Image2DCPtr TimeFrequencyData::GetNegatedSum(const Image2DCPtr &left, const Image2DCPtr &right) const
{
	return StokesImager::CreateNegatedSum(left, right);
}

Image2DCPtr TimeFrequencyData::GetDifference(const Image2DCPtr &left, const Image2DCPtr &right) const
{
	return StokesImager::CreateDifference(left, right);
}

Image2DCPtr TimeFrequencyData::getSinglePhaseFromTwoPolPhase(size_t polA, size_t polB) const
{
	return StokesImager::CreateAvgPhase(_data[polA]._images[0], _data[polB]._images[0]);
}

Mask2DCPtr TimeFrequencyData::GetCombinedMask() const
{
	if(MaskCount() == 0)
		return GetSetMask<false>();
	else if(MaskCount() == 1)
		return GetMask(0);
	else
	{
		Mask2DPtr mask(new Mask2D(*GetMask(0)));
		size_t i = 0;
		while(i!= MaskCount())
		{
			const Mask2DCPtr& curMask = GetMask(i);
			for(unsigned y=0;y<mask->Height();++y) {
				for(unsigned x=0;x<mask->Width();++x) {
					bool v = curMask->Value(x, y);
					if(v)
						mask->SetValue(x, y, true);
				}
			}
			++i;
		}
		return mask;
	}
}

TimeFrequencyData TimeFrequencyData::Make(enum ComplexRepresentation representation) const
{
	if(representation == _complexRepresentation)
		return TimeFrequencyData(*this);
	else if(_complexRepresentation == ComplexParts)
	{
		TimeFrequencyData data;
		data._complexRepresentation = representation;
		data._data.resize(_data.size());
		for(size_t i=0; i!=_data.size(); ++i)
		{
			const PolarizedTimeFrequencyData& source = _data[i];
			PolarizedTimeFrequencyData& dest = data._data[i];
			dest._polarization = source._polarization;
			dest._flagging = source._flagging;
			switch(representation)
			{
				case RealPart:
					dest._images[0] = source._images[0];
					break;
				case ImaginaryPart:
					dest._images[0] = source._images[1];
					break;
				case AmplitudePart:
					dest._images[0] = GetAbsoluteFromComplex(source._images[0], source._images[1]);
					break;
				case PhasePart:
					dest._images[0] = StokesImager::CreateAvgPhase(source._images[0], source._images[1]);
					break;
				case ComplexParts:
					break; // already handled above.
			}
		}
		return data;
	} else throw BadUsageException("Request for time/frequency data with a phase representation that can not be extracted from the source (source is not complex)");
}

TimeFrequencyData TimeFrequencyData::MakeFromComplexCombination(const TimeFrequencyData& real, const TimeFrequencyData& imaginary)
{
	if(real.ComplexRepresentation() == ComplexParts ||
		imaginary.ComplexRepresentation() == ComplexParts)
		throw BadUsageException("Trying to create complex TF data from incorrect phase representations");
	if(real.Polarizations() != imaginary.Polarizations())
		throw BadUsageException("Combining real/imaginary time frequency data from different polarisations");
	TimeFrequencyData data;
	data._data.resize(real._data.size());
	data._complexRepresentation = ComplexParts;
	for(size_t i=0; i!=real._data.size(); ++i)
	{
		data._data[i]._polarization = real._data[i]._polarization;
		data._data[i]._images[0] = real._data[i]._images[0];
		data._data[i]._images[1] = imaginary._data[i]._images[0];
		data._data[i]._flagging = real._data[i]._flagging;
	}
	return data;
}

TimeFrequencyData TimeFrequencyData::MakeFromPolarizationCombination(const TimeFrequencyData &xx, const TimeFrequencyData &xy, const TimeFrequencyData &yx, const TimeFrequencyData &yy)
{
	if(xx.ComplexRepresentation() != xy.ComplexRepresentation() ||
		xx.ComplexRepresentation() != yx.ComplexRepresentation() ||
		xx.ComplexRepresentation() != yy.ComplexRepresentation())
		throw BadUsageException("Trying to create dipole time frequency combination from data with different phase representations!");

	TimeFrequencyData data;
	data._data.resize(4);
	data._complexRepresentation = xx._complexRepresentation;
	for(size_t i=0; i!=xx._data.size(); ++i)
	{
		data._data[0] = xx._data[0];
		data._data[1] = xy._data[0];
		data._data[2] = yx._data[0];
		data._data[3] = yy._data[0];
	}
	return data;
}

TimeFrequencyData TimeFrequencyData::MakeFromPolarizationCombination(const TimeFrequencyData &first, const TimeFrequencyData &second)
{
	if(first.ComplexRepresentation() != second.ComplexRepresentation())
		throw BadUsageException("Trying to create polarization combination from data with different phase representations!");

	TimeFrequencyData data;
	data._data = first._data;
	data._complexRepresentation = first._complexRepresentation;
	data._data.insert(data._data.end(), second._data.begin(), second._data.end());
	return data;
}

void TimeFrequencyData::SetImagesToZero()
{
	if(!IsEmpty())
	{
		Image2DPtr zeroImage = Image2D::CreateZeroImagePtr(ImageWidth(), ImageHeight());
		Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(ImageWidth(), ImageHeight());
		for(PolarizedTimeFrequencyData& data : _data)
		{
			data._images[0] = zeroImage;
			if(data._images[1])
				data._images[1] = zeroImage;
			data._flagging = mask;
		}
	}
}

void TimeFrequencyData::MultiplyImages(long double factor)
{
	for(PolarizedTimeFrequencyData& data : _data)
	{
		if(data._images[0])
		{
			Image2DPtr newImage(new Image2D(*data._images[0]));
			newImage->MultiplyValues(factor);
			data._images[0] = newImage;
		}
		if(data._images[1])
		{
			Image2DPtr newImage(new Image2D(*data._images[1]));
			newImage->MultiplyValues(factor);
			data._images[1] = newImage;
		}
	}
}

void TimeFrequencyData::JoinMask(const TimeFrequencyData& other)
{
	if(other.MaskCount() == 0)
	{
		// Nothing to be done; other has no flags
	} else if(other.MaskCount() == MaskCount())
	{
		for(size_t i=0;i<MaskCount();++i)
		{
			Mask2D mask(*GetMask(i));
			mask.Join(*other.GetMask(i));
			SetMask(i, Mask2DPtr(new Mask2D(mask)));
		}
	} else if(other.MaskCount() == 1)
	{
		if(MaskCount() == 0)
		{
			for(size_t i=0; i!=_data.size(); ++i)
				_data[i]._flagging = other._data[0]._flagging;
		}	
		else {
			for(size_t i=0;i<MaskCount();++i)
			{
				Mask2D mask(*GetMask(i));
				mask.Join(*other.GetMask(0));
				SetMask(i, Mask2DPtr(new Mask2D(mask)));
			}
		}
	} else if(MaskCount() == 1)
	{
		Mask2D mask(*GetMask(0));
		mask.Join(*other.GetSingleMask());
		SetMask(0, Mask2DPtr(new Mask2D(mask)));
	}	else if(MaskCount() == 0 && _data.size() == other._data.size())
	{
		for(size_t i=0; i!=_data.size(); ++i)
			_data[i]._flagging = other._data[i]._flagging;
	}
	else
		throw BadUsageException("Joining time frequency flagging with incompatible structures");
}

