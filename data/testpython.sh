export LD_LIBRARY_PATH=`pwd`/../build
export PYTHONPATH=`pwd`/../build/python:${PYTHONPATH}
python3 falsepositives.py
