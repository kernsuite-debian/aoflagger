#ifndef SIROPERATOR_H
#define SIROPERATOR_H

#include "../../structures/mask2d.h"
#include "../../structures/types.h"
#include "../../structures/xyswappedmask2d.h"

/**
 * This class contains functions that implement an algorithm to dilate
 * a flag mask: the "scale-invariant rank (SIR) operator".
 * The amount of dilation is relative to the size of the flagged
 * areas in the input, hence it is scale invariant. This behaviour is very
 * effective for application after amplitude based RFI detection and is a step
 * in the default LOFAR flagging pipeline.
 * 
 * The rule for this scale invariant dilation is as follows:
 * Consider the sequence w(y) of size N, where w(y) = 0 if sample y is flagged and w(y) = 1
 * otherwise. If there exists a subsequence within w that
 * includes y and that has a flagged ratio of η or more, y will be flagged.
 * 
 * Thus:
 * if Y1 and Y2 exists, such that \\sum_{y=Y1}^{Y2-1} w(y)  <  η (Y2 - Y1), flag y.
 * 
 * The algorithm will be applied both in time and in frequency direction, thus w(y) can
 * contain a slice through the time-frequency image in either directions.
 * 
 * The algorithm is described in Offringa, van de Gronde and Roerdink 2012 (A&A).
 * 
 * @author A.R. Offringa
 */
class SIROperator
{
public:
	/**
		* This is the proof of concept, reference version of the O(N) algorithm. It is
		* fast, but OperateHorizontally() and OperateVertically() have been optimized
		* for operating on a mask directly, which is the common mode of operation.
		* 
		* It contains extra comments to explain the algorithm within the code.
		* 
		* @param [in,out] flags The input array of flags to be dilated that will be overwritten
		* by the dilatation of itself.
		* @param [in] flagsSize Size of the @c flags array.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have (see class description for the definition).
		*/
	static void Operate(bool* flags, const unsigned flagsSize, num_t eta)
	{
		// The test for a sample to become flagged can be rewritten as
		//         \\sum_{y=Y1}^{Y2-1} ( η - w(y) ) >= 0.
		// With w(y) =      flags[y] : 0
		//                 !flags[y] : 1
		
		// Make an array in which flagged samples are η and unflagged samples are η-1,
		// such that we can test for \\sum_{y=Y1}^{Y2-1} values[y] >= 0
		std::unique_ptr<num_t[]> values(new num_t[flagsSize]);
		for(unsigned i=0 ; i<flagsSize ; ++i)
		{
			if(flags[i])
				values[i] = eta;
			else
				values[i] = eta - 1.0;
		}
		
		// For each x, we will now search for the largest sum of sequantial values that contains x.
		// If this sum is larger then 0, this value is part of a sequence that exceeds the test.
		
		// Define W(x) = \\sum_{y=0}^{x-1} values[y], such that the largest sequence containing x
		// starts at the element after W(y) is minimal in the range 0 <= y <= x, and ends when
		// W(y) is maximal in the range x < y < N.
		
		// Calculate these W's and minimum prefixes
		const unsigned wSize = flagsSize+1;
		std::unique_ptr<num_t[]> w(new num_t[wSize]);
		w[0] = 0.0;
		unsigned currentMinIndex = 0;
		std::unique_ptr<unsigned[]> minIndices(new unsigned[wSize]);
		minIndices[0] = 0;
		for(unsigned i=1 ; i!=wSize ; ++i)
		{
			w[i] = w[i-1] + values[i-1];

			if(w[i] < w[currentMinIndex])
			{
				currentMinIndex = i;
			}
			minIndices[i] = currentMinIndex;
		}
		
		// Calculate the maximum suffixes
		unsigned currentMaxIndex = wSize-1;
		std::unique_ptr<unsigned[]> maxIndices(new unsigned[wSize]);
		for(unsigned i=flagsSize-1 ; i!=0 ; --i)
		{
			// We directly assign maxIndices[i] to the max index over
			// all indices *higher* than i, since maxIndices[i] is
			// not allowed to be i (maxIndices[i] = max i: x < i < N).
			maxIndices[i] = currentMaxIndex;
			
			if(w[i] > w[currentMaxIndex])
			{
				currentMaxIndex = i;
			}
		}
		maxIndices[0] = currentMaxIndex;
		
		// See if max sequence exceeds limit.
		for(unsigned i=0 ; i!=flagsSize ; ++i )
		{
			const num_t maxW = w[maxIndices[i]] - w[minIndices[i]];
			flags[i] = (maxW >= 0.0);
		}
	}
	
