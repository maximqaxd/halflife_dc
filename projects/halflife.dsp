# Microsoft Developer Studio Project File - Name="halflife" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604

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
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"
CPP=shcl.exe

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
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/halflife" /I "../src/common" /I "../src/engine" /I "../src/render" /I "../src/util" /I "../src/network" /I "../src/client" /I "../src/audio" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
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
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/halflife" /I "../src/common" /I "../src/engine" /I "../src/render" /I "../src/util" /I "../src/network" /I "../src/client" /I "../src/audio" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /c
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
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\halflife\aflock.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\agrunt.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\airtank.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\animating.cpp
DEP_CPP_ANIMA=\
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
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\animation.cpp
DEP_CPP_ANIMAT=\
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
	"..\src\halflife\eng_builtins.h"\
	"..\src\halflife\enginecallback.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\apache.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\barnacle.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\barney.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\bigmomma.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\bloater.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\bmodels.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\bullsquid.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\buttons.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\cbase.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saveexports.inc"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\client.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\combat.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\controller.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\crossbow.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\crowbar.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\defaultai.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\doors.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\util\dreamcast_crt.c
DEP_CPP_DREAM=\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	
# ADD CPP /TP
# End Source File
# Begin Source File

SOURCE=..\src\halflife\effects.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\egon.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\explode.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\flyingmonster.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\func_break.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\func_tank.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\game.cpp
DEP_CPP_GAME_=\
	"..\src\common\platform.h"\
	"..\src\engine\const.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\halflife\extdll.h"\
	"..\src\halflife\game.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\gamerules.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\teamplay_gamerules.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\gargantua.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\gauss.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\genericmonster.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\ggrenade.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\globals.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\glock.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\gman.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_ai.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_battery.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_cine.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_cycler.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\h_export.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\handgrenade.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\hassassin.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\headcrab.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\healthkit.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\hgrunt.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\hornet.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\hornetgun.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\houndeye.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\ichthyosaur.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\islave.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\squadmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\items.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\leech.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\lights.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\maprules.cpp
DEP_CPP_MAPRU=\
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
	"..\src\halflife\maprules.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\monstermaker.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\monsters.cpp
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
	"..\src\halflife\extdll.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\monsterstate.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\mortar.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\mp5.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\multiplay_gamerules.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\items.h"\
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\nihilanth.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\nodes.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\osprey.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\pathcorner.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\plane.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\plats.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\player.cpp
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
	"..\src\halflife\game.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\playermonster.cpp
DEP_CPP_PLAYER=\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\python.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\rat.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\roach.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\rosenberg.cpp
DEP_CPP_ROSEN=\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\rpg.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\satchel.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\saveexports.cpp

!IF  "$(CFG)" == "halflife - Win32 (WCE SH4) Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "halflife - Win32 (WCE SH4) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\halflife\schedule.cpp
DEP_CPP_SCHED=\
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
	"..\src\halflife\nodes.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\scripted.h"\
	"..\src\halflife\scriptevent.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\scientist.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\scripted.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\shotgun.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\singleplay_gamerules.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\skill.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\sound.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\talkmonster.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\soundent.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\spectator.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\squadmonster.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\squeakgrenade.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\subs.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\talkmonster.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\teamplay_gamerules.cpp
DEP_CPP_TEAMP=\
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
	"..\src\halflife\game.h"\
	"..\src\halflife\gamerules.h"\
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\teamplay_gamerules.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\tempmonster.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\tentacle.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\triggers.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\tripmine.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\turret.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\monsters.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\util.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\warp.cpp
DEP_CPP_WARP_=\
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\weapons.cpp
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
	"..\src\halflife\model_indices.h"\
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\world.cpp
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
	"..\src\halflife\model_indices.h"\
	"..\src\halflife\monsterevent.h"\
	"..\src\halflife\nodes.h"\
	"..\src\halflife\player.h"\
	"..\src\halflife\saverestore.h"\
	"..\src\halflife\schedule.h"\
	"..\src\halflife\skill.h"\
	"..\src\halflife\soundent.h"\
	"..\src\halflife\teamplay_gamerules.h"\
	"..\src\halflife\vector.h"\
	"..\src\halflife\weapons.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\xen.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\halflife\zombie.cpp
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
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
