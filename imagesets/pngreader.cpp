#include "pngreader.h"

#include <png.h>

std::unique_ptr<imagesets::BaselineData> imagesets::PngReader::Read(
    class ProgressListener& progress) {
  FILE* fp = fopen(_path.c_str(), "rb");
  if (fp == nullptr) throw std::runtime_error("Could not open file");

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (!png_ptr) throw std::runtime_error("Error creating png struct");

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    throw std::runtime_error("Error creating png read struct");
  }

  png_init_io(png_ptr, fp);

  png_read_info(png_ptr, info_ptr);

  const unsigned width = png_get_image_width(png_ptr, info_ptr);
  const unsigned height = png_get_image_height(png_ptr, info_ptr);
  const unsigned color_type = png_get_color_type(png_ptr, info_ptr);
  const unsigned bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  Logger::Debug << "Png file: " << width << 'x' << height
                << " colortype=" << color_type << ", bit_depth=" << bit_depth
                << '\n';

  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) throw std::runtime_error("Png error");

  png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for (size_t y = 0; y < height; y++)
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

  png_read_image(png_ptr, row_pointers);

  fclose(fp);

  const Image2DPtr image = Image2D::CreateZeroImagePtr(width, height);
  const size_t bytesPerSample = 4;
  for (size_t f = 0; f < height; ++f) {
    for (size_t t = 0; t < width; ++t) {
      int r = row_pointers[f][t * bytesPerSample],
          g = row_pointers[f][t * bytesPerSample + 1],
          b = row_pointers[f][t * bytesPerSample + 2];
      image->SetValue(t, height - 1 - f, r + g + b);
    }
  }

  const TimeFrequencyData tfData(TimeFrequencyData::AmplitudePart,
                                 aocommon::Polarization::StokesI, image);
  return std::unique_ptr<imagesets::BaselineData>(
      new BaselineData(tfData, TimeFrequencyMetaDataCPtr()));
}
