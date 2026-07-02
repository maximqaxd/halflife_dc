# Microsoft Developer Studio Project File - Name="halflife" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=halflife - Win32 (WCE SH4) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "halflife.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "halflife.mak" CFG="halflife - Win32 (WCE SH4) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "halflife - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE "halflife - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE "halflife - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "halflife - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/WCESH4Rel"
# PROP Intermediate_Dir "../obj/WCESH4Rel/halflife"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/engine" /I "../src/common" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "halflife___Win32__WCE_SH4__Debug"
# PROP BASE Intermediate_Dir "halflife___Win32__WCE_SH4__Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/WCESH4Dbg"
# PROP Intermediate_Dir "../obj/WCESH4Dbg/halflife"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/engine" /I "../src/common" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../obj/Win32Rel"
# PROP BASE Intermediate_Dir "../obj/Win32Rel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/Win32Rel"
# PROP Intermediate_Dir "../obj/Win32Rel"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../src/engine" /I "../src/common" /I "../src/halflife" /I "../dx6sdk/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /YX /FD /c
# ADD CPP /nologo /W3 /Gi /GX /O2 /I "../src/engine" /I "../src/common" /I "../src/halflife" /I "../dx6sdk/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x419
# ADD RSC /l 0x419
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "../obj/Win32Dbg"
# PROP BASE Intermediate_Dir "../obj/Win32Dbg"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/Win32Dbg"
# PROP Intermediate_Dir "../obj/Win32Dbg"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/halflife" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/halflife" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /FR /YX /FD /GZ /c
RSC=rc.exe
# ADD BASE RSC /l 0x419
# ADD RSC /l 0x419
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "halflife - Win32 (WCE SH4) Release"
# Name "halflife - Win32 (WCE SH4) Debug"
# Name "halflife - Win32 Release"
# Name "halflife - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\halflife\aflock.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_AFLOC=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_AFLOC=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_AFLOC=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\agrunt.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_AGRUN=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_AGRUN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_AGRUN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\airtank.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_AIRTA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_AIRTA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_AIRTA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\animation.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ANIMA=\
	"..\src\engine\studio.h"\
	"..\src\halflife\activity.h"\
	"..\src\halflife\activitymap.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\enginecallback.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_ANIMA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\mathlib.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progdefs.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ANIMA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\studio.h"\
	"..\src\halflife\activity.h"\
	"..\src\halflife\activitymap.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\enginecallback.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\util\dreamcast_crt.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\apache.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_APACH=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_APACH=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_APACH=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\barnacle.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BARNA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_BARNA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BARNA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\barney.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BARNE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_BARNE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BARNE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\bigmomma.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BIGMO=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_BIGMO=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BIGMO=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\bloater.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BLOAT=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_BLOAT=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BLOAT=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\bmodels.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BMODE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_BMODE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BMODE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\bullsquid.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BULLS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_BULLS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BULLS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\buttons.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_BUTTO=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_BUTTO=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_BUTTO=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\cbase.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_CBASE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_CBASE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_CBASE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\client.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_CLIEN=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\spectator.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_CLIEN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_CLIEN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\spectator.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\combat.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_COMBA=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_COMBA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_COMBA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\controller.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_CONTR=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_CONTR=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_CONTR=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\crossbow.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_CROSS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_CROSS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_CROSS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\crowbar.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_CROWB=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_CROWB=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_CROWB=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\defaultai.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_DEFAU=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_DEFAU=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_DEFAU=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\doors.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_DOORS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_DOORS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_DOORS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\dreamcast_crt.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\effects.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_EFFEC=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_EFFEC=\
	"..\src\halflife\const.h"\
	"..\src\halflife\customentity.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	"..\src\halflife\shake.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_EFFEC=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\egon.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_EGON_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_EGON_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\customentity.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_EGON_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\explode.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_EXPLO=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_EXPLO=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_EXPLO=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\flyingmonster.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_FLYIN=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\flyingmonster.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_FLYIN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_FLYIN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\flyingmonster.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\func_break.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_FUNC_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_FUNC_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_FUNC_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\func_tank.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_FUNC_T=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_FUNC_T=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_FUNC_T=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\gamerules.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GAMER=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GAMER=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GAMER=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\gargantua.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GARGA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GARGA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\customentity.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GARGA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\explode.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\func_break.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\gauss.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GAUSS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GAUSS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	"..\src\halflife\shake.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GAUSS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\genericmonster.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GENER=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_GENER=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GENER=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\ggrenade.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GGREN=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GGREN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GGREN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\globals.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GLOBA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_GLOBA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GLOBA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\glock.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GLOCK=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GLOCK=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GLOCK=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\gman.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_GMAN_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_GMAN_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_GMAN_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_ai.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_H_AI_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_H_AI_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_H_AI_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_battery.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_H_BAT=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_H_BAT=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_H_BAT=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_cine.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_H_CIN=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_H_CIN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_H_CIN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_cycler.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_H_CYC=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_H_CYC=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_H_CYC=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_export.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_H_EXP=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_H_EXP=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_H_EXP=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\handgrenade.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HANDG=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HANDG=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HANDG=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\hassassin.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HASSA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HASSA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HASSA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\headcrab.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HEADC=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_HEADC=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HEADC=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\healthkit.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HEALT=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HEALT=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HEALT=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\hgrunt.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HGRUN=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HGRUN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\customentity.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HGRUN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\hornet.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HORNE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HORNE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HORNE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\hornetgun.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HORNET=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_HORNET=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HORNET=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\hornet.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\houndeye.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_HOUND=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_HOUND=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_HOUND=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\ichthyosaur.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ICHTH=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\flyingmonster.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_ICHTH=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ICHTH=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\flyingmonster.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\islave.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ISLAV=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_ISLAV=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ISLAV=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\items.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ITEMS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_ITEMS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ITEMS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\leech.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_LEECH=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_LEECH=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_LEECH=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\lights.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_LIGHT=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_LIGHT=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_LIGHT=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\link_helper.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_LINK_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_LINK_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_LINK_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\monstermaker.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MONST=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_MONST=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MONST=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\monsters.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MONSTE=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_MONSTE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MONSTE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\monsterstate.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MONSTER=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_MONSTER=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MONSTER=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\mortar.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MORTA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_MORTA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MORTA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\mp5.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MP5_C=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_MP5_C=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MP5_C=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\multiplay_gamerules.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_MULTI=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_MULTI=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_MULTI=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\nihilanth.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_NIHIL=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_NIHIL=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_NIHIL=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\nodes.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_NODES=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_NODES=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_NODES=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\osprey.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_OSPRE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_OSPRE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\customentity.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_OSPRE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\pathcorner.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_PATHC=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_PATHC=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_PATHC=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\plane.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_PLANE=\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_PLANE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_PLANE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\plats.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_PLATS=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_PLATS=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_PLATS=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\player.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_PLAYE=\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_PLAYE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_PLAYE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\python.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_PYTHO=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_PYTHO=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_PYTHO=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\rat.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_RAT_C=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_RAT_C=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_RAT_C=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\roach.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ROACH=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_ROACH=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ROACH=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\rpg.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_RPG_C=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_RPG_C=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_RPG_C=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\satchel.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SATCH=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_SATCH=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SATCH=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\scientist.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SCIEN=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SCIEN=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SCIEN=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\scripted.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SCRIP=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SCRIP=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SCRIP=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\shotgun.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SHOTG=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_SHOTG=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SHOTG=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\singleplay_gamerules.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SINGL=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_SINGL=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SINGL=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\skill.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SKILL=\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SKILL=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SKILL=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\sound.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SOUND=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_SOUND=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SOUND=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\soundent.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SOUNDE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SOUNDE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SOUNDE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\spectator.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SPECT=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\spectator.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SPECT=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SPECT=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\spectator.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\squadmonster.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SQUAD=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SQUAD=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SQUAD=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\plane.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\squeakgrenade.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SQUEA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_SQUEA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SQUEA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\subs.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_SUBS_=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_SUBS_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_SUBS_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\doors.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\talkmonster.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TALKM=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_TALKM=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TALKM=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\defaultai.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\tempmonster.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TEMPM=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_TEMPM=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TEMPM=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\tentacle.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TENTA=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_TENTA=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TENTA=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\triggers.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TRIGG=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_TRIGG=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TRIGG=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\trains.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\tripmine.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TRIPM=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_TRIPM=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TRIPM=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\turret.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_TURRE=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_TURRE=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_TURRE=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\util.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_UTIL_=\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_UTIL_=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_UTIL_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\shake.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\weapons.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_WEAPO=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_WEAPO=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_WEAPO=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\world.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_WORLD=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	
NODEP_CPP_WORLD=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_WORLD=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\client.h"\
	"..\src\halflife\decals.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\xen.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_XEN_C=\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_XEN_C=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_XEN_C=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\animation.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\effects.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\zombie.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

DEP_CPP_ZOMBI=\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	
NODEP_CPP_ZOMBI=\
	"..\src\halflife\const.h"\
	"..\src\halflife\eiface.h"\
	"..\src\halflife\platform.h"\
	"..\src\halflife\progs.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

DEP_CPP_ZOMBI=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\basemonster.h"\
	"..\src\halflife\cbase.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	

!ELSEIF  "$(CFG)" == "halflife - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
