#include "../lua/data.h"
#include "../lua/functions.h"

#include "pyfunctions.h"

#include <aocommon/polarization.h>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include <iostream>

namespace py = pybind11;

using namespace aocommon;

PYBIND11_MODULE(aoflagger, m) {
  m.doc() = "AOFlagger module for detection of radio-frequency interference";

  py::enum_<enum aoflagger::TelescopeId>(m, "TelescopeId")
      .value("Generic", aoflagger::TelescopeId::GENERIC_TELESCOPE)
      .value("AARTFAAC", aoflagger::TelescopeId::AARTFAAC_TELESCOPE)
      .value("Arecibo", aoflagger::TelescopeId::ARECIBO_TELESCOPE)
      .value("Bighorns", aoflagger::TelescopeId::BIGHORNS_TELESCOPE)
      .value("JVLA", aoflagger::TelescopeId::JVLA_TELESCOPE)
      .value("LOFAR", aoflagger::TelescopeId::LOFAR_TELESCOPE)
      .value("MWA", aoflagger::TelescopeId::MWA_TELESCOPE)
      .value("Parkes", aoflagger::TelescopeId::PARKES_TELESCOPE)
      .value("WSRT", aoflagger::TelescopeId::WSRT_TELESCOPE);

  py::class_<aoflagger::ImageSet>(
      m, "ImageSet",
      "A set of time-frequency 'images' which together contain data for one \n"
      "correlated baseline or dish. \n"
      "The class either holds 1, 2, 4 or 8 images. These images have time on "
      "the \n"
      "x-axis (most rapidly changing index) and frequency on the y-axis. The \n"
      "cells specify flux levels, which do not need to have been calibrated. \n"
      "\n"
      "If the set contains only one image, it specifies amplitudes of a single "
      "\n"
      "polarization. If it contains two images, it specifies the real and "
      "imaginary \n"
      "parts of a single polarization. With four images, it contains the real "
      "\n"
      "and imaginary values of two polarizations (ordered real pol A, imag pol "
      "A, \n"
      "real pol B, imag pol B). With eight images, it contains complex values "
      "for \n"
      "four correlated polarizations (ordered real pol A, imag pol A, real pol "
      "B, \n"
      "... etc). \n"
      "\n"
      "This class wraps the C++ class aoflagger::ImageSet.\n")
      .def("width", &aoflagger::ImageSet::Width,
           "Get width (number of time steps) of images")
      .def("height", &aoflagger::ImageSet::Height,
           "Get height (number of frequency channels) of images")
      .def("image_count", &aoflagger::ImageSet::ImageCount,
           "Get number of images, see class description for details")
      .def("horizontal_stride", &aoflagger::ImageSet::HorizontalStride)
      .def("set", &aoflagger::ImageSet::Set,
           "Set all samples to the specified value")
      .def("get_image_buffer", aoflagger_python::GetImageBuffer,
           "Get access to one of the image sets stored in this object. \n"
           "Returns a numpy double array of ntimes x nchannels.")
      .def("set_image_buffer", aoflagger_python::SetImageBuffer,
           "Replace the data of one of the image sets. This function expects\n"
           "a numpy double array of ntimes x nchannels.")
      .def("resize_without_reallocation",
           &aoflagger::ImageSet::ResizeWithoutReallocation);

  py::class_<aoflagger::FlagMask>(
      m, "FlagMask",
      "A two-dimensional flag mask.\n\n"
      "The flag mask specifies which values in an ImageSet are flagged.\n"
      "A value true means a value is flagged, i.e., contains RFI and should\n"
      "not be used in further data processing (calibration, imaging, etc.).\n"
      "A flag denotes that all the value at that time-frequency position "
      "should\n"
      "be ignored for all polarizations.\n"
      "\n"
      "If polarization-specific flags are needed, one could run the flagger "
      "on\n"
      "each polarization individually. However, note that some algorithms, "
      "like\n"
      "the morphological scale-invariant rank operator (SIR operator), work "
      "best\n"
      "when seeing the flags from all polarizations.\n"
      "\n"
      "This class wraps the C++ class aoflagger::FlagMask.")
      .def("width", &aoflagger::FlagMask::Width,
           "Get width (number of time steps) of flag mask")
      .def("height", &aoflagger::FlagMask::Height,
           "Get height (number of frequency channels) of flag mask")
      .def("horizontal_stride", &aoflagger::FlagMask::HorizontalStride)
      .def("get_buffer", aoflagger_python::GetBuffer,
           "Returns the flag mask as a bool numpy array with dimensions ntimes "
           "x nchannels.")
      .def("set_buffer", aoflagger_python::SetBuffer,
           "Sets the flag mask from a bool numpy array with dimensions ntimes "
           "x nchannels.");

  py::class_<aoflagger::Strategy>(
      m, "Strategy",
      "Holds a flagging strategy.\n\n"
      "Telescope-specific flagging strategies can be created with \n"
      "AOFlagger.make_strategy(), or "
      "can be loaded from disk with AOFlagger.load_strategy(). Strategies\n"
      "can not be changed with this interface. A user can create strategies\n"
      "with the @c rfigui tool that is part of the aoflagger package.")
      .def("run", aoflagger_python::Run1)
      .def("run", aoflagger_python::Run2);

  py::class_<aoflagger::QualityStatistics>(m, "QualityStatistics")
      .def(py::self += py::self)
      .def("collect_statistics",
           &aoflagger::QualityStatistics::CollectStatistics)
      .def("write_statistics", &aoflagger::QualityStatistics::WriteStatistics);

  py::class_<aoflagger::AOFlagger>(
      m, "AOFlagger",
      "Main class that gives access to the aoflagger functions.")
      .def(py::init<>())
      .def("make_image_set", aoflagger_python::MakeImageSet1,
           py::return_value_policy::move)
      .def("make_image_set", aoflagger_python::MakeImageSet2,
           py::return_value_policy::move)
      .def("make_image_set", aoflagger_python::MakeImageSet3,
           py::return_value_policy::move)
      .def("make_image_set", aoflagger_python::MakeImageSet4,
           py::return_value_policy::move)
      .def("make_flag_mask", aoflagger_python::MakeFlagMask1,
           py::return_value_policy::move)
      .def("make_flag_mask", aoflagger_python::MakeFlagMask2,
           py::return_value_policy::move)
      .def("find_strategy_file", aoflagger_python::FindStrategyFile1)
      .def("find_strategy_file", aoflagger_python::FindStrategyFile2)
      .def("load_strategy_file", aoflagger_python::LoadStrategyFile,
           py::return_value_policy::move)
      .def("make_quality_statistics", aoflagger_python::MakeQualityStatistics1,
           py::return_value_policy::move)
      .def("make_quality_statistics", aoflagger_python::MakeQualityStatistics2,
           py::return_value_policy::move)
      .def_static("get_version_string", &aoflagger::AOFlagger::GetVersionString)
      .def_static("get_version_date", &aoflagger::AOFlagger::GetVersionDate);
}
