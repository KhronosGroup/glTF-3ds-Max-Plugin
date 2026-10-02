@ECHO off
SETLOCAL enabledelayedexpansion

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

REM Loop through existing 3ds Max SDKs (2020 -> 2027)
for /L %%V in (2020,1,2027) do (

    REM Build the environment variable name
    set "SDKVAR=ADSK_3DSMAX_SDK_%%V"
    
    REM Resolve dynamic env var value using delayed expansion
    call set "SDKPATH=%%!SDKVAR!%%"
    
    REM Check if resolved value is non-empty
    if defined SDKPATH (
		SET CONFIG=Release-Max%%V
		ECHO Building '!CONFIG!'

		msbuild KHRglTF.sln /p:Configuration=!CONFIG! /p:Platform=x64 /v:minimal

		IF ERRORLEVEL 1 GOTO :Failed		
    ) else (
        echo Skipped !SDKVAR! ( 3ds Max SDK unavailable )
    )
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