@ECHO OFF
SETLOCAL EnableDelayedExpansion

PUSHD "%~dp0"

REM Check for Visual Studio x64 dev environment
if /I NOT "%VSCMD_ARG_TGT_ARCH%"=="x64" (
	echo.
	echo ##############################################
    echo Visual Studio x64 environment not initialized!
	echo ##############################################
    goto :Failed
)

REM Check for 3rdParty/_dist_* folder, call dependency build script if missing
if NOT EXIST "%~dp03rdParty/_dist_v141/Release" (
	PUSHD "%~dp03rdParty"
	call build_dependencies.cmd
	IF ERRORLEVEL 1 goto FailedDeps 
	POPD
)

SET VER_LIST=2020 2021 2022 2023 2024 2025 2026 2027
FOR %%v IN (%VER_LIST%) DO (
	SET CONFIG=Release-Max%%v
	ECHO Building '!CONFIG!'
	msbuild KHRglTF.sln /p:Configuration=!CONFIG! /p:Platform=x64 /v:minimal
	IF ERRORLEVEL 1 GOTO Failed
)

popd
exit /b

:Failed
	echo.
	echo ERROR: failed to build !CONFIG!
	echo.
	popd
	exit /b 1
	
:FailedDeps
	echo.
	echo Error: Dependency build failed with errorlevel %ERRORLEVEL%
	echo.
	popd
	popd
	exit /b 2