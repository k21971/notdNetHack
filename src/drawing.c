/*	SCCS Id: @(#)drawing.c	3.4	1999/12/02	*/
/* Copyright (c) NetHack Development Team 1992.			  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h"

/* Relevent header information in rm.h and objclass.h. */

#ifdef C
#undef C
#endif

#ifdef TEXTCOLOR
#define C(n) n
#else
#define C(n)
#endif

#define g_FILLER(symbol) 0

uchar oc_syms[MAXOCLASSES] = DUMMY; /* the current object  display symbols */
glyph_t showsyms[MAXPCHARS]  = DUMMY; /* the current feature display symbols */
uchar monsyms[MAXMCLASSES] = DUMMY; /* the current monster display symbols */
uchar warnsyms[WARNCOUNT]  = DUMMY;  /* the current warning display symbols */
#ifdef USER_DUNGEONCOLOR
uchar showsymcolors[MAXPCHARS] = DUMMY; /* current feature display colors */
#endif


/* Default object class symbols.  See objclass.h. */
const char def_oc_syms[MAXOCLASSES] = {
/* 0*/	'\0',		/* placeholder for the "random class" */
	ILLOBJ_SYM,
	WEAPON_SYM,
	ARMOR_SYM,
	RING_SYM,
/* 5*/	AMULET_SYM,
	TOOL_SYM,
	FOOD_SYM,
	POTION_SYM,
	SCROLL_SYM,
/*10*/	SPBOOK_SYM,
	WAND_SYM,
	GOLD_SYM,
	GEM_SYM,
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM,
	TILE_SYM,
	BED_SYM,
	SCOIN_SYM,
	BELT_SYM,
};

const char invisexplain[] = "remembered, unseen, creature";

/* Object descriptions.  Used in do_look(). */
const char * const objexplain[] = {	/* these match def_oc_syms, above */
/* 0*/	0,
	"strange object",
	"weapon",
	"suit or piece of armor",
	"ring",
/* 5*/	"amulet",
	"useful item (pick-axe, key, lamp...)",
	"piece of food",
	"potion",
	"scroll",
/*10*/	"spellbook",
	"wand",
	"pile of coins",
	"gem or rock",
	"boulder or statue",
/*15*/	"iron ball",
	"iron chain",
	"splash of venom",
	"broken tile or slab",
	"bed or chair",
	"strange coin",
	"belt"
};

/* Object class names.  Used in object_detect(). */
const char * const oclass_names[] = {
/* 0*/	0,
	"illegal objects",
	"weapons",
	"armor",
	"rings",
/* 5*/	"amulets",
	"tools",
	"food",
	"potions",
	"scrolls",
/*10*/	"spellbooks",
	"wands",
	"coins",
	"rocks",
	"large stones",
/*15*/	"iron balls",
	"chains",
	"venoms",
	"tiles",
	"furnature",
	"strange coins"
};

/* Default monster class symbols.  See monsym.h. */
const char def_monsyms[MAXMCLASSES] = {
	'\0',		/* holder */
	DEF_ANT,
	DEF_BLOB,
	DEF_COCKATRICE,
	DEF_DOG,
	DEF_EYE,
	DEF_FELINE,
	DEF_GREMLIN,
	DEF_HUMANOID,
	DEF_IMP,
	DEF_JELLY,		/* 10 */
	DEF_KOBOLD,
	DEF_LEPRECHAUN,
	DEF_MIMIC,
	DEF_NYMPH,
	DEF_ORC,
	DEF_PIERCER,
	DEF_QUADRUPED,
	DEF_RODENT,
	DEF_SPIDER,
	DEF_TRAPPER,		/* 20 */
	DEF_UNICORN,
	DEF_VORTEX,
	DEF_WORM,
	DEF_XAN,
	DEF_LIGHT,
	DEF_ZRUTY,
	DEF_LAW_ANGEL,
	DEF_NEU_ANGEL,
	DEF_CHA_ANGEL,
	DEF_BAT,
	DEF_CENTAUR,
	DEF_DRAGON,		/* 30 */
	DEF_ELEMENTAL,
	DEF_FUNGUS,
	DEF_GNOME,
	DEF_GIANT,
	'\0',
	DEF_JABBERWOCK,
	DEF_KETER,
	DEF_LICH,
	DEF_MUMMY,
	DEF_NAGA,		/* 40 */
	DEF_OGRE,
	DEF_PUDDING,
	DEF_QUANTMECH,
	DEF_RUSTMONST,
	DEF_SNAKE,
	DEF_TROLL,
	DEF_UMBER,
	DEF_VAMPIRE,
	DEF_WRAITH,
	DEF_XORN,		/* 50 */
	DEF_YETI,
	DEF_ZOMBIE,
	DEF_HUMAN,
	DEF_GHOST,
	DEF_SHADE,
	DEF_GOLEM,
	DEF_DEMON,
	DEF_EEL,
	DEF_LIZARD,
	DEF_PLANT,
	DEF_NEU_OUTSIDER,
	DEF_WORM_TAIL,		/* 60 */
	DEF_MIMIC_DEF,		/* 61 */
};

/* The explanations below are also used when the user gives a string
 * for blessed genocide, so no text should wholly contain any later
 * text.  They should also always contain obvious names (eg. cat/feline).
 */
const char * const monexplain[MAXMCLASSES] = {
    0,
    "ant or other insect",	"blob",			"cockatrice",
    "dog or other canine",	"eye, sphere, or auton",	"cat or other feline",
    "gremlinoid",			"humanoid",		"imp or minor demon",
    "jelly",			"kobold",		"leprechaun",
    "mimic",			"nymph",		"orc",
    "piercer",			"quadruped",		"rodent",
    "arachnid or centipede",	"trapper, lurker, or metroid", "unicorn or horse",
    "vortex",		"worm", "xan or other mythical/fantastic insect",
    "light",			"zruty or ammit",

    "lawful angelic being",	"neutral angelic being",	"chaotic angelic being",
	"bat or bird",		"centaur",
    "dragon",			"elemental",		"fungus or mold",
    "gnome or gizmo",	"giant humanoid",	0,
    "jabberwockian",	"Keter Sephiroth",		"lich",
    "mummy",			"naga",			"ogre",
    "pudding or ooze",		"quantum mechanic",	"rust monster or disenchanter",
    "snake",			"troll",		"unknown abomination",
    "vampire",			"wraith",		"radially symmetric organism",
    "apelike creature",		"zombie",

    "human or elf",		"ghost",		"shade",	"golem",
    "major demon",		"sea monster",	"lizard",	"plant",
    "neutral spiritual being",	"long worm tail",		"mimic"
};

