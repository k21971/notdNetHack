/*	SCCS Id: @(#)decl.h	3.4	2001/12/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DECL_H
#define DECL_H

#define E extern

E int NDECL((*occupation));
E int NDECL((*afternmv));

E const char *hname;
E int hackpid;
#if defined(UNIX) || defined(VMS)
E int locknum;
#endif
#ifdef DEF_PAGER
E char *catmore;
#endif	/* DEF_PAGER */

E char SAVEF[];
#ifdef MICRO
E char SAVEP[];
#endif

E NEARDATA int bases[MAXOCLASSES];

E long has_loaded_bones;

E long last_clear_screen;

E NEARDATA int multi;
E char multi_txt[BUFSZ];
#if 0
E NEARDATA int warnlevel;
#endif
E NEARDATA int nroom;
E NEARDATA int nsubroom;
E NEARDATA int occtime;

#define WARNCOUNT 6			/* number of different warning levels */
E uchar warnsyms[WARNCOUNT];

E int x_maze_max, y_maze_max;

#ifdef REDO
E NEARDATA int in_doagain;
#endif

E struct dgn_topology {		/* special dungeon levels for speed */
	/*Dungeons of Doom*/
    d_level	d_village_level;	
 	int village_variant;
#define GRASS_VILLAGE 1
#define LAKE_VILLAGE 2
#define FOREST_VILLAGE 3
#define CAVE_VILLAGE 4
    d_level	d_oracle_level;
    d_level	d_bigroom_level;	/* unused */
    d_level	d_bigroomb_level;	/* unused */
#ifdef REINCARNATION
    d_level	d_rogue_level;
#endif
	/*Medusa's Island + Friends */
	int		challenge_variant;
#define MEDUSA_LEVEL1	1
#define MEDUSA_LEVEL2	2
#define MEDUSA_LEVEL3	3
#define MEDUSA_LEVEL4	4
#define GRUE_LEVEL1		5
#define GRUE_LEVEL2		6
    d_level	d_challenge_level;
	/*Castle*/
	d_level	d_stronghold_level;
	/*Gehennom*/
    d_level	d_valley_level;
    d_level	d_stair1_level;
    d_level	d_stair2_level;
    d_level	d_stair3_level;
    d_level	d_wiz1_level;
    d_level	d_wiz2_level;
    d_level	d_wiz3_level;
//    d_level	d_juiblex_level;
//    d_level	d_orcus_level;
    d_level	d_hell1_level;
	int		hell1_variant;
#define BAEL_LEVEL		1
#define DISPATER_LEVEL	2
#define MAMMON_LEVEL	3
#define BELIAL_LEVEL	4
#define CHROMA_LEVEL	5
	d_level	d_hell2_level;
	int		hell2_variant;
#define LEVIATHAN_LEVEL	1
#define LILITH_LEVEL	2
#define BAALZEBUB_LEVEL	3
#define MEPHISTOPHELES_LEVEL	4
	d_level	d_hell3_level;
	d_level	d_abyss_level;
	int		abyss_variant;
#define JUIBLEX_LEVEL	1
#define JUIBLEX_RES		1
#define ZUGGTMOY_LEVEL	2
#define ZUGGTMOY_RES	2
#define YEENOGHU_LEVEL	3
#define BAPHOMET_LEVEL	4
#define NIGHT_LEVEL		5
#define NIGHT_RES		3
#define KOSTCH_LEVEL	6
	d_level	d_abys2_level;
	int		abys2_variant;
#define ORCUS_LEVEL		1
#define MALCANTHET_LEVEL	2
#define GRAZ_ZT_LEVEL	3
#define LOLTH_LEVEL		4
	d_level	d_brine_level;
	int		brine_variant;
#define DEMOGORGON_LEVEL	1
#define DAGON_LEVEL		2
#define LAMASHTU_LEVEL		3
//    d_level	d_baalzebub_level;	/* unused */
//    d_level	d_asmodeus_level;	/* unused */
    d_level	d_portal_level;		/* only in goto_level() [do.c] */
    d_level	d_sanctum_level;
	/*Planes*/
    d_level	d_earth_level;
    d_level	d_water_level;
    d_level	d_fire_level;
    d_level	d_air_level;
    d_level	d_astral_level;
	/*Vlad's Tower*/
    xchar	d_tower_dnum;
    /*Void levels*/
    xchar	d_void_dnum;
    d_level	d_ilsensin_level;
    d_level	d_farvoid_level;
    d_level	d_aligvoid_level;
    d_level	d_nrvoid2_level;
    d_level	d_nearvoid_level;

    xchar	d_sacris_dnum;
    d_level	d_sacris_level;

    xchar	d_nowhere_dnum;
    d_level	d_nowhere_level;

	/*The Lost Tomb*/
    xchar	d_tomb_dnum;
	/*The Sunless Sea*/
    xchar	d_sea_dnum;
	int		sea_variant;
#define SUNLESS_SEA_LEVEL		1
#define PARADISE_ISLAND_LEVEL	2
#define SUNKEN_CITY_LEVEL		3
#define PEANUT_ISLAND_LEVEL		4
	/*The Temple of Moloch*/
    xchar	d_temple_dnum;
	/*Sokoban*/
    xchar	d_sokoban_dnum;
	/*Mines of the Gnomes of Zurich*/
    xchar	d_mines_dnum;
	/*Ice caves*/
    xchar	d_ice_dnum;
    	/*Black forest*/
    xchar	d_blackforest_dnum;
    	/*Dismal swamp*/
    xchar	d_dismalswamp_dnum;
    	/*Archipelago*/
    xchar	d_archipelago_dnum;
	/*Quest Levels*/
	xchar	d_quest_dnum;
    d_level	d_qstart_level, d_qlocate_level, d_nemesis_level;
	/*Neutral Quest*/
	xchar	d_neutral_dnum;
	xchar	d_rlyeh_dnum;
    d_level	d_gatetown_level, d_spire_level, d_sum_of_all_level, d_lethe_headwaters, d_bridge_temple, d_lethe_temples, d_rlyeh_level;
    d_level	d_nkai_a, d_nkai_b, d_nkai_c, d_nkai_z;
	/*Spire*/
   	xchar d_spire_dnum;
	d_level	d_sigil_level;
	/*Chaos Quest*/
	xchar	d_chaos_dvariant;
	xchar	d_chaos_dnum;
    d_level	d_chaosf_level, d_chaoss_level, d_chaost_level, d_chaosm_level,
		d_chaosfrh_level, d_chaosffh_level, d_chaossth_level,
		d_chaosvth_level, d_chaose_level;
	/*Chaos Quest 2*/
    d_level	d_elshava_level;
    d_level	d_lastspire_level;
	/*Chaos Quest 2*/
    d_level	d_forest_1_level;
    d_level	d_forest_2_level;
    d_level	d_forest_3_level;
    d_level	d_ford_level;
    d_level	d_forest_4_level;
    d_level	d_mordor_1_level;
    d_level	d_mordor_2_level;
    d_level	d_spider_level;
    d_level	d_mordor_depths_1_level;
    d_level	d_mordor_depths_2_level;
    d_level	d_mordor_depths_3_level;
    d_level	d_borehole_1_level;
    d_level	d_borehole_2_level;
    d_level	d_borehole_3_level;
    d_level	d_borehole_4_level;
	/*Law Quest*/
	xchar	d_law_dnum;
	d_level d_path1, d_path2, d_path3, d_illregrd, 
		d_arcadia1, d_arcadia2, d_arcadia3, d_arcward, d_arcfort,
		d_tower1, d_tower2, d_tower3, d_tower4, d_tower5,
		d_tower6, d_tower7, d_tower8, d_tower9, d_towertop;
	char alt_tower;
	char alt_tulani; /*Note: TULANI_CAST == FALSE, i.e. no changes needed. */
#define TULANI_CASTE	0
#define GAE_CASTE		1
#define BRIGHID_CASTE	2
#define UISCERRE_CASTE	3
#define CAILLEA_CASTE	4
	int eprecursor_typ;
#define	PRE_DRACAE	1
#define	PRE_POLYP	2
	/*Fort Knox*/
    d_level	d_knox_level;
    d_level	d_icetown_level;
    d_level	d_iceboss_level;
    d_level	d_bftemple_level;
    d_level	d_bfboss_level;
    d_level	d_dsbog_level;
    d_level	d_dsboss_level;
    d_level	d_leveetwn_level;
    d_level	d_arcboss_level;
	d_level d_minetown_level;
#ifdef RECORD_ACHIEVE
    d_level     d_mineend_level;
    d_level     d_sokoend_level;
#endif

} dungeon_topology;
/* macros for accesing the dungeon levels by their old names */
	/*Dungeons of Doom*/
#define village_level		(dungeon_topology.d_village_level)
#define oracle_level		(dungeon_topology.d_oracle_level)
#define bigroom_level		(dungeon_topology.d_bigroom_level)
#ifdef REINCARNATION
#define rogue_level		(dungeon_topology.d_rogue_level)
#endif
#define challenge_level		(dungeon_topology.d_challenge_level)
#define stronghold_level	(dungeon_topology.d_stronghold_level)
	/*Gehennom*/
#define valley_level		(dungeon_topology.d_valley_level)
#define wiz1_level		(dungeon_topology.d_wiz1_level)
#define wiz2_level		(dungeon_topology.d_wiz2_level)
#define wiz3_level		(dungeon_topology.d_wiz3_level)
#define abyss1_level	(dungeon_topology.d_abyss_level)
#define abyss1_juiblex_level	(dungeon_topology.d_abyss_juiblex_level)
#define abyss2_level	(dungeon_topology.d_abys2_level)
#define orcus_level		(dungeon_topology.d_abys2_level)
#define abyss3_level	(dungeon_topology.d_brine_level)
#define hell1_level		(dungeon_topology.d_hell1_level)
#define hell2_level		(dungeon_topology.d_hell2_level)
#define hell3_level		(dungeon_topology.d_hell3_level)
////define juiblex_level		(dungeon_topology.d_juiblex_level)
//define orcus_level		(dungeon_topology.d_orcus_level)
//define demogoron_level		(dungeon_topology.d_demogorgon_level)
//define baalzebub_level		(dungeon_topology.d_baalzebub_level)
//define asmodeus_level		(dungeon_topology.d_asmodeus_level)
#define portal_level		(dungeon_topology.d_portal_level)
#define sanctum_level		(dungeon_topology.d_sanctum_level)
/*Void*/
#define ilsensin_level		(dungeon_topology.d_ilsensin_level)
#define farvoid_level		(dungeon_topology.d_farvoid_level)
#define aligvoid_level		(dungeon_topology.d_aligvoid_level)
#define nrvoid2_level		(dungeon_topology.d_nrvoid2_level)
#define nearvoid_level		(dungeon_topology.d_nearvoid_level)

