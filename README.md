# AST answerer  
A Clang-AST based project for understanding the OpenUSD AST and generating code for a Swift interface to Usd

## Prerequisites

1. Clone LLVM
```
git clone --depth 1 https://github.com/llvm/llvm-project.git
```
(Per [LLVM's instructions](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm), `--depth 1` saves storage space and speeds up checkout time by doing a shallow clone.)  
Currently, I'm using `60fc4ac67a613e4e36cef019fb2d13d70a06cfe8`. 

2. Build LLVM
```
cd llvm-project

cmake -S llvm -B build -G Ninja \
  -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra' \
  -DCMAKE_BUILD_TYPE=Release
    
cmake --build build
```
Note: this will take a while. 

3. Build vanilla OpenUSD without Python
```
python3 build_scripts/build_usd.py \
    --no-python \
    /opt/local/OpenUSD_no-python
```

## Using AST answerer

1. Clone AST answerer  
```
git clone git@github.com:apple/SwiftUsd-ast-answerer.git
```

2. Run CMake
```
cd ast-answerer
cmake -S . -B build \
  -G Xcode \
  -DUSD_SOURCE_REPO_PATH=/PATH/TO/YOUR/USD/SOURCE/REPO \
  -DUSD_INSTALL_NO_PYTHON_PATH=/PATH/TO/YOUR/USD/INSTALL/WITHOUT/PYTHON
```

Replace `/PATH/TO/YOUR/USD/SOURCE/REPO` with the absolute path to a Usd source repo. Replace `/PATH/TO/YOUR/USD/INSTALL/WITHOUT/PYTHON` with the absolute path to your no-python install of OpenUSD, e.g. ` /opt/local/OpenUSD_no-python`.

For example,
```
cmake -S . -B build \
  -G Xcode \
  -DUSD_SOURCE_REPO_PATH=/Users/maddyadams/OpenUSD \
    -DUSD_INSTALL_NO_PYTHON_PATH=/opt/local/OpenUSD_no-python
```

3. Build and run the Xcode project  
Open `build/ast-answerer.xcodeproj`, and run.  
Note: The first time that you run, the compiler will have to parse and build the AST for all the source files in Usd, which takes about one minute. It serializes the AST to disk, so subsequent runs should load it and start processing the AST almost immediately. 

## Documentation
See `DOC.md`, and documentation in the source code. 