const struct symdef def_warnsyms[WARNCOUNT] = {
	{'0', "unknown creature causing you worry", C(CLR_WHITE)},  	/* white warning  */
	{'1', "unknown creature causing you concern", C(CLR_RED)},	/* pink warning   */
	{'2', "unknown creature causing you anxiety", C(CLR_RED)},	/* red warning    */
	{'3', "unknown creature causing you disquiet", C(CLR_RED)},	/* ruby warning   */
	{'4', "unknown creature causing you alarm",
						C(CLR_MAGENTA)},        /* purple warning */
	{'5', "unknown creature causing you dread",
						C(CLR_BRIGHT_MAGENTA)}	/* black warning  */
};

/*
 *  Default screen symbols with explanations and colors.
 *  Note:  {ibm|dec|mac}_graphics[] arrays also depend on this symbol order.
 */
const struct symdef defsyms[MAXPCHARS] = {
/* 0*/	{' ', "unknown area",C(NO_COLOR)},	/* stone */
	{'|', "wall",		C(CLR_GRAY)},	/* vwall */
	{'-', "wall",		C(CLR_GRAY)},	/* hwall */
	{'-', "wall",		C(CLR_GRAY)},	/* tlcorn */
	{'-', "wall",		C(CLR_GRAY)},	/* trcorn */
	{'-', "wall",		C(CLR_GRAY)},	/* blcorn */
	{'-', "wall",		C(CLR_GRAY)},	/* brcorn */
	{'-', "wall",		C(CLR_GRAY)},	/* crwall */
	{'-', "wall",		C(CLR_GRAY)},	/* tuwall */
	{'-', "wall",		C(CLR_GRAY)},	/* tdwall */
/*10*/	{'|', "wall",		C(CLR_GRAY)},	/* tlwall */
	{'|', "wall",		C(CLR_GRAY)},	/* trwall */
	{'.', "doorway",	C(CLR_GRAY)},	/* ndoor */
	{'-', "open door",	C(CLR_BROWN)},	/* vodoor */
	{'|', "open door",	C(CLR_BROWN)},	/* hodoor */
	{'+', "closed door",	C(CLR_BROWN)},	/* vcdoor */
	{'+', "closed door",	C(CLR_BROWN)},	/* hcdoor */
	{'#', "iron bars",	C(HI_METAL)},	/* bars */
	{'#', "tree",		C(CLR_GREEN)},	/* tree */
	{'#', "dead tree",	C(CLR_BROWN)},	/* dead tree */
	{'.', "floor of a dark room", C(CLR_BLACK)},	/* drkroom */
	{'.', "floor of a room",C(CLR_GRAY)},	/* litroom */
	{'#', "bright room",C(CLR_WHITE)},	/* brightrm */
/*20*/	{'#', "corridor",	C(CLR_BLACK)},	/* dark corr */
	{'#', "lit corridor",	C(CLR_GRAY)},	/* lit corr (see mapglyph.c) */
	{'<', "staircase up",	C(CLR_GRAY)},	/* upstair */
	{'>', "staircase down",	C(CLR_GRAY)},	/* dnstair */
	{'<', "ladder up",	C(CLR_BROWN)},	/* upladder */
	{'>', "ladder down",	C(CLR_BROWN)},	/* dnladder */
	{'<', "branch staircase up",	C(CLR_YELLOW)},	/* brupstair */
	{'>', "branch staircase down",	C(CLR_YELLOW)},	/* brdnstair */
	{'<', "branch ladder up",	C(CLR_YELLOW)},	/* brupladder */
	{'>', "branch ladder down",	C(CLR_YELLOW)},	/* brdnladder */
	{'_', "altar",		C(CLR_GRAY)},	/* altar */
	{'|', "grave",      C(CLR_GRAY)},   /* grave */
	{'+', "hellish seal",      C(CLR_BRIGHT_MAGENTA)},   /* seal */
	{'\\', "opulent throne",C(HI_GOLD)},	/* throne */
#ifdef SINKS
	{'#', "sink",		C(CLR_WHITE)},	/* sink */
#else
	{'#', "",		C(CLR_WHITE)},	/* sink */
#endif
/*30*/	{'{', "fountain",	C(CLR_BLUE)},	/* fountain */
	{'{', "forge",		C(CLR_RED)},	/* forge */
	{'}', "water",		C(CLR_BLUE)},	/* pool */
	{'.', "ice",		C(CLR_CYAN)},	/* ice */
	{',', "grass",		C(CLR_BRIGHT_GREEN)},	/* lit grass */
	{',', "grass",		C(CLR_GREEN)},	/* unlit grass */
	{'.', "soil",		C(CLR_BROWN)},	/* lit soil */
	{'.', "soil",		C(CLR_BLACK)},	/* unlit soil */
	{'~', "sand",		C(CLR_YELLOW)},	/* lit sand */
	{'~', "sand",		C(CLR_BROWN)},	/* unlit sand */
	{'}', "molten lava",	C(CLR_RED)},	/* lava */
	{'.', "lowered drawbridge",C(CLR_BROWN)},	/* vodbridge */
	{'.', "lowered drawbridge",C(CLR_BROWN)},	/* hodbridge */
	{'#', "raised drawbridge",C(CLR_BROWN)},/* vcdbridge */
	{'#', "raised drawbridge",C(CLR_BROWN)},/* hcdbridge */
	{'#', "air",		C(CLR_BLUE)},	/* open air */
	{'#', "cloud",		C(CLR_GRAY)},	/* [part of] a cloud */
	{'#', "fog cloud",	C(HI_ZAP)},	/* [part of] a cloud */
	{'#', "dust cloud",	C(CLR_WHITE)},	/* [part of] a cloud */
/*40*/	{'~', "shallow water",	C(CLR_BLUE)},	/* shallow water */
	{'}', "water",		C(CLR_BLUE)},	/* under water */
	{'^', "arrow trap",	C(HI_METAL)},	/* trap */
	{'^', "dart trap",	C(HI_METAL)},	/* trap */
	{'^', "falling rock trap",C(CLR_GRAY)},	/* trap */
	{'^', "squeaky board",	C(CLR_BROWN)},	/* trap */
	{'^', "bear trap",	C(HI_METAL)},	/* trap */
	{'^', "land mine",	C(CLR_RED)},	/* trap */
	{'^', "rolling boulder trap",	C(CLR_GRAY)},	/* trap */
	{'^', "sleeping gas trap",C(HI_ZAP)},	/* trap */
	{'^', "rust trap",	C(CLR_BLUE)},	/* trap */
/*50*/	{'^', "fire trap",	C(CLR_ORANGE)},	/* trap */
	{'^', "pit",		C(CLR_BLACK)},	/* trap */
	{'^', "spiked pit",	C(CLR_BLACK)},	/* trap */
	{'^', "hole",	C(CLR_BROWN)},	/* trap */
	{'^', "trap door",	C(CLR_BROWN)},	/* trap */
	{'^', "teleportation trap", C(CLR_MAGENTA)},	/* trap */
	{'^', "level teleporter", C(CLR_MAGENTA)},	/* trap */
	{'^', "magic portal",	C(CLR_BRIGHT_MAGENTA)},	/* trap */
	{'"', "web",		C(CLR_GRAY)},	/* web */
	{'^', "statue trap",	C(CLR_GRAY)},	/* trap */
/*60*/	{'^', "magic trap",	C(HI_ZAP)},	/* trap */
	{'^', "anti-magic field", C(HI_ZAP)},	/* trap */
	{'^', "polymorph trap",	C(CLR_BRIGHT_GREEN)},	/* trap */
	{'^', "essence trap",	C(CLR_GREEN)},	/* "trap" */
	{'^', "mummy trap",	C(CLR_YELLOW)},	/* trap */
	{'^', "switch",	C(CLR_MAGENTA)},	/* "trap" */
	{'^', "flesh hook",	C(CLR_GREEN)},	/* trap */
	{'|', "wall",		C(CLR_GRAY)},	/* vbeam */
	{'-', "wall",		C(CLR_GRAY)},	/* hbeam */
	{'\\',"wall",		C(CLR_GRAY)},	/* lslant */
	{'/', "wall",		C(CLR_GRAY)},	/* rslant */
	{'*', "",		C(CLR_WHITE)},	/* dig beam */
	{'!', "",		C(CLR_WHITE)},	/* camera flash beam */
	{')', "",		C(HI_WOOD)},	/* boomerang open left */
/*70*/	{'(', "",		C(HI_WOOD)},	/* boomerang open right */
	{'0', "",		C(HI_ZAP)},	/* 4 magic shield symbols */
	{'#', "",		C(HI_ZAP)},
	{'@', "",		C(HI_ZAP)},
	{'*', "",		C(HI_ZAP)},
	{'/', "",		C(CLR_GREEN)},	/* swallow top left	*/
	{'-', "",		C(CLR_GREEN)},	/* swallow top center	*/
	{'\\', "",		C(CLR_GREEN)},	/* swallow top right	*/
	{'|', "",		C(CLR_GREEN)},	/* swallow middle left	*/
	{'|', "",		C(CLR_GREEN)},	/* swallow middle right	*/
/*80*/	{'\\', "",		C(CLR_GREEN)},	/* swallow bottom left	*/
	{'-', "",		C(CLR_GREEN)},	/* swallow bottom center*/
	{'/', "",		C(CLR_GREEN)},	/* swallow bottom right	*/
	{'/', "",		C(CLR_ORANGE)},	/* explosion top left     */
	{'-', "",		C(CLR_ORANGE)},	/* explosion top center   */
	{'\\', "",		C(CLR_ORANGE)},	/* explosion top right    */
	{'|', "",		C(CLR_ORANGE)},	/* explosion middle left  */
	{' ', "",		C(CLR_ORANGE)},	/* explosion middle center*/
	{'|', "",		C(CLR_ORANGE)},	/* explosion middle right */
	{'\\', "",		C(CLR_ORANGE)},	/* explosion bottom left  */
/*90*/	{'-', "",		C(CLR_ORANGE)},	/* explosion bottom center*/
	{'/', "",		C(CLR_ORANGE)},	/* explosion bottom right */
/*
 *  Note: Additions to this array should be reflected in the
 *	  symbol_names[] and {ibm,dec,mac}_graphics[] arrays below.
 */
};

