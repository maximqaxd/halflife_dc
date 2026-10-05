// input.h -- external (non-keyboard) input devices

#ifdef __cplusplus
extern "C" {
#endif

void IN_Init( void );

void IN_Shutdown( void );

void IN_Commands( void );
// oportunity for devices to stick commands on the script buffer

void IN_Move( usercmd_t* cmd );
// add additional movement on top of the keyboard move cmd

void IN_Accumulate( void );

int IN_ControllerPresent( void );
// FALSE while the pad is unplugged

#if HLDC_DUALSTICK
qboolean IN_HasSecondStick( void );
// TRUE when both second-stick axes are advertised by the active controller

char *IN_PresetOverlay( int preset );
// optional script for this controller and preset, or NULL

void IN_ExecPreset( int preset, qboolean insert );
// queue the original preset followed by its optional overlay
#endif

DLL_EXPORT void IN_ClearStates( void );
// restores all button and position states to defaults

#ifdef __cplusplus
}
#endif
