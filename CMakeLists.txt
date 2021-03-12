# AOFlagger 2.8 and higher use 'OBJECT' libraries and the 'TARGET_OBJECTS keywords. These
# require cmake 2.8.8.
cmake_minimum_required(VERSION 2.8.8)

project(AOFlagger)

include(CMakeVersionInfo.txt)
set(AOFLAGGER_VERSION "${AOFLAGGER_VERSION_MAJOR}.${AOFLAGGER_VERSION_MINOR}.${AOFLAGGER_VERSION_SUBMINOR}")

# Some CMake 'find package' files are stored in this directory
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0>=3.0.0)
pkg_check_modules(SIGCXX sigc++-2.0)

# Find and include git submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --checkout
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# Include aocommon/pybind11 headers
include_directories("${CMAKE_SOURCE_DIR}/aocommon/include")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/pybind11")
include_directories(${pybind11_INCLUDE_DIR})

set(CASACORE_MAKE_REQUIRED_EXTERNALS_OPTIONAL TRUE)
find_package(Casacore REQUIRED COMPONENTS casa ms tables measures)
include_directories(${CASACORE_INCLUDE_DIRS})

find_package(CFITSIO REQUIRED)
include_directories(${CFITSIO_INCLUDE_DIR})

find_path(FFTW3_INCLUDE_DIR NAMES fftw3.h)

find_library(GSL_LIB NAMES gsl)
find_library(GSL_CBLAS_LIB NAMES gslcblas)
find_path(GSL_INCLUDE_DIR NAMES gsl/gsl_version.h)
if(GSL_INCLUDE_DIR)
  include_directories(${GSL_INCLUDE_DIR})
endif(GSL_INCLUDE_DIR)

find_package(Threads REQUIRED)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

find_package(PNG REQUIRED)
find_package(PythonLibs 3 REQUIRED)
find_package(PythonInterp REQUIRED)
message(STATUS "Using python version ${PYTHON_VERSION_STRING}")
include_directories(${PYTHON_INCLUDE_DIRS})

# boost::alignment requires Boost 1.56
find_package(Boost 1.56.0 REQUIRED COMPONENTS date_time filesystem system unit_test_framework)
include_directories(${Boost_INCLUDE_DIR})

find_library(FFTW3_LIB fftw3 REQUIRED)
include_directories(${FFTW3_INCLUDE_DIR})

enable_language(Fortran OPTIONAL)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(Lua 5.2 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# The following stuff will set the "rpath" correctly, so that
# LD_LIBRARY_PATH doesn't have to be set.

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=vla -DNDEBUG -funroll-loops -O3 -std=c++11")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Debug build selected: setting linking flag --no-undefined")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif()
  
include(CheckCXXCompilerFlag)
include(CheckSymbolExists) 
include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)

CHECK_SYMBOL_EXISTS(posix_fallocate "fcntl.h" HAVE_POSIX_FALLOCATE) 
CHECK_CXX_SYMBOL_EXISTS(exp10 "cmath" HAVE_EXP10)
if(HAVE_POSIX_FALLOCATE)
  add_definitions(-DHAVE_POSIX_FALLOCATE)
endif(HAVE_POSIX_FALLOCATE)

if(PORTABLE)
  message(STATUS "Portable build requested; a generic build will be created with slightly decreased performance.")
else()	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif(PORTABLE)

if(GTKMM_FOUND)
  add_definitions(-DHAVE_GTKMM)
endif(GTKMM_FOUND)

if(GSL_LIB AND GSL_CBLAS_LIB AND GSL_INCLUDE_DIR)
  add_definitions(-DHAVE_GSL)
  link_libraries(${GSL_LIB} ${GSL_CBLAS_LIB})
  message(STATUS "GSL found.")
endif(GSL_LIB AND GSL_CBLAS_LIB AND GSL_INCLUDE_DIR)

