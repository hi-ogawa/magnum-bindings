if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2019" call "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
rem SDL2 has the DLL in lib/x64, and in the static build it's imported so the
rem DLL has to be found.
set PATH=%APPVEYOR_BUILD_FOLDER%\deps\bin;%APPVEYOR_BUILD_FOLDER%\SDL\lib\x64;%PATH%

rem Build pybind11. Downloaded in the appveyor.yml script.
cd pybind11-2.3.0 || exit /b
mkdir -p build && cd build || exit /b
cmake .. ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DPYBIND11_PYTHON_VERSION=3.6 ^
    -DPYBIND11_TEST=OFF ^
    -G Ninja || exit /b
ninja install || exit /b
cd .. && cd ..

rem Build Corrade
git clone --depth 1 git://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DBUILD_DEPRECATED=OFF ^
    -DBUILD_STATIC=%BUILD_STATIC% ^
    -DWITH_INTERCONNECT=OFF ^
    -DWITH_PLUGINMANAGER=ON ^
    -DWITH_TESTSUITE=OFF ^
    -DUTILITY_USE_ANSI_COLORS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 git://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DBUILD_DEPRECATED=OFF ^
    -DBUILD_STATIC=%BUILD_STATIC% ^
    -DWITH_AUDIO=OFF ^
    -DWITH_DEBUGTOOLS=OFF ^
    -DWITH_GL=ON ^
    -DWITH_MESHTOOLS=ON ^
    -DWITH_PRIMITIVES=ON ^
    -DWITH_SCENEGRAPH=ON ^
    -DWITH_SHADERS=ON ^
    -DWITH_TEXT=OFF ^
    -DWITH_TEXTURETOOLS=ON ^
    -DWITH_TRADE=ON ^
    -DWITH_VK=OFF ^
    -DWITH_SDL2APPLICATION=ON ^
    -DWITH_GLFWAPPLICATION=ON ^
    -DWITH_WINDOWLESSWGLAPPLICATION=ON ^
    -DWITH_ANYIMAGEIMPORTER=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Plugins
git clone --depth 1 git://github.com/mosra/magnum-plugins.git || exit /b
cd magnum-plugins || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DBUILD_STATIC=%BUILD_STATIC% ^
    -DWITH_DDSIMPORTER=ON ^
    -DWITH_STBIMAGEIMPORTER=ON ^
    -DWITH_TINYGLTFIMPORTER=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build. BUILD_GL_TESTS is enabled just to be sure, it should not be needed
rem by any plugin.
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DPYBIND11_PYTHON_VERSION=3.6 ^
    -DWITH_PYTHON=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b

rem Verify the setuptools install
cd src/python || exit /b
python setup.py install --root="%APPVEYOR_BUILD_FOLDER%/install" || exit /b

rem Run tests & gather coverage
cd ../../../src/python/corrade || exit /b
coverage run -m unittest -v || exit /b
cp .coverage ../.coverage.corrade || exit /b

cd ../magnum || exit /b
set MAGNUM_SKIP_GL_TESTS=ON
coverage run -m unittest -v || exit /b
cp .coverage ../.coverage.magnum || exit /b

rem Test docstring validity
cd ../../../doc/python || exit /b
set PYTHONPATH="%APPVEYOR_BUILD_FOLDER%/build/src/python"
rem We (deliberately) don't have numpy installed, so this would fail
rem TODO: any idea how to fix this? doctest has SKIPs but not conditional ones
rem python -m doctest -v *.rst || exit /b

rem Upload coverage
cd ../../src/python || exit /b
coverage combine || exit /b
rem TODO: Currently disabled because I can't seem to convince it to relocate
rem the paths via codecov.yml: https://github.com/mosra/magnum-bindings/pull/3
rem codecov -X gcov || exit /b
