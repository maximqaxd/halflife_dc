# Microsoft Developer Studio Project File - Name="client" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604
# TARGTYPE "Win32 (x86) Static Library" 0x0104

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
!MESSAGE "client - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "client - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"

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
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
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
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "client - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../src/engine" /I "../src/common" /I "../src/util" /I "../src/halflife" /I "../src/client" /I "../dx6sdk/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "CLIENT_DLL" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../src/engine" /I "../src/common" /I "../src/util" /I "../src/halflife" /I "../src/client" /I "../dx6sdk/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "CLIENT_DLL" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x419
# ADD RSC /l 0x419
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/util" /I "../src/halflife" /I "../src/client" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "CLIENT_DLL" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/util" /I "../src/halflife" /I "../src/client" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "CLIENT_DLL" /FR /YX /FD /GZ /c
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

# Name "client - Win32 (WCE SH4) Release"
# Name "client - Win32 (WCE SH4) Debug"
# Name "client - Win32 Release"
# Name "client - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\client\ammo.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_AMMO_=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_AMMO_=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\ammohistory.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_AMMOH=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_AMMOH=\
	"..\src\client\ammo.h"\
	"..\src\client\ammohistory.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\battery.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_BATTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_BATTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\cdll_int.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_CDLL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_CDLL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\death.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_DEATH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_DEATH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\dreamcast_crt.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\flashlight.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_FLASH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_FLASH=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\geiger.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_GEIGE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_GEIGE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	"..\src\util\dreamcast_crt.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\health.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_HEALT=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_HEALT=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\hud.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_HUD_C=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_HUD_C=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\hud_msg.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_HUD_M=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_HUD_M=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\hud_redraw.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_HUD_R=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_HUD_R=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\hud_update.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_HUD_U=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_HUD_U=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\message.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_MESSA=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_MESSA=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\MOTD.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_MOTD_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_MOTD_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\parsemsg.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\saytext.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_SAYTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_SAYTE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\scoreboard.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_SCORE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_SCORE=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\train.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_TRAIN=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_TRAIN=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\parsemsg.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\client\util.cpp

!IF  "$(CFG)" == "client - Win32 (WCE SH4) Release"

DEP_CPP_UTIL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 (WCE SH4) Debug"

DEP_CPP_UTIL_=\
	"..\src\client\ammo.h"\
	"..\src\client\cl_dll.h"\
	"..\src\client\health.h"\
	"..\src\client\hud.h"\
	"..\src\client\util_vector.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\halflife\cdll_dll.h"\
	

!ELSEIF  "$(CFG)" == "client - Win32 Release"

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
