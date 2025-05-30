/*	SCCS Id: @(#)pager.c	3.4	2003/08/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

#include "hack.h"
#include "dlb.h"
#include "xhity.h"

STATIC_DCL boolean FDECL(is_swallow_sym, (int));
STATIC_DCL int FDECL(append_str, (char *, const char *));
STATIC_DCL struct monst * FDECL(lookat, (int, int, char *, char *, char *));
STATIC_DCL int FDECL(do_look, (BOOLEAN_P));
STATIC_DCL boolean FDECL(help_menu, (int *));
STATIC_DCL char * get_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_generation_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_weight_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_resistance_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_weakness_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_conveys_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mm_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mt_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mb_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_ma_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mv_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mg_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mw_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_mf_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_speed_description_of_monster_type(struct monst *, char *);
STATIC_DCL char * get_description_of_damage_prefix(uchar, uchar);
STATIC_DCL char * get_description_of_ancient_breath(struct monst *, char *);
STATIC_DCL int generate_list_of_resistances(struct monst *, char *, int);
#ifdef PORT_HELP
extern void NDECL(port_help);
#endif

extern const int monstr[];
extern struct attack noattack;


/* Returns "true" for characters that could represent a monster's stomach. */
STATIC_OVL boolean
is_swallow_sym(c)
int c;
{
    int i;
    for (i = S_sw_tl; i <= S_sw_br; i++)
	if ((int)showsyms[i] == c) return TRUE;
    return FALSE;
}

/*
 * Append new_str to the end of buf if new_str doesn't already exist as
 * a substring of buf.  Return 1 if the string was appended, 0 otherwise.
 * This only works on buf of LONGBUFSZ.
 */
STATIC_OVL int
append_str(buf, new_str)
    char *buf;
    const char *new_str;
{
    int space_left;	/* space remaining in buf */

    if (strstri(buf, new_str)) return 0;

    space_left = LONGBUFSZ - strlen(buf) - 1;
    (void) strncat(buf, " or ", space_left);
    (void) strncat(buf, new_str, space_left - 4);
    return 1;
}

static const char * const sizeStr[] = {
	"fine",
	"gnat-sized",
	"diminutive",
	"tiny",
	"cat-sized",
	"bigger-than-a-breadbox",
	"small",
	"human-sized",
	"medium",
	"large",
	"gigantic",
	"colossal",
	"cosmically-huge"
};

static const char * const headStr[] = {
	"",
	"",
	"",
	"",
	"",
	" snouted",
	" flat-faced",
	" short-necked",
	" inverted-faced",
	" long-necked"
};

static const char * const bodyStr[] = {
	" animal",
	" ophidian",
	" squigglenoid",
	" insectoid",
	" humanoid",
	" snake-legged humanoid",
	" snake-backed animal",
	" centauroid"
};


/* extracted from lookat(); also used by do_floorname() */
boolean
object_from_map(glyph, x, y, obj_p)
    int glyph, x, y;
    struct obj **obj_p;
{
    boolean fakeobj = FALSE;
    struct obj *otmp = vobj_at(x,y);

    if (!otmp || otmp->otyp != glyph_to_obj(glyph)) {
	if (glyph_to_obj(glyph) != STRANGE_OBJECT) {
	    otmp = mksobj(glyph_to_obj(glyph), MKOBJ_NOINIT);
	    if (!otmp)
		return FALSE;
	    fakeobj = TRUE;
	    if (otmp->oclass == COIN_CLASS)
		otmp->quan = 2L; /* to force pluralization */
	    else if (otmp->otyp == SLIME_MOLD)
		otmp->spe = current_fruit;	/* give the fruit a type */
	}
    }

    /* if located at adajcent spot, mark it as having been seen up close */
    if (otmp && distu(x, y) <= 2 && !Blind && !Hallucination)
        otmp->dknown = 1;

    *obj_p = otmp;

    return fakeobj;
}

#define MM_FLAG 1
#define MT_FLAG 2
#define MB_FLAG 3
#define MV_FLAG 4
#define MG_FLAG 5
#define MA_FLAG 6
#define MF_FLAG 7
#define MSYM	8

char *
flag_to_word(flag, category)
	unsigned long flag; /* flag to check */
	int category; /* category (MM, MT, etc.). 1-indexed from MM in order in monflag.h */
{
	switch (category){
	case MM_FLAG:
		switch (flag) {
			case MM_FLY: return "fly";
			case MM_SWIM: return "swim";
			case MM_AMORPHOUS: return "ooze";
			case MM_WALLWALK: return "walk on walls";
			case MM_CLING: return "cling";
			case MM_TUNNEL: return "tunnel";
			case MM_NEEDPICK: return "dig";
			case MM_AMPHIBIOUS: return "breathe underwater";
			case MM_BREATHLESS: return "don't breathe";
			case MM_TPORT: return "teleport";
			case MM_TPORT_CNTRL: return "control their teleports";
			case MM_TENGTPORT: return "teleport to follow you";
			case MM_STATIONARY: return "don't move";
			case MM_FLOAT: return "float";
			case MM_NOTONL: return "avoid projectiles";
			case MM_FLEETFLEE: return "flee";
			case MM_WEBRIP: return "tear webs";
			case MM_DOORBUST: return "break down doors";
			case MM_AQUATIC: return "lives underwater";
		}
	break;
	case MT_FLAG:
		switch (flag){
			case MT_WANTSAMUL: return "want the Amulet of Yendor";
			case MT_WANTSBELL: return "want the Bell of Opening";
			case MT_WANTSBOOK: return "want the Book of the Dead";
			case MT_WANTSCAND: return "want the Candelabrum";
			case MT_WANTSARTI: return "want artifacts";
			case MT_WAITFORU: return "wait for you";
			case MT_CLOSE: return "let you close in";
			case MT_PEACEFUL: return "start peaceful";
			case MT_DOMESTIC: return "can be tamed by feeding";
			case MT_WANDER: return "wander around";
			case MT_STALK: return "stalk you";
			case MT_ROCKTHROW: return "throw boulders";
			case MT_GREEDY: return "like gold";
			case MT_JEWELS: return "like precious gems";
			case MT_COLLECT: return "like weapons and food";
			case MT_MAGIC: return "like magic items";
			case MT_CONCEAL: return "hides under objects";
			case MT_HIDE: return "hides on the ceiling";
			case MT_MINDLESS: return "are mindless";
			case MT_ANIMAL: return "are animals";
			case MT_CARNIVORE: return "are carnivores";
			case MT_HERBIVORE: return "are herbivores";
			case MT_HOSTILE: return "are always hostile";
			case MT_TRAITOR: return "are traitorous";
			case MT_NOTAKE: return "don't pick up items";
			case MT_METALLIVORE: return "eat metal";
			case MT_MAGIVORE: return "eat magic";
			case MT_BOLD: return "recover from fear quickly";
		}
	break;
	case MB_FLAG:
		switch (flag){
			case MB_NOEYES: return "have no eyes";
			case MB_NOHANDS: return "have no hands";
			case MB_NOLIMBS: return "have no limbs";
			case MB_NOHEAD: return "have no head";
			case MB_HUMANOID: return "are humanoid";
			case MB_ANIMAL: return "are animals";
			case MB_SLITHY: return "are slithery";
			case MB_UNSOLID: return "are unsolid";
			case MB_THICK_HIDE: return "have a thick hide";
			case MB_OVIPAROUS: return "are oviparous";
			case MB_ACID: return "are acidic";
			case MB_POIS: return "are poisonous";
			case MB_CHILL: return "are cold to the touch";
			case MB_TOSTY: return "are hot to the touch";
			case MB_HALUC: return "are hallucinogenic to eat";
			case MB_MALE: return "are always male";
			case MB_FEMALE: return "are always female";
			case MB_NEUTER: return "are always gender-neutral";
			case MB_STRONG: return "are physically strong";
			case MB_WINGS: return "have wings";
			case MB_LONGHEAD: return "have a long head";
			case MB_LONGNECK: return "have a long neck";
			case MB_NOFEET: return "have no feet";
			case MB_HAS_FEET: return "have feet";
			case MB_CAN_AMULET: return "can wear an amulet";
			case MB_INDIGESTIBLE: return "are indigestible";
			case MB_INSUBSTANTIAL: return "are insubstantial";
			case MB_NOGLOVES: return "cannot wear gloves";
			case MB_NOHAT: return "cannot wear hats";
			case MB_SNAKELEG: return "have snake-like legs";
			case MB_CENTAUR: return "have centaur-like legs";
		}
	case MV_FLAG:
		switch (flag){
			case MV_NORMAL: return "have normal sight";
			case MV_INFRAVISION: return "have infravision";
			case MV_DARKSIGHT: return "have darkvision";
			case MV_LOWLIGHT2: return "have low-light vision";
			case MV_LOWLIGHT3: return "have exceptional low-light vision";
			case MV_CATSIGHT: return "have catsight";
			case MV_ECHOLOCATE: return "can echolocate";
			case MV_BLOODSENSE: return "have bloodsense";
			case MV_LIFESENSE: return "have lifesense";
			case MV_EXTRAMISSION: return "have extramission";
			case MV_TELEPATHIC: return "have telepathy";
			case MV_RLYEHIAN: return "have R'lyehian sight";
			case MV_SEE_INVIS: return "can see invisible";
			case MV_DETECTION: return "can detect monsters";
			case MV_OMNI: return "have omnivision";
			case MV_SCENT: return "have a keen sense of smell";
			case MV_EARTHSENSE: return "can sense vibrations";
		}
	break;
	case MG_FLAG:
		switch (flag){
			case MG_REGEN: return "regenerate naturally";
			case MG_NOPOLY: return "cannot be polymorphed into";
			case MG_MERC: return "work for coin";
			case MG_PNAME: return "have a title";
			case MG_LORD: return "are lords or ladies";
			case MG_PRINCE: return "are princes or princesses";
			case MG_NASTY: return "are particularly nasty";
			case MG_INFRAVISIBLE: return "are infravisible";
			case MG_OPAQUE: return "block line of sight";
			case MG_DISPLACEMENT: return "have displacement";
			case MG_HATESSILVER: return "hate silver";
			case MG_HATESIRON: return "hate iron";
			case MG_HATESUNHOLY: return "hate unholy";
			case MG_HATESHOLY: return "hate holy";
			case MG_RIDER: return "transcend mortality";
			case MG_DEADLY: return "should not be consumed";
			case MG_TRACKER: return "can track";
			case MG_NOSPELLCOOLDOWN: return "have no spell cooldown";
			case MG_RBLUNT: return "resist blunt";
			case MG_RSLASH: return "resist slashing";
			case MG_RPIERCE: return "resist piercing";
			case MG_VBLUNT: return "are vulnerable to blunt";
			case MG_VSLASH: return "are vulnerable to slashing";
			case MG_VPIERCE: return "are vulnerable to piercing";
			case MG_WRESIST: return "resist weapon damage";
			case MG_NOTAME: return "are not tameable";
			case MG_NOWISH: return "cannot be wished for";
			case MG_BACKSTAB: return "can sneak attack";
			case MG_COMMANDER: return "command others";
			case MG_SANLOSS: return "drive you insane";
			case MG_INSIGHT: return "reveal the truth";
			case MG_RIDER_HP: return "have rider health";
			case MG_FUTURE_WISH: return "are from the future";
			case MG_HATESUNBLESSED: return "hate unblessed";
		}
	break;
	case MA_FLAG:
		switch (flag){
			case MA_UNDEAD: return "undead";
			case MA_WERE: return "werecreatures";
			case MA_HUMAN: return "humans";
			case MA_ELF: return "elves";
			case MA_DROW: return "drow";
			case MA_DWARF: return "dwarves";
			case MA_GNOME: return "gnomes";
			case MA_ORC: return "orcs";
			case MA_VAMPIRE: return "vampires";
			case MA_CLOCK: return "clockwork automatons and androids";
			case MA_UNLIVING: return "unliving";
			case MA_PLANT: return "plants";
			case MA_GIANT: return "giants";
			case MA_INSECTOID: return "insects";
			case MA_ARACHNID: return "arachnids";
			case MA_AVIAN: return "avians";
			case MA_REPTILIAN: return "reptiles";
			case MA_ANIMAL: return "animals";
			case MA_AQUATIC: return "aquatic creatures";
			case MA_DEMIHUMAN: return "demihumans";
			case MA_FEY: return "fey";
			case MA_ELEMENTAL: return "elementals";
			case MA_DRAGON: return "dragons";
			case MA_DEMON: return "demons";
			case MA_MINION: return "divine minions";
			case MA_PRIMORDIAL: return "primordials";
			case MA_ET: return "extraterrestrial";
			case MA_G_O_O: return "Great Old Ones";
			case MA_XORN: return "xorns";
		}
	break;
	case MF_FLAG:
		switch (flag){
			case MF_MARTIAL_B: return "basic martial skill";
			case MF_MARTIAL_S: return "skilled martial skill";
			case MF_MARTIAL_E: return "expert martial skill";
			case MF_BAB_FULL: return "full base attack bonus";
			case MF_BAB_HALF: return "half base attack bonus";
			case MF_LEVEL_30: return "can reach level 30";
			case MF_LEVEL_45: return "can reach level 45";
			case MF_PHYS_SCALING: return "receives level-based bonus to physical damage";
		}
	break;
	case MSYM:
		return (char*)monexplain[flag];
	default:
		impossible("flag to words out of bounds?");
	break;
	}
	return "";
}

