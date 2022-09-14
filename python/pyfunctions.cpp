#include "pyfunctions.h"

#include <pybind11/pybind11.h>

#include <stdexcept>

namespace py = pybind11;

namespace aoflagger_python {

py::array_t<double> GetImageBuffer(const aoflagger::ImageSet* imageSet,
                                   size_t imageIndex) {
  if (imageIndex >= imageSet->ImageCount())
    throw std::out_of_range(
        "aoflagger.get_image_buffer: Image index out of bounds");
  const float* values = imageSet->ImageBuffer(imageIndex);
  py::buffer_info buf = py::buffer_info(
      nullptr,  // ask NumPy to allocate
      sizeof(double), py::format_descriptor<double>::value, 2,
      {ptrdiff_t(imageSet->Height()), ptrdiff_t(imageSet->Width())},
      {sizeof(double) * imageSet->Width(), sizeof(double)}
      /* Strides for each dimension */
  );
  py::array_t<double> result(buf);

  buf = result.request();
  char* resultData = (char*)buf.ptr;
  int stride0 = buf.strides[0];
  int stride1 = buf.strides[1];
  for (size_t y = 0; y != imageSet->Height(); ++y) {
    const float* rowOut = values + y * imageSet->HorizontalStride();
    char* rowIn = resultData + y * stride0;
    for (size_t x = 0; x != imageSet->Width(); ++x) {
      *reinterpret_cast<double*>(rowIn + x * stride1) = rowOut[x];
    }
  }
  return result;
}

void SetImageBuffer(aoflagger::ImageSet* imageSet, size_t imageIndex,
                    py::array_t<double>& values) {
  if (imageIndex >= imageSet->ImageCount())
    throw std::out_of_range(
        "aoflagger.get_image_buffer: Image index out of bounds");
  if (values.ndim() != 2)
    throw std::runtime_error(
        "ImageSet.set_image_buffer(): require a two-dimensional array");
  if (values.shape(0) != int(imageSet->Height()) ||
      values.shape(1) != int(imageSet->Width()))
    throw std::runtime_error(
        "ImageSet.set_image_buffer(): dimensions of provided array doesn't "
        "match with image set");
  const py::buffer_info buf = values.request();
  int stride0 = buf.strides[0];
  int stride1 = buf.strides[1];
  const char* data = static_cast<const char*>(buf.ptr);
  if (!data)
    throw std::runtime_error(
        "Data needs to be provided that is interpretable as a double array");
  float* buffer = imageSet->ImageBuffer(imageIndex);
  for (size_t y = 0; y != imageSet->Height(); ++y) {
    const char* rowIn = data + y * stride0;
    float* rowOut = buffer + y * imageSet->HorizontalStride();
    for (size_t x = 0; x != imageSet->Width(); ++x) {
      rowOut[x] = *reinterpret_cast<const double*>(rowIn + x * stride1);
    }
  }
}

py::array_t<bool> GetBuffer(const aoflagger::FlagMask* flagMask) {
  const bool* values = flagMask->Buffer();
  py::buffer_info buf = py::buffer_info(
      nullptr,  // ask NumPy to allocate
      sizeof(bool), py::format_descriptor<bool>::value, 2,
      {ptrdiff_t(flagMask->Height()), ptrdiff_t(flagMask->Width())},
      {sizeof(bool) * flagMask->Width(), sizeof(bool)}
      /* Strides for each dimension */
  );
  py::array_t<bool> result(buf);
  buf = result.request();
  char* resultData = static_cast<char*>(buf.ptr);
  int stride0 = buf.strides[0];
  int stride1 = buf.strides[1];
  for (size_t y = 0; y != flagMask->Height(); ++y) {
    const bool* rowOut = values + y * flagMask->HorizontalStride();
    char* rowIn = resultData + y * stride0;
    for (size_t x = 0; x != flagMask->Width(); ++x) {
      *reinterpret_cast<bool*>(rowIn + x * stride1) = rowOut[x];
    }
  }
  return result;
}

void SetBuffer(aoflagger::FlagMask* flagMask, pybind11::array_t<bool>& values) {
  if (values.ndim() != 2)
    throw std::runtime_error(
        "FlagMask.set_buffer(): Invalid dimensions specified for data array; "
        "two dimensional array required");
  if (values.shape(0) != int(flagMask->Height()) ||
      values.shape(1) != int(flagMask->Width()))
    throw std::runtime_error(
        "FlagMask.set_buffer(): dimensions of provided array don't match with "
        "image set");
  py::buffer_info buf = values.request();
  const char* data = static_cast<const char*>(buf.ptr);
  if (!data)
    throw std::runtime_error(
        "Data needs to be provided that is interpretable as a bool array");
  bool* buffer = flagMask->Buffer();
  int stride0 = buf.strides[0];
  int stride1 = buf.strides[1];
  for (size_t y = 0; y != flagMask->Height(); ++y) {
    const char* rowIn = data + y * stride0;
    bool* rowOut = buffer + y * flagMask->HorizontalStride();
    for (size_t x = 0; x != flagMask->Width(); ++x) {
      rowOut[x] = *reinterpret_cast<const double*>(rowIn + x * stride1);
    }
  }
}

py::object MakeImageSet1(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height, size_t count) {
  return py::cast(flagger->MakeImageSet(width, height, count));
}

py::object MakeImageSet2(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height, size_t count, size_t widthCapacity) {
  return py::cast(flagger->MakeImageSet(width, height, count, widthCapacity));
}

py::object MakeImageSet3(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height, size_t count, float initialValue) {
  return py::cast(flagger->MakeImageSet(width, height, count, initialValue));
}

py::object MakeImageSet4(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height, size_t count, float initialValue,
                         size_t widthCapacity) {
  return py::cast(
      flagger->MakeImageSet(width, height, count, initialValue, widthCapacity));
}

py::object MakeFlagMask1(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height) {
  return py::cast(flagger->MakeFlagMask(width, height));
}

py::object MakeFlagMask2(aoflagger::AOFlagger* flagger, size_t width,
                         size_t height, bool initialValue) {
  return py::cast(flagger->MakeFlagMask(width, height, initialValue));
}

py::object FindStrategyFile1(aoflagger::AOFlagger* flagger,
                             enum aoflagger::TelescopeId telescopeId) {
  return FindStrategyFile2(flagger, telescopeId, "");
}

py::object FindStrategyFile2(aoflagger::AOFlagger* flagger,
                             enum aoflagger::TelescopeId telescopeId,
                             const char* scenario) {
  std::string path =
      flagger->FindStrategyFile(telescopeId, std::string(scenario));
  if (path.empty())
    throw std::runtime_error(
        "find_strategy(): Could not find requested strategy");
  return py::cast(path);
}

py::object LoadStrategyFile(aoflagger::AOFlagger* flagger,
                            const char* filename) {
  return py::cast(flagger->LoadStrategyFile(std::string(filename)),
                  py::return_value_policy::move);
}

py::object Run1(aoflagger::Strategy* strategy,
                const aoflagger::ImageSet& input) {
  return py::cast(strategy->Run(input));
}

py::object Run2(aoflagger::Strategy* strategy, const aoflagger::ImageSet& input,
                const aoflagger::FlagMask& existingFlags) {
  return py::cast(strategy->Run(input, existingFlags));
}

py::object MakeQualityStatistics1(aoflagger::AOFlagger* flagger,
                                  py::array_t<double>& scanTimes,
                                  py::array_t<double>& channelFrequencies,
                                  size_t nPolarizations,
                                  bool computeHistograms) {
  if (scanTimes.ndim() != 1)
    throw std::runtime_error(
        "AOFlagger.make_quality_statistics(): Invalid dimensions specified for "
        "scanTimes array; one dimensional array required");
  size_t nScans = scanTimes.shape(0);
  py::buffer_info tbuf = scanTimes.request();
  const double* scanTimesArr = reinterpret_cast<const double*>(tbuf.ptr);
  if (!scanTimesArr)
    throw std::runtime_error(
        "scanTimes data needs to be provided that is interpretable as a double "
        "array");

  if (channelFrequencies.ndim() != 1)
    throw std::runtime_error(
        "AOFlagger.make_quality_statistics(): Invalid dimensions specified for "
        "channelFrequencies array; one dimensional array required");
  size_t nChannels = channelFrequencies.shape(0);
  py::buffer_info fbuf = scanTimes.request();
  const double* channelFrequenciesArr =
      reinterpret_cast<const double*>(fbuf.ptr);
  if (!channelFrequenciesArr)
    throw std::runtime_error(
        "Data needs to be provided that is interpretable as a double array");

  return py::cast(flagger->MakeQualityStatistics(
      scanTimesArr, nScans, channelFrequenciesArr, nChannels, nPolarizations,
      computeHistograms));
}

py::object MakeQualityStatistics2(aoflagger::AOFlagger* flagger,
                                  py::array_t<double>& scanTimes,
                                  py::array_t<double>& channelFrequencies,
                                  size_t nPolarizations) {
  return MakeQualityStatistics1(flagger, scanTimes, channelFrequencies,
                                nPolarizations, false);
}

}  // namespace aoflagger_python
