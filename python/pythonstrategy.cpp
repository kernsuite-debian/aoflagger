#include "pythonstrategy.h"

#include "../lua/data.h"
#include "../lua/functions.h"

#include "../python/pyfunctions.h"

#include <filesystem>

#include <pybind11/pybind11.h>
#include <pybind11/eval.h>

#include <fstream>

PythonStrategy::PythonStrategy() : _code() {
  std::ifstream file("strategy.py");
  if (file.good()) {
    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    std::vector<char> data(size + 1, 0);
    file.seekg(0, std::ios::beg);
    file.read(data.data(), size);
    _code = data.data();
  }

  Py_Initialize();

  // The following statement add the curr path to the Python search path
  const std::filesystem::path workingDir =
      std::filesystem::canonical(std::filesystem::current_path());
  PyObject* sysPath = PySys_GetObject(const_cast<char*>("path"));
  PyList_Insert(sysPath, 0, PyUnicode_FromString(workingDir.string().c_str()));
}

PythonStrategy::~PythonStrategy() { Py_Finalize(); }

void PythonStrategy::Execute(TimeFrequencyData& tfData,
                             TimeFrequencyMetaDataCPtr metaData,
                             class ScriptData& scriptData) {
  /*
  pybind11::object
  global(pybind11::module::import("__main__").attr("__dict__"));
  // exec requires a recent pybind11 version !
  pybind11::exec(_code.c_str(), global);
  pybind11::object flagFunction = aoflagger_python::get_flag_function();

  if(flagFunction.is_none())
          throw std::runtime_error("Incorrect Python strategy: strategy did not
  provide a flag method. Make sure your strategy uses
  aoflagger.set_flag_function() to provide the flag function to the caller");
  else {
          aoflagger_lua::Data::Context context;
          aoflagger_lua::Data data(tfData, metaData, context);
          flagFunction(std::ref(data));
          tfData = data.TFData();
  }
  */
}
