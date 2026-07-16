
#include "extdll.h"
#include "util.h"
#include "cbase.h"


extern "C"
{
void DLLEXPORT GiveFnptrsToDll( enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals );
int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion );
void monster_flyer( entvars_t *pev );
void monster_flyer_flock( entvars_t *pev );
void monster_alien_grunt( entvars_t *pev );
void item_airtank( entvars_t *pev );
void monster_apache( entvars_t *pev );
void hvr_rocket( entvars_t *pev );
void monster_barnacle( entvars_t *pev );
void monster_barney( entvars_t *pev );
void monster_barney_dead( entvars_t *pev );
void info_bigmomma( entvars_t *pev );
void bmortar( entvars_t *pev );
void monster_bigmomma( entvars_t *pev );
void monster_bloater( entvars_t *pev );
void func_wall( entvars_t *pev );
void func_wall_toggle( entvars_t *pev );
void func_conveyor( entvars_t *pev );
void func_illusionary( entvars_t *pev );
void func_monsterclip( entvars_t *pev );
void func_rotating( entvars_t *pev );
void func_pendulum( entvars_t *pev );
void squidspit( entvars_t *pev );
void monster_bullchicken( entvars_t *pev );
void env_global( entvars_t *pev );
void multisource( entvars_t *pev );
void func_button( entvars_t *pev );
void func_rot_button( entvars_t *pev );
void momentary_rot_button( entvars_t *pev );
void env_spark( entvars_t *pev );
void env_debris( entvars_t *pev );
void button_target( entvars_t *pev );
void monster_alien_controller( entvars_t *pev );
void controller_head_ball( entvars_t *pev );
void controller_energy_ball( entvars_t *pev );
void crossbow_bolt( entvars_t *pev );
void weapon_crossbow( entvars_t *pev );
void ammo_crossbow( entvars_t *pev );
void weapon_crowbar( entvars_t *pev );
void func_door( entvars_t *pev );
void func_water( entvars_t *pev );
void func_door_rotating( entvars_t *pev );
void momentary_door( entvars_t *pev );
void info_target( entvars_t *pev );
void env_bubbles( entvars_t *pev );
void beam( entvars_t *pev );
void env_lightning( entvars_t *pev );
void env_beam( entvars_t *pev );
// void trip_beam( entvars_t *pev );
void env_laser( entvars_t *pev );
void env_glow( entvars_t *pev );
void env_sprite( entvars_t *pev );
void gibshooter( entvars_t *pev );
void env_shooter( entvars_t *pev );
void test_effect( entvars_t *pev );
void env_shake( entvars_t *pev );
void env_fade( entvars_t *pev );
void env_message( entvars_t *pev );
void env_funnel( entvars_t *pev );
void env_beverage( entvars_t *pev );
void item_sodacan( entvars_t *pev );
void weapon_egon( entvars_t *pev );
void ammo_egonclip( entvars_t *pev );
void spark_shower( entvars_t *pev );
void env_explosion( entvars_t *pev );
void func_breakable( entvars_t *pev );
void func_pushable( entvars_t *pev );
void func_tank( entvars_t *pev );
void func_tanklaser( entvars_t *pev );
void func_tankrocket( entvars_t *pev );
void func_tankmortar( entvars_t *pev );
void func_tankcontrols( entvars_t *pev );
void garg_stomp( entvars_t *pev );
void monster_gargantua( entvars_t *pev );
void weapon_gauss( entvars_t *pev );
void ammo_gaussclip( entvars_t *pev );
void monster_generic( entvars_t *pev );
void grenade( entvars_t *pev );
void weapon_glock( entvars_t *pev );
void weapon_9mmhandgun( entvars_t *pev );
void ammo_glockclip( entvars_t *pev );
void ammo_9mmclip( entvars_t *pev );
void monster_gman( entvars_t *pev );
void weapon_handgrenade( entvars_t *pev );
void monster_human_assassin( entvars_t *pev );
void monster_headcrab( entvars_t *pev );
void monster_babycrab( entvars_t *pev );
void item_healthkit( entvars_t *pev );
void func_healthcharger( entvars_t *pev );
void monster_human_grunt( entvars_t *pev );
void monster_grunt_repel( entvars_t *pev );
void monster_hgrunt_dead( entvars_t *pev );
void hornet( entvars_t *pev );
void weapon_hornetgun( entvars_t *pev );
void monster_houndeye( entvars_t *pev );
void func_recharge( entvars_t *pev );
void monster_cine_scientist( entvars_t *pev );
void monster_cine_panther( entvars_t *pev );
void monster_cine_barney( entvars_t *pev );
void monster_cine2_scientist( entvars_t *pev );
void monster_cine2_hvyweapons( entvars_t *pev );
void monster_cine2_slave( entvars_t *pev );
void monster_cine3_scientist( entvars_t *pev );
void monster_cine3_barney( entvars_t *pev );
void cine_blood( entvars_t *pev );
void cycler( entvars_t *pev );
void cycler_prdroid( entvars_t *pev );
void cycler_sprite( entvars_t *pev );
void cycler_weapon( entvars_t *pev );
void cycler_wreckage( entvars_t *pev );
void monster_ichthyosaur( entvars_t *pev );
void monster_alien_slave( entvars_t *pev );
void monster_vortigaunt( entvars_t *pev );
void world_items( entvars_t *pev );
void item_suit( entvars_t *pev );
void item_battery( entvars_t *pev );
void item_antidote( entvars_t *pev );
void item_security( entvars_t *pev );
void item_longjump( entvars_t *pev );
void monster_leech( entvars_t *pev );
void light( entvars_t *pev );
void light_spot( entvars_t *pev );
void light_environment( entvars_t *pev );
void monstermaker( entvars_t *pev );
void func_mortar_field( entvars_t *pev );
void monster_mortar( entvars_t *pev );
void weapon_mp5( entvars_t *pev );
void weapon_9mmAR( entvars_t *pev );
void ammo_mp5clip( entvars_t *pev );
void ammo_9mmAR( entvars_t *pev );
void ammo_9mmbox( entvars_t *pev );
void ammo_mp5grenades( entvars_t *pev );
void ammo_ARgrenades( entvars_t *pev );
void monster_nihilanth( entvars_t *pev );
void nihilanth_energy_ball( entvars_t *pev );
void info_node( entvars_t *pev );
void info_node_air( entvars_t *pev );
void testhull( entvars_t *pev );
void node_viewer( entvars_t *pev );
void node_viewer_human( entvars_t *pev );
void node_viewer_fly( entvars_t *pev );
void node_viewer_large( entvars_t *pev );
void monster_osprey( entvars_t *pev );
void path_corner( entvars_t *pev );
void path_track( entvars_t *pev );
void func_plat( entvars_t *pev );
void func_platrot( entvars_t *pev );
void func_train( entvars_t *pev );
void func_tracktrain( entvars_t *pev );
void func_traincontrols( entvars_t *pev );
void func_trackchange( entvars_t *pev );
void func_trackautochange( entvars_t *pev );
void func_guntarget( entvars_t *pev );
void player( entvars_t *pev );
void monster_hevsuit_dead( entvars_t *pev );
void player_weaponstrip( entvars_t *pev );
void player_loadsaved( entvars_t *pev );
// void prop( entvars_t *pev );
void weapon_python( entvars_t *pev );
void weapon_357( entvars_t *pev );
void ammo_357( entvars_t *pev );
void monster_rat( entvars_t *pev );
void monster_cockroach( entvars_t *pev );
void weapon_rpg( entvars_t *pev );
void laser_spot( entvars_t *pev );
void rpg_rocket( entvars_t *pev );
void ammo_rpgclip( entvars_t *pev );
void monster_satchel( entvars_t *pev );
void weapon_satchel( entvars_t *pev );
void monster_scientist( entvars_t *pev );
void monster_scientist_dead( entvars_t *pev );
void monster_sitting_scientist( entvars_t *pev );
void scripted_sequence( entvars_t *pev );
void aiscripted_sequence( entvars_t *pev );
void scripted_sentence( entvars_t *pev );
void monster_furniture( entvars_t *pev );
void weapon_shotgun( entvars_t *pev );
void ammo_buckshot( entvars_t *pev );
void ambient_generic( entvars_t *pev );
void env_sound( entvars_t *pev );
void speaker( entvars_t *pev );
void soundent( entvars_t *pev );
void monster_snark( entvars_t *pev );
void weapon_snark( entvars_t *pev );
void info_null( entvars_t *pev );
void info_player_deathmatch( entvars_t *pev );
void info_player_start( entvars_t *pev );
void info_landmark( entvars_t *pev );
void DelayedUse( entvars_t *pev );
// void my_monster( entvars_t *pev );
void monster_tentacle( entvars_t *pev );
void monster_tentaclemaw( entvars_t *pev );
void func_friction( entvars_t *pev );
void trigger_auto( entvars_t *pev );
void trigger_relay( entvars_t *pev );
void multi_manager( entvars_t *pev );
void env_render( entvars_t *pev );
void trigger( entvars_t *pev );
void trigger_hurt( entvars_t *pev );
void trigger_monsterjump( entvars_t *pev );
void trigger_cdaudio( entvars_t *pev );
void target_cdaudio( entvars_t *pev );
void trigger_multiple( entvars_t *pev );
void trigger_once( entvars_t *pev );
void trigger_counter( entvars_t *pev );
void trigger_transition( entvars_t *pev );
void fireanddie( entvars_t *pev );
void trigger_changelevel( entvars_t *pev );
void func_ladder( entvars_t *pev );
void trigger_push( entvars_t *pev );
void trigger_teleport( entvars_t *pev );
void info_teleport_destination( entvars_t *pev );
void trigger_autosave( entvars_t *pev );
void trigger_endsection( entvars_t *pev );
void trigger_gravity( entvars_t *pev );
void trigger_changetarget( entvars_t *pev );
void trigger_camera( entvars_t *pev );
void monster_tripmine( entvars_t *pev );
void weapon_tripmine( entvars_t *pev );
void monster_turret( entvars_t *pev );
void monster_miniturret( entvars_t *pev );
void monster_sentry( entvars_t *pev );
void weaponbox( entvars_t *pev );
void infodecal( entvars_t *pev );
void bodyque( entvars_t *pev );
void worldspawn( entvars_t *pev );
void xen_plantlight( entvars_t *pev );
void xen_hair( entvars_t *pev );
void xen_ttrigger( entvars_t *pev );
void xen_tree( entvars_t *pev );
void xen_spore_small( entvars_t *pev );
void xen_spore_medium( entvars_t *pev );
void xen_spore_large( entvars_t *pev );
void xen_hull( entvars_t *pev );
void monster_zombie( entvars_t *pev );

