/** @file
 * This is the header file for the Image2D class.
 * @author André Offringa <offringa@gmail.com>
 */
#ifndef IMAGE2D_H
#define IMAGE2D_H

#include "../baseexception.h"
#include "types.h"

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <memory>

#include <exception>
#include <cmath>

typedef boost::intrusive_ptr<class Image2D> Image2DPtr;
typedef boost::intrusive_ptr<const class Image2D> Image2DCPtr;

void swap(Image2D& left, Image2D& right);
void swap(Image2D& left, Image2D&& right);
void swap(Image2D&& left, Image2D& right);

/**
 * This class represents a two dimensional single-valued (=gray scale) image. It can be
 * read from and written to a @c .fits file and written to a @c .png file. A new Image2D can
 * be constructed with e.g. the CreateFromFits(), CreateUnsetImage() or CreateFromDiff() static methods.
 */
class Image2D : public boost::intrusive_ref_counter<Image2D> {
	public:
		Image2D() noexcept;
		
		Image2D(const Image2D& source);
		
		Image2D(Image2D&& source) noexcept;
		
		Image2D& operator=(const Image2D& rhs);
		
		Image2D& operator=(Image2D&& rhs) noexcept;
		
		template<typename... Args>
		static Image2DPtr MakePtr(Args&&... args)
		{
			// This function is to have a generic 'make_<ptr>' function, that e.g. calls
			// the more efficient make_shared() when Image2DPtr is a shared_ptr, but
			// also works when Image2DPtr is a boost::intrusive_ptr.
			return Image2DPtr(new Image2D(std::forward<Args>(args)...));
		}
		
		/**
		 * Destructor.
		 */
		~Image2D() noexcept;
		
		static Image2D MakeUnsetImage(size_t width, size_t height)
		{
			return Image2D(width, height);
		}
		
		static Image2D MakeUnsetImage(size_t width, size_t height, size_t widthCapacity)
		{
			return Image2D(width, height, widthCapacity);
		}
		
		static Image2D MakeSetImage(size_t width, size_t height, num_t initialValue)
		{
			Image2D image(width, height);
			image.SetAll(initialValue);
			return image;
		}
		
		static Image2D MakeSetImage(size_t width, size_t height, num_t initialValue, size_t widthCapacity)
		{
			Image2D image(width, height, widthCapacity);
			image.SetAll(initialValue);
			return image;
		}
		
		/**
		 * Creates an image containing unset values.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @return The new image. Should be deleted by the caller.
		 */
		static Image2D *CreateUnsetImage(size_t width, size_t height)
		{
			return new Image2D(width, height);
		}
		
		/**
		 * Creates an image containing unset values.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @param widthCapacity Minimum capacity to which the image can be horizontally
		 * resized without reallocation
		 * @return The new image. Should be deleted by the caller.
		 */
		static Image2D *CreateUnsetImage(size_t width, size_t height, size_t widthCapacity)
		{
			return new Image2D(width, height, widthCapacity);
		}
		
		/**
		 * As CreateUnsetImage(size_t,size_t), but returns a smart pointer instead.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @return A (unique) smart pointer to the new image.
		 */
		static Image2DPtr CreateUnsetImagePtr(size_t width, size_t height)
		{
			return Image2DPtr(CreateUnsetImage(width, height));
		}
		
		/**
		 * As CreateUnsetImage(size_t,size_t,size_t), but returns a smart pointer instead.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @param widthCapacity Minimum capacity to which the image can be horizontally
		 * resized without reallocation
		 * @return A (unique) smart pointer to the new image.
		 */
		static Image2DPtr CreateUnsetImagePtr(size_t width, size_t height, size_t widthCapacity)
		{
			return Image2DPtr(CreateUnsetImage(width, height));
		}
		
		static Image2D *CreateSetImage(size_t width, size_t height, num_t initialValue);
		
		static Image2D *CreateSetImage(size_t width, size_t height, num_t initialValue, size_t widthCapacity);
		
		static Image2DPtr CreateSetImagePtr(size_t width, size_t height, num_t initialValue)
		{
			return Image2DPtr(CreateSetImage(width, height, initialValue));
		}

		static Image2DPtr CreateSetImagePtr(size_t width, size_t height, num_t initialValue, size_t widthCapacity)
		{
			return Image2DPtr(CreateSetImage(width, height, initialValue, widthCapacity));
		}

		/**
		 * Creates an image containing zeros.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @return The new created image.
		 */
		static Image2D MakeZeroImage(size_t width, size_t height)
		{
			return MakeSetImage(width, height, 0.0);
		}
		
		/**
		 * As CreateZeroImage(), but returns a smart pointer instead.
		 * @param width Width of the new image.
		 * @param height Height of the new image.
		 * @return The (unique) smart pointer to the new image.
		 */
		static Image2DPtr CreateZeroImagePtr(size_t width, size_t height)
		{
			// TODO make this more efficient & use make_shared
			return Image2DPtr(new Image2D(MakeZeroImage(width, height)));
		}

		bool Empty() const { return _width==0 || _height==0; }
		
		Image2D& operator+=(const Image2D& rhs);
		
		/**
		 * Creates a new image by adding two images of the same size together.
		 * @param imageA first image.
		 * @param imageB second image.
		 * @return The new created image.
		 * @throws IOException if the images do not match in size.
		 */
		static Image2D MakeFromSum(const Image2D &imageA, const Image2D &imageB);
		
		/**
		 * Creates a new image by subtracting two images of the same size.
		 * @param imageA first image.
		 * @param imageB second image.
		 * @return The new created image. Should be deleted by the caller.
		 * @throws IOException if the images do not match in size.
		 */
		static Image2D MakeFromDiff(const Image2D &imageA, const Image2D &imageB);

		/**
		 * Retrieves the average value of the image.
		 * @return The average value.
		 */
		num_t GetAverage() const;
		
		/**
		 * Returns the maximum value in the image.
		 * @return The maximimum value.
		 */
		num_t GetMaximum() const;
		
		/**
		 * Returns the maximum value in the specified range.
		 * @return The maximimum value.
		 */
		num_t GetMaximum(size_t xOffset, size_t yOffset, size_t width, size_t height) const;

		/**
		 * Returns the minimum value in the image.
		 * @return The minimum value.
		 */
		num_t GetMinimum() const;
		
		/**
		 * Returns the minimum value in the specified range.
		 * @return The minimum value.
		 */
		num_t GetMinimum(size_t xOffset, size_t yOffset, size_t width, size_t height) const;

		/**
		 * Returns the maximum finite value in the image.
		 * @return The maximimum value.
		 */
		num_t GetMaximumFinite() const;
		
		/**
		 * Returns the minimum finite value in the image.
		 * @return The minimum value.
		 */
		num_t GetMinimumFinite() const;
		
		/**
		 * Retrieves the value at a specific position.
		 * @param x x-coordinate
		 * @param y y-coordinate
		 * @return The value.
		 */
		num_t Value(size_t x, size_t y) const { return _dataPtr[y][x]; }
		
		/**
		 * Get the width of the image.
		 * @return Width of the image.
		 */
		size_t Width() const { return _width; }
		
		/**
		 * Get the height of the image.
		 * @return Height of the image.
		 */
		size_t Height() const { return _height; }
		
		/**
		 * Change a value at a specific position.
		 * @param x x-coordinate of value to change.
		 * @param y y-coordinate of value to change.
		 * @param newValue New value.
		 */
		void SetValue(size_t x, size_t y, num_t newValue)
		{
			_dataPtr[y][x] = newValue;
		}

		void SetAll(num_t value);
		
		void AddValue(size_t x, size_t y, num_t addValue)
		{
			_dataPtr[y][x] += addValue;
		}
		
		/**
		 * Check whether this image is completely zero.
		 * @return @c true if the value only contains zeros.
		 */
		bool ContainsOnlyZeros() const;
		
