# Microsoft Developer Studio Project File - Name="client" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604

CFG=client - Win32 (WCE SH4) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "client.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "client.mak" CFG="client - Win32 (WCE SH4) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "client - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE "client - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"
CPP=shcl.exe

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/WCESH4Rel"
# PROP Intermediate_Dir "../obj/WCESH4Rel/client"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/client" /I "../src/common" /I "../src/engine" /I "../src/render" /I "../src/util" /I "../src/network" /I "../src/halflife" /I "../src/audio" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "GLQUAKE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH4Dbg"
# PROP BASE Intermediate_Dir "WCESH4Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/WCESH4Dbg"
# PROP Intermediate_Dir "../obj/WCESH4Dbg/client"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/client" /I "../src/common" /I "../src/engine" /I "../src/render" /I "../src/util" /I "../src/network" /I "../src/halflife" /I "../src/audio" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "GLQUAKE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "client - Win32 (WCE SH4) Release"
# Name "client - Win32 (WCE SH4) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\client\ammo.cpp
DEP_CPP_AMMO_=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\ammo_secondary.cpp
DEP_CPP_AMMO_S=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\ammohistory.cpp
DEP_CPP_AMMOH=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\battery.cpp
DEP_CPP_BATTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\cdll_int.cpp
DEP_CPP_CDLL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\crouchstate.cpp
DEP_CPP_CROUC=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\death.cpp
DEP_CPP_DEATH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
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

SOURCE=..\src\client\flashlight.cpp
DEP_CPP_FLASH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\geiger.cpp
DEP_CPP_GEIGE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\health.cpp
DEP_CPP_HEALT=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\hud.cpp
DEP_CPP_HUD_C=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\hud_msg.cpp
DEP_CPP_HUD_M=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\hud_redraw.cpp
DEP_CPP_HUD_R=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\hud_update.cpp
DEP_CPP_HUD_U=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\menu.cpp
DEP_CPP_MENU_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\message.cpp
DEP_CPP_MESSA=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\client.h"\
	"..\src\engine\cmd.h"\
	"..\src\engine\color.h"\
	"..\src\engine\common.h"\
	"..\src\engine\console.h"\
	"..\src\engine\const.h"\
	"..\src\engine\crc.h"\
	"..\src\engine\cshift.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\quakedef.h"\
	"..\src\engine\render.h"\
	"..\src\engine\save.h"\
	"..\src\engine\sbar.h"\
	"..\src\engine\screen.h"\
	"..\src\engine\server.h"\
	"..\src\engine\sound.h"\
	"..\src\engine\spritegn.h"\
	"..\src\engine\studio.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
NODEP_CPP_MESSA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\motd.cpp
DEP_CPP_MOTD_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\parsemsg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\client\saytext.cpp
DEP_CPP_SAYTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\scoreboard.cpp
DEP_CPP_SCORE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\status_icons.cpp
DEP_CPP_STATU=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\statusbar.cpp
DEP_CPP_STATUS=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\text_message.cpp
DEP_CPP_TEXT_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\train.cpp
DEP_CPP_TRAIN=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\src\client\util.cpp
DEP_CPP_UTIL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\common\platform.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\wrect.h"\
	"..\src\halflife\cdll_dll.h"\
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
