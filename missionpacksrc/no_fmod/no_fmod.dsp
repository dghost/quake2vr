# Microsoft Developer Studio Project File - Name="no_fmod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=no_fmod - Win32 Debug Alpha
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "no_fmod.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "no_fmod.mak" CFG="no_fmod - Win32 Debug Alpha"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "no_fmod - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "no_fmod - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "no_fmod - Win32 Debug Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "no_fmod - Win32 Release Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "no_fmod - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386 /out:".\gamex86.dll"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /out:".\gamex86.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "no_fmod___"
# PROP BASE Intermediate_Dir "no_fmod___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\DebugAXP"
# PROP Intermediate_Dir ".\DebugAXP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /MTd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /MTd /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:".\debug\gamex86.dll" /pdbtype:sept
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:".\debugAXP\gameaxp.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "no_fmod__0"
# PROP BASE Intermediate_Dir "no_fmod__0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ReleaseAXP"
# PROP Intermediate_Dir ".\ReleaseAXP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA /out:".\release\gamex86.dll"
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA /out:".\ReleaseAXP\gameaxp.dll"

!ENDIF 

# Begin Target

# Name "no_fmod - Win32 Release"
# Name "no_fmod - Win32 Debug"
# Name "no_fmod - Win32 Debug Alpha"
# Name "no_fmod - Win32 Release Alpha"
# Begin Group "Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\dm_ball.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\dm_tag.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_ai.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_camera.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_chase.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_cmds.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_combat.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_crane.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_fog.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_func.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_items.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_jetpack.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_lights.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_lock.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_main.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_mappack.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_misc.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_monster.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newai.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newdm.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newfnc.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newtarg.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newtrig.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newutils.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_newweap.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_patchplayermodels.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_pendulum.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_phys.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_save.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_spawn.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_sphere.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_svcmds.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_target.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_thing.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_tracktrain.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_trigger.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_turret.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_utils.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_vehicle.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\g_weapon.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\grenade.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\km_cvar.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_actor.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_actor_weap.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_berserk.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_boss2.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_boss3.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_boss31.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_boss32.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_boss5.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_brain.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_brainbeta.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_carrier.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_chick.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_dog.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_fixbot.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_flash.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_flipper.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_float.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_flyer.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_gekk.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_gladb.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_gladiator.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_gunner.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_hover.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_infantry.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_insane.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_medic.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_move.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_mutant.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_parasite.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_soldier.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_stalker.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_supertank.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_tank.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_turret.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_widow.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\m_widow2.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_chasecam.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_client.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_hud.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_menu.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_text.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_trail.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_view.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\p_weapon.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\q_shared.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\z_anim.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\z_sentien.c

!IF  "$(CFG)" == "no_fmod - Win32 Release"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "no_fmod - Win32 Release Alpha"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\g_main.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\km_cvar.h
# End Source File
# Begin Source File

SOURCE=..\laz_misc.h
# End Source File
# Begin Source File

SOURCE=.\m_actor.h
# End Source File
# Begin Source File

SOURCE=.\m_berserk.h
# End Source File
# Begin Source File

SOURCE=.\m_boss2.h
# End Source File
# Begin Source File

SOURCE=.\m_boss31.h
# End Source File
# Begin Source File

SOURCE=.\m_boss32.h
# End Source File
# Begin Source File

SOURCE=.\m_brain.h
# End Source File
# Begin Source File

SOURCE=.\m_chick.h
# End Source File
# Begin Source File

SOURCE=.\m_dog.h
# End Source File
# Begin Source File

SOURCE=.\m_fiend.h
# End Source File
# Begin Source File

SOURCE=.\m_fixbot.h
# End Source File
# Begin Source File

SOURCE=.\m_flipper.h
# End Source File
# Begin Source File

SOURCE=.\m_float.h
# End Source File
# Begin Source File

SOURCE=.\m_flyer.h
# End Source File
# Begin Source File

SOURCE=.\m_gekk.h
# End Source File
# Begin Source File

SOURCE=.\m_gladiator.h
# End Source File
# Begin Source File

SOURCE=.\m_gunner.h
# End Source File
# Begin Source File

SOURCE=.\m_hover.h
# End Source File
# Begin Source File

SOURCE=.\m_infantry.h
# End Source File
# Begin Source File

SOURCE=.\m_insane.h
# End Source File
# Begin Source File

SOURCE=.\m_medic.h
# End Source File
# Begin Source File

SOURCE=.\m_mutant.h
# End Source File
# Begin Source File

SOURCE=.\m_parasite.h
# End Source File
# Begin Source File

SOURCE=..\m_player.h
# End Source File
# Begin Source File

SOURCE=.\m_rider.h
# End Source File
# Begin Source File

SOURCE=.\m_soldier.h
# End Source File
# Begin Source File

SOURCE=.\m_soldierh.h
# End Source File
# Begin Source File

SOURCE=.\m_supertank.h
# End Source File
# Begin Source File

SOURCE=.\m_tank.h
# End Source File
# Begin Source File

SOURCE=.\m_turret.h
# End Source File
# Begin Source File

SOURCE=.\m_zombie.h
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\p_menu.h
# End Source File
# Begin Source File

SOURCE=.\p_text.h
# End Source File
# Begin Source File

SOURCE=..\pak.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=..\z_anim.h
# End Source File
# Begin Source File

SOURCE=..\z_sentien.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.def,*.res"
# Begin Source File

SOURCE=.\no_fmod.def
# End Source File
# End Group
# Begin Source File

SOURCE=.\gamex86.rc
# End Source File
# End Target
# End Project