const char * const symbol_names[MAXPCHARS] = {
	"S_stone",
	"S_vwall",
	"S_hwall",
	"S_tlcorn",
	"S_trcorn",
	"S_blcorn",
	"S_brcorn",
	"S_crwall",
	"S_tuwall",
	"S_tdwall",
	"S_tlwall",
	"S_trwall",
	"S_ndoor",
	"S_vodoor",
	"S_hodoor",
	"S_vcdoor",
	"S_hcdoor",
	"S_bars",
	"S_tree",
	"S_deadtree",
	"S_drkroom",
	"S_litroom",
	"S_brightrm",
	"S_corr",
	"S_litcorr",
	"S_upstair",
	"S_dnstair",
	"S_upladder",
	"S_dnladder",
	"S_brupstair",
	"S_brdnstair",
	"S_brupladder",
	"S_brdnladder",
	"S_altar",
	"S_grave",
	"S_seal",
	"S_throne",
	"S_sink",
	"S_fountain",
	"S_forge",
	"S_pool",
	"S_ice",
	"S_litgrass",
	"S_drkgrass",
	"S_litsoil",
	"S_drksoil",
	"S_litsand",
	"S_drksand",
	"S_lava",
	"S_vodbridge",
	"S_hodbridge",
	"S_vcdbridge",
	"S_hcdbridge",
	"S_air",
	"S_cloud",
	"S_fog",
	"S_dust",
	"S_puddle",
	"S_water",
	"S_arrow_trap",
	"S_dart_trap",
	"S_falling_rock_trap",
	"S_squeaky_board",
	"S_bear_trap",
	"S_land_mine",
	"S_rolling_boulder_trap",
	"S_sleeping_gas_trap",
	"S_rust_trap",
	"S_fire_trap",
	"S_pit",
	"S_spiked_pit",
	"S_hole",
	"S_trap_door",
	"S_teleportation_trap",
	"S_level_teleporter",
	"S_magic_portal",
	"S_web",
	"S_statue_trap",
	"S_magic_trap",
	"S_anti_magic_trap",
	"S_polymorph_trap",
	"S_essence_trap",
	"S_mummy_trap",
	"S_switch",
	"S_flesh_hook",
	"S_vbeam",
	"S_hbeam",
	"S_lslant",
	"S_rslant",
	"S_digbeam",
	"S_flashbeam",
	"S_boomleft",
	"S_boomright",
	"S_ss1",
	"S_ss2",
	"S_ss3",
	"S_ss4",
	"S_sw_tl",
	"S_sw_tc",
	"S_sw_tr",
	"S_sw_ml",
	"S_sw_mr",
	"S_sw_bl",
	"S_sw_bc",
	"S_sw_br",
	"S_explode1",
	"S_explode2",
	"S_explode3",
	"S_explode4",
	"S_explode5",
	"S_explode6",
	"S_explode7",
	"S_explode8",
	"S_explode9",
};