#define sacris_level		(dungeon_topology.d_sacris_level)

#define nowhere_level		(dungeon_topology.d_nowhere_level)

	/*Planes*/
#define earth_level		(dungeon_topology.d_earth_level)
#define water_level		(dungeon_topology.d_water_level)
#define fire_level		(dungeon_topology.d_fire_level)
#define air_level		(dungeon_topology.d_air_level)
#define astral_level		(dungeon_topology.d_astral_level)
	/*Vlad's Tower*/
#define tower_dnum		(dungeon_topology.d_tower_dnum)
	/*The Lost Tomb*/
#define tomb_dnum		(dungeon_topology.d_tomb_dnum)
	/*The Sunless Sea*/
#define sea_dnum		(dungeon_topology.d_sea_dnum)
	/*The Temple of Moloch*/
#define temple_dnum		(dungeon_topology.d_temple_dnum)
	/*Medusa's Island + Friends*/
#define challenge_level		(dungeon_topology.d_challenge_level)
	/*Sokoban*/
#define sokoban_dnum		(dungeon_topology.d_sokoban_dnum)
	/*Mines of the Gnomes of Zurich*/
#define mines_dnum		(dungeon_topology.d_mines_dnum)
	/*Ice caves*/
#define ice_dnum		(dungeon_topology.d_ice_dnum)
	/*Black forest*/
#define blackforest_dnum		(dungeon_topology.d_blackforest_dnum)
	/*Dismal swamp*/
#define dismalswamp_dnum		(dungeon_topology.d_dismalswamp_dnum)
	/*Archipelago*/
#define archipelago_dnum		(dungeon_topology.d_archipelago_dnum)
	/*Quest Levels*/
#define quest_dnum		(dungeon_topology.d_quest_dnum)
#define qstart_level		(dungeon_topology.d_qstart_level)
#define qlocate_level		(dungeon_topology.d_qlocate_level)
#define nemesis_level		(dungeon_topology.d_nemesis_level)
	/*Neutral Quest*/
#define neutral_dnum		(dungeon_topology.d_neutral_dnum)
#define rlyeh_dnum		(dungeon_topology.d_rlyeh_dnum)
#define gatetown_level		(dungeon_topology.d_gatetown_level)
#define spire_level		(dungeon_topology.d_spire_level)
#define sum_of_all_level		(dungeon_topology.d_sum_of_all_level)
#define lethe_headwaters		(dungeon_topology.d_lethe_headwaters)
#define bridge_temple		(dungeon_topology.d_bridge_temple)
#define lethe_temples		(dungeon_topology.d_lethe_temples)
#define nkai_a_level		(dungeon_topology.d_nkai_a)
#define nkai_b_level		(dungeon_topology.d_nkai_b)
#define nkai_c_level		(dungeon_topology.d_nkai_c)
#define nkai_z_level		(dungeon_topology.d_nkai_z)
#define rlyeh_level		(dungeon_topology.d_rlyeh_level)
	/*Spire*/
#define spire_dnum		(dungeon_topology.d_spire_dnum)
#define sigil_level		(dungeon_topology.d_sigil_level)
	/*Chaos Quest*/
#define chaos_dvariant		(dungeon_topology.d_chaos_dvariant)
#define TEMPLE_OF_CHAOS	0
#define MITHARDIR		1
#define MORDOR			2
#define chaos_dnum		(dungeon_topology.d_chaos_dnum)
#define chaosf_level		(dungeon_topology.d_chaosf_level)
#define chaoss_level		(dungeon_topology.d_chaoss_level)
#define chaost_level		(dungeon_topology.d_chaost_level)
#define chaosm_level		(dungeon_topology.d_chaosm_level)
#define chaosfrh_level		(dungeon_topology.d_chaosfrh_level)
#define chaosffh_level		(dungeon_topology.d_chaosffh_level)
#define chaossth_level		(dungeon_topology.d_chaossth_level)
#define chaosvth_level		(dungeon_topology.d_chaosvth_level)
#define chaose_level		(dungeon_topology.d_chaose_level)
	/*Chaos Quest 2*/
