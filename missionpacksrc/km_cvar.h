/* Knightmare's cvar header file */

extern	cvar_t	*mega_gibs; // whether to spawn extra gibs, default to 0
extern	cvar_t	*mp_monster_replace; //whether to replace monsters with mission pack modified variants
extern	cvar_t	*mp_monster_ammo_replace; //whether to replace monsters' ammo items if above is enabled
extern	cvar_t	*kamikaze_flyer_replace; //whether to replace some flyers with kamikaze flyers, probability 0-1
extern	cvar_t	*allow_player_use_abandoned_turret; //whether to allow player to use turrets in exisiting maps
extern	cvar_t	*turn_rider;  //whether to turn player on rotating object
extern	cvar_t	*adjust_train_corners; //whether to subtract (1,1,1) from train path corners to fix misalignments

//Lazarus
//extern	cvar_t	*cd_loopcount;

extern	cvar_t	*tpp_auto; //whether to automatically go into third-person when pushing a pushable

extern	cvar_t	*ion_ripper_extra_sounds;
extern	cvar_t	*pack_give_xatrix_ammo;
extern	cvar_t	*pack_give_rogue_ammo;

extern	cvar_t	*add_velocity_throw;	//whether to add player's velocity to thrown objects

extern	cvar_t	*falling_armor_damage; //whether player's armor absorbs damage from falling
extern	cvar_t	*player_jump_sounds; //whether to play that STUPID grunting sound when the player jumps

// Server-side speed control stuff
extern	cvar_t	*player_max_speed;
extern	cvar_t	*player_crouch_speed;
extern	cvar_t	*player_accel;
extern	cvar_t	*player_stopspeed;

extern	cvar_t	*use_vwep;

extern	cvar_t	*max_health;
extern	cvar_t	*max_foodcube_health;
extern	cvar_t	*max_armor;
extern	cvar_t	*max_bullets;
extern	cvar_t	*max_shells;
extern	cvar_t	*max_rockets;
extern	cvar_t	*max_grenades;
extern	cvar_t	*max_cells;
extern	cvar_t	*max_slugs;
extern	cvar_t	*max_magslugs;
extern	cvar_t	*max_traps;
extern	cvar_t	*max_prox;
extern	cvar_t	*max_tesla;
extern	cvar_t	*max_flechettes;
extern	cvar_t	*max_rounds;
extern	cvar_t	*max_shocksphere;
extern	cvar_t	*max_fuel;


// maximum settings if a player gets a pack
extern	cvar_t	*pack_bullets;
extern	cvar_t	*pack_shells;
extern	cvar_t	*pack_rockets;
extern	cvar_t	*pack_grenades;
extern	cvar_t	*pack_cells;
extern	cvar_t	*pack_slugs;
extern	cvar_t	*pack_magslugs;
extern	cvar_t	*pack_traps;
extern	cvar_t	*pack_flechettes;
extern	cvar_t	*pack_rounds;
extern	cvar_t	*pack_prox;
extern	cvar_t	*pack_tesla;
extern	cvar_t	*pack_shocksphere;
extern	cvar_t	*pack_fuel;

extern	cvar_t	*bando_bullets;
extern	cvar_t	*bando_shells;
extern	cvar_t	*bando_cells;
extern	cvar_t	*bando_slugs;
extern	cvar_t	*bando_magslugs;
extern	cvar_t	*bando_flechettes;
extern	cvar_t	*bando_rounds;
extern	cvar_t	*bando_fuel;

// weapon balancing
extern	cvar_t	*blaster_damage;
extern	cvar_t	*blaster_speed;
extern	cvar_t	*blaster_color;

extern	cvar_t	*shotgun_damage;
extern	cvar_t	*shotgun_count;
extern	cvar_t	*shotgun_hspread;
extern	cvar_t	*shotgun_vspread;

extern	cvar_t	*sshotgun_damage;
extern	cvar_t	*sshotgun_count;
extern	cvar_t	*sshotgun_hspread;
extern	cvar_t	*sshotgun_vspread;

extern	cvar_t	*machinegun_damage;
extern	cvar_t	*machinegun_hspread;
extern	cvar_t	*machinegun_vspread;

extern	cvar_t	*chaingun_damage;
extern	cvar_t	*chaingun_hspread;
extern	cvar_t	*chaingun_vspread;

extern	cvar_t	*grenade_damage;
extern	cvar_t	*grenade_radius;
extern	cvar_t	*grenade_speed;

extern	cvar_t	*hand_grenade_damage;
extern	cvar_t	*hand_grenade_radius;

