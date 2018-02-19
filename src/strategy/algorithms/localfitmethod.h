#ifndef LocalFitMethod_H
#define LocalFitMethod_H

#include <string>
#include <mutex>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "surfacefitmethod.h"

class LocalFitMethod final : public SurfaceFitMethod {
	public:
		enum Method { None, Average, GaussianWeightedAverage, FastGaussianWeightedAverage, Median, Minimum };
		LocalFitMethod();
		~LocalFitMethod();
		void SetToAverage(unsigned hSquareSize, unsigned vSquareSize)
		{
			ClearWeights();
			_hSquareSize = hSquareSize;
			_vSquareSize = vSquareSize;
			_method = Average;
		}
		void SetToWeightedAverage(unsigned hSquareSize, unsigned vSquareSize, long double hKernelSize, long double vKernelSize)
		{
			ClearWeights();
			_hSquareSize = hSquareSize;
			_vSquareSize = vSquareSize;
			_method = FastGaussianWeightedAverage;
			_hKernelSize = hKernelSize;
			_vKernelSize = vKernelSize;
		}
		void SetToMedianFilter(unsigned hSquareSize, unsigned vSquareSize)
		{
			ClearWeights();
			_hSquareSize = hSquareSize;
			_vSquareSize = vSquareSize;
			_method = Median;
		}
		void SetToMinimumFilter(unsigned hSquareSize, unsigned vSquareSize)
		{
			ClearWeights();
			_hSquareSize = hSquareSize;
			_vSquareSize = vSquareSize;
			_method = Minimum;
		}
		void SetToNone()
		{
			ClearWeights();
			_method = None;
		}
		void SetParameters(unsigned hSquareSize, unsigned vSquareSize, enum Method method)
		{
			ClearWeights();
			_hSquareSize = hSquareSize;
			_vSquareSize = vSquareSize;
			_method = method;
		}
		virtual void Initialize(const TimeFrequencyData &input) final override;
		//[[ deprecated("Trying to make surfacemethod go away") ]]
		unsigned TaskCount();
		virtual void PerformFit(unsigned taskNumber) final override;
		TimeFrequencyData Background() const { return _background; }
	private:
		struct ThreadLocal {
			LocalFitMethod *image;
			unsigned currentX, currentY;
			unsigned startX, startY, endX, endY;
			size_t emptyWindows;
		};
		long double CalculateBackgroundValue(unsigned x, unsigned y);
		long double FitBackground(unsigned x, unsigned y, ThreadLocal &local);
		long double CalculateAverage(unsigned x, unsigned y, ThreadLocal &local);
		long double CalculateMedian(unsigned x, unsigned y, ThreadLocal &local);
		long double CalculateMinimum(unsigned x, unsigned y, ThreadLocal &local);
		long double CalculateWeightedAverage(unsigned x, unsigned y, ThreadLocal &local);
		void ClearWeights();
		void InitializeGaussianWeights();
		void PerformGaussianConvolution(Image2DPtr input);
		void CalculateWeightedAverageFast();
		Image2DPtr CreateFlagWeightsMatrix();
		void ElementWiseDivide(Image2DPtr leftHand, Image2DCPtr rightHand);

		Image2DCPtr _original;
		TimeFrequencyData _background;
		Image2DPtr _background2D;
		Mask2DCPtr _mask;
		unsigned _hSquareSize, _vSquareSize;
		num_t **_weights;
		long double _hKernelSize, _vKernelSize;
		enum Method _method;
};

#endif
