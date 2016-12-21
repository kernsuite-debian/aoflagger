#ifndef TIMEFREQUENCYDATA_H
#define TIMEFREQUENCYDATA_H

#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <stdexcept>

#include "image2d.h"
#include "mask2d.h"
#include "polarization.h"

#include "../baseexception.h"

class TimeFrequencyData
{
	public:
		enum PhaseRepresentation { PhasePart, AmplitudePart, RealPart, ImaginaryPart, ComplexRepresentation };

		TimeFrequencyData() :
			_phaseRepresentation(AmplitudePart),
			_data()
		{ }

		TimeFrequencyData(PhaseRepresentation phaseRepresentation,
				PolarizationEnum polarisationType, const Image2DCPtr& image) :
			_phaseRepresentation(phaseRepresentation)
		{
			if(phaseRepresentation == ComplexRepresentation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create complex representation from single image");
			_data.emplace_back(polarisationType, image);
		}

		TimeFrequencyData(PolarizationEnum polarisationType,
											const Image2DCPtr &real,
											const Image2DCPtr &imaginary) :
				_phaseRepresentation(ComplexRepresentation)
		{
			_data.emplace_back(polarisationType, real, imaginary);
		}
		
		TimeFrequencyData(PhaseRepresentation phase,
											PolarizationEnum polarisationA,
											const Image2DCPtr &imageA,
											PolarizationEnum polarisationB,
											const Image2DCPtr &imageB) :
				_phaseRepresentation(phase)
		{
			_data.reserve(2);
			_data.emplace_back(polarisationA, imageA);
			_data.emplace_back(polarisationB, imageB);
		}
		
		TimeFrequencyData(PolarizationEnum polarisationA,
											const Image2DCPtr &realA,
											const Image2DCPtr &imaginaryA,
											PolarizationEnum polarisationB,
											const Image2DCPtr &realB,
											const Image2DCPtr &imaginaryB) :
				_phaseRepresentation(ComplexRepresentation)
		{
			_data.reserve(2);
			_data.emplace_back(polarisationA, realA, imaginaryA);
			_data.emplace_back(polarisationB, realB, imaginaryB);
		}
		
		TimeFrequencyData(
			PhaseRepresentation phase,
			PolarizationEnum* polarizations,
			size_t polarizationCount,
			Image2DCPtr* images) :
				_phaseRepresentation(phase)
		{
			_data.reserve(polarizationCount);
			if(phase == ComplexRepresentation)
			{
				for(size_t p=0; p!=polarizationCount; p++)
					_data.emplace_back(polarizations[p], images[p*2], images[p*2+1]);
			}
			else {
				for(size_t p=0; p!=polarizationCount; p++)
					_data.emplace_back(polarizations[p], images[p]);
			}
		}
		
		TimeFrequencyData(
			PolarizationEnum* polarizations,
			size_t polarizationCount,
			Image2DCPtr* realImages, Image2DCPtr* imaginaryImages) :
				_phaseRepresentation(ComplexRepresentation)
		{
			_data.reserve(polarizationCount);
			for(size_t p=0; p!=polarizationCount; p++)
				_data.emplace_back(polarizations[p], realImages[p], imaginaryImages[p]);
		}
		
		TimeFrequencyData(
			PolarizationEnum* polarizations,
			size_t polarizationCount,
			Image2DPtr* realImages, Image2DPtr* imaginaryImages) :
				_phaseRepresentation(ComplexRepresentation)
		{
			_data.reserve(polarizationCount);
			for(size_t p=0; p!=polarizationCount; p++)
				_data.emplace_back(polarizations[p], realImages[p], imaginaryImages[p]);
		}
		
		static TimeFrequencyData FromLinear(
			PhaseRepresentation phaseRepresentation,
			const Image2DCPtr& xx,
			const Image2DCPtr& xy,
			const Image2DCPtr& yx,
			const Image2DCPtr& yy)
		{
			TimeFrequencyData data;
			data._phaseRepresentation = phaseRepresentation;
			if(phaseRepresentation == ComplexRepresentation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create complex full-Stokes representation from four images");
			data._data.reserve(4);
			data._data.emplace_back(Polarization::XX, xx);
			data._data.emplace_back(Polarization::XY, xy);
			data._data.emplace_back(Polarization::YX, yx);
			data._data.emplace_back(Polarization::YY, yy);
			return data;
		}

		static TimeFrequencyData FromLinear(
			const Image2DCPtr &xxReal, const Image2DCPtr &xxImag,
			const Image2DCPtr &xyReal, const Image2DCPtr &xyImag,
			const Image2DCPtr &yxReal, const Image2DCPtr &yxImag,
			const Image2DCPtr &yyReal, const Image2DCPtr &yyImag)
		{
			TimeFrequencyData data;
			data._phaseRepresentation = ComplexRepresentation;
			data._data.reserve(4);
			data._data.emplace_back(Polarization::XX, xxReal, xxImag);
			data._data.emplace_back(Polarization::XY, xyReal, xyImag);
			data._data.emplace_back(Polarization::YX, yxReal, yxImag);
			data._data.emplace_back(Polarization::YY, yyReal, yyImag);
			return data;
		}

		static TimeFrequencyData FromLinear(size_t polarisationCount, Image2DCPtr *realImages, Image2DCPtr *imagImages)
		{
			switch(polarisationCount)
			{
				case 1:
					return TimeFrequencyData(Polarization::StokesI, realImages[0], imagImages[0]);
				case 2:
					return TimeFrequencyData(Polarization::XX, realImages[0], imagImages[0],
																	 Polarization::YY, realImages[1], imagImages[1]);
				case 4:
					return FromLinear(realImages[0], imagImages[0], realImages[1], imagImages[1], realImages[2], imagImages[2], realImages[3], imagImages[3]);
				default:
					throw BadUsageException("Can not create TimeFrequencyData structure with polarization type other than 1, 2 or 4 polarizations using FromLinear().");
			}
		}
		
		bool IsEmpty() const { return _data.empty(); }

		bool HasPolarisation(PolarizationEnum polarisation) const
		{
			for(const PolarizedTimeFrequencyData& data : _data)
				if(data._polarization == polarisation)
					return true;
			return false;
		}
		
		PolarizationEnum GetPolarisation(size_t index) const
		{ return _data[index]._polarization; }
		
		bool HasXX() const { return HasPolarisation(Polarization::XX); }

		bool HasXY() const { return HasPolarisation(Polarization::XY); }

		bool HasYX() const { return HasPolarisation(Polarization::YX); }

		bool HasYY() const { return HasPolarisation(Polarization::YY); }
		
		/**
		 * This function returns a new Image2D that contains
		 * an image that can be used best for thresholding-like
		 * RFI methods, or visualization. The encapsulated data
		 * may be converted in order to do so.
		 * @return A new image containing the TF-data.
		 */
		Image2DCPtr GetSingleImage() const
		{
			switch(_phaseRepresentation)
			{
				case PhasePart:
				case AmplitudePart:
				case RealPart:
				case ImaginaryPart:
					return GetSingleImageFromSinglePhaseImage();
				case ComplexRepresentation:
					return GetSingleAbsoluteFromComplex();
			}
			throw BadUsageException("Incorrect phase representation");
		}

		Mask2DCPtr GetSingleMask() const
		{
			return GetCombinedMask();
		}

		std::array<Image2DCPtr,2> GetSingleComplexImage() const
		{
			if(_phaseRepresentation != ComplexRepresentation)
				throw BadUsageException("Trying to create single complex image, but no complex data available");
			if(_data.size() != 1)
				throw BadUsageException("Not implemented");
			if(_data[0]._images[0] == nullptr || _data[0]._images[1] == nullptr)
				throw BadUsageException("Requesting non-existing image");
			return { _data[0]._images[0], _data[0]._images[1] };
		}
		
		void Set(PolarizationEnum polarisationType,
			const Image2DCPtr &real,
			const Image2DCPtr &imaginary)
		{
			_phaseRepresentation = ComplexRepresentation;
			_data.clear();
			_data.emplace_back(polarisationType, real, imaginary);
		}

		void SetNoMask()
		{
			for(PolarizedTimeFrequencyData& data : _data)
				data._flagging = nullptr;
		}

		void SetGlobalMask(const Mask2DCPtr &mask)
		{
			SetNoMask();
			for(PolarizedTimeFrequencyData& data : _data)
				data._flagging = mask;
		}

		Mask2DCPtr GetMask(PolarizationEnum polarisation) const
		{
			for(const PolarizedTimeFrequencyData& data : _data)
			{
				if(data._polarization == polarisation)
				{
					if(data._flagging == nullptr)
						return GetSetMask<false>();
					else
						return data._flagging;
				}
			}
			return GetSingleMask();
		}

		void SetIndividualPolarisationMasks(const Mask2DCPtr* maskPerPolarization)
		{
			for(size_t p=0; p!=_data.size(); ++p)
				_data[p]._flagging = maskPerPolarization[p];
		}

		void SetIndividualPolarisationMasks(const Mask2DPtr* maskPerPolarization)
		{
			for(size_t p=0; p!=_data.size(); ++p)
				_data[p]._flagging = maskPerPolarization[p];
		}

		void SetIndividualPolarisationMasks(const Mask2DCPtr &maskA, const Mask2DCPtr &maskB)
		{
			if(_data.size() != 2)
				throw BadUsageException("Trying to set two individual mask in non-matching time frequency data");
			_data[0]._flagging = maskA;
			_data[1]._flagging = maskB;
		}

		void SetIndividualPolarisationMasks(const Mask2DCPtr &maskA, const Mask2DCPtr &maskB, const Mask2DCPtr &maskC, const Mask2DCPtr &maskD)
		{
			if(_data.size() != 4)
				throw BadUsageException("Trying to set four individual mask in non-matching time frequency data");
			_data[0]._flagging = maskA;
			_data[1]._flagging = maskB;
			_data[2]._flagging = maskC;
			_data[3]._flagging = maskD;
		}

		TimeFrequencyData *CreateTFData(PhaseRepresentation phase) const;
		
		TimeFrequencyData Make(PolarizationEnum polarisation) const
		{
			for(const PolarizedTimeFrequencyData& data : _data)
			{
				if(data._polarization == polarisation)
					return TimeFrequencyData(_phaseRepresentation, data);
			}
			
			TimeFrequencyData newData;
			size_t
				xxPol = getPolarisationIndex(Polarization::XX),
				xyPol = getPolarisationIndex(Polarization::XY),
				yxPol = getPolarisationIndex(Polarization::YX),
				yyPol = getPolarisationIndex(Polarization::YY);
			bool hasLinear = xxPol < _data.size() || xyPol < _data.size();
			if(hasLinear)
			{
				if(_phaseRepresentation == ComplexRepresentation)
				{
					switch(polarisation)
					{
					case Polarization::StokesI:
						newData = TimeFrequencyData(Polarization::StokesI, getFirstSum(xxPol, yyPol), getSecondSum(xxPol, yyPol));
						break;
					case Polarization::StokesQ:
						newData = TimeFrequencyData(Polarization::StokesQ, getFirstDiff(xxPol, yyPol), getSecondDiff(xxPol, yyPol));
						break;
					case Polarization::StokesU:
						newData = TimeFrequencyData(Polarization::StokesU, getFirstSum(xyPol, yxPol), getSecondSum(xyPol, yxPol));
						break;
					case Polarization::StokesV:
						newData = TimeFrequencyData(Polarization::StokesV, getNegRealPlusImag(xyPol, yxPol), getRealMinusImag(xyPol, yxPol));
						break;
					default:
						throw BadUsageException("Polarisation not available or not implemented");
					}
				}
				else // _phaseRepresentation != ComplexRepresentation
				{
					// TODO should be done on real or imaginary
					switch(polarisation)
					{
						case Polarization::StokesI:
							newData = TimeFrequencyData(_phaseRepresentation, Polarization::StokesI, getFirstSum(xxPol, yyPol));
							break;
						case Polarization::StokesQ:
							newData = TimeFrequencyData(_phaseRepresentation, Polarization::StokesQ, getFirstDiff(xxPol, yyPol));
							break;
						default:
							throw BadUsageException("Requested polarisation type not available in time frequency data");
					}
				}
			}
			else {
				size_t
					rrPol = getPolarisationIndex(Polarization::RR),
					rlPol = getPolarisationIndex(Polarization::RL),
					lrPol = getPolarisationIndex(Polarization::LR),
					llPol = getPolarisationIndex(Polarization::LL);
				bool hasCircular = rrPol < _data.size() || rlPol < _data.size();
				if(hasCircular)
				{
					if(_phaseRepresentation == ComplexRepresentation)
					{
						switch(polarisation)
						{
						case Polarization::StokesI:
							newData = TimeFrequencyData(Polarization::StokesI, getFirstSum(rrPol, llPol), getSecondSum(rrPol, llPol));
							break;
						case Polarization::StokesQ: // Q = RL + LR
							newData = TimeFrequencyData(Polarization::StokesQ, getFirstSum(rlPol, rlPol), getSecondSum(rlPol, lrPol));
							break;
						case Polarization::StokesU: // U_r = RL_i - LR_i, U_i = -RL_r + LR_r
							newData = TimeFrequencyData(Polarization::StokesU, getSecondDiff(rlPol, lrPol), getFirstDiff(lrPol, rlPol));
							break;
						case Polarization::StokesV: // V = RR - LL
							newData = TimeFrequencyData(Polarization::StokesV, getFirstDiff(rrPol, llPol), getSecondDiff(rrPol, llPol));
							break;
						default:
							throw BadUsageException("Requested polarisation type not available in time frequency data");
							break;
						}
					}
				}
				else
					throw BadUsageException("Trying to convert the polarization in time frequency data in an invalid way");
			}
			newData.SetGlobalMask(GetMask(polarisation));
			return newData;
		}

		TimeFrequencyData *CreateTFData(PolarizationEnum polarisation) const
		{
			return new TimeFrequencyData(Make(polarisation));
		}

		Image2DCPtr GetRealPart() const
		{
			if(_data.size() != 1)
			{
				throw BadUsageException("This tfdata contains !=1 polarizations; which real part should I return?");
			} else if(_phaseRepresentation == ComplexRepresentation || _phaseRepresentation == RealPart) {
				return _data[0]._images[0];
			} else {
				throw BadUsageException("Trying to retrieve real part from time frequency data in which phase doesn't have a complex or real representation");
			}
		}

		Image2DCPtr GetImaginaryPart() const
		{
			if(_data.size() != 1)
			{
				throw BadUsageException("This tfdata contains !=1 polarizations; which imaginary part should I return?");
			} else if(_phaseRepresentation == ComplexRepresentation) {
				return _data[0]._images[1];
			} else if(_phaseRepresentation == ImaginaryPart) {
				return _data[0]._images[0];
			} else {
				throw BadUsageException("Trying to retrieve imaginary part from time frequency data in which phase doesn't have a complex or real representation");
			}
		}

		size_t ImageWidth() const
		{
			if(!_data.empty() && _data[0]._images[0] != nullptr)
				return _data[0]._images[0]->Width();
			else
				return 0;
		}
		
		size_t ImageHeight() const
		{
			if(!_data.empty() && _data[0]._images[0] != nullptr)
				return _data[0]._images[0]->Height();
			else
				return 0;
		}

		enum PhaseRepresentation PhaseRepresentation() const
		{
			return _phaseRepresentation;
		}
		
		std::vector<PolarizationEnum> Polarisations() const
		{
			std::vector<PolarizationEnum> pols;
			for(const PolarizedTimeFrequencyData& data : _data)
				pols.push_back(data._polarization);
			return pols;
		}

		void Subtract(const TimeFrequencyData &rhs)
		{
			if(rhs._data.size() != _data.size() || rhs._phaseRepresentation != _phaseRepresentation)
			{
				std::stringstream s;
				s << "Can not subtract time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< rhs._data.size() << " vs. " << _data.size() << ")";
				throw BadUsageException(s.str());
			}
			for(size_t i=0; i!=_data.size(); ++i)
			{
				if(_data[i]._images[0] == nullptr)
					throw BadUsageException("Can't subtract TFs with unset image data");
				_data[i]._images[0] = Image2D::CreateFromDiff(_data[i]._images[0], rhs._data[i]._images[0]);
				if(_data[i]._images[1])
					_data[i]._images[1] = Image2D::CreateFromDiff(_data[i]._images[1], rhs._data[i]._images[1]);
			}
		}

		void SubtractAsRHS(const TimeFrequencyData &lhs)
		{
			if(lhs._data.size() != _data.size() || lhs._phaseRepresentation != _phaseRepresentation)
			{
				std::stringstream s;
				s << "Can not subtract time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< lhs._data.size() << " vs. " << _data.size() << ")";
				throw BadUsageException(s.str());
			}
			for(size_t i=0; i!=_data.size(); ++i)
			{
				if(_data[i]._images[0] == nullptr)
					throw BadUsageException("Can't subtract TFs with unset image data");
				_data[i]._images[0] = Image2D::CreateFromDiff(lhs._data[i]._images[0], _data[i]._images[0]);
				if(_data[i]._images[1])
					_data[i]._images[1] = Image2D::CreateFromDiff(lhs._data[i]._images[1], _data[i]._images[1]);
			}
		}