#undef C

#ifdef ASCIIGRAPH

#ifdef PC9800
void NDECL((*ibmgraphics_mode_callback)) = 0;	/* set in tty_start_screen() */
#endif /* PC9800 */

#ifdef CURSES_GRAPHICS
void NDECL((*cursesgraphics_mode_callback)) = 0;
#endif

static glyph_t ibm_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xb3,	/* S_vwall:	meta-3, vertical rule */
	0xc4,	/* S_hwall:	meta-D, horizontal rule */
	0xda,	/* S_tlcorn:	meta-Z, top left corner */
	0xbf,	/* S_trcorn:	meta-?, top right corner */
	0xc0,	/* S_blcorn:	meta-@, bottom left */
	0xd9,	/* S_brcorn:	meta-Y, bottom right */
	0xc5,	/* S_crwall:	meta-E, cross */
	0xc1,	/* S_tuwall:	meta-A, T up */
	0xc2,	/* S_tdwall:	meta-B, T down */
/*10*/	0xb4,	/* S_tlwall:	meta-4, T left */
	0xc3,	/* S_trwall:	meta-C, T right */
	0xfa,	/* S_ndoor:	meta-z, centered dot */
	0xfe,	/* S_vodoor:	meta-~, small centered square */
	0xfe,	/* S_hodoor:	meta-~, small centered square */
	g_FILLER(S_vcdoor),
	g_FILLER(S_hcdoor),
	240,	/* S_bars:	equivalence symbol */
	241,	/* S_tree:	plus or minus symbol */
	241,	/* S_deadtree:	plus or minus symbol */
	0xfa,	/* S_drkroom:	meta-z, centered dot */
	0xfa,	/* S_litroom:	centered circle */
	0xfe,	/* S_brightrm:	centered square */
/*20*/	0xb0,	/* S_corr:	meta-0, light shading */
	0xb1,	/* S_litcorr:	meta-1, medium shading */
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	g_FILLER(S_upladder),
	g_FILLER(S_dnladder),
	g_FILLER(S_brupstair),
	g_FILLER(S_brdnstair),
	g_FILLER(S_brupladder),
	g_FILLER(S_brdnladder),
	g_FILLER(S_altar),
	g_FILLER(S_grave),
	g_FILLER(S_seal),
	g_FILLER(S_throne),
	g_FILLER(S_sink),
/*30*/	0xf4,	/* S_fountain:	meta-t, integral top half */
	0xf4,	/* S_forge:	meta-t, integral top half */
	0xf7,	/* S_pool:	meta-w, approx. equals */
	0xfa,	/* S_ice:	meta-z, centered dot */
	0xfa,	/* S_litgrass:	meta-z, centered dot */
	0xfa,	/* S_drkgrass:	meta-z, centered dot */
	0xfa,	/* S_litsoil:	meta-z, centered dot */
	0xfa,	/* S_drksoil:	meta-z, centered dot */
	0xf7,	/* S_litsand:	meta-z, approx. equals */
	0xf7,	/* S_drksand:	meta-z, approx. equals */
	0xf7,	/* S_lava:	meta-w, approx. equals */
	0xfa,	/* S_vodbridge:	meta-z, centered dot */
	0xfa,	/* S_hodbridge:	meta-z, centered dot */
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
	g_FILLER(S_air),
	g_FILLER(S_cloud),
	g_FILLER(S_fog),
	g_FILLER(S_dust),
	g_FILLER(S_puddle),
	0xf7,	/* S_water:	meta-w, approx. equals */
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
	g_FILLER(S_sleeping_gas_trap),
/*50*/	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
	g_FILLER(S_web),
/*60*/	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	g_FILLER(S_essence_trap),
	g_FILLER(S_mummy_trap),
	g_FILLER(S_switch),
	g_FILLER(S_flesh_hook),
	0xb3,	/* S_vbeam:	meta-3, vertical rule */
	0xc4,	/* S_hbeam:	meta-D, horizontal rule */
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_digbeam),
	g_FILLER(S_flashbeam),
/*70*/	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	g_FILLER(S_sw_tc),
	g_FILLER(S_sw_tr),
	0xb3,	/* S_sw_ml:	meta-3, vertical rule */
/*80*/	0xb3,	/* S_sw_mr:	meta-3, vertical rule */
	g_FILLER(S_sw_bl),
	g_FILLER(S_sw_bc),
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	g_FILLER(S_explode2),
	g_FILLER(S_explode3),
	0xb3,	/* S_explode4:	meta-3, vertical rule */
	g_FILLER(S_explode5),
	0xb3,	/* S_explode6:	meta-3, vertical rule */
/*90*/	g_FILLER(S_explode7),
	g_FILLER(S_explode8),
	g_FILLER(S_explode9)
};
#endif  /* ASCIIGRAPH */

#ifdef TERMLIB
void NDECL((*decgraphics_mode_callback)) = 0;  /* set in tty_start_screen() */

static glyph_t dec_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xf8,	/* S_vwall:	meta-x, vertical rule */
	0xf1,	/* S_hwall:	meta-q, horizontal rule */
	0xec,	/* S_tlcorn:	meta-l, top left corner */
	0xeb,	/* S_trcorn:	meta-k, top right corner */
	0xed,	/* S_blcorn:	meta-m, bottom left */
	0xea,	/* S_brcorn:	meta-j, bottom right */
	0xee,	/* S_crwall:	meta-n, cross */
	0xf6,	/* S_tuwall:	meta-v, T up */
	0xf7,	/* S_tdwall:	meta-w, T down */
/*10*/	0xf5,	/* S_tlwall:	meta-u, T left */
	0xf4,	/* S_trwall:	meta-t, T right */
	0xfe,	/* S_ndoor:	meta-~, centered dot */
	0xe1,	/* S_vodoor:	meta-a, solid block */
	0xe1,	/* S_hodoor:	meta-a, solid block */
	g_FILLER(S_vcdoor),
	g_FILLER(S_hcdoor),
	0xfb,	/* S_bars:	meta-{, small pi */
	0xe7,	/* S_tree:	meta-g, plus-or-minus */
	0xe7,	/* S_deadtree:	meta-g, plus-or-minus */
	0xfe,	/* S_drkroom:	meta-~, centered dot */
	0xfe,	/* S_litroom:	meta-~, centered dot */
	g_FILLER(S_brightrm),
