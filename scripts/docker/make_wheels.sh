#!/usr/bin/bash

# Script to make python wheels for several versions
# The working directory matters, script should be run with ./make_wheels.sh
# The setup.py file containing the build instructions is located two directories up.

# Below are some instructions on how to create new binary wheels for AOFlagger using this script.

# 1. Modify the file aoflagger/setup.py if you want to apply changes to the wheel file (update version, documentation, ..)
# 2. Go to the scripts/docker folder and run ./make_wheels.sh
# This will create wheels for python 3.6, 3.7, 3.8, 3.9 and 3.10 and copy them outside the docker container to the respective output folders, which will be automatically generated.
# 3. Update the .whl files to pyPI, using 'twine upload package_name.whl'
# You may need to install twine, which can be done with 'pip install twine'
# You need an account on PyPI to upload wheels.

set -euo pipefail

for py_version in 310 39 38 37 36
do
    pushd ../..

    sed -i "s/master_wheel.*/master_wheel${py_version}/" scripts/docker/py310_wheel.docker

    docker build -t aoflagger-py${py_version} -f scripts/docker/py310_wheel.docker .
    dockerid=$(docker create aoflagger-py${py_version})
    docker cp $dockerid:/output/ output-${py_version}
    docker rm ${dockerid}

    popd

done