extern	cvar_t	*rocket_damage;
extern	cvar_t	*rocket_damage2;
extern	cvar_t	*rocket_rdamage;
extern	cvar_t	*rocket_radius;
extern	cvar_t	*rocket_speed;

extern	cvar_t	*hyperblaster_damage;
extern	cvar_t	*hyperblaster_speed;
extern	cvar_t	*hyperblaster_color;

extern	cvar_t	*railgun_damage;
extern	cvar_t	*rail_color;

extern	cvar_t	*bfg_damage;
extern	cvar_t	*bfg_damage2;
extern	cvar_t	*bfg_rdamage;
extern	cvar_t	*bfg_radius;
extern	cvar_t	*bfg_speed;

extern	cvar_t	*jump_kick_damage;

extern	cvar_t	*tesla_damage;
extern	cvar_t	*tesla_radius;
extern	cvar_t	*tesla_life;
extern	cvar_t	*tesla_health;

extern	cvar_t	*chainfist_damage;

extern	cvar_t	*plasmabeam_damage;

extern	cvar_t	*etf_rifle_damage;
extern	cvar_t	*etf_rifle_radius_damage;
extern	cvar_t	*etf_rifle_radius;
extern	cvar_t	*etf_rifle_speed;

extern	cvar_t	*disruptor_damage;
extern	cvar_t	*disruptor_speed;

extern	cvar_t	*prox_damage;
extern	cvar_t	*prox_radius;
extern	cvar_t	*prox_speed;
extern	cvar_t	*prox_life;
extern	cvar_t	*prox_health;

extern	cvar_t	*ionripper_damage;
extern	cvar_t	*ionripper_speed;

extern	cvar_t	*phalanx_damage;
extern	cvar_t	*phalanx_damage2;
extern	cvar_t	*phalanx_radius_damage;
extern	cvar_t	*phalanx_radius;
extern	cvar_t	*phalanx_speed;

extern	cvar_t	*trap_life;
extern	cvar_t	*trap_health;

extern	cvar_t	*nuke_delay;
extern	cvar_t	*nuke_life;
extern	cvar_t	*nuke_radius;

extern	cvar_t	*nbomb_delay;
extern	cvar_t	*nbomb_life;
extern	cvar_t	*nbomb_radius;
extern	cvar_t	*nbomb_damage;

extern	cvar_t	*double_time;
extern	cvar_t	*quad_fire_time;

extern	cvar_t	*defender_blaster_damage;
extern	cvar_t	*defender_blaster_speed;

extern	cvar_t	*vengeance_health_threshold;

extern	cvar_t	*shockwave_bounces;
extern	cvar_t	*shockwave_damage;
extern	cvar_t	*shockwave_damage2;
extern	cvar_t	*shockwave_rdamage;
extern	cvar_t	*shockwave_speed;
extern	cvar_t	*shockwave_radius;
extern	cvar_t	*shockwave_effect_damage;
extern	cvar_t	*shockwave_effect_radius;

extern	cvar_t	*doppleganger_time;

extern	cvar_t	*box_shells; //value of shells
extern	cvar_t	*box_bullets; //value of bullets
extern	cvar_t	*box_grenades; //value of grenade pack
extern	cvar_t	*box_rockets; //value of rocket pack
extern	cvar_t	*box_cells; //value of cell pack
extern	cvar_t	*box_slugs; //value of slug box
extern	cvar_t	*box_magslugs; //value ofmagslug box
extern	cvar_t	*box_flechettes; //value of flechettes
extern	cvar_t	*box_prox; //value of prox
extern	cvar_t	*box_tesla; //value of tesla pack
extern	cvar_t	*box_disruptors; //value of disruptor pack
extern	cvar_t	*box_shocksphere; //value of shocksphere
extern	cvar_t	*box_trap; //value of trap
extern	cvar_t	*box_fuel; //value of fuel

extern cvar_t	*armor_bonus_value; //value of armor shards
extern cvar_t	*health_bonus_value; //value of stimpacks
extern cvar_t	*powerup_max;
extern cvar_t	*nuke_max;
extern cvar_t	*nbomb_max;
extern cvar_t	*doppleganger_max;
extern cvar_t	*defender_time;
extern cvar_t	*vengeance_time;
extern cvar_t	*hunter_time;
extern cvar_t	*quad_time;
extern cvar_t	*inv_time;
extern cvar_t	*breather_time;
extern cvar_t	*enviro_time;
extern cvar_t	*silencer_shots;
extern cvar_t	*ir_time;
extern cvar_t	*double_time;
extern cvar_t	*quad_fire_time;
extern cvar_t	*stasis_time;