void
warned_monster_reasons(mon, wbuf)
	struct monst * mon;
	char * wbuf;
{
	char buf[BUFSZ];
	strcpy(buf, "");

	unsigned long ma_flags = mon->data->mflagsa;

	if (is_undead(mon->data)) ma_flags |= MA_UNDEAD;

	boolean has_ma = FALSE;
	boolean has_other = FALSE;

#define wflag_and_mflag(wflag, mflag) (((mflag & i) > 0) && ((wflag & i) > 0))

	for (unsigned long i = MA_UNDEAD; i <= MA_XORN; i = i << 1){
		if (!wflag_and_mflag(flags.warntypea, ma_flags) \
			|| ((ma_flags & MA_UNDEAD) > 0 && u.sealsActive&SEAL_ACERERAK)) continue;

		Strcat(buf, flag_to_word(i, MA_FLAG));
		Strcat(buf, ", ");
		if (!has_ma) has_ma = TRUE;
	}

	if ((flags.montype & (unsigned long long int)((unsigned long long int)1 << (int)(mon->data->mlet))) > 0){
		Strcat(buf, makeplural(monexplain[(int)(mon->data->mlet)]));
		Strcat(buf, ", ");
		if (!has_ma) has_ma = TRUE;
	}

	if (u.sealsActive&SEAL_PAIMON && is_magical(mon->data)){
		Strcat(buf, "magical beings, ");
		has_ma = TRUE;
	}

	if (u.sealsActive&SEAL_ANDROMALIUS && is_thief(mon->data)){
		Strcat(buf, "thieves, ");
		has_ma = TRUE;
	}

	if (u.sealsActive&SEAL_TENEBROUS && !nonliving(mon->data)){
		Strcat(buf, "living beings, ");
		has_ma = TRUE;
	}

	//if (u.sealsActive&SEAL_ACERERAK && is_undead(mon->data)) Strcat(buf, "are undead");

	if (uwep && uwep->oclass == WEAPON_CLASS && uwep->obj_material == WOOD && uwep->otyp != MOON_AXE &&\
					 (uwep->oward & WARD_THJOFASTAFUR) && ((mon)->data->mlet == S_LEPRECHAUN || (mon)->data->mlet == S_NYMPH || is_thief((mon)->data)))
	{
		Strcat(buf, "tricksters, ");
		has_ma = TRUE;
	}

	if (mon->mtame && beastMateryRadius(mon)){
		Strcat(buf, "nearby pets, ");
		has_ma = TRUE;
	}

	if (has_ma){
		buf[strlen(buf)-2] = '\0';
		Strcat(buf, " and monsters that ");
	}
	else Strcat(buf, "monsters that ");

	for (unsigned long i = MM_FLY; i <= MM_DOORBUST; i = i << 1){
		if (!wflag_and_mflag(flags.warntypem, mon->data->mflagsm)) continue;
		Strcat(buf, flag_to_word(i, MM_FLAG));
		Strcat(buf, ", ");
		if (!has_other) has_other = TRUE;
	}

	for (unsigned long i = MT_WANTSAMUL; i <= MT_BOLD; i = i << 1){
		if (!wflag_and_mflag(flags.warntypet, mon->data->mflagst)) continue;
		Strcat(buf, flag_to_word(i, MT_FLAG));
		Strcat(buf, ", ");
		if (!has_other) has_other = TRUE;
	}

	for (unsigned long i = MB_NOEYES; i <= MB_NOHAT; i = i << 1){
		if (!wflag_and_mflag(flags.warntypeb, mon->data->mflagsb)) continue;
		Strcat(buf, flag_to_word(i, MB_FLAG));
		Strcat(buf, ", ");
	}

	for (unsigned long i = MV_NORMAL; i <= MV_EARTHSENSE; i = i << 1){
		if (!wflag_and_mflag(flags.warntypev, mon->data->mflagsv)) continue;
		Strcat(buf, flag_to_word(i, MV_FLAG));
		Strcat(buf, ", ");
		if (!has_other) has_other = TRUE;
	}

	for (unsigned long i = MG_REGEN; i <= MG_HATESUNBLESSED; i = i << 1){
		if (!wflag_and_mflag(flags.warntypeg, mon->data->mflagsg)) continue;
		Strcat(buf, flag_to_word(i, MG_FLAG));
		Strcat(buf, ", ");
		if (!has_other) has_other = TRUE;
	}

	if ((Upolyd && youmonst.data->mtyp == PM_SHARK && has_blood((mon)->data) && \
			(mon)->mhp < (mon)->mhpmax && is_pool(u.ux, u.uy, TRUE) && is_pool((mon)->mx, (mon)->my, TRUE)))
	{
		Strcat(buf, "are bleeding in the nearby water, ");
		has_other = TRUE;
	}

	if (has_other)
		buf[strlen(buf)-2] = '\0';
	else if (has_ma)
		buf[strlen(buf)-18] = '\0';
	else {
		buf[strlen(buf)-14] = '\0';
		Strcat(buf, "nothing in particular");
	}
	Strcat(wbuf, buf);
}

#undef MM_FLAG
#undef MT_FLAG
#undef MB_FLAG
#undef MV_FLAG
#undef MG_FLAG
#undef MA_FLAG

/*
 * Return the name of the glyph found at (x,y).
 * If not hallucinating and the glyph is a monster, also monster data.
 */
STATIC_OVL struct monst *
lookat(x, y, buf, monbuf, shapebuff)
    int x, y;
    char *buf, *monbuf, *shapebuff;
{
    register struct monst *mtmp = (struct monst *) 0;
    struct permonst *pm = (struct permonst *) 0;
    int glyph;

	int do_halu = Hallucination;
	
    buf[0] = monbuf[0] = shapebuff[0] = 0;
    glyph = glyph_at(x,y);
    if (u.ux == x && u.uy == y && senseself()) {
	char race[QBUFSZ];

	/* if not polymorphed, show both the role and the race */
	race[0] = 0;
	if (!Upolyd) {
	    Sprintf(race, "%s ", urace.adj);
	}

	Sprintf(buf, "%s%s%s called %s",
		Invis ? "invisible " : "",
		race,
		mons[u.umonnum].mname,
		plname);
	/* file lookup can't distinguish between "gnomish wizard" monster
	   and correspondingly named player character, always picking the
	   former; force it to find the general "wizard" entry instead */
	if (Role_if(PM_WIZARD) && Race_if(PM_GNOME) && !Upolyd)
	    pm = &mons[PM_WIZARD];

#ifdef STEED
	if (u.usteed) {
	    char steedbuf[BUFSZ];

	    Sprintf(steedbuf, ", mounted on %s", y_monnam(u.usteed));
	    /* assert((sizeof buf >= strlen(buf)+strlen(steedbuf)+1); */
	    Strcat(buf, steedbuf);
	}
#endif
	/* When you see yourself normally, no explanation is appended
	   (even if you could also see yourself via other means).
	   Sensing self while blind or swallowed is treated as if it
	   were by normal vision (cf canseeself()). */
	if ((Invisible || u.uundetected) && !Blind && !u.uswallow) {
	    unsigned how = 0;

	    if (Infravision)	 how |= 1;
	    if (Unblind_telepat) how |= 2;
	    if (Detect_monsters) how |= 4;

	    if (how)
		Sprintf(eos(buf), " [seen: %s%s%s%s%s]",
			(how & 1) ? "infravision" : "",
			/* add comma if telep and infrav */
			((how & 3) > 2) ? ", " : "",
			(how & 2) ? "telepathy" : "",
			/* add comma if detect and (infrav or telep or both) */
			((how & 7) > 4) ? ", " : "",
			(how & 4) ? "monster detection" : "");
	}
    } else if (u.uswallow) {
	/* all locations when swallowed other than the hero are the monster */
	Sprintf(buf, "interior of %s",
				    Blind ? "a monster" : a_monnam(u.ustuck));
	pm = u.ustuck->data;
    } else if (glyph_is_monster(glyph)) {
	bhitpos.x = x;
	bhitpos.y = y;
	mtmp = m_at(x,y);
	do_halu = Hallucination || Delusion(mtmp);
	if (mtmp != (struct monst *) 0) {
	    char *name, monnambuf[BUFSZ];
	    boolean accurate = !do_halu;
		struct permonst * mdat = mtmp->data;
		if (mtmp->m_ap_type == M_AP_MONSTER)
				mdat = &mons[mtmp->mappearance];

	    if (mdat->mtyp == PM_COYOTE && accurate)
			name = coyotename(mtmp, monnambuf);
	    else
			name = distant_monnam(mtmp, ARTICLE_NONE, monnambuf);

	    Sprintf(buf, "%s%s%s",
		    (mtmp->mx != x || mtmp->my != y) ?
			((mtmp->isshk && accurate)
				? "tail of " : "tail of a ") : "",
		    (mtmp->mtame && accurate) ? 
#ifdef BARD
		    (EDOG(mtmp)->friend ? "friendly " : (EDOG(mtmp)->loyal) ? "loyal " : "tame ") :
#else
		    "tame " :
#endif
		    (mtmp->mpeaceful && accurate) ? (mtmp->mtyp==PM_UVUUDAUM) ? "meditating " : "peaceful " : "",
		    name);
	    if (mtmp->mtyp==PM_DREAD_SERAPH && mtmp->mvar_dreadPrayer_progress)
		Strcat(buf, "praying ");
		
	    if (u.ustuck == mtmp)
		Strcat(buf, (sticks(&youmonst)) ?
			", being held" : ", holding you");
	    if (mtmp->mleashed)
		Strcat(buf, ", leashed to you");

	    if (mtmp->mtrapped && cansee(mtmp->mx, mtmp->my)) {
		struct trap *t = t_at(mtmp->mx, mtmp->my);
		int tt = t ? t->ttyp : NO_TRAP;

		/* newsym lets you know of the trap, so mention it here */
		if (tt == BEAR_TRAP || tt == FLESH_HOOK || tt == PIT ||
			tt == SPIKED_PIT || tt == WEB)
		    Sprintf(eos(buf), ", trapped in %s",
			    an(defsyms[trap_to_defsym(tt)].explanation));
	    }

		{
		int ways_seen = 0, normal = 0, xraydist = xraydist();
		boolean useemon = (boolean) canseemon(mtmp);

		/* normal vision */
		if ((mtmp->wormno ? worm_known(mtmp) : cansee(mtmp->mx, mtmp->my)) &&
			mon_visible(mtmp) && !mtmp->minvis) {
		    ways_seen++;
		    normal++;
		}
		/* see invisible */
		if (useemon && mtmp->minvis)
		    ways_seen++;
		/* infravision */
		if ((!mtmp->minvis || See_invisible(mtmp->mx,mtmp->my)) && see_with_infrared(mtmp))
		    ways_seen++;
		/* bloodsense */
		if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) && see_with_bloodsense(mtmp))
		    ways_seen++;
		/* lifesense */
		if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) && see_with_lifesense(mtmp))
		    ways_seen++;
		/* senseall */
		if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) && see_with_senseall(mtmp))
		    ways_seen++;
		/* earthsense */
		if (see_with_earthsense(mtmp))
		    ways_seen++;
		/* smell */
		if (sense_by_scent(mtmp))
		    ways_seen++;
		/* telepathy */
		if (tp_sensemon(mtmp))
		    ways_seen++;
		/* r'lyehian sight */
		if (rlyehian_sensemon(mtmp))
	            ways_seen++;
		/* xray */
		if (useemon && xraydist > 0 &&
			distu(mtmp->mx, mtmp->my) <= xraydist) {
			ways_seen++;
			if (!couldsee(mtmp->mx, mtmp->my))
				normal--;
		}
		if (Detect_monsters)
		    ways_seen++;
		if (MATCH_WARN_OF_MON(mtmp))
		    ways_seen++;
		
		if (ways_seen > 1 || !normal) {
		    if (normal) {
			Strcat(monbuf, "normal vision");
			/* can't actually be 1 yet here */
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (useemon && mtmp->minvis) {
			Strcat(monbuf, "see invisible");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
			if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) &&
			    see_with_infrared(mtmp)) {
			Strcat(monbuf, "infravision");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
			if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) &&
			    see_with_bloodsense(mtmp)) {
			Strcat(monbuf, "bloodsense");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
			if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) &&
			    see_with_lifesense(mtmp)) {
			Strcat(monbuf, "lifesense");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
			}
			if ((!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) &&
			    see_with_senseall(mtmp)) {
			Strcat(monbuf, "omnisense");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
			}
		    if (see_with_earthsense(mtmp)) {
			Strcat(monbuf, "earthsense");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
			}
		    if (sense_by_scent(mtmp)) {
			Strcat(monbuf, "scent");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (tp_sensemon(mtmp)) {
			Strcat(monbuf, "telepathy");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (rlyehian_sensemon(mtmp)) {
			Strcat(monbuf, "r'lyehian sight");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (useemon && xraydist > 0 &&
			    distu(mtmp->mx, mtmp->my) <= xraydist) {
			/* Eye of the Overworld */
			Strcat(monbuf, "astral vision");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (Detect_monsters) {
			Strcat(monbuf, "monster detection");
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (MATCH_WARN_OF_MON(mtmp)) {
		    	char wbuf[BUFSZ];
			if (do_halu){
				Strcat(monbuf, "paranoid delusion");
				if (ways_seen-- > 1) Strcat(monbuf, ", ");
			} else {
				//going to need more detail about how it is seen....
				ways_seen = 0;
				if(u.sealsActive&SEAL_PAIMON && is_magical(mdat)) ways_seen++;
				if(u.sealsActive&SEAL_ANDROMALIUS && is_thief(mdat)) ways_seen++;
				if(u.sealsActive&SEAL_TENEBROUS && !nonliving(mdat)) ways_seen++;
				if(u.specialSealsActive&SEAL_ACERERAK && is_undead(mdat)) ways_seen++;
				if(uwep && ((uwep->oward & WARD_THJOFASTAFUR) && 
					(mdat->mlet == S_LEPRECHAUN || mdat->mlet == S_NYMPH || is_thief(mdat)))) ways_seen++;
				if(youracedata->mtyp == PM_SHARK && has_blood_mon(mtmp) &&
						(mtmp)->mhp < (mtmp)->mhpmax && is_pool(u.ux, u.uy, TRUE) && is_pool((mtmp)->mx, (mtmp)->my, TRUE)) ways_seen++;
				if(MATCH_WARN_OF_MON(mtmp)){
					Sprintf(wbuf, "warned of ");
					warned_monster_reasons(mtmp, wbuf);
					Strcat(monbuf, wbuf);
				}
		    }
			}
		}
	    }
		if(!do_halu){
			if(mdat->msize == MZ_TINY) Sprintf(shapebuff, "a tiny");
			else if(mdat->msize == MZ_SMALL) Sprintf(shapebuff, "a small");
			else if(mdat->msize == MZ_MEDIUM) Sprintf(shapebuff, "a medium");
			else if(mdat->msize == MZ_LARGE) Sprintf(shapebuff, "a large");
			else if(mdat->msize == MZ_HUGE) Sprintf(shapebuff, "a huge");
			else if(mdat->msize == MZ_GIGANTIC) Sprintf(shapebuff, "a gigantic");
			else Sprintf(shapebuff, "an odd-sized");
			
			if((mdat->mflagsb&MB_HEADMODIMASK) == MB_LONGHEAD) Strcat(shapebuff, " snouted");
			else if((mdat->mflagsb&MB_HEADMODIMASK) == MB_LONGNECK) Strcat(shapebuff, " long-necked");
			
			if((mdat->mflagsb&MB_BODYTYPEMASK) == MB_ANIMAL) Strcat(shapebuff, " animal");
			else if((mdat->mflagsb&MB_BODYTYPEMASK) == MB_SLITHY) Strcat(shapebuff, " ophidian");
			else if((mdat->mflagsb&MB_BODYTYPEMASK) == MB_HUMANOID) Strcat(shapebuff, " humanoid");
			else if((mdat->mflagsb&MB_BODYTYPEMASK) == (MB_HUMANOID|MB_ANIMAL)) Strcat(shapebuff, " centauroid");
			else if((mdat->mflagsb&MB_BODYTYPEMASK) == (MB_HUMANOID|MB_SLITHY)) Strcat(shapebuff, " snake-legged humanoid");
			else if((mdat->mflagsb&MB_BODYTYPEMASK) == (MB_ANIMAL|MB_SLITHY)) Strcat(shapebuff, " snake-backed animal");
			else Strcat(shapebuff, " thing");
		} else {
			Sprintf(shapebuff, "%s%s%s", an(sizeStr[rn2(SIZE(sizeStr))]), headStr[rn2(SIZE(headStr))], bodyStr[rn2(SIZE(bodyStr))]);
		}
	}
    } else if glyph_is_cloud(glyph) {
	Sprintf(buf, "%s cloud", get_description_of_damage_type(glyph_to_cloud_type(glyph)));
    } else if (glyph_is_object(glyph)) {
	struct obj *otmp = 0;
	boolean fakeobj = object_from_map(glyph, x, y, &otmp);

	if (otmp) {
	    Strcpy(buf, (otmp->otyp != STRANGE_OBJECT)
		    ? distant_name(otmp, xname)
		    : obj_descr[STRANGE_OBJECT].oc_name);
	    if (fakeobj)
		dealloc_obj(otmp), otmp = 0;
        }
	if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR)
	    Strcat(buf, " embedded in stone");
	else if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
	    Strcat(buf, " embedded in a wall");
	else if (closed_door(x,y))
	    Strcat(buf, " embedded in a door");
	else if (is_pool(x,y, FALSE))
	    Strcat(buf, " in water");
	else if (is_lava(x,y))
	    Strcat(buf, " in molten lava");     /* [can this ever happen?] */
    } else if (glyph_is_trap(glyph)) {
	int tnum = what_trap(glyph_to_trap(glyph));
	Strcpy(buf, defsyms[trap_to_defsym(tnum)].explanation);
    } else if(!glyph_is_cmap(glyph)) {
	Strcpy(buf,"dark part of a room");
    } else switch(glyph_to_cmap(glyph)) {
    case S_altar:
	if(!In_endgame(&u.uz))
	    Sprintf(buf, "%s altar", align_str(a_align(x, y)));
	else Sprintf(buf, "aligned altar");
	break;
    case S_ndoor:
	if (is_drawbridge_wall(x, y) >= 0)
	    Strcpy(buf,"open drawbridge portcullis");
	else if ((levl[x][y].doormask & ~D_TRAPPED) == D_BROKEN)
	    Strcpy(buf,"broken door");
	else
	    Strcpy(buf,"doorway");
	break;
    case S_cloud:
    case S_fog:
	Strcpy(buf, Is_airlevel(&u.uz) ? "cloudy area" : "fog/vapor cloud");
	break;
    case S_dust:
	Strcpy(buf, "dust storm");
	break;
	//Lethe patch by way of Slashem
    case S_water:
    case S_pool:
	Strcpy(buf, level.flags.lethe? "sparkling water" : "water");
	break;
    default:
	Strcpy(buf,defsyms[glyph_to_cmap(glyph)].explanation);
	break;
    }

    return ((mtmp && !do_halu) ? mtmp : (struct monst *) 0);
}

