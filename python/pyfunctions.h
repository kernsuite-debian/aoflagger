#ifndef PY_FUNCTION_H
#define PY_FUNCTION_H

#include "../lua/data.h"

#include "../interface/aoflagger.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace aoflagger_python {
pybind11::object get_flag_function();

void set_flag_function(PyObject* callable);

pybind11::list polarizations(const aoflagger_lua::Data* data);

pybind11::array_t<double> GetImageBuffer(const aoflagger::ImageSet* imageSet,
                                         size_t imageIndex);

void SetImageBuffer(aoflagger::ImageSet* imageSet, size_t imageIndex,
                    pybind11::array_t<double>& values);

pybind11::array_t<bool> GetBuffer(const aoflagger::FlagMask* flagMask);

void SetBuffer(aoflagger::FlagMask* flagMask, pybind11::array_t<bool>& values);

pybind11::object MakeImageSet1(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height, size_t count);
pybind11::object MakeImageSet2(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height, size_t count,
                               size_t widthCapacity);
pybind11::object MakeImageSet3(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height, size_t count, float initialValue);
pybind11::object MakeImageSet4(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height, size_t count, float initialValue,
                               size_t widthCapacity);

pybind11::object MakeFlagMask1(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height);
pybind11::object MakeFlagMask2(aoflagger::AOFlagger* flagger, size_t width,
                               size_t height, bool initialValue);

pybind11::object FindStrategyFile1(aoflagger::AOFlagger* flagger,
                                   enum aoflagger::TelescopeId telescopeId);
pybind11::object FindStrategyFile2(aoflagger::AOFlagger* flagger,
                                   enum aoflagger::TelescopeId telescopeId,
                                   const char* scenario);

pybind11::object LoadStrategyFile(aoflagger::AOFlagger* flagger,
                                  const char* filename);

pybind11::object Run1(aoflagger::Strategy* strategy,
                      const aoflagger::ImageSet& input);
pybind11::object Run2(aoflagger::Strategy* strategy,
                      const aoflagger::ImageSet& input,
                      const aoflagger::FlagMask& existingFlags);

pybind11::object MakeQualityStatistics1(
    aoflagger::AOFlagger* flagger, pybind11::array_t<double>& scanTimes,
    pybind11::array_t<double>& channelFrequencies, size_t nPolarizations,
    bool computeHistograms);
pybind11::object MakeQualityStatistics2(
    aoflagger::AOFlagger* flagger, pybind11::array_t<double>& scanTimes,
    pybind11::array_t<double>& channelFrequencies, size_t nPolarizations);
};  // namespace aoflagger_python

#endif