/*20*/	g_FILLER(S_corr),
	g_FILLER(S_litcorr),
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	0xf9,	/* S_upladder:	meta-y, greater-than-or-equals */
	0xfa,	/* S_dnladder:	meta-z, less-than-or-equals */
	g_FILLER(S_brupstair),
	g_FILLER(S_brdnstair),
	0xf9,	/* S_brupladder:	meta-y, greater-than-or-equals */
	0xfa,	/* S_brdnladder:	meta-z, less-than-or-equals */
	g_FILLER(S_altar),	/* 0xc3, \E)3: meta-C, dagger */
	g_FILLER(S_grave),
	g_FILLER(S_seal),
	g_FILLER(S_throne),
	g_FILLER(S_sink),
/*30*/	g_FILLER(S_fountain),	/* 0xdb, \E)3: meta-[, integral top half */
	g_FILLER(S_forge),	/* 0xdb, \E)3: meta-[, integral top half */
	0xe0,	/* S_pool:	meta-\, diamond */
	0xfe,	/* S_ice:	meta-~, centered dot */
	0xfe,	/* S_litgrass:	meta-~, centered dot */
	0xfe,	/* S_drkgrass:	meta-~, centered dot */
	0xfe,	/* S_litsoil:	meta-~, centered dot */
	0xfe,	/* S_drksoil:	meta-~, centered dot */
	g_FILLER(S_litsand),
	g_FILLER(S_drksand),
	0xe0,	/* S_lava:	meta-\, diamond */
	0xfe,	/* S_vodbridge:	meta-~, centered dot */
	0xfe,	/* S_hodbridge:	meta-~, centered dot */
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
	g_FILLER(S_air),
	g_FILLER(S_cloud),
	g_FILLER(S_fog),
	g_FILLER(S_dust),
	g_FILLER(S_puddle),
	0xe0,	/* S_water:	meta-\, diamond */
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
	g_FILLER(S_sleeping_gas_trap),
/*50*/	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
	g_FILLER(S_web),	/* 0xbd, \E)3: meta-=, int'l currency */
/*60*/	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	g_FILLER(S_essence_trap),
	g_FILLER(S_mummy_trap),
	g_FILLER(S_switch),
	g_FILLER(S_flesh_hook),
	0xf8,	/* S_vbeam:	meta-x, vertical rule */
	0xf1,	/* S_hbeam:	meta-q, horizontal rule */
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_digbeam),
	g_FILLER(S_flashbeam),
/*70*/	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	0xef,	/* S_sw_tc:	meta-o, high horizontal line */
	g_FILLER(S_sw_tr),
	0xf8,	/* S_sw_ml:	meta-x, vertical rule */
/*80*/	0xf8,	/* S_sw_mr:	meta-x, vertical rule */
	g_FILLER(S_sw_bl),
	0xf3,	/* S_sw_bc:	meta-s, low horizontal line */
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	0xef,	/* S_explode2:	meta-o, high horizontal line */
	g_FILLER(S_explode3),
	0xf8,	/* S_explode4:	meta-x, vertical rule */
	g_FILLER(S_explode5),
	0xf8,	/* S_explode6:	meta-x, vertical rule */
/*90*/	g_FILLER(S_explode7),
	0xf3,	/* S_explode8:	meta-s, low horizontal line */
	g_FILLER(S_explode9)
};
#endif  /* TERMLIB */

#ifdef MAC_GRAPHICS_ENV
static glyph_t mac_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xba,	/* S_vwall */
	0xcd,	/* S_hwall */
	0xc9,	/* S_tlcorn */
	0xbb,	/* S_trcorn */
	0xc8,	/* S_blcorn */
	0xbc,	/* S_brcorn */
	0xce,	/* S_crwall */
	0xca,	/* S_tuwall */
	0xcb,	/* S_tdwall */
/*10*/	0xb9,	/* S_tlwall */
	0xcc,	/* S_trwall */
	0xb0,	/* S_ndoor */
	0xee,	/* S_vodoor */
	0xee,	/* S_hodoor */
	0xef,	/* S_vcdoor */
	0xef,	/* S_hcdoor */
	0xf0,	/* S_bars:	equivalency symbol */
	0xf1,	/* S_tree:	plus-or-minus */
	0xf1,	/* S_deadtree:	plus-or-minus */
	g_FILLER(S_drkroom),
	g_FILLER(S_litroom),
	g_FILLER(S_brightrm),
/*20*/	0xB0,	/* S_corr */
	g_FILLER(S_litcorr),
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	g_FILLER(S_upladder),
	g_FILLER(S_dnladder),
	g_FILLER(S_brupstair),
	g_FILLER(S_brdnstair),
	g_FILLER(S_brupladder),
	g_FILLER(S_brdnladder),
	g_FILLER(S_altar),
	0xef,	/* S_grave:	same as open door */
	g_FILLER(S_seal),
	g_FILLER(S_throne),
	g_FILLER(S_sink),
/*30*/	g_FILLER(S_fountain),
	g_FILLER(S_forge),
	0xe0,	/* S_pool */
	g_FILLER(S_ice),
	g_FILLER(S_litgrass),
	g_FILLER(S_drkgrass),
	g_FILLER(S_litsoil),
	g_FILLER(S_drksoil),
	g_FILLER(S_litsand),
	g_FILLER(S_drksand),
	g_FILLER(S_lava),
	g_FILLER(S_vodbridge),
	g_FILLER(S_hodbridge),
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
	g_FILLER(S_air),
	g_FILLER(S_cloud),
	g_FILLER(S_fog),
	g_FILLER(S_dust),
	g_FILLER(S_puddle),
	g_FILLER(S_water),
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
	g_FILLER(S_sleeping_gas_trap),
/*50*/	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
	g_FILLER(S_web),
/*60*/	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	g_FILLER(S_essence_trap),
	g_FILLER(S_mummy_trap),
	g_FILLER(S_switch),
	g_FILLER(S_flesh_hook),
	g_FILLER(S_vbeam),
	g_FILLER(S_hbeam),
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_digbeam),
	g_FILLER(S_flashbeam),
/*70*/	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	g_FILLER(S_sw_tc),
	g_FILLER(S_sw_tr),
	g_FILLER(S_sw_ml),
/*80*/	g_FILLER(S_sw_mr),
	g_FILLER(S_sw_bl),
	g_FILLER(S_sw_bc),
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	g_FILLER(S_explode2),
	g_FILLER(S_explode3),
	g_FILLER(S_explode4),
	g_FILLER(S_explode5),
	g_FILLER(S_explode6),
/*90*/	g_FILLER(S_explode7),
	g_FILLER(S_explode8),
	g_FILLER(S_explode9)
};
#endif	/* MAC_GRAPHICS_ENV */