/*
 * Look in the "data" file for more info.  Called if the user typed in the
 * whole name (user_typed_name == TRUE), or we've found a possible match
 * with a character/glyph and flags.help is TRUE.
 *
 * NOTE: when (user_typed_name == FALSE), inp is considered read-only and
 *	 must not be changed directly, e.g. via lcase(). We want to force
 *	 lcase() for data.base lookup so that we can have a clean key.
 *	 Therefore, we create a copy of inp _just_ for data.base lookup.
 * 
 * Returns TRUE if it found an entry and printed to the nhwindow
 */
boolean
checkfile(inp, pm, user_typed_name, without_asking, printwindow)
    char *inp;
    struct permonst *pm;
    boolean user_typed_name, without_asking;
	winid *printwindow;
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ];
    char *ep, *dbase_str;
    long txt_offset;
    int chk_skip;
    boolean found_in_file = FALSE, skipping_entry = FALSE, wrote = FALSE;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
	pline("Cannot open data file!");
	return FALSE;
    }

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    if (pm != (struct permonst *) 0 && !user_typed_name)
	dbase_str = strcpy(newstr, pm->mname);
    else dbase_str = strcpy(newstr, inp);
    (void) lcase(dbase_str);

    if (!strncmp(dbase_str, "interior of ", 12))
	dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
	dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
	dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
	dbase_str += 4;
    if (!strncmp(dbase_str, "tame ", 5))
	dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
	dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
	dbase_str += 10;
    if (!strncmp(dbase_str, "statue of ", 10))
	dbase_str[6] = '\0';
    else if (!strncmp(dbase_str, "figurine of ", 12))
	dbase_str[8] = '\0';

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
	/* adjust the input to remove "named " and convert to lower case */
	char *alt = 0;	/* alternate description */

	if ((ep = strstri(dbase_str, " named ")) != 0)
	    alt = ep + 7;
	else
	    ep = strstri(dbase_str, " called ");
	if (!ep) ep = strstri(dbase_str, ", ");
	if (ep && ep > dbase_str) *ep = '\0';

	/*
	 * If the object is named, then the name is the alternate description;
	 * otherwise, the result of makesingular() applied to the name is. This
	 * isn't strictly optimal, but named objects of interest to the user
	 * will usually be found under their name, rather than under their
	 * object type, so looking for a singular form is pointless.
	 */

	if (!alt)
	    alt = makesingular(dbase_str);
	else
	    if (user_typed_name)
		(void) lcase(alt);

	/* skip first record; read second */
	txt_offset = 0L;
	if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
	    impossible("can't read 'data' file");
	    (void) dlb_fclose(fp);
	    return FALSE;
	} else if (sscanf(buf, "%8lx\n", &txt_offset) < 1 || txt_offset <= 0)
	    goto bad_data_file;

	/* look for the appropriate entry */
	while (dlb_fgets(buf,BUFSZ,fp)) {
	    if (*buf == '.') break;  /* we passed last entry without success */

	    if (digit(*buf)) {
		/* a number indicates the end of current entry */
		skipping_entry = FALSE;
	    } else if (!skipping_entry) {
		if (!(ep = index(buf, '\n'))) goto bad_data_file;
		*ep = 0;
		/* if we match a key that begins with "~", skip this entry */
		chk_skip = (*buf == '~') ? 1 : 0;
		if (pmatch(&buf[chk_skip], dbase_str) ||
			(alt && pmatch(&buf[chk_skip], alt))) {
		    if (chk_skip) {
			skipping_entry = TRUE;
			continue;
		    } else {
			found_in_file = TRUE;
			break;
		    }
		}
	    }
	}
    }

    if(found_in_file) {
	long entry_offset;
	int  entry_count;
	int  i;

	/* skip over other possible matches for the info */
	do {
	    if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
	} while (!digit(*buf));
	if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2) {
bad_data_file:	impossible("'data' file in wrong format");
		(void) dlb_fclose(fp);
		return FALSE;
	}

	if (user_typed_name || without_asking || yn("More info?") == 'y') {

	    if (dlb_fseek(fp, txt_offset + entry_offset, SEEK_SET) < 0) {
		pline("? Seek error on 'data' file!");
		(void) dlb_fclose(fp);
		return FALSE;
	    }
		char *encyc_header = "Encyclopedia entry:";
		putstr(*printwindow, 0, "\n");
		putstr(*printwindow, 0, encyc_header);
		putstr(*printwindow, 0, "\n");
	    for (i = 0; i < entry_count; i++) {
		if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
		if ((ep = index(buf, '\n')) != 0) *ep = 0;
		if (index(buf+1, '\t') != 0) (void) tabexpand(buf+1);
		putstr(*printwindow, 0, buf + 1);
		wrote = TRUE;
	    }
	}
    } else if (user_typed_name)
	pline("I don't have any information on those things.");

    (void) dlb_fclose(fp);
	return wrote;
}

/* getpos() return values */
#define LOOK_TRADITIONAL	0	/* '.' -- ask about "more info?" */
#define LOOK_QUICK		1	/* ',' -- skip "more info?" */
#define LOOK_ONCE		2	/* ';' -- skip and stop looping */
#define LOOK_VERBOSE		3	/* ':' -- show more info w/o asking */

/* also used by getpos hack in do_name.c */
const char what_is_an_unknown_object[] = "an unknown object";