#define elshava_level		(dungeon_topology.d_elshava_level)
#define lastspire_level		(dungeon_topology.d_lastspire_level)
	/*Chaos Quest 3*/
#define forest_1_level		(dungeon_topology.d_forest_1_level)
#define forest_2_level		(dungeon_topology.d_forest_2_level)
#define forest_3_level		(dungeon_topology.d_forest_3_level)
#define ford_level			(dungeon_topology.d_ford_level)
#define forest_4_level		(dungeon_topology.d_forest_4_level)
#define mordor_1_level		(dungeon_topology.d_mordor_1_level)
#define mordor_2_level		(dungeon_topology.d_mordor_2_level)
#define spider_level		(dungeon_topology.d_spider_level)
#define mordor_depths_1_level		(dungeon_topology.d_mordor_depths_1_level)
#define mordor_depths_2_level		(dungeon_topology.d_mordor_depths_2_level)
#define mordor_depths_3_level		(dungeon_topology.d_mordor_depths_3_level)
#define borehole_1_level	(dungeon_topology.d_borehole_1_level)
#define borehole_2_level	(dungeon_topology.d_borehole_2_level)
#define borehole_3_level	(dungeon_topology.d_borehole_3_level)
#define borehole_4_level	(dungeon_topology.d_borehole_4_level)
	/*Law Quest*/
#define law_dnum		(dungeon_topology.d_law_dnum)
#define path1_level		(dungeon_topology.d_path1)
#define path2_level		(dungeon_topology.d_path2)
#define path3_level		(dungeon_topology.d_path3)
#define illregrd_level	(dungeon_topology.d_illregrd)
#define arcadia1_level	(dungeon_topology.d_arcadia1)
#define arcadia2_level	(dungeon_topology.d_arcadia2)
#define arcadia3_level	(dungeon_topology.d_arcadia3)
#define arcward_level	(dungeon_topology.d_arcward)
#define arcfort_level	(dungeon_topology.d_arcfort)
#define tower1_level	(dungeon_topology.d_tower1)
#define tower2_level	(dungeon_topology.d_tower2)
#define tower3_level	(dungeon_topology.d_tower3)
//define tower4_level	(dungeon_topology.d_tower4)
//define tower5_level	(dungeon_topology.d_tower5)
//define tower6_level	(dungeon_topology.d_tower6)
//define tower7_level	(dungeon_topology.d_tower7)
//define tower8_level	(dungeon_topology.d_tower8)
//define tower9_level	(dungeon_topology.d_tower9)
#define towertop_level	(dungeon_topology.d_towertop)
	/*Fort Knox*/
#define knox_level		(dungeon_topology.d_knox_level)
/*Ice caves*/
#define icetown_level           (dungeon_topology.d_icetown_level)
#define iceboss_level           (dungeon_topology.d_iceboss_level)
/*Black forest*/
#define bftemple_level           (dungeon_topology.d_bftemple_level)
#define bfboss_level           (dungeon_topology.d_bfboss_level)
/*Dismal swamp*/
#define dsbog_level           (dungeon_topology.d_dsbog_level)
#define dsboss_level           (dungeon_topology.d_dsboss_level)
/*Archipelago*/
#define leveetwn_level           (dungeon_topology.d_leveetwn_level)
#define arcboss_level           (dungeon_topology.d_arcboss_level)

#define minetown_level           (dungeon_topology.d_minetown_level)
#ifdef RECORD_ACHIEVE
#define mineend_level           (dungeon_topology.d_mineend_level)
#define sokoend_level           (dungeon_topology.d_sokoend_level)
#endif

E NEARDATA stairway dnstair, upstair;		/* stairs up and down */
#define xdnstair	(dnstair.sx)
#define ydnstair	(dnstair.sy)
#define xupstair	(upstair.sx)
#define yupstair	(upstair.sy)

E NEARDATA stairway dnladder, upladder;		/* ladders up and down */
#define xdnladder	(dnladder.sx)
#define ydnladder	(dnladder.sy)
#define xupladder	(upladder.sx)
#define yupladder	(upladder.sy)

E NEARDATA stairway sstairs;

E NEARDATA dest_area updest, dndest;	/* level-change destination areas */

E NEARDATA coord inv_pos;
E NEARDATA dungeon dungeons[];
E NEARDATA s_level *sp_levchn;
#define dunlev_reached(x)	(dungeons[(x)->dnum].dunlev_ureached)

#include "quest.h"
E struct q_score quest_status;

E NEARDATA char pl_character[PL_CSIZ];
E NEARDATA char pl_race;		/* character's race */

