#ifndef SAMPLEROW_H
#define SAMPLEROW_H

#include <algorithm>
#include <limits>
#include <cmath>
#include <memory>
#include <vector>

#include "image2d.h"
#include "mask2d.h"

#include "../strategy/algorithms/convolutions.h"

class SampleRow {
	public:
		explicit SampleRow(size_t size) :
			_values(size)
		{ }
		
		static SampleRow MakeEmpty(size_t size)
		{
			return SampleRow(size);
		}
		
		static SampleRow MakeZero(size_t size) {
			SampleRow row(size);
			std::fill(row._values.begin(), row._values.end(), 0.0);
			return row;
		}
		
		static SampleRow MakeFromRow(const Image2D* image, size_t y)
		{
			SampleRow row(image->Width());
			std::copy(image->ValuePtr(0, y), image->ValuePtr(0, y) + image->Width(), row._values.data());
			return row;
		}
		
		static SampleRow MakeFromRow(const Image2D* image, size_t xStart, size_t length, size_t y)
		{
			SampleRow row(length);
			std::copy(image->ValuePtr(xStart, y), image->ValuePtr(xStart, y) + length, row._values.data());
			return row;
		}
		
		static SampleRow MakeFromRowWithMissings(const Image2D* image, const Mask2D* mask, size_t y)
		{
			SampleRow row(image->Width());
			for(size_t x=0;x<image->Width();++x)
			{
				if(mask->Value(x, y))
					row._values[x] = std::numeric_limits<num_t>::quiet_NaN();
				else
					row._values[x] = image->Value(x, y);
			}
			return row;
		}
		
		static SampleRow MakeAmplitudeFromRow(const Image2D* real, const Image2D* imaginary, size_t y)
		{
			SampleRow row(real->Width());
			for(size_t x=0;x<real->Width();++x)
			{
				row._values[x] = sqrtn(real->Value(x,y)*real->Value(x,y) + imaginary->Value(x,y)*imaginary->Value(x,y));
			}
			return row;
		}
		
		static SampleRow MakeFromColumn(const Image2D* image, size_t x)
		{
			SampleRow row(image->Height());
			for(size_t y=0;y<image->Height();++y)
				row._values[y] = image->Value(x, y);
			return row;
		}
		
		static SampleRow MakeFromColumnWithMissings(const Image2D* image, const Mask2D* mask, size_t x)
		{
			SampleRow row(image->Height());
			for(size_t y=0;y<image->Height();++y)
			{
				if(mask->Value(x, y))
					row._values[y] = std::numeric_limits<num_t>::quiet_NaN();
				else
					row._values[y] = image->Value(x, y);
			}
			return row;
		}
		
		static SampleRow MakeFromRowSum(const Image2D* image, size_t yStart, size_t yEnd)
		{
			if(yEnd > yStart) {
				SampleRow row = MakeFromRow(image, yStart);
	
				for(size_t y=yStart+1;y<yEnd;++y) {
					for(size_t x=0;x<image->Width();++x)
						row._values[x] += image->Value(x, y);
				}
				return row;
			} else {
				return MakeZero(image->Width());
			}
		}
		
		static SampleRow MakeFromColumnSum(const Image2D* image, size_t xStart, size_t xEnd)
		{
			if(xEnd > xStart) {
				SampleRow row = MakeFromColumn(image, xStart);
	
				for(size_t x=xStart+1;x<xEnd;++x) {
					for(size_t y=0;y<image->Width();++y)
						row._values[y] += image->Value(x, y);
				}
				return row;
			} else {
				return MakeZero(image->Width());
			}
		}
		
		void SetHorizontalImageValues(Image2D* image, unsigned y) const noexcept
		{
			for(size_t i=0;i<_values.size();++i)
			{
				image->SetValue(i, y, _values[i]);
			}
		}
		
		void SetHorizontalImageValues(Image2D* image, unsigned xStart, unsigned y) const noexcept
		{
			for(size_t i=0;i<_values.size();++i)
			{
				image->SetValue(i+xStart, y, _values[i]);
			}
		}
		
		void SetVerticalImageValues(Image2D* image, unsigned x) const noexcept
		{
			for(size_t i=0;i<_values.size();++i)
			{
				image->SetValue(x, i, _values[i]);
			}
		}
		
		void SetVerticalImageValues(Image2D* image, unsigned x, unsigned yStart) const noexcept
		{
			for(size_t i=0;i<_values.size();++i)
			{
				image->SetValue(x, i+yStart, _values[i]);
			}
		}
		
		num_t Value(size_t i) const noexcept { return _values[i]; }
		
		void SetValue(size_t i, num_t newValue) noexcept { _values[i] = newValue; }

		size_t Size() const noexcept { return _values.size(); }
		
		size_t IndexOfMax() const noexcept
		{
			size_t maxIndex = 0;
			num_t maxValue = _values[0];
			for(size_t i = 1; i<_values.size();++i)
			{
				if(_values[i] > maxValue)
				{
					maxIndex = i;
					maxValue = _values[i];
				}
			}
			return maxIndex;
		}

		numl_t RMS() const noexcept
		{
			if(_values.empty()) return std::numeric_limits<numl_t>::quiet_NaN();
			numl_t sum = 0.0;
			for(num_t v : _values)
				sum += v * v;
			return sqrtnl(sum / _values.size());
		}
		
		num_t Median() const noexcept
		{
			if(_values.empty()) return std::numeric_limits<num_t>::quiet_NaN();

			std::vector<num_t> copy(_values);
			if(_values.size() % 2 == 0)
			{
				size_t
					m = _values.size() / 2 - 1;
				std::nth_element(copy.begin(), copy.begin() + m, copy.end());
				num_t leftMid = *(copy.begin() + m);
				std::nth_element(copy.begin(), copy.begin() + m + 1, copy.end());
				num_t rightMid = *(copy.begin() + m + 1);
				return (leftMid + rightMid) / 2;
			} else {
				size_t
					m = _values.size() / 2;
				std::nth_element(copy.begin(), copy.begin() + m, copy.end());
				num_t mid = *(copy.begin() + m);
				return mid;
			}
		}
		
		numl_t Mean() const noexcept
		{
			numl_t mean = 0.0;
			for(num_t v : _values)
				mean += v;
			return mean / _values.size();
		}
		
		numl_t MeanWithMissings() const noexcept
		{
			numl_t mean = 0.0;
			size_t count = 0;
			for(num_t v : _values)
			{
				if(std::isfinite(v))
				{
					mean += v;
					++count;
				}
			}
			return mean / count;
		}
		
		numl_t StdDev(double mean) const noexcept
		{
			numl_t stddev = 0.0;
			for(num_t v : _values)
				stddev += (v - mean) * (v - mean);
			return sqrtnl(stddev / _values.size());
		}
		
		num_t RMSWithMissings() const
		{
			return MakeWithoutMissings().RMS();
		}
		
		num_t MedianWithMissings() const
		{
			return MakeWithoutMissings().Median();
		}
		
		num_t StdDevWithMissings(double mean) const
		{
			return MakeWithoutMissings().StdDev(mean);
		}
		
		SampleRow MakeWithoutMissings() const;
		
		bool ValueIsMissing(size_t i) const
		{
			return !std::isfinite(Value(i));
		}
		
		void SetValueMissing(size_t i)
		{
			SetValue(i, std::numeric_limits<num_t>::quiet_NaN());
		}
		
		void ConvolveWithGaussian(num_t sigma)
		{
			Convolutions::OneDimensionalGausConvolution(_values.data(), _values.size(), sigma);
		}
		
		void ConvolveWithSinc(num_t frequency)
		{
			Convolutions::OneDimensionalSincConvolution(_values.data(), _values.size(), frequency);
		}
		
		SampleRow* operator-=(const SampleRow& source) noexcept
		{
			for(unsigned i=0;i<_values.size();++i)
				_values[i] -= source._values[i];
			return this;
		}
		
		num_t WinsorizedMean() const
		{
			std::vector<num_t> data(_values);
			std::sort(data.begin(), data.end(), numLessThanOperator);
			size_t lowIndex = (size_t) floor(0.1 * data.size());
			size_t highIndex = (size_t) ceil(0.9 * data.size())-1;
			num_t lowValue = data[lowIndex];
			num_t highValue = data[highIndex];

			// Calculate mean
			num_t mean = 0.0;
			for(size_t x = 0;x < data.size(); ++x) {
				const num_t value = data[x];
				if(value < lowValue)
					mean += lowValue;
				else if(value > highValue)
					mean += highValue;
				else
					mean += value;
			}
			return mean / (num_t) data.size();
		}
		
		num_t WinsorizedMeanWithMissings() const
		{
			return MakeWithoutMissings().WinsorizedMean();
		}
		
	private:
		std::vector<num_t> _values;
		
		// We need this less than operator, because the normal operator
		// does not enforce a strictly ordered set, because a<b != !(b<a) in the case
		// of nans/infs.
		static bool numLessThanOperator(const num_t& a, const num_t& b) noexcept {
			if(std::isfinite(a)) {
				if(std::isfinite(b))
					return a < b;
				else
					return true;
			}
			return false;
		}
};

#endif
