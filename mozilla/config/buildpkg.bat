@echo off
rem The contents of this file are subject to the Netscape Public License
rem Version 1.0 (the "NPL"); you may not use this file except in
rem compliance with the NPL.  You may obtain a copy of the NPL at
rem http://www.mozilla.org/NPL/
rem
rem Software distributed under the NPL is distributed on an "AS IS" basis,
rem WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
rem for the specific language governing rights and limitations under the
rem NPL.
rem
rem The Initial Developer of this code under the NPL is Netscape
rem Communications Corporation.  Portions created by Netscape are
rem Copyright (C) 1998 Netscape Communications Corporation.  All Rights
rem Reserved.
@echo on

@echo off
if not exist %2 echo Warning: %2 does not exist! (you may need to check it out)
if not exist %2 exit 1

pushd %2

goto NO_CAFE

if "%MOZ_CAFE%"=="" goto NO_CAFE

mkdir %MOZ_SRC%\ns\dist\classes\%2
%MOZ_TOOLS%\bin\sj.exe -classpath %MOZ_SRC%\ns\dist\classes;%MOZ_SRC%\ns\sun-java\classsrc -d %MOZ_SRC%\ns\dist\classes *.java 
goto END

:NO_CAFE


%MOZ_TOOLS%\perl5\perl.exe %MOZ_SRC%\ns\config\outofdate.pl -d %MOZ_SRC%\ns\dist\classes\%2 *.java >> %1
%MOZ_TOOLS%\bin\java.exe -argfile %1


:END

popd
