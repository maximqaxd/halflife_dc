#ifndef HLDC_FIXES_H
#define HLDC_FIXES_H

//
// Opt-in corrections for defects that shipped in the Dreamcast game.
//
// The engine reproduces the original behaviour by default, so a build with
// HLDC_FIXES left at 0 behaves exactly like the released game. Define it to 1
// in the project settings to get a corrected engine instead.
//
// Every guarded site names the symptom it produces in the game, so a fix can
// be turned off again when tracking down a difference in behaviour.
//
#ifndef HLDC_FIXES
#define HLDC_FIXES 0
#endif

#endif // HLDC_FIXES_H
