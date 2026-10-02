@echo off
setlocal EnableDelayedExpansion

:: Check for Visual Studio x64 dev environment
if /I NOT "%VSCMD_ARG_TGT_ARCH%"=="x64" (
	echo.
	echo ##############################################
    echo Visual Studio x64 environment not initialized!
	echo ##############################################
    goto :Failed
)

:: the MSVC toolset version to use for building the libraries
:: has to match the minimal targeted 3ds Max version's SDK requirement ( v141 for 3ds Max 2020 - 2022 )
set "VC_TOOLSET=v141"

:: to hold the built libraries
set "DIST_DIR=%~dp0_dist_%VC_TOOLSET%"
:: to hold the downloaded zip archives
set "ARCHIVE_DIR=%~dp0_archives"

:: skip tinygltf as it has been ( slightly ) modified
:: modified tinygltf has been moved to the KHRglTFEporter Project folder, as its only used there
set "CLONE_TINYGLTF=0"

echo.
echo =====================================================
echo Building 3rd party dependencies
echo =====================================================
echo.

if "%CLONE_TINYGLTF%"=="1" (
	echo.
	echo "Cloning 'tinygltf' ..."
	echo =====================================================
	call :CLONE "tinygltf" "https://github.com/syoyo/tinygltf.git" "v3.0.1"
	if errorlevel 1 goto :ERROR
)

echo.
echo "Cloning and building 'draco' ..."
echo =====================================================
call :CLONE "draco" "https://github.com/Google/draco.git" "1.5.7"
if errorlevel 1 goto :ERROR
call :CONFIGURE "draco" "-DCMAKE_INSTALL_PREFIX=installed"
if errorlevel 1 goto :ERROR
call :BUILD "draco" "INSTALL"
if errorlevel 1 goto :ERROR

echo.
echo "Cloning and building 'jsoncpp' ..."
echo =====================================================
call :CLONE "jsoncpp" "https://github.com/open-source-parsers/jsoncpp.git" "1.9.8"
if errorlevel 1 goto :ERROR
call :CONFIGURE "jsoncpp" "-DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF -DJSONCPP_WITH_CMAKE_PACKAGE=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_OBJECT_LIBS=OFF -DBUILD_STATIC_LIBS=ON"
if errorlevel 1 goto :ERROR
call :BUILD "jsoncpp" "jsoncpp_static"
if errorlevel 1 goto :ERROR

echo.
echo "Cloning and building 'KTX' ..."
echo =====================================================
call :CLONE "KTX-Software" "https://github.com/KhronosGroup/KTX-Software.git" "v4.1.0"
if errorlevel 1 goto :ERROR
call :CONFIGURE "KTX-Software" "-DKTX_FEATURE_STATIC_LIBRARY=ON"
if errorlevel 1 goto :ERROR
call :BUILD "KTX-Software" "ktx"
if errorlevel 1 goto :ERROR

echo.
echo "Cloning and building 'libwebp' ..."
echo =====================================================
call :CLONE "libwebp" "https://github.com/webmproject/libwebp.git" "v1.6.0"
if errorlevel 1 goto :ERROR
call :CONFIGURE "libwebp"
if errorlevel 1 goto :ERROR
call :BUILD "libwebp" "webp" "sharpyuv" "webpdecoder"
if errorlevel 1 goto :ERROR

echo.
echo =====================================================
echo SUCCESS: all operations finished successfully.
echo =====================================================
echo.
exit /b 0


:: =====================================================
:: CLONE
:: %~1 = target folder
:: %~2 = git repository
:: %~3 = tag
:: =====================================================

:CLONE
  if not exist "%~1" (
  	:: Check if git is available
	where git >nul 2>&1
	if errorlevel 1 (
		echo ERROR: Git is not available in PATH. Aborting...
		exit /b 1
	)  
    git -c advice.detachedHead=false clone --depth 1 -b "%~3" "%~2" "%~1"
    if errorlevel 1 (
        echo ERROR: failed to clone %~1
        exit /b 1
    )
    :: additionally also download the archive-zip for backup purposes 
    if not exist "%ARCHIVE_DIR%" mkdir "%ARCHIVE_DIR%"
    set "ZIP_URL=%~2"
    :: strip .git and append archive path
    set "ZIP_URL=!ZIP_URL:.git=!/archive/refs/tags/%~3.zip"
    echo !ZIP_URL!
    echo Downloading archive for %~2 %~3...
    curl -L -f --progress-bar -o "%ARCHIVE_DIR%/%~1-%~3.zip" "!ZIP_URL!"
    if errorlevel 1 (
      echo ERROR: Failed to download archive for %~2 %~3
      exit /b 1
    ) 
  )
  exit /b 0

:: =====================================================
:: CONFIGURE
:: %~1 = source folder
:: %~2 and higher = CMake options
:: =====================================================

:CONFIGURE
  :: Check if cmake is available
  where cmake >nul 2>&1
  if errorlevel 1 (
    echo ERROR: cmake is not available in PATH. Aborting...
    exit /b 1
  )  

  set "SRC_DIR=%~1"
  :: Strip source directory argument.
  shift

  set "CMAKE_OPTIONS="

  :CONFIGURE_OPTIONS
    if "%~1"=="" goto :RUN_CONFIGURE
    set "CMAKE_OPTIONS=%CMAKE_OPTIONS% %1"
    shift
    goto :CONFIGURE_OPTIONS

  :RUN_CONFIGURE
    echo.
    echo Configuring "%SRC_DIR%" ...
    pushd "%SRC_DIR%"
    if errorlevel 1 (
        echo ERROR: could not enter "%SRC_DIR%"
        exit /b 1
    )
    cmake -S . -B "build_%VC_TOOLSET%" ^
        -G "Visual Studio 17 2022" ^
        -A x64 ^
        -T "%VC_TOOLSET%" ^
        -D"CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=%DIST_DIR%\Debug" ^
        -D"CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=%DIST_DIR%\Release" ^
        -DCMAKE_CXX_STANDARD=17 ^
        -DCMAKE_CXX_STANDARD_REQUIRED=ON ^
        %CMAKE_OPTIONS%
    popd
    exit /b %ERRORLEVEL%

:: =====================================================
:: BUILD
:: %~1 = source folder
:: %~2 and higher = CMake targets
::
:: If no targets are specified, the default target is built.
:: =====================================================

:BUILD
  :: Check if cmake is available
  where cmake >nul 2>&1
  if errorlevel 1 (
    echo ERROR: cmake is not available in PATH. Aborting...
    exit /b 1
  )  
  
  set "SRC_DIR=%~1"
  :: Strip source directory argument.
  shift
  set "TARGETS="

  :EXTRACT_TARGETS
    if "%~1"=="" goto :RUN_BUILD
    set "TARGETS=%TARGETS% %1"
    shift
    goto :EXTRACT_TARGETS

  :RUN_BUILD
    echo.
    echo Building "%SRC_DIR%" ...
    pushd "%SRC_DIR%"
    if not "%TARGETS%"=="" (
      cmake --build build_%VC_TOOLSET% --config Debug	--target %TARGETS%
      cmake --build build_%VC_TOOLSET% --config Release	--target %TARGETS%
    ) else (
      cmake --build build_%VC_TOOLSET% --config Debug
      cmake --build build_%VC_TOOLSET% --config Release
    )
    popd
    exit /b %ERRORLEVEL%


:: =====================================================
:: ERROR
:: =====================================================

:ERROR
  echo =====================================================
  echo ERROR: dependency build failed.
  echo =====================================================
  echo.

  exit /b 1