struct {const char *name;void *func;} lib_hl_exports[] = {
// entity exports, generated by genexports.py
	{ "monster_flyer", &monster_flyer },
	{ "monster_flyer_flock", &monster_flyer_flock },
	{ "monster_alien_grunt", &monster_alien_grunt },
	{ "item_airtank", &item_airtank },
	{ "monster_apache", &monster_apache },
	{ "hvr_rocket", &hvr_rocket },
	{ "monster_barnacle", &monster_barnacle },
	{ "monster_barney", &monster_barney },
	{ "monster_barney_dead", &monster_barney_dead },
	{ "info_bigmomma", &info_bigmomma },
	{ "bmortar", &bmortar },
	{ "monster_bigmomma", &monster_bigmomma },
	{ "monster_bloater", &monster_bloater },
	{ "func_wall", &func_wall },
	{ "func_wall_toggle", &func_wall_toggle },
	{ "func_conveyor", &func_conveyor },
	{ "func_illusionary", &func_illusionary },
	{ "func_monsterclip", &func_monsterclip },
	{ "func_rotating", &func_rotating },
	{ "func_pendulum", &func_pendulum },
	{ "squidspit", &squidspit },
	{ "monster_bullchicken", &monster_bullchicken },
	{ "env_global", &env_global },
	{ "multisource", &multisource },
	{ "func_button", &func_button },
	{ "func_rot_button", &func_rot_button },
	{ "momentary_rot_button", &momentary_rot_button },
	{ "env_spark", &env_spark },
	{ "env_debris", &env_debris },
	{ "button_target", &button_target },
	{ "monster_alien_controller", &monster_alien_controller },
	{ "controller_head_ball", &controller_head_ball },
	{ "controller_energy_ball", &controller_energy_ball },
	{ "crossbow_bolt", &crossbow_bolt },
	{ "weapon_crossbow", &weapon_crossbow },
	{ "ammo_crossbow", &ammo_crossbow },
	{ "weapon_crowbar", &weapon_crowbar },
	{ "func_door", &func_door },
	{ "func_water", &func_water },
	{ "func_door_rotating", &func_door_rotating },
	{ "momentary_door", &momentary_door },
	{ "info_target", &info_target },
	{ "env_bubbles", &env_bubbles },
	{ "beam", &beam },
	{ "env_lightning", &env_lightning },
	{ "env_beam", &env_beam },
//	{ "trip_beam", &trip_beam },
	{ "env_laser", &env_laser },
	{ "env_glow", &env_glow },
	{ "env_sprite", &env_sprite },
	{ "gibshooter", &gibshooter },
	{ "env_shooter", &env_shooter },
	{ "test_effect", &test_effect },
	{ "env_shake", &env_shake },
	{ "env_fade", &env_fade },
	{ "env_message", &env_message },
	{ "env_funnel", &env_funnel },
	{ "env_beverage", &env_beverage },
	{ "item_sodacan", &item_sodacan },
	{ "weapon_egon", &weapon_egon },
	{ "ammo_egonclip", &ammo_egonclip },
	{ "spark_shower", &spark_shower },
	{ "env_explosion", &env_explosion },
	{ "func_breakable", &func_breakable },
	{ "func_pushable", &func_pushable },
	{ "func_tank", &func_tank },
	{ "func_tanklaser", &func_tanklaser },
	{ "func_tankrocket", &func_tankrocket },
	{ "func_tankmortar", &func_tankmortar },
	{ "func_tankcontrols", &func_tankcontrols },
	{ "garg_stomp", &garg_stomp },
	{ "monster_gargantua", &monster_gargantua },
	{ "weapon_gauss", &weapon_gauss },
	{ "ammo_gaussclip", &ammo_gaussclip },
	{ "monster_generic", &monster_generic },
	{ "grenade", &grenade },
	{ "weapon_glock", &weapon_glock },
	{ "weapon_9mmhandgun", &weapon_9mmhandgun },
	{ "ammo_glockclip", &ammo_glockclip },
	{ "ammo_9mmclip", &ammo_9mmclip },
	{ "monster_gman", &monster_gman },
	{ "weapon_handgrenade", &weapon_handgrenade },
	{ "monster_human_assassin", &monster_human_assassin },
	{ "monster_headcrab", &monster_headcrab },
	{ "monster_babycrab", &monster_babycrab },
	{ "item_healthkit", &item_healthkit },
	{ "func_healthcharger", &func_healthcharger },
	{ "monster_human_grunt", &monster_human_grunt },
	{ "monster_grunt_repel", &monster_grunt_repel },
	{ "monster_hgrunt_dead", &monster_hgrunt_dead },
	{ "hornet", &hornet },
	{ "weapon_hornetgun", &weapon_hornetgun },
	{ "monster_houndeye", &monster_houndeye },
	{ "func_recharge", &func_recharge },
	{ "monster_cine_scientist", &monster_cine_scientist },
	{ "monster_cine_panther", &monster_cine_panther },
	{ "monster_cine_barney", &monster_cine_barney },
	{ "monster_cine2_scientist", &monster_cine2_scientist },
	{ "monster_cine2_hvyweapons", &monster_cine2_hvyweapons },
	{ "monster_cine2_slave", &monster_cine2_slave },
	{ "monster_cine3_scientist", &monster_cine3_scientist },
	{ "monster_cine3_barney", &monster_cine3_barney },
	{ "cine_blood", &cine_blood },
	{ "cycler", &cycler },
	{ "cycler_prdroid", &cycler_prdroid },
	{ "cycler_sprite", &cycler_sprite },
	{ "cycler_weapon", &cycler_weapon },
	{ "cycler_wreckage", &cycler_wreckage },
	{ "monster_ichthyosaur", &monster_ichthyosaur },
	{ "monster_alien_slave", &monster_alien_slave },
	{ "monster_vortigaunt", &monster_vortigaunt },
	{ "world_items", &world_items },
	{ "item_suit", &item_suit },
	{ "item_battery", &item_battery },
	{ "item_longjump", &item_longjump },
	{ "monster_leech", &monster_leech },
	{ "light", &light },
	{ "light_spot", &light_spot },
	{ "light_environment", &light_environment },
	{ "monstermaker", &monstermaker },
	{ "func_mortar_field", &func_mortar_field },
	{ "monster_mortar", &monster_mortar },
	{ "weapon_mp5", &weapon_mp5 },
	{ "weapon_9mmAR", &weapon_9mmAR },
	{ "ammo_mp5clip", &ammo_mp5clip },
	{ "ammo_9mmAR", &ammo_9mmAR },
	{ "ammo_9mmbox", &ammo_9mmbox },
	{ "ammo_mp5grenades", &ammo_mp5grenades },
	{ "ammo_ARgrenades", &ammo_ARgrenades },
	{ "monster_nihilanth", &monster_nihilanth },
	{ "nihilanth_energy_ball", &nihilanth_energy_ball },
	{ "info_node", &info_node },
	{ "info_node_air", &info_node_air },
	{ "testhull", &testhull },
	{ "node_viewer", &node_viewer },
	{ "node_viewer_human", &node_viewer_human },
	{ "node_viewer_fly", &node_viewer_fly },
	{ "node_viewer_large", &node_viewer_large },
	{ "monster_osprey", &monster_osprey },
	{ "path_corner", &path_corner },
	{ "path_track", &path_track },
	{ "func_plat", &func_plat },
	{ "func_platrot", &func_platrot },
	{ "func_train", &func_train },
	{ "func_tracktrain", &func_tracktrain },
	{ "func_traincontrols", &func_traincontrols },
	{ "func_trackchange", &func_trackchange },
	{ "func_trackautochange", &func_trackautochange },
	{ "func_guntarget", &func_guntarget },
	{ "player", &player },
	{ "monster_hevsuit_dead", &monster_hevsuit_dead },
	{ "player_weaponstrip", &player_weaponstrip },
	{ "player_loadsaved", &player_loadsaved },
//	{ "prop", &prop },
	{ "weapon_python", &weapon_python },
	{ "weapon_357", &weapon_357 },
	{ "ammo_357", &ammo_357 },
	{ "monster_rat", &monster_rat },
	{ "monster_cockroach", &monster_cockroach },
	{ "weapon_rpg", &weapon_rpg },
	{ "laser_spot", &laser_spot },
	{ "rpg_rocket", &rpg_rocket },
	{ "ammo_rpgclip", &ammo_rpgclip },
	{ "monster_satchel", &monster_satchel },
	{ "weapon_satchel", &weapon_satchel },
	{ "monster_scientist", &monster_scientist },
	{ "monster_scientist_dead", &monster_scientist_dead },
	{ "monster_sitting_scientist", &monster_sitting_scientist },
	{ "scripted_sequence", &scripted_sequence },
	{ "aiscripted_sequence", &aiscripted_sequence },
	{ "scripted_sentence", &scripted_sentence },
	{ "monster_furniture", &monster_furniture },
	{ "weapon_shotgun", &weapon_shotgun },
	{ "ammo_buckshot", &ammo_buckshot },
	{ "ambient_generic", &ambient_generic },
	{ "env_sound", &env_sound },
	{ "speaker", &speaker },
	{ "soundent", &soundent },
	{ "monster_snark", &monster_snark },
	{ "weapon_snark", &weapon_snark },
	{ "info_null", &info_null },
	{ "info_player_deathmatch", &info_player_deathmatch },
	{ "info_player_start", &info_player_start },
	{ "info_landmark", &info_landmark },
	{ "DelayedUse", &DelayedUse },
//    { "my_monster", &my_monster },
	{ "monster_tentacle", &monster_tentacle },
	{ "monster_tentaclemaw", &monster_tentaclemaw },
	{ "func_friction", &func_friction },
	{ "trigger_auto", &trigger_auto },
	{ "trigger_relay", &trigger_relay },
	{ "multi_manager", &multi_manager },
	{ "env_render", &env_render },
	{ "trigger", &trigger },
	{ "trigger_hurt", &trigger_hurt },
	{ "trigger_monsterjump", &trigger_monsterjump },
	{ "trigger_cdaudio", &trigger_cdaudio },
	{ "target_cdaudio", &target_cdaudio },
	{ "trigger_multiple", &trigger_multiple },
	{ "trigger_once", &trigger_once },
	{ "trigger_counter", &trigger_counter },
	{ "trigger_transition", &trigger_transition },
	{ "fireanddie", &fireanddie },
	{ "trigger_changelevel", &trigger_changelevel },
	{ "func_ladder", &func_ladder },
	{ "trigger_push", &trigger_push },
	{ "trigger_teleport", &trigger_teleport },
	{ "info_teleport_destination", &info_teleport_destination },
	{ "trigger_autosave", &trigger_autosave },
	{ "trigger_endsection", &trigger_endsection },
	{ "trigger_gravity", &trigger_gravity },
	{ "trigger_changetarget", &trigger_changetarget },
	{ "trigger_camera", &trigger_camera },
	{ "monster_tripmine", &monster_tripmine },
	{ "weapon_tripmine", &weapon_tripmine },
	{ "monster_turret", &monster_turret },
	{ "monster_miniturret", &monster_miniturret },
	{ "monster_sentry", &monster_sentry },
	{ "weaponbox", &weaponbox },
	{ "infodecal", &infodecal },
	{ "bodyque", &bodyque },
	{ "worldspawn", &worldspawn },
	{ "xen_plantlight", &xen_plantlight },
	{ "xen_hair", &xen_hair },
	{ "xen_ttrigger", &xen_ttrigger },
	{ "xen_tree", &xen_tree },
	{ "xen_spore_small", &xen_spore_small },
	{ "xen_spore_medium", &xen_spore_medium },
	{ "xen_spore_large", &xen_spore_large },
	{ "xen_hull", &xen_hull },
	{ "monster_zombie", &monster_zombie },
	{ 0, 0} 
};
}

/*
 * Placeholder entity factories. entityclass.c's GetDispatch table names every
 * LINK_ENTITY_TO_CLASS factory in the game; these no-op spawns let the statically
 * linked game DLL resolve the ones whose source files are not reconstructed yet.
 * Remove each stub when its real implementation lands.
 */
extern "C" {

// effects.cpp
void env_warpball( entvars_t *pev ) {}

// items.cpp
void item_helmet( entvars_t *pev ) {}
void item_armorvest( entvars_t *pev ) {}

// player.cpp / scientist.cpp
void monster_player( entvars_t *pev ) {}
void monster_rosenberg( entvars_t *pev ) {}

// triggers.cpp
void trigger_playerfreeze( entvars_t *pev ) {}

}