static const char * const bogusobjects[] = {
       /* Real */
	   "eleven arrow"
	   "ortish arrow",
       "ruined arrow",
       "vulgar arrow",
       "ja",
       "bamboozled sparrow",
       "crossbow colt",
	   "crossbow dolt",
	   "throwing fart",
       "shurunken",
	   "bomberang",
       "elven drear",
       "orcish dear",
       "dwarvish peer",
       "sliver spear",
       "throwing spar",
       "2x3dent", /*Homestuck*/
       "danger",
       "elven danger",
       "orcish danger",
       "sliver danger",
	   "allthame",
       "scalp",
       "stiletto",
	   "silletto",
       "criesknife",
       "old battle-axe",
       "length-challenged sword",
       "re-curved sword",
       "de-curved sword",
       "circular sword",
       "longer sword",
       "three-handed sword",
       "two-handled sword",
       "catana",
       "dunesword",
       "bi-partisan",
       "parisian",
       "sniptem",	/*Order of the Stick*/
       "fave",
       "rancor",
       "lancer",
       "halbeard",
       "snake-eyed bardiche",
       "vouge",
       "dwarvish hassock",
       "fauchard",
       "mysarme",
       "uisarme",
       "bob-guisarme",
       "lucifer's hammer",
       "bec de corwin",
       "yet another poorly-differentiated polearm",
       "cursed YAPDP",
       "can of mace",
       "evening star",
       "dawn star",
       "day star",
       "peace hammer",
       "wooden sandwich",
       "halfstaff",
       "thong club",
       "dire flail",
       "dire fail",
       "trollwhip",
       "partially-eaten yumi",
       "X-bow",
       "conical flat",
       "mithril-boat",
       "barded mail",
       "yet-another-substandard-mail-variant",
       "cursed YASMV",
       "scale-reinforced banded splint mail with chain joints",
       "leather studd armor",
       "white shirt",
       "red shirt",
       "pair of brown pants",
       "undershirt",
       "cope [sic]",
       "cope [with it]",
       "lion skin",
       "lion sin",
       "bear skin robe",
       "bear skin rug",
       "undersized shield",
       "shield of reflection",
       "shield of rarefaction",
       "wide-eyed shield",
       "pair of padding gloves",
       "pair of rideable gloves",
       "pair of feching gloves",
       "pair of wandering walking shoes",
       "pair of walkabout shoes",
       "pair of hardly shoes",
       "pair of jackboots",
       "pair of combat boats",
       "pair of hiking boots",
       "pair of wild hiking boots",
       "pair of muddy boots",
       "gnomerang",
       "gnagger",
       "hipospray ampule",
	   
	   "can of Greece",
	   "can of crease",
	   "can of Grease",
	   "can of greaser",
	   
	   "cursed boomerang axe named \"The Axe of Dungeon Collapse\"",
		
       "blessed greased +5 silly object of hilarity",
	   "kinda lame joke",
	   
	   "brazier of commanding fire elementals",
	   "brassiere of commanding fire elementals",
	   
	   "bowl of commanding water elementals",
	   "bowel of commanding water elementals",
	   
	   "censer of controlling air elementals",
	   "censure of controlling air elementals",
	   
	   "stone of controlling earth elementals",
	   "loan of controlling earth elementals",
	   
# ifdef TOURIST
       /* Modern */
       "polo mallet",
       "string vest",
       "applied theology textbook",        /* AFutD */
       "handbag",
       "onion ring",
       "tuxedo",
       "breath mint",
       "potion of antacid",
       "potion of wokitoff", /* stjepan sejic */
       "potion of hull feeling",
       "traffic cone",
       "chainsaw",
//	   "pair of high-heeled stilettos",    /* the *other* stiletto */
	   "high-heeled stiletto",
	   "comic book",
	   "lipstick",
       "dinner-jacket",
       "three-piece suit",

       /* Silly */
       "left-handed iron chain",
       "holy hand grenade",                /* Monty Python */
       "decoder ring",
       "amulet of huge gold chains",       /* Foo' */
       "rubber Marduk",
       "unicron horn",                     /* Transformers */
       "holy grail",                       /* Monty Python */
       "chainmail bikini",
	   "leather shorts",
       "first class one-way ticket to Albuquerque", /* Weird Al */
       "yellow spandex dragon scale mail", /* X-Men */

       /* Musical Instruments */
       "grand piano",
       "set of two slightly sampled electric eels", /* Oldfield */
       "kick drum",                        /* 303 */
       "tooled airhorn",

       /* Pop Culture */
       "flux capacitor",                   /* BTTF */
       "Walther PPK",                      /* Bond */
       "hanging chad",                     /* US Election 2000 */
       "99 red balloons",                  /* 80s */
       "pincers of peril",                 /* Goonies */
       "ring of schwartz",                 /* Spaceballs */
       "signed copy of Diaspora",          /* Greg Egan */
       "file containing the missing evidence in the Kelner case", /* Naked Gun */
       "blessed 9 helm of Des Lynam",     /* Bottom */
	   "oscillation overthruster",
	   "magic device",
	   
	   "Infinity Gauntlet",

        /* Geekery */
       "AAA chipset",                      /* Amiga */
       "thoroughly used copy of Nethack for Dummies",
       "named pipe",                       /* UNIX */
       "kernel trap",
       "copy of nethack 3.4.4",            /* recursion... */
       "YAFM", "YAAP", "YANI", "YAAD", "YASD", "YAFAP", "malevolent RNG", /* rgrn */
       "cursed smooth manifold",           /* Topology */
       "vi clone",
       "maximally subsentient emacs mode",
       "bongard diagram",                  /* Intelligence test */
	   "pair of Joo Janta 200 Super-Chromatic Peril Sensitive Sunglasses",	/* K2 */

	   "smooth-gemmed head-ch102020202020202020202020202020202020202020202",	/* my cat says "hello" */
	   
	   /* Movies etc. */
	   "overloading phaser", /* Star Trek */
	   "thermal detinator", /* Star Wars */
       "thing that is not tea",/*"no tea here!", "no tea, sadly",*/ /* HGttG */
		"potion almost, but not quite, entirely unlike tea",
		"potion of Pan-Galactic Gargle Blaster", "black scroll-case labeled DON'T PANIC", /* HGttG */
	   "ridiculously dangerous epaulet [it's armed]", /* Schlock Mercenary*/
	   "pokeball", /* Pokemon */
	   "scroll of oxygen-destroyer plans", /* Godzilla */
	   "hologram of Death Star plans",
	   "sonic screwdriver", /* Dr. Who */

	   /* British goodness */
       "bum bag",
       "blessed tin of marmite",
       "tesco value potion",
       "ringtone of drawbridge opening",
       "burberry cap",
       "potion of bitter",
       "cursed -2 bargain plane ticket to Ibiza",
       "black pudding corpse",
# endif
       /* Fantasy */
       "leaf of pipe weed",                /* LOTR */
       "knife missile",                    /* Iain M. Banks */
       "large gem",                        /* Valhalla */
       "ring of power",                    /* LOTR */
       "silmaril",                         /* LOTR */
       "pentagram of protection",          /* Quake */
	   "crown of swords",					/* Wheel of Time */
	   
	   /* Interwebs */
	   "memetic kill agent",
		"global retrocausality torus",		/* SCP Foundation */
	   "bottle of squid pro quo ink",		/* MSPA */
		"highly indulgent self-insert",
	   "cursed -1 phillips head",			/* xkcd nethack joke */
	   
	   /* Mythology */
	   "sampo",
	   "shamir",
	   "golden fleece",
	   "gold apple labeled \"For the Fairest\"",
	   "Holy Grail",
	   "Ark of the Covenant",
	   
	   /* Other Games */
	   "Chaos Emerald",
	   "bronze sphere",
	   "modron cube",
	   
	   /* Books */
       "monster manual",                   /* D&D */
       "monster book of monsters",         /* Harry Potter */
	   "Codex of the Infinite Planes",	   /* DnD */
	   "spellbook of Non-Conduit Transdimensional Fabric Fluxes and Real-Time Inter-dimensional Matrix Transformations", /* Maldin's Greyhawk */
		   "spellbook named the Codex of Betrayal",
		   "spellbook named the Demonomicon of Iggwilv",
		   "spellbook named the Book of Keeping",
		   "Black Scroll of Ahm",
	   "Elder Scroll", /*the Elder Scrolls*/
       "spellbook named The Ta'ge Fragments", /* Cthulhutech */
       "spellbook named Tome of Eternal Darkness", /* Eternal Darkness */
       "history book called A Chronicle of the Daevas", /* SCP Foundation */
       "spellbook named The Book of Sand",                     /* Jorge Luis Borges */
       "spellbook named Octavo",       	   /* Discworld */
		"spellbook called Necrotelicomnicon",
	   "lost copy of The Nice and Accurate Prophecies of Anges Nutter", /* Good Omens */
	   "spellbook of Addition Made Simple",
	   "spellbook of Noncommutative Hyperdimensional Geometry Made (Merely!) Complicated",
	   "heavily obfuscated spellbook",
	   "scroll of abstruse logic",
			"scroll of identity", /*deepy*/
			"scroll of cure blindness",
	   "spellbook of sub-formal introtransreductive logic [for Dummies!]",
	   "spellbook named A Brief History of Time",
	   "spellbook named The Book of Eibon", 					/* Clark Ashton Smith */
	   "playbook named The King in Yellow", 					/* Robert W. Chambers */
	   "playbook called the Scottish play", 					/* aka Macbeth */
	   "spellbook named De Vermis Mysteriis",					/* Robert Bloch */
	   "HANDBOOK FOR THE IMMINENTLY DECEASED named ~ATH",		/* Homestuck */
	   "spellbook named Necronomicon",							/* HP Lovecraft (yes, I know there is a game artifact of this name) */
	   "spellbook named Al Azif",								/* "Arabic" name for the Necronomicon, HP Lovecraft */
	   "spellbook named Unaussprechlichen Kulten",				/*  Robert E. Howard; Lovecraft and Derleth */
	   "spellbook called Nameless Cults",						/*  (the original name) */
	   "spellbook called Unspeakable Cults",					/*  One of the two things "Unaussprechlichen" translates to */
	   "spellbook called Unpronouncable Cults",					/*  The other thing "Unaussprechlichen" translates to */
	   "spellbook named The Diary of Drenicus the Wise",		/*  Dicefreaks, The Gates of Hell */
	   "spellbook named Clavicula Salomonis Regis",				/* ie, The Lesser Key of Solomon */
	   "copy of The Five Books of Moses",						/* aka the Torah */
	   "spellbook named The Six and Seventh Books of Moses",	/* 18th- or 19th-century magical text allegedly written by Moses */
	   "spellbook named The Book of Coming Forth by Day", "spellbook named The Book of emerging forth into the Light",
	   "spellbook named Sepher Ha-Razim",						/*  Book given to Noah by the angel Raziel */
	   "spellbook named Sefer Raziel HaMalakh", 				/* Book of Raziel the Angel, given to Adam */
	   "spellbook named The Testament of Solomon",
	   "spellbook named The Book of Enoch",
	   "spellbook named The Book of Inverted Darkness",
	   "spellbook named The Uruk Tablets",
	   "spellbook named The Book of the Law",
	   "spellbook named the Book of Shadows",
	   "spellbook named the Book of Mirrors",
	   "book called the Gospel of the Thricefold Exile",
       "dead sea scroll",
	   "mayan codex",
	   "simple textbook on the zoologically dubious",			/* Homestuck */
	   "spellbook named The Book of Night with Moon",			/* So you want to be a Wizard? */
	   "spellbook named The Grimmerie",			/* Wicked */
	   "book of horrors", /* MLP */
	   "book of Remembrance and Forgetting", /* the Old Kingdom */

       /* Historical */
	   "Amarna letter",
	   "lost ark of the covenant",
       "cat o'nine tails",
       "piece of eight",
       "codpiece",
       "straight-jacket",
       "bayonet",
       "iron maiden",
       "oubliette",
       "pestle and mortar",
       "plowshare",
	   "petard",
	   "caltrop",

       /* Mashups */
       "potion of rebigulation",           /* Simpsons */
       "potion of score doubling",
	   "potion of bad breath",
	   "potion of fire breathing",
	   "potion of intoxication",
	   "potion of immorality",
	   "potion of immoratlity",
	   "potion of gain divinity",
       "scroll labeled ED AWK YACC",      /* the standard scroll */
       "scroll labeled RTFM",
       "scroll labeled KLAATU BARADA NIKTO", /* Evil Dead 3 */
       "scroll of omniscience",
       "scroll of mash keyboard",
       "scroll of RNG taming",
       "scroll of fungicide",
	   "scroll of apocalypse",
	   "scroll of profits",
	   "scroll of stupidity",
	   "spellbook of firebear",
	   "spellbook of dip",
	   "spellbook of cone of cord",
       "helm of telemetry",
       "helm of head-butting",
       "pair of blue suede boots of charisma",
	   "pair of boots of rug-cutting",
	   "pair of cursed boots of mazing",
       "pair of wisdom boots",
       "cubic zirconium",
       "amulet of instadeath",
       "amulet of bad luck",
       "amulet of refraction",
       "amulet of deflection",
       "amulet of rarefaction",
       "amulet versus YASD",
       "O-ring",
       "ring named Frost Band",
       "expensive exact replica of the Amulet of Yendor",
       "giant beatle",
       "lodestone",
       "rubber chicken",                   /* c corpse */
       "figurine of a god",
       "tin of Player meat",
       "tin of whoop ass",
	   "ragnarok horn",
       "cursed -3 earring of adornment",
       "ornamental cape",
       "acid blob skeleton",
       "wand of washing",
	   "wand of intoxication",
	   "wand of vaporization",
	   "wand of disruption",
	   "wand of transubstantiation",
	   "wand of disintegration",
	   "wand of renewal",
	   
	   "little piece of home",

# ifdef MAIL
       "brand new, all time lowest introductory rate special offer",
       "tin of spam",
# endif
       "dirty rag"
};

/* Return a random bogus object name, for hallucination */
const char *
rndobjnam()
{
    int name;
	if(!rn2(3)){
		name = rn2(SIZE(bogusobjects));
		return bogusobjects[name];
//	} else if(!rn2(2)){
//		name = rn2(TIN);
//		return OBJ_DESCR(objects[name]);
	} else{
		name = rn2(TIN);
		return OBJ_NAME(objects[name]);
	}
}


