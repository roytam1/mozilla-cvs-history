@echo off
if "%MOZ_TOOLS%" == "" goto no_moz_tools

echo.
echo MOZ_TOOLS is set to %MOZ_TOOLS%
echo.

echo copying exes and dlls to %MOZ_TOOLS%\bin
if not exist %MOZ_TOOLS%\bin\NUL mkdir %MOZ_TOOLS%\bin >NUL
copy bin\x86\* %MOZ_TOOLS%\bin >NUL

echo copying include files to %MOZ_TOOLS%\include
if not exist %MOZ_TOOLS%\include\NUL mkdir %MOZ_TOOLS%\include >NUL
copy include\* %MOZ_TOOLS%\include >NUL

echo copying include files to %MOZ_TOOLS%\include\libIDL
if not exist %MOZ_TOOLS%\include\libIDL\NUL mkdir %MOZ_TOOLS%\include\libIDL >NUL
copy include\libIDL\* %MOZ_TOOLS%\include\libIDL  >NUL

echo copying lib files to %MOZ_TOOLS%\lib 
if not exist %MOZ_TOOLS%\lib\NUL mkdir %MOZ_TOOLS%\lib >NUL
copy lib\* %MOZ_TOOLS%\lib >NUL

echo.
echo done copying
echo.
echo make sure that MOZ_TOOLS\bin is on your path
echo.
goto done

:no_moz_tools
echo.
echo. ERROR!
echo.
echo You need to set MOZ_TOOLS in your environment.
echo MOZ_TOOLS should be the name of a directory that 
echo you create to hold these tools. 
echo.
echo. e.g.
echo.   mkdir c:\moz_tools
echo.   set MOZ_TOOLS=c:\moz_tools
echo.
echo MOZ_TOOLS should be set permanently so that it is 
echo available to the build system whenever mozilla is building.
echo.
echo.
echo Please set MOZ_TOOLS and run install.bat again
echo.

:done