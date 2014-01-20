#include "g_local.h"

/******* Knightmare's cvar code file ***********/

// enable/disable options
cvar_t	*mega_gibs;			 // whether to spawn extra gibs, default to 0
cvar_t	*mp_monster_replace; //whether to replace monsters with mission pack modified variants
cvar_t	*mp_monster_ammo_replace; //whether to replace monsters' ammo items if above is enabled
cvar_t	*kamikaze_flyer_replace; //whether to replace some flyers with kamikaze flyers, probability 0-1
cvar_t	*allow_player_use_abandoned_turret; //whether to allow player to use turrets in exisiting maps
cvar_t	*turn_rider;  //whether to turn player on rotating object
cvar_t	*adjust_train_corners; //whether to subtract (1,1,1) from train path corners to fix misalignments

cvar_t	*tpp_auto; //whether to automatically go into third-person when pushing a pushable

cvar_t	*ion_ripper_extra_sounds;
cvar_t	*pack_give_xatrix_ammo;
cvar_t	*pack_give_rogue_ammo;

cvar_t	*add_velocity_throw;	//whether to add player's velocity to thrown objects

cvar_t	*falling_armor_damage; //whether player's armor absorbs damage from falling
cvar_t	*player_jump_sounds; //whether to play that STUPID grunting sound when the player jumps

// Server-side speed control stuff
cvar_t	*player_max_speed;
cvar_t	*player_crouch_speed;
cvar_t	*player_accel;
cvar_t	*player_stopspeed;

cvar_t	*use_vwep;

// weapon balancing
cvar_t	*blaster_damage;
cvar_t	*blaster_speed;
cvar_t	*blaster_color;  //blaster color- 1=yellow, 2=green, 3=blue, 4=red

cvar_t	*shotgun_damage;
cvar_t	*shotgun_count;
cvar_t	*shotgun_hspread;
cvar_t	*shotgun_vspread;

cvar_t	*sshotgun_damage;
cvar_t	*sshotgun_count;
cvar_t	*sshotgun_hspread;
cvar_t	*sshotgun_vspread;

cvar_t	*machinegun_damage;
cvar_t	*machinegun_hspread;
cvar_t	*machinegun_vspread;

cvar_t	*chaingun_damage;
cvar_t	*chaingun_hspread;
cvar_t	*chaingun_vspread;

cvar_t	*grenade_damage;
cvar_t	*grenade_radius;
cvar_t	*grenade_speed;

cvar_t	*hand_grenade_damage;
cvar_t	*hand_grenade_radius;

cvar_t	*rocket_damage;
cvar_t	*rocket_damage2;
cvar_t	*rocket_rdamage;
cvar_t	*rocket_radius;
cvar_t	*rocket_speed;

cvar_t	*hyperblaster_damage;
cvar_t	*hyperblaster_speed;
cvar_t	*hyperblaster_color;  //hyperblaster color- 1=yellow, 2=green, 3=blue, 4=red

cvar_t	*railgun_damage;
cvar_t	*rail_color;

cvar_t	*bfg_damage;
cvar_t	*bfg_damage2;
cvar_t	*bfg_rdamage;
cvar_t	*bfg_radius;
cvar_t	*bfg_speed;

cvar_t	*jump_kick_damage;

cvar_t	*tesla_damage;
cvar_t	*tesla_radius;
cvar_t	*tesla_life;
cvar_t	*tesla_health;

cvar_t	*chainfist_damage;

cvar_t	*plasmabeam_damage;

cvar_t	*etf_rifle_damage;
cvar_t	*etf_rifle_radius_damage;
cvar_t	*etf_rifle_radius;
cvar_t	*etf_rifle_speed;

cvar_t	*disruptor_damage;
cvar_t	*disruptor_speed;

cvar_t	*prox_damage;
cvar_t	*prox_radius;
cvar_t	*prox_speed;
cvar_t	*prox_life;
cvar_t	*prox_health;

cvar_t	*ionripper_damage;
cvar_t	*ionripper_speed;

cvar_t	*phalanx_damage;
cvar_t	*phalanx_damage2;
cvar_t	*phalanx_radius_damage;
cvar_t	*phalanx_radius;
cvar_t	*phalanx_speed;

cvar_t	*trap_life;
cvar_t	*trap_health;

cvar_t	*nuke_delay;
cvar_t	*nuke_life;
cvar_t	*nuke_radius;