#ifdef UTF8_GLYPHS
/* Probably best to only use characters from this list
 * http://en.wikipedia.org/wiki/WGL4 */
static glyph_t utf8_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0x2502,	/* S_vwall:	BOX DRAWINGS LIGHT VERTICAL */
	0x2500,	/* S_hwall:	BOX DRAWINGS LIGHT HORIZONTAL */
	0x250c,	/* S_tlcorn:	BOX DRAWINGS LIGHT DOWN AND RIGHT */
	0x2510,	/* S_trcorn:	BOX DRAWINGS LIGHT DOWN AND LEFT */
	0x2514,	/* S_blcorn:	BOX DRAWINGS LIGHT UP AND RIGHT */
	0x2518,	/* S_brcorn:	BOX DRAWINGS LIGHT UP AND LEFT */
	0x253c,	/* S_crwall:	BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
	0x2534,	/* S_tuwall:	BOX DRAWINGS LIGHT UP AND HORIZONTAL */
	0x252c,	/* S_tdwall:	BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
	0x2524,	/* S_tlwall:	BOX DRAWINGS LIGHT VERTICAL AND LEFT */
	0x251c,	/* S_trwall:	BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
	0x00b7,	/* S_ndoor:	MIDDLE DOT */
	0x2592,	/* S_vodoor:	MEDIUM SHADE */
	0x2592,	/* S_hodoor:	MEDIUM SHADE */
	g_FILLER(S_vcdoor),
	g_FILLER(S_hcdoor),
	0x2261,	/* S_bars:	IDENTICAL TO */
	0x03a8,	/* S_tree:	GREEK CAPITAL LETTER PSI */
	0x03a8,	/* S_deadtree:GREEK CAPITAL LETTER PSI */
	0x00b7,	/* S_drkroom:	MIDDLE DOT */
	0x00b7,	/* S_litroom:	MIDDLE DOT */
	0x00b7,	/* S_brightrm:	MIDDLE DOT */
	g_FILLER(S_corr),
	g_FILLER(S_litcorr),
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	0x2264,	/* S_upladder:	LESS-THAN OR EQUAL TO */
	0x2265,	/* S_dnladder:	GREATER-THAN OR EQUAL TO */
	g_FILLER(S_brupstair),
	g_FILLER(S_brdnstair),
	0x2264,	/* S_brupladder:	LESS-THAN OR EQUAL TO */
	0x2265,	/* S_brdnladder:	GREATER-THAN OR EQUAL TO */
	0x03A9,	/* S_altar:	GREEK CAPITAL LETTER OMEGA */
	0x2020,	/* S_grave:	DAGGER */
	g_FILLER(S_seal),
	g_FILLER(S_throne),
	g_FILLER(S_sink),
	0x00b6,	/* S_fountain:	PILCROW SIGN */
	0x00b6,	/* S_forge:	PILCROW SIGN */
	0x224b,	/* S_pool:	TRIPLE TILDE */
	0x00b7,	/* S_ice:	MIDDLE DOT */
	0x00b7,	/* S_litgrass:	MIDDLE DOT */
	0x00b7,	/* S_drkgrass:	MIDDLE DOT */
	0x00b7,	/* S_litsoil:	MIDDLE DOT */
	0x00b7,	/* S_drksoil:	MIDDLE DOT */
	0x224b,	/* S_litsand:	TRIPLE TILDE */
	0x224b,	/* S_drksand:	TRIPLE TILDE */
	0x224b,	/* S_lava:	TRIPLE TILDE */
	0x00b7,	/* S_vodbridge:	MIDDLE DOT */
	0x00b7,	/* S_hodbridge:	MIDDLE DOT */
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
	g_FILLER(S_air),
	g_FILLER(S_cloud),
	g_FILLER(S_fog),
	g_FILLER(S_dust),
	g_FILLER(S_puddle),
	0x2248,	/* S_water:	ALMOST EQUAL TO */
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
	g_FILLER(S_sleeping_gas_trap),
	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
	0x00A4,	/* S_web:	CURRENCY SIGN */
	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	g_FILLER(S_essence_trap),
	g_FILLER(S_mummy_trap),
	g_FILLER(S_switch),
	g_FILLER(S_flesh_hook),
	0x2502,	/* S_vbeam:	BOX DRAWINGS LIGHT VERTICAL */
	0x2500,	/* S_hbeam:	BOX DRAWINGS LIGHT HORIZONTAL */
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_digbeam),
	g_FILLER(S_flashbeam),
	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	0x2594,	/* S_sw_tc:	UPPER ONE EIGHTH BLOCK */
	g_FILLER(S_sw_tr),
	0x258f,	/* S_sw_ml:	LEFT ONE EIGHTH BLOCK */
	0x2595,	/* S_sw_mr:	RIGHT ONE EIGHTH BLOCK */
	g_FILLER(S_sw_bl),
	0x2581,	/* S_sw_bc:	LOWER ONE EIGHTH BLOCK */
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	0x2594,	/* S_explode2:	UPPER ONE EIGHTH BLOCK */
	g_FILLER(S_explode3),
	0x258f,	/* S_explode4:	LEFT ONE EIGHTH BLOCK */
	g_FILLER(S_explode5),
	0x2595,	/* S_explode6:	RIGHT ONE EIGHTH BLOCK */
	g_FILLER(S_explode7),
	0x2581,	/* S_explode8:	LOWER ONE EIGHTH BLOCK */
	g_FILLER(S_explode9)
};
#endif

#ifdef PC9800
void NDECL((*ascgraphics_mode_callback)) = 0;	/* set in tty_start_screen() */
#endif

/*
 * Convert the given character to an object class.  If the character is not
 * recognized, then MAXOCLASSES is returned.  Used in detect.c invent.c,
 * options.c, pickup.c, sp_lev.c, and lev_main.c.
 */
int
def_char_to_objclass(ch)
    char ch;
{
    int i;
    for (i = 1; i < MAXOCLASSES; i++)
	if (ch == def_oc_syms[i]) break;
    return i;
}

/*
 * Convert a character into a monster class.  This returns the _first_
 * match made.  If there are are no matches, return MAXMCLASSES.
 */
int
def_char_to_monclass(ch)
    char ch;
{
    int i;
    for (i = 1; i < MAXMCLASSES; i++)
	if (def_monsyms[i] == ch) break;
    return i;
}

