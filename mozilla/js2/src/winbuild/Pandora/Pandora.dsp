# Microsoft Developer Studio Project File - Name="Pandora" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Pandora - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Pandora.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Pandora.mak" CFG="Pandora - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Pandora - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Pandora - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Pandora - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PANDORA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../RegExp" /I "\ns_trunk\mozilla\modules\plugin\base\public" /I "\ns_trunk\mozilla\nsprpub\pr\include" /I "\ns_trunk\mozilla\dist\include" /I "\ns_trunk\mozilla\dist\include\java" /I "\ns_trunk\mozilla\dist\include\nspr" /I "\ns_trunk\mozilla\dist\include\xpcom" /I "\ns_trunk\mozilla\modules\plugin\tools\sdk\samples\include" /I "\ns_trunk\mozilla\dist\include\xpconnect" /I "\ns_trunk\mozilla\js\src" /I "\ns_trunk\mozilla\js\src\xpconnect\src" /I "\ns_trunk\mozilla\dist\include\string" /I "\ns_trunk\mozilla\dist\include\caps" /I "\ns_trunk\mozilla\content\base\public" /I "\ns_trunk\mozilla\widget\public" /I "\ns_trunk\mozilla\dist\sdk\dom\include" /I "\ns_trunk\mozilla\dist\include\dom" /I "\ns_trunk\mozilla\dom\src\base" /I "\ns_trunk\mozilla\dist\include\widget" /I "\ns_trunk\mozilla\dist\include\layout" /I "\ns_trunk\mozilla\dist\include\gfx" /I "\ns_trunk\mozilla\dist\include\necko" /I "\ns_trunk\mozilla\dist\include\xuldoc" /I "\ns_trunk\mozilla\dist\include\docshell" /I "\ns_trunk\mozilla\dist\include\webbrwsr" /I "\ns_trunk\mozilla\dist\include\sidebar" /I "\ns_trunk\mozilla\dist\include\plugin" /I "\ns_trunk\mozilla\dist\include\content" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PANDORA_EXPORTS" /D "XP_PC" /D "EPIMETHEUS" /D "IS_LITTLE_ENDIAN" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release/NPandora.dll"

!ELSEIF  "$(CFG)" == "Pandora - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PANDORA_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../../RegExp" /I "\ns_trunk\mozilla\modules\plugin\base\public" /I "\ns_trunk\mozilla\nsprpub\pr\include" /I "\ns_trunk\mozilla\dist\include" /I "\ns_trunk\mozilla\dist\include\java" /I "\ns_trunk\mozilla\dist\include\nspr" /I "\ns_trunk\mozilla\dist\include\xpcom" /I "\ns_trunk\mozilla\modules\plugin\tools\sdk\samples\include" /I "\ns_trunk\mozilla\dist\include\xpconnect" /I "\ns_trunk\mozilla\js\src" /I "\ns_trunk\mozilla\js\src\xpconnect\src" /I "\ns_trunk\mozilla\dist\include\string" /I "\ns_trunk\mozilla\dist\include\caps" /I "\ns_trunk\mozilla\content\base\public" /I "\ns_trunk\mozilla\widget\public" /I "\ns_trunk\mozilla\dist\sdk\dom\include" /I "\ns_trunk\mozilla\dist\include\dom" /I "\ns_trunk\mozilla\dom\src\base" /I "\ns_trunk\mozilla\dist\include\widget" /I "\ns_trunk\mozilla\dist\include\layout" /I "\ns_trunk\mozilla\dist\include\gfx" /I "\ns_trunk\mozilla\dist\include\necko" /I "\ns_trunk\mozilla\dist\include\xuldoc" /I "\ns_trunk\mozilla\dist\include\docshell" /I "\ns_trunk\mozilla\dist\include\webbrwsr" /I "\ns_trunk\mozilla\dist\include\sidebar" /I "\ns_trunk\mozilla\dist\include\plugin" /I "\ns_trunk\mozilla\dist\include\content" /D "WIN32" /D "DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PANDORA_EXPORTS" /D "XP_PC" /D "EPIMETHEUS" /D "IS_LITTLE_ENDIAN" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/NPandora.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Pandora - Win32 Release"
# Name "Pandora - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\bytecodecontainer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\exception.cpp
# End Source File
# Begin Source File

SOURCE=..\..\formatter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\hash.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2array.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2boolean.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2date.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2engine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2eval.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2function.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2math.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2metadata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2number.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2regexp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\js2string.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lexer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\..\ns_trunk\mozilla\modules\plugin\tools\sdk\samples\common\np_entry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\..\ns_trunk\mozilla\modules\plugin\tools\sdk\samples\common\npn_gate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\..\ns_trunk\mozilla\modules\plugin\tools\sdk\samples\common\npp_gate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\npscriptable.def
# End Source File
# Begin Source File

SOURCE=..\..\nsScriptablePeer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\numerics.cpp
# End Source File
# Begin Source File

SOURCE=..\..\parser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\plugin.cpp

!IF  "$(CFG)" == "Pandora - Win32 Release"

!ELSEIF  "$(CFG)" == "Pandora - Win32 Debug"

# ADD CPP /I "\ns_trunk\mozilla\dist\include\content"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\prmjtime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\reader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\regexpwrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\strings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\token.cpp
# End Source File
# Begin Source File

SOURCE=..\..\utilities.cpp
# End Source File
# Begin Source File

SOURCE=..\..\world.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\npandora.h
# End Source File
# Begin Source File

SOURCE=..\..\nsScriptablePeer.h
# End Source File
# Begin Source File

SOURCE=..\..\plugin.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\npscriptable.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\dist\lib\xpcom.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\dist\lib\js3250.lib
# End Source File
# End Target
# End Project