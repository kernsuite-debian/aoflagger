pybind11_add_module(python_aoflagger MODULE pyinterface.cpp pyfunctions.cpp)

set_target_properties(python_aoflagger PROPERTIES SOVERSION 0)
set_target_properties(python_aoflagger PROPERTIES OUTPUT_NAME "aoflagger")
target_link_libraries(python_aoflagger PUBLIC aoflagger-lib PUBLIC ${ALL_LIBRARIES})
install (TARGETS python_aoflagger DESTINATION lib) 
