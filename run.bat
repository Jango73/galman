@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "EXIT_SUCCESS=0"
set "EXIT_FAILURE=1"
set "VERBOSE_ENABLED=1"
set "VERBOSE_DISABLED=0"

set "ROOT_FOLDER=%~dp0"
set "BUILD_TYPE=Release"
set "VERBOSE_BUILD=%VERBOSE_DISABLED%"

:parse
if "%~1"=="" goto done

if /I "%~1"=="--debug" (
  set "BUILD_TYPE=Debug"
  shift
  goto parse
)
if /I "%~1"=="-d" (
  set "BUILD_TYPE=Debug"
  shift
  goto parse
)
if /I "%~1"=="--release" (
  set "BUILD_TYPE=Release"
  shift
  goto parse
)
if /I "%~1"=="-r" (
  set "BUILD_TYPE=Release"
  shift
  goto parse
)
if /I "%~1"=="--verbose" (
  set "VERBOSE_BUILD=%VERBOSE_ENABLED%"
  shift
  goto parse
)
if /I "%~1"=="-v" (
  set "VERBOSE_BUILD=%VERBOSE_ENABLED%"
  shift
  goto parse
)
if /I "%~1"=="-c" (
  if "%~2"=="" (
    echo Option -c requires an argument. 1>&2
    call :usage 1>&2
    exit /b %EXIT_FAILURE%
  )
  set "BUILD_TYPE=%~2"
  shift
  shift
  goto parse
)
if /I "%~1"=="-h" (
  call :usage
  exit /b %EXIT_SUCCESS%
)
if /I "%~1"=="--help" (
  call :usage
  exit /b %EXIT_SUCCESS%
)

echo Unknown option: %~1 1>&2
call :usage 1>&2
exit /b %EXIT_FAILURE%

:done
if /I not "%BUILD_TYPE%"=="Debug" if /I not "%BUILD_TYPE%"=="Release" (
  echo Invalid build type: %BUILD_TYPE% (use Debug or Release) 1>&2
  exit /b %EXIT_FAILURE%
)

if /I "%BUILD_TYPE%"=="Debug" (
  set "BUILD_FOLDER=%ROOT_FOLDER%build-debug"
) else (
  set "BUILD_FOLDER=%ROOT_FOLDER%build-release"
)
set "LOG_FILE=%ROOT_FOLDER%temp\run.log"

if not exist "%BUILD_FOLDER%" (
  echo [run] %BUILD_FOLDER% not found, running build first...
  if %VERBOSE_BUILD%==%VERBOSE_ENABLED% (
    call "%ROOT_FOLDER%build.bat" -c "%BUILD_TYPE%" -v
  ) else (
    call "%ROOT_FOLDER%build.bat" -c "%BUILD_TYPE%"
  )
)

if not exist "%ROOT_FOLDER%temp" (
  mkdir "%ROOT_FOLDER%temp"
)

echo [run] Logging to %LOG_FILE%
powershell -NoProfile -Command "& { & '%BUILD_FOLDER%\\galman.exe' 2>&1 | Tee-Object -FilePath '%LOG_FILE%' }"
exit /b %EXIT_SUCCESS%

:usage
echo Usage: run.bat [--debug^|--release] [-d^|-r] [--verbose^|-v] [-c Debug^|Release]
echo   --debug,   -d  Build type Debug (default)
echo   --release, -r  Build type Release
echo   --verbose, -v  Verbose build output
echo   -c             Build type (Debug or Release)
exit /b %EXIT_SUCCESS%
