#ifndef XYSWAPPEDMASK2D_H
#define XYSWAPPEDMASK2D_H

#include "mask2d.h"
#include "image2d.h"

/**
 * This class wraps a mask and swappes the x and y axes. It provides only the
 * trivial @ref Mask2D functions, and swappes x and y in there, such that the original
 * width becomes the new height, etc. It is useful to convert an algorithm
 * that works originally only in one direction to work in the other direction
 * without rewriting it. If a template parameter is used, the overhead should
 * be negligable.
 * 
 * Note that this method uses references to the original mask. However, masks
 * are normally wrapped in a smart pointer. The caller should make sure the
 * mask exists as long as the XYSwappedMask2D exists.
 * 
 * @author Andre Offringa
 */
template<typename MaskLike>
class XYSwappedMask2D
{
	public:
		explicit XYSwappedMask2D(MaskLike &mask) : _mask(mask)
		{
		}
		
		XYSwappedMask2D<MaskLike>& operator=(const XYSwappedMask2D<MaskLike>& source)
		{
			_mask = source._mask;
			return *this;
		}
		
		bool Value(unsigned x, unsigned y) const
		{
			return _mask.Value(y, x);
		}
		
		void SetValue(unsigned x, unsigned y, bool newValue)
		{
			_mask.SetValue(y, x, newValue);
		}
		
		void SetHorizontalValues(unsigned x, unsigned y, bool value, unsigned count)
		{
			_mask.SetVerticalValues(y, x, value, count);
		}
		
		unsigned Width() const
		{
			return _mask.Height();
		}
		
		unsigned Height() const
		{
			return _mask.Width();
		}
		
	private:
		MaskLike &_mask;
};

template<typename ImageLike>
class XYSwappedImage2D
{
	public:
		explicit XYSwappedImage2D(ImageLike &image) : _image(image)
		{
		}
		
		inline num_t Value(unsigned x, unsigned y) const
		{
			return _image.Value(y, x);
		}
		
		inline void SetValue(unsigned x, unsigned y, num_t newValue)
		{
			_image.SetValue(y, x, newValue);
		}
		
		inline unsigned Width() const
		{
			return _image.Height();
		}
		
		inline unsigned Height() const
		{
			return _image.Width();
		}
		
	private:
		ImageLike &_image;
};

#endif // XYSWAPPEDMASK2D_H