STATIC_OVL int
do_look(quick)
    boolean quick;	/* use cursor && don't search for "more info" */
{
	char out_str[LONGBUFSZ];
	out_str[0] = 0;
    const char *x_str = 0;
	const char firstmatch[BUFSZ] = {0};
    struct permonst *pm = 0;
    glyph_t sym;		/* typed symbol or converted glyph */
    boolean from_screen;	/* question from the screen */
	short otyp = STRANGE_OBJECT;	/* to pass to artifact_name */
	int oartifact;					/* to pass to artifact_name */
	int i, ans = 0;
	coord   cc;			/* screen pos of unknown glyph */
	boolean force_defsyms = FALSE;	/* force using glyphs from defsyms[].sym */
	boolean save_verbose = flags.verbose;

    if (quick) {
		from_screen = TRUE;	/* yes, we want to use the cursor */
    } else {
		i = ynq("Specify unknown object by cursor?");
		if (i == 'q') return MOVE_CANCELLED;
		from_screen = (i == 'y');
    }

    if (from_screen) {
		sym = 0;
	} else {
		getlin("Specify what? (type the word)", out_str);
		if (out_str[0] == '\0' || out_str[0] == '\033')
			return MOVE_CANCELLED;

		if (out_str[1]) {	/* user typed in a complete string */
			winid datawin = create_nhwindow(NHW_MENU);
			// check if they specified a monster
			int mntmp = NON_PM;
			if ((mntmp = name_to_mon(out_str)) >= LOW_PM && !is_horror(&mons[mntmp])) {
				char temp_buf[LONGBUFSZ];
				strcat(out_str, "\n");
				temp_buf[0] = '\0';
				/* create a temporary mtmp to describe */
				struct monst fake_mon = {0};
				set_mon_data(&fake_mon, mntmp);
				get_description_of_monster_type(&fake_mon, temp_buf);
				(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
				pm = &mons[mntmp];
				char * temp_print;
				temp_print = strtok(out_str, "\n");
				while (temp_print != NULL)
				{
					putstr(datawin, 0, temp_print);
					temp_print = strtok(NULL, "\n");
				}
			}
			// check if they specified an artifact
			else if ((x_str = artifact_name(out_str, &otyp, &oartifact))) {
				putstr(datawin, 0, x_str);
				describe_item(NULL, otyp, oartifact, &datawin);
			}
			// check encyclopedia
			if(checkfile(out_str, pm, (mntmp==NON_PM && otyp==STRANGE_OBJECT), TRUE, &datawin) || mntmp != NON_PM || otyp != STRANGE_OBJECT)
				display_nhwindow(datawin, TRUE);
			destroy_nhwindow(datawin);
			return MOVE_CANCELLED;
		}
		sym = out_str[0];
    }

	if (from_screen) {
		cc.x = u.ux;
		cc.y = u.uy;
	    int glyph;	/* glyph at selected position */

	    if (flags.verbose)
		pline("Please move the cursor to %s.",
		       what_is_an_unknown_object);
	    else
		pline("Pick an object.");

	    ans = getpos(&cc, quick, what_is_an_unknown_object);
	    if (ans < 0 || cc.x < 0) {
			return MOVE_CANCELLED;	/* done */
	    }
	    flags.verbose = FALSE;	/* only print long question once */

	    /* Convert the glyph at the selected position to a symbol. */
	    glyph = glyph_at(cc.x,cc.y);
	    if (glyph_is_cmap(glyph)) {
		if (iflags.UTF8graphics) {
			/* Temporary workaround as NetHack can't yet
			 * display UTF-8 glyphs on the topline */
			force_defsyms = TRUE;
			sym = defsyms[glyph_to_cmap(glyph)].sym;
		} else {
		sym = showsyms[glyph_to_cmap(glyph)];
		}
	    } else if (glyph_is_trap(glyph)) {
		sym = showsyms[trap_to_defsym(glyph_to_trap(glyph))];
	    } else if (glyph_is_object(glyph)) {
		sym = oc_syms[(int)objects[glyph_to_obj(glyph)].oc_class];
		if (sym == '`' && iflags.bouldersym && ((int)glyph_to_obj(glyph) == BOULDER || (int)glyph_to_obj(glyph) == MASS_OF_STUFF))
			sym = iflags.bouldersym;
	    } else if (glyph_is_monster(glyph)) {
		/* takes care of pets, detected, ridden, and regular mons */
		sym = monsyms[(int)mons[glyph_to_mon(glyph)].mlet];
	    } else if (glyph_is_cloud(glyph)) {
		sym = showsyms[S_cloud];
	    } else if (glyph_is_swallow(glyph)) {
		sym = showsyms[glyph_to_swallow(glyph)+S_sw_tl];
	    } else if (glyph_is_invisible(glyph)) {
		sym = DEF_INVISIBLE;
	    } else if (glyph_is_warning(glyph)) {
		sym = glyph_to_warning(glyph);
	    	sym = warnsyms[sym];
	    } else {
		impossible("do_look:  bad glyph %d at (%d,%d)",
						glyph, (int)cc.x, (int)cc.y);
		sym = ' ';
	    }
	}

    /*
     * The user typed one letter, or we're identifying from the screen.
     */
	do_look_letter(sym, from_screen, quick, force_defsyms, cc, out_str, firstmatch);
	
	/* Finally, print out our explanation. */
	if (out_str[0]) {
		winid datawin = create_nhwindow(NHW_MENU);
		char * temp_print;
		temp_print = strtok(out_str, "\n");
		while (temp_print != NULL)
		{
			putstr(datawin, 0, temp_print);
			temp_print = strtok(NULL, "\n");
		}
	    /* check the data file for information about this thing */
	    if (ans != LOOK_QUICK && ans != LOOK_ONCE &&
			(ans == LOOK_VERBOSE || (flags.help && !quick))) {
		char temp_buf[BUFSZ];
		Strcpy(temp_buf, level.flags.lethe //lethe
					&& !strcmp(firstmatch, "water")?
				"lethe" : firstmatch);
		(void)checkfile(temp_buf, pm, FALSE, (boolean)(ans == LOOK_VERBOSE), &datawin);
	    }
		display_nhwindow(datawin, TRUE);
		destroy_nhwindow(datawin);
	} else {
	    pline("I've never heard of such things.");
	}
	flags.verbose = save_verbose;
	return MOVE_CANCELLED;
}

char*
do_look_letter(sym, from_screen, quick, force_defsyms, cc, out_str, firstmatch)
	glyph_t sym;
	boolean from_screen;
	boolean quick;
	boolean force_defsyms;
	coord cc;
	char* out_str;
	const char* firstmatch;
{
	char look_buf[BUFSZ];
    const char *x_str = 0;
    struct permonst *pm = 0;
    struct monst *mtmp = 0;
    int     i, ans = 0;
    int	    found;		/* count of matching syms found */
    boolean save_verbose;	/* saved value of flags.verbose */
    boolean need_to_look;	/* need to get explan. from glyph */
    boolean hit_trap;		/* true if found trap explanation */
    boolean hit_cloud;		/* true if found cloud explanation */
    int skipped_venom;		/* non-zero if we ignored "splash of venom" */
	int hallu_obj;		/* non-zero if found hallucinable object */
	short otyp = STRANGE_OBJECT;	/* to pass to artifact_name */
    static const char *mon_interior = "the interior of a monster";

	save_verbose = flags.verbose;
    flags.verbose = flags.verbose && !quick;

    do {
	/* Reset some variables. */
	need_to_look = FALSE;
	pm = (struct permonst *)0;
	skipped_venom = 0;
	hallu_obj = 0;
	found = 0;
	out_str[0] = '\0';
	/*
	 * Check all the possibilities, saving all explanations in a buffer.
	 * When all have been checked then the string is printed.
	 */

	/* Check for monsters */
	for (i = 0; i < MAXMCLASSES; i++) {
	    if (sym == (from_screen ? monsyms[i] : def_monsyms[i]) &&
		monexplain[i]) {
		need_to_look = TRUE;
		if (!found) {
		    Sprintf(out_str, "%c       %s", (uchar)sym, an(monexplain[i]));
		    firstmatch = monexplain[i];
		    found++;
		} else {
		    found += append_str(out_str, an(monexplain[i]));
		}
	    }
	}
	/* handle '@' as a special case if it refers to you and you're
	   playing a character which isn't normally displayed by that
	   symbol; firstmatch is assumed to already be set for '@' */
	if ((from_screen ?
		(sym == monsyms[S_HUMAN] && cc.x == u.ux && cc.y == u.uy) :
		(sym == def_monsyms[S_HUMAN] && !iflags.showrace)) &&
	    !(Race_if(PM_HUMAN) || Race_if(PM_ELF) || Race_if(PM_DROW) || Race_if(PM_MYRKALFR)) && !Upolyd)
	    found += append_str(out_str, "you");	/* tack on "or you" */

	/*
	 * Special case: if identifying from the screen, and we're swallowed,
	 * and looking at something other than our own symbol, then just say
	 * "the interior of a monster".
	 */
	if (u.uswallow && from_screen && is_swallow_sym(sym)) {
	    if (!found) {
		Sprintf(out_str, "%c       %s", (uchar)sym, mon_interior);
		firstmatch = mon_interior;
	    } else {
		found += append_str(out_str, mon_interior);
	    }
	    need_to_look = TRUE;
	}

	/* Now check for objects */
	for (i = 1; i < MAXOCLASSES; i++) {
	    if (sym == (from_screen ? oc_syms[i] : def_oc_syms[i])) {
		need_to_look = TRUE;
		if (from_screen && i == VENOM_CLASS) {
		    skipped_venom++;
		    continue;
		}
		if (!found) {
		    Sprintf(out_str, "%c       %s", (uchar)sym, an(objexplain[i]));
		    firstmatch = objexplain[i];
		    found++;
			hallu_obj++;
		} else {
		    found += append_str(out_str, an(objexplain[i]));
		}
	    }
	}

	if (sym == DEF_INVISIBLE) {
	    if (!found) {
		Sprintf(out_str, "%c       %s", (uchar)sym, an(invisexplain));
		firstmatch = invisexplain;
		found++;
	    } else {
		found += append_str(out_str, an(invisexplain));
	    }
	}

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_switch)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)

	/* Now check for graphics symbols */
	for (hit_trap = FALSE, hit_cloud = FALSE, i = 0; i < MAXPCHARS; i++) {
	    x_str = defsyms[i].explanation;
	    if (sym == (force_defsyms ? defsyms[i].sym : (from_screen ? showsyms[i] : defsyms[i].sym)) && *x_str) {
		/* avoid "an air", "a water", or "a floor of a room" */
		int article = (i == S_drkroom || i == S_litroom) ? 2 :		/* 2=>"the" */
			      !(strcmp(x_str, "air") == 0 ||	/* 1=>"an"  */
				strcmp(x_str, "shallow water") == 0 || /* 0=>(none)*/
				strcmp(x_str, "water") == 0);

		if (!found) {
		    if (is_cmap_trap(i)) {
			Sprintf(out_str, "%c       a trap", (uchar)sym);
			hit_trap = TRUE;
		    } else if (level.flags.lethe && !strcmp(x_str, "water")) { //Lethe patch
			Sprintf(out_str, "%c       sparkling water", (uchar)sym); //Lethe patch
		    } else if (strstr(x_str, "cloud") != NULL) { //Don't print a bunch of cloud messages
				Sprintf(out_str, "%c       cloud", (uchar)sym);
				hit_cloud = TRUE;
		    } else {
			Sprintf(out_str, "%c       %s", (uchar)sym,
				article == 2 ? the(x_str) :
				article == 1 ? an(x_str) : x_str);
		    }
		    firstmatch = x_str;
		    found++;
		} else if (!u.uswallow && !(hit_trap && is_cmap_trap(i)) && 
				!(hit_cloud && (strstr(x_str, "cloud") != NULL)) &&
			   !(found >= 3 && is_cmap_drawbridge(i))
		) {
		    if (level.flags.lethe && !strcmp(x_str, "water")) //lethe
				found += append_str(out_str, "sparkling water"); //lethe
		    else if (strstr(x_str, "cloud") != NULL){ //cloudspam
				found += append_str(out_str, "cloud"); //cloudspam
				hit_cloud = TRUE;
			}
		    else //lethe
		    	found += append_str(out_str,
					article == 2 ? the(x_str) :
					article == 1 ? an(x_str) : x_str);
		    if (is_cmap_trap(i)) hit_trap = TRUE;
		}

		if (i == S_altar || is_cmap_trap(i))
		    need_to_look = TRUE;
	    }
	}

	/* Now check for warning symbols */
	for (i = 1; i < WARNCOUNT; i++) {
	    x_str = def_warnsyms[i].explanation;
	    if (sym == (from_screen ? warnsyms[i] : def_warnsyms[i].sym)) {
		if (!found) {
			Sprintf(out_str, "%c       %s",
				(uchar)sym, def_warnsyms[i].explanation);
			firstmatch = def_warnsyms[i].explanation;
			found++;
		} else {
			found += append_str(out_str, def_warnsyms[i].explanation);
		}
		/* Kludge: warning trumps boulders on the display.
		   Reveal the boulder too or player can get confused */
		if (from_screen && boulder_at(cc.x, cc.y))
			Strcat(out_str, " co-located with a large object");
		break;	/* out of for loop*/
	    }
	}
    
	/* if we ignored venom and list turned out to be short, put it back */
	if (skipped_venom && found < 2) {
	    x_str = objexplain[VENOM_CLASS];
	    if (!found) {
		Sprintf(out_str, "%c       %s", (uchar)sym, an(x_str));
		firstmatch = x_str;
		found++;
	    } else {
		found += append_str(out_str, an(x_str));
	    }
	}

	/* handle optional boulder symbol as a special case */ 
	if (iflags.bouldersym && sym == iflags.bouldersym) {
	    if (!found) {
		firstmatch = "boulder";
		Sprintf(out_str, "%c       %s", (uchar)sym, an(firstmatch));
		found++;
	    } else {
		found += append_str(out_str, "boulder");
	    }
	}
	
	/*
	 * If we are looking at the screen, follow multiple possibilities or
	 * an ambiguous explanation by something more detailed.
	 */
	if (from_screen) {
	    if (hallu_obj && Hallucination) {
			char temp_buf[LONGBUFSZ];
			Sprintf(temp_buf, " (%s)", rndobjnam());
			(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
	    } else if (found > 1 || need_to_look) {

		char monbuf[LONGBUFSZ];
		char shapebuf[LONGBUFSZ];
		char temp_buf[LONGBUFSZ];

		mtmp = lookat(cc.x, cc.y, look_buf, monbuf, shapebuf);
		firstmatch = look_buf;
		if (*firstmatch) {
			if(shapebuf[0]){
				Sprintf(temp_buf, " (%s", firstmatch);
				(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
				Sprintf(temp_buf, ", %s)", shapebuf);
				(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
				found = 1;	/* we have something to look up */
			}
			else {
				Sprintf(temp_buf, " (%s)", firstmatch);
				(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
				found = 1;	/* we have something to look up */
			}
		}
		if (monbuf[0]) {
		    Sprintf(temp_buf, " [seen: %s]", monbuf);
			(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
		}
		if (mtmp != (struct monst *) 0) {
			strcat(out_str, "\n");
			temp_buf[0] = '\0';
			get_description_of_monster_type(mtmp, temp_buf);
			(void)strncat(out_str, temp_buf, LONGBUFSZ - strlen(out_str) - 1);
			if(uarmh && uarmh->oartifact == ART_ALL_SEEING_EYE_OF_THE_FLY){
				probe_monster(mtmp);
			}
		}
		}
	}

	/* Finally, return our explanation. */
	if (found) {
		flags.verbose = save_verbose;
		return out_str;
	}
    } while (from_screen && !quick && ans != LOOK_ONCE);

	flags.verbose = save_verbose;

    return 0;
}

int
append(char * buf, int condition, char * text, boolean many)
{
	if (condition) {
		if (buf != NULL) {
			if (many) {
				(void)strcat(buf, ", ");
			}
			(void)strcat(buf, text);
		}
		return many + 1;
	}
	return many;
}
int
appendgroup(char * buf, int condition, char * primer, char * text, boolean many, boolean group)
{
	if (condition) {
		if (buf != NULL) {
			if (group) {
				(void)strcat(buf, "/");
			}
			else {
				if (many)
					(void)strcat(buf, ", ");
				(void)strcat(buf, primer);
				(void)strcat(buf, " ");
			}
			(void)strcat(buf, text);
		}
		return group + 1;
	}
	return group;
}

int
generate_list_of_resistances(struct monst * mtmp, char * temp_buf, int resists)
{
	unsigned int mr_flags;
	unsigned long mg_flags = mtmp->data->mflagsg;
	if (resists == 1){
		mr_flags = mtmp->data->mresists;
	}
	if (resists == 0)
		mr_flags = mtmp->data->mconveys;
	int many = 0;
	many = append(temp_buf, (mr_flags & MR_FIRE), "fire", many);
	many = append(temp_buf, (mr_flags & MR_COLD), "cold", many);
	many = append(temp_buf, (mr_flags & MR_SLEEP), "sleep", many);
	many = append(temp_buf, (mr_flags & MR_DISINT), "disintegration", many);
	many = append(temp_buf, (mr_flags & MR_ELEC), "electricity", many);
	many = append(temp_buf, (mr_flags & MR_POISON), "poison", many);
	many = append(temp_buf, (mr_flags & MR_ACID), "acid", many);
	many = append(temp_buf, (mr_flags & MR_STONE), "petrification", many);
	many = append(temp_buf, (mr_flags & MR_DRAIN), "level drain", many);
	many = append(temp_buf, (mr_flags & MR_SICK), "sickness", many);
	many = append(temp_buf, (mr_flags & MR_MAGIC), "magic", many);
	many = append(temp_buf, (((mg_flags & MG_WRESIST) != 0L) && resists == 1), "weapon attacks", many);
	many = append(temp_buf, (((mg_flags & MG_RBLUNT ) != 0L) && resists == 1), "blunt", many);
	many = append(temp_buf, (((mg_flags & MG_RSLASH ) != 0L) && resists == 1), "slashing", many);
	many = append(temp_buf, (((mg_flags & MG_RPIERCE) != 0L) && resists == 1), "piercing", many);
	//many = append(temp_buf, ((mg_flags & MG_RALL) == MG_RALL), "blunt & slashing & piercing", many);
	return many;
}

char *
get_generation_description_of_monster_type(struct monst * mtmp, char * temp_buf)
{
	struct permonst * ptr = mtmp->data;
	int many = 0;

	many = append(temp_buf, !(ptr->geno & G_NOGEN), "Normal generation", many);
	many = append(temp_buf, (ptr->geno & G_NOGEN), "Special generation", many);
	many = append(temp_buf, (ptr->geno & G_UNIQ), "unique", many);
	many = 0;
	many = append(temp_buf, (ptr->geno & G_SGROUP), " in groups", many);
	many = append(temp_buf, (ptr->geno & G_LGROUP), " in large groups", many);
	if ((ptr->geno & G_NOGEN) == 0) {
		char frequency[BUFSZ] = "";
		sprintf(frequency, ", with frequency %ld.", (ptr->geno & G_FREQ));
		strcat(temp_buf, frequency);
	}
	else {
		strcat(temp_buf, ".");
	}
	return temp_buf;
}

char *
get_weight_description_of_monster_type(struct monst * mtmp, char * temp_buf)
{
	struct permonst * ptr = mtmp->data;

	sprintf(temp_buf, "Weight = %d. Nutrition = %d.", ptr->cwt, ptr->cnutrit);

	return temp_buf;
}

char *
get_resistance_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	char temp_buf[BUFSZ] = "";
	temp_buf[0] = '\0';
	int count = generate_list_of_resistances(mtmp, temp_buf, 1);

	if (species_reflects(mtmp))
		strcat(description, "Reflects. ");

	if (count == 0) {
		strcat(description, "No resistances.");
	}
	else {
		strcat(description, "Resists ");
		strcat(description, temp_buf);
		strcat(description, ".");
	}
	return description;
}

char *
get_weakness_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	char temp_buf[BUFSZ] = "";
	temp_buf[0] = '\0';
	int many = 0;

	many = append(temp_buf, hates_holy_mon(mtmp)	, "holy"		, many);
	many = append(temp_buf, hates_unholy_mon(mtmp)	, "unholy"		, many);
	many = append(temp_buf, hates_unblessed_mon(mtmp), "uncursed"	, many);
	many = append(temp_buf, hates_silver(ptr)		, "silver"		, many);
	many = append(temp_buf, hates_iron(ptr)			, "iron"		, many);

	if (many) {
		strcat(description, "Weak to ");
		strcat(description, temp_buf);
		strcat(description, ". ");
	}
	return description;
}

char *
get_conveys_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	char temp_buf[BUFSZ] = "";
	temp_buf[0] = '\0';
	int count = generate_list_of_resistances(mtmp, temp_buf, 0);
	if ((ptr->geno & G_NOCORPSE) != 0) {
		strcat(description, "Leaves no corpse. ");
	}
	else if (count == 0) {
		strcat(description, "Corpse conveys no resistances. ");
	}
	else {
		strcat(description, "Corpse conveys ");
		strcat(description, temp_buf);
		if (count == 1) {
			strcat(description, " resistance. ");
		}
		else {
			strcat(description, " resistances. ");
		}
	}

	return description;
}

char *
get_mm_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Movement: ");
	int many = 0;
	many = append(description, notonline(ptr)			, "avoids you"			, many);
	many = append(description, fleetflee(ptr)			, "flees"				, many);
	many = append(description, species_flies(ptr)		, "flies"				, many);
	many = append(description, species_floats(ptr)		, "floats"				, many);
	many = append(description, is_clinger(ptr)			, "clings to ceilings"	, many);
	many = append(description, species_swims(ptr)		, "swims"				, many);
	many = append(description, breathless_mon(mtmp)		, "is breathless"		, many);
	many = append(description, amphibious(ptr)			, (ptr->mflagsm&MM_AQUATIC) ? "lives underwater" : "survives underwater"	, many);
	many = append(description, species_passes_walls(ptr), "phases"				, many);
	many = append(description, amorphous(ptr)			, "squeezes in gaps"	, many);
	many = append(description, tunnels(ptr)				, "tunnels"				, many);
	many = append(description, needspick(ptr)			, "digs"				, many);
	many = append(description, species_tears_webs(ptr)	, "rips webs"			, many);
	many = append(description, species_teleports(ptr), "teleports"			, many);
	many = append(description, species_controls_teleports(ptr)	, "controls teleports"	, many);
	many = append(description, mteleport(ptr)			, "teleports often"		, many);
	many = append(description, stationary_mon(mtmp)		, "stationary"			, many);
	many = append(description, (many==0)				, "moves normally"		, many);
	strcat(description, ". ");
	return description;
}

