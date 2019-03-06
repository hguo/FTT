# FTK: The Feature Tracking Kit

FTK is a library that provides building blocks for feature tracking algorithms in scientific datasets.  FTK is header-only and written in C++17.

FTK is still in its early alpha stage, thus examples, documents, and tests are still in progress. 

## Major components in FTK

* Hypermesh: data structures for high-dimensional meshes and mesh elements including *n*-simplices, *n*-cubes, and *n*-prisms; utilities to generalize given 2D/3D structured/unstructured meshes into 3D/4D spacetime meshes

* Numeric: root-find algorithms for inverse interpolations and parallel vector operators in *n*-simplices, *n*-cubes, and simplex-prisms; lightweight linear algebra utilities to support root-finding

* CCL: connected component labeling algorithm for building feature tracking algorithms

* Geometry: utilities to transform connect components to geometry for visualization and analysis

* Tracking graph: data structures to record births, deaths, merges, and splits of features; visualization algorithms for tracking graphs

* IO: interfaces to stage multiple timesteps of the inputs and to store outputs in key-value stores (LevelDB, RocksDB, and Mochi), file systems, and in situ staging areas

## Download, build, and install

You may include FTK headers and call FTK functions directly from your C++ code, because FTK is header-only.  

### Checkout FTK source from Git

```bash
$ git clone https://github.com/hguo/ftk $FTK_SOURCE_DIR
```

### Installation (optional)

You may install FTK to an designated path.  The installation will also generate FTKConfig.cmake in the installation path, such that you can use find_package(FTK) to find and use FTK in your CMakeLists.txt

```bash
$ git clone https://github.com/hguo/ftk $FTK_SOURCE_DIR
$ mkdir $FTK_SOURCE_DIR/build && cd $FTK_SOURCE_DIR/build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$FTK_INSTALL_DIR
$ make install
```

The installed files are organized as follows: 

```bash
$ tree $FTK_INSTALL_DIR
.
├── include
│   ├── ftk
│   │   ├── algorithms
│   │   │   ├── bfs.h
...
│   └── hypermesh
│       ├── ndarray.hh
│       ├── regular_mesh.hh
...
└── lib
    └── cmake
        └── FTKConfig.cmake
```

### Build FTK examples

```bash
$ cd $FTK_SOURCE_DIR/build
$ cmake .. -DFTK_BUILD_EXAMPLES=1
$ make
```

### Include FTK in your CMake project

You may use the FTK installation in your own CMakeLists.txt file:

```cmake
find_package(FTK REQUIRED)
include_directories (${FTK_INCLUDE_DIR})
```

When you configure your build, please specify FTK_DIR with CMake: 

```bash
$ cmake -DFTK_DIR=$FTK_INSTALL_DIR/lib/cmake
```

## Applications that use FTK

* [vortexfinder2](https://github.com/hguo/vortexfinder2): Vortex finder for time-dependent Ginzburg-Landau superconductor simulation data