		static TimeFrequencyData *CreateTFDataFromDiff(const TimeFrequencyData &lhs, const TimeFrequencyData &rhs)
		{
			if(lhs._data.size() != rhs._data.size() || lhs._phaseRepresentation != rhs._phaseRepresentation)
			{
				std::stringstream s;
				s << "Can not subtract time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< lhs._data.size() << " vs. " << rhs._data.size() << ")";
				throw BadUsageException(s.str());
			}
			TimeFrequencyData *data = new TimeFrequencyData(lhs);
			for(size_t i=0;i<lhs._data.size();++i)
			{
				if(lhs._data[i]._images[0] == nullptr)
					throw BadUsageException("Can't subtract TFs with unset image data");
				data->_data[i]._images[0] = Image2D::CreateFromDiff(lhs._data[i]._images[0], rhs._data[i]._images[0]);
				if(lhs._data[i]._images[1])
					data->_data[i]._images[1] = Image2D::CreateFromDiff(lhs._data[i]._images[1], rhs._data[i]._images[1]);
			}
			return data;
		}

		static TimeFrequencyData *CreateTFDataFromSum(const TimeFrequencyData &lhs, const TimeFrequencyData &rhs)
		{
			if(lhs._data.size() != rhs._data.size() || lhs._phaseRepresentation != rhs._phaseRepresentation)
			{
				std::stringstream s;
				s << "Can not add time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< lhs._data.size() << " vs. " << rhs._data.size() << ")";
				throw BadUsageException(s.str());
			}
			TimeFrequencyData *data = new TimeFrequencyData(lhs);
			for(size_t i=0;i<lhs._data.size();++i)
			{
				if(lhs._data[i]._images[0] == nullptr)
					throw BadUsageException("Can't add TFs with unset image data");
				data->_data[i]._images[0] = Image2D::CreateFromSum(lhs._data[i]._images[0], rhs._data[i]._images[0]);
				if(lhs._data[i]._images[1])
					data->_data[i]._images[1] = Image2D::CreateFromSum(lhs._data[i]._images[1], rhs._data[i]._images[1]);
			}
			return data;
		}

		size_t ImageCount() const {
			size_t masks = 0;
			for(const PolarizedTimeFrequencyData& data : _data)
			{
				if(data._images[0]) ++masks;
				if(data._images[1]) ++masks;
			}
			return masks; 
		}
		
		size_t MaskCount() const {
			size_t masks = 0;
			for(const PolarizedTimeFrequencyData& data : _data)
				if(data._flagging) ++masks;
			return masks; 
		}

		const Image2DCPtr &GetImage(size_t imageIndex) const
		{
			size_t index = 0;
			for(const PolarizedTimeFrequencyData& data : _data)
			{
				if(data._images[0])
				{
					if(index == imageIndex)
						return data._images[0];
					++index;
				}
				if(data._images[1])
				{
					if(index == imageIndex)
						return data._images[1];
					++index;
				}
			}
			throw BadUsageException("Invalid image index in GetImage()");
		}
		const Mask2DCPtr &GetMask(size_t maskIndex) const
		{
			size_t index = 0;
			for(const PolarizedTimeFrequencyData& data : _data)
			{
				if(data._flagging)
				{
					if(index == maskIndex)
						return data._flagging;
					++index;
				}
			}
			std::ostringstream msg;
			msg << "Invalid mask index of " << maskIndex << " in GetMask(): mask count is " << MaskCount();
			throw BadUsageException(msg.str());
		}
		void SetImage(size_t imageIndex, const Image2DCPtr &image)
		{
			size_t index = 0;
			for(PolarizedTimeFrequencyData& data : _data)
			{
				if(data._images[0])
				{
					if(index == imageIndex)
					{
						data._images[0] = image;
						return;
					}
					++index;
				}
				if(data._images[1])
				{
					if(index == imageIndex)
					{
						data._images[1] = image;
						return;
					}
					++index;
				}
			}
			throw BadUsageException("Invalid image index in SetImage()");
		}
		void SetMask(size_t maskIndex, const Mask2DCPtr &mask)
		{
			size_t index = 0;
			for(PolarizedTimeFrequencyData& data : _data)
			{
				if(data._flagging)
				{
					if(index == maskIndex)
					{
						data._flagging = mask;
						return;
					}
					++index;
				}
			}
			throw BadUsageException("Invalid mask index in SetMask()");
		}
		void SetMask(const TimeFrequencyData &source)
		{
			source.CopyFlaggingTo(this);
		}

		static TimeFrequencyData *CreateTFDataFromComplexCombination(const TimeFrequencyData &real, const TimeFrequencyData &imaginary);

		static TimeFrequencyData *CreateTFDataFromPolarizationCombination(const TimeFrequencyData &xx, const TimeFrequencyData &xy, const TimeFrequencyData &yx, const TimeFrequencyData &yy);

		static TimeFrequencyData *CreateTFDataFromPolarizationCombination(const TimeFrequencyData &xx, const TimeFrequencyData &yy);

		void SetImagesToZero();
		template<bool Value>
		void SetMasksToValue()
		{
			if(!IsEmpty())
			{
				Mask2DPtr mask = Mask2D::CreateSetMaskPtr<Value>(ImageWidth(), ImageHeight());
				for(PolarizedTimeFrequencyData& data : _data)
				{
					data._flagging = mask;
				}
			}
		}
		
		void MultiplyImages(long double factor);
		void JoinMask(const TimeFrequencyData &other);

		void Trim(unsigned timeStart, unsigned freqStart, unsigned timeEnd, unsigned freqEnd)
		{
			for(PolarizedTimeFrequencyData& data : _data)
			{
				if(data._images[0])
					data._images[0] = data._images[0]->Trim(timeStart, freqStart, timeEnd, freqEnd);
				if(data._images[1])
					data._images[1] = data._images[1]->Trim(timeStart, freqStart, timeEnd, freqEnd);
				if(data._flagging)
					data._flagging = data._flagging->Trim(timeStart, freqStart, timeEnd, freqEnd);
			}
		}
		
		std::string Description() const
		{
			std::ostringstream s;
			switch(_phaseRepresentation)
			{
				case RealPart: s << "Real component of "; break;
				case ImaginaryPart: s << "Imaginary component of "; break;
				case PhasePart: s << "Phase of "; break;
				case AmplitudePart: s << "Amplitude of "; break;
				case ComplexRepresentation:
					break;
			}
			if(_data.empty())
				s << "empty";
			else
			{
				s << "(" << Polarization::TypeToFullString(_data[0]._polarization);
				for(size_t i=1; i!=_data.size(); ++i)
					s << "," << Polarization::TypeToFullString(_data[i]._polarization);
				s << ")";
			}
			return s.str();
		}
		
		size_t PolarisationCount() const
		{
			return _data.size();
		}
		
		TimeFrequencyData *CreateTFDataFromPolarisationIndex(size_t index) const
		{
			return new TimeFrequencyData(_phaseRepresentation, _data[index]);
		}
		
		void SetPolarizationData(size_t polarizationIndex, const TimeFrequencyData &data)
		{
			if(data.PolarisationCount() != 1)
				throw BadUsageException("Trying to set multiple polarizations by single polarization index");
			else if(data.PhaseRepresentation() != PhaseRepresentation())
				throw BadUsageException("Trying to combine TFData's with different phase representations");
			else
				_data[polarizationIndex] = data._data[0];
		}

		void SetImageSize(size_t width, size_t height)
		{
			for(size_t i=0;i<_data.size();++i)
			{
				if(_data[i]._images[0])
					_data[i]._images[0] = Image2D::CreateUnsetImagePtr(width, height);
				if(_data[i]._images[1])
					_data[i]._images[1] = Image2D::CreateUnsetImagePtr(width, height);
				if(_data[i]._flagging)
					_data[i]._flagging = Mask2D::CreateUnsetMaskPtr(width, height);
			}
		}

		void CopyFrom(const TimeFrequencyData &source, size_t destX, size_t destY)
		{
			if(source._data.size() != _data.size())
				throw BadUsageException("CopyFrom: tf data do not match");
			for(size_t i=0;i<_data.size();++i)
			{
				if(_data[i]._images[0])
				{
					Image2DPtr image = Image2D::CreateCopy(_data[i]._images[0]);
					image->CopyFrom(source._data[i]._images[0], destX, destY);
					_data[i]._images[0] = image;
				}
				if(_data[i]._images[1])
				{
					Image2DPtr image = Image2D::CreateCopy(_data[i]._images[1]);
					image->CopyFrom(source._data[i]._images[1], destX, destY);
					_data[i]._images[1] = image;
				}
				if(_data[i]._flagging)
				{
					Mask2DPtr mask = Mask2D::CreateCopy(_data[i]._flagging);
					mask->CopyFrom(source._data[i]._flagging, destX, destY);
					_data[i]._flagging = mask;
				}
			}
		}
		
		/**
		 * Will return true when this is the imaginary part of the visibilities. Will throw
		 * an exception when the data is neither real nor imaginary.
		 */
		bool IsImaginary() const
		{
			if(PhaseRepresentation() == RealPart)
				return false;
			else if(PhaseRepresentation() == ImaginaryPart)
				return true;
			else
				throw std::runtime_error("Data is not real or imaginary");
		}
		
	private:
		Image2DCPtr GetSingleAbsoluteFromComplex() const
		{
			if(_data.size() == 4)
				return GetAbsoluteFromComplex(getFirstSum(0, 3), getSecondSum(0, 3));
			else if(_data.size() == 2)
				return GetAbsoluteFromComplex(getFirstSum(0, 1), getSecondSum(0, 1));
			else
				return getAbsoluteFromComplex(0);
		}

		Image2DCPtr GetSingleImageFromSinglePhaseImage() const
		{
			if(_data.size() == 4)
			{
				if(_phaseRepresentation == PhasePart)
					return getSinglePhaseFromTwoPolPhase(0, 3);
				else
					return getFirstSum(0, 3);
			}
			if(_data.size() == 2)
			{
				if(_phaseRepresentation == PhasePart)
					return getSinglePhaseFromTwoPolPhase(0, 1);
				else
					return getFirstSum(0, 1);
			}
			else // if(_data.size() == 1)
				return _data[0]._images[0];
		}

		Image2DCPtr GetRealPartFromDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				default:
				case Polarization::XX: return _data[0]._images[0];
				case Polarization::XY: return _data[1]._images[0];
				case Polarization::YX: return _data[2]._images[0];
				case Polarization::YY: return _data[3]._images[0];
			}
		}

		Image2DCPtr GetRealPartFromAutoDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				default:
				case Polarization::XX: return _data[0]._images[0];
				case Polarization::YY: return _data[1]._images[0];
			}
		}

		Image2DCPtr GetRealPartFromCrossDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XY: return _data[0]._images[0];
				case Polarization::YX: return _data[1]._images[0];
				default: throw BadUsageException("Could not extract real part for given polarisation");
			}
		}

		Image2DCPtr GetImaginaryPartFromDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return _data[0]._images[1];
				case Polarization::XY: return _data[1]._images[1];
				case Polarization::YX: return _data[2]._images[1];
				case Polarization::YY: return _data[3]._images[1];
				default: throw BadUsageException("Could not extract imaginary part for given polarisation");
			}
		}

		Image2DCPtr GetImaginaryPartFromAutoDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return _data[0]._images[1];
				case Polarization::YY: return _data[1]._images[1];
				default: throw BadUsageException("Could not extract imaginary part for given polarisation");
			}
		}

		Image2DCPtr GetImaginaryPartFromCrossDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XY: return _data[0]._images[1];
				case Polarization::YX: return _data[1]._images[1];
				default: throw BadUsageException("Could not extract imaginary part for given polarisation");
			}
		}

		Image2DCPtr GetAmplitudePartFromDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return getAbsoluteFromComplex(0);
				case Polarization::XY: return getAbsoluteFromComplex(1);
				case Polarization::YX: return getAbsoluteFromComplex(2);
				case Polarization::YY: return getAbsoluteFromComplex(3);
				default: throw BadUsageException("Could not extract amplitude part for given polarisation");
			}
		}

		Image2DCPtr GetAmplitudePartFromAutoDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return getAbsoluteFromComplex(0);
				case Polarization::YY: return getAbsoluteFromComplex(1);
				default: throw BadUsageException("Could not extract amplitude part for given polarisation");
			}
		}

		Image2DCPtr GetAmplitudePartFromCrossDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XY: return getAbsoluteFromComplex(0);
				case Polarization::YX: return getAbsoluteFromComplex(1);
				default: throw BadUsageException("Could not extract amplitude part for given polarisation");
			}
		}

		Image2DCPtr GetPhasePartFromDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return getPhaseFromComplex(0);
				case Polarization::XY: return getPhaseFromComplex(1);
				case Polarization::YX: return getPhaseFromComplex(2);
				case Polarization::YY: return getPhaseFromComplex(3);
				default: throw BadUsageException("Could not extract phase part for given polarisation");
			}
		}

		Image2DCPtr GetPhasePartFromAutoDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XX: return getPhaseFromComplex(0);
				case Polarization::YY: return getPhaseFromComplex(1);
				default: throw BadUsageException("Could not extract phase part for given polarisation");
			}
		}

		Image2DCPtr GetPhasePartFromCrossDipole(PolarizationEnum polarisation) const
		{
			switch(polarisation)
			{
				case Polarization::XY: return getPhaseFromComplex(0);
				case Polarization::YX: return getPhaseFromComplex(1);
				default: throw BadUsageException("Could not extract phase part for given polarisation");
			}
		}

		void CopyFlaggingTo(TimeFrequencyData *data) const
		{
			if(MaskCount() == 0)
				data->SetNoMask();
			else if(MaskCount() == 1)
				data->SetGlobalMask(GetMask(0));
			else
			{
				if(_data.size() == data->_data.size())
				{
					for(size_t i=0; i!=_data.size(); ++i)
						data->_data[i]._flagging = _data[i]._flagging;
				} else
					throw BadUsageException("Trying to copy flagging from incompatible time frequency data");
			}
		}
		
		/*Image2DCPtr GetSingleAbsoluteFromComplexDipole() const
		{
			Image2DCPtr real = getFirstSum(0, 3);
			Image2DCPtr imag = getSecondSum(0, 3);
			return GetAbsoluteFromComplex(real, imag);
		}

		Image2DCPtr GetSingleAbsoluteFromComplexAutoDipole() const
		{
			Image2DCPtr real = getFirstSum(0, 1);
			Image2DCPtr imag = getSecondSum(0, 1);
			return GetAbsoluteFromComplex(real, imag);
		}

		Image2DCPtr GetSingleAbsoluteFromComplexCrossDipole() const
		{
			Image2DCPtr real = getFirstSum(0, 1);
			Image2DCPtr imag = getSecondSum(0, 1);
			return GetAbsoluteFromComplex(real, imag);
		}*/

		Image2DCPtr getAbsoluteFromComplex(size_t polIndex) const
		{
			return GetAbsoluteFromComplex(_data[polIndex]._images[0], _data[polIndex]._images[1]);
		}

		Image2DCPtr getPhaseFromComplex(size_t polIndex) const
		{
			return GetPhaseFromComplex(_data[polIndex]._images[0], _data[polIndex]._images[1]);
		}
		
		Image2DCPtr GetAbsoluteFromComplex(const Image2DCPtr &real, const Image2DCPtr &imag) const;
		Image2DCPtr GetPhaseFromComplex(const Image2DCPtr &real, const Image2DCPtr &imag) const;

		Image2DCPtr getFirstSum(size_t dataIndexA, size_t dataIndexB) const
		{
			if(dataIndexA >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(dataIndexB >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetSum(_data[dataIndexA]._images[0], _data[dataIndexB]._images[0]);
		}
		
		Image2DCPtr getSecondSum(size_t dataIndexA, size_t dataIndexB) const
		{
			if(dataIndexA >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(dataIndexB >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetSum(_data[dataIndexA]._images[1], _data[dataIndexB]._images[1]);
		}
		
		Image2DCPtr getFirstDiff(size_t dataIndexA, size_t dataIndexB) const
		{
			if(dataIndexA >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(dataIndexB >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetDifference(_data[dataIndexA]._images[0], _data[dataIndexB]._images[0]);
		}
		
		Image2DCPtr getSecondDiff(size_t dataIndexA, size_t dataIndexB) const
		{
			if(dataIndexA >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(dataIndexB >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetDifference(_data[dataIndexA]._images[1], _data[dataIndexB]._images[1]);
		}
		
		Image2DCPtr getNegRealPlusImag(size_t xyIndex, size_t yxIndex) const
		{
			if(xyIndex >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(yxIndex >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetNegatedSum(_data[xyIndex]._images[1], _data[yxIndex]._images[0]);
		}
		
		Image2DCPtr getRealMinusImag(size_t xyIndex, size_t yxIndex) const
		{
			if(xyIndex >= _data.size())
				throw BadUsageException("Polarisation not available");
			if(yxIndex >= _data.size())
				throw BadUsageException("Polarisation not available");
			return GetDifference(_data[xyIndex]._images[0], _data[yxIndex]._images[1]);
		}
		
		Image2DCPtr GetSum(const Image2DCPtr &left, const Image2DCPtr &right) const;
		Image2DCPtr GetNegatedSum(const Image2DCPtr &left, const Image2DCPtr &right) const;
		Image2DCPtr GetDifference(const Image2DCPtr &left, const Image2DCPtr &right) const;

		Image2DCPtr getSinglePhaseFromTwoPolPhase(size_t polA, size_t polB) const;
		Image2DCPtr GetZeroImage() const;
		template<bool InitValue>
		Mask2DCPtr GetSetMask() const
		{
			if(ImageWidth() == 0 || ImageHeight() == 0)
				throw BadUsageException("Can't make a mask without an image");
			return Mask2D::CreateSetMaskPtr<InitValue>(ImageWidth(), ImageHeight());
		}
		Mask2DCPtr GetCombinedMask() const;

		/**
		 * Returns the data index of the given polarisation, or _data.size() if
		 * not found.
		 */
		size_t getPolarisationIndex(PolarizationEnum polarisation) const
		{
			for(size_t i=0; i!=_data.size(); ++i)
				if(_data[i]._polarization == polarisation)
					return i;
			return _data.size();
		}

		struct PolarizedTimeFrequencyData
		{
			PolarizedTimeFrequencyData() :
				_images{nullptr, nullptr},
				_flagging(nullptr),
				_polarization(Polarization::StokesI)
			{ }
			PolarizedTimeFrequencyData(PolarizationEnum polarisation, const Image2DCPtr& image) :
				_images{image, nullptr},
				_flagging(nullptr),
				_polarization(polarisation)
			{ }
				
			PolarizedTimeFrequencyData(PolarizationEnum polarisation, const Image2DCPtr& imageA, const Image2DCPtr& imageB) :
				_images{imageA, imageB},
				_flagging(nullptr),
				_polarization(polarisation)
			{ }
				
			// Second image is only filled when phase representation = complex
			Image2DCPtr _images[2];
			Mask2DCPtr _flagging;
			PolarizationEnum _polarization;
		};
		
		TimeFrequencyData(enum PhaseRepresentation phase, const PolarizedTimeFrequencyData& source) :
		_phaseRepresentation(phase), _data(1, source)
		{
		}

		enum PhaseRepresentation _phaseRepresentation;
		
		std::vector<PolarizedTimeFrequencyData> _data;
};

#endif
