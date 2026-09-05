# Microsoft Developer Studio Project File - Name="halflife_dc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (WCE SH4) Application" 0x8601

CFG=halflife_dc - Win32 (WCE SH4) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "halflife_dc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "halflife_dc.mak" CFG="halflife_dc - Win32 (WCE SH4) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "halflife_dc - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Application")
!MESSAGE "halflife_dc - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Application")
!MESSAGE "halflife_dc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "halflife_dc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/WCESH4Rel"
# PROP Intermediate_Dir "../obj/WCESH4Rel/dc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /O2 /Ob2 /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /I "../src/audio" /I "../src/halflife" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "GLQUAKE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
RSC=rc.exe
# ADD BASE RSC /l 0x419 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x419 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 coredll.lib /nologo /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# ADD LINK32 coredll.lib winsock.lib d3dim.lib ddraw.lib DSOUND.lib wdm.lib mapledev.lib platutil.lib dxguid.lib dinput.lib FloatMath.lib shintr.lib zlib.lib client.lib halflife.lib /nologo /map /debug /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /nodefaultlib:"libc.lib" /libpath:"../obj/WCESH4Rel/" /force:multiple /subsystem:$(CESubsystem) /STACK:65536,4096
# Begin Custom Build
OutDir=.\../obj/WCESH4Rel
InputPath=\Dev\Dreamcast\halflife_dc\obj\WCESH4Rel\halflife_dc.exe
SOURCE="$(InputPath)"

"BuildGDI step" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	../postbuild_dc.bat $(OutDir)/halflife_dc.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH4Dbg"
# PROP BASE Intermediate_Dir "WCESH4Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/WCESH4Dbg"
# PROP Intermediate_Dir "../obj/WCESH4Dbg/dc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /Od /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /I "../src/audio" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /D "GLQUAKE" /D _CRTIMP= /YX /Qsh4r7 /Qs /Qfast /c
RSC=rc.exe
# ADD BASE RSC /l 0x419 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x419 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 coredll.lib /nologo /debug /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# ADD LINK32 coredll.lib winsock.lib d3dim.lib ddraw.lib DSOUND.lib wdm.lib mapledev.lib platutil.lib dxguid.lib dinput.lib FloatMath.lib shintr.lib zlib.lib client.lib halflife.lib /nologo /debug /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /nodefaultlib:"libc.lib" /libpath:"../obj/WCESH4Dbg/" /force:multiple /subsystem:$(CESubsystem) /STACK:65536,4096
# Begin Custom Build
OutDir=.\../obj/WCESH4Dbg
InputPath=\Dev\Dreamcast\halflife_dc\obj\WCESH4Dbg\halflife_dc.exe
SOURCE="$(InputPath)"

"BuildGDI step" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	../postbuild_dc.bat $(OutDir)/halflife_dc.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../obj/Win32Rel"
# PROP BASE Intermediate_Dir "../obj/Win32Rel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../obj/Win32Rel"
# PROP Intermediate_Dir "../obj/Win32Rel"
# PROP Ignore_Export_Lib 0
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "GLQUAKE" /YX /FD /c
# ADD CPP /nologo /W3 /Gi /GX /O1 /Op /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /I "../dx6sdk/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "GLQUAKE" /D "_DEBUG" /YX /FD /c
MTL=midl.exe
RSC=rc.exe
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib d3dim.lib dsound.lib dinput.lib winmm.lib ws2_32.lib dxguid.lib halflife.lib client.lib /nologo /subsystem:windows /machine:I386 /libpath:"../dx6sdk/lib" /libpath:"../obj/Win32Rel"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib d3dim.lib dsound.lib dinput.lib winmm.lib ws2_32.lib dxguid.lib halflife.lib client.lib /nologo /subsystem:windows /incremental:yes /map /debug /machine:I386 /libpath:"../dx6sdk/lib" /libpath:"../obj/Win32Rel"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "../obj/Win32Dbg"
# PROP BASE Intermediate_Dir "../obj/Win32Dbg"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../obj/Win32Dbg"
# PROP Intermediate_Dir "../obj/Win32Dbg"
# PROP Ignore_Export_Lib 0
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "GLQUAKE" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../src/engine" /I "../src/common" /I "../src/render" /I "../src/util" /I "../src/util/zlib" /I "../src/network" /I "../dx6sdk/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "GLQUAKE" /FR /YX /FD /GZ /c
MTL=midl.exe
RSC=rc.exe
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib d3dim.lib dsound.lib dinput.lib winmm.lib ws2_32.lib dxguid.lib halflife.lib client.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../dx6sdk/lib" /libpath:"../obj/Win32Dbg"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib d3dim.lib dsound.lib dinput.lib winmm.lib ws2_32.lib dxguid.lib halflife.lib client.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept /libpath:"../dx6sdk/lib" /libpath:"../obj/Win32Dbg"