		/**
		 * Compute the sum of all values
		 */
		num_t Sum() const
		{
			num_t sum = 0.0;
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					sum += Value(x, y);
			}
			return sum;
		}
		
		void SetToAbs()
		{
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					SetValue(x, y,  fabsn(Value(x, y)));
			}
		}
		
		/**
		 * Retrieve a factor to multiply the values with to normalise them.
		 * @return Normalisation factor.
		 */
		num_t GetMaxMinNormalizationFactor() const;

		num_t GetStdDev() const;

		num_t GetRMS() const
		{
			return GetRMS(0, 0, _width, _height);
		}

		num_t GetMode() const;
		
		num_t GetRMS(size_t xOffset, size_t yOffset, size_t width, size_t height) const;

		/**
		 * Normalize the data so that the variance is 1.
		 */
		void NormalizeVariance();

		/**
		* Create a new image instance by reading a fitsfile.
		* @param file The fits file.
		* @param imageNumber The number of the image.
		* @return The new created image. Should be deleted by the caller.
		* @throws FitsIOException if something goes wrong during reading the .fits file.
		*/
		static Image2D MakeFromFits(class FitsFile &file, int imageNumber);

		/**
		* Number of images that can be read from the current HUD block
		* in the fits file.
		* @param file Fits file.
		* @return Number of images.
		*/
		static long GetImageCountInHUD(class FitsFile &file);

		/**
		* Save the image to a fits file.
		* @param filename Fits filename.
		* @throws IOException if something goes wrong during writing
		*/
		void SaveToFitsFile(const std::string &filename) const;

		/**
		 * Count the number of values that are above a specified value.
		 */
		size_t GetCountAbove(num_t value) const;
		size_t GetCountBelowOrEqual(num_t value) const
		{
			return _width*_height - GetCountAbove(value);
		}

		/**
		 * Returns a threshold for which @c count values are above the
		 * the threshold. That is, GetCountAbove(GetTresholdForCountAbove(x)) = x.
		 */
		num_t GetTresholdForCountAbove(size_t count) const;

		/**
		 * Copies all values to the specified array. The array should be of size width*height.
		 */
		void CopyData(num_t *destination) const;

		/**
		 * Multiply all values with a factor.
		 */
		void MultiplyValues(num_t factor);
		
		/**
		 * Will set all values to lhs - this.
		 */
		void SubtractAsRHS(const Image2DCPtr &lhs);

		/**
		 * Flips the image round the diagonal, i.e., x becomes y and y becomes x.
		 */
		Image2D CreateXYFlipped() const
		{
			Image2D image(_height, _width);
			for(unsigned y=0;y<_height;++y)
			{
				for(unsigned x=0;x<_width;++x)
					image._dataPtr[x][y] = _dataPtr[y][x];
			}
			return image;
		}
		
		void SwapXY()
		{
			*this = CreateXYFlipped();
		}
		
		/**
		 * Resample the image horizontally by decreasing the width
		 * with an integer factor.
		 */
		Image2D ShrinkHorizontally(size_t factor) const;

		/**
		 * Resample the image vertically by decreasing the height
		 * with an integer factor.
		 */
		Image2D ShrinkVertically(size_t factor) const;

		/**
		 * Resample the image horizontally by increasing the width
		 * with an integer factor.
		 */
		Image2D EnlargeHorizontally(size_t factor, size_t newWidth) const;

		/**
		 * Resample the image vertically by increasing the width
		 * with an integer factor.
		 */
		Image2D EnlargeVertically(size_t factor, size_t newHeight) const;

		Image2D Trim(size_t startX, size_t startY, size_t endX, size_t endY) const;
		
		void SetTrim(size_t startX, size_t startY, size_t endX, size_t endY);
		
		/**
		 * Copies source onto this image at the given position
		 */
		void CopyFrom(const Image2D& source, size_t destX, size_t destY)
		{
			size_t
				x2 = source._width + destX,
				y2 = source._height + destY;
			if(x2 > _width) x2 = _width;
			if(y2 > _height) y2 = _height;
			for(size_t y=destY;y<y2;++y)
			{
				for(size_t x=destX;x<x2;++x)
					SetValue(x, y, source.Value(x-destX, y-destY));
			}
		}
		
		/**
		 * Returns a pointer to one row of data. This can be used to step
		 * quickly over the data in x direction. Note that the next row
		 * is not exactly at "one times width", because the number of
		 * samples in a row is made divisable by four. This makes it
		 * possible to execute SSE instruction easily.
		 * 
		 * If you want to skip over a whole row, use the Stride() method
		 * to determine the intrinsicly used width of one row.
		 * 
		 * @see Stride()
		 */
		num_t *ValuePtr(unsigned x, unsigned y)
		{
			return &_dataPtr[y][x];
		}
		
		/**
		 * Returns a constant pointer to one row of data. This can be used to
		 * step quickly over the data in x direction. Note that the next row
		 * is not exactly at "one times width", because the number of
		 * samples in a row is made divisable by four. This makes it
		 * possible to execute SSE instruction easily.
		 * 
		 * If you want to skip over a whole row, use the Stride() method
		 * to determine the intrinsicly used width of one row.
		 * 
		 * @see Stride()
		 */
		const num_t *ValuePtr(unsigned x, unsigned y) const
		{
			return &_dataPtr[y][x];
		}
		
		num_t *Data()
		{
			return _dataConsecutive;
		}
		
		const num_t *Data() const
		{
			return _dataConsecutive;
		}
		
		/**
		 * This value specifies the intrinsic width of one row. It is
		 * normally the first number that is >= Width() and divisable by
		 * four. When using the ValuePtr(unsigned, unsigned) method,
		 * this value can be used to step over one row.
		 * 
		 * @see ValuePtr(unsigned, unsigned)
		 */
		size_t Stride() const
		{
			return _stride;
		}
		
		/**
		 * This call will set the width of the image to a new value. It won't
		 * reallocate the memory: this call is meant to be fast and avoid
		 * memory fragmentation. A new width should therefore be smaller or
		 * equal than the current available allocated space. The currently
		 * allocated space in horizontal direction is equal to Stride().
		 * On constructing an image, a larger capacity can be requested so
		 * that the image can be quickly resized later to a larger dimension.
		 * If the image is enlarged, the new space will be uninitialized.
		 * 
		 * @param newWidth The new width of the image. Should satisfy newWidth <= Stride().
		 */
		void ResizeWithoutReallocation(size_t newWidth);
		
	private:
		friend void swap(Image2D&, Image2D&);
		friend void swap(Image2D&, Image2D&&);
		friend void swap(Image2D&&, Image2D&);
		
		Image2D(size_t width, size_t height) :
			Image2D(width, height, width)
		{ }
		Image2D(size_t width, size_t height, size_t widthCapacity);
		
		void allocate();
		
		size_t _width, _height;
		size_t _stride;
		num_t **_dataPtr, *_dataConsecutive;
};

inline void swap(Image2D& left, Image2D& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._dataPtr, right._dataPtr);
	std::swap(left._dataConsecutive, right._dataConsecutive);
}

inline void swap(Image2D& left, Image2D&& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._dataPtr, right._dataPtr);
	std::swap(left._dataConsecutive, right._dataConsecutive);
}

inline void swap(Image2D&& left, Image2D& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._dataPtr, right._dataPtr);
	std::swap(left._dataConsecutive, right._dataConsecutive);
}

#endif
