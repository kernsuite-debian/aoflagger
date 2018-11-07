#include "pythonstrategy.h"

#include "../../python/data.h"
#include "../../python/functions.h"

#include <boost/python.hpp>
#include <boost/filesystem.hpp>

#include <fstream>

using namespace boost::python;

PythonStrategy::PythonStrategy() : _code(
	"import aoflagger\n"
	"\n"
	"def flag(data):\n"
	"  print(\'In flag() function\')\n"
	"  processed_polarizations = data.polarizations()\n"
	"  processed_representations = [ aoflagger.ComplexRepresentation.AmplitudePart ]\n"
	"  \n"
	"  for polarization in processed_polarizations:\n"
	"    pol_data = data.convert_to_polarization(polarization)\n"
	"    for representation in processed_representations:\n"
	"      print(\'Flagging polarization \' + str(polarization) + \' (\' + str(representation) + \')\')\n"
	"      repr_data = pol_data.convert_to_polarization(polarization)\n"
	"      \n"
	"      aoflagger.sumthreshold(repr_data, 1.0, 1.0, True, True)\n"
	"\n"
	"aoflagger.set_flag_function(flag)\n"
	"\n"
	"print(\'File parsed\')\n"
	)
{
	std::ifstream file("strategy.py");
	if(file.good())
	{
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::vector<char> data(size+1, 0);
		file.seekg(0, std::ios::beg);
		file.read(data.data(), size);
		_code = data.data();
	}

	Py_Initialize();

	// The following statement add the curr path to the Python search path
	boost::filesystem::path workingDir = boost::filesystem::current_path().normalize();
	PyObject* sysPath = PySys_GetObject(const_cast<char*>("path"));
	PyList_Insert( sysPath, 0, PyUnicode_FromString(workingDir.string().c_str()));
}

PythonStrategy::~PythonStrategy()
{
	Py_Finalize();
}

std::string PythonStrategy::getPythonError()
{
	using namespace boost::python;
	using namespace boost;

	PyObject *exc,*val,*tb;
	object formatted_list, formatted;
	PyErr_Fetch(&exc,&val,&tb);
	PyErr_NormalizeException(&exc,&val,&tb);
	handle<> hexc(exc),hval(allow_null(val)),htb(allow_null(tb)); 
	object traceback(import("traceback"));
	if (!tb) {
		object format_exception_only(traceback.attr("format_exception_only"));
		formatted_list = format_exception_only(hexc,hval);
	} else {
		object format_exception(traceback.attr("format_exception"));
		formatted_list = format_exception(hexc,hval,htb);
	}
	formatted = str("\n").join(formatted_list);
	return extract<std::string>(formatted);
}

void PythonStrategy::Execute(TimeFrequencyData& tfData)
{
	try {
		object main = import("__main__");
		object global(main.attr("__dict__"));
		object result = exec(_code.c_str(), global, global);
		object flagFunction = aoflagger_python::get_flag_function();

		if(flagFunction.is_none())
			throw std::runtime_error("Incorrect Python strategy: strategy did not provide a flag method. Make sure your strategy uses aoflagger.set_flag_function() to provide the flag function to the caller");
		else {
			aoflagger_python::Data data(tfData);
			flagFunction(boost::ref(data));
			tfData = data.TFData();
		}
	} catch(const error_already_set&) {
		throw std::runtime_error(getPythonError());
	}
}
