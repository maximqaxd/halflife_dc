# Microsoft Developer Studio Project File - Name="zlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604

CFG=zlib - Win32 (WCE SH4) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "zlib.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "zlib.mak" CFG="zlib - Win32 (WCE SH4) Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "zlib - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE "zlib - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "zlib - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/WCESH4Rel"
# PROP Intermediate_Dir "../obj/WCESH4Rel/zlib"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /O2 /Ob2 /Oa /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "_CRTIMP=" /D "NO_ERRNO_H" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /Oa /O2 /Ob2 /I "../src/util/zlib" /I "../src/util" /I "../src/engine" /I "../src/common" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "_CRTIMP=" /D "NO_ERRNO_H" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zlib - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH4Dbg"
# PROP BASE Intermediate_Dir "WCESH4Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/WCESH4Dbg"
# PROP Intermediate_Dir "../obj/WCESH4Dbg/zlib"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "_CRTIMP=" /D "NO_ERRNO_H" /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /Oa /O2 /Ob2 /I "../src/util/zlib" /I "../src/util" /I "../src/engine" /I "../src/common" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "_CRTIMP=" /D "NO_ERRNO_H" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF

# Begin Target

# Name "zlib - Win32 (WCE SH4) Release"
# Name "zlib - Win32 (WCE SH4) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\util\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\src\util\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
