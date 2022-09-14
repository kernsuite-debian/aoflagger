#include "pngfile.h"

#include <stdexcept>

PngFile::PngFile(const std::string& filename, size_t width, size_t height)
    : _filename(filename), _width(width), _height(height), _pixelSize(4) {}

PngFile::~PngFile() {}

void PngFile::BeginWrite() {
  _fp = fopen(_filename.c_str(), "wb");
  if (!_fp) throw std::runtime_error("Can not open file");

  _png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) nullptr,
                                     nullptr, nullptr);

  if (!_png_ptr) {
    fclose(_fp);
    throw std::runtime_error("Can not create png write structure");
  }

  _info_ptr = png_create_info_struct(_png_ptr);
  if (!_info_ptr) {
    png_destroy_write_struct(&_png_ptr, (png_infopp) nullptr);
    fclose(_fp);
    throw std::runtime_error("Can not write info structure to file");
  }

  if (setjmp(png_jmpbuf(_png_ptr))) {
    png_destroy_write_struct(&_png_ptr, &_info_ptr);
    fclose(_fp);
    throw std::runtime_error(
        "Unknown error occured during writing of png file");
  }

  png_init_io(_png_ptr, _fp);

  png_set_IHDR(_png_ptr, _info_ptr, _width, _height, 8,
               PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  _row_pointers = (png_bytep*)png_malloc(_png_ptr, _height * sizeof(png_bytep));

  for (size_t i = 0; i < _height; i++)
    _row_pointers[i] = (png_bytep)png_malloc(_png_ptr, _width * _pixelSize);
}

void PngFile::Close() {
  png_set_rows(_png_ptr, _info_ptr, _row_pointers);
  png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
  png_write_end(_png_ptr, _info_ptr);

  for (unsigned i = 0; i < _height; i++) png_free(_png_ptr, _row_pointers[i]);
  png_free(_png_ptr, _row_pointers);

  png_destroy_write_struct(&_png_ptr, &_info_ptr);
  fclose(_fp);
}

void PngFile::Clear(int colorR, int colorG, int colorB, int colorA) {
  for (size_t y = 0; y < _height; y++) {
    int xa = 0;
    for (unsigned x = 0; x < _width; x++) {
      _row_pointers[y][xa++] = colorR;
      _row_pointers[y][xa++] = colorG;
      _row_pointers[y][xa++] = colorB;
      _row_pointers[y][xa++] = colorA;
    }
  }
}
