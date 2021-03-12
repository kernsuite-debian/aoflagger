Installation on Redhat
======================

This page describes installing AOFlagger 2.14 on RHEL 7.6
(Thanks to Leonardo Saavedra from NRAO)

aoflagger depends on many packages, most of then are provided by RHEL 7.6, but you will need to install the modern version of them. In particular: **fftw** and **boost**

* https://gitlab.com/aroffringa/aoflagger
* http://www.fftw.org
* https://github.com/casacore/casacore
* https://www.boost.org/

Install RPMs
------------

You need install hdf from epel (or install from the sources as detailed below), so you need to set EPEL repo.

.. code-block :: bash
    
    yum install hdf hdf-devel
    yum install gtkmm30 gtkmm30-devel

Set environment
---------------

.. code-block :: bash
    
    mkdir ~/Downloads2/aoflagger
    cd ~/Downloads2/aoflagger
    wget -c http://www.fftw.org/fftw-3.3.8.tar.gz
    wget -c https://github.com/casacore/casacore/archive/v3.1.1.tar.gz
    wget -c https://sourceforge.net/projects/aoflagger/files/aoflagger-2.14.0/aoflagger-2.14.0.tar.bz2
    wget -c https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
    LOCAL5=/opt/local5

Install FFTW 3.3.8
------------------

.. code-block :: bash
    
    tar xzvf fftw-3.3.8.tar.gz
    cd fftw-3.3.8/
    ./configure --prefix=${LOCAL5} --enable-threads --enable-openmp  --enable-shared
    make -j `nproc`
    make install
    cd ..

Install Casacore 3.3.1
----------------------

Set new environment
^^^^^^^^^^^^^^^^^^^

.. code-block :: bash
    
    export LD_LIBRARY_PATH=${LOCAL5}/lib:$LD_LIBRARY_PATH
    export PATH=$PATH:${LOCAL5}/bin

.. code-block :: bash
    
    tar xzvf v3.1.1.tar.gz
    cd casacore-3.1.1/
    mkdir build
    cd build
    cmake ../ -DCMAKE_PREFIX_PATH=${LOCAL5}
    make -j `nproc`
    make install
    vim cmake_install.cmake <-- modified CMAKE_INSTALL_PREFIX to point to ${LOCAL5}
    make install

Install boost 1.68
------------------

Previous to compile boost, you have to make sure that your python installation uses a 4-byte representation for Unicode characters.
To check that, you can run the following script, if it reports 'UCS2 Build', you have to either install a new python or use other python version.

https://docs.python.org/2/faq/extending.html#can-i-create-an-object-class-with-some-methods-implemented-in-c-and-others-in-python-e-g-through-inheritance

.. code-block :: bash
    
    python
    cat CheckUnide.py
    import sys

    if sys.maxunicode > 65535:
        print ('UCS4 Build')
    else:
        print ('UCS2 Build')

.. code-block :: bash
    
    python CheckUnide.py
    UCS4 Build

.. code-block :: bash
    
    tar xzvf boost_1_68_0.tar.gz
    cd boost_1_68_0
    ./bootstrap.sh --with-python=python --prefix=${LOCAL5} --with-icu
    ./b2
    ./b2 link=shared install --prefix=${LOCAL5}
    (cd ${LOCAL5}/lib ; ln -s libboost_python27.so  libboost_python.so)
    cd ..

Install HDF5
------------

This is optional, you can skip if you have hdf5 from Epel.
Download hdf-1.10.5.tar from https://www.hdfgroup.org/downloads/hdf5/source-code/

.. code-block :: bash
    
    ./configure --enable-cxx --with-szlib --enable-shared --enable-hl --disable-silent-rules --enable-fortran --enable-fortran2003 --prefix=${LOCAL5}
    make -j `nproc`
    make check
    make install
    make installcheck

Install AOFlagger 2.14
----------------------

.. code-block :: bash
    
    tar xjvf aoflagger-2.14.0.tar.bz2
    cd aoflagger-2.14.0
    mkdir build
    cd build
    cmake ../ -DCMAKE_PREFIX_PATH=${LOCAL5} -DCMAKE_INSTALL_PREFIX={LOCAL5}
    make -j `nproc`
    vim cmake_install.cmake  <-- modified (or make sure) CMAKE_INSTALL_PREFIX  point to ${LOCAL5}
    make install

Check aoflagger version
-----------------------

.. code-block :: bash
    
    which python
    /opt/local5/bin/python
    aoflagger --version
    AOFlagger 2.14.0 (2019-02-14)