char *
get_mt_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Thinking: ");
	int many = 0;
	int eats = 0;
	int likes = 0;
	int wants = 0;

	many = append(description, bold(ptr)				, "fearless"					, many);
	many = append(description, hides_under(ptr)			, "hides"						, many);
	many = append(description, is_hider(ptr)			, "camouflaged"					, many);
	many = append(description, notake(ptr)				, "doesn't pick up items"		, many);
	many = append(description, mindless_mon(mtmp)		, "mindless"					, many);
	many = append(description, detached_from_purpose_mon(mtmp), "detached from original purpose", many);
	many = append(description, is_animal(ptr)			, "animal minded"				, many);
	eats = appendgroup(description, carnivorous(ptr)	, "eats",	"meat"				, many, eats);
	eats = appendgroup(description, herbivorous(ptr)	, "eats",	"veggies"			, many, eats);
	eats = appendgroup(description, metallivorous(ptr)	, "eats",	"metal"				, many, eats);
	many = appendgroup(description, magivorous(ptr)		, "eats",	"magic"				, many, eats) + many;
	many = append(description, is_domestic(ptr)			, "domestic"					, many);
	many = append(description, is_wanderer(ptr)			, "wanders"						, many);
	many = append(description, always_hostile_mon(mtmp)	, "usually hostile"				, many);
	many = append(description, always_peaceful(ptr)		, "usually peaceful"			, many);
	many = append(description, throws_rocks(ptr)		, "throws rocks"				, many);
	likes= appendgroup(description, likes_gold(ptr)		, "likes",	"gold"				, many, likes);
	likes= appendgroup(description, likes_gems(ptr)		, "likes",	"gems"				, many, likes);
	likes= appendgroup(description, likes_objs(ptr)		, "likes",	"equipment"			, many, likes);
	many = appendgroup(description, likes_magic(ptr)	, "likes",	"magic items"		, many, likes) + many;
	wants= appendgroup(description, wants_bell(ptr)		, "wants",	"the bell"				, many, wants);
	wants= appendgroup(description, wants_book(ptr)		, "wants",	"the book"				, many, wants);
	wants= appendgroup(description, wants_cand(ptr)		, "wants",	"the candalabrum"		, many, wants);
	wants= appendgroup(description, wants_qart(ptr)		, "wants",	"your quest artifact"	, many, wants);
	many = appendgroup(description, wants_amul(ptr)		, "wants",	"the amulet"			, many, wants) + many;
	many = append(description, can_betray(ptr)			, "traitorous"					, many);
	many = append(description, (many==0)				, "nothing special"				, many);
	strcat(description, ". ");
	return description;
}
char *
get_mb_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Body: ");
	int many = 0;
	many = append(description, your_race(ptr)			, "same race as you"		, many);
	many = append(description, !haseyes(ptr)			, "eyeless"					, many);
	many = append(description, sensitive_ears(ptr)		, "has sensitive ears"		, many);
	many = append(description, nohands(ptr)				, "can't manipulate objects", many);
	many = append(description, nogloves(ptr)			, "handless"				, many);
	many = append(description, nolimbs(ptr)				, "limbless"				, many);
	many = append(description, !has_head(ptr)			, "headless"				, many);
	many = append(description, has_head(ptr) && nohat(ptr), "can't wear helms"		, many);
	many = append(description, has_horns(ptr)			, "has horns"				, many);
	many = append(description, is_whirly(ptr)			, "whirly"					, many);
	many = append(description, flaming(ptr)				, "flaming"					, many);
	many = append(description, is_stone(ptr)			, "stony"					, many);
	many = append(description, is_anhydrous(ptr)		, "water-less"				, many);
	many = append(description, unsolid(ptr)				, "unsolid"					, many);
	many = append(description, slithy(ptr)				, "slithy"					, many);
	many = append(description, thick_skinned(ptr)		, "thick-skinned"			, many);
	many = append(description, lays_eggs(ptr)			, "oviparous"				, many);
	many = append(description, acidic(ptr)				, "acidic to eat"			, many);
	many = append(description, poisonous(ptr)			, "poisonous to eat"		, many);
	many = append(description, freezing(ptr)			, "freezing to eat"			, many);
	many = append(description, burning(ptr)				, "burning to eat"			, many);
	many = append(description, hallucinogenic(ptr)		, "hallucinogenic to eat"	, many);
	many = append(description, is_deadly(ptr)			, "deadly to eat"			, many);
	many = append(description, is_male(ptr)				, "always male"				, many);
	many = append(description, is_female(ptr)			, "always female"			, many);
	many = append(description, is_neuter(ptr)			, "neuter"					, many);
	many = append(description, strongmonst(ptr)			, "strong"					, many);
	many = append(description, infravisible(ptr)		, "infravisible"			, many);
	many = append(description, humanoid(ptr)			, "humanoid"				, many);
	many = append(description, animaloid(ptr)			, "animaloid"				, many);
	many = append(description, serpentine(ptr)			, "serpent"					, many);
	many = append(description, centauroid(ptr)			, "centauroid"				, many);
	many = append(description, snakemanoid(ptr)			, "human-serpent"			, many);
	many = append(description, leggedserpent(ptr)		, "animal-serpent"			, many);
	many = append(description, (many==0)				, "unknown thing"			, many);
	strcat(description, ". ");
	return description;
}

char *
get_ma_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Race: ");
	int many = 0;
	many = append(description, (ptr->mflagsa & MA_UNDEAD)		, "undead"				, many);
	many = append(description, (ptr->mflagsa & MA_WERE)			, "lycanthrope"			, many);
	many = append(description, (ptr->mflagsa & MA_HUMAN)		, "human"				, many);
	many = append(description, (ptr->mflagsa & MA_ELF)			, "elf"					, many);
	many = append(description, (ptr->mflagsa & MA_DROW)			, "drow"				, many);
	many = append(description, (ptr->mflagsa & MA_DWARF)		, "dwarf"				, many);
	many = append(description, (ptr->mflagsa & MA_GNOME)		, "gnome"				, many);
	many = append(description, (ptr->mflagsa & MA_ORC)			, "orc"					, many);
	many = append(description, (ptr->mflagsa & MA_VAMPIRE)		, "vampire"				, many);
	many = append(description, (ptr->mflagsa & MA_CLOCK)		, "clockwork automaton"	, many);
	many = append(description, (ptr->mflagsa & MA_UNLIVING)		, "not alive"			, many);
	many = append(description, (ptr->mflagsa & MA_PLANT)		, "plant"				, many);
	many = append(description, (ptr->mflagsa & MA_GIANT)		, "giant"				, many);
	many = append(description, (ptr->mflagsa & MA_INSECTOID)	, "insectoid"			, many);
	many = append(description, (ptr->mflagsa & MA_ARACHNID)		, "arachnid"			, many);
	many = append(description, (ptr->mflagsa & MA_AVIAN)		, "avian"				, many);
	many = append(description, (ptr->mflagsa & MA_REPTILIAN)	, "reptilian"			, many);
	many = append(description, (ptr->mflagsa & MA_ANIMAL)		, "mundane animal"		, many);
	many = append(description, (ptr->mflagsa & MA_AQUATIC)		, "water-dweller"		, many);
	many = append(description, (ptr->mflagsa & MA_DEMIHUMAN)	, "demihuman"			, many);
	many = append(description, (ptr->mflagsa & MA_FEY)			, "fey"					, many);
	many = append(description, (ptr->mflagsa & MA_ELEMENTAL)	, "elemental"			, many);
	many = append(description, (ptr->mflagsa & MA_DRAGON)		, "dragon"				, many);
	many = append(description, (ptr->mflagsa & MA_DEMON)		, "demon"				, many);
	many = append(description, (ptr->mflagsa & MA_MINION)		, "minion of a deity"	, many);
	many = append(description, (ptr->mflagsa & MA_PRIMORDIAL)	, "primordial"			, many);
	many = append(description, (ptr->mflagsa & MA_ET)			, "alien"				, many);
	many = append(description, (ptr->mflagsa & MA_G_O_O)		, "great old one"		, many);
	many = append(description, (ptr->mflagsa & MA_XORN)			, "xorn"				, many);
	many = append(description, (many==0)						, "sui generis"			, many);
	strcat(description, ". ");
	return description;
}

char *
get_mv_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Vision: ");
	int many = 0;
	many = append(description, goodsmeller(ptr)			, "scent"					, many);
	many = append(description, species_perceives(ptr)	, "see invisible"			, many);
	many = append(description, species_is_telepathic(ptr)	, "telepathy"				, many);
	many = append(description, normalvision(ptr)		, "normal vision"			, many);
	many = append(description, darksight(ptr)			, "darksight"				, many);
	many = append(description, catsight(ptr)			, "catsight"				, many);
	many = append(description, lowlightsight2(ptr)		, "good low light vision"	, many);
	many = append(description, lowlightsight3(ptr)		, "great low light vision"	, many);
	many = append(description, echolocation(ptr)		, "echolocation"			, many);
	many = append(description, extramission(ptr)		, "extramission"			, many);
	many = append(description, rlyehiansight(ptr)		, "rlyehian sight"			, many);
	many = append(description, infravision(ptr)			, "infravision"				, many);
	many = append(description, bloodsense(ptr)			, "bloodsense"				, many);
	many = append(description, lifesense(ptr)			, "lifesense"				, many);
	many = append(description, earthsense(ptr)			, "earthsense"				, many);
	many = append(description, senseall(ptr)			, "senseall"				, many);
	many = append(description, (many==0)				, "blind"					, many);
	strcat(description, ". ");
	return description;
}
char * get_mg_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Mechanics: ");
	int many = 0;
	many = append(description, is_tracker(ptr)			, "tracks you"				, many);
	many = append(description, species_displaces(ptr), "displacing"				, many);
	many = append(description, polyok(ptr)				, "valid polymorph form"	, many);
	many = append(description, !polyok(ptr)				, "invalid polymorph form"	, many);
	many = append(description, is_untamable(ptr)		, "untamable"				, many);
	many = append(description, extra_nasty(ptr)			, "nasty"					, many);
	many = append(description, nospellcooldowns(ptr)	, "quick-caster"			, many);
	many = append(description, is_lord(ptr)				, "lord"					, many);
	many = append(description, is_prince(ptr)			, "prince"					, many);
	many = append(description, opaque(ptr)				, "opaque"					, many);
	many = append(description, species_regenerates(ptr)	, "regenerating"			, many);
	many = append(description, levl_follower(mtmp)		, "stalks you"				, many);
	many = append(description, (many==0)				, "normal"					, many);
	strcat(description, ". ");
	return description;
}

char *
get_mf_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Martial Skill: ");
	int many = 0;
	char buff[BUFSZ] = {0};

	Sprintf(buff, "can reach level %d", permonst_max_lev(ptr));
	many = append(description, TRUE				, buff					, many);
	
	if(MON_BAB(mtmp) != 1)
		Sprintf(buff, "recieves +%.2f to-hit per level", MON_BAB(mtmp));
	else
		Sprintf(buff, "recieves +1 to-hit per level");
	many = append(description, TRUE				, buff					, many);

	many = append(description, ptr->mflagsf&MF_MARTIAL_B				, "basic martial prowess"					, many);
	many = append(description, ptr->mflagsf&MF_MARTIAL_S				, "skillful martial prowess"					, many);
	many = append(description, ptr->mflagsf&MF_MARTIAL_E				, "expert martial prowess"					, many);

	many = append(description, has_phys_scaling(ptr)			, "recieves bonus physical damage"			, many);

	strcat(description, ". ");
	return description;
}

