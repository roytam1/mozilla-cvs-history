@echo off

rem * To set up your local build read and customize this batch file.
rem * You shouldn't have to touch anything but "set" statements
rem * Batch file contributed by Lucas Gonze (lucas@gonze.com) and
rem * Rick Ekle (rele@iconect.net)

rem * This stuff forces us to not reset the environment variables more
rem * than once.  It would cause some vars to get longer than 127 chars
rem * (the longest possible batch line) to run this stuff more than once.
if "%1%" == "-force" goto skiptest
if "%MOZBUILD%" == "ALREADYDONE" goto alldone
:skiptest
echo setting up environment...
set MOZBUILD=ALREADYDONE

rem * if this isn't set properly your build will get
rem * "file './config/WIN' not found"
set MOZ_BITS=32

set MOZ_GOLD=1
set MOZ_MEDIUM=1
set NO_SECURITY=1
set NSPR20=1

rem * don't set this if you want to make an optimized release build
set MOZ_DEBUG=1

rem * set if running NT 3.51, don't set otherwise
rem * set MOZ_NT=351

rem * location of resulting executable and totally optional.
rem * If you don't set this, mozilla.exe will be generated into
rem * mozilla\cmd\winfe\mkfiles32\x86dbg (or x86rel for optimized builds)
rem * set MOZ_OUT=d:\bin

rem * top of your tree, drive letter and path, i.e. set d:\mozilla_src.
rem * The topmost directory under this should be mozilla
rem * so if you extracted zip file to z:\foo, this should be z:\foo
set MOZ_SRC=d:\moz\

rem * location of the bin directory containing your GNU tools. The build

rem * looks for MOZ_TOOLS\bin\gmake.exe, so after you download and unpack
rem * wintools.zip, make some directory called "bin", put your new
rem * uname.exe, shmsdos.exe, and gmake.exe in it, and set MOZ_TOOLS to
rem * the place where you put it.
set MOZ_TOOLS=d:

rem * where to find cp.exe and rm.exe, aka gnu tools for windows
set gnuTools=D:\CDK\H-I386~1\bin

rem * the important thing is that your new moz tools should be first
SET PATH=%MOZ_TOOLS%\bin;%gnuTools%;%PATH%

rem * if you are running VC++ 5.0 or higher, this will prevent the build

rem * from trying to pick up uuid2.lib, which only exists in previous
rem * versions of vc++.
set _MSC_VER=1100

rem * vcvars32.bat is created by visual c++ install to
rem * enable command line builds
echo setting up visual c++ environment...
call d:\bin\vcvars32.bat

:alldone
@echo environment set up, building mozilla...

rem * uncomment these to make the script build as well

rem cd %MOZ_SRC%
rem cd mozilla
rem nmake -f client.mak