E NEARDATA char pl_fruit[PL_FSIZ];
E NEARDATA int current_fruit;
E NEARDATA struct fruit *ffruit;

E NEARDATA char tune[6];

#define MAXLINFO (MAXDUNGEON * MAXLEVEL)
E struct linfo level_info[MAXLINFO];

E NEARDATA struct sinfo {
	int gameover;		/* self explanatory? */
	int stopprint;		/* inhibit further end of game disclosure */
#if defined(UNIX) || defined(VMS) || defined (__EMX__) || defined(WIN32)
	int done_hup;		/* SIGHUP or moral equivalent received
				 * -- no more screen output */
#endif
	int something_worth_saving;	/* in case of panic */
	int panicking;		/* `panic' is in progress */
#if defined(VMS) || defined(WIN32)
	int exiting;		/* an exit handler is executing */
#endif
	int in_impossible;
#ifdef PANICLOG
	int in_paniclog;
#endif
} program_state;

E boolean restoring;
E boolean loading_mons;

E const char quitchars[];
E const char vowels[];
E const char ynchars[];
E const char ynqchars[];
E const char ynaqchars[];
E const char ynNaqchars[];
E NEARDATA long yn_number;

E const char disclosure_options[];

E NEARDATA int smeq[];
E NEARDATA int doorindex;
E NEARDATA int altarindex;
E NEARDATA char *save_cm;
#define KILLED_BY_AN	 0
#define KILLED_BY	 1
#define NO_KILLER_PREFIX 2
E NEARDATA int killer_format;
E const char *killer;
E const char *title_override;
E const char *delayed_killer;
#ifdef GOLDOBJ
E long done_money;
#endif
E char killer_buf[BUFSZ];

E long killer_flags;

#ifdef DUMP_LOG
E char dump_fn[];		/* dumpfile name (dump patch) */
#endif
E const char *configfile;
E NEARDATA char plname[PL_NSIZ];
E NEARDATA char inherited[];
E NEARDATA char dogname[];
E NEARDATA char catname[];
E NEARDATA char horsename[];
E NEARDATA char lizardname[];
E NEARDATA char spidername[];
E NEARDATA char dragonname[];
E NEARDATA char parrotname[];
E NEARDATA char monkeyname[];
E NEARDATA char whisperername[];
#ifdef CONVICT
E NEARDATA char ratname[];
#endif /* CONVICT */
E char preferred_pet;
E const char *occtxt;			/* defined when occupation != NULL */
E const char *nomovemsg;
E const char nul[];
E char lock[];

#ifdef QWERTZ
E const char qykbd_dir[], qzkbd_dir[], ndir[];
E char const *sdir;
#else
E const char sdir[], ndir[];
#endif
E const schar xdir[], ydir[], zdir[];
E char misc_cmds[];

#define DORUSH			misc_cmds[0]
#define DORUN			misc_cmds[1]
#define DOFORCEFIGHT		misc_cmds[2]
#define DONOPICKUP		misc_cmds[3]
#define DORUN_NOPICKUP		misc_cmds[4]
#define DOESCAPE		misc_cmds[5]
#ifdef REDO			/* JDS: moved from config.h */
# undef  DOAGAIN /* remove previous definition from config.h */
# define DOAGAIN		misc_cmds[6]
#endif

/* the number of miscellaneous commands */
#ifdef REDO
# define MISC_CMD_COUNT		7
#else
# define MISC_CMD_COUNT		6
#endif

E NEARDATA schar tbx, tby;		/* set in mthrowu.c */

E NEARDATA struct multishot { int n, i; short o; boolean s; struct obj * x;} m_shot;

E NEARDATA struct dig_info {		/* apply.c, hack.c */
	int	effort;
	d_level level;
	coord	pos;
	long lastdigtime;
	boolean down, chew, warned, quiet;
} digging;

E NEARDATA long moves, monstermoves;
E NEARDATA long nonce;
E NEARDATA long wailmsg;
E boolean goat_seenonce;

E NEARDATA boolean in_mklev;
E NEARDATA boolean stoned;
E NEARDATA boolean golded;
E NEARDATA boolean glassed;
E NEARDATA boolean unweapon;
E NEARDATA boolean mrg_to_wielded;
E NEARDATA boolean mon_ranged_gazeonly;
E NEARDATA struct obj *current_wand;

E NEARDATA boolean in_steed_dismounting;

E const int shield_static[];

#include "spell.h"
E NEARDATA struct spell spl_book[];	/* sized in decl.c */

#include "color.h"
#ifdef TEXTCOLOR
E const int zapcolors[];
#endif

E const char def_oc_syms[MAXOCLASSES];	/* default class symbols */
E uchar oc_syms[MAXOCLASSES];		/* current class symbols */
E const char def_monsyms[MAXMCLASSES];	/* default class symbols */
E uchar monsyms[MAXMCLASSES];		/* current class symbols */

#include "obj.h"
E NEARDATA struct obj *magic_chest_objs[10];
E NEARDATA struct obj *invent,
	*uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
#ifdef TOURIST
	*uarmu,				/* under-wear, so to speak */
#endif
	*uskin, *uamul, *ubelt, *uleft, *uright, *ublindf,
	*uwep, *uswapwep, *uquiver;

/* Needs to update, so it's redefined each time whenever it's used */
#define ARMOR_SLOTS { uarm, uarmc, uarmf, uarmh, uarmg, uarms, uarmu, ubelt }
#define WORN_SLOTS { uarm, uarmc, uarmf, uarmh, uarmg, uarms, uarmu, uamul, ubelt, uleft, uright, ublindf, uwep, uswapwep, uquiver }

E NEARDATA struct obj *urope;		/* defined only when entangled */
E NEARDATA struct obj *uchain;		/* defined only when punished */
E NEARDATA struct obj *uball;
E NEARDATA struct obj *migrating_objs;
E NEARDATA struct obj *billobjs;
E NEARDATA struct obj zeroobj;		/* init'd and defined in decl.c */
E NEARDATA anything zeroany;   /* init'd and defined in decl.c */

#include "mutations.h"
#include "you.h"
E NEARDATA struct you u;

#include "gods.h"
E NEARDATA struct god * godlist;

#include "onames.h"
#include "gnames.h"
#ifndef PM_H		/* (pm.h has already been included via youprop.h) */
#include "pm.h"
#endif

E NEARDATA struct monst youmonst;	/* init'd and defined in decl.c */
E NEARDATA struct monst *mydogs, *migrating_mons;

E NEARDATA struct permonst upermonst;	/* init'd in decl.c, 
					 * defined in polyself.c 
					 */

E NEARDATA struct mvitals {
	uchar	born; /*How many of this monster have been created in a way that respects extinction*/
	uchar	died; /*How many of this monster have died of any cause*/
	uchar	killed; /*How many of this monster have died at the PC's hands*/
	uchar	dissected; /*How many of this monster has the PC dissected*/
	uchar	reanimated; /*How many of this monster has the PC reanimated*/
	long long mvflags;
	int	san_lost;
	int	insight_gained;
	Bitfield(seen,1);
	Bitfield(vis_insight,1);
	Bitfield(insightkill,1);
} mvitals[NUMMONS];

E NEARDATA struct c_color_names {
    const char	*const c_black, *const c_amber, *const c_golden,
		*const c_light_blue,*const c_red, *const c_green,
		*const c_silver, *const c_blue, *const c_purple,
		*const c_white, *const c_yellow;
} c_color_names;
#define NH_BLACK		c_color_names.c_black
#define NH_AMBER		c_color_names.c_amber
#define NH_GOLDEN		c_color_names.c_golden
#define NH_LIGHT_BLUE		c_color_names.c_light_blue
#define NH_RED			c_color_names.c_red
#define NH_GREEN		c_color_names.c_green
#define NH_SILVER		c_color_names.c_silver
#define NH_BLUE			c_color_names.c_blue
#define NH_PURPLE		c_color_names.c_purple
#define NH_WHITE		c_color_names.c_white
#define NH_YELLOW		c_color_names.c_yellow

/* The names of the colors used for gems, etc. */
E const char *c_obj_colors[];

E struct c_common_strings {
    const char	*const c_nothing_happens, *const c_thats_enough_tries,
		*const c_silly_thing_to, *const c_shudder_for_moment,
		*const c_something, *const c_Something,
		*const c_You_can_move_again,
		*const c_Never_mind, *c_vision_clears,
		*const c_the_your[2];
} c_common_strings;
#define nothing_happens    c_common_strings.c_nothing_happens
#define thats_enough_tries c_common_strings.c_thats_enough_tries
#define silly_thing_to	   c_common_strings.c_silly_thing_to
#define shudder_for_moment c_common_strings.c_shudder_for_moment
#define something	   c_common_strings.c_something
#define Something	   c_common_strings.c_Something
#define You_can_move_again c_common_strings.c_You_can_move_again
#define Never_mind	   c_common_strings.c_Never_mind
#define vision_clears	   c_common_strings.c_vision_clears
#define the_your	   c_common_strings.c_the_your

/* material strings */
E const struct material materials[];

/* Monster name articles */
#define ARTICLE_NONE	0
#define ARTICLE_THE	1
#define ARTICLE_A	2
#define ARTICLE_YOUR	3

/* Monster name suppress masks */
#define SUPPRESS_IT		0x01
#define SUPPRESS_INVISIBLE	0x02
#define SUPPRESS_HALLUCINATION  0x04
#define SUPPRESS_SADDLE		0x08
#define EXACT_NAME		0x0F

/* Vision */
E NEARDATA boolean vision_full_recalc;	/* TRUE if need vision recalc */
E NEARDATA char **viz_array;		/* could see/in sight row pointers */

/* Window system stuff */
E NEARDATA winid WIN_MESSAGE, WIN_STATUS;
E NEARDATA winid WIN_MAP, WIN_INVEN;

/* pline (et al) for a single string argument (suppress compiler warning) */
#define pline1(cstr) pline("%s", cstr)
#define Your1(cstr) Your("%s", cstr)
#define You1(cstr) You("%s", cstr)
#define verbalize1(cstr) verbalize("%s", cstr)
#define You_hear1(cstr) You_hear("%s", cstr)
#define Sprintf1(buf, cstr) Sprintf(buf, "%s", cstr)
#define panic1(cstr) panic("%s", cstr)

E char toplines[];
#ifndef TCAP_H
E struct tc_gbl_data {	/* also declared in tcap.h */
    char *tc_AS, *tc_AE;	/* graphics start and end (tty font swapping) */
    int   tc_LI,  tc_CO;	/* lines and columns */
} tc_gbl_data;
#define AS tc_gbl_data.tc_AS
#define AE tc_gbl_data.tc_AE
#define LI tc_gbl_data.tc_LI
#define CO tc_gbl_data.tc_CO
#endif

/* xxxexplain[] is in drawing.c */
E const char * const monexplain[], invisexplain[], * const objexplain[], * const oclass_names[];

/* Some systems want to use full pathnames for some subsets of file names,
 * rather than assuming that they're all in the current directory.  This
 * provides all the subclasses that seem reasonable, and sets up for all
 * prefixes being null.  Port code can set those that it wants.
 */
#define HACKPREFIX	0
#define LEVELPREFIX	1
#define SAVEPREFIX	2
#define BONESPREFIX	3
#define DATAPREFIX	4	/* this one must match hardcoded value in dlb.c */
#define SCOREPREFIX	5
#define LOCKPREFIX	6
#define CONFIGPREFIX	7
#define TROUBLEPREFIX	8
#define PREFIX_COUNT	9
/* used in files.c; xxconf.h can override if needed */
# ifndef FQN_MAX_FILENAME
#define FQN_MAX_FILENAME 512
# endif

#if defined(NOCWD_ASSUMPTIONS) || defined(VAR_PLAYGROUND)
/* the bare-bones stuff is unconditional above to simplify coding; for
 * ports that actually use prefixes, add some more localized things
 */
#define PREFIXES_IN_USE
#endif

E char *fqn_prefix[PREFIX_COUNT];
#ifdef PREFIXES_IN_USE
E char *fqn_prefix_names[PREFIX_COUNT];
#endif

#ifdef AUTOPICKUP_EXCEPTIONS
struct autopickup_exception {
	char *pattern;
	regex_t match;
	boolean grab;
	boolean is_regexp;
	struct autopickup_exception *next;
};
#endif /* AUTOPICKUP_EXCEPTIONS */

