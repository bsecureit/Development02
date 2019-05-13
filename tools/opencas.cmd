@echo off

rem opencon - launch the openconsole binary.
rem Runs the OpenConsole.exe binary generated by the build in the debug directory.
rem    Passes any args along.

if not exist %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\OpenConsole.exe (
    echo Could not locate the OpenConsole.exe in %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%. Double check that it has been built and try again.
    goto :eof
)
if not exist %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\cascadia.exe (
    echo Could not locate the cascadia.exe in %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%. Double check that it has been built and try again.
    goto :eof
)

setlocal
rem Generate a unique name, so that we can debug multiple revisions of the binary at the same time if needed.
set rand_val=%random%
set _r=%random%
set _last_build=%OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%
set copy_dir=OpenConsole\%_r%

(xcopy /Y %_last_build%\OpenConsole.exe %TEMP%\%copy_dir%\OpenConsole.exe*) > nul
(xcopy /Y %_last_build%\cascadia.exe %TEMP%\%copy_dir%\cascadia.exe*) > nul
(xcopy /Y %_last_build%\VtPipeTerm.exe %TEMP%\%copy_dir%\VtPipeTerm.exe*) > nul
(xcopy /Y %_last_build%\Nihilist.exe %TEMP%\%copy_dir%\Nihilist.exe*) > nul
(xcopy /Y %_last_build%\console.dll %TEMP%\%copy_dir%\console.dll*) > nul

start %TEMP%\%copy_dir%\cascadia.exe %*
