# NMAKE file for building nsinstall.exe.
#
# This file is edited from an NMAKE file generated by
# Microsoft Developer Studio, Format Version 4.20
#
# To build, say
#     nmake /f nsinstall.mak CFG=Release
# or
#     nmake /f nsinstall.mak CFG=Debug
# If CFG is omitted, a release build is assumed.

!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified.  Defaulting Release.
!ENDIF 

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "nsinstall.mak" CFG="Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Release" (based on "Win32 Console Application")
!MESSAGE "Debug" (based on "Win32 Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\nsinstall.exe"

CLEAN : 
	-@erase "$(INTDIR)\nsinstall.obj"
	-@erase "$(OUTDIR)\nsinstall.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/nsinstall.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nsinstall.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo\
 /subsystem:console /incremental:no /pdb:"$(OUTDIR)/nsinstall.pdb"\
 /out:"$(OUTDIR)/nsinstall.exe" 
LINK32_OBJS= \
	"$(INTDIR)\nsinstall.obj"

"$(OUTDIR)\nsinstall.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\nsinstall.exe"

CLEAN : 
	-@erase "$(INTDIR)\nsinstall.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\nsinstall.exe"
	-@erase "$(OUTDIR)\nsinstall.ilk"
	-@erase "$(OUTDIR)\nsinstall.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "DEBUG"\
 /D "_CONSOLE" /Fp"$(INTDIR)/nsinstall.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nsinstall.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo\
 /subsystem:console /incremental:yes /pdb:"$(OUTDIR)/nsinstall.pdb" /debug\
 /out:"$(OUTDIR)/nsinstall.exe" 
LINK32_OBJS= \
	"$(INTDIR)\nsinstall.obj"

"$(OUTDIR)\nsinstall.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

################################################################################
# Begin Source File

SOURCE=.\nsinstall.c
DEP_CPP_SHMSD=\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\nsinstall.obj" : $(SOURCE) $(DEP_CPP_SHMSD) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################