cvar_t	*nbomb_delay;
cvar_t	*nbomb_life;
cvar_t	*nbomb_radius;
cvar_t	*nbomb_damage;

cvar_t	*defender_blaster_damage;
cvar_t	*defender_blaster_speed;

cvar_t	*vengeance_health_threshold;

cvar_t	*shockwave_bounces;
cvar_t	*shockwave_damage;
cvar_t	*shockwave_damage2;
cvar_t	*shockwave_rdamage;
cvar_t	*shockwave_radius;
cvar_t	*shockwave_speed;
cvar_t	*shockwave_effect_damage;
cvar_t	*shockwave_effect_radius;

cvar_t	*box_shells; //value of shells
cvar_t	*box_bullets; //value of bullets
cvar_t	*box_grenades; //value of grenade pack
cvar_t	*box_rockets; //value of rocket pack
cvar_t	*box_cells; //value of cell pack
cvar_t	*box_slugs; //value of slug box
cvar_t	*box_magslugs; //value ofmagslug box
cvar_t	*box_flechettes; //value of flechettes
cvar_t	*box_prox; //value of prox
cvar_t	*box_tesla; //value of tesla pack
cvar_t	*box_disruptors; //value of disruptor pack
cvar_t	*box_shocksphere; //value of shocksphere
cvar_t	*box_trap; //value of trap
cvar_t	*box_fuel; //value of fuel

cvar_t	*armor_bonus_value; //value of armor shards
cvar_t	*health_bonus_value; //value of stimpacks
cvar_t	*powerup_max;
cvar_t	*nuke_max;
cvar_t	*nbomb_max;
cvar_t	*doppleganger_max;
cvar_t	*defender_time;
cvar_t	*vengeance_time;
cvar_t	*hunter_time;
cvar_t	*doppleganger_time;
cvar_t	*quad_time;
cvar_t	*inv_time;
cvar_t	*breather_time;
cvar_t	*enviro_time;
cvar_t	*silencer_shots;
cvar_t	*ir_time;
cvar_t	*double_time;
cvar_t	*quad_fire_time;
cvar_t	*stasis_time;

// maximum values
cvar_t	*max_health;
cvar_t	*max_foodcube_health;
cvar_t	*max_armor;	
cvar_t	*max_bullets;	
cvar_t	*max_shells;	
cvar_t	*max_rockets;	
cvar_t	*max_grenades;	
cvar_t	*max_cells;		
cvar_t	*max_slugs;
cvar_t	*max_magslugs;
cvar_t	*max_traps;
cvar_t	*max_prox;
cvar_t	*max_tesla;
cvar_t	*max_flechettes;
cvar_t	*max_rounds;
cvar_t	*max_shocksphere;
cvar_t	*max_fuel;

// maximum settings if a player gets a pack
cvar_t	*pack_health;
cvar_t	*pack_armor;
cvar_t	*pack_bullets;  // 300
cvar_t	*pack_shells;   // 200
cvar_t	*pack_rockets;  // 100
cvar_t	*pack_grenades; // 100
cvar_t	*pack_cells;    // 300
cvar_t	*pack_slugs;    // 100
cvar_t	*pack_magslugs;
cvar_t	*pack_traps;
cvar_t	*pack_flechettes;
cvar_t	*pack_rounds;
cvar_t	*pack_prox;
cvar_t	*pack_tesla;
cvar_t	*pack_shocksphere;
cvar_t	*pack_fuel;

cvar_t	*bando_bullets;  // 300
cvar_t	*bando_shells;   // 200
cvar_t	*bando_cells;    // 300
cvar_t	*bando_slugs;    // 100
cvar_t	*bando_magslugs;
cvar_t	*bando_flechettes; // 250
cvar_t	*bando_rounds; // 150
cvar_t	*bando_fuel;

cvar_t	*strong_mines;


