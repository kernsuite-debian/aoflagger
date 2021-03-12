Installation
============

.. toctree::
   :maxdepth: 2
   :hidden:
 
   installation-redhat

The easiest way to install AOFlagger is from a binary package. Binary packages for 
Debian and Ubuntu are available. For other distributions, AOFlagger can be compiled
from source.

Binary distributions
^^^^^^^^^^^^^^^^^^^^

Some distributions provide binaries. These might not always be the latest version,
but if you don't require specific features from later versions, it is highly
recommended to install aoflagger from binaries. If you do need a newer version,
or if your distribution doesn't provide a binary, then AOFlagger should be
compiled from source.

Debian/Ubuntu package
---------------------

Debian and Ubuntu have binaries available.
To install AOFlagger on Debian/Ubuntu system, run:

.. code-block :: bash
    
    sudo apt-get install aoflagger

Kern
----

`Kern <https://kernsuite.info/>`_ is a distribution that provides
the most commonly used astronomical software. It also provides AOFlagger, but is
currently out of date.

Compilation
^^^^^^^^^^^

The AOFlagger source-code repository can be found here: https://gitlab.com/aroffringa/aoflagger

If you are compiling AOFlagger for the first time, take this into account:

 * It is easiest to install AOFlagger on a Linux machine. Ubuntu, Debian and derived distributions are most easy to install to.
   Red Hat and derived distributions (e.g. CentOS) are harder, in particular when installing the graphical interface.
   Installation on MacOS is possible too, but installing the dependencies in the right way can be a lot of work.
   A Docker or Singularity container might in that case be a solution.
 * It is easier when you can get root access to install distribution packages. This is particularly the case if you want the graphical interface. 
 * If you run into problems compiling the package, please ask colleagues or a local adminstrator first.
 * If you can't solve an issue, please mail me. Please don't mail me with problems to install or compile Casacore.
   I get a lot of mail about the packages I wrote, so be sure that you have tried a few options yourself first.
   That said, if you cannot solve the issue, I'm more than happy to help.

On Red Hat
----------

The following document lists some instructions that can be helpful for installing AOFlagger on Red Hat: :doc:`Installation on Redhat <installation-redhat>`.

Prerequisites
-------------

The following libraries are required to build the AOFlagger:

* `Casacore <https://github.com/casacore/casacore>`_, for opening measurement sets.
  Version >=2.0 is required.
* `Lua <https://www.lua.org/>`_, for scripting.
* `FFTW <http://www.fftw.org/>`_, used to perform Fourier transformations.
* `Boost <http://www.boost.org/>`_, used for date and time calculations, Python connections and some other general functionalities.
* `LAPACK <http://www.netlib.org/lapack/>`_, for linear algebra, such as singular value decomposition.
* `CFITSIO <http://heasarc.nasa.gov/fitsio/>`_, for reading and writing FITS files.
* `Gtkmm <http://www.gtkmm.org/>`_ (only for rfigui and other graphical programs, but you probably want that). Version 3.10 or later is required (since AOFlagger 2.7; AOFlagger version 2.5 required gtkmm >= 3.0; earlier versions required gtkmm 2.x).
* `GNU science library <https://www.gnu.org/software/gsl/>`_
* `libpng <http://www.libpng.org/>`_
* `libxml <http://xmlsoft.org/>`_, used for saving scripts / configurations.
* Python 3 libraries.

The Gtkmm libraries and headers are required for the gui but not for the console executable. If they are not present, cmake will warn about it, but the console programs can still be compiled.

To compile Gtkmm "by hand" is discouraged; it has many dependencies. It is better to use a package manager (or ask your system
administrator). Here is an apt command for Debian / Ubuntu system
that installs all dependencies:

.. code-block:: bash

    apt-get install   \
      cmake \
      build-essential \
      pkg-config \
      casacore-data casacore-dev \
      libblas-dev liblapack-dev \
      python3 \
      libboost-date-time-dev \
      libboost-filesystem-dev \
      libboost-system-dev \
      libboost-test-dev \
      libcfitsio-dev \
      libfftw3-dev \
      libgsl-dev \
      libgtkmm-3.0-dev \
      liblua5.3-dev \
      libpng-dev  \
      libpython3-dev \
      libxml2-dev \

.. note::
    Certain versions of Ubuntu ship the Lapack packages as "liblapack3gf".

Compilation commands
--------------------

The following commands are a quick summary of compilation and installation, assuming
your working directory is the AOFlagger root directory:

.. code-block:: bash
    
    mkdir build
    cd build/
    cmake ../
    make -j 8
    sudo make install

This will normally work when you have installed all libraries in their default locations. If you have not done so, you might need to tell cmake where the libraries are. The easiest way to do this is by adding search paths to cmake, e.g.:

.. code-block:: bash
    
    cmake ../ -DCMAKE_PREFIX_PATH=/opt/cep/casacore/

This will include ``/opt/cep/casacore`` to the search path. Multiple paths can be specified by separating them with a colon ':'. Alternatively, if for some reason you need to specify individual libraries, the syntax is:

.. code-block:: bash
    
    cmake ../ -DCASA_INCLUDE_DIR=/home/anoko/casacore/include -DCASA_MS_LIB=.. [..etc..]

One can set the ``-D...`` defines to the appropriate paths when CMake can not automatically find them.
The 'make install' statement will install the binaries :doc:`aoflagger <using_aoflagger>`,
:doc:`rfigui <using_rfigui>`, :doc:`aoqplot <using_aoqplot>` and :doc:`aoquality <using_aoquality>`, and the library ``libaoflagger`` together with its header ``aoflagger.h``. If you do not have administrator or sudo rights, you can install the software to a different location by specifying a ``CMAKE_INSTALL_PREFIX`` when calling cmake, e.g.:

.. code-block:: bash
    
    cmake ../ -DCMAKE_INSTALL_PREFIX=/home/esmeralda/

Building the Python interface
-----------------------------

The Python lib that provides the :doc:`Python interface <python_interface>` is build by default. The
Python aoflagger lib should be installed in your Python path after ``make install``.
Unlike the C++ library, the Python library does not start with ``lib``. E.g., on my
machine, the two libraries are:

 * ``lib/libaoflagger.so.0`` -> C++ library
 * ``lib/aoflagger.cpython-38-x86_64-linux-gnu.so`` -> Python library

Bottomline, if a machine is set up normally, one should be able to
run ``import aoflagger`` from Python after installing AOFlagger.
 
Building a machine independent binary
-------------------------------------

To build AOFlagger so that it can run on any machine, add ``-DPORTABLE=True`` to the cmake command line:

.. code-block:: bash

     cmake ../ -DPORTABLE=True
      
