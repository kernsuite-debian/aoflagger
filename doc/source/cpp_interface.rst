C++ interface
=============

AOFlagger provides external C++ and Python Application Programming Interfaces (API)
to call the aoflagger from those languages.

.. note::
   These interfaces are not used to design a custom flagging strategy.
   Custom flagging strategies make use of an internal Lua interface, which are described
   in the :doc:`chapter on designing strategies <designing_strategies>`.
   The external interfaces described here make
   it possible to "push data" through AOFlagger from other software.

These external interfaces allow, for example, the integration of the AOFlagger inside a pipeline
and observatory. To use the interface, the C++ header file "``aoflagger.h``"
is installed as part of the package, and can be ``#include``\ d in a program's source code.
Additionally, the program needs to be linked with libaoflagger. The best way to link
a C++ program to aoflagger, is by using cmake's ``find_package`` utility, which can
also handle versioning, etc. For example

.. code-block:: cmake

   find_package(AOFlagger 3 REQUIRED)
   include_directories(${AOFLAGGER_INCLUDE_DIR})
   target_link_libraries(yoursoftware ${AOFLAGGER_LIB})

Reference
~~~~~~~~~

The documentation for the external C++ API is automatically extracted from the code
using Doxygen. 
The C++ API reference that summarizes all functions, classes, etc. can be found here:

 * http://www.andreoffringa.org/aoflagger/doxygen/

The Python interface mirrors the C++ API, and these pages can therefore also be
used as reference for the Python interface.
