# vb_UGens
a modest collection of UGens for SuperCollider https://supercollider.github.io/ 

requires fftw3 on Linux.

steps to compile:

- cd into the project folder you want to build
- mkdir build
- cd build
- cmake -DSC_PATH="path/to/sc-src" -DCMAKE_BUILD_TYPE=RELEASE ..
- make

for compiled mac versions see https://vboehm.net/downloads