void
assign_graphics(graph_chars, glth, maxlen, offset)
register glyph_t *graph_chars;
int glth, maxlen, offset;
{
    register int i;
    for (i = 0; i < maxlen; i++)
	showsyms[i+offset] = (((i < glth) && graph_chars[i]) ?
		       graph_chars[i] : defsyms[i+offset].sym);
    if (monsyms[S_GHOST] == DEF_GHOST || monsyms[S_GHOST] == 183)
		monsyms[S_GHOST] = showsyms[S_litroom];
}

#ifdef USER_DUNGEONCOLOR
void
assign_colors(graph_colors, glth, maxlen, offset)
register uchar *graph_colors;
int glth, maxlen, offset;
{
    register int i;

    for (i = 0; i < maxlen; i++)
	showsymcolors[i+offset] =
	    (((i < glth) && (graph_colors[i] < CLR_MAX)) ?
	     graph_colors[i] : defsyms[i+offset].color);
}
#endif

void
switch_graphics(gr_set_flag)
int gr_set_flag;
{
    iflags.IBMgraphics = FALSE;
    iflags.DECgraphics = FALSE;
    iflags.UTF8graphics = FALSE;
#ifdef CURSES_GRAPHICS
    iflags.cursesgraphics = FALSE;
#endif
    switch (gr_set_flag) {
    default:
    case ASCII_GRAPHICS:
	    assign_graphics((glyph_t *)0, 0, MAXPCHARS, 0);
#ifdef PC9800
	    if (ascgraphics_mode_callback) (*ascgraphics_mode_callback)();
#endif
	    break;
#ifdef ASCIIGRAPH
    case IBM_GRAPHICS:
/*
 * Use the nice IBM Extended ASCII line-drawing characters (codepage 437).
 *
 * OS/2 defaults to a multilingual character set (codepage 850, corresponding
 * to the ISO 8859 character set.  We should probably do a VioSetCp() call to
 * set the codepage to 437.
 */
	    iflags.IBMgraphics = TRUE;
	    assign_graphics(ibm_graphics, SIZE(ibm_graphics), MAXPCHARS, 0);
	    /*
	     * Special angel symbols disabled because they are broken:
	     * they don't work if you start the game with IBMgraphics
	     * and have the "monsters" option configured (but do if
	     * you switch to IBMgraphics in-game), and aren't reset
	     * when you switch away from IBMgraphics.
	     *
	     * monsyms[S_LAW_ANGEL] = 0x8F;
	     * monsyms[S_NEU_ANGEL] = 0x8E;
	     */
#ifdef PC9800
	    if (ibmgraphics_mode_callback) (*ibmgraphics_mode_callback)();
#endif
	    break;
#endif /* ASCIIGRAPH */
#ifdef TERMLIB
    case DEC_GRAPHICS:
/*
 * Use the VT100 line drawing character set.
 */
	    iflags.DECgraphics = TRUE;
	    assign_graphics(dec_graphics, SIZE(dec_graphics), MAXPCHARS, 0);
	    if (decgraphics_mode_callback) (*decgraphics_mode_callback)();
	    break;
#endif /* TERMLIB */
#ifdef MAC_GRAPHICS_ENV
    case MAC_GRAPHICS:
	    assign_graphics(mac_graphics, SIZE(mac_graphics), MAXPCHARS, 0);
	    break;
#endif
#ifdef UTF8_GLYPHS
    case UTF8_GRAPHICS:
	    assign_graphics(utf8_graphics, SIZE(utf8_graphics), MAXPCHARS, 0);
	    iflags.UTF8graphics = TRUE;
	    break;
#endif
#ifdef CURSES_GRAPHICS
    case CURS_GRAPHICS:
	    assign_graphics((glyph_t *)0, 0, MAXPCHARS, 0);
	    iflags.cursesgraphics = TRUE;
	    break;
#endif
    }
    return;
}

/** Change the UTF8graphics symbol at position with codepoint "value". */
void
assign_utf8graphics_symbol(position, value)
int position;
glyph_t value;
{
#ifdef UTF8_GLYPHS
	if (position < MAXPCHARS) {
		utf8_graphics[position] = value;
		/* need to update showsym */
		if (iflags.UTF8graphics) {
			switch_graphics(UTF8_GRAPHICS);
		}
	}
#endif
}


#ifdef REINCARNATION

/*
 * saved display symbols for objects & features.
 */
static uchar save_oc_syms[MAXOCLASSES] = DUMMY;
static glyph_t save_showsyms[MAXPCHARS]  = DUMMY;
static uchar save_monsyms[MAXPCHARS]   = DUMMY;

static const glyph_t r_oc_syms[MAXOCLASSES] = {
/* 0*/	'\0',
	ILLOBJ_SYM,
	WEAPON_SYM,
	']',			/* armor */
	RING_SYM,
/* 5*/	',',			/* amulet */
	TOOL_SYM,
	':',			/* food */
	POTION_SYM,
	SCROLL_SYM,
/*10*/	SPBOOK_SYM,
	WAND_SYM,
	GEM_SYM,		/* gold -- yes it's the same as gems */
	GEM_SYM,
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM,
	TILE_SYM,
	BED_SYM,
	SCOIN_SYM
};

# ifdef ASCIIGRAPH
/* Rogue level graphics.  Under IBM graphics mode, use the symbols that were
 * used for Rogue on the IBM PC.  Unfortunately, this can't be completely
 * done because some of these are control characters--armor and rings under
 * DOS, and a whole bunch of them under Linux.  Use the TTY Rogue characters
 * for those cases.
 */
static const uchar IBM_r_oc_syms[MAXOCLASSES] = {	/* a la EPYX Rogue */
/* 0*/	'\0',
	ILLOBJ_SYM,
#  if defined(MSDOS) || defined(OS2) || ( defined(WIN32) && !defined(MSWIN_GRAPHICS) )
	0x18,			/* weapon: up arrow */
/*	0x0a, */ ARMOR_SYM,	/* armor:  Vert rect with o */
/*	0x09, */ RING_SYM,	/* ring:   circle with arrow */
/* 5*/	0x0c,			/* amulet: "female" symbol */
	TOOL_SYM,
	0x05,			/* food:   club (as in cards) */
	0xad,			/* potion: upside down '!' */
	0x0e,			/* scroll: musical note */
/*10*/	SPBOOK_SYM,
	0xe7,			/* wand:   greek tau */
	0x0f,			/* gold:   yes it's the same as gems */
	0x0f,			/* gems:   fancy '*' */
#  else
	')',			/* weapon  */
	ARMOR_SYM,		/* armor */
	RING_SYM,		/* ring */
/* 5*/	',',			/* amulet  */
	TOOL_SYM,
	':',			/* food    */
	0xad,			/* potion: upside down '!' */
	SCROLL_SYM,		/* scroll  */
/*10*/	SPBOOK_SYM,
	0xe7,			/* wand:   greek tau */
	GEM_SYM,		/* gold:   yes it's the same as gems */
	GEM_SYM,		/* gems    */
#  endif
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM,
	TILE_SYM,
	BED_SYM,
	SCOIN_SYM,
	BELT_SYM,
};
# endif /* ASCIIGRAPH */