!ENDIF 

# Begin Target

# Name "halflife_dc - Win32 (WCE SH4) Release"
# Name "halflife_dc - Win32 (WCE SH4) Debug"
# Name "halflife_dc - Win32 Release"
# Name "halflife_dc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\audio\afile.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AFILE=\
	"..\src\audio\afile.h"\
	"..\src\audio\audio.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AFILE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AFILE=\
	"..\src\audio\afile.h"\
	"..\src\audio\audio.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AFILE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\audio.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AUDIO=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\halflife\vector.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AUDIO=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO=\
	"..\src\audio\vector.h"\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\audio_cd.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AUDIO_=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_cd.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AUDIO_=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_cd.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\audio_mgr.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AUDIO_M=\
	"..\src\audio\afile.h"\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_cd.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"netinfo.h"\
	{$(INCLUDE)}"platutil.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_M=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AUDIO_M=\
	"..\src\audio\afile.h"\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_cd.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"netinfo.h"\
	{$(INCLUDE)}"platutil.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_M=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\audio_static.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AUDIO_S=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_S=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AUDIO_S=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_S=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\audio_stream.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_AUDIO_ST=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_ST=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_AUDIO_ST=\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_AUDIO_ST=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\buildnum.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_BUILD=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_BUILD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_BUILD=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_BUILD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\chase.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CHASE=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CHASE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CHASE=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CHASE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cl_cam.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_CA=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_CA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_CA=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_CA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\CL_DEMO.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_DE=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\shake.h"\
	"..\src\engine\sound.h"\
	"..\src\engine\spritegn.h"\
	"..\src\engine\studio.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_DE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\cl_draw.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_DR=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_DR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_DR=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_DR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cl_ents.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_EN=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
	"..\src\engine\CL_TENT.H"\
	"..\src\engine\client.h"\
	"..\src\engine\cmd.h"\
	"..\src\engine\color.h"\
	"..\src\engine\common.h"\
	"..\src\engine\console.h"\
	"..\src\engine\const.h"\
	"..\src\engine\crc.h"\
	"..\src\engine\cshift.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_EN=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_EN=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
	"..\src\engine\CL_TENT.H"\
	"..\src\engine\client.h"\
	"..\src\engine\cmd.h"\
	"..\src\engine\color.h"\
	"..\src\engine\common.h"\
	"..\src\engine\console.h"\
	"..\src\engine\const.h"\
	"..\src\engine\crc.h"\
	"..\src\engine\cshift.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_EN=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cl_input.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_IN=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_IN=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_IN=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_IN=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\CL_MAIN.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_MA=\
	"..\src\common\clientid.h"\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
	"..\src\engine\cl_servercache.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\winquake.h"\
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_MA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_MA=\
	"..\src\common\clientid.h"\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
	"..\src\engine\cl_servercache.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_MA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\CL_PARSE.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_PA=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_PA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_PA=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_PA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cl_pred.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_PR=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_PR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_PR=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_PR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\CL_TENT.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CL_TE=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\common\r_efx.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CL_TE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CL_TE=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\common\r_efx.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CL_TE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cmd.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CMD_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CMD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CMD_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CMD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\cmodel.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CMODE=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CMODE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CMODE=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CMODE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\eng_common.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_COMMO=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_COMMO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_COMMO=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_COMMO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\CONSOLE.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CONSO=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CONSO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CONSO=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CONSO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\crc.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CRC_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	"..\src\util\zlib\zconf.h"\
	"..\src\util\zlib\zlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CRC_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CRC_C=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CRC_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\cvar.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_CVAR_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_CVAR_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_CVAR_=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_CVAR_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\d3dmath.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_D3DMA=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_D3DMA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_D3DMA=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_D3DMA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_accum.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_AC=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_AC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_AC=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_AC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_d3d.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_D3=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_debug.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_D3=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_D3=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_debug.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_D3=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_debug.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_DE=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_debug.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_DE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_DE=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_DE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_draw.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_DR=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\qgl.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_DR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_DR=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_DR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# ADD CPP /TP

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_model.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_MO=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\qgl.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_water.h"\
	"..\src\render\textures.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_MO=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_water.h"\
	"..\src\render\textures.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_refrag.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_RE=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_RE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_RE=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_RE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_rlight.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_RL=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\qgl.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_RL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_RL=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_RL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_rmain.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_RM=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\anorm_dots.h"\
	"..\src\engine\anorms.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\shake.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_RM=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_RM=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\anorm_dots.h"\
	"..\src\engine\anorms.h"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\shake.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_RM=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_rmisc.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_RMI=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_RMI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_RMI=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_RMI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_rsurf.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_RS=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_water.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_RS=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_RS=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_water.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_RS=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_screen.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_SC=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_SC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_SC=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_SC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_vidnt.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_VI=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_VI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_VI=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_VI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\dc_warp.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DC_WA=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_warp_sin.h"\
	"..\src\render\gl_water.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DC_WA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DC_WA=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\gl_warp_sin.h"\
	"..\src\render\gl_water.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_DC_WA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\decals.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DECAL=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DECAL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DECAL=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_DECAL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\dreamcast_crt.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_DREAM=\
	"..\src\util\dreamcast_crt.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_DREAM=\
	"..\src\util\dreamcast_crt.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\eng_cdll_exp.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ENG_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENG_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ENG_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENG_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\eng_cdll_int.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ENG_CD=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\hud_handlers.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENG_CD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ENG_CD=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\hud_handlers.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cl_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENG_CD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\entityclass.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ENTIT=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENTIT=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ENTIT=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ENTIT=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\glHud.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_GLHUD=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_GLHUD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_GLHUD=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_GLHUD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\hashpak.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_HASHP=\
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
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_HASHP=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_HASHP=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_HASHP=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\host.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_HOST_=\
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
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\profile.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_HOST_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_HOST_=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\profile.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_HOST_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\host_cmd.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_HOST_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\ui.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\text_draw.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_HOST_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_HOST_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_HOST_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\HUD.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_HUD_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_HUD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_HUD_C=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_HUD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\in_dc.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_IN_DC=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_DC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_IN_DC=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_IN_DC=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	".\aplusag.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# ADD CPP /TP

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\in_joy.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_IN_JO=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_JO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_IN_JO=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_JO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# ADD CPP /TP

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\in_kbd.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_IN_KB=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_KB=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_IN_KB=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_KB=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# ADD CPP /TP

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\in_mouse.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_IN_MO=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_IN_MO=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\in_dc.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\vmu.h"\
	{$(INCLUDE)}"maplusag.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_IN_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	
# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# ADD CPP /TP

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# ADD CPP /TP

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\info.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_INFO_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_INFO_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_INFO_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_INFO_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\KEYS.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_KEYS_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_KEYS_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_KEYS_=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_KEYS_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\kzap.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_KZAP_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_KZAP_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_KZAP_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_KZAP_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\l_studio.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_L_STU=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_L_STU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_L_STU=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_L_STU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\langtags.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\mathlib.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_MATHL=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"shsgintr.h"\
	
NODEP_CPP_MATHL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_MATHL=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_MATHL=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\mnemo.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_MNEMO=\
	"..\src\audio\afile.h"\
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
	"..\src\engine\dc_precache_data.inc"\
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_MNEMO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_MNEMO=\
	"..\src\audio\afile.h"\
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
	"..\src\engine\dc_precache_data.inc"\
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_MNEMO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\network\net_chan.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_NET_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_NET_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_NET_C=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_NET_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\network\net_ws.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_NET_W=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_NET_W=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_NET_W=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_NET_W=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\physics.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_PHYSI=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_PHYSI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_PHYSI=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_PHYSI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\pmove.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_PMOVE=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_PMOVE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_PMOVE=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_PMOVE=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\pmovetst.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_PMOVET=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_PMOVET=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_PMOVET=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_PMOVET=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\pr_cmds.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_PR_CM=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_PR_CM=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_PR_CM=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_PR_CM=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\pr_edict.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_PR_ED=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_PR_ED=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_PR_ED=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_PR_ED=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\qgl.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_QGL_C=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\qgl.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_QGL_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_QGL_C=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\qgl.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_QGL_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\R_PART.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_R_PAR=\
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
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_PAR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_R_PAR=\
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
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_R_PAR=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\r_studio.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_R_STU=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
	"..\src\engine\client.h"\
	"..\src\engine\cmd.h"\
	"..\src\engine\color.h"\
	"..\src\engine\common.h"\
	"..\src\engine\console.h"\
	"..\src\engine\const.h"\
	"..\src\engine\crc.h"\
	"..\src\engine\cshift.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_STU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_R_STU=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
	"..\src\engine\client.h"\
	"..\src\engine\cmd.h"\
	"..\src\engine\color.h"\
	"..\src\engine\common.h"\
	"..\src\engine\console.h"\
	"..\src\engine\const.h"\
	"..\src\engine\crc.h"\
	"..\src\engine\cshift.h"\
	"..\src\engine\custom.h"\
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_draw.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_R_STU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\r_studio_neo.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_R_STUD=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_STUD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_R_STUD=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\CL_TENT.H"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_STUD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\r_trans.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_R_TRA=\
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
	"..\src\engine\d_local.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_TRA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_R_TRA=\
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
	"..\src\engine\d_local.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\render\r_trans.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_R_TRA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\r_triangle.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_R_TRI=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_R_TRI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_R_TRI=\
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
	"..\src\engine\d_local.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_triangle.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_R_TRI=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\audio\snd_null.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SND_N=\
	"..\src\audio\afile.h"\
	"..\src\audio\audio.h"\
	"..\src\audio\audio_cd.h"\
	"..\src\audio\audio_mgr.h"\
	"..\src\audio\audio_static.h"\
	"..\src\audio\audio_stream.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SND_N=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SND_N=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SND_N=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\SV_MAIN.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SV_MA=\
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
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SV_MA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SV_MA=\
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
	"..\src\engine\customentity.h"\
	"..\src\engine\cvar.h"\
	"..\src\engine\cvardef.h"\
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\network\net.h"\
	"..\src\render\cmodel.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SV_MA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\sv_move.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SV_MO=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SV_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SV_MO=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SV_MO=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\sv_phys.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SV_PH=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SV_PH=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SV_PH=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SV_PH=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\sv_upld.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SV_UP=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\game_entity_api.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SV_UP=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SV_UP=\
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
	"..\src\engine\decal.h"\
	"..\src\engine\draw.h"\
	"..\src\engine\eiface.h"\
	"..\src\engine\glquake.h"\
	"..\src\engine\hashpak.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SV_UP=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\sv_user.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SV_US=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_SV_US=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SV_US=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SV_US=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\sys_dc.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SYS_D=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_debug.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"ceddcdrm.h"\
	{$(INCLUDE)}"ceddstor.h"\
	{$(INCLUDE)}"segagdrm.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"wdm.h"\
	
