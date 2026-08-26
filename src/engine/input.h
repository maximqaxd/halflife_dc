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

DLL_EXPORT void IN_ClearStates( void );
// restores all button and position states to defaults

#ifdef __cplusplus
}
#endif