void
assign_rogue_graphics(is_rlevel)
boolean is_rlevel;
{
    /* Adjust graphics display characters on Rogue levels */

    if (is_rlevel) {
	register int i;

	(void) memcpy((genericptr_t)save_showsyms,
		      (genericptr_t)showsyms, sizeof showsyms);
	(void) memcpy((genericptr_t)save_oc_syms,
		      (genericptr_t)oc_syms, sizeof oc_syms);
	(void) memcpy((genericptr_t)save_monsyms,
		      (genericptr_t)monsyms, sizeof monsyms);

	/* Use a loop: char != uchar on some machines. */
	for (i = 0; i < MAXMCLASSES; i++)
	    monsyms[i] = def_monsyms[i];
# if defined(ASCIIGRAPH) && !defined(MSWIN_GRAPHICS)
	if (iflags.IBMgraphics
#  if defined(USE_TILES) && defined(MSDOS)
		&& !iflags.grmode
#  endif
		)
	    monsyms[S_HUMAN] = 0x01; /* smiley face */
# endif
	for (i = 0; i < MAXPCHARS; i++)
	    showsyms[i] = defsyms[i].sym;

/*
 * Some day if these rogue showsyms get much more extensive than this,
 * we may want to create r_showsyms, and IBM_r_showsyms arrays to hold
 * all of this info and to simply initialize it via a for() loop like r_oc_syms.
 */

# ifdef ASCIIGRAPH
	if (!iflags.IBMgraphics
#  if defined(USE_TILES) && defined(MSDOS)
		|| iflags.grmode
#  endif
				) {
# endif
	    showsyms[S_vodoor]  = showsyms[S_hodoor]  = showsyms[S_ndoor] = '+';
	    showsyms[S_upstair] = showsyms[S_dnstair] = showsyms[S_brupstair] = showsyms[S_brdnstair] = '%';
# ifdef ASCIIGRAPH
	} else {
	    /* a la EPYX Rogue */
	    showsyms[S_vwall]   = 0xba; /* all walls now use	*/
	    showsyms[S_hwall]   = 0xcd; /* double line graphics	*/
	    showsyms[S_tlcorn]  = 0xc9;
	    showsyms[S_trcorn]  = 0xbb;
	    showsyms[S_blcorn]  = 0xc8;
	    showsyms[S_brcorn]  = 0xbc;
	    showsyms[S_crwall]  = 0xce;
	    showsyms[S_tuwall]  = 0xca;
	    showsyms[S_tdwall]  = 0xcb;
	    showsyms[S_tlwall]  = 0xb9;
	    showsyms[S_trwall]  = 0xcc;
	    showsyms[S_ndoor]   = 0xce;
	    showsyms[S_vodoor]  = 0xce;
	    showsyms[S_hodoor]  = 0xce;
	    showsyms[S_drkroom] = 0xfa; /* centered dot */
	    showsyms[S_litroom] = 0xfa; /* centered open circle */
	    showsyms[S_brightrm]= 0xfe; /* centered square */
	    showsyms[S_corr]    = 0xb1;
	    showsyms[S_litcorr] = 0xb2;
	    showsyms[S_upstair] = 0xf0; /* Greek Xi */
	    showsyms[S_dnstair] = 0xf0;
	    showsyms[S_brupstair] = 0xf0;
	    showsyms[S_brdnstair] = 0xf0;
#ifndef MSWIN_GRAPHICS
	    showsyms[S_arrow_trap] = 0x04; /* diamond (cards) */
	    showsyms[S_dart_trap] = 0x04;
	    showsyms[S_falling_rock_trap] = 0x04;
	    showsyms[S_squeaky_board] = 0x04;
	    showsyms[S_bear_trap] = 0x04;
	    showsyms[S_land_mine] = 0x04;
	    showsyms[S_rolling_boulder_trap] = 0x04;
	    showsyms[S_sleeping_gas_trap] = 0x04;
	    showsyms[S_rust_trap] = 0x04;
	    showsyms[S_fire_trap] = 0x04;
	    showsyms[S_pit] = 0x04;
	    showsyms[S_spiked_pit] = 0x04;
	    showsyms[S_hole] = 0x04;
	    showsyms[S_trap_door] = 0x04;
	    showsyms[S_teleportation_trap] = 0x04;
	    showsyms[S_level_teleporter] = 0x04;
	    showsyms[S_magic_portal] = 0x04;
	    showsyms[S_web] = 0x04;
	    showsyms[S_statue_trap] = 0x04;
	    showsyms[S_magic_trap] = 0x04;
	    showsyms[S_anti_magic_trap] = 0x04;
	    showsyms[S_polymorph_trap] = 0x04;
	    showsyms[S_essence_trap] = 0x04;
	    showsyms[S_mummy_trap] = 0x04;
	    showsyms[S_switch] = 0x04;
	    showsyms[S_flesh_hook] = 0x04;
#endif
		monsyms[S_GHOST] = showsyms[S_litroom];
	}
#endif /* ASCIIGRAPH */

	for (i = 0; i < MAXOCLASSES; i++) {
#ifdef ASCIIGRAPH
	    if (iflags.IBMgraphics
# if defined(USE_TILES) && defined(MSDOS)
		&& !iflags.grmode
# endif
		)
		oc_syms[i] = IBM_r_oc_syms[i];
	    else
#endif /* ASCIIGRAPH */
		oc_syms[i] = r_oc_syms[i];
	}
#if defined(MSDOS) && defined(USE_TILES)
	if (iflags.grmode) tileview(FALSE);
#endif
    } else {
	(void) memcpy((genericptr_t)showsyms,
		      (genericptr_t)save_showsyms, sizeof showsyms);
	(void) memcpy((genericptr_t)oc_syms,
		      (genericptr_t)save_oc_syms, sizeof oc_syms);
	(void) memcpy((genericptr_t)monsyms,
		      (genericptr_t)save_monsyms, sizeof monsyms);
#if defined(MSDOS) && defined(USE_TILES)
	if (iflags.grmode) tileview(TRUE);
#endif
    }
}
#endif /* REINCARNATION */

/*drawing.c*/