NODEP_CPP_SYS_D=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SYS_D=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_debug.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_SYS_D=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	".\dm.h"\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\common.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\sys_win.cpp
# End Source File
# Begin Source File

SOURCE=..\src\engine\sys_engine.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_SYS_E=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"ceddcdrm.h"\
	{$(INCLUDE)}"ceddstor.h"\
	{$(INCLUDE)}"segagdrm.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"wdm.h"\
	
NODEP_CPP_SYS_E=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_SYS_E=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"ceddcdrm.h"\
	{$(INCLUDE)}"ceddstor.h"\
	{$(INCLUDE)}"segagdrm.h"\
	{$(INCLUDE)}"shintr.h"\
	{$(INCLUDE)}"wdm.h"\
	
NODEP_CPP_SYS_E=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\text_draw.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_TEXT_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\text_draw.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_TEXT_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_TEXT_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_accum.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\text_draw.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_TEXT_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\render\textures.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_TEXTU=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\textures.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_TEXTU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_TEXTU=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\textures.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_TEXTU=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\tmessage.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_TMESS=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_TMESS=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_TMESS=\
	"..\src\common\dll_state.h"\
	"..\src\common\platform.h"\
	"..\src\common\qfont.h"\
	"..\src\engine\beamdef.h"\
	"..\src\engine\bothdefs.h"\
	"..\src\engine\bspfile.h"\
	"..\src\engine\cdll_int.h"\
	"..\src\engine\cl_demo.h"\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\tmessage.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_TMESS=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\menu.cpp
