echo off
REM  Check out, build and deliever the CCK stuff
REM  3/16/99 Frank Petitta   Netscape Communications Corp.
REM
REM  Basic operation outline:
REM    _MSC_VER and MOZ_DEBUG are the only System Vars used(currently)
REM    IF _MSC_VER doesnt equal 1200 then VC+ is not version 6.0,
REM    6.0 is the standard so the build will not happen if _MSC_VER is
REM    any value other than 1200!
REM    System var MOZ_DEBUG is used to detemine Debug or Non-Debug builds
REM
REM  * I hate this Batch CRAP, I going to use this as a temp and write this again in PERL!!!*
REM

REM echo on

:SetUp

REM   Set all of environ vars for the build process
set BuildGood=0 
call C:\"Program Files"\"Microsoft Visual Studio"\VC98\Bin\vcvars32.bat

REM   Set/get Sys vars to make sure you are doing the right thing.
REM   Make sure we are building with the right version of VC+ (6.0)
if  not "%_MSC_VER%"=="1200" set ErrorType=1
if  not "%_MSC_VER%"=="1200" goto Errors

D:
cd\builds

REM remove the mozilla directory
echo y | rd /s mozilla

REM check out mozilla/cck
cvs co mozilla/cck

REM  Copy the build files to the build directory
C:
cd\cckscripts
copy WizardMachine.dep D:\builds\mozilla\cck\driver
copy WizardMachine.mak D:\builds\mozilla\cck\driver

D:
cd\builds\mozilla\cck\driver

REM Send Pull completion notification
echo.CCK source pull complete.  >> tempfile.txt
blat tempfile.txt  -t page-petitta@netscape.com -s "CCK Pull Notification" -i Undertaker
if exist tempfile.txt del tempfile.txt

REM build the damn thing, then send notification if the exe is there.
if "%MOZ_DEBUG%"=="1" NMAKE /f "WizardMachine.mak" CFG="WizardMachine - Win32 Debug"
if "%MOZ_DEBUG%"=="0" NMAKE /f "WizardMachine.mak" CFG="WizardMachine - Win32 Release"

REM See if the target is there
if "%MOZ_DEBUG%"=="1" if exist D:\builds\mozilla\cck\driver\debug\wizardmachine.exe set BuildGood=1
if "%MOZ_DEBUG%"=="0" if exist D:\builds\mozilla\cck\driver\release\wizardmachine.exe set BuildGood=1

REM If the target is there then do the right thing, Mail notification then upload it.
echo.CCK build complete and verified.  >> tempfile.txt
if "%BuildGood%"=="1" blat tempfile.txt -t page-petitta@netscape.com -s "CCK Build Notification" -i Undertaker
if exist tempfile.txt del tempfile.txt

REM Houston we have a problem, abort, abort!!!!!
if "%BuildGood%" =="0" echo.CCK build died, casualty assesment.  >> tempfile.txt
if "%BuildGood%" =="0" blat tempfile.txt -t page-petitta@netscape.com -s "CCK Build Notification" -i Undertaker
if exist tempfile.txt del tempfile.txt
if "%BuildGood%" =="0" set ErrorType=2
if "%BuildGood%" =="0" goto Errors

:BuildNumber
REM Get the build date to label the folder we create on upload.
C:
Perl C:\CCKScripts\date.pl
call C:\CCKScripts\bdate.bat
if "%BuildID%" == "" goto set ErrorType = 3
if "%BuildID%" == "" goto EndOfScript
O:
if "%MOZ_DEBUG%"=="1" md \products\client\cck\cck50\debug\"%BuildID%"
if "%MOZ_DEBUG%"=="0" md \products\client\cck\cck50\release\"%BuildID%"

REM  Put it where we all can get it.
:UploadIt
if "%MOZ_DEBUG%"=="1" goto DebugUpLoad
if "%MOZ_DEBUG%"=="0" goto ReleaseUpload

:DebugUpLoad
    D:
    cd\builds\mozilla\cck\driver\Debug
    copy *.exe O:\products\client\cck\cck50\debug\"%BuildID%"
    cd\builds\mozilla\cck\cckwiz\inifiles
    copy cck.ini O:\products\client\cck\cck50\debug\"%BuildID%"
    goto EndOfScript
	
:ReleaseUpload
    D:
    cd\builds\mozilla\cck\driver\Release
    copy *.exe O:\products\client\cck\cck50\release\"%BuildID%"
    cd\builds\mozilla\cck\cckwiz\inifiles
    copy cck.ini O:\products\client\cck\cck50\release\"%BuildID%"
    goto EndOfScript

goto EndOfScript

REM  Capture the errors, do something smart with them.
:Errors
if "%ErrorType%"=="1" echo.  Incorrect version of VC+, not 6.0! Script halted!!

if "%ErrorType%"=="2" echo.  The build blew up in your face, get to work laughing boy!!

if "%ErrorType%"=="3" echo.  BuildNumber Generation Failed

if "%ErrorType%"=="4" echo.  Busted4

if "%ErrorType%"=="5" echo.  Busted5


REM  Like , duh.  Oh my gosh and all that stuff!
:EndOfScript
echo.   This is the end, my friend.   My only friend, the end......
