# vb_UGens
a modest collection of UGens for SuperCollider https://supercollider.github.io/ 

Volker Böhm, 2020 https://vboehm.net



## Building

For building this collection of UGens/plugins you need the SuperCollider sources. Get them from here https://github.com/supercollider/supercollider. You don't need to compile them, though!

Then clone the vb_UGens repository and its submodules:

```
git clone --recurse-submodules https://github.com/v7b1/vb_UGens.git
```

To build all projects/UGens in one go:

```
cd vb_UGens
mkdir build
cd build
cmake .. -DSC_PATH="path/to/SC/sources" -DCMAKE_BUILD_TYPE="Release"
make install
```

where `"path/to/SC/sources"` is your local path to the SuperCollider sources.

You should find a newly created folder vbUGens in the build folder. (If you prefer a different install location, you can specify a different directory with `-DCMAKE_INSTALL_PREFIX="path/to/my/folder"`.)
Copy this to your SC extensions folder and recompile the class library from SuperCollider.app.



Single projects can be built by:

```
cd vb_UGens/projects/<whateverproject>
mkdir build
cd build
cmake .. -DSC_PATH="path/to/SC/sources" -DCMAKE_BUILD_TYPE="Release"
make
```



For compiled mac versions see https://vboehm.net/downloads