# End Source File
# Begin Source File

SOURCE=..\src\engine\ui.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_UI_C9a=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\ui.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_UI_C9a=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_UI_C9a=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\ui.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_UI_C9a=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\VIEW.C

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_VIEW_=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\quakedef.h"\
	"..\src\engine\render.h"\
	"..\src\engine\save.h"\
	"..\src\engine\sbar.h"\
	"..\src\engine\screen.h"\
	"..\src\engine\server.h"\
	"..\src\engine\shake.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"floatmathlib.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_VIEW_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_VIEW_=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pmove.h"\
	"..\src\engine\pr_cmds.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\progdefs.h"\
	"..\src\engine\progs.h"\
	"..\src\engine\quakedef.h"\
	"..\src\engine\render.h"\
	"..\src\engine\save.h"\
	"..\src\engine\sbar.h"\
	"..\src\engine\screen.h"\
	"..\src\engine\server.h"\
	"..\src\engine\shake.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_VIEW_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\vmu.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_VMU_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\esfile.h"\
	"..\src\util\kzap.h"\
	"..\src\util\vmu.h"\
	"..\src\util\vmu_icons.inc"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_VMU_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_VMU_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\esfile.h"\
	"..\src\util\kzap.h"\
	"..\src\util\vmu.h"\
	"..\src\util\vmu_icons.inc"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_VMU_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\wad.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_WAD_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_WAD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_WAD_C=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_WAD_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\won.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_WON_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_WON_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_WON_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\won.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_WON_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\world.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_WORLD=\
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
	"..\src\engine\hldc_fixes.h"\
	"..\src\engine\host_cmd.h"\
	"..\src\engine\info.h"\
	"..\src\engine\input.h"\
	"..\src\engine\keys.h"\
	"..\src\engine\mathlib.h"\
	"..\src\engine\modelgen.h"\
	"..\src\engine\pr_dlls.h"\
	"..\src\engine\pr_edict.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_WORLD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_WORLD=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\sv_proto.h"\
	"..\src\engine\sys.h"\
	"..\src\engine\vid.h"\
	"..\src\engine\view.h"\
	"..\src\engine\vmodes.h"\
	"..\src\engine\wad.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\render\r_studio.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_WORLD=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\Zap.cpp

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ZAP_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ZAP_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ZAP_C=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ZAP_C=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\util\zapsave.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ZAPSA=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ZAPSA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ZAPSA=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	"..\src\util\kzap.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ZAPSA=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\engine\zone.c

!IF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Release"

DEP_CPP_ZONE_=\
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
	"..\src\engine\hldc_fixes.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	{$(INCLUDE)}"shintr.h"\
	
NODEP_CPP_ZONE_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 (WCE SH4) Debug"

DEP_CPP_ZONE_=\
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
	"..\src\engine\glquake.h"\
	"..\src\engine\host_cmd.h"\
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
	"..\src\engine\winquake.h"\
	"..\src\engine\world.h"\
	"..\src\engine\wrect.h"\
	"..\src\engine\zone.h"\
	"..\src\network\net.h"\
	"..\src\render\dc_model.h"\
	"..\src\util\dreamcast_crt.h"\
	
NODEP_CPP_ZONE_=\
	"..\src\engine\cmdlib.h"\
	"..\src\engine\lbmlib.h"\
	"..\src\engine\r_shared.h"\
	"..\src\engine\scriplib.h"\
	"..\src\engine\trilib.h"\
	

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Release"

!ELSEIF  "$(CFG)" == "halflife_dc - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
