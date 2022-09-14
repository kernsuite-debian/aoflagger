/** @file
 * This is the header file for the PngFile class.
 * @author André Offringa <offringa@gmail.com>
 */
#ifndef PNGFILE_H
#define PNGFILE_H

#include <string>

#include <png.h>

/**
 * This class wraps the libpng library. It can save an Image2D class to a .png
 * file.
 * @see Image2D
 */
class PngFile {
 public:
  /**
   * Construct a new png file with a filename, a width and a height.
   * @param filename Name of the png file.
   * @param width Width of the image
   * @param height Height of the image
   */
  PngFile(const std::string& filename, size_t width, size_t height);

  /**
   * Destructor.
   */
  ~PngFile();

  /**
   * Start writing.
   * @throws IOException if something goes wrong.
   */
  void BeginWrite();

  /**
   * Closes the image.
   */
  void Close();

  /**
   * Returns the size of one pixel in bytes.
   * @return Size of one pixel in bytes.
   */
  int PixelSize() const { return _pixelSize; }

  /**
   * Clears the entire image.
   * @param colorR Red background value.
   * @param colorG Green background value.
   * @param colorB Blue background value.
   * @param colorA Alfa background value.
   */
  void Clear(int colorR = 255, int colorG = 255, int colorB = 255,
             int colorA = 255);

  /**
   * Sets a pixel in the image to a specific color.
   * @param x x-coordinate.
   * @param y y-coordinate.
   * @param colorR Red value.
   * @param colorG Green value.
   * @param colorB Blue value.
   * @param colorA Alfa value.
   */
  void PlotPixel(size_t x, size_t y, int colorR, int colorG, int colorB,
                 int colorA) {
    _row_pointers[y][x * _pixelSize] = colorR;
    _row_pointers[y][x * _pixelSize + 1] = colorG;
    _row_pointers[y][x * _pixelSize + 2] = colorB;
    _row_pointers[y][x * _pixelSize + 3] = colorA;
  }

  /**
   * Retrieve the array of row pointers.
   * @return an array of row pointers.
   */
  png_bytep* RowPointers() const { return _row_pointers; }

 private:
  const std::string _filename;
  const size_t _width, _height;
  png_bytep* _row_pointers;
  png_structp _png_ptr;
  png_infop _info_ptr;
  FILE* _fp;
  const int _pixelSize;
};

#endif
