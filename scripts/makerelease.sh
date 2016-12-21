#! /bin/bash
if [[ "$1" == "" ]] ; then
    echo Usage: ${0} \<version\>
else
    VERSION="$1"
    echo Check if these values inside src/version.h are right:
    cat ../src/version.h
    curdir=`pwd`
    cd ${curdir%/scripts}
# To use current work tree attributes: "--worktree-attributes"
    git archive --format=tar --prefix=aoflagger-${VERSION}/ master | bzip2 -9 > ${curdir}/aoflagger-${VERSION}.tar.bz2 &&

    mkdir /tmp/buildaoflagger${VERSION}
    cd /tmp/buildaoflagger${VERSION}
    tar -xjvf ${curdir}/aoflagger-${VERSION}.tar.bz2
    cd aoflagger-${VERSION}
    mkdir build
    cd build
    cmake ../ -DPORTABLE=True
    make -j 4 && make check -j 4
    
    cd ${curdir}
    rm -rf /tmp/buildaoflagger${VERSION}
	
    echo Wrote ${curdir}/aoflagger-${VERSION}.tar.bz2
fi
