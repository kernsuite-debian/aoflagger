cd ..
cpplint --verbose=1 --filter=-legal/copyright,-build/header_guard,-build/c++11,-build/include_subdir,-build/include_order,-readability/casting,-runtime/int --recursive --exclude=build --exclude=external --extensions=cpp,h .
cd scripts