configure_file(version.h.in version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(AOQPLOT_FILES
  aoqplot/aoqplotwindow.cpp
  aoqplot/baselineplotpage.cpp
  aoqplot/datawindow.cpp
  aoqplot/grayscaleplotpage.cpp
  aoqplot/histogrampage.cpp
  aoqplot/timefrequencyplotpage.cpp
  aoqplot/twodimensionalplotpage.cpp
  aoqplot/controllers/aoqplotcontroller.cpp
  aoqplot/controllers/aoqplotpagecontroller.cpp
  aoqplot/controllers/baselinepagecontroller.cpp
  aoqplot/controllers/heatmappagecontroller.cpp
  aoqplot/controllers/histogrampagecontroller.cpp
  aoqplot/controllers/tfpagecontroller.cpp)

set(PLOT_FILES
  plot/colorscale.cpp
  plot/heatmapplot.cpp
  plot/horizontalplotscale.cpp
  plot/legend.cpp
  plot/plot2d.cpp
  plot/plotpropertieswindow.cpp
  plot/title.cpp
  plot/verticalplotscale.cpp)

set(GUI_FILES
  rfigui/controllers/imagecomparisoncontroller.cpp
  rfigui/controllers/rfiguicontroller.cpp
  rfigui/complexplaneplotwindow.cpp
  rfigui/gotowindow.cpp
  rfigui/imagepropertieswindow.cpp
  rfigui/heatmapwidget.cpp
  rfigui/msoptionwindow.cpp
  rfigui/opendialog.cpp
  rfigui/plotframe.cpp
  rfigui/plotwindow.cpp
  rfigui/progresswindow.cpp
  rfigui/rfiguimenu.cpp
  rfigui/rfiguiwindow.cpp
  rfigui/settings.cpp
  rfigui/strategyeditor.cpp
  util/rfiplots.cpp
  util/multiplot.cpp
  ${PLOT_FILES} ${AOQPLOT_FILES})

set(IMAGING_FILES
  imaging/uvimager.cpp
  imaging/model.cpp)

set(INTERFACE_FILES
  interface/aoflagger.cpp
  interface/flagmask.cpp
  interface/imageset.cpp
  interface/qualitystatistics.cpp
  interface/strategy.cpp
  )
  
set(LUA_FILES
  lua/datawrapper.cpp
  lua/default-strategy.cpp
  lua/functions.cpp
  lua/functionswrapper.cpp
  lua/luastrategy.cpp
  lua/optionsfunction.cpp
  lua/scriptdata.cpp
  lua/telescopefile.cpp)
  
set(MSIO_FILES
  msio/baselinematrixloader.cpp
  msio/baselinereader.cpp
  msio/directbaselinereader.cpp
  msio/fitsfile.cpp
  msio/indirectbaselinereader.cpp
  msio/memorybaselinereader.cpp
  msio/multibanddata.cpp
  msio/msstatreader.cpp
  msio/pngfile.cpp
  msio/rspreader.cpp
  msio/singlebaselinefile.cpp
  msio/spatialtimeloader.cpp)
  
set(STRUCTURES_FILES
  structures/colormap.cpp
  structures/image2d.cpp
  structures/mask2d.cpp
  structures/msiterator.cpp
  structures/msmetadata.cpp
  structures/samplerow.cpp
  structures/stokesimager.cpp
  structures/timefrequencydata.cpp)
  
set(QUALITY_FILES
  quality/collector.cpp
  quality/histogramcollection.cpp
  quality/histogramtablesformatter.cpp
  quality/qualitytablesformatter.cpp
  quality/rayleighfitter.cpp
  quality/statisticscollection.cpp)

set(ALGORITHMS_FILES
  algorithms/antennaselector.cpp
  algorithms/baselineselector.cpp
	algorithms/baselinetimeplaneimager.cpp
  algorithms/combinatorialthresholder.cpp
  algorithms/fringestoppingfitter.cpp
  algorithms/fringetestcreater.cpp
	algorithms/highpassfilter.cpp
  algorithms/localfitmethod.cpp
  algorithms/morphology.cpp
  algorithms/sinusfitter.cpp
  algorithms/morphologicalflagger.cpp
  algorithms/sumthreshold.cpp
  algorithms/sumthresholdmissing.cpp
  algorithms/svdmitigater.cpp
  algorithms/testsetgenerator.cpp
  algorithms/thresholdconfig.cpp
  algorithms/thresholdtools.cpp
  algorithms/timefrequencystatistics.cpp)

set(PYTHON_FILES
  python/pythonstrategy.cpp
  python/pyfunctions.cpp)

set(IMAGESETS_FILES
  imagesets/bhfitsimageset.cpp
  imagesets/filterbankset.cpp
  imagesets/fitsimageset.cpp
  imagesets/imageset.cpp
  imagesets/indexableset.cpp
  imagesets/msimageset.cpp
  imagesets/parmimageset.cpp
  imagesets/pngreader.cpp
  imagesets/rfibaselineset.cpp)

set(STRATEGY_FILES)

set(UTIL_FILES
  util/logger.cpp
  util/ffttools.cpp
  util/integerdomain.cpp
  util/plot.cpp
  util/rng.cpp
  util/stopwatch.cpp)

set(ALL_LIBRARIES
	${CASACORE_LIBRARIES}
	${FFTW3_LIB}
	${CFITSIO_LIBRARY}
	${PYTHON_LIBRARIES}
	${PNG_LIBRARIES} ${LIBXML2_LIBRARIES}
	${LUA_LIBRARIES}
	${Boost_LIBRARIES}
	${BLAS_LIBRARIES} ${LAPACK_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

if(GTKMM_FOUND)
	message(STATUS "GTKMM found.") 
	include_directories(${GTKMM_INCLUDE_DIRS})
	link_directories(${GTKMM_LIBDIR})
	set(ALL_LIBRARIES ${ALL_LIBRARIES} ${GTKMM_LIBRARIES})
endif(GTKMM_FOUND)

if(SIGCXX_FOUND)
	message(STATUS "SIGCXX found.") 
	include_directories(${SIGCXX_INCLUDE_DIRS})
	link_directories(${SIGCXX_LIBDIR})
	set(ALL_LIBRARIES ${ALL_LIBRARIES} ${SIGCXX_LIBRARIES})
endif(SIGCXX_FOUND)

if(GTKMM_FOUND)
	set(AOFLAGGER_PLOT_FILES ${PLOT_FILES})
endif(GTKMM_FOUND)

set(ALLFILES
  ${ALGORITHMS_FILES}
	${AOFLAGGER_PLOT_FILES}
  ${IMAGESETS_FILES}
	${IMAGING_FILES}
  ${INTERFACE_FILES}
	${LUA_FILES}
	${MSIO_FILES}
  ${PYTHON_FILES}
	${QUALITY_FILES}
	${STRUCTURES_FILES}
	${UTIL_FILES}
)
add_library(aoflagger-lib SHARED ${ALLFILES})
set_target_properties(aoflagger-lib PROPERTIES SOVERSION 0)
set_target_properties(aoflagger-lib PROPERTIES OUTPUT_NAME aoflagger)
target_link_libraries(aoflagger-lib ${ALL_LIBRARIES})
install (TARGETS aoflagger-lib DESTINATION lib) 

add_executable(aoquality applications/aoquality.cpp)
target_link_libraries(aoquality aoflagger-lib ${ALL_LIBRARIES})
install (TARGETS aoquality DESTINATION bin)

if(GTKMM_FOUND)
	add_library(aoflaggergui OBJECT ${GUI_FILES})
	set(AOFLAGGERGUI_OBJECT $<TARGET_OBJECTS:aoflaggergui>)
  add_executable(rfigui applications/rfigui.cpp ${AOFLAGGERGUI_OBJECT})
  target_link_libraries(rfigui aoflagger-lib ${ALL_LIBRARIES})
	install (TARGETS rfigui DESTINATION bin)
  
	add_executable(aoqplot applications/aoqplot.cpp ${AOFLAGGERGUI_OBJECT})
  target_link_libraries(aoqplot aoflagger-lib ${ALL_LIBRARIES})
 	install (TARGETS aoqplot DESTINATION bin)
	
	add_executable(badstations applications/badstations.cpp)
	target_link_libraries(badstations aoflagger-lib ${ALL_LIBRARIES})
  
else()
  message(WARNING " The graphical user interface library GTKMM was not found; rfigui and aoqplot will not be compiled.")
endif(GTKMM_FOUND)

add_executable(aoflagger-bin
	applications/aoflagger.cpp
	aoluarunner/options.cpp 
	aoluarunner/runner.cpp aoluarunner/baselineiterator.cpp aoluarunner/writethread.cpp)
set_target_properties(aoflagger-bin PROPERTIES OUTPUT_NAME aoflagger)
target_link_libraries(aoflagger-bin aoflagger-lib ${ALL_LIBRARIES})
install (TARGETS aoflagger-bin DESTINATION bin)

add_executable(runtests EXCLUDE_FROM_ALL
	test/runtests.cpp
	
	test/experiments/defaultstrategyspeedtest.cpp
	test/experiments/highpassfilterexperiment.cpp
	
	test/lua/defaultstrategytest.cpp
	test/lua/flagnanstest.cpp
	test/lua/tmetadata.cpp
	test/lua/optionsfunctiontest.cpp
	test/lua/telescopefiletest.cpp
	
	test/interface/interfacetest.cpp
	
	test/quality/qualitytablesformattertest.cpp
	test/quality/statisticscollectiontest.cpp
	test/quality/statisticsderivatortest.cpp
	
	test/algorithms/convolutionstest.cpp
	test/algorithms/dilationtest.cpp
	test/algorithms/highpassfiltertest.cpp
	test/algorithms/noisestatisticstest.cpp
	test/algorithms/siroperatortest.cpp
	test/algorithms/sumthresholdmissingtest.cpp
	test/algorithms/sumthresholdtest.cpp
	test/algorithms/thresholdtoolstest.cpp
		
	test/structures/image2dtest.cpp
	test/structures/timefrequencydatatest.cpp
	test/structures/tmask2d.cpp
	test/structures/tversionstring.cpp
	
	test/util/numberparsertest.cpp
	
)
target_link_libraries(runtests aoflagger-lib ${ALL_LIBRARIES})

add_test(runtests runtests)
add_custom_target(check COMMAND runtests DEPENDS runtests)

install (FILES interface/aoflagger.h DESTINATION include)
install (DIRECTORY data/icons data/applications DESTINATION share )
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/strategies DESTINATION share/aoflagger FILES_MATCHING PATTERN *.lua)

# Set-up cmake configuration files
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/config/aoflagger-config.cmake.in"
    "${PROJECT_BINARY_DIR}/CMakeFiles/aoflagger-config.cmake" @ONLY)
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/config/aoflagger-config-version.cmake.in"
    "${PROJECT_BINARY_DIR}/CMakeFiles/aoflagger-config-version.cmake" @ONLY)

install(FILES
    "${PROJECT_BINARY_DIR}/CMakeFiles/aoflagger-config.cmake"
    "${PROJECT_BINARY_DIR}/CMakeFiles/aoflagger-config-version.cmake"
    DESTINATION share/aoflagger/cmake)

add_subdirectory(python)

add_subdirectory(cpack)

add_custom_target(sphinxdoc
  make html
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc
  COMMENT "Generating documentation with Sphinx" VERBATIM)

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
  # add target to generate API documentation with Doxygen
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  message(STATUS "Doxygen found: API documentation can be compiled.")
  add_custom_target(doxygendoc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
  add_custom_target(doc DEPENDS doxygendoc sphinxdoc)
else()
  message(STATUS "Doxygen not found: API documentation can not compiled.")  
  add_custom_target(doc DEPENDS sphinxdoc)  
endif()

# This is just for development: it is to convert the default Lua stategy
# into a c++ array.
add_custom_target(strategy
  xxd -i data/strategies/generic-default.lua > ${CMAKE_CURRENT_SOURCE_DIR}/lua/default-strategy.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Converting default strategy to C code" VERBATIM)