char *
get_mw_description_of_monster_type(struct monst * mtmp, char * description)
{
	struct permonst * ptr = mtmp->data;
	strcat(description, "Wards: ");
	int many = 0;
	many = append(description, (u.wardsknown & WARD_HEPTAGRAM) && !heptUnwardable(mtmp) && heptWarded(mtmp->data)								, "heptagram"			, many);
	many = append(description, (u.wardsknown & WARD_GORGONEION) && !gorgUnwardable(mtmp) && gorgWarded(mtmp->data)								, "gorgoneion"			, many);
	many = append(description, (u.wardsknown & WARD_ACHERON) && !standardUnwardable(mtmp) && circleWarded(mtmp->data)							, "circle of acheron"	, many);
	many = append(description, (u.wardsknown & WARD_PENTAGRAM) && !standardUnwardable(mtmp) && pentWarded(mtmp->data)							, "pentagram"			, many);
	many = append(description, (u.wardsknown & WARD_HEXAGRAM) && !(standardUnwardable(mtmp) || mtmp->mpeaceful) && hexWarded(mtmp->data)		, "hexagram"			, many);
	many = append(description, (u.wardsknown & WARD_HAMSA) && !standardUnwardable(mtmp) && hamWarded(mtmp->data)								, "hamsa"				, many);
	many = append(description, (u.wardsknown & WARD_ELDER_SIGN) && !standardUnwardable(mtmp) && (mtmp->data->mflagsw&MW_ELDER_SIGN)				, "elder sign"			, many);
	many = append(description, (u.wardsknown & WARD_ELDER_SIGN) && !standardUnwardable(mtmp) && (mtmp->data->mflagsw&MW_EYE_OF_YGG)				, "eye of yggdrasil"	, many);
	many = append(description, (u.wardsknown & WARD_EYE) && !standardUnwardable(mtmp) && (mtmp->data->mflagsw&MW_ELDER_EYE_ELEM)				, "elder elemental eye"	, many);
	many = append(description, (u.wardsknown & WARD_EYE) && !standardUnwardable(mtmp) && (mtmp->data->mflagsw&MW_ELDER_EYE_ENERGY)				, "4-fold elemental eye", many);
	many = append(description, (u.wardsknown & WARD_EYE) && !standardUnwardable(mtmp) && (mtmp->data->mflagsw&MW_ELDER_EYE_PLANES)				, "7-fold elemental eye", many);
	many = append(description, (u.wardsknown & WARD_QUEEN) && !standardUnwardable(mtmp) && queenWarded(mtmp->data)								, "scion queen mother"	, many);
	many = append(description, (u.wardsknown & WARD_CAT_LORD) && !(standardUnwardable(mtmp) || catWardInactive) && catWarded(mtmp->data)		, "cat lord"			, many);
	many = append(description, (u.wardsknown & WARD_GARUDA) && !standardUnwardable(mtmp) && wingWarded(mtmp->data)								, "wings of garuda"		, many);
	many = append(description, (u.wardsknown & WARD_YELLOW) && !yellowUnwardable(mtmp) && yellowWarded(mtmp->data)								, "yellow sign"		, many);
	many = append(description, (u.wardsknown & WARD_TOUSTEFNA) && !standardUnwardable(mtmp) && touWarded(mtmp->data)							, "toustefna stave"		, many);
	many = append(description, (u.wardsknown & WARD_DREPRUN) && !standardUnwardable(mtmp) && dreWarded(mtmp->data)								, "dreprun stave"		, many);
	many = append(description, (u.wardsknown & WARD_VEIOISTAFUR) && !standardUnwardable(mtmp) && veiWarded(mtmp->data)							, "veioistafur stave"	, many);
	many = append(description, (u.wardsknown & WARD_THJOFASTAFUR) && !standardUnwardable(mtmp) && thjWarded(mtmp->data)							, "thjofastafur stave"	, many);
	if(!many)
		strcat(description, "none known");
	strcat(description, ". ");
	return description;
}

char *
get_speed_description_of_monster_type(struct monst * mtmp, char * description)
{
	int speed = mtmp->data->mmove;
	if (speed > 35) {
		sprintf(description, "Extremely fast (%d). ", speed);
	}
	else if (speed > 19) {
		sprintf(description, "Very fast (%d). ", speed);
	}
	else if (speed > 12) {
		sprintf(description, "Fast (%d). ", speed);
	}
	else if (speed == 12) {
		sprintf(description, "Normal speed (%d). ", speed);
	}
	else if (speed > 8) {
		sprintf(description, "Slow (%d). ", speed);
	}
	else if (speed > 3) {
		sprintf(description, "Very slow (%d). ", speed);
	}
	else if (speed > 0) {
		sprintf(description, "Almost immobile (%d). ", speed);
	}
	else {
		sprintf(description, "Immobile (%d). ", speed);
	}

	if (stationary_mon(mtmp)) sprintf(description, "Can't move around. Speed %d. ", speed);

	return description;
}

char *
get_description_of_attack_type(uchar id)
{
	switch (id){
	//case AT_ANY: return "fake attack; dmgtype_fromattack wildcard";
	case AT_NONE: return "passive";
	case AT_CLAW: return "claw";
	case AT_BITE: return "bite";
	case AT_OBIT: return "bite";
	case AT_WBIT: return "bite";
	case AT_KICK: return "kick";
	case AT_BUTT: return "head butt";
	case AT_TAIL: return "tail slap";
	case AT_TONG: return "tongue";
	case AT_TUCH: return "touch";
	case AT_STNG: return "sting";
	case AT_HUGS: return "crushing bearhug";
	case AT_SPIT: return "spit";
	case AT_ENGL: return "engulf";
	case AT_BREA: return "breath";
	case AT_BRSH: return "splashing breath";
	case AT_EXPL: return "explosion";
	case AT_BOOM: return "on death";
	case AT_GAZE: return "targeted gaze";
	case AT_TENT: return "tentacles";
	case AT_ARRW: return "launch ammo";
	case AT_WHIP: return "whip";
	case AT_VINE: return "vines";
	case AT_VOMT: return "vomit";
	case AT_LRCH: return "reaching attack";
	case AT_HODS: return "mirror attack";
	case AT_LNCK: return "reaching bite";
	case AT_5SBT: return "long reach bite";
	case AT_MMGC: return "uses magic spell(s)";
	case AT_ILUR: return "swallow attack";
	case AT_HITS: return "automatic hit";
	case AT_WISP: return "mist wisps";
	case AT_TNKR: return "tinker";
	case AT_SRPR: return "spiritual blade";
	case AT_XSPR: return "offhand spiritual blade";
	case AT_MSPR: return "extra-hand spiritual blade";
	case AT_DSPR: return "million-arm spiritual blade";
	case AT_ESPR: return "floating spiritual blade";
	case AT_BEAM: return "ranged beam";
	case AT_DEVA: return "million-arm weapon";
	case AT_5SQR: return "long reach touch";
	case AT_WDGZ: return "passive gaze";
	case AT_BKGT: return "hungry mist";
	case AT_BKG2: return "horned light";
	case AT_MARI: return "carried weapon";
	case AT_XWEP: return "offhand weapon";
	case AT_WEAP: return "weapon";
	case AT_MAGC: return "uses magic spell(s)";
	case AT_REND: return "extra effect for previous attacks";
	default:
			impossible("bug in get_description_of_attack_type(%d)", id);
			return "<MISSING DECRIPTION, THIS IS A BUG>";
	}
}

char *
get_description_of_damage_type(uchar id)
{
	switch (id){
	//case AD_ANY: return "fake damage; attacktype_fordmg wildcard";
	case AD_PHYS: return "physical";
	case AD_MAGM: return "magic missiles";
	case AD_FIRE: return "fire";
	case AD_COLD: return "frost";
	case AD_SLEE: return "sleep";
	case AD_DISN: return "disintegration";
	case AD_ELEC: return "shock";
	case AD_DRST: return "poison (STR)";
	case AD_ACID: return "acid";
	case AD_SPC1: return "for extension of buzz()";
	case AD_SPC2: return "for extension of buzz()";
	case AD_BLND: return "blinds";
	case AD_STUN: return "stuns";
	case AD_SLOW: return "slows";
	case AD_PLYS: return "paralyses";
	case AD_DRLI: return "drains life";
	case AD_DREN: return "drains energy";
	case AD_LEGS: return "damages legs";
	case AD_STON: return "petrifies";
	case AD_STCK: return "sticky";
	case AD_SGLD: return "steals gold";
	case AD_SITM: return "steals item";
	case AD_SEDU: return "seduces & steals multiple items";
	case AD_TLPT: return "teleports you";
	case AD_RUST: return "rusts armour";
	case AD_CONF: return "confuses";
	case AD_DGST: return "digests";
	case AD_HEAL: return "heals wounds";
	case AD_WRAP: return "crushes and drowns";
	case AD_WERE: return "confers lycanthropy";
	case AD_DRDX: return "poison (DEX)";
	case AD_DRCO: return "poison (CON)";
	case AD_DRIN: return "drains intelligence";
	case AD_DISE: return "confers diseases";
	case AD_DCAY: return "decays organics";
	case AD_SSEX: return "seduces";
	case AD_HALU: return "causes hallucination";
	case AD_DETH: return "drains life force";
	case AD_PEST: return "causes illness";
	case AD_FAMN: return "causes hunger";
	case AD_SLIM: return "slimes";
	case AD_ENCH: return "disenchants";
	case AD_CORR: return "corrodes armor";
	case AD_POSN: return "poison";
	case AD_WISD: return "drains wisdom";
	case AD_VORP: return "vorpal strike";
	case AD_SHRD: return "destroys armor";
	case AD_SLVR: return "silver arrows";
	case AD_BALL: return "iron balls";
	case AD_BLDR: return "boulders";
	case AD_VBLD: return "boulder volley";
	case AD_TCKL: return "tickle";
	case AD_WET: return "splash with water";
	case AD_LETHE: return "splash with lethe water";
	case AD_BIST: return "bisecting beak";
	case AD_CNCL: return "cancellation";
	case AD_DEAD: return "deadly gaze";
	case AD_SUCK: return "sucks you apart";
	case AD_MALK: return "immobilize and shock";
	case AD_UVUU: return "splatters your head";
	case AD_ABDC: return "abduction teleportation";
	case AD_KAOS: return "spawn Chaos";
	case AD_LSEX: return "deadly seduction";
	case AD_HLBD: return "creates demons";
	case AD_SPNL: return "spawns Leviathan or Levistus";
	case AD_MIST: return "migo mist projector";
	case AD_TELE: return "monster teleports away";
	case AD_POLY: return "polymorphs you";
	case AD_PSON: return "psionic";
	case AD_GROW: return "grows brethren on death";
	case AD_SOUL: return "strengthens brethren on death";
	case AD_TENT: return "deadly tentacles";
	case AD_JAILER: return "sets Lucifer to appear and drops third key of law when killed";
	case AD_AXUS: return "fire-shock-drain-cold combo";
	case AD_UNKNWN: return "Priest of an unknown God";
	case AD_SOLR: return "silver arrows";
	case AD_CHKH: return "escalating damage";
	case AD_HODS: return "mirror attack";
	case AD_CHRN: return "cursed unicorn horn";
	case AD_LOAD: return "loadstones";
	case AD_GARO: return "physical damage + rumor";
	case AD_GARO_MASTER: return "physical damage + oracle";
	case AD_LVLT: return "level teleport";
	case AD_BLNK: return "mental invasion";
	case AD_WEEP: return "level teleport and drain";
	case AD_SPOR: return "generate spore";
	case AD_FNEX: return "fern spore explosion";
	case AD_SSUN: return "reflected sunlight";
	case AD_MAND: return "mandrake shriek";
	case AD_BARB: return "retaliatory physical";
	case AD_LUCK: return "drains luck";
	case AD_VAMP: return "drains blood";
	case AD_WEBS: return "spreads webbing";
	case AD_ILUR: return "drains memories";
	case AD_TNKR: return "tinkers";
	case AD_FRWK: return "firework explosions";
	case AD_STDY: return "studies you, making you vulnerable";
	case AD_OONA: return "Oona's energy type";
	case AD_NTZC: return "anti-equipment";
	case AD_WTCH: return "spawns tentacles";
	case AD_SHDW: return "shadow weapons";
	case AD_STTP: return "teleports your gear away";
	case AD_HDRG: return "half-dragon breath";
	case AD_STAR: return "silver starlight rapier";
	case AD_BSTR: return "black-star rapier";
	case AD_EELC: return "elemental electric";
	case AD_EFIR: return "elemental fire";
	case AD_EDRC: return "elemental poison (CON)";
	case AD_ECLD: return "elemental cold";
	case AD_EACD: return "elemental acid";
	case AD_CNFT: return "conflict-inducing touch";
	case AD_BLUD: return "blood";
	case AD_SURY: return "Arrows of Slaying";
	case AD_NPDC: return "drains constitution";
	case AD_GLSS: return "silver mirror shards";
	case AD_MERC: return "mercury blade";
	case AD_GOLD: return "turns victim to gold";
	case AD_ACFR: return "holy fire";
	case AD_DESC: return "dessication";
	case AD_BLAS: return "blasphemy";
	case AD_SESN: return "four seasons";
	case AD_POLN: return "pollen";
	case AD_FATK: return "forces target to attack";
	case AD_DUNSTAN: return "stones throw themselves at target";
	case AD_IRIS: return "iridescent tentacles";
	case AD_NABERIUS: return "tarnished bloody fangs";
	case AD_OTIAX: return "mist tendrils";
	case AD_SIMURGH: return "thirty-colored feathers";
	case AD_CMSL: return "cold missile";
	case AD_FMSL: return "fire missile";
	case AD_EMSL: return "electric missile";
	case AD_SMSL: return "physical missile";
	case AD_WMTG: return "War Machine targeting gaze";
	case AD_NUKE: return "physical large explosion";
	case AD_LOKO: return "lokoban prize scatter";
	case AD_LASR: return "War Machine laser beam";
	case AD_CLRC: return "random clerical spell";
	case AD_SPEL: return "random magic spell";
	case AD_RBRE: return "random breath weapon";
	case AD_RGAZ: return "random gaze attack";
	case AD_RETR: return "elemental gaze attack";
	case AD_SAMU: return "steal Amulet";
	case AD_CURS: return "steal intrinsic";
	case AD_BDFN: return "spears of blood";
	case AD_SQUE: return "steal Quest Artifact or Amulet";
	case AD_SPHR: return "create elemental spheres";
	case AD_DARK: return "dark";
	case AD_ROCK: return "rocks";
	case AD_RNBW: return "Iris special attack, hallu + sick";
	case AD_JACK: return "big fiery explosion, always leaves corpse";
	case AD_YANK: return "yanks you to them";
	case AD_PAIM: return "exploding magic missile spellbooks";
	case AD_ALIG: return "alignment blast and opposite alignment";
	case AD_SPIR: return "releases other alignment spirits";
	case AD_COSM: return "crystal memories";
	case AD_CRYS: return "dilithium crystals";
	case AD_NUDZ: return "mirror blast";
	case AD_WHIS: return "whispers from the void";
	case AD_LRVA: return "implant larva";
	case AD_HOOK: return "flesh hook";
	case AD_MDWP: return "mindwipe";
	case AD_SSTN: return "slow stoning";
	case AD_NPDS: return "non-poison-based drain strength";
	case AD_NPDD: return "non-poison-based drain dexterity";
	case AD_NPDR: return "non-poison-based drain charisma";
	case AD_NPDA: return "non-poison-based all attribute drain";
	case AD_DOBT: return "agnosis infliction";
	case AD_APCS: return "revelatory whispers";
	case AD_PULL: return "pull closer";
	case AD_PAIN: return "poison (STR) and crippling pain";
	case AD_MROT: return "inflict curses";
	case AD_LAVA: return "crushing lava";
	case AD_PYCL: return "pyroclastic";
	case AD_MOON: return "silver moonlight";
	case AD_HOLY: return "holy energy";
	case AD_UNHY: return "unholy energy";
	case AD_PERH: return "level-based damage";
	case AD_SVPN: return "severe poison";
	case AD_HLUH: return "corrupted holy energy";
	case AD_TSMI: return "magic-item-stealing tentacles";
	case AD_BYAK: return "byakhee eggs";
	case AD_UNRV: return "unnerving";
	case AD_DRHP: return "drains bonus HP";
	case AD_PUSH: return "push away";
	case AD_LICK: return "monstrous tongue lick";
	case AD_PFBT: return "poison and disease damage";
	case AD_OMUD: return "inchoate orc-spawn";
	default:
			impossible("bug in get_description_of_damage_type(%d)", id);
			return "<MISSING DESCRIPTION, THIS IS A BUG>";
	}
}