void lithium_defaults(void)
{
	mega_gibs = gi.cvar("mega_gibs", "0", CVAR_ARCHIVE);
	mp_monster_replace = gi.cvar("mp_monster_replace", "0", CVAR_ARCHIVE);
	mp_monster_ammo_replace = gi.cvar("mp_monster_ammo_replace", "0", CVAR_ARCHIVE);
	kamikaze_flyer_replace = gi.cvar("kamikaze_flyer_replace", "0", CVAR_ARCHIVE);
	allow_player_use_abandoned_turret = gi.cvar("allow_player_use_abandoned_turret", "0", CVAR_ARCHIVE);
	turn_rider = gi.cvar("turn_rider", "1", CVAR_ARCHIVE);
	adjust_train_corners = gi.cvar("adjust_train_corners", "1", CVAR_ARCHIVE);

	tpp_auto = gi.cvar("tpp_auto", "1", CVAR_ARCHIVE);

	ion_ripper_extra_sounds = gi.cvar("ion_ripper_extra_sounds", "1", CVAR_ARCHIVE);
	pack_give_xatrix_ammo = gi.cvar("pack_give_xatrix_ammo", "0", CVAR_ARCHIVE);
	pack_give_rogue_ammo = gi.cvar("pack_give_rogue_ammo", "0", CVAR_ARCHIVE);

	add_velocity_throw = gi.cvar("add_velocity_throw", "0", CVAR_ARCHIVE);

	falling_armor_damage = gi.cvar("falling_armor_damage", "1", CVAR_ARCHIVE);
	player_jump_sounds = gi.cvar("player_jump_sounds", "1", CVAR_ARCHIVE);

	player_max_speed = gi.cvar("player_max_speed", "300", 0);
	player_crouch_speed = gi.cvar("player_crouch_speed", "100", 0); 
	player_accel = gi.cvar("player_accel", "10", 0);
	player_stopspeed = gi.cvar("player_stopspeed", "100", 0);

	use_vwep = gi.cvar("use_vwep", "1", CVAR_ARCHIVE);

	box_shells = gi.cvar("box_shells", "10", 0);
	box_bullets = gi.cvar("box_bullets", "50", 0);
	box_grenades = gi.cvar("box_grenades", "5", 0);
	box_rockets = gi.cvar("box_rockets", "5", 0);
	box_cells = gi.cvar("box_cells", "50", 0);
	box_slugs = gi.cvar("box_slugs", "10", 0);
	box_magslugs = gi.cvar("box_magslugs", "10", 0);
	box_flechettes = gi.cvar("box_flechettes", "50", 0);
	box_prox = gi.cvar("box_prox", "5", 0);
	box_tesla = gi.cvar("box_tesa", "5", 0);
	box_disruptors = gi.cvar("box_disruptors", "15", 0);
	box_shocksphere = gi.cvar("box_shocksphere", "1", 0);
	box_trap = gi.cvar("box_trap", "1", 0);
	box_fuel = gi.cvar("box_fuel", "500", 0);

	armor_bonus_value = gi.cvar("armor_bonus_value", "2", 0);
	health_bonus_value = gi.cvar("health_bonus_value", "2", 0);
	powerup_max = gi.cvar("powerup_max", "2", 0);
	doppleganger_max = gi.cvar("doppleganger_max", "1", 0);
	nuke_max = gi.cvar("nuke_max", "1", 0);
	nbomb_max = gi.cvar("nbomb_max", "4", 0);
	defender_time = gi.cvar("defender_time", "60", 0);
	vengeance_time = gi.cvar("vengeance_time", "60", 0);
	hunter_time = gi.cvar("hunter_time", "60", 0);
	doppleganger_time = gi.cvar("doppleganger_time", "30", 0);
	quad_time = gi.cvar("quad_time", "30", 0);
	inv_time = gi.cvar("inv_time", "30", 0);
	breather_time = gi.cvar("breather_time", "30", 0);
	enviro_time = gi.cvar("enviro_time", "30", 0);
	silencer_shots = gi.cvar("silencer_shots", "30", 0);
	ir_time = gi.cvar("ir_time", "30", 0);
	double_time = gi.cvar("double_time", "30", 0);
	quad_fire_time = gi.cvar("quad_fire_time", "30", 0);
	stasis_time = gi.cvar("stasis_time", "30", 0);

	pack_health = gi.cvar("pack_health", "120", 0);
	pack_armor = gi.cvar("pack_armor", "250", 0);
	pack_bullets = gi.cvar("pack_bullets", "300", 0);
	pack_shells = gi.cvar("pack_shells", "200", 0);
	pack_rockets = gi.cvar("pack_rockets", "100", 0);
	pack_grenades = gi.cvar("pack_grenades", "100", 0);
	pack_cells = gi.cvar("pack_cells", "300", 0);
	pack_slugs = gi.cvar("pack_slugs", "100", 0);
	pack_magslugs = gi.cvar("pack_magslugs", "100", 0);
	pack_traps = gi.cvar("pack_traps", "50", 0);
	pack_flechettes = gi.cvar("pack_flechettes", "300", 0);
	pack_rounds = gi.cvar("pack_rounds", "200", 0);
	pack_prox = gi.cvar("pack_prox", "100", 0);
	pack_tesla = gi.cvar("pack_tesla", "100", 0);
	pack_shocksphere = gi.cvar("pack_shocksphere", "20", 0);
	pack_fuel = gi.cvar("pack_fuel", "2000", 0);

	bando_bullets = gi.cvar("bando_bullets", "250", 0);
	bando_shells = gi.cvar("bando_shells", "150", 0);
	bando_cells = gi.cvar("bando_cells", "250", 0);
	bando_slugs = gi.cvar("bando_slugs", "75", 0);
	bando_magslugs = gi.cvar("bando_magslugs", "75", 0);
	bando_flechettes = gi.cvar("bando_flechettes", "250", 0);
	bando_rounds = gi.cvar("bando_rounds", "150", 0);
	bando_fuel = gi.cvar("bando_fuel", "1500", 0);

	max_health = gi.cvar("max_health", "100", 0);
	max_foodcube_health = gi.cvar("max_foodcube_health", "300", 0);
	max_armor = gi.cvar("max_armor", "200", 0);
	max_bullets = gi.cvar("max_bullets", "200", 0);
	max_shells = gi.cvar("max_shells", "100", 0);
	max_rockets = gi.cvar("max_rockets", "50", 0);
	max_grenades = gi.cvar("max_grenades", "50", 0);
	max_cells = gi.cvar("max_cells", "200", 0);
	max_slugs = gi.cvar("max_slugs", "50", 0);
	max_magslugs = gi.cvar("max_magslugs", "60", 0);
	max_traps = gi.cvar("max_traps", "10", 0);
	max_prox = gi.cvar("max_prox", "50", 0);
	max_tesla = gi.cvar("max_tesla", "50", 0);
	max_flechettes = gi.cvar("max_flechettes", "200", 0);
	max_rounds = gi.cvar("max_rounds", "100", 0);
	max_shocksphere = gi.cvar("max_shocksphere", "10", 0);
	max_fuel = gi.cvar("max_fuel", "1000", 0);

	blaster_damage = gi.cvar("blaster_damage", "10", 0);
	blaster_speed = gi.cvar("blaster_speed", "1000", 0);
	blaster_color = gi.cvar("blaster_color", "1", 0);

	shotgun_damage = gi.cvar("shotgun_damage", "6", 0);
	shotgun_count = gi.cvar("shotgun_count", "12", 0);
	shotgun_hspread = gi.cvar("shotgun_hspread", "500", 0);
	shotgun_vspread = gi.cvar("shotgun_vspread", "500", 0);

	sshotgun_damage = gi.cvar("sshotgun_damage", "6", 0);
	sshotgun_count = gi.cvar("sshotgun_count", "22", 0);
	sshotgun_hspread = gi.cvar("sshotgun_hspread", "1000", 0);
	sshotgun_vspread = gi.cvar("sshotgun_vspread", "500", 0);

	machinegun_damage = gi.cvar("machinegun_damage", "8", 0);
	machinegun_hspread = gi.cvar("machinegun_hspread", "300", 0);
	machinegun_vspread = gi.cvar("machinegun_vspread", "500", 0);

	chaingun_damage = gi.cvar("chaingun_damage", "8", 0);
	chaingun_hspread = gi.cvar("chaingun_hspread", "300", 0);
	chaingun_vspread = gi.cvar("chaingun_vspread", "500", 0);

	grenade_damage = gi.cvar("grenade_damage", "120", 0);
	grenade_radius = gi.cvar("grenade_radius", "160", 0);
	grenade_speed = gi.cvar("grenade_speed", "600", 0);

	hand_grenade_damage = gi.cvar("hand_grenade_damage", "125", 0);
	hand_grenade_radius = gi.cvar("hand_grenade_radius", "165", 0);
	
	rocket_damage = gi.cvar("rocket_damage", "100", 0);
	rocket_damage2 = gi.cvar("rocket_damage2", "20", 0);
	rocket_rdamage = gi.cvar("rocket_rdamage", "120", 0);
	rocket_radius = gi.cvar("rocket_radius", "140", 0);
	rocket_speed = gi.cvar("rocket_speed", "650", 0);

	hyperblaster_damage = gi.cvar("hyperblaster_damage", "10", 0);
	hyperblaster_speed = gi.cvar("hyperblaster_speed", "1000", 0);
	hyperblaster_color = gi.cvar("hyperblaster_color", "1", 0);

	railgun_damage = gi.cvar("railgun_damage", "150", 0);
	rail_color = gi.cvar("rail_color", "1", 0);

	bfg_damage = gi.cvar("bfg_damage", "500", 0);
	bfg_damage2 = gi.cvar("bfg_damage2", "10", 0);
	bfg_rdamage = gi.cvar("bfg_rdamage", "200", 0);
	bfg_radius = gi.cvar("bfg_radius", "1000", 0);
	bfg_speed = gi.cvar("bfg_speed", "400", 0);

	jump_kick_damage = gi.cvar("jump_kick_damage", "10", 0);

	tesla_damage = gi.cvar("tesla_damage", "3", 0);
	tesla_radius = gi.cvar("tesla_radius", "128", 0);
	tesla_life = gi.cvar("tesla_life", "30", 0);
	tesla_health = gi.cvar("tesla_health", "20", 0);

	chainfist_damage = gi.cvar("chainfist_damage", "30", 0);

	plasmabeam_damage = gi.cvar("plasmabeam_damage", "15", 0);

	etf_rifle_damage = gi.cvar("etf_rifle_damage", "10", 0);
	etf_rifle_radius_damage = gi.cvar("etf_rifle_radius_damage", "20", 0);
	etf_rifle_radius = gi.cvar("etf_rifle_radius", "100", 0);
	etf_rifle_speed = gi.cvar("etf_rifle_speed", "750", 0);

	disruptor_damage = gi.cvar("disruptor_damage", "50", 0);
	disruptor_speed = gi.cvar("disruptor_speed", "1000", 0);

	prox_damage = gi.cvar("prox_damage", "135", 0);
	prox_radius = gi.cvar("prox_radius", "192", 0);
	prox_speed = gi.cvar("prox_speed", "600", 0);
	prox_life = gi.cvar("prox_life", "600", 0);
	prox_health = gi.cvar("prox_health", "20", 0);

	ionripper_damage = gi.cvar("ionripper_damage", "30", 0);
	ionripper_speed = gi.cvar("ionripper_speed", "500", 0);

	phalanx_damage = gi.cvar("phalanx_damage", "70", 0);
	phalanx_damage2 = gi.cvar("phalanx_damage2", "10", 0);
	phalanx_radius_damage = gi.cvar("phalanx_radius_damage", "120", 0);
	phalanx_radius = gi.cvar("phalanx_radius", "120", 0);
	phalanx_speed = gi.cvar("phalanx_speed", "725", 0);

	trap_life = gi.cvar("trap_life", "30", 0);
	trap_health = gi.cvar("trap_health", "30", 0);

	nuke_delay = gi.cvar("nuke_delay", "4", 0);
	nuke_life = gi.cvar("nuke_life", "6", 0);
	nuke_radius = gi.cvar("nuke_radius", "512", 0);

	nbomb_delay = gi.cvar("nbomb_delay", "4", 0);
	nbomb_life = gi.cvar("nbomb_life", "6", 0);
	nbomb_radius = gi.cvar("nbomb_radius", "256", 0);
	nbomb_damage = gi.cvar("nbomb_damage", "5000", 0);

	defender_blaster_damage = gi.cvar("defender_blaster_damage", "10", 0);
	defender_blaster_speed = gi.cvar("defender_blaster_speed", "1000", 0);

	vengeance_health_threshold = gi.cvar("vengeance_health_threshold", "25", 0);

	shockwave_bounces = gi.cvar("shockwave_bounces", "6", 0);
	shockwave_damage = gi.cvar("shockwave_damage", "80", 0);
	shockwave_damage2 = gi.cvar("shockwave_damage2", "40", 0);
	shockwave_rdamage = gi.cvar("shockwave_rdamage", "120", 0);
	shockwave_radius = gi.cvar("shockwave_radius", "300", 0);
	shockwave_speed = gi.cvar("shockwave_speed", "650", 0);
	shockwave_effect_damage = gi.cvar("shockwave_effect_damage", "100", 0);
	shockwave_effect_radius = gi.cvar("shockwave_effect_radius", "340", 0);

	// Rogue
	strong_mines = gi.cvar ("strong_mines", "0", 0);
}