#ifdef RECORD_ACHIEVE
struct u_achieve {
        Bitfield(get_keys,9);        /* the alignment keys */
        Bitfield(get_bell,1);        /* You have obtained the bell of 
                                      * opening */
        Bitfield(get_skey,1);        /* You have obtained the silver key */
        Bitfield(get_candelabrum,1); /* You have obtained the candelabrum */
        Bitfield(get_book,1);        /* You have obtained the book of 
                                      * the dead */
        Bitfield(enter_gehennom,1);  /* Entered Gehennom (including the 
                                      * Valley) by any means */
        Bitfield(perform_invocation,1); /* You have performed the invocation
                                         * ritual */
        Bitfield(get_amulet,1);      /* You have obtained the amulet
                                      * of Yendor */
        Bitfield(ascended,1);        /* You ascended to demigod[dess]hood.
                                      * Not quite the same as 
                                      * u.uevent.ascended. */
        Bitfield(get_luckstone,1);   /* You obtained the luckstone at the
                                      * end of the mines. */
        Bitfield(finish_sokoban,1);  /* You obtained the sokoban prize. */
        Bitfield(killed_challenge,1);   /* You defeated the challenge boss. */
		Bitfield(killed_lucifer,1);		/* Bragging rights */
		Bitfield(killed_asmodeus,1);		/* Bragging rights */
		Bitfield(killed_demogorgon,1);		/* Bragging rights */
		unsigned long long	trophies;	/* Flags for Junethack trophies */
		unsigned long iea_flags;	/* IEA flags for Junethack trophy */
	Bitfield(get_kroo,1);        /* ring of kroo get*/
	Bitfield(get_raggo,1);        /* raggo's rock get*/
        Bitfield(get_gilly,1);        /* gillystone get*/
        Bitfield(get_abominable,1);        /* abominable cloak get*/
        Bitfield(get_poplar,1);        /* poplar punisher get*/
        Bitfield(did_unknown,1);        /* ?????????????????*/
        Bitfield(killed_illurien,1);        /* illurien down */
        Bitfield(get_ckey,1);        /* Cage key get */
        Bitfield(max_punch,1);        /* Mystic punched to the max */
        Bitfield(used_smith,1);        /* Used a blacksmith's service */
        Bitfield(garnet_spear,1);        /* Used a garnet tip spear*/
        Bitfield(inked_up,1);        /* Fell tat */
        Bitfield(new_races,1);        /* Ascended a new race */
#define	ARC_QUEST		0x1LL << 0
#define	CAV_QUEST		0x1LL << 1
#define	CON_QUEST		0x1LL << 2
#define	KNI_QUEST		0x1LL << 3
#define	ANA_QUEST		0x1LL << 4
#define	AND_QUEST		0x1LL << 5
#define	ANA_ASC			0x1LL << 6
#define	BIN_QUEST		0x1LL << 7
#define	BIN_ASC			0x1LL << 8
#define	PIR_QUEST		0x1LL << 9
#define	BRD_QUEST		0x1LL << 10
#define	NOB_QUEST		0x1LL << 11
#define	HDR_NOB_QUEST	0x1LL << 12
#define	HDR_SHR_QUEST	0x1LL << 13
#define	DRO_NOB_QUEST	0x1LL << 14
#define	DRO_SHR_QUEST	0x1LL << 15
#define	DWA_NOB_QUEST	0x1LL << 16
#define	DWA_KNI_QUEST	0x1LL << 17
#define	GNO_RAN_QUEST	0x1LL << 18
#define	ELF_SHR_QUEST	0x1LL << 19
#define	CLOCK_ASC		0x1LL << 20
#define	CHIRO_ASC		0x1LL << 21
#define	YUKI_ASC		0x1LL << 22
#define	HALF_ASC		0x1LL << 23
#define	LAW_QUEST		0x1LL << 24
#define	NEU_QUEST		0x1LL << 25
#define	CHA_QUEST		0x1LL << 26
#define	MITH_QUEST		0x1LL << 27
#define	MORD_QUEST		0x1LL << 28
#define	SECOND_THOUGHTS	0x1LL << 29
#define	ILLUMIAN		0x1LL << 30
#define	RESCUE			0x1LL << 31
#define	FULL_LOADOUT	0x1LL << 32
#define	NIGHTMAREHUNTER	0x1LL << 33
#define	SPEED_PHASE		0x1LL << 34
#define	QUITE_MAD		0x1LL << 35
#define	TOTAL_DRUNK		0x1LL << 36
#define	MAD_QUEST		0x1LL << 37
#define	LAMASHTU_KILL	0x1LL << 38
#define	BAALPHEGOR_KILL	0x1LL << 39
#define	ANGEL_VAULT		0x1LL << 40
#define	ANCIENT_VAULT	0x1LL << 41
#define	TANNINIM_VAULT	0x1LL << 42
#define	CASTLE_WISH		0x1LL << 43
#define	UNKNOWN_WISH	0x1LL << 44
#define	FEM_DRA_NOB_QUEST	0x1LL << 45
#define	DEVIL_VAULT	0x1LL << 46
#define	DEMON_VAULT	0x1LL << 47
#define	BOKRUG_QUEST	0x1LL << 48
#define	HEA_QUEST		0x1LL << 49
#define	DRO_HEA_QUEST	0x1LL << 50
#define	MONK_QUEST		0x1LL << 51
#define	IEA_UPGRADES	0x1LL << 52
#define	UH_QUEST		0x1LL << 53
#define	UH_ASC			0x1LL << 54
#define ACHIEVE_NUMBER	66
};

E struct u_achieve achieve;
#endif

#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)
E struct realtime_data {
  time_t realtime;    /* Amount of actual playing time up until the last time
                       * the game was restored. */
  time_t restoretime; /* The time that the game was started or restored. */
  time_t last_displayed_time; /* Last time displayed on the status line */
} realtime_data;
#endif /* RECORD_REALTIME || REALTIME_ON_BOTL */

#ifdef SIMPLE_MAIL
E int mailckfreq;
#endif

struct _plinemsg {
    xchar msgtype;
    char *pattern;
    regex_t match;
    boolean is_regexp;
    struct _plinemsg *next;
};

E struct _plinemsg *pline_msg;

#define MSGTYP_NORMAL  0
#define MSGTYP_NOREP   1
#define MSGTYP_NOSHOW  2
#define MSGTYP_STOP    3

struct querytype {
    xchar querytype;
    char *pattern;
    regex_t match;
    boolean is_regexp;
    struct querytype *next;
};

extern struct querytype *query_types;

#define QUERYTYP_NORMAL 0
#define QUERYTYP_YN     1
#define QUERYTYP_YESNO  2

#define ROLL_FROM(array)	array[rn2(SIZE(array))]


/* FIXME: These should be integrated into objclass and permonst structs,
   but that invalidates saves */
E glyph_t objclass_unicode_codepoint[NUM_OBJECTS];
E glyph_t permonst_unicode_codepoint[NUMMONS];


E boolean curses_stupid_hack;

#define LIGHTSABER_MAX_CHARGE 150000

#undef E

#endif /* DECL_H */