char *
get_description_of_damage_prefix(uchar aatyp, uchar adtyp)
{
	switch (aatyp)
	{
	case AT_WEAP:
	case AT_XWEP:
	case AT_DEVA:
		switch (adtyp)
		{
		case AD_PHYS:
			return "";
		case AD_FIRE:
		case AD_COLD:
		case AD_ELEC:
		case AD_ACID:
			return "physical + 4d6 ";
		case AD_EFIR:
		case AD_ECLD:
		case AD_EELC:
		case AD_EACD:
			return "physical + 3d7 ";
		default:
			return "physical + ";
		}
		break;
	}
	return "";
}

char *
get_description_of_attack(struct attack *mattk, char * main_temp_buf)
{
	if (!(mattk->damn + mattk->damd + mattk->aatyp + mattk->adtyp)) {
		main_temp_buf[0] = '\0';
		return main_temp_buf;
	}

	char temp_buf[BUFSZ] = "";
	if (mattk->damn + mattk->damd) {
		sprintf(main_temp_buf, "%dd%d", mattk->damn, mattk->damd);
#ifndef USE_TILES
		strcat(main_temp_buf, ",");
#endif
		strcat(main_temp_buf, " ");
	}
	else {
		main_temp_buf[0] = '\0';
	}
#ifndef USE_TILES
	while (strlen(main_temp_buf) < 6) {
		strcat(main_temp_buf, " ");
	}
#endif
	sprintf(temp_buf, "%s%s - %s%s", mattk->offhand ? "offhand " : "", get_description_of_attack_type(mattk->aatyp), get_description_of_damage_prefix(mattk->aatyp, mattk->adtyp), get_description_of_damage_type(mattk->adtyp));
	strcat(main_temp_buf, temp_buf);
#ifdef USE_TILES
	strcat(main_temp_buf, "; ");
#endif
	return main_temp_buf;
}

char *
get_description_of_ancient_breath(struct monst * mtmp, char * description)
{
	switch(mtmp->mtyp){
		case PM_ANCIENT_OF_BLESSINGS:
			sprintf(description, "Inhales blessings and exhales injuries.");
		break;
		case PM_ANCIENT_OF_VITALITY:
			sprintf(description, "Inhales death and exhales life.");
		break;
		case PM_ANCIENT_OF_CORRUPTION:
			sprintf(description, "Inhales good health and exhales slimy corruption.");
		break;
		case PM_ANCIENT_OF_THE_BURNING_WASTES:
			sprintf(description, "Inhales moisture and exhales sweltering heat.");
		break;
		case PM_ANCIENT_OF_THOUGHT:
			sprintf(description, "Inhales thought and exhales psychic shrieks.");
		break;
		case PM_ANCIENT_OF_ICE:
			sprintf(description, "Inhales warmth and exhales ice.");
		break;
		case PM_ANCIENT_OF_DEATH:
			sprintf(description, "Inhales life and exhales death.");
		break;
		case PM_BAALPHEGOR:
			if(mtmp->mhp < mtmp->mhpmax)
				sprintf(description, "Inhales free will and exhales static curses.");
		break;
	}
	return description;
}

char *
get_description_of_monster_type(struct monst * mtmp, char * description)
{
	/*
	pline("%d<><><>", plined_length("12345678901234567890123456789012345678901234567890123456789012345678901234567890"));//0 passed
	pline("%d<><><>", plined_length("1234567890123456789012345678901234567890123456789012345678901234567890123456789"));
	*/
	char temp_buf[BUFSZ] = "";
	char main_temp_buf[BUFSZ] = "";
	struct permonst * ptr = mtmp->data;
	int monsternumber = monsndx(ptr);

	/* monsters pretending to be other monsters won't be given away by the pokedex */
	struct monst fakemon;
	if (mtmp->m_ap_type == M_AP_MONSTER) {
		fakemon = *mtmp;
		monsternumber = fakemon.mtyp = mtmp->mappearance;
		ptr = fakemon.data = &mons[monsternumber];
		mtmp = &fakemon;
	}

	char name[BUFSZ] = "";
	Strcat(name, ptr->mname);
	append_template_desc(mtmp, name, type_is_pname(ptr), FALSE);

	temp_buf[0] = '\0';
	if (iflags.pokedex) {
		sprintf(temp_buf, "Accessing Pokedex entry for %s... ", (!Upolyd || (monsternumber < NUMMONS)) ? name : "this weird creature");
		strcat(description, temp_buf);

		strcat(description, "\n");
		strcat(description, " ");
		strcat(description, "\n");
		if (iflags.pokedex & POKEDEX_SHOW_STATS){
			strcat(description, "Base statistics of this monster type:");
			strcat(description, "\n");
			int ac = 10-(ptr->nac+ptr->dac+ptr->pac);
			sprintf(temp_buf, "Base level = %d. Difficulty = %d. AC = %d. DR = %d. MR = %d. Alignment %d. ",
				ptr->mlevel, monstr[monsndx(ptr)], ac, mdat_avg_mdr(mtmp), ptr->mr, ptr->maligntyp);
			strcat(description, temp_buf);
			temp_buf[0] = '\0';
			strcat(description, get_speed_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_GENERATION){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_generation_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_RESISTS){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_resistance_description_of_monster_type(mtmp, temp_buf));
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_weakness_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_CONVEYS){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_conveys_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_WEIGHT){
			temp_buf[0] = '\0';
			strcat(description, get_weight_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MM){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mm_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MT){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mt_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MB){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mb_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MV){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mv_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MG){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mg_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_MA){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_ma_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_WARDS){
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mw_description_of_monster_type(mtmp, temp_buf));
		}
		if (iflags.pokedex & POKEDEX_SHOW_ATTACKS){
			strcat(description, "\n");
			strcat(description, "Attacks:");
			strcat(description, "\n");
			struct attack * attk;
			struct attack prev_attk = noattack;
			int res[4];
			int indexnum = 0;
			int tohitmod = 0;
			int subout[SUBOUT_ARRAY_SIZE] = {0};
			/* zero out res[] */
			res[0] = MM_MISS;
			res[1] = MM_MISS;
			res[2] = MM_MISS;
			res[3] = MM_MISS;
			do {
				/* get next attack */
				attk = getattk(mtmp, (struct monst *)0, res, &indexnum, &prev_attk, TRUE, subout, &tohitmod);

				main_temp_buf[0] = '\0';
				get_description_of_attack(attk, temp_buf);
				if (temp_buf[0] == '\0') {
					if (indexnum == 0) {
#ifndef USE_TILES
						strcat(description, "    ");
#endif
						strcat(description, "none");
						strcat(description, "\n");
					}
					continue;
				}
#ifndef USE_TILES
				strcat(main_temp_buf, "    ");
#endif
				strcat(main_temp_buf, temp_buf);
				strcat(main_temp_buf, "\n");
				strcat(description, main_temp_buf);
			} while (!((attk)->aatyp == 0 && (attk)->adtyp == 0 && (attk)->damn == 0 && (attk)->damd == 0));		/* no more attacks */
			if(is_ancient(mtmp->data)){
				temp_buf[0] = '\0';
				get_description_of_ancient_breath(mtmp, temp_buf);
				if(temp_buf[0] != '\0'){
					strcat(description, "\n");
					strcat(description, temp_buf);
				}
					
			}
			temp_buf[0] = '\0';
			strcat(description, "\n");
			strcat(description, get_mf_description_of_monster_type(mtmp, temp_buf));
		}
		if(wizard){
			strcat(description, "\n");
			strcat(description, "Faction (debug-mode-only): ");
			sprintf(temp_buf, "%d", mtmp->mfaction);
			strcat(description, temp_buf);
			strcat(description, "\n");
		}
	}
	return description;
}

int
dowhatis()
{
	return do_look(FALSE);
}

int
doquickwhatis()
{
	return do_look(TRUE);
}

int
doidtrap()
{
	register struct trap *trap;
	int x, y, tt;

	if (!getdir("^")) return MOVE_CANCELLED;
	x = u.ux + u.dx;
	y = u.uy + u.dy;
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->tx == x && trap->ty == y) {
		if (!trap->tseen) break;
		tt = trap->ttyp;
		if (u.dz) {
		    if (u.dz < 0 ? (tt == TRAPDOOR || tt == HOLE) :
			    tt == ROCKTRAP) break;
		}
		tt = what_trap(tt);
		pline("That is %s%s%s.",
		      an(defsyms[trap_to_defsym(tt)].explanation),
		      !trap->madeby_u ? "" : (tt == WEB) ? " woven" :
			  /* trap doors & spiked pits can't be made by
			     player, and should be considered at least
			     as much "set" as "dug" anyway */
			  (tt == HOLE || tt == PIT) ? " dug" : " set",
		      !trap->madeby_u ? "" : " by you");
		return MOVE_CANCELLED;
	    }
	pline("I can't see a trap there.");
	return MOVE_CANCELLED;
}

char *
dowhatdoes_core(q, cbuf)
char q;
char *cbuf;
{
	dlb *fp;
	char bufr[BUFSZ];
	register char *buf = &bufr[6], *ep, ctrl, meta;

	fp = dlb_fopen(CMDHELPFILE, "r");
	if (!fp) {
		pline("Cannot open data file!");
		return 0;
	}

  	ctrl = ((q <= '\033') ? (q - 1 + 'A') : 0);
	meta = ((0x80 & q) ? (0x7f & q) : 0);
	while(dlb_fgets(buf,BUFSZ-6,fp)) {
	    if ((ctrl && *buf=='^' && *(buf+1)==ctrl) ||
		(meta && *buf=='M' && *(buf+1)=='-' && *(buf+2)==meta) ||
		*buf==q) {
		ep = index(buf, '\n');
		if(ep) *ep = 0;
		if (ctrl && buf[2] == '\t'){
			buf = bufr + 1;
			(void) strcpy(buf, "^?      ");
			eos(buf)[0] = ' ';	/* overwrite the '\0', we want the rest of the string */
			buf[1] = ctrl;
		} else if (meta && buf[3] == '\t'){
			buf = bufr + 2;
			(void) strcpy(buf, "M-?     ");
			eos(buf)[0] = ' ';	/* overwrite the '\0', we want the rest of the string */
			buf[2] = meta;
		} else if(buf[1] == '\t'){
			buf = bufr;
			buf[0] = q;
			(void) strcpy(buf+1, "       ");
			eos(buf)[0] = ' ';	/* overwrite the '\0', we want the rest of the string */
		}
		(void) dlb_fclose(fp);
		Strcpy(cbuf, buf);
		return cbuf;
	    }
	}
	(void) dlb_fclose(fp);
	return (char *)0;
}

int
dowhatdoes()
{
	char bufr[BUFSZ];
	char q, *reslt;

#if defined(UNIX) || defined(VMS)
	introff();
#endif
	q = yn_function("What command?", (char *)0, '\0');
#if defined(UNIX) || defined(VMS)
	intron();
#endif
	reslt = dowhatdoes_core(q, bufr);
	if (reslt)
		pline("%s", reslt);
	else
		pline("I've never heard of such commands.");
	return MOVE_CANCELLED;
}

/* data for help_menu() */
static const char *help_menu_items[] = {
/* 0*/	"Long description of the game and commands.",
/* 1*/	"List of game commands.",
/* 2*/	"Concise history of NetHack.",
/* 3*/	"Info on a character in the game display.",
/* 4*/	"Info on what a given key does.",
/* 5*/	"List of game options.",
/* 6*/	"Longer explanation of game options.",
/* 7*/  "Full list of keyboard commands.",
/* 8*/	"List of extended commands.",
/* 9*/	"The NetHack license.",
#ifdef PORT_HELP
	"%s-specific help and commands.",
#define PORT_HELP_ID 100
#define WIZHLP_SLOT 11
#else
#define WIZHLP_SLOT 10
#endif
#ifdef WIZARD
	"List of wizard-mode commands.",
#endif
	"",
	(char *)0
};

STATIC_OVL boolean
help_menu(sel)
	int *sel;
{
	winid tmpwin = create_nhwindow(NHW_MENU);
#ifdef PORT_HELP
	char helpbuf[QBUFSZ];
#endif
	int i, n;
	menu_item *selected;
	anything any;

	any.a_void = 0;		/* zero all bits */
	start_menu(tmpwin);
#ifdef WIZARD
	if (!wizard) help_menu_items[WIZHLP_SLOT] = "",
		     help_menu_items[WIZHLP_SLOT+1] = (char *)0;
#endif
	for (i = 0; help_menu_items[i]; i++)
#ifdef PORT_HELP
	    /* port-specific line has a %s in it for the PORT_ID */
	    if (help_menu_items[i][0] == '%') {
		Sprintf(helpbuf, help_menu_items[i], PORT_ID);
		any.a_int = PORT_HELP_ID + 1;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 helpbuf, MENU_UNSELECTED);
	    } else
#endif
	    {
		any.a_int = (*help_menu_items[i]) ? i+1 : 0;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			ATR_NONE, help_menu_items[i], MENU_UNSELECTED);
	    }
	end_menu(tmpwin, "Select one item:");
	n = select_menu(tmpwin, PICK_ONE, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *sel = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

int
dohelp()
{
	int sel = 0;

	if (help_menu(&sel)) {
		switch (sel) {
			case  0:  display_file(HELP, TRUE);  break;
			case  1:  display_file(SHELP, TRUE);  break;
			case  2:  (void) dohistory();  break;
			case  3:  (void) dowhatis();  break;
			case  4:  (void) dowhatdoes();  break;
			case  5:  option_help();  break;
			case  6:  display_file(OPTIONFILE, TRUE);  break;
			case  7:  dokeylist(); break;
			case  8:  (void) doextlist();  break;
			case  9:  display_file(LICENSE, TRUE);  break;
#ifdef WIZARD
			/* handle slot 9 or 10 */
			default: display_file(DEBUGHELP, TRUE);  break;
#endif
#ifdef PORT_HELP
			case PORT_HELP_ID:  port_help();  break;
#endif
		}
	}
	return MOVE_CANCELLED;
}

int
dohistory()
{
	display_file(HISTORY, TRUE);
	return MOVE_CANCELLED;
}

/*pager.c*/
