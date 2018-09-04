#ifndef MORPHOLOGICAL_FLAGGER_H
#define MORPHOLOGICAL_FLAGGER_H

#include <string>

#include "../../structures/mask2d.h"

class MorphologicalFlagger {
	public:
		static inline bool SquareContainsFlag(const Mask2D* mask, size_t xLeft, size_t yTop, size_t xRight, size_t yBottom);
		static void DilateFlags(Mask2D* mask, size_t timeSize, size_t frequencySize)
		{
			DilateFlagsHorizontally(mask, timeSize);
			DilateFlagsVertically(mask, frequencySize);
		}
		static void DilateFlagsHorizontally(Mask2D* mask, size_t timeSize);
		static void DilateFlagsVertically(Mask2D* mask, size_t frequencySize);
		static void LineRemover(Mask2D* mask, size_t maxTimeContamination, size_t maxFreqContamination);
		static void DensityTimeFlagger(Mask2D* mask, num_t minimumGoodDataRatio);
		static void DensityFrequencyFlagger(Mask2D* mask, num_t minimumGoodDataRatio);
		
	private:
		static void FlagTime(Mask2D* mask, size_t x);
		static void FlagFrequency(Mask2D* mask, size_t y);
		static void MaskToInts(const Mask2D* mask, int **maskAsInt);
		static void SumToLeft(const Mask2D* mask, int **sums, size_t width, size_t step, bool reverse);
		static void SumToTop(const Mask2D* mask, int **sums, size_t width, size_t step, bool reverse);
		static void ThresholdTime(const Mask2D* mask, int **flagMarks, int **sums, int thresholdLevel, int width);
		static void ThresholdFrequency(const Mask2D* mask, int **flagMarks, int **sums, int thresholdLevel, int width);
		static void ApplyMarksInTime(Mask2D* mask, int **flagMarks);
		static void ApplyMarksInFrequency(Mask2D* mask, int **flagMarks);
};

#endif