	/**
		* Performs a horizontal dilation directly on a mask. Algorithm is equal to Operate().
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	static void OperateHorizontally(Mask2D& mask, num_t eta)
	{
		operateHorizontally<Mask2D>(mask, eta);
	}
	
	
	/**
		* Performs a horizontal dilation directly on a mask, with missing value.
		* Missing values are values for which it is not know they should have
		* been flagged. An example is when the correlator has flagged
		* samples: these should be excluded from RFI detection, but they might
		* or might not contain RFI. The way this is implemented is by concatenating
		* all non-missing samples in a row, and performing the algorithm on that.
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] missing Flag mask that identifies missing values.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	static void OperateHorizontallyMissing(Mask2D& mask, const Mask2D& missing, num_t eta)
	{
		operateHorizontallyMissing<Mask2D>(mask, missing, eta);
	}
	
	/**
		* Performs a vertical dilation directly on a mask. Algorithm is equal to Operate().
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	static void OperateVertically(Mask2D& mask, num_t eta)
	{
		XYSwappedMask2D<Mask2D> swappedMask(mask);
		operateHorizontally<XYSwappedMask2D<Mask2D>>(swappedMask, eta);
	}
	
	/**
		* Performs a vertical dilation directly on a mask, with missing value.
		* Identical to @ref OperateHorizontallyMissing(), but then vertically.
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] missing Flag mask that identifies missing values.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	static void OperateVerticallyMissing(Mask2D& mask, const Mask2D& missing, num_t eta)
	{
		XYSwappedMask2D<Mask2D> swappedMask(mask);
		XYSwappedMask2D<const Mask2D> swappedMissing(missing);
		operateHorizontallyMissing(swappedMask, swappedMissing, eta);
	}
private:
	SIROperator() = delete;

	/**
		* Performs a horizontal dilation directly on a mask. Algorithm is equal to Operate().
		* This is the implementation.
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	template<typename MaskLike>
	static void operateHorizontally(MaskLike &mask, num_t eta)
	{
		const unsigned
			width = mask.Width(),
			wSize = width+1;
		std::unique_ptr<num_t[]>
			values(new num_t[width]),
			w(new num_t[wSize]);
		std::unique_ptr<unsigned[]>
			minIndices(new unsigned[wSize]),
			maxIndices(new unsigned[wSize]);
		
		for(unsigned row=0;row<mask.Height();++row)
		{
			for(unsigned i=0 ; i<width ; ++i)
			{
				if(mask.Value(i, row))
					values[i] = eta;
				else
					values[i] = eta - 1.0;
			}
			
			w[0] = 0.0;
			unsigned currentMinIndex = 0;
			minIndices[0] = 0;
			for(unsigned i=1 ; i!=wSize ; ++i)
			{
				w[i] = w[i-1] + values[i-1];

				if(w[i] < w[currentMinIndex])
				{
					currentMinIndex = i;
				}
				minIndices[i] = currentMinIndex;
			}
			
			// Calculate the maximum suffixes
			unsigned currentMaxIndex = wSize-1;
			for(unsigned i=width-1 ; i!=0 ; --i)
			{
				maxIndices[i] = currentMaxIndex;
				if(w[i] > w[currentMaxIndex])
				{
					currentMaxIndex = i;
				}
			}
			maxIndices[0] = currentMaxIndex;
			
			// See if max sequence exceeds limit.
			for(unsigned i=0 ; i!=width ; ++i )
			{
				const num_t maxW = w[maxIndices[i]] - w[minIndices[i]];
				mask.SetValue(i, row, (maxW >= 0.0));
			}
		}
	}
	
	/**
		* Performs a horizontal dilation directly on a mask with missing values.
		* This is the implementation.
		* 
		* @param [in,out] mask The input flag mask to be dilated.
		* @param [in] missing Flag mask that identifies missing values.
		* @param [in] eta The η parameter that specifies the minimum number of good data
		* that any subsequence should have.
		*/
	template<typename MaskLikeA, typename MaskLikeB>
	static void operateHorizontallyMissing(MaskLikeA& mask, const MaskLikeB& missing, num_t eta)
	{
		const unsigned
			width = mask.Width(),
			maxWSize = width+1;
		std::unique_ptr<num_t[]>
			values(new num_t[width]),
			w(new num_t[maxWSize]);
		std::unique_ptr<unsigned[]>
			minIndices(new unsigned[maxWSize]),
			maxIndices(new unsigned[maxWSize]);
		
		for(unsigned row=0;row<mask.Height();++row)
		{
			unsigned nAvailable = 0;
			for(unsigned i=0 ; i<width ; ++i)
			{
				if(!missing.Value(i, row))
				{
					if(mask.Value(i, row))
						values[nAvailable] = eta;
					else
						values[nAvailable] = eta - 1.0;
					++nAvailable;
				}
			}
			
			if(nAvailable != 0)
			{
				unsigned wSize = nAvailable + 1;
				w[0] = 0.0;
				unsigned currentMinIndex = 0;
				minIndices[0] = 0;
				for(unsigned i=1 ; i!=wSize ; ++i)
				{
					w[i] = w[i-1] + values[i-1];

					if(w[i] < w[currentMinIndex])
					{
						currentMinIndex = i;
					}
					minIndices[i] = currentMinIndex;
				}
				
				// Calculate the maximum suffixes
				unsigned currentMaxIndex = wSize-1;
				for(unsigned i=nAvailable-1 ; i!=0 ; --i)
				{
					maxIndices[i] = currentMaxIndex;
					if(w[i] > w[currentMaxIndex])
					{
						currentMaxIndex = i;
					}
				}
				maxIndices[0] = currentMaxIndex;
				
				// See if max sequence exceeds limit.
				nAvailable = 0;
				for(unsigned i=0 ; i!=width ; ++i )
				{
					if(!missing.Value(i, row))
					{
						const num_t maxW = w[maxIndices[nAvailable]] - w[minIndices[nAvailable]];
						mask.SetValue(i, row, (maxW >= 0.0));
						++nAvailable;
					}
				}
			}
		}
	}
};

#endif

