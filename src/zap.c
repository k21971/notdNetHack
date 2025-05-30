/*	SCCS Id: @(#)zap.c	3.4	2003/08/24	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

#include "xhity.h"

/* Disintegration rays have special treatment; corpses are never left.
 * But the routine which calculates the damage is separate from the routine
 * which kills the monster.  The damage routine returns this cookie to
 * indicate that the monster should be disintegrated.
 */
#define MAGIC_COOKIE 1000

#ifdef OVLB
static NEARDATA boolean obj_zapped;
static NEARDATA int poly_zapped;
#endif

extern boolean notonhead;	/* for long worms */

/* kludge to use mondied instead of killed */
extern boolean m_using;
STATIC_DCL void FDECL(polyuse, (struct obj*, int, int));
STATIC_DCL struct obj * FDECL(poly_obj_core, (struct obj *, int));
STATIC_DCL void FDECL(create_polymon, (struct obj *, int));
STATIC_DCL boolean FDECL(zap_updown, (struct obj *));
STATIC_DCL boolean FDECL(zap_reflect, (struct monst *, struct zapdata *));
STATIC_DCL int FDECL(zapdamage, (struct monst *, struct monst *, struct zapdata *));
STATIC_DCL int FDECL(zhit, (struct monst *, struct monst *, struct zapdata *));
#ifdef STEED
STATIC_DCL boolean FDECL(zap_steed, (struct obj *));
#endif

#ifdef OVL0
STATIC_DCL void FDECL(backfire, (struct obj *));
STATIC_DCL int FDECL(spell_hit_bonus, (int));
#endif

#define is_hero_spell(type)	((type) >= 10 && (type) < 20)
#define wand_damage_die(skill)	(((skill) > 1) ? (2*(skill) + 4) : 6)
#define fblt_damage_die(skill)	((skill)+1)
#define wandlevel(otyp)	(otyp == WAN_MAGIC_MISSILE ? 1 : otyp == WAN_SLEEP ? 1 : otyp == WAN_STRIKING ? 2 : otyp == WAN_FIRE ? 4 : otyp == WAN_COLD ? 4 : otyp == WAN_LIGHTNING ? 5 : otyp == WAN_DEATH ? 7 : 1)

#ifndef OVLB
STATIC_VAR const char are_blinded_by_the_flash[];
extern const char * const flash_types[];
#else
STATIC_VAR const char are_blinded_by_the_flash[] = "are blinded by the flash!";

static const int dirx[8] = {0, 1, 1,  1,  0, -1, -1, -1},
				 diry[8] = {1, 1, 0, -1, -1, -1,  0,  1};

struct zapdata tzapdat;	/* temporary zap data -- assumes there is only 1 zap happening at once (which is a safe assumption for now) */

/* returns the string formerly given in the flash_type char*[] array. Not exhaustive -- if new combinations are used, they must be added here as well */
/* adtyp  -- AD_TYPE damage type, defined in monattk.h */
/* olet  -- O_CLASS type -- wand, spell, corpse (breath attack), weapon (raygun) */
char *
flash_type(adtyp, ztyp)
int adtyp, ztyp;
{
	switch (ztyp)
	{
	case ZAP_WAND:
		switch (adtyp)
		{
		case AD_MAGM: return "magic missile";
		case AD_FIRE: return "bolt of fire";
		case AD_COLD: return "bolt of cold";
		case AD_SLEE: return "sleep ray";
		case AD_DEAD: return "death ray";
		case AD_ELEC: return "lightning bolt";
		case AD_DARK: return "bolt of darkness";
		case AD_HOLY: return "holy missile";
		case AD_UNHY: return "unholy missile";
		case AD_STAR: return "stream of silver stars";
		default:      impossible("unknown wand damage type in flash_type: %d", adtyp);
			return "NaN ray";
		}
		break;
	case ZAP_SPELL:
		switch (adtyp)
		{
		case AD_MAGM: return "magic missile";
		case AD_PHYS: return "sothothic missile";
		case AD_FIRE: return "fireball";
		case AD_COLD: return "cone of cold";
		case AD_SLEE: return "sleep ray";
		case AD_DEAD: return "finger of death";
		case AD_ELEC: return "bolt of lightning";
		case AD_DRST: return "poison spray";
		case AD_ACID: return "acid splash";
		case AD_STAR: return "stream of silver stars";
		case AD_HOLY: return "holy missile";
		case AD_UNHY: return "unholy missile";
		case AD_HLUH: return "corrupted missile";
		case AD_DISN: return "disintegration ray";
		case AD_LASR: return "laser beam";
		case AD_MADF: return "burst of magenta fire";
		default:      impossible("unknown spell damage type in flash_type: %d", adtyp);
			return "cube of questions";
		}
		break;

	case ZAP_BREATH:
		switch (adtyp)
		{
		case AD_MAGM: return "blast of missiles";
		case AD_EFIR: 
		case AD_FIRE: 
			return "blast of fire";
		case AD_COLD:
		case AD_ECLD:
			return "blast of frost";
		case AD_SLEE: return "blast of sleep gas";
		case AD_DISN: return "blast of disintegration";
		case AD_EELC: 
		case AD_ELEC: 
			return "blast of lightning";
		case AD_DRST: return "blast of poison gas";
		case AD_ACID:
		case AD_EACD:
			return "blast of acid";
		case AD_DRLI: return "blast of dark energy";
		case AD_GOLD: return "blast of golden shards";
		// These are provided to deal with spray breaths, and aren't handled for direct hits.
		case AD_BLUD: return "spray of blood";
		case AD_SLIM: return "spout of acidic slime";
		case AD_WET: return "blast of water";
		case AD_DARK: return "blast of darkness";
		case AD_PHYS: return "blast";
		case AD_DISE: return "blast of spores";
		default:      impossible("unknown breath damage type in flash_type: %d", adtyp);
			return "blast of static";
		}
	case ZAP_RAYGUN:
		switch (adtyp)
		{
		case AD_MAGM: return "magic ray";
		case AD_FIRE: return "heat ray";
		case AD_COLD: return "cold ray";
		case AD_SLEE: return "stun ray";
		case AD_DEAD: return "death ray";
		case AD_DISN: return "disintegration ray";
		default:      impossible("unknown raygun damage type in flash_type: %d", adtyp);
			return "barrage of lost packets";
		}
	case ZAP_FLAMETHROWER:
		switch(adtyp){
			case AD_FIRE: return "stream of burning oil";
			default:      impossible("unknown flamethrower damage type in flash_type: %d", adtyp);
			return "stream of bad gas prices";
		}
	default:
		impossible("unknown ztyp in flash_type: %d", ztyp);
	}
	return "404 BEAM NOT FOUND";
}

/* returns the colour each zap should be */
int
zap_glyph_color(adtyp)
int adtyp;
{
	switch (adtyp)
	{
	case AD_DEAD:
	case AD_DISN:
	case AD_DARK:
	case AD_UNHY:
		return CLR_BLACK;
	case AD_BLUD:
		return CLR_RED;
	case AD_SLIM:
	case AD_DRST:
		return CLR_GREEN;
	case AD_PHYS:
		return CLR_BRIGHT_MAGENTA;
	case AD_WET:
		return CLR_BLUE;
	case AD_DISE:
		return CLR_MAGENTA;
		//	return CLR_CYAN;
	case AD_HLUH:
		return CLR_GRAY;
		//	return NO_COLOR;
	case AD_EFIR:
	case AD_FIRE:
		return CLR_ORANGE;
	case AD_LASR:
		return CLR_RED;
		//	return CLR_BRIGHT_GREEN;
	case AD_ACID:
	case AD_EACD:
	case AD_GOLD:
		return CLR_YELLOW;
	case AD_MAGM:
	case AD_SLEE:
		return CLR_BRIGHT_BLUE;
	case AD_MADF:
		return CLR_BRIGHT_MAGENTA;
		//	return CLR_BRIGHT_CYAN;
	case AD_ECLD:
	case AD_COLD:
	case AD_EELC:
	case AD_ELEC:
	case AD_STAR:
	case AD_HOLY:
		return CLR_WHITE;
	case AD_DRLI:
		return CLR_MAGENTA;
	default:
		impossible("unaccounted-for zap type in zap_glyph_color: %d", adtyp);
		return CLR_WHITE;
	}
}

int
wand_adtype(wand)
int wand;
{
	switch (wand)
	{
	case WAN_MAGIC_MISSILE: return AD_MAGM;
	case WAN_FIRE:          return AD_FIRE;
	case WAN_COLD:          return AD_COLD;
	case WAN_SLEEP:         return AD_SLEE;
	case WAN_DEATH:         return AD_DEAD;
	case WAN_LIGHTNING:     return AD_ELEC;
	default:
		impossible("unaccounted-for wand passed to wand_adtype: %d", wand);
		return -1;
	}
}

/* Routines for IMMEDIATE wands and spells. */
/* bhitm: monster mtmp was hit by the effect of wand or spell otmp */
int
bhitm(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	boolean wake = TRUE;	/* Most 'zaps' should wake monster */
	boolean reveal_invis = FALSE;
	boolean dbldam = !flags.mon_moving && Spellboost;
	int dmg, otyp = otmp->otyp;
	const char *zap_type_text = "spell";
	struct obj *obj;
	boolean disguised_mimic = (mtmp->data->mlet == S_MIMIC &&
				   mtmp->m_ap_type != M_AP_NOTHING);

	if (u.uswallow && mtmp == u.ustuck)
	    reveal_invis = FALSE;

	switch(otyp) {
	case WAN_STRIKING:
	case ROD_OF_FORCE:
		use_skill(P_WAND_POWER, wandlevel(otyp));
		zap_type_text = otyp == ROD_OF_FORCE ? "rod" : "wand";
		/* fall through */
	case SPE_FORCE_BOLT:
		reveal_invis = TRUE;
		if (MON_WEP(mtmp) && MON_WEP(mtmp)->otyp == ROD_OF_FORCE) {
			MON_WEP(mtmp)->age = min(MON_WEP(mtmp)->age+10000, LIGHTSABER_MAX_CHARGE);
			reveal_invis = FALSE;
			break;	/* skip makeknown */
		} else if (MON_SWEP(mtmp) && MON_SWEP(mtmp)->otyp == ROD_OF_FORCE) {
			MON_SWEP(mtmp)->age = min(MON_SWEP(mtmp)->age+10000, LIGHTSABER_MAX_CHARGE);
			reveal_invis = FALSE;
			break;	/* skip makeknown */
		} else if (resists_magm(mtmp)) {	/* match effect on player */
			shieldeff(mtmp->mx, mtmp->my);
			break;	/* skip makeknown */
		} else if (u.uswallow || otyp == WAN_STRIKING || rnd(20) < 10 + find_mac(mtmp) + 2*P_SKILL(otyp == SPE_FORCE_BOLT ? P_ATTACK_SPELL : P_WAND_POWER)) {
			if(otyp == WAN_STRIKING || otyp == ROD_OF_FORCE) dmg = d(wand_damage_die(P_SKILL(P_WAND_POWER))-4,12);
			else dmg = d(fblt_damage_die(P_SKILL(P_ATTACK_SPELL)),12);
			if (!flags.mon_moving && otyp == SPE_FORCE_BOLT){
				if(uwep && uwep->oartifact == ART_ANNULUS && uwep->otyp == CHAKRAM)
					dmg += d((u.ulevel+1)/2, 12);
				if(u.ulevel == 30 && (artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_PATIENCE))
					dmg += d(10, 12);
			}
			if(dbldam) dmg *= 2;
			if(!flags.mon_moving && Double_spell_size) dmg *= 1.5;
			if (otyp == SPE_FORCE_BOLT){
				if(u.ukrau_duration) dmg *= 1.5;
			    dmg += spell_damage_bonus();
			}
			
			hit(zap_type_text, mtmp, exclam(dmg));
			(void) resist(mtmp, otmp->otyp == ROD_OF_FORCE ? WAND_CLASS : otmp->oclass, dmg, TELL);
			if(otyp == ROD_OF_FORCE && !DEADMONSTER(mtmp) && u.usteed != mtmp){
				mhurtle(mtmp, sgn(mtmp->mx - u.ux), sgn(mtmp->my - u.uy), BOLT_LIM, FALSE);
			}
		} else if(!flags.mon_moving || cansee(mtmp->mx, mtmp->my)) miss(zap_type_text, mtmp);
		makeknown(otyp);
		break;
	case WAN_SLOW_MONSTER:
	case SPE_SLOW_MONSTER:
		if (!resist(mtmp, otmp->oclass, 0, TELL)) {
			mon_adjust_speed(mtmp, -1, otmp, TRUE);
			m_dowear(mtmp, FALSE); /* might want speed boots */
			if (u.uswallow && (mtmp == u.ustuck) &&
			    is_whirly(mtmp->data)) {
				You("disrupt %s!", mon_nam(mtmp));
				pline("A huge hole opens up...");
				expels(mtmp, mtmp->data, TRUE);
			}
		} else if(cansee(mtmp->mx,mtmp->my)) shieldeff(mtmp->mx, mtmp->my);
		break;
	case WAN_SPEED_MONSTER:
		if (!resist(mtmp, otmp->oclass, 0, TELL)) {
			mon_adjust_speed(mtmp, 1, otmp, TRUE);
			m_dowear(mtmp, FALSE); /* might want speed boots */
		} else if(cansee(mtmp->mx,mtmp->my)) shieldeff(mtmp->mx, mtmp->my);
		break;
	case WAN_UNDEAD_TURNING:
	case SPE_TURN_UNDEAD:
		wake = FALSE;
		if (unturn_dead(mtmp)) wake = TRUE;
		if (is_undead(mtmp->data)) {
			reveal_invis = TRUE;
			wake = TRUE;
			if(otyp == WAN_UNDEAD_TURNING) dmg = d(wand_damage_die(P_SKILL(P_WAND_POWER)),8);
			else dmg = rnd(8);
			if(dbldam) dmg *= 2;
			if(!flags.mon_moving && Double_spell_size) dmg *= 1.5;
			if (otyp == SPE_TURN_UNDEAD){
				if(u.ukrau_duration) dmg *= 1.5;
				dmg += spell_damage_bonus();
			}
			flags.bypasses = TRUE;	/* for make_corpse() */
			if (!resist(mtmp, otmp->oclass, dmg, TELL)) {
			    if (mtmp->mhp > 0) monflee(mtmp, 0, FALSE, TRUE);
			} else if(cansee(mtmp->mx,mtmp->my)) shieldeff(mtmp->mx, mtmp->my);
		}
		break;
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
	case POT_POLYMORPH:
		if (resists_magm(mtmp) || resists_poly(mtmp->data)) {
		    /* magic resistance protects from polymorph traps, so make
		       it guard against involuntary polymorph attacks too... */
		    shieldeff(mtmp->mx, mtmp->my);
		} else if (!resist(mtmp, otmp->oclass, 0, TELL)) {
		    /* natural shapechangers aren't affected by system shock
		       (unless protection from shapechangers is interfering
		       with their metabolism...) */
		    if (mtmp->cham == CHAM_ORDINARY && !rn2(25)) {
			if (canseemon(mtmp)) {
			    pline("%s shudders!", Monnam(mtmp));
			    makeknown(otyp);
			}
			/* dropped inventory shouldn't be hit by this zap */
			for (obj = mtmp->minvent; obj; obj = obj->nobj)
			    bypass_obj(obj);
			/* flags.bypasses = TRUE; ## for make_corpse() */
			/* no corpse after system shock */
			xkilled(mtmp, 3);
		    } else if (newcham(mtmp, NON_PM,
				       (otyp != POT_POLYMORPH), FALSE)) {
			if (!Hallucination && canspotmon(mtmp))
			    makeknown(otyp);
		    }
		} else if(cansee(mtmp->mx,mtmp->my)) shieldeff(mtmp->mx, mtmp->my);
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		(void) cancel_monst(mtmp, otmp, TRUE, TRUE, FALSE,0);
		break;
	case WAN_TELEPORTATION:
	case SPE_TELEPORT_AWAY:
		reveal_invis = !u_teleport_mon(mtmp, TRUE);
		break;
	case WAN_MAKE_INVISIBLE:
	    {
		int oldinvis = mtmp->minvis;
		char nambuf[BUFSZ];

		/* format monster's name before altering its visibility */
		Strcpy(nambuf, Monnam(mtmp));
		mon_set_minvis(mtmp);
		if (!oldinvis && knowninvisible(mtmp)) {
		    pline("%s turns transparent!", nambuf);
		    makeknown(otyp);
		}
		break;
	    }
	case WAN_NOTHING:
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
		wake = FALSE;
		break;
	case WAN_PROBING:
		wake = FALSE;
		reveal_invis = TRUE;
		probe_monster(mtmp);
		makeknown(otyp);
		break;
	case WAN_OPENING:
	case SPE_KNOCK:
		wake = FALSE;	/* don't want immediate counterattack */
		if (u.uswallow && mtmp == u.ustuck) {
			if (is_animal(mtmp->data)) {
				if (Blind) You_feel("a sudden rush of air!");
				else pline("%s opens its mouth!", Monnam(mtmp));
			}
			expels(mtmp, mtmp->data, TRUE);
#ifdef STEED
		} else if (!!(obj = which_armor(mtmp, W_SADDLE))) {
			mtmp->misc_worn_check &= ~obj->owornmask;
			update_mon_intrinsics(mtmp, obj, FALSE, FALSE);
			obj->owornmask = 0L;
			obj_extract_self(obj);
			place_object(obj, mtmp->mx, mtmp->my);
			/* call stackobj() if we ever drop anything that can merge */
			newsym(mtmp->mx, mtmp->my);
#endif
		}
		break;
	case SPE_HEALING:
	case SPE_EXTRA_HEALING:
	case SPE_FULL_HEALING:
	case SPE_MASS_HEALING:{
		int delta = mtmp->mhp;
		const char *starting_word_ptr = injury_desc_word(mtmp);
		int health = otyp == SPE_FULL_HEALING ? (50*min(2, P_SKILL(P_HEALING_SPELL))) : (d(otyp == SPE_EXTRA_HEALING ? (6 + P_SKILL(P_HEALING_SPELL)) : 6, otyp != SPE_HEALING ? 8 : 4) + 6*(P_SKILL(P_HEALING_SPELL)-1));
		reveal_invis = TRUE;
		if(has_template(mtmp, PLAGUE_TEMPLATE) && otyp == SPE_FULL_HEALING){
			if(canseemon(mtmp))
				pline("%s is no longer sick!", Monnam(mtmp));
			set_template(mtmp, 0);
			if(!mtmp->mtame && rnd(!always_hostile(mtmp->data) ? 12 : 20) < ACURR(A_CHA)){
				pline("%s is very grateful!", Monnam(mtmp));
				mtmp->mpeaceful = TRUE;
				char qbuf[BUFSZ];
				Sprintf(qbuf, "Turn %s away from your party?", mhim(mtmp));
				if(yn(qbuf) != 'y'){
					struct monst *newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
					if(newmon){
						mtmp = newmon;
						newsym(mtmp->mx, mtmp->my);
					}
				}
			}
		}
	    if (mtmp->mtyp != PM_PESTILENCE) {
			char hurtmonbuf[BUFSZ];
			Strcpy(hurtmonbuf, Monnam(mtmp));
			wake = FALSE;		/* wakeup() makes the target angry */
			/* skill adjustment ranges from -6 to + 18 (-6 means 0 hp healed minimum)*/
			mtmp->mhp += health;
			if (mtmp->mhp > mtmp->mhpmax)
				mtmp->mhp = mtmp->mhpmax;
			if (mtmp->mblinded) {
				mtmp->mblinded = 0;
				mtmp->mcansee = 1;
			}
			delta = mtmp->mhp - delta; //Note: final minus initial
			if (canseemon(mtmp)) {
				if (disguised_mimic) {
				if (mtmp->m_ap_type == M_AP_OBJECT &&
					mtmp->mappearance == STRANGE_OBJECT) {
					/* it can do better now */
					set_mimic_sym(mtmp);
					newsym(mtmp->mx, mtmp->my);
				} else
					mimic_hit_msg(mtmp, otyp);
				} else {
					if (!can_see_hurtnss_of_mon(mtmp)) {
						pline("%s looks%s better.", Monnam(mtmp),
							otyp != SPE_HEALING ? " much" : "" );
					}
					else {
						const char * ending_word_ptr = injury_desc_word(mtmp);
						// Note: this compares the string pointers recieved from injury_desc_word. They should be the same if the level is unchanged, and different otherwise.
						if(starting_word_ptr != ending_word_ptr){
							pline("%s %s %s.",
								hurtmonbuf, 
								(mtmp->mhp < mtmp->mhpmax) ? "now looks only" : "looks",
								ending_word_ptr);
						}
						else if(delta != 0){
							pline("%s looks better, but still %s.", hurtmonbuf, ending_word_ptr);
						}
						// else {
							// pline("%s is still %s.", hurtmonbuf, ending_word_ptr);
						// }
					}
				}
			}

			if(mtmp->mtame && Role_if(PM_HEALER)){
				int xp = (experience(mtmp, 0)) * delta / mtmp->mhpmax;
				if(wizard) pline("%d out of %d XP", xp, experience(mtmp, 0));
				if(xp){
					more_experienced(xp, 0);
					newexplevel();
				}
			}
			if (mtmp->mtame || mtmp->mpeaceful) {
				adjalign(Role_if(PM_HEALER) ? 1 : sgn(u.ualign.type));
			}
	    } else {	/* Pestilence */
			/* Pestilence will always resist; damage is half of 3d{4,8} */
			(void) resist(mtmp, otmp->oclass, health/2, TELL);
	    }
	}break;
	case WAN_LIGHT:	/* (broken wand) */
	case WAN_DARKNESS:	/* (broken wand) */
		if (flash_hits_mon(mtmp, otmp)) {
		    makeknown(WAN_LIGHT);
		    reveal_invis = TRUE;
		}
		break;
	case WAN_SLEEP:	/* (broken wand) */
		/* [wakeup() doesn't rouse victims of temporary sleep,
		    so it's okay to leave `wake' set to TRUE here] */
		reveal_invis = TRUE;
		if (sleep_monst(mtmp, d(1 + otmp->spe, 12), WAND_CLASS))
		    slept_monst(mtmp);
		if (!Blind) makeknown(WAN_SLEEP);
		break;
	case SPE_STONE_TO_FLESH:
		if (monsndx(mtmp->data) == PM_STONE_GOLEM) {
		    char *name = Monnam(mtmp);
		    /* turn into flesh golem */
		    if (newcham(mtmp, PM_FLESH_GOLEM, FALSE, FALSE)) {
			if (canseemon(mtmp))
			    pline("%s turns to flesh!", name);
		    } else {
			if (canseemon(mtmp))
			    pline("%s looks rather fleshy for a moment.",
				  name);
		    }
		} else
		    wake = FALSE;
		break;
	case SPE_DRAIN_LIFE:
	case WAN_DRAINING:{	/* KMH */
		int levlost = 0;
		reveal_invis = TRUE;
		if(otyp == WAN_DRAINING){
			levlost = (wand_damage_die(P_SKILL(P_WAND_POWER))-4)/2;
			dmg = d(levlost,8);
		} else {
			levlost = 1;
			dmg = rnd(8);
			if (uwep && (uwep->oartifact == ART_DEATH_SPEAR_OF_KEPTOLO || 
						uwep->oartifact == ART_STAFF_OF_NECROMANCY)
			){
				dmg += d((u.ulevel+1)/3, 4);
				levlost += (u.ulevel+1)/6;
			}
		}
		if(dbldam){
			dmg *= 2;
			levlost *= 2;
		}
		if(!flags.mon_moving && Double_spell_size){
			dmg *= 1.5;
			levlost *= 1.5;
		}
		if (otyp == SPE_DRAIN_LIFE){
			if(u.ukrau_duration){
				dmg *= 1.5;
				levlost *= 1.5;
			}
			dmg += spell_damage_bonus();
		}
		if (resists_drli(mtmp)){
		    shieldeff(mtmp->mx, mtmp->my);
	break;	/* skip makeknown */
		}else if (!resist(mtmp, otmp->oclass, dmg, TELL) &&
				mtmp->mhp > 0) {
		    mtmp->mhp -= dmg;
		    mtmp->mhpmax -= dmg;
		    if (mtmp->mhp <= 0 || mtmp->mhpmax <= 0 || mtmp->m_lev < levlost)
				xkilled(mtmp, 1);
		    else {
				mtmp->m_lev -= levlost;
				if (canseemon(mtmp))
					pline("%s suddenly seems weaker!", Monnam(mtmp));
		    }
		} else if(cansee(mtmp->mx,mtmp->my)) shieldeff(mtmp->mx, mtmp->my);
		makeknown(otyp);
	}break;
	default:
		impossible("What an interesting effect (%d)", otyp);
		break;
	}
	if(wake) {
	    if(mtmp->mhp > 0) {
		wakeup(mtmp, TRUE);
		m_respond(mtmp);
		if(mtmp->isshk && !*u.ushops) hot_pursuit(mtmp);
	    } else if(mtmp->m_ap_type)
		seemimic(mtmp); /* might unblock if mimicing a boulder/door */
	}
	/* note: bhitpos won't be set if swallowed, but that's okay since
	 * reveal_invis will be false.  We can't use mtmp->mx, my since it
	 * might be an invisible worm hit on the tail.
	 */
	if (reveal_invis) {
	    if (mtmp->mhp > 0 && cansee(bhitpos.x, bhitpos.y) &&
							!canspotmon(mtmp))
		map_invisible(bhitpos.x, bhitpos.y);
	}
	return 0;
}

void
probe_monster(mtmp)
struct monst *mtmp;
{
	struct obj *otmp;

	mstatusline(mtmp);
	if (notonhead) return;	/* don't show minvent for long worm tail */
	
	mdrslotline(mtmp);
	
#ifndef GOLDOBJ
	if (mtmp->minvent || mtmp->mgold) {
#else
	if (mtmp->minvent) {
#endif
	    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		otmp->dknown = 1;	/* treat as "seen" */
	    (void) display_minventory(mtmp, MINV_ALL, (char *)0);
	} else {
	    pline("%s is not carrying anything.", noit_Monnam(mtmp));
	}
}

#endif /*OVLB*/
#ifdef OVL1

/*
 * Return the object's physical location.  This only makes sense for
 * objects that are currently on the level (i.e. migrating objects
 * are nowhere).  By default, only things that can be seen (in hero's
 * inventory, monster's inventory, or on the ground) are reported.
 * By adding BURIED_TOO and/or CONTAINED_TOO flags, you can also get
 * the location of buried and contained objects.  Note that if an
 * object is carried by a monster, its reported position may change
 * from turn to turn.  This function returns FALSE if the position
 * is not available or subject to the constraints above.
 */
boolean
get_obj_location(obj, xp, yp, locflags)
struct obj *obj;
xchar *xp, *yp;
int locflags;
{
	switch (obj->where) {
	    case OBJ_INVENT:
		*xp = u.ux;
		*yp = u.uy;
		return TRUE;
	    case OBJ_FLOOR:
		*xp = obj->ox;
		*yp = obj->oy;
		return TRUE;
	    case OBJ_MINVENT:
		if (obj->ocarry->mx) {
		    *xp = obj->ocarry->mx;
		    *yp = obj->ocarry->my;
		    return TRUE;
		}
		break;	/* !mx => migrating monster */
	    case OBJ_BURIED:
		if (locflags & BURIED_TOO) {
		    *xp = obj->ox;
		    *yp = obj->oy;
		    return TRUE;
		}
		break;
	    case OBJ_CONTAINED:
		if (locflags & CONTAINED_TOO)
		    return get_obj_location(obj->ocontainer, xp, yp, locflags);
		break;
		case OBJ_INTRAP:
		if (locflags & INTRAP_TOO){
			*xp = obj->otrap->tx;
			*yp = obj->otrap->ty;
			return TRUE;
		}
		break;
	}
	*xp = *yp = 0;
	return FALSE;
}

boolean
get_mon_location(mon, xp, yp, locflags)
struct monst *mon;
xchar *xp, *yp;
int locflags;	/* non-zero means get location even if monster is buried */
{
	if (mon == &youmonst) {
	    *xp = u.ux;
	    *yp = u.uy;
	    return TRUE;
	} else if (mon && !DEADMONSTER(mon) && mon->mx > 0 && (!mon->mburied || locflags)) {
	    *xp = mon->mx;
	    *yp = mon->my;
	    return TRUE;
	} else {	/* migrating or buried */
	    *xp = *yp = 0;
	    return FALSE;
	}
}

/* used by revive() and animate_statue() */
struct monst *
montraits(obj,cc)
struct obj *obj;
coord *cc;
{
	struct monst *mtmp = (struct monst *)0;
	struct monst *mtmp2 = (struct monst *)0;

	if (get_ox(obj, OX_EMON))
		mtmp2 = get_mtraits(obj, TRUE);
	if (mtmp2) {
		set_mon_data(mtmp2, mtmp2->mtyp);
		if (mtmp2->mhpmax <= 0 && !is_rider(mtmp2->data)) {
			/* no good; free any mx made and return null */
			rem_all_mx(mtmp2);
			return (struct monst *)0;
		}
		mtmp = makemon(mtmp2->data,
				cc->x, cc->y, NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH);
		if (!mtmp) return mtmp;

		/* heal the monster */
		if (mtmp->mhpmax > mtmp2->mhpmax && is_rider(mtmp2->data))
			mtmp2->mhpmax = mtmp->mhpmax;
		mtmp2->mhp = mtmp2->mhpmax;
		mtmp2->deadmonster = FALSE;
		/* Get these ones from mtmp */
		mtmp2->minvent = mtmp->minvent; /*redundant*/
		/* monster ID is available if the monster died in the current
		   game, but should be zero if the corpse was in a bones level
		   (we cleared it when loading bones) */
		if (!mtmp2->m_id)
		    mtmp2->m_id = mtmp->m_id;
		mtmp2->mx   = mtmp->mx;
		mtmp2->my   = mtmp->my;
		mtmp2->mux  = mtmp->mux;
		mtmp2->muy  = mtmp->muy;
		mtmp2->mw   = mtmp->mw;
		mtmp2->msw	= mtmp->msw;
		mtmp2->wormno = mtmp->wormno;
		mtmp2->misc_worn_check = mtmp->misc_worn_check;
		mtmp2->weapon_check = mtmp->weapon_check;
		mtmp2->mtrapseen = mtmp->mtrapseen;
		mtmp2->mflee = mtmp->mflee;
		mtmp2->mburied = mtmp->mburied;
		mtmp2->mundetected = mtmp->mundetected;
		mtmp2->mfleetim = mtmp->mfleetim;
		mtmp2->mlstmv = mtmp->mlstmv;
		mtmp2->m_ap_type = mtmp->m_ap_type;
		mtmp2->mappearance = mtmp->mappearance;
		/* set these ones explicitly */
		mtmp2->mavenge = 0;
		mtmp2->meating = 0;
		mtmp2->mleashed = 0;
		mtmp2->mtrapped = 0;
		mtmp2->msleeping = 0;
		mtmp2->mfrozen = 0;
      if(mtmp->mtyp == PM_GIANT_TURTLE && (mtmp->mflee))
        mtmp2->mcanmove=0;
      else
		mtmp2->mcanmove = 1;
		if(mtmp2->mblinded){
			mtmp2->mblinded = 0;
			mtmp2->mcansee = 1;	/* set like in makemon */
		}
		if(mtmp2->mdeafened){
			mtmp2->mdeafened = 0;
			mtmp2->mcanhear = 1;	/* set like in makemon */
		}
		mtmp2->mstun = 0;
		mtmp2->mconf = 0;
		replmon(mtmp,mtmp2);
	}
	return mtmp2;
}

/*
 * get_container_location() returns the following information
 * about the outermost container:
 * loc argument gets set to: 
 *	OBJ_INVENT	if in hero's inventory; return 0.
 *	OBJ_FLOOR	if on the floor; return 0.
 *	OBJ_BURIED	if buried; return 0.
 *	OBJ_MINVENT	if in monster's inventory; return monster.
 * container_nesting is updated with the nesting depth of the containers
 * if applicable.
 */
struct monst *
get_container_location(obj, loc, container_nesting)
struct obj *obj;
int *loc;
int *container_nesting;
{
	if (!obj || !loc)
		return 0;

	if (container_nesting) *container_nesting = 0;
	while (obj && obj->where == OBJ_CONTAINED) {
		if (container_nesting) *container_nesting += 1;
		obj = obj->ocontainer;
	}
	if (obj) {
	    *loc = obj->where;	/* outermost container's location */
	    if (obj->where == OBJ_MINVENT) return obj->ocarry;
	}
	return (struct monst *)0;
}

/*
 * Attempt to revive the given corpse, return the revived monster if
 * successful.  Note: this does NOT use up the corpse if it fails.
 */
struct monst *
revive(obj, dolls)
struct obj *obj;
boolean dolls;
{
	struct monst *mtmp = (struct monst *)0;
	struct obj *container = (struct obj *)0;
	int container_nesting = 0;
	schar savetame = 0;
	boolean recorporealization = FALSE;
	boolean in_container = FALSE;
	if(obj->otyp == CORPSE || obj->otyp == FOSSIL ||
		(dolls && (obj->otyp == BROKEN_ANDROID || obj->otyp == BROKEN_GYNOID || obj->otyp == LIFELESS_DOLL))
	) {
		int montype = obj->corpsenm;
		xchar x, y;
		int wasfossil = (obj->otyp == FOSSIL);
		
		if(is_changed_mtyp(montype))
			return (struct monst *) 0; //Can't revive these corpses, the main part of the monster vaporized.
		
		if (obj->where == OBJ_CONTAINED) {
			/* deal with corpses in [possibly nested] containers */
			struct monst *carrier;
			int holder = 0;

			container = obj->ocontainer;
			carrier = get_container_location(container, &holder,
							&container_nesting);
			switch(holder) {
			    case OBJ_MINVENT:
				x = carrier->mx; y = carrier->my;
				in_container = TRUE;
				break;
			    case OBJ_INVENT:
				x = u.ux; y = u.uy;
				in_container = TRUE;
				break;
			    case OBJ_FLOOR:
				if (!get_obj_location(obj, &x, &y, CONTAINED_TOO))
					return (struct monst *) 0;
				in_container = TRUE;
				break;
			    default:
			    	return (struct monst *)0;
			}
		} else {
			/* only for invent, minvent, or floor */
			if (!get_obj_location(obj, &x, &y, 0))
			    return (struct monst *) 0;
		}
		if (in_container) {
			/* Rules for revival from containers:
			   - the container cannot be locked
			   - the container cannot be heavily nested (>2 is arbitrary)
			   - the container cannot be a statue or bag of holding
			     (except in very rare cases for the latter)
			*/
			if (!x || !y || container->olocked || container_nesting > 2 ||
			    container->otyp == STATUE ||
			    (container->otyp == BAG_OF_HOLDING && rn2(40)))
				return (struct monst *)0;
		}

		if (MON_AT(x,y)) {
		    coord new_xy;

		    if (enexto(&new_xy, x, y, &mons[montype]))
			x = new_xy.x,  y = new_xy.y;
		}

		if(cant_create(&montype, TRUE)) {
			/* make a zombie or worm instead */
			mtmp = makemon(&mons[montype], x, y,
				       NO_MINVENT|MM_NOWAIT);
			if (mtmp) {
				mtmp->mhp = mtmp->mhpmax = 100;
				mon_adjust_speed(mtmp, 2, (struct obj *)0, TRUE); /* MFAST */
			}
		} else {
		    if (get_ox(obj, OX_EMON)) {
			    coord xy;
			    xy.x = x; xy.y = y;
				mtmp = montraits(obj, &xy);
				if (mtmp && get_mx(mtmp, MX_EDOG))
					wary_dog(mtmp, TRUE);
		    } else {
				/* Generic android corpse */
				if(is_android(&mons[montype])){
					if(rn2(2)){
						if(montype == PM_ANDROID)
							montype = PM_FLAYED_ANDROID;
						else 
							montype = PM_FLAYED_GYNOID;
						mtmp = makemon(&mons[montype], x, y,
						   NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH);
						if(mtmp && !rn2(4))
							set_template(mtmp, M_BLACK_WEB);
					}
					else {
						mtmp = makemon(&mons[montype], x, y,
						   NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH|MM_EDOG);
						if(mtmp){
							initedog(mtmp);
							if(rn2(2))
								mtmp->mcrazed = TRUE;
						}
					}
				}
				else {
 		            mtmp = makemon(&mons[montype], x, y,
				       NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH);
				}
			}
		    if (mtmp) {
				if (get_ox(obj, OX_EMID)) {
					unsigned m_id;
					struct monst *ghost;
					m_id = obj->oextra_p->emid_p[0];
					ghost = find_mid(m_id, FM_FMON);
						if (ghost && ghost->mtyp == PM_GHOST) {
							int x2, y2;
							x2 = ghost->mx; y2 = ghost->my;
							if (ghost->mtame)
								savetame = ghost->mtame;
							if (canseemon(ghost))
							pline("%s is suddenly drawn into its former body!",
							Monnam(ghost));
						mondead(ghost);
						recorporealization = TRUE;
						newsym(x2, y2);
					}
					/* either it worked or it didn't. */
					rem_ox(obj, OX_EMID);
				}
				/* Monster retains its name */
				if (get_ox(obj, OX_ENAM))
					mtmp = christen_monst(mtmp, ONAME(obj));
				/* flag the quest leader as alive. */
				if (mtmp->mtyp == urole.ldrnum || mtmp->m_id ==
					quest_status.leader_m_id) {
					quest_status.leader_m_id = mtmp->m_id;
					quest_status.leader_is_dead = FALSE;
				}
			}
		}
		if (mtmp) {
			if (obj->oeaten)
				mtmp->mhp = eaten_stat(mtmp->mhp, obj);
			/* track that this monster was revived at least once */
			mtmp->mrevived = 1;

			if (recorporealization) {
				/* If mtmp is revivification of former tame ghost*/
				if (savetame) {
				    struct monst *mtmp2 = tamedog(mtmp, (struct obj *)0);
				    if (mtmp2) {
					mtmp2->mtame = savetame;
					mtmp = mtmp2;
				    }
				}
				/* was ghost, now alive, it's all very confusing */
				mtmp->mconf = 1;
			}

			switch (obj->where) {
			    case OBJ_INVENT:
				useup(obj);
				break;
			    case OBJ_FLOOR:
					/* in case MON_AT+enexto for invisible mon */
					x = obj->ox,  y = obj->oy;
					/* not useupf(), which charges */
					if (obj->quan > 1L)
						obj = splitobj(obj, 1L);
					delobj(obj);
					newsym(x, y);
				break;
			    case OBJ_MINVENT:
					m_useup(obj->ocarry, obj);
				break;
			    case OBJ_CONTAINED:
					if (obj->quan > 1L)
						obj = splitobj(obj, 1L);
					obj_extract_self(obj);
					obfree(obj, (struct obj *) 0);
				break;
			    default:
				panic("revive");
			}
			if(wasfossil){
				if (can_undead(mtmp->data)) {
					set_template(mtmp, SKELIFIED);
					newsym(mtmp->mx, mtmp->my);
				}
				else {
					if (cansee(mtmp->mx, mtmp->my)) {
						pline_The("fossil briefly animates before crumbling into dust.");
					}
					mongone(mtmp);
				}
			}
		}
	}
	return mtmp;
}

void
revive_egg(obj)
struct obj *obj;
{
	/*
	 * Note: generic eggs with corpsenm set to NON_PM will never hatch.
	 */
	if (obj->otyp != EGG) return;
	if (obj->corpsenm != NON_PM && !dead_species(obj->corpsenm, TRUE))
	    attach_egg_hatch_timeout(obj);
}

/* try to revive all corpses and eggs carried by `mon' */
int
unturn_dead(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp2;
	char owner[BUFSZ], corpse[BUFSZ];
	boolean youseeit;
	int once = 0, res = 0;

	youseeit = (mon == &youmonst) ? TRUE : canseemon(mon);
	otmp2 = (mon == &youmonst) ? invent : mon->minvent;

	while ((otmp = otmp2) != 0) {
	    otmp2 = otmp->nobj;
	    if (otmp->otyp == EGG)
		revive_egg(otmp);
	    if (otmp->otyp != CORPSE && otmp->otyp != FOSSIL) continue;
	    /* save the name; the object is liable to go away */
	    if (youseeit) Strcpy(corpse, corpse_xname(otmp, TRUE));

	    /* for a merged group, only one is revived; should this be fixed? */
	    if ((mtmp2 = revive(otmp, FALSE)) != 0) {
		++res;
		if (youseeit) {
		    if (!once++) Strcpy(owner,
					(mon == &youmonst) ? "Your" :
					s_suffix(Monnam(mon)));
		    pline("%s %s suddenly comes alive!", owner, corpse);
		} else if (canseemon(mtmp2))
		    pline("%s suddenly appears!", Amonnam(mtmp2));
	    }
	}
	return res;
}
#endif /*OVL1*/

#ifdef OVLB
static const char charged_objs[] = { WAND_CLASS, WEAPON_CLASS, ARMOR_CLASS, 0 };

void
costly_cancel(obj)
register struct obj *obj;
{
	char objroom;
	struct monst *shkp = (struct monst *)0;

	if (obj->no_charge) return;

	switch (obj->where) {
	case OBJ_INVENT:
		if (obj->unpaid) {
		    shkp = shop_keeper(*u.ushops);
		    if (!shkp) return;
		    Norep("You cancel an unpaid object, you pay for it!");
		    bill_dummy_object(obj);
		}
		break;
	case OBJ_FLOOR:
		objroom = *in_rooms(obj->ox, obj->oy, SHOPBASE);
		shkp = shop_keeper(objroom);
		if (!shkp || !inhishop(shkp)) return;
		if (costly_spot(u.ux, u.uy) && objroom == *u.ushops) {
		    Norep("You cancel it, you pay for it!");
		    bill_dummy_object(obj);
		} else
		    (void) stolen_value(obj, obj->ox, obj->oy, FALSE, FALSE);
		break;
	}
}

/* cancel obj, possibly carried by you or a monster */
void
cancel_item(obj)
register struct obj *obj;
{
	boolean	u_ring = (obj == uleft) || (obj == uright);
	register boolean holy = (obj->otyp == POT_WATER && obj->blessed);
	
	adj_abon(obj, -obj->spe);

	switch(obj->otyp) {
		case RIN_GAIN_STRENGTH:
			if ((obj->owornmask & W_RING) && u_ring) {
				ABON(A_STR) -= obj->spe;
				flags.botl = 1;
			}
			break;
		case RIN_GAIN_CONSTITUTION:
			if ((obj->owornmask & W_RING) && u_ring) {
				ABON(A_CON) -= obj->spe;
				flags.botl = 1;
			}
			break;
		case RIN_ADORNMENT:
			if ((obj->owornmask & W_RING) && u_ring) {
				ABON(A_CHA) -= obj->spe;
				flags.botl = 1;
			}
			break;
		case RIN_INCREASE_ACCURACY:
			if ((obj->owornmask & W_RING) && u_ring)
				u.uhitinc -= obj->spe;
			break;
		case RIN_INCREASE_DAMAGE:
			if ((obj->owornmask & W_RING) && u_ring)
				u.udaminc -= obj->spe;
			break;
		/* case RIN_PROTECTION:  not needed */
	}

	/* MRKR: Cancelled *DSM reverts to scales.  */
	/*       Suggested by Daniel Morris in RGRN */

	if (obj->otyp >= GRAY_DRAGON_SCALE_MAIL &&
	    obj->otyp <= YELLOW_DRAGON_SCALE_MAIL) {
		/* dragon scale mail reverts to dragon scales */

		boolean worn = (obj == uarm);

		if (!Blind) {
			char buf[BUFSZ];
			pline("%s %s reverts to its natural form!", 
		              Shk_Your(buf, obj), xname(obj));
		} else if (worn) {
			Your("armor feels looser.");
		}
		costly_cancel(obj);

		if (worn) {
			setworn((struct obj *)0, W_ARM);
		}

		/* assumes same order */
		obj->otyp = GRAY_DRAGON_SCALES +
			obj->otyp - GRAY_DRAGON_SCALE_MAIL;

		if (worn) {
			setworn(obj, W_ARM);
		}
	}

	if (obj->otyp >= GRAY_DRAGON_SCALE_SHIELD &&
	    obj->otyp <= YELLOW_DRAGON_SCALE_SHIELD) {
		/* dragon scale shield reverts to dragon scales */

		boolean worn = (obj == uarms);

		if (!Blind) {
			char buf[BUFSZ];
			pline("%s %s reverts to its natural form!", 
		              Shk_Your(buf, obj), xname(obj));
		} else if (worn) {
			Your("shield reverts to its natural form!");
		}
		costly_cancel(obj);

		if (worn) {
			setworn((struct obj *)0, W_ARMS);
		}

		/* assumes same order */
		obj->otyp = GRAY_DRAGON_SCALES +
			obj->otyp - GRAY_DRAGON_SCALE_SHIELD;
	}

	if (objects[obj->otyp].oc_magic
	    || (obj->spe && (obj->oclass == ARMOR_CLASS ||
			     obj->oclass == WEAPON_CLASS || is_weptool(obj)))
	    || obj->otyp == POT_ACID || obj->otyp == POT_SICKNESS) {
	    if (obj->spe != ((obj->oclass == WAND_CLASS) ? -1 : 0) &&
	       obj->otyp != WAN_CANCELLATION &&
		 /* can't cancel cancellation */
		 obj->otyp != MAGIC_LAMP &&
		 obj->otyp != RIN_WISHES &&
		 obj->otyp != CANDELABRUM_OF_INVOCATION) {
		costly_cancel(obj);
		obj->spe = (obj->oclass == WAND_CLASS) ? -1 : 0;
	    }
	    switch (obj->oclass) {
	      case SCROLL_CLASS:
		costly_cancel(obj);
		if (obj->otyp == SCR_GOLD_SCROLL_OF_LAW) break;	//no cancelling these
		obj->otyp = SCR_BLANK_PAPER;
		remove_oprop(obj, OPROP_TACTB);
		obj->spe = 0;
		obj->oward = 0;
		break;
	      case SPBOOK_CLASS:
		if (obj->otyp != SPE_CANCELLATION &&
			obj->otyp != SPE_BOOK_OF_THE_DEAD &&
			!obj->oartifact
		){
		    costly_cancel(obj);
		    obj->otyp = SPE_BLANK_PAPER;
			obj->obj_color = objects[SPE_BLANK_PAPER].oc_color;
			remove_oprop(obj, OPROP_TACTB);
			obj->spe = 0;
			obj->oward = 0;
		}
		break;
	      case POTION_CLASS:
		/* Potions of amnesia are uncancelable. */
		if (obj->otyp == POT_AMNESIA) break;

		costly_cancel(obj);
		if (obj->otyp == POT_SICKNESS ||
		    obj->otyp == POT_SEE_INVISIBLE) {
	    /* sickness is "biologically contaminated" fruit juice; cancel it
	     * and it just becomes fruit juice... whereas see invisible
	     * tastes like "enchanted" fruit juice, it similarly cancels.
	     */
		    obj->otyp = POT_FRUIT_JUICE;
		} else {
			if (obj->otyp == POT_STARLIGHT)
				end_burn(obj, FALSE);
			obj->otyp = POT_WATER;
			obj->odiluted = 0; /* same as any other water */
		}
		set_object_color(obj);
		break;
	    }
	}
	if (holy) costly_cancel(obj);
	unbless(obj);
	uncurse(obj);
#ifdef INVISIBLE_OBJECTS
	if (obj->oinvis) obj->oinvis = 0;
#endif
	return;
}

/* Remove a positive enchantment or charge from obj,
 * possibly carried by you or a monster
 */
boolean
drain_scorpion(obj)
struct obj *obj;
{
	int cprops = 0;
	int i;
	long flag;
	for (i = 0; i < 32; i++){
		flag = 0x1L<<i;
		if(check_carapace_mod(obj, flag) && !(flag == CPROP_HARD_SCALE && check_carapace_mod(obj, CPROP_PLATES)))
			cprops++;
	}
	if(!cprops)
		return FALSE;
	cprops = rn2(cprops);
	for (i = 0; i < 32; i++){
		flag = 0x1L<<i;
		if(check_carapace_mod(obj, flag) && !(flag == CPROP_HARD_SCALE && check_carapace_mod(obj, CPROP_PLATES))){
			if(!cprops)
				break;
			else cprops--;
		}
	}
	int price = 1;
	if(flag == CPROP_ILL_STING
	 || flag == CPROP_CROWN
	){
		price = 3;
	}
	else if(flag == CPROP_PLATES
	 || flag == CPROP_WHIPPING
	 || flag == CPROP_SHIELDS
	 || flag == CPROP_IMPURITY
	 || flag == CPROP_SWIMMING
	 || flag == CPROP_CLAWS
	){
		price = 2;
	}
	if(artinstance[ART_SCORPION_CARAPACE].CarapaceLevel - price < 10)
		return FALSE;

	artinstance[ART_SCORPION_CARAPACE].CarapaceLevel -= price;
	artinstance[ART_SCORPION_CARAPACE].CarapaceXP = newuexp(artinstance[ART_SCORPION_CARAPACE].CarapaceLevel) - 1;
	
	obj->ovara_carapace &= ~flag;
	return TRUE;
}

boolean
drain_item(obj)
struct obj *obj;
{
	boolean u_ring;

	/* Is this a charged/enchanted object? */
	if (!obj || (!objects[obj->otyp].oc_charged &&
			obj->oclass != WEAPON_CLASS &&
			obj->oclass != ARMOR_CLASS && !is_weptool(obj)) ||
			obj->spe <= 0)
	    return (FALSE);
	if (obj_resists(obj, 10, 90))
	    return (FALSE);

	/* Charge for the cost of the object */
	costly_cancel(obj);	/* The term "cancel" is okay for now */

	adj_abon(obj, -1);

	/* Drain the object and any implied effects */
	obj->spe--;
	u_ring = (obj == uleft) || (obj == uright);
	switch(obj->otyp) {
	case RIN_GAIN_STRENGTH:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_STR)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_GAIN_CONSTITUTION:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_CON)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_ADORNMENT:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_CHA)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_INCREASE_ACCURACY:
	    if ((obj->owornmask & W_RING) && u_ring)
	    	u.uhitinc--;
	    break;
	case RIN_INCREASE_DAMAGE:
	    if ((obj->owornmask & W_RING) && u_ring)
	    	u.udaminc--;
	    break;
	case RIN_PROTECTION:
	    flags.botl = 1;
	    break;
	}
	if (carried(obj)) update_inventory();
	return (TRUE);
}

/* Decrement enchantment or charge from obj,
 * possibly carried by you or a monster
 */
boolean
damage_item(obj)
register struct obj *obj;
{
	boolean u_ring;

	/* Is this a charged/enchanted object? */
	if (!obj || (!objects[obj->otyp].oc_charged &&
			obj->oclass != WEAPON_CLASS &&
			obj->oclass != ARMOR_CLASS && !is_weptool(obj)))
	    return (FALSE);
	if (obj_resists(obj, 10, 90))
	    return (FALSE);

	/* Charge for the cost of the object */
	costly_cancel(obj);	/* The term "cancel" is okay for now */
	
	adj_abon(obj, -1);
	
	/* Drain the object and any implied effects */
	obj->spe--;
	u_ring = (obj == uleft) || (obj == uright);
	switch(obj->otyp) {
	case RIN_GAIN_STRENGTH:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_STR)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_GAIN_CONSTITUTION:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_CON)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_ADORNMENT:
	    if ((obj->owornmask & W_RING) && u_ring) {
	    	ABON(A_CHA)--;
	    	flags.botl = 1;
	    }
	    break;
	case RIN_INCREASE_ACCURACY:
	    if ((obj->owornmask & W_RING) && u_ring)
	    	u.uhitinc--;
	    break;
	case RIN_INCREASE_DAMAGE:
	    if ((obj->owornmask & W_RING) && u_ring)
	    	u.udaminc--;
	    break;
	case RIN_PROTECTION:
	    flags.botl = 1;
	    break;
	}
	if (carried(obj)) update_inventory();
	return (TRUE);
}

#endif /*OVLB*/
#ifdef OVL0

boolean
obj_resists(obj, ochance, achance)
struct obj *obj;
int ochance, achance;	/* percent chance for ordinary objects, artifacts */
{
	if (is_asc_obj(obj) ||
	    (obj->otyp == CORPSE && is_rider(&mons[obj->corpsenm]))
	) {
		return TRUE;
	} else {
		int chance = rn2(100);

		return((boolean)(chance < ((obj->oartifact || is_lightsaber(obj) || is_imperial_elven_armor(obj) || is_slab(obj) || obj->blood_smithed || obj->spe >= 10) ? achance : ochance)));
	}
}

boolean
obj_shudders(obj)
struct obj *obj;
{
	int	zap_odds;

	if (obj->oclass == WAND_CLASS)
		zap_odds = 3;	/* half-life = 2 zaps */
	else if (obj->cursed)
		zap_odds = 3;	/* half-life = 2 zaps */
	else if (obj->blessed)
		zap_odds = 12;	/* half-life = 8 zaps */
	else
		zap_odds = 8;	/* half-life = 6 zaps */

	/* adjust for "large" quantities of identical things */
	if(obj->quan > 4L) zap_odds /= 2;

	return((boolean)(! rn2(zap_odds)));
}
#endif /*OVL0*/
#ifdef OVLB

/* Use up at least minwt number of things made of material mat.
 * There's also a chance that other stuff will be used up.  Finally,
 * there's a random factor here to keep from always using the stuff
 * at the top of the pile.
 */
STATIC_OVL void
polyuse(objhdr, mat, minwt)
    struct obj *objhdr;
    int mat, minwt;
{
    register struct obj *otmp, *otmp2;

    for(otmp = objhdr; minwt > 0 && otmp; otmp = otmp2) {
	otmp2 = otmp->nexthere;
	if (otmp == uball || otmp == uchain) continue;
	if (obj_resists(otmp, 0, 0)) continue;	/* preserve unique objects */
#ifdef MAIL
	if (otmp->otyp == SCR_MAIL) continue;
#endif

	if (((int) otmp->obj_material == mat) ==
		(rn2(minwt + 1) != 0)) {
	    /* appropriately add damage to bill */
	    if (costly_spot(otmp->ox, otmp->oy)) {
		if (*u.ushops)
			addtobill(otmp, FALSE, FALSE, FALSE);
		else
			(void)stolen_value(otmp,
					   otmp->ox, otmp->oy, FALSE, FALSE);
	    }
	    if (otmp->quan < LARGEST_INT)
		minwt -= (int)otmp->quan;
	    else
		minwt = 0;
	    delobj(otmp);
	}
    }
}

/*
 * Polymorph some of the stuff in this pile into a monster, preferably
 * a golem of the kind okind.
 */
STATIC_OVL void
create_polymon(obj, okind)
    struct obj *obj;
    int okind;
{
	struct permonst *mdat = (struct permonst *)0;
	struct monst *mtmp;
	const char *material;
	int pm_index;

	/* no golems if you zap only one object -- not enough stuff */
	if(!obj || (!obj->nexthere && obj->quan == 1L)) return;

	/* some of these choices are arbitrary */
	switch(okind) {
	case GREEN_STEEL:
	    pm_index = PM_GREEN_STEEL_GOLEM;
	    material = "metal ";
	    break;
	case IRON:
	case LEAD:
	case METAL:
	case MITHRIL:
	    pm_index = PM_IRON_GOLEM;
	    material = "metal ";
	    break;
	case COPPER:
	case SILVER:
	case PLATINUM:
	case GEMSTONE:
	case MINERAL:
	case SALT:
	    pm_index = rn2(2) ? PM_STONE_GOLEM : PM_CLAY_GOLEM;
	    material = "lithic ";
	    break;
	case 0:
	case FLESH:
	    /* there is no flesh type, but all food is type 0, so we use it */
	    pm_index = PM_FLESH_GOLEM;
	    material = "organic ";
	    break;
	case WOOD:
	    pm_index = PM_WOOD_GOLEM;
	    material = "wood ";
	    break;
	case LEATHER:
	    pm_index = PM_LEATHER_GOLEM;
	    material = "leather ";
	    break;
	case CLOTH:
	    pm_index = PM_ROPE_GOLEM;
	    material = "cloth ";
	    break;
	case SHELL_MAT:
	case CHITIN:
	case BONE:
	    pm_index = PM_SKELETON;     /* nearest thing to "bone golem" */
	    material = "bony ";
	    break;
	case GOLD:
	    pm_index = PM_GOLD_GOLEM;
	    material = "gold ";
	    break;
	case OBSIDIAN_MT:
	case GLASS:
	    pm_index = PM_GLASS_GOLEM;
	    material = "glassy ";
	    break;
	case PAPER:
	    pm_index = PM_PAPER_GOLEM;
	    material = "paper ";
	    break;
	case DRAGON_HIDE:
	    pm_index = PM_PSEUDODRAGON;
	    material = "draconic ";
	    break;
	default:
	    /* if all else fails... */
	    pm_index = PM_STRAW_GOLEM;
	    material = "";
	    break;
	}

	if (!(mvitals[pm_index].mvflags & G_GENOD && !In_quest(&u.uz)))
		mdat = &mons[pm_index];

	mtmp = makemon(mdat, obj->ox, obj->oy, NO_MM_FLAGS);
	polyuse(obj, okind, (int)mons[pm_index].cwt);

	if(mtmp && cansee(mtmp->mx, mtmp->my)) {
	    pline("Some %sobjects meld, and %s arises from the pile!",
		  material, a_monnam(mtmp));
	}
}

/* Assumes obj is on the floor. */
void
do_osshock(obj)
struct obj *obj;
{
	long i;

#ifdef MAIL
	if (obj->otyp == SCR_MAIL) return;
#endif
	obj_zapped = TRUE;

	if(poly_zapped < 0) {
	    /* some may metamorphosize */
	    for (i = obj->quan; i; i--)
		if (! rn2(Luck + 45)) {
		    poly_zapped = obj->obj_material;
		    break;
		}
	}

	/* if quan > 1 then some will survive intact */
	if (obj->quan > 1L) {
	    if (obj->quan > LARGEST_INT)
		obj = splitobj(obj, (long)rnd(30000));
	    else
		obj = splitobj(obj, (long)rnd((int)obj->quan - 1));
	}

	/* appropriately add damage to bill */
	if (costly_spot(obj->ox, obj->oy)) {
		if (*u.ushops)
			addtobill(obj, FALSE, FALSE, FALSE);
		else
			(void)stolen_value(obj,
					   obj->ox, obj->oy, FALSE, FALSE);
	}

	/* zap the object */
	delobj(obj);
}

/* 
 * Polymorphs obj into a random object from the source's class.
 *
 * May shrink stacks, has controls against various abuses,
 * and is generally has behaviour befitting of polymorph magic.
 * 
 * Replaces obj in chains, returing a pointer to the new object.
 */
struct obj *
randpoly_obj(obj)
struct obj * obj;
{
	int new_otyp = STRANGE_OBJECT;	// default to random
	struct obj * otmp;				// new object

	/* things that affect what otyp will be created by polymorph */
	switch(obj->otyp) {
		case SPE_BLANK_PAPER:
		case SCR_BLANK_PAPER:
		case SCR_AMNESIA:
			new_otyp = rn2(2) ? SPE_BLANK_PAPER : SCR_BLANK_PAPER;
			break;
#ifdef MAIL
		case SCR_MAIL:
			new_otyp = SCR_MAIL;
			break;
#endif
		case POT_WATER:
		case POT_AMNESIA:
			new_otyp = POT_WATER;
			break;
		case POT_BLOOD:
			new_otyp = POT_BLOOD;
			break;
		case EGG:
			if (obj->spe)
				new_otyp = EGG;
			break;
		case HYPOSPRAY_AMPULE:
			new_otyp = HYPOSPRAY_AMPULE;
			break;
		case SCR_GOLD_SCROLL_OF_LAW:
			new_otyp = GOLD_PIECE;
			break;
	}
	if(obj->oclass == TILE_CLASS){
		if(obj->otyp >= FIRST_GLYPH && obj->otyp <= LAST_GLYPH){
			new_otyp = rn2(LAST_GLYPH - FIRST_GLYPH + 1) + FIRST_GLYPH;
		}
		else if(obj->otyp == APHANACTONAN_ARCHIVE){
			new_otyp = rn2(SPE_STONE_TO_FLESH - SPE_DIG + 1) + SPE_DIG;
		}
		else if(obj->otyp == APHANACTONAN_RECORD){
			new_otyp = rn2(2) ?
				(rn2(SPE_STONE_TO_FLESH - SPE_DIG + 1) + SPE_DIG) : 
				(rn2(SCR_RESISTANCE - SCR_ENCHANT_ARMOR + 1) + SCR_ENCHANT_ARMOR);
		}
	}
	/* turn crocodile corpses into shoes */
	if (obj->otyp == CORPSE && obj->corpsenm == PM_CROCODILE) {
		new_otyp = LOW_BOOTS;
	}
	/* too-worn-out spellbooks turn blank */
	if (obj->oclass == SPBOOK_CLASS && obj->spestudied > MAX_SPELL_STUDY) {
		new_otyp = SPE_BLANK_PAPER;
		remove_oprop(obj, OPROP_TACTB);
	}

	/* create the new object, otmp, of the new type (or random type) */
	otmp = poly_obj_core(obj, new_otyp);

	/* after-creating-otmp special handling */

	/* Sokoban guilt, boulder-like objects. Assumed to be the players fault. */
	if (is_boulder(obj) && In_sokoban(&u.uz)) {
	    change_luck(-1);
	}
	/* 'n' merged objects may be fused into 1 object */
	if (otmp->quan > 1L &&
		(!objects[otmp->otyp].oc_merge || (otmp->quan > (long)rn2(1000)))) {
	    otmp->quan = 1L;
	}
	/* potions of water have their BUC randomized (and were guaranteed to turn into water) */
	if (obj->otyp == POT_WATER) {
		int state = rn2(3) - 1;
		otmp->blessed = state > 0;
		otmp->cursed = state < 0;
	}
	/* potions of blood get a random appropriate bloodtype (and were guaranteed to turn into blood)  */
	if (obj->otyp == POT_BLOOD) {
		struct obj * dummy = mksobj(POT_BLOOD, NO_MKOBJ_FLAGS);
		otmp->corpsenm = dummy->corpsenm;
		delobj(dummy);
	}
#ifdef MAIL
	/* scrolls of mail have spe=1 (and were guaranteed to turn into mail) */
	if (obj->otyp == SCR_MAIL) {
		otmp->spe = 1;
	}
#endif
	/* eggs laid by the player try to become other plausibly-player-laid eggs (and were guaranteed to turn into eggs)  */
	if (obj->otyp == EGG && obj->spe) {
		int mtyp, tryct = 100;
		/* first, turn into a generic egg */
		if (otmp->otyp == EGG)
		    kill_egg(otmp);
		else {
		    otmp->otyp = EGG;
		    otmp->owt = weight(otmp);
		}
		otmp->corpsenm = NON_PM;
		otmp->spe = 0;

		/* now change it into something layed by the hero */
		while (tryct--) {
		    mtyp = can_be_hatched(random_monster());
		    if (mtyp != NON_PM && !dead_species(mtyp, TRUE)) {
			otmp->spe = 1;	/* layed by hero */
			otmp->corpsenm = mtyp;
			attach_egg_hatch_timeout(otmp);
			break;
		    }
		}
	}
	/* hypospray ampules randomize the contained potion (and were guaranteed to turn into ampules)  */
	if (obj->otyp == HYPOSPRAY_AMPULE) {
		int hypospray_ampules[] = {POT_GAIN_ABILITY,POT_RESTORE_ABILITY,POT_BLINDNESS,POT_CONFUSION,POT_PARALYSIS,
				POT_SPEED,POT_HALLUCINATION,POT_HEALING,POT_EXTRA_HEALING,POT_GAIN_ENERGY,
				POT_SLEEPING,POT_FULL_HEALING,POT_POLYMORPH,POT_AMNESIA};
		do {
			otmp->ovar1_ampule = (long)ROLL_FROM(hypospray_ampules);
		} while(otmp->ovar1_ampule == obj->ovar1_ampule);
		otmp->spe = obj->spe;
	}
	/* gold scrolls of law turn a small randomize amount of gold (and were guaranteed to turn into gold pieces) */
	if (obj->otyp == SCR_GOLD_SCROLL_OF_LAW) {
		otmp->quan = d(obj->quan,50) + 50 * obj->quan;
	}
	/* crocodile corpses were turned into shoes */
	if (obj->otyp == CORPSE && obj->corpsenm == PM_CROCODILE) {
		otmp->spe = 0;
		otmp->oeroded = 0;
		otmp->oerodeproof = TRUE;
		otmp->cursed = FALSE;
	}

	/* general post-poly changes */
	switch (otmp->oclass) {
	case TOOL_CLASS:
		if (otmp->otyp == CANDLE_OF_INVOCATION) {
			otmp = poly_obj(otmp, WAX_CANDLE);
			otmp->age = 400L;
		}
	    else if (otmp->otyp == MAGIC_LAMP) {
			otmp = poly_obj(otmp, OIL_LAMP);
			otmp->age = 1500L;	/* "best" oil lamp possible */
		}
	    else if (otmp->otyp == MAGIC_TORCH) {
			otmp->otyp = TORCH;
			otmp->age = 1500L;	/* "best" torch possible */
	    } else if (otmp->otyp == MAGIC_MARKER) {
			otmp->recharged = 1;	/* degraded quality */
	    }
	    /* don't care about the recharge count of other tools */
	    break;

	case WAND_CLASS:
	    while (otmp->otyp == WAN_WISHING || otmp->otyp == WAN_POLYMORPH)
			otmp = randpoly_obj(otmp);
	    /* altering the object tends to degrade its quality
	       (analogous to spellbook `read count' handling) */
	    if ((int)otmp->recharged < rn2(7))	/* recharge_limit */
			otmp->recharged++;
	    break;

	case POTION_CLASS:
	    while (otmp->otyp == POT_POLYMORPH)
			otmp = randpoly_obj(otmp);
	    break;

	case SPBOOK_CLASS:
	    while (otmp->otyp == SPE_POLYMORPH)
			otmp = randpoly_obj(otmp);
	    /* reduce spellbook abuse */
		if(otmp->otyp != SPE_BLANK_PAPER) {
			otmp->spestudied = obj->spestudied + 1;
		}
	    break;

	case RING_CLASS:
		while (otmp->otyp == RIN_WISHES)
			otmp = randpoly_obj(otmp);
		break;

	case GEM_CLASS:
	    if (otmp->quan > (long)rnd(4) && obj->obj_material == MINERAL && otmp->obj_material != MINERAL) {
			otmp = poly_obj(otmp, ROCK);	/* transmutation backfired */
			set_obj_quan(otmp, otmp->quan/2); /* some material has been lost */
	    }
	    break;
	}

	delobj(obj);
	return otmp;
}
/*
 * Polymorphs the object to the given object ID.
 * If given STRANGE OBJECT, uses randpoly_obj().
 * 
 * Returns a pointer to the new object.
 */
struct obj *
poly_obj(obj, id)
struct obj * obj;
int id;
{
	struct obj * otmp;
	if (id == STRANGE_OBJECT) {
		impossible("poly_obj called with id=STRANGE_OBJECT?");
		return randpoly_obj(obj);
	}
	otmp = poly_obj_core(obj, id);

	delobj(obj);
	return otmp;
}

/* 
 * the shared core of randpoly_obj and poly_obj 
 * does not destroy obj, caller must do so.
 */
struct obj *
poly_obj_core(obj, id)
struct obj *obj;
int id;
{
	struct obj *otmp;
	xchar ox, oy;
	int obj_location = obj->where;

	if (id == STRANGE_OBJECT) { /* preserve symbol */
			int try_limit = 3;
			/* Try up to 3 times to make the magic-or-not status of
			   the new item be the same as it was for the old one. */
			otmp = (struct obj *)0;
			do {
			if (otmp) delobj(otmp);
			otmp = mkobj(obj->oclass, FALSE);
			} while (--try_limit > 0 &&
			  objects[obj->otyp].oc_magic != objects[otmp->otyp].oc_magic);
	} else {
	    /* literally replace obj with this new thing */
	    otmp = mksobj(id, MKOBJ_NOINIT);
	/* Actually more things use corpsenm but they polymorph differently */
#define USES_CORPSENM(typ) ((typ)==CORPSE || (typ)==STATUE || (typ)==FIGURINE)
	    if (USES_CORPSENM(obj->otyp) && USES_CORPSENM(id))
		otmp->corpsenm = obj->corpsenm;
#undef USES_CORPSENM
	}

	/* preserve quantity, unless otmp cannot be stacked */
	otmp->quan = (objects[otmp->otyp].oc_merge) ? obj->quan : 1;
	/* preserve the shopkeepers (lack of) interest */
	otmp->no_charge = obj->no_charge;
	/* preserve inventory letter if in inventory */
	if (obj_location == OBJ_INVENT)
	    otmp->invlet = obj->invlet;

	/* keep special fields (including charges on wands) */
	if (index(charged_objs, otmp->oclass)) otmp->spe = obj->spe;
	otmp->recharged = obj->recharged;

	otmp->cursed = obj->cursed;
	otmp->blessed = obj->blessed;
	otmp->bknown = obj->bknown;
	otmp->oeroded = obj->oeroded;
	otmp->oeroded2 = obj->oeroded2;
	otmp->ostolen = obj->ostolen;
	otmp->shopOwned = obj->shopOwned;
	otmp->sknown = obj->sknown;
	if (!is_flammable(otmp) && !is_rustprone(otmp)) otmp->oeroded = 0;
	if (!is_corrodeable(otmp) && !is_rottable(otmp)) otmp->oeroded2 = 0;
	if (is_damageable(otmp))
	    otmp->oerodeproof = obj->oerodeproof;

	/* Keep chest/box traps and poisoned ammo if we may */
	if (obj->otrapped && Is_box(otmp) && otmp->otyp != MAGIC_CHEST) otmp->otrapped = TRUE;

	if (obj->opoisoned && is_poisonable(otmp))
		otmp->opoisoned = obj->opoisoned;

	/* no box contents --KAA */
	if (Has_contents(otmp)) delete_contents(otmp);

	/* add focusing gems to lightsabers */
	if (is_lightsaber(otmp)) {
		struct obj *gem = mksobj(rn2(6) ? BLUE_FLUORITE : GREEN_FLUORITE, NO_MKOBJ_FLAGS);
		gem->quan = 1;
		gem->owt = weight(gem);
		add_to_container(otmp, gem);
	}
	//Transfer body type flags. A non-armor item that becomes armor SHOULD end up MB_HUMANOID
	if(is_suit(otmp) || is_shirt(otmp) || is_helmet(otmp)){
		set_obj_shape(otmp, obj->bodytypeflag);
	}

	/* update the weight */
	otmp->owt = weight(otmp);
	/* update the color */
	set_object_color(otmp);

	/* for now, take off worn items being polymorphed */
	if (obj_location == OBJ_INVENT) {
	    if (id == STRANGE_OBJECT)
			remove_worn_item(obj, TRUE);
	    else {
			/* This is called only for stone to flesh.  It's a lot simpler
			* than it otherwise might be.  We don't need to check for
			* special effects when putting them on (no meat objects have
			* any) and only three worn masks are possible.
			*/
			otmp->owornmask = obj->owornmask;
			remove_worn_item(obj, TRUE);
			setworn(otmp, otmp->owornmask);
			if (otmp->owornmask & LEFT_RING)
				uleft = otmp;
			if (otmp->owornmask & RIGHT_RING)
				uright = otmp;
			if (otmp->owornmask & W_WEP)
				uwep = otmp;
			if (otmp->owornmask & W_SWAPWEP)
				uswapwep = otmp;
			if (otmp->owornmask & W_QUIVER)
				uquiver = otmp;
			goto no_unwear;
	    }
	}
	else if (obj_location == OBJ_MINVENT) {
		struct monst * mon = obj->ocarry;
		/* unwield/unwear */
		mon->misc_worn_check &= ~obj->owornmask;
		update_mon_intrinsics(mon, obj, FALSE, FALSE);
		if (obj->owornmask & W_WEP){
			setmnotwielded(mon,obj);
			MON_NOWEP(mon);
		}
		if (obj->owornmask & W_SWAPWEP){
			setmnotwielded(mon,obj);
			MON_NOSWEP(mon);
		}
	}

	/* preserve the mask in case being used by something else */
	otmp->owornmask = obj->owornmask;
no_unwear:

	if (obj_location == OBJ_FLOOR && is_boulder(obj) && !is_boulder(otmp))
	    unblock_point(obj->ox, obj->oy);
	else if (obj_location == OBJ_FLOOR && obj->otyp != BOULDER && otmp->otyp == BOULDER)
		block_point(obj->ox, obj->oy);

	/* copy OX structures */
	mov_all_ox(obj, otmp);
	/* copy not otyp-related timers */
	copy_timer(get_timer(obj->timed, DESUMMON_OBJ), TIMER_OBJECT, (genericptr_t)otmp);

	/* ** we are now done adjusting the object ** */


	/* swap otmp for obj */
	replace_object(obj, otmp);
	if (obj_location == OBJ_INVENT) {
	    /*
	     * We may need to do extra adjustments for the hero if we're
	     * messing with the hero's inventory.  The following calls are
	     * equivalent to calling freeinv on obj and addinv on otmp,
	     * while doing an in-place swap of the actual objects.
	     */
	    freeinv_core(obj);
	    addinv_core1(otmp);
	    addinv_core2(otmp);
	}

	if ((!(carried(otmp) || (otmp->where == OBJ_CONTAINED && carried(otmp->ocontainer))) || obj->unpaid) &&
		get_obj_location(otmp, &ox, &oy, BURIED_TOO|CONTAINED_TOO) &&
		costly_spot(ox, oy)) {
	    register struct monst *shkp =
		shop_keeper(*in_rooms(ox, oy, SHOPBASE));

	    if ((!obj->no_charge ||
		 (Has_contents(obj) &&
		    (contained_cost(obj, shkp, 0L, FALSE, FALSE) != 0L)))
	       && inhishop(shkp)) {
		if(shkp->mpeaceful) {
		    if(*u.ushops && *in_rooms(u.ux, u.uy, 0) ==
			    *in_rooms(shkp->mx, shkp->my, 0) &&
			    !costly_spot(u.ux, u.uy))
			make_angry_shk(shkp, ox, oy);
		    else {
			pline("%s gets angry!", Monnam(shkp));
			hot_pursuit(shkp);
		    }
		} else Norep("%s is furious!", Monnam(shkp));
	    }
	}
	return otmp;
}

/*
 * Object obj was hit by the effect of the wand/spell otmp.  Return
 * non-zero if the wand/spell had any effect.
 */
int
bhito(obj, otmp)
struct obj *obj, *otmp;
{
	int res = 1;	/* affected object by default */
	struct obj *item;
	xchar refresh_x, refresh_y;

	if (obj->bypass) {
		/* The bypass bit is currently only used as follows:
		 *
		 * POLYMORPH - When a monster being polymorphed drops something
		 *             from its inventory as a result of the change.
		 *             If the items fall to the floor, they are not
		 *             subject to direct subsequent polymorphing
		 *             themselves on that same zap. This makes it
		 *             consistent with items that remain in the
		 *             monster's inventory. They are not polymorphed
		 *             either.
		 * UNDEAD_TURNING - When an undead creature gets killed via
		 *	       undead turning, prevent its corpse from being
		 *	       immediately revived by the same effect.
		 *
		 * The bypass bit on all objects is reset each turn, whenever
		 * flags.bypasses is set.
		 *
		 * We check the obj->bypass bit above AND flags.bypasses
		 * as a safeguard against any stray occurrence left in an obj
		 * struct someplace, although that should never happen.
		 */
		if (flags.bypasses)
			return 0;
		else {
#ifdef DEBUG
			pline("%s for a moment.", Tobjnam(obj, "pulsate"));
#endif
			obj->bypass = 0;
		}
	}

	/*
	 * Some parts of this function expect the object to be on the floor
	 * obj->{ox,oy} to be valid.  The exception to this (so far) is
	 * for the STONE_TO_FLESH spell.
	 */
	if (!(obj->where == OBJ_FLOOR || otmp->otyp == SPE_STONE_TO_FLESH))
	    impossible("bhito: obj is not floor or Stone To Flesh spell");

	if (obj == uball) {
		res = 0;
	} else if (obj == uchain) {
		if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK) {
		    unpunish();
		    makeknown(otmp->otyp);
		} else
		    res = 0;
	} else
	switch(otmp->otyp) {
	case WAN_LIGHT:
	case SCR_LIGHT:
	case SPE_LIGHT:
		if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
			obj->otyp == LANTERN || obj->otyp == LANTERN_PLATE_MAIL || obj->otyp == POT_OIL ||
			obj->otyp == DWARVISH_HELM || obj->otyp == GNOMISH_POINTY_HAT ||
			obj->otyp == TALLOW_CANDLE || obj->otyp == WAX_CANDLE) &&
			!((!Is_candle(obj) && obj->age == 0) || (obj->otyp == MAGIC_LAMP && obj->spe == 0))
			&& (!obj->cursed || rn2(2))
			&& !obj->lamplit) {

			// Assumes the player is the only cause of this effect for purposes of shk billing

			if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == LANTERN || obj->otyp == LANTERN_PLATE_MAIL || 
				obj->otyp == DWARVISH_HELM
			) {
				check_unpaid(obj);
			}
			else {
				if (obj->unpaid && costly_spot(obj->ox, obj->oy) &&
					obj->age == 20L * (long)objects[obj->otyp].oc_cost) {
					const char *ithem = obj->quan > 1L ? "them" : "it";
					verbalize("You burn %s, you bought %s!", ithem, ithem);
					bill_dummy_object(obj);
				}
			}
			begin_burn(obj);
		}
		res = 0;
		break;
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
		if (obj->otyp == WAN_POLYMORPH ||
			obj->otyp == SPE_POLYMORPH ||
			obj->otyp == POT_POLYMORPH ||
			obj_resists(obj, 0, 100)) {
		    res = 0;
		    break;
		}
		/* KMH, conduct */
		u.uconduct.polypiles++;
		/* any saved lock context will be dangerously obsolete */
		if (Is_box(obj)) (void) boxlock(obj, otmp);

		if (obj_shudders(obj)) {
		    if (cansee(obj->ox, obj->oy))
			makeknown(otmp->otyp);
		    do_osshock(obj);
		    break;
		}
		obj = randpoly_obj(obj);;
		newsym(obj->ox,obj->oy);
		break;
	case WAN_PROBING:
		res = !obj->dknown;
		/* target object has now been "seen (up close)" */
		obj->dknown = 1;
		if (Is_container(obj) || obj->otyp == STATUE || (obj->otyp == CRYSTAL_SKULL && Insight >= 20)) {
		    if (!obj->cobj)
			pline("%s empty.", Tobjnam(obj, "are"));
		    else {
			struct obj *o;
			/* view contents (not recursively) */
			for (o = obj->cobj; o; o = o->nobj)
			    o->dknown = 1;	/* "seen", even if blind */
			(void) display_cinventory(obj);
		    }
		    res = 1;
		}
		if (res) makeknown(WAN_PROBING);
		break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	case ROD_OF_FORCE:
		if (is_boulder(obj) || obj->otyp == STATUE)
			break_boulder(obj);
		else {
			if (!flags.mon_moving)
			    (void)hero_breaks(obj, obj->ox, obj->oy, FALSE);
			else
			    (void)breaks(obj, obj->ox, obj->oy);
			res = 0;
		}
		/* BUG[?]: shouldn't this depend upon you seeing it happen? */
		makeknown(otmp->otyp);
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		cancel_item(obj);
#ifdef TEXTCOLOR
		newsym(obj->ox,obj->oy);	/* might change color */
#endif
		break;
	case SPE_DRAIN_LIFE:
	case WAN_DRAINING:	/* KMH */
		(void) drain_item(obj);
		if(obj->oartifact == ART_SCORPION_CARAPACE)
			drain_scorpion(obj);
		break;
	case WAN_TELEPORTATION:
	case SPE_TELEPORT_AWAY:
		rloco(obj);
		break;
	case WAN_MAKE_INVISIBLE:
#ifdef INVISIBLE_OBJECTS
		obj->oinvis = TRUE;
		newsym(obj->ox,obj->oy);	/* make object disappear */
#endif
		break;
	case WAN_UNDEAD_TURNING:
	case SPE_TURN_UNDEAD:
		if (obj->otyp == EGG)
			revive_egg(obj);
		else
			res = !!revive(obj, FALSE);
		break;
	case WAN_OPENING:
	case SPE_KNOCK:
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
		if(Is_box(obj))
			res = boxlock(obj, otmp);
		else
			res = 0;
		if (res /* && otmp->oclass == WAND_CLASS */)
			makeknown(otmp->otyp);
		break;
	case WAN_SLOW_MONSTER:		/* no effect on objects */
	case SPE_SLOW_MONSTER:
	case WAN_SPEED_MONSTER:
	case WAN_NOTHING:
	case SPE_HEALING:
	case SPE_EXTRA_HEALING:
	case SPE_FULL_HEALING:
		res = 0;
		break;
	case SPE_STONE_TO_FLESH:
		refresh_x = obj->ox; refresh_y = obj->oy;
		if ((obj->obj_material != MINERAL &&
			 obj->obj_material != SALT &&
			 obj->obj_material != GEMSTONE) ||
			obj->oartifact
		) {
		    res = 0;
		    break;
		}
		/* add more if stone objects are added.. */
		switch (objects[obj->otyp].oc_class) {
		    case ROCK_CLASS:	/* boulders and statues */
			if (obj->otyp == BOULDER) {
			    obj = poly_obj(obj, MASSIVE_CHUNK_OF_MEAT);
			    goto smell;
			} else if (obj->otyp == STATUE) {
			    xchar oox, ooy;

			    (void) get_obj_location(obj, &oox, &ooy, 0);
			    refresh_x = oox; refresh_y = ooy;
			    if (vegetarian(&mons[obj->corpsenm])||
					obj->corpsenm == PM_DJINNI) {
				/* Don't animate monsters that aren't flesh */
	/* drop any objects contained inside the statue */
				while ((item = obj->cobj) != 0) {
				    obj_extract_self(item);
				    place_object(item, obj->ox, obj->oy);
				}
				obj = poly_obj(obj, MEATBALL);
			    	goto smell;
			    }
				struct monst * mtmp;
			    if (!(mtmp = animate_statue(obj, oox, ooy, ANIMATE_SPELL, (int *)0))) {
				struct obj *item;
makecorpse:			if (mons[obj->corpsenm].geno &
							(G_NOCORPSE|G_UNIQ)) {
				    res = 0;
				    break;
				}
				/* Unlikely to get here since genociding
				 * monsters also sets the G_NOCORPSE flag.
				 * Drop the contents, poly_obj looses them.
				 */
				while ((item = obj->cobj) != 0) {
				    obj_extract_self(item);
				    place_object(item, oox, ooy);
				}
				obj = poly_obj(obj, CORPSE);
				break;
			    }
				else {
					/* creature was created */
					if (get_mx(mtmp, MX_ESUM)) {
						/* vanish it */
						monvanished(mtmp);
					}
				}
			} else if (obj->otyp == FOSSIL) {
				int corpsetype = obj->corpsenm;
			    xchar oox, ooy;

			    (void) get_obj_location(obj, &oox, &ooy, 0);
			    refresh_x = oox; refresh_y = ooy;
				/* Don't corpsify monsters that aren't flesh or have corpses */
			    if (vegetarian(&mons[obj->corpsenm])||
					(mons[obj->corpsenm].geno & G_NOCORPSE)) {
					obj = poly_obj(obj, MEATBALL);
			    } else {
					obj = poly_obj(obj, CORPSE);
					if(obj){
						obj->corpsenm = corpsetype;
						fix_object(obj);
					}
				}
				goto smell;
			} else { /* new rock class object... */
			    /* impossible? */
			    res = 0;
			}
			break;
		    case TOOL_CLASS:	/* figurine */
		    {
			struct monst *mon;
			xchar oox, ooy;

			if (obj->otyp != FIGURINE) {
			    res = 0;
			    break;
			}
			if (vegetarian(&mons[obj->corpsenm])) {
			    /* Don't animate monsters that aren't flesh */
			    obj = poly_obj(obj, MEATBALL);
			    goto smell;
			}
			(void) get_obj_location(obj, &oox, &ooy, 0);
			refresh_x = oox; refresh_y = ooy;
			mon = make_familiar(obj, oox, ooy, FALSE);
			if (mon) {
				(void) stop_timer(FIG_TRANSFORM, obj->timed);
			    delobj(obj);
			    if (cansee(mon->mx, mon->my))
				pline_The("figurine animates!");
			    break;
			}
			goto makecorpse;
		    }
		    /* maybe add weird things to become? */
		    case RING_CLASS:	/* some of the rings are stone */
			obj = poly_obj(obj, MEAT_RING);
			goto smell;
		    case WAND_CLASS:	/* marble wand */
			obj = poly_obj(obj, MEAT_STICK);
			goto smell;
		    case GEM_CLASS:	/* rocks & gems */
			obj = poly_obj(obj, MEATBALL);
smell:
			if (herbivorous(youracedata) &&
			    (!carnivorous(youracedata) ||
			     Role_if(PM_MONK) || !u.uconduct.unvegetarian))
			    Norep("You smell the odor of meat.");
			else
			    Norep("You smell a delicious smell.");
			break;
		    case WEAPON_CLASS:	/* crysknife */
		    	/* fall through */
		    default:
			res = 0;
			break;
		}
		newsym(refresh_x, refresh_y);
		break;
	default:
		impossible("What an interesting effect (%d)", otmp->otyp);
		break;
	}
	return res;
}

/* returns nonzero if something was hit */
int
bhitpile(obj,fhito,tx,ty)
    struct obj *obj;
    int FDECL((*fhito), (OBJ_P,OBJ_P));
    int tx, ty;
{
    int hitanything = 0;
    register struct obj *otmp, *next_obj;

    if (obj->otyp == SPE_FORCE_BOLT || obj->otyp == ROD_OF_FORCE || obj->otyp == WAN_STRIKING) {
		struct trap *t = t_at(tx, ty);

		/* We can't settle for the default calling sequence of
		   bhito(otmp) -> break_statue(otmp) -> activate_statue_trap(ox,oy)
		   because that last call might end up operating on our `next_obj'
		   (below), rather than on the current object, if it happens to
		   encounter a statue which mustn't become animated. */
		if (t && t->ttyp == STATUE_TRAP &&
			activate_statue_trap(t, tx, ty, TRUE) && obj->otyp == WAN_STRIKING)
			makeknown(obj->otyp);
    }

    poly_zapped = -1;
    for(otmp = level.objects[tx][ty]; otmp; otmp = next_obj) {
		/* Fix for polymorph bug, Tim Wright */
		next_obj = otmp->nexthere;
		hitanything += (*fhito)(otmp, obj);
    }
    if(poly_zapped >= 0)
		create_polymon(level.objects[tx][ty], poly_zapped);

    return hitanything;
}
#endif /*OVLB*/
#ifdef OVL1

/* returns an int from 0-100 meaning chance to use a charge when zapping
 * 100 => always uses a charge
 * 0   => never uses a charge
 * 
 * if (rn2(100) < zapcost(wand, magr)) spe--;
 */
int
zapcostchance(wand, magr)
struct obj * wand;		/* wand being zapped */
struct monst * magr;	/* creature zapping the wand (if any) */
{
	int base;	/* base chance */
	switch (wand->otyp)
	{
	case WAN_MAGIC_MISSILE:
		base = 10;
		break;
	
	default:
		base = 100;
		break;
	}

	return base;
}

/*
 * zappable - returns 1 if zap is available, 0 otherwise.
 *	      it removes a charge from the wand if zappable.
 * added by GAN 11/03/86
 */
int
zappable(wand)
register struct obj *wand;
{
	if(wand->oclass == WAND_CLASS){
		if(wand->oartifact && wand->spe < 1 && wand->age < moves){
			wand->spe = 1;
			wand->age = moves + (long)(rnz(100)*(Role_if(PM_PRIEST) ? .8 : 1));
		}
		if(wand->spe < 0 || (wand->spe == 0 && (wand->oartifact || rn2(121))))
			return 0;
		if(wand->spe == 0)
			You("wrest one last charge from the worn-out wand.");

		if (rn2(100) < zapcostchance(wand, &youmonst))
			wand->spe--;
		return 1;
	}
	else if(wand->otyp == ROD_OF_FORCE){
		if(wand->age <= 0)
			return 0;
		wand->age = max(wand->age-10000, 0);
		return 1;
	}
	else if(wand->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(wand, IEA_BOLTS)){
		int encost = u.twoweap ? 20 : 10;
		if(!wand->owornmask || u.uen < encost)
			return 0;
		u.uen -= encost;
		flags.botl = TRUE;
		return 1;
	}
	else if(wand->oartifact == ART_STAR_EMPEROR_S_RING){
		if(!wand->owornmask || u.uen < 15)
			return 0;
		u.uen -= 15;
		flags.botl = TRUE;
		return 1;
	}
	return 0;
}

/*
 * zapnodir - zaps a NODIR wand/spell.
 * added by GAN 11/03/86
 */
void
zapnodir(obj)
register struct obj *obj;
{
	boolean known = FALSE;

	switch(obj->otyp) {
		case WAN_LIGHT:
			litroom(TRUE,obj);
			if(u.sealsActive&SEAL_TENEBROUS) unbind(SEAL_TENEBROUS,TRUE);
			if (!Blind) known = TRUE;
		break;
		case SPE_LIGHT:
			if(!Race_if(PM_DROW)){
				litroom(!(obj->cursed),obj);
				if(!(obj->cursed) && u.sealsActive&SEAL_TENEBROUS) unbind(SEAL_TENEBROUS,TRUE);
			} else {
				litroom((obj->cursed), obj);
				if((obj->cursed) && u.sealsActive&SEAL_TENEBROUS) unbind(SEAL_TENEBROUS,TRUE);
			}
			if (!Blind) known = TRUE;
		break;
		case WAN_DARKNESS:
			litroom(FALSE,obj);
			if (!Blind) known = TRUE;
		break;
		case WAN_SECRET_DOOR_DETECTION:
		case SPE_DETECT_UNSEEN:
			if(!findit()) return;
			if (!Blind) known = TRUE;
		break;
		case WAN_CREATE_MONSTER:
			if(!DimensionalLock){
				known = create_critters(rn2(23) ? 1 : rn1(7,2),
						(struct permonst *)0);
			}
			else {
				pline("Unfortunately, nothing happens.");
			}
		break;
		case WAN_WISHING:
			known = TRUE;
			if(Luck + rn2(5) < 0) {
				pline("Unfortunately, nothing happens.");
				break;
			}
			makewish(0);	// does not allow artifact wishes
		break;
		case WAN_ENLIGHTENMENT:
			known = TRUE;
			You_feel("self-knowledgeable...");
			display_nhwindow(WIN_MESSAGE, FALSE);
			doenlightenment();
			pline_The("feeling subsides.");
			exercise(A_WIS, TRUE);
		break;
		case SPE_HASTE_SELF:
			if(!Very_fast) /* wwf@doe.carleton.ca */
				You("are suddenly moving %sfaster.",
					Fast ? "" : "much ");
			else {
				Your("%s get new energy.",
					makeplural(body_part(LEG)));
			}
			exercise(A_DEX, TRUE);
			incr_itimeout(&HFast, rn1(10, 100 + 60 * bcsign(obj)));
		break;
		default:
			if(obj->otyp == SPE_FULL_HEALING){
				objects[SPE_FULL_HEALING].oc_dir = IMMEDIATE;
				pline("Bad full healing zap dir detected and fixed.");
				break;
			}
			pline("Bad zapnodir item: %d", obj->otyp);
		break;
	}
	if (known && !objects[obj->otyp].oc_name_known) {
		makeknown(obj->otyp);
		more_experienced(0,10);
	}
}
#endif /*OVL1*/
#ifdef OVL0

STATIC_OVL void
backfire(otmp)
struct obj *otmp;
{
	otmp->in_use = TRUE;	/* in case losehp() is fatal */
	pline("%s suddenly explodes!", The(xname(otmp)));
	losehp(d(otmp->spe+2,6), "exploding wand", KILLED_BY_AN);
	useup(otmp);
}

static NEARDATA const char zap_syms[] = { ARMOR_CLASS, WAND_CLASS, TOOL_CLASS, RING_CLASS, 0 };

int
dozap()
{
	register struct obj *obj;
	int damage;

	if(check_capacity((char *)0)) return MOVE_CANCELLED;
	obj = getobj(zap_syms, "zap");
	if(!obj) return MOVE_CANCELLED;

	if(on_level(&spire_level,&u.uz)){
		pline1(nothing_happens);
		return MOVE_ZAPPED;
	}

	check_unpaid(obj);

	/* zappable addition done by GAN 11/03/86 */
	if(!zappable(obj)) pline1(nothing_happens);
	else if(obj->cursed && obj->oclass == WAND_CLASS && !obj->oartifact && !rn2(100)) {
		backfire(obj);	/* the wand blows up in your face! */
		exercise(A_STR, FALSE);
		return MOVE_ZAPPED;
	} else if(!(objects[obj->otyp].oc_dir == NODIR) && !getdir((char *)0)) {
		if (!Blind)
		    pline("%s glows and fades.", The(xname(obj)));
		/* make him pay for knowing !NODIR */
	} else if(!u.dx && !u.dy && !u.dz && !(objects[obj->otyp].oc_dir == NODIR)) {
	    if ((damage = zapyourself(obj, TRUE)) != 0) {
			char buf[BUFSZ];
			Sprintf(buf, "zapped %sself with a wand", uhim());
			losehp(damage, buf, NO_KILLER_PREFIX);
	    }
	} else {
		/*	Are we having fun yet?
		 * weffects -> buzz(obj->otyp) -> zhitm (temple priest) ->
		 * attack -> hitum -> known_hitum -> ghod_hitsu ->
		 * buzz(AD_ELEC) -> destroy_item(WAND_CLASS) ->
		 * useup -> obfree -> dealloc_obj -> free(obj)
		 */
		current_wand = obj;
		weffects(obj);
		obj = current_wand;
		current_wand = 0;
	}
	if (obj && obj->oclass == WAND_CLASS && !obj->oartifact && obj->spe < 0) {
	    pline("%s to dust.", Tobjnam(obj, "turn"));
	    useup(obj);
	}
	update_inventory();	/* maybe used a charge */
	return MOVE_ZAPPED;
}

int
zapyourself(obj, ordinary)
struct obj *obj;
boolean ordinary;
{
	int	damage = 0;
	char buf[BUFSZ];

	switch(obj->otyp) {
		case WAN_STRIKING:
		    makeknown(WAN_STRIKING);
		case SPE_FORCE_BOLT:
		case ROD_OF_FORCE:
			if (uwep && uwep->otyp == ROD_OF_FORCE) {
				You_hear("a rushing sound.");
				uwep->age = min(uwep->age+10000, LIGHTSABER_MAX_CHARGE);
			}
			else if (uswapwep && uswapwep->otyp == ROD_OF_FORCE) {
				You_hear("a rushing sound.");
				uswapwep->age = min(uswapwep->age+10000, LIGHTSABER_MAX_CHARGE);
			}
		    else if(Antimagic) {
				shieldeff(u.ux, u.uy);
				pline("Boing!");
		    } else {
			if (ordinary) {
			    You("bash yourself!");
			    damage = d(2,12);
			} else
			    damage = d(1 + obj->spe,6);
			exercise(A_STR, FALSE);
		    }
		    break;

		case WAN_LIGHTNING:
		    makeknown(WAN_LIGHTNING);
		case SPE_LIGHTNING_BOLT:
		case SPE_LIGHTNING_STORM:
		    if (!Shock_resistance) {
				You("shock yourself!");
				damage = d(12,6);
				exercise(A_CON, FALSE);
		    } else {
				shieldeff(u.ux, u.uy);
				You("zap yourself, but seem unharmed.");
				ugolemeffects(AD_ELEC, d(12,6));
		    }
			if(!UseInvShock_res(&youmonst)){
				destroy_item(&youmonst, WAND_CLASS, AD_ELEC);
				destroy_item(&youmonst, RING_CLASS, AD_ELEC);
			}
		    if (!resists_blnd(&youmonst)) {
				lightning_blind(&youmonst, rnd(50));
		    }
		    break;
		case SPE_FIREBALL:
		case SPE_FIRE_STORM:
		    You("explode a fireball on top of yourself!");
		    explode(u.ux, u.uy, AD_FIRE, WAND_CLASS, d(6,6), EXPL_FIERY, 1);
			if(u.explosion_up){
				int ex, ey;
				for(int i = 0; i < u.explosion_up; i++){
					ex = u.ux + rn2(3) - 1;
					ey = u.uy + rn2(3) - 1;
					if(isok(ex, ey) && ZAP_POS(levl[ex][ey].typ))
						explode(ex, ey, AD_FIRE, WAND_CLASS, d(6,6), EXPL_FIERY, 1);
					else
						explode(u.ux, u.uy, AD_FIRE, WAND_CLASS, d(6,6), EXPL_FIERY, 1);
				}
			}
		    break;
		case WAN_FIRE:
		    makeknown(WAN_FIRE);
		case FIRE_HORN:
		    if (Fire_resistance) {
				shieldeff(u.ux, u.uy);
				You_feel("rather warm.");
				ugolemeffects(AD_FIRE, d(12,6));
		    } else {
				pline("You've set yourself afire!");
				damage = d(12,6);
		    }
			if(!UseInvFire_res(&youmonst)){
				destroy_item(&youmonst, SCROLL_CLASS, AD_FIRE);
				destroy_item(&youmonst, POTION_CLASS, AD_FIRE);
				destroy_item(&youmonst, SPBOOK_CLASS, AD_FIRE);
			}
		    burn_away_slime();
		    melt_frozen_air();
		    (void) burnarmor(&youmonst, FALSE);
		    break;

		case WAN_COLD:
		    makeknown(WAN_COLD);
		case SPE_CONE_OF_COLD:
		case SPE_BLIZZARD:
		case FROST_HORN:
		    if (Cold_resistance) {
				shieldeff(u.ux, u.uy);
				You_feel("a little chill.");
				ugolemeffects(AD_COLD, d(12,6));
		    } else {
				You("imitate a popsicle!");
				damage = d(12,6);
		    }
			roll_frigophobia();
			if(!UseInvCold_res(&youmonst)){
				destroy_item(&youmonst, POTION_CLASS, AD_COLD);
			}
		    break;
		case SPE_ACID_SPLASH:
		    You("splash acid on top of yourself!");
		    explode(u.ux, u.uy, AD_ACID, WAND_CLASS, d(6,6), EXPL_NOXIOUS, 1);
		    break;

		case WAN_MAGIC_MISSILE:
		    makeknown(WAN_MAGIC_MISSILE);
		case SPE_MAGIC_MISSILE:
		    if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The("missiles bounce!");
		    } else {
			damage = d(4,6);
			pline("Idiot!  You've shot yourself!");
		    }
		    break;
		case IMPERIAL_ELVEN_GAUNTLETS:
			if(u.ualign.record > 3){
				damage = d(max(1, 1+(obj->spe+1)/2),8);
				if(hates_holy(youracedata))
					damage *= 2;
				pline("Idiot!  You've shot yourself!");
			}
			else if(u.ualign.record < -3){
				damage = d(max(1, 1+(obj->spe+1)/2),8);
				if(hates_unholy(youracedata))
					damage *= 2;
				pline("Idiot!  You've shot yourself!");
			}
			else {
				if(Antimagic) {
					shieldeff(u.ux, u.uy);
					pline_The("missiles bounce!");
				} else {
				damage = d(max(1, 1+(obj->spe+1)/2),4);
					pline("Idiot!  You've shot yourself!");
				}
			}
		break;
		case WAN_POLYMORPH:
		    if (!Unchanging)
		    	makeknown(WAN_POLYMORPH);
		case SPE_POLYMORPH:
		    if (!Unchanging)
		    	polyself(FALSE);
		    break;

		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		    (void) cancel_monst(&youmonst, obj, TRUE, FALSE, TRUE,0);
		    break;
		case WAN_DRAINING:	/* KMH */
		case SPE_DRAIN_LIFE:
			if (!Drain_resistance) {
				losexp("life drainage",TRUE,FALSE,FALSE);
				makeknown(obj->otyp);
			} else {
				shieldeff(u.ux, u.uy);
			}
			damage = 0;	/* No additional damage */
			break;

		case WAN_MAKE_INVISIBLE: {
		    /* have to test before changing HInvis but must change
		     * HInvis before doing newsym().
		     */
		    int msg = !Invis && !Blind && !BInvis;

		    if (BInvis && (is_mummy_wrap(uarmc))){
			/* A mummy wrapping absorbs it and protects you */
		        You_feel("rather itchy under your %s.", xname(uarmc));
		        break;
		    }
		    if (ordinary || !rn2(10)) {	/* permanent */
				HInvis |= TIMEOUT_INF;
		    } else {			/* temporary */
		    	incr_itimeout(&HInvis, d(obj->spe, 250));
		    }
		    if (msg) {
			makeknown(WAN_MAKE_INVISIBLE);
			newsym(u.ux, u.uy);
			self_invis_message();
		    }
		    break;
		}

		case WAN_SPEED_MONSTER:
			if(!Very_fast) makeknown(WAN_SPEED_MONSTER);
//		case SPE_HASTE_SELF:
			if(!Very_fast) /* wwf@doe.carleton.ca */
				You("are suddenly moving %sfaster.",
					Fast ? "" : "much ");
			else {
				Your("%s get new energy.",
					makeplural(body_part(LEG)));
			}
			exercise(A_DEX, TRUE);
			incr_itimeout(&HFast, rn1(10, 100 + 60 * bcsign(obj)));
		break;

		case WAN_SLEEP:
		    makeknown(WAN_SLEEP);
		case SPE_SLEEP:
		    if(Sleep_resistance) {
			shieldeff(u.ux, u.uy);
			You("don't feel sleepy!");
		    } else {
			pline_The("sleep ray hits you!");
			fall_asleep(-rnd(50), TRUE);
		    }
		    break;

		case WAN_SLOW_MONSTER:
		case SPE_SLOW_MONSTER:
		    if(HFast & (TIMEOUT | INTRINSIC)) {
			u_slow_down();
			makeknown(obj->otyp);
		    }
		    break;

		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    tele();
		    break;

		case WAN_DEATH:
		case SPE_FINGER_OF_DEATH:
			//Shooting yourself with a death effect while inside a Circle of Acheron doesn't protect you, since the spell originates inside the ward.
			if(u.sealsActive&SEAL_BUER) unbind(SEAL_BUER,TRUE);
		    if (nonliving(youracedata) || is_demon(youracedata) || is_angel(youracedata)) {
			pline((obj->otyp == WAN_DEATH) ?
			  "The wand shoots an apparently harmless beam at you."
			  : "You seem no deader than before.");
			break;
		    } else if(u.sealsActive&SEAL_OSE || resists_death(&youmonst)){
				(obj->otyp == WAN_DEATH) ? 
					pline("The wand shoots an apparently harmless beam at you."):
					You("shoot yourself with an apparently harmless beam.");
			}
		    Sprintf(buf, "shot %sself with a death ray", uhim());
		    killer = buf;
		    killer_format = NO_KILLER_PREFIX;
		    You("irradiate yourself with pure energy!");
		    You("die.");
		    makeknown(obj->otyp);
			/* They might survive with an amulet of life saving */
		    done(DIED);
		    break;
		case SPE_POISON_SPRAY:
		    if (nonliving(youracedata) || Poison_resistance) {
				You("shoot yourself with an apparently harmless spray of droplets.");
				break;
			}
		    Sprintf(buf, "shot %sself with a posion spray", uhim());
		    killer = buf;
		    killer_format = NO_KILLER_PREFIX;
		    You("mist yourself with pure poison!");
		    You("die.");
		    makeknown(obj->otyp);
			/* They might survive with an amulet of life saving */
		    done(DIED);
		    break;
		case WAN_UNDEAD_TURNING:
		    makeknown(WAN_UNDEAD_TURNING);
		case SPE_TURN_UNDEAD:
		    (void) unturn_dead(&youmonst);
		    if (is_undead(youracedata)) {
			You_feel("frightened and %sstunned.",
			     Stunned ? "even more " : "");
			make_stunned(HStun + rnd(30), FALSE);
		    } else
			You("shudder in dread.");
		    break;
		case SPE_FULL_HEALING:
			if (Sick) You("are no longer ill.");
			if (Slimed) {
				pline_The("slime disappears!");
				Slimed = 0;
			 /* flags.botl = 1; -- healup() handles this */
			}
			healup(50*P_SKILL(P_HEALING_SPELL), 0, TRUE, TRUE);
			break;
		case SPE_HEALING:
		case SPE_EXTRA_HEALING:
		case SPE_MASS_HEALING:
		{
			int dice = (obj->otyp == SPE_EXTRA_HEALING) ? (6+P_SKILL(P_HEALING_SPELL)) : 6;
			if(uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_HEALING_H)
				dice *= 2;
		    healup((d(dice, obj->otyp != SPE_HEALING ? 8 : 4) + 6*(P_SKILL(P_HEALING_SPELL)-1)),
			   0, FALSE, (obj->otyp != SPE_HEALING));
		    You_feel("%sbetter.",
				obj->otyp != SPE_HEALING ? "much " : "");
		    break;
		}
		case WAN_DARKNESS:	/* (broken wand) */
		 /* assert( !ordinary ); */
		    damage = d(obj->spe, 25);
		break;
		case WAN_LIGHT:	/* (broken wand) */
		 /* assert( !ordinary ); */
		    damage = d(obj->spe, 25);
#ifdef TOURIST
		case EXPENSIVE_CAMERA:
#endif
		    damage += rnd(25);
		    if (!resists_blnd(&youmonst)) {
			You(are_blinded_by_the_flash);
			make_blinded((long)damage, FALSE);
			makeknown(obj->otyp);
			if (!Blind) Your1(vision_clears);
		    }
		    damage = 0;	/* reset */
		    break;
		case WAN_OPENING:
		    if (Punished) makeknown(WAN_OPENING);
		case SPE_KNOCK:
		    if (Punished) Your("chain quivers for a moment.");
		    break;
		case WAN_DIGGING:
		case SPE_DIG:
		case SPE_DETECT_UNSEEN:
		case WAN_NOTHING:
		case WAN_LOCKING:
		case SPE_WIZARD_LOCK:
		    break;
		case WAN_PROBING:
		    for (obj = invent; obj; obj = obj->nobj)
			obj->dknown = 1;
		    /* note: `obj' reused; doesn't point at wand anymore */
		    makeknown(WAN_PROBING);
		    ustatusline();
		    break;
		case SPE_STONE_TO_FLESH:
		    {
		    struct obj *otemp, *onext;
		    boolean didmerge;

		    if (u.umonnum == PM_STONE_GOLEM)
			(void) polymon(PM_FLESH_GOLEM);
		    if (Stoned) fix_petrification();	/* saved! */
		    /* but at a cost.. */
		    for (otemp = invent; otemp; otemp = onext) {
			onext = otemp->nobj;
			(void) bhito(otemp, obj);
			}
		    /*
		     * It is possible that we can now merge some inventory.
		     * Do a higly paranoid merge.  Restart from the beginning
		     * until no merges.
		     */
		    do {
			didmerge = FALSE;
			for (otemp = invent; !didmerge && otemp; otemp = otemp->nobj)
			    for (onext = otemp->nobj; onext; onext = onext->nobj)
			    	if (merged(&otemp, &onext)) {
			    		didmerge = TRUE;
			    		break;
			    		}
		    } while (didmerge);
		    }
		    break;
		default: impossible("object %d used?",obj->otyp);
		    break;
	}
	return(damage);
}

#ifdef STEED
/* you've zapped a wand downwards while riding
 * Return TRUE if the steed was hit by the wand.
 * Return FALSE if the steed was not hit by the wand.
 */
STATIC_OVL boolean
zap_steed(obj)
struct obj *obj;	/* wand or spell */
{
	int steedhit = FALSE;
	
	switch (obj->otyp) {

	   /*
	    * Wands that are allowed to hit the steed
	    * Carefully test the results of any that are
	    * moved here from the bottom section.
	    */
		case WAN_PROBING:
		    probe_monster(u.usteed);
		    makeknown(WAN_PROBING);
		    steedhit = TRUE;
		    break;
		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    /* you go together */
		    tele();
		    if(Teleport_control || !couldsee(u.ux0, u.uy0) ||
			(distu(u.ux0, u.uy0) >= 16))
				makeknown(obj->otyp);
		    steedhit = TRUE;
		    break;

		/* Default processing via bhitm() for these */
		case WAN_MAKE_INVISIBLE:
		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		case WAN_POLYMORPH:
		case SPE_POLYMORPH:
		case WAN_STRIKING:
		case SPE_FORCE_BOLT:
		case ROD_OF_FORCE:
		case WAN_SLOW_MONSTER:
		case SPE_SLOW_MONSTER:
		case WAN_SPEED_MONSTER:
		case SPE_HEALING:
		case SPE_EXTRA_HEALING:
		case SPE_FULL_HEALING:
		case WAN_DRAINING:
		case SPE_DRAIN_LIFE:
		case WAN_OPENING:
		case SPE_KNOCK:
		    (void) bhitm(u.usteed, obj);
		    steedhit = TRUE;
		    break;

		default:
		    steedhit = FALSE;
		    break;
	}
	return steedhit;
}
#endif

#endif /*OVL0*/
#ifdef OVL3

/*
 * cancel a monster (possibly the hero).  inventory is cancelled only
 * if the monster is zapping itself directly, since otherwise the
 * effect is too strong.  currently non-hero monsters do not zap
 * themselves with cancellation.
 * Monster gaze attacks pass in the number of invetory items to cancel
 */
boolean
cancel_monst(mdef, obj, youattack, allow_cancel_kill, self_cancel, gaze_cancel)
register struct monst	*mdef;
register struct obj	*obj;
boolean			youattack, allow_cancel_kill, self_cancel;
int gaze_cancel;
{
	boolean	youdefend = (mdef == &youmonst);
	static const char writing_vanishes[] =
				"Some writing vanishes from %s head!";
	static const char your[] = "your";	/* should be extern */

	if (youdefend ? (!youattack && Antimagic)
		      : resist(mdef, obj ? obj->oclass : WAND_CLASS, 0, TELL)){
		if(cansee(mdef->mx,mdef->my)) shieldeff(mdef->mx, mdef->my);
		return FALSE;	/* resisted cancellation */
	}

	if (self_cancel) {	/* 1st cancel inventory */
	    struct obj *otmp;

	    for (otmp = (youdefend ? invent : mdef->minvent);
			    otmp; otmp = otmp->nobj) cancel_item(otmp);
	    if (youdefend) {
			flags.botl = 1;	/* potential AC change */
			find_ac();
	    }
	}else if(gaze_cancel){
	    struct obj *otmp;
		int invsize = 0, i=0, j=0;
		//int canThese[gaze_cancel];
		
		for(i=0;i<gaze_cancel;i++){//canThese[i]=rn2(invsize); would be better
			invsize = 0;
			for (otmp = (youdefend ? invent : mdef->minvent);
					otmp; otmp = otmp->nobj) invsize++;
			if (invsize){
				otmp = (youdefend ? invent : mdef->minvent);
				for (j = rn2(invsize); j >= 0; j--){
					if (!j) cancel_item(otmp);
					otmp = otmp->nobj;
				}
			}
		}
		
	    if (youdefend) {
			flags.botl = 1;	/* potential AC change */
			find_ac();
	    }
	}

	/* now handle special cases */
	if (youdefend) {
	    if (Upolyd) {
		if (( (u.umonnum == PM_CLAY_GOLEM) || (u.umonnum == PM_SPELL_GOLEM) ) && !Blind)
		    pline(writing_vanishes, your);

		if (Unchanging)
		    Your("amulet grows hot for a moment, then cools.");
		else
		    rehumanize();
	    }
		losepw(d(10,10));
		if(!Race_if(PM_INCANTIFIER)){
			u.uenbonus -= 10;
			calc_total_maxen();
		}
	} else {
	    set_mcan(mdef, TRUE);

	    if (is_were(mdef->data) && mdef->data->mlet != S_HUMAN)
		were_change(mdef);

	    if (mdef->mtyp == PM_CLAY_GOLEM || mdef->mtyp == PM_SPELL_GOLEM) {
		if (canseemon(mdef))
		    pline(writing_vanishes, s_suffix(mon_nam(mdef)));

		if (allow_cancel_kill) {
		    if (youattack)
			killed(mdef);
		    else
			monkilled(mdef, "", AD_SPEL);
		}
	    }
	}
	return TRUE;
}

/* you've zapped an immediate type wand up or down */
STATIC_OVL boolean
zap_updown(obj)
struct obj *obj;	/* wand or spell */
{
	boolean striking = FALSE, disclose = FALSE;
	int x, y, xx, yy, ptmp;
	struct obj *otmp;
	struct engr *e;
	struct trap *ttmp;
	char buf[BUFSZ];

	/* some wands have special effects other than normal bhitpile */
	/* drawbridge might change <u.ux,u.uy> */
	x = xx = u.ux;	/* <x,y> is zap location */
	y = yy = u.uy;	/* <xx,yy> is drawbridge (portcullis) position */
	ttmp = t_at(x, y); /* trap if there is one */

	switch (obj->otyp) {
	case WAN_PROBING:
	    ptmp = 0;
	    if (u.dz < 0) {
		You("probe towards the %s.", ceiling(x,y));
	    } else {
		ptmp += bhitpile(obj, bhito, x, y);
		You("probe beneath the %s.", surface(x,y));
		ptmp += display_binventory(x, y, TRUE);
	    }
	    if (!ptmp) Your("probe reveals nothing.");
	    return TRUE;	/* we've done our own bhitpile */
	case WAN_OPENING:
	case SPE_KNOCK:
	    /* up or down, but at closed portcullis only */
	    if (is_db_wall(x,y) && find_drawbridge(&xx, &yy)) {
		open_drawbridge(xx, yy);
		disclose = TRUE;
	    } else if (u.dz > 0 && (x == xdnstair && y == ydnstair) &&
			/* can't use the stairs down to quest level 2 until
			   leader "unlocks" them; give feedback if you try */
			on_level(&u.uz, &qstart_level) && !ok_to_quest()) {
		pline_The("stairs seem to ripple momentarily.");
		disclose = TRUE;
	    }
	    break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	case ROD_OF_FORCE:
	    striking = TRUE;
	    /*FALLTHRU*/
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    /* down at open bridge or up or down at open portcullis */
	    if ((levl[x][y].typ == DRAWBRIDGE_DOWN) ? (u.dz > 0) :
			(is_drawbridge_wall(x,y) >= 0 && !is_db_wall(x,y)) &&
		    find_drawbridge(&xx, &yy)) {
		if (!striking)
		    close_drawbridge(xx, yy);
		else
		    destroy_drawbridge(xx, yy);
		disclose = TRUE;
	    } else if (striking && u.dz < 0 && rn2(3) &&
			!Weightless && !Is_waterlevel(&u.uz) &&
			!Underwater && !In_outdoors(&u.uz)) {
		/* similar to zap_dig() */
		pline("A rock is dislodged from the %s and falls on your %s.",
		      ceiling(x, y), body_part(HEAD));
		losehp(rnd((uarmh && is_hard(uarmh)) ? 2 : 6),
		       "falling rock", KILLED_BY_AN);
		if ((otmp = mksobj_at(ROCK, x, y, MKOBJ_NOINIT)) != 0) {
		    (void)xname(otmp);	/* set dknown, maybe bknown */
		    stackobj(otmp);
		}
		newsym(x, y);
	    } else if (!striking && ttmp && ttmp->ttyp == TRAPDOOR && u.dz > 0) {
		if (!Blind) {
			if (ttmp->tseen) {
				pline("A trap door beneath you closes up then vanishes.");
				disclose = TRUE;
			} else {
				You("see a swirl of %s beneath you.",
					is_ice(x,y) ? "frost" : "dust");
			}
		} else {
			You_hear("a twang followed by a thud.");
		}
		deltrap(ttmp);
		ttmp = (struct trap *)0;
		newsym(x, y);
	    }
	    break;
	case SPE_STONE_TO_FLESH:
	    if (Weightless || Is_waterlevel(&u.uz) ||
		     Underwater || (Is_qstart(&u.uz) && u.dz < 0)) {
		pline1(nothing_happens);
	    } else if (u.dz < 0) {	/* we should do more... */
		pline("Blood drips on your %s.", body_part(FACE));
	    } else if (u.dz > 0 && !OBJ_AT(u.ux, u.uy)) {
		/*
		Print this message only if there wasn't an engraving
		affected here.  If water or ice, act like waterlevel case.
		*/
		e = engr_at(u.ux, u.uy);
		if (!(e && e->engr_type == ENGRAVE)) {
		    if (is_pool(u.ux, u.uy, FALSE) || is_ice(u.ux, u.uy))
			pline1(nothing_happens);
		    else if (IS_PUDDLE(levl[u.ux][u.uy].typ))
			    pline("The water at your %s turns slightly %s.",
				makeplural(body_part(FOOT)), hcolor(NH_RED));
			else pline("Blood %ss %s your %s.",
			      is_lava(u.ux, u.uy) ? "boil" : "pool",
			      Levitation ? "beneath" : "at",
			      makeplural(body_part(FOOT)));
		}
	    }
	    break;
	default:
	    break;
	}

	if (u.dz > 0) {
	    /* zapping downward */
	    (void) bhitpile(obj, bhito, x, y);

	    if (ttmp) {
			switch (obj->otyp) {
			case WAN_CANCELLATION:
			case SPE_CANCELLATION: 
				/* MRKR: Disarm magical traps */
				/* from an idea posted to rgrn by Haakon Studebaker */
				/* code by Malcolm Ryan */
				if (ttmp->ttyp == MAGIC_TRAP || 
				ttmp->ttyp == TELEP_TRAP ||
				ttmp->ttyp == LEVEL_TELEP || 
				ttmp->ttyp == POLY_TRAP) {
				
				if (ttmp->tseen) {
					You("disarm a %s.", 
						defsyms[trap_to_defsym(ttmp->ttyp)].explanation);
					//Charitably assume that if the PC doesn't know the trap is there, this was an accident.
					if(u.specialSealsActive&SEAL_YOG_SOTHOTH)
						unbind(SEAL_SPECIAL|SEAL_YOG_SOTHOTH, TRUE);
				}
				deltrap(ttmp);
				}
				break;
			default:
				break;
			}
	    }	    /* subset of engraving effects; none sets `disclose' */
	    if ((e = engr_at(x, y)) != 0 && e->engr_type != HEADSTONE) {
		switch (obj->otyp) {
		case WAN_POLYMORPH:
		case SPE_POLYMORPH:
		    del_engr(e);
		    make_engr_at(x, y, random_engraving(buf), moves, (xchar)0);
		    break;
		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		case WAN_MAKE_INVISIBLE:
		    del_engr(e);
		    break;
		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    rloc_engr(e);
		    break;
		case SPE_STONE_TO_FLESH:
		    if (e->engr_type == ENGRAVE) {
			/* only affects things in stone */
			pline_The(Hallucination ?
			    "floor runs like butter!" :
			    "edges on the floor get smoother.");
			wipe_engr_at(x, y, d(2,4));
			}
		    break;
		case WAN_STRIKING:
		case SPE_FORCE_BOLT:
		case ROD_OF_FORCE:
		    wipe_engr_at(x, y, d(2,4));
		    break;
		default:
		    break;
		}
	    }
	}

	return disclose;
}

#endif /*OVL3*/
#ifdef OVLB

/* called for various wand and spell effects - M. Stephenson */
//the u.d_ variables should have been set via a call to getdir before this is called. -D_E
void
weffects(obj)
register struct	obj	*obj;
{
	int otyp = obj->otyp;
	boolean disclose = FALSE, was_unkn = !objects[otyp].oc_name_known;

	exercise(A_WIS, TRUE);
#ifdef STEED
	if (u.usteed && (objects[otyp].oc_dir != NODIR) &&
	    !u.dx && !u.dy && (u.dz > 0) && zap_steed(obj)) {
		disclose = TRUE;
	} else
#endif
	if (objects[otyp].oc_dir == IMMEDIATE) {
	    obj_zapped = FALSE;

		/* Death magic is impure */
		if(otyp == SPE_DRAIN_LIFE || otyp == WAN_DRAINING ){
			IMPURITY_UP(u.uimp_death_magic)
		}

	    if (u.uswallow) {
		(void) bhitm(u.ustuck, obj);
		/* [how about `bhitpile(u.ustuck->minvent)' effect?] */
	    } else if (u.dz) {
		disclose = zap_updown(obj);
	    } else {
		(void) bhit(u.dx,u.dy, rn1(8,6),ZAPPED_WAND, bhitm,bhito, obj, NULL);
	    }
	    /* give a clue if obj_zapped */
	    if (obj_zapped)
		You_feel("shuddering vibrations.");

	} else if (objects[otyp].oc_dir == NODIR) {
	    zapnodir(obj);

	} else {
		struct zapdata zapdat = { 0 };
		int range = rn1(7, 7);
	    /* neither immediate nor directionless */

		/* Wands of death are impure, unbind buer */
		if(otyp == SPE_FINGER_OF_DEATH || otyp == WAN_DEATH ){
			IMPURITY_UP(u.uimp_death_magic)
			if(u.sealsActive&SEAL_BUER)
				unbind(SEAL_BUER,TRUE);
		}
		
	    if (otyp == WAN_DIGGING || otyp == SPE_DIG)
			zap_dig(-1,-1,-1);//-1-1-1 = "use defaults"
		else if(otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(obj, IEA_BOLTS)){
			basiczap(&zapdat, 0, ZAP_WAND, 1);
			zapdat.damn = max(1, P_SKILL(P_WAND_POWER)+(obj->spe+1)/2);
			if(u.twoweap)
				zapdat.damn *= 2;
			zapdat.affects_floor = FALSE;
			if(u.ualign.record > 3){
				zapdat.damd = 8;
				zapdat.adtyp = AD_HOLY;
				zapdat.phase_armor = TRUE;
			}
			else if(u.ualign.record < -3){
				zapdat.damd = 8;
				zapdat.adtyp = AD_UNHY;
				zapdat.phase_armor = TRUE;
			}
			else {
				zapdat.damd = 4;
				zapdat.adtyp = AD_MAGM;
			}
			use_skill(P_WAND_POWER, 1);
			zap(&youmonst, u.ux, u.uy, u.dx, u.dy, range, &zapdat);
		}
		else if(obj->oartifact == ART_STAR_EMPEROR_S_RING){
			basiczap(&zapdat, AD_STAR, ZAP_WAND, wand_damage_die(P_SKILL(P_WAND_POWER)));
			zapdat.unreflectable = ZAP_REFL_NEVER;
			zapdat.damd = 8;
			zapdat.affects_floor = FALSE;
			use_skill(P_WAND_POWER, 3);
			zap(&youmonst, u.ux, u.uy, u.dx, u.dy, range, &zapdat);
		}
	    else if (otyp >= SPE_MAGIC_MISSILE && otyp <= SPE_ACID_SPLASH){
			basiczap(&zapdat, spell_adtype(otyp), ZAP_SPELL, u.ulevel / 2 + 1);
			/* some spells are special */
			switch (otyp) {
			case SPE_MAGIC_MISSILE:
				zapdat.single_target = TRUE;
				if(otyp == SPE_MAGIC_MISSILE){ 
					if(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WRATH)
						zapdat.damd += 2;
					if(YogSpell && !YOG_BAD){
						zapdat.adtyp = AD_PHYS;
						zapdat.unreflectable = ZAP_REFL_NEVER;
						zapdat.always_hits = TRUE;
					}
				}
				break;
			case SPE_FIREBALL:
				zapdat.explosive = TRUE;
				zapdat.single_target = TRUE;
				zapdat.directly_hits = FALSE;
				zapdat.affects_floor = FALSE;
				zapdat.no_hit_wall = TRUE;
				zapdat.damn *= 1.5;
				break;
			case SPE_ACID_SPLASH:
				range = 1;
				zapdat.splashing = TRUE;
				zapdat.unreflectable = ZAP_REFL_NEVER;
				zapdat.directly_hits = FALSE;
				zapdat.affects_floor = FALSE;
				zapdat.no_bounce = TRUE;
				break;
			case SPE_POISON_SPRAY:
				zapdat.no_bounce = TRUE;
				break;
			}
			zap(&youmonst, u.ux, u.uy, u.dx, u.dy, range, &zapdat);

	    } else if (otyp >= WAN_MAGIC_MISSILE && otyp <= WAN_LIGHTNING){
			basiczap(&zapdat, wand_adtype(otyp), ZAP_WAND, wand_damage_die(P_SKILL(P_WAND_POWER)) / ((otyp == WAN_MAGIC_MISSILE) ? 2 : 1));
			use_skill(P_WAND_POWER, wandlevel(otyp));
			zap(&youmonst, u.ux, u.uy, u.dx, u.dy, range, &zapdat);
	    } else
		impossible("weffects: unexpected spell or wand");
	    disclose = TRUE;
	}
	if (disclose && was_unkn) {
	    makeknown(otyp);
	    more_experienced(0,10);
	}
	return;
}
#endif /*OVLB*/
#ifdef OVL0

/*
 * Generate the to damage bonus for a spell. Based on the hero's intelligence
 */
int
spell_damage_bonus()
{
    int tmp, intell;
	
	intell = ACURR(base_casting_stat());

    /* Punish low intellegence before low level else low intellegence
       gets punished only when high level */
    if (intell < 10)
	tmp = -3;
    else if (u.ulevel < 5)
	tmp = 0;
    else if (intell < 14)
	tmp = 0;
    else if (intell <= 18)
	tmp = 1;
    else		/* helm of brilliance */
	tmp = 2;

	tmp += kraubon();
	
    return tmp;
}

int
kraubon()
{
	int bonus = 0;
	if(u.ukrau){
		bonus += u.ukrau/3;
		//remainder is probabilistic
		if(rn2(3) < u.ukrau%3)
			bonus++;
	}
	return bonus;
}

/*
 * Generate the to hit bonus for a spell.  Based on the hero's skill in
 * spell class and dexterity.
 */
STATIC_OVL int
spell_hit_bonus(adtyp)
int adtyp;
{
    int hit_bon = 0;
    int dex = ACURR(A_DEX);
	int skill = P_SKILL(spell_skill_from_adtype(adtyp));
	
	if(skill == P_MARTIAL_ARTS){
		switch (skill) {
		case P_ISRESTRICTED: hit_bon = 0; break;
		case P_UNSKILLED:    hit_bon = 2; break;
		case P_BASIC:        hit_bon = 3; break;
		case P_SKILLED:      hit_bon = 4; break;
		case P_EXPERT:       hit_bon = 5; break;
		case P_MASTER:       hit_bon = 7; break;
		case P_GRAND_MASTER: hit_bon = 9; break;
		}
	}
	else {
		switch (skill) {
		case P_ISRESTRICTED:
		case P_UNSKILLED:   hit_bon = -4; break;
		case P_BASIC:       hit_bon =  0; break;
		case P_SKILLED:     hit_bon =  2; break;
		case P_EXPERT:      hit_bon =  5; break;
		}
	}

    if (dex < 4)
	hit_bon -= 3;
    else if (dex < 6)
	hit_bon -= 2;
    else if (dex < 8)
	hit_bon -= 1;
    else if (dex < 14)
	hit_bon -= 0;		/* Will change when print stuff below removed */
    else
	hit_bon += dex - 14; /* Even increment for dextrous heroes (see weapon.c abon) */

    return hit_bon;
}

const char *
exclam(force)
register int force;
{
	/* force == 0 occurs e.g. with sleep ray */
	/* note that large force is usual with wands so that !! would
		require information about hand/weapon/wand */
	return (const char *)((force < 0) ? "?" : (force <= 4) ? "." : "!");
}

void
hit(str,mtmp,force)
register const char *str;
register struct monst *mtmp;
register const char *force;		/* usually either "." or "!" */
{
	if (mtmp == &youmonst) {
		pline("%s %s you%s", The(str), vtense(str, "hit"), force);
	}
	else {
		if ((!cansee(bhitpos.x, bhitpos.y) && !canspotmon(mtmp) &&
			!(u.uswallow && mtmp == u.ustuck))
			|| !flags.verbose)
			pline("%s %s it.", The(str), vtense(str, "hit"));
		else pline("%s %s %s%s", The(str), vtense(str, "hit"),
			mon_nam(mtmp), force);
	}
}

void
miss(str,mtmp)
register const char *str;
register struct monst *mtmp;
{
	pline("%s %s %s.", The(str), vtense(str, "miss"),
		(mtmp == &youmonst) ? "you"
		: ((cansee(bhitpos.x,bhitpos.y) || canspotmon(mtmp)) && flags.verbose) ? mon_nam(mtmp)
		: "it");
}
#endif /*OVL0*/
#ifdef OVL1

/*
 *  Called for the following distance effects:
 *	when an IMMEDIATE wand is zapped (ZAPPED_WAND)
 *	when a light beam is flashed (FLASHED_LIGHT)
 *	when a mirror is applied (INVIS_BEAM)
 *  A thrown/kicked object falls down at the end of its range or when a monster
 *  is hit.  The variable 'bhitpos' is set to the final position of the weapon
 *  thrown/zapped.  The ray of a wand may affect (by calling a provided
 *  function) several objects and monsters on its path.  The return value
 *  is the monster hit (weapon != ZAPPED_WAND), or a null monster pointer.
 *
 *  Check !u.uswallow before calling bhit().
 *  This function reveals the absence of a remembered invisible monster in
 *  necessary cases (throwing or kicking weapons).  The presence of a real
 *  one is revealed for a weapon, but if not a weapon is left up to fhitm().
 */
struct monst *
bhit(ddx,ddy,range,weapon,fhitm,fhito,obj,obj_destroyed)
register int ddx,ddy,range;		/* direction and range */
int weapon;				/* see values in hack.h */
int FDECL((*fhitm), (MONST_P, OBJ_P)),	/* fns called when mon/obj hit */
    FDECL((*fhito), (OBJ_P, OBJ_P));
struct obj *obj;			/* object tossed/used */
boolean *obj_destroyed;/* has object been deallocated? Pointer to boolean, may be NULL */
{
	struct monst *mtmp;
	struct trap *trap;
	uchar typ;
	boolean shopdoor = FALSE;
	if (obj_destroyed) { *obj_destroyed = FALSE; }

	if (weapon == FLASHED_LIGHT) {
	    tmp_at(DISP_BEAM, cmap_to_glyph(S_flashbeam));
	} else if (weapon != ZAPPED_WAND && weapon != INVIS_BEAM && weapon != TRIGGER_BEAM)
	    tmp_at(DISP_FLASH, obj_to_glyph(obj));

	bhitpos.x = u.ux;
	bhitpos.y = u.uy;

	while(range-- > 0) {
	    int x,y;
		
	    bhitpos.x += ddx;
	    bhitpos.y += ddy;
	    x = bhitpos.x; y = bhitpos.y;

	    if(!isok(x, y)) {
		bhitpos.x -= ddx;
		bhitpos.y -= ddy;
		break;
	    }
		
		trap = t_at(x, y);

		if (trap && trap->ttyp == STATUE_TRAP && weapon != TRIGGER_BEAM)
			activate_statue_trap(trap, x, y, (obj->otyp == WAN_STRIKING || obj->otyp == SPE_FORCE_BOLT || obj->otyp == ROD_OF_FORCE) ? TRUE : FALSE);
		
	    if(is_pick(obj) && inside_shop(x, y) &&
					   (mtmp = shkcatch(obj, x, y))) {
		tmp_at(DISP_END, 0);
		return(mtmp);
	    }

	    typ = levl[bhitpos.x][bhitpos.y].typ;
	    if (typ == IRONBARS){
			if ((obj->otyp == SPE_FORCE_BOLT || obj->otyp == ROD_OF_FORCE || obj->otyp == WAN_STRIKING)){
				break_iron_bars(bhitpos.x, bhitpos.y, TRUE);
			}
		}

	    if (weapon == ZAPPED_WAND && find_drawbridge(&x,&y))
		switch (obj->otyp) {
		    case WAN_OPENING:
		    case SPE_KNOCK:
			if (is_db_wall(bhitpos.x, bhitpos.y)) {
			    if (cansee(x,y) || cansee(bhitpos.x,bhitpos.y))
				makeknown(obj->otyp);
			    open_drawbridge(x,y);
			}
			break;
		    case WAN_LOCKING:
		    case SPE_WIZARD_LOCK:
			if ((cansee(x,y) || cansee(bhitpos.x, bhitpos.y))
			    && levl[x][y].typ == DRAWBRIDGE_DOWN)
			    makeknown(obj->otyp);
			close_drawbridge(x,y);
			break;
		    case WAN_STRIKING:
		    case SPE_FORCE_BOLT:
		    case ROD_OF_FORCE:
			if (typ != DRAWBRIDGE_UP)
			    destroy_drawbridge(x,y);
			makeknown(obj->otyp);
			break;
		}

	    if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
		notonhead = (bhitpos.x != mtmp->mx ||
			     bhitpos.y != mtmp->my);
		if (weapon != FLASHED_LIGHT) {
			if (weapon != ZAPPED_WAND && weapon != TRIGGER_BEAM) {
			    if(weapon != INVIS_BEAM) tmp_at(DISP_END, 0);
			    if (cansee(bhitpos.x,bhitpos.y) && !canspotmon(mtmp)) {
				if (weapon != INVIS_BEAM) {
				    map_invisible(bhitpos.x, bhitpos.y);
				    return(mtmp);
				}
			    } else
				return(mtmp);
			}
			if (weapon != INVIS_BEAM) {
				int res = (*fhitm)(mtmp, obj);
				if(obj->otyp == SPE_FULL_HEALING)
					return (struct monst *)0;
				if (weapon == TRIGGER_BEAM && res) {
					if (obj_destroyed) *obj_destroyed = TRUE;	/* doesn't destroy obj, but signals effect was done*/
					return mtmp;
				}
				else
					range -= 3;
			}
		} else {
		    /* FLASHED_LIGHT hitting invisible monster
		       should pass through instead of stop so
		       we call flash_hits_mon() directly rather
		       than returning mtmp back to caller. That
		       allows the flash to keep on going. Note
		       that we use mtmp->minvis not canspotmon()
		       because it makes no difference whether
		       the hero can see the monster or not.*/
		    if (mtmp->minvis) {
			obj->ox = u.ux,  obj->oy = u.uy;
			(void) flash_hits_mon(mtmp, obj);
		    } else {
			tmp_at(DISP_END, 0);
		    	return(mtmp); 	/* caller will call flash_hits_mon */
		    }
		}
	    } else {
		if (weapon == ZAPPED_WAND && obj->otyp == WAN_PROBING &&
		   glyph_is_invisible(levl[bhitpos.x][bhitpos.y].glyph)) {
		    unmap_object(bhitpos.x, bhitpos.y);
		    newsym(x, y);
		}
	    }
	    if(fhito) {
			if (bhitpile(obj, fhito, bhitpos.x, bhitpos.y)) {
				if (weapon == TRIGGER_BEAM) {
					if (obj_destroyed) *obj_destroyed = TRUE;	/* doesn't destroy obj, but signals effect was done*/
					return (struct monst *)0;
				}
				else
					range--;
			}
	    }
	    if(weapon == ZAPPED_WAND && (IS_DOOR(typ) || typ == SDOOR)) {
		switch (obj->otyp) {
		case WAN_OPENING:
		case WAN_LOCKING:
		case WAN_STRIKING:
		case SPE_KNOCK:
		case SPE_WIZARD_LOCK:
		case SPE_FORCE_BOLT:
		case ROD_OF_FORCE:
		    if (doorlock(obj, bhitpos.x, bhitpos.y)) {
			if (cansee(bhitpos.x, bhitpos.y) ||
			    (obj->otyp == WAN_STRIKING))
			    makeknown(obj->otyp);
			if (levl[bhitpos.x][bhitpos.y].doormask == D_BROKEN
			    && *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE)) {
			    shopdoor = TRUE;
			    add_damage(bhitpos.x, bhitpos.y, 400L);
			}
		    }
		    break;
		}
	    }
	    if(!ZAP_POS(typ) || closed_door(bhitpos.x, bhitpos.y)) {
		bhitpos.x -= ddx;
		bhitpos.y -= ddy;
		break;
	    }
	    if(weapon != ZAPPED_WAND && weapon != INVIS_BEAM && weapon != TRIGGER_BEAM) {
		/* 'I' present but no monster: erase */
		/* do this before the tmp_at() */
		if (glyph_is_invisible(levl[bhitpos.x][bhitpos.y].glyph)
			&& cansee(x, y)) {
		    unmap_object(bhitpos.x, bhitpos.y);
		    newsym(x, y);
		}
		tmp_at(bhitpos.x, bhitpos.y);
		delay_output();
	    }
	}

	if (weapon != ZAPPED_WAND && weapon != INVIS_BEAM && weapon != TRIGGER_BEAM) tmp_at(DISP_END, 0);

	if(shopdoor)
	    pay_for_damage("destroy", FALSE);

	return (struct monst *)0;
}

/* returns TRUE if mdef reflects the a zap with this data */
boolean
zap_reflect(mdef, zapdata)
struct monst * mdef;
struct zapdata * zapdata;
{
	boolean youdef = (mdef == &youmonst);
	boolean reflect;

	if (noreflect_zap(zapdata))
		return FALSE;

	if (!(youdef ? Reflecting : mon_reflects(mdef, (char *)0)))
		return FALSE;

	if (enhanced_zap(zapdata)) {
		struct obj * otmp;
		if (!(
			/* weapon */
			((otmp = youdef ? uwep : MON_WEP(mdef)) && (
			otmp->oartifact == ART_STAFF_OF_TWELVE_MIRRORS ||
			otmp->oartifact == ART_DRAGONLANCE ||
			(youdef && is_lightsaber(otmp) && (
			(activeFightingForm(FFORM_SHIEN)) ||
			(activeFightingForm(FFORM_SORESU))))
			)) ||
			/* body armor */
			((otmp = youdef ? uarm : which_armor(mdef, W_ARM)) && (
			otmp->otyp == SILVER_DRAGON_SCALES ||
			otmp->otyp == SILVER_DRAGON_SCALE_MAIL ||
			otmp->otyp == JUMPSUIT
			)) ||
			/* shield */
			((otmp = youdef ? uarms : which_armor(mdef, W_ARMS)) && (
			otmp->otyp == SILVER_DRAGON_SCALE_SHIELD ||
			otmp->oartifact == ART_ITLACHIAYAQUE
			))
			)) {
			return FALSE;
		}
	}
	return TRUE;
}

int
zapdamage(magr, mdef, zapdata)
struct monst * magr;
struct monst * mdef;
struct zapdata * zapdata;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	int dmg;
	/* calculate damage */
	if (zapdata->flat) {
		dmg = zapdata->flat;
	}
	else if (zapdata->damn && zapdata->damd) {
		dmg = d(zapdata->damn, zapdata->damd);
	}
	else {
		dmg = 0;
		impossible("zap with no damage?");
	}
	if(zapdata->bonus > 0)
		dmg += zapdata->bonus;

	/* damage bonuses */
	if (magr && zapdata->ztyp == ZAP_SPELL) {
		if (youagr){
			dmg += spell_damage_bonus();
			if(dmg < 1)
				dmg = 1;
		}
		if (youagr && u.ukrau_duration)
			dmg *= 1.5;
		if (youagr ? Spellboost : mon_resistance(magr, SPELLBOOST)) {
			dmg *= 2;
		}
	}
	/* general resistance */
	if (mdef && Half_spel(mdef))
		dmg /= 2;

	/* monsters can save for half damage */
	if (mdef && !youdef) {
		if (resist(mdef, (zapdata->ztyp == ZAP_WAND) ? WAND_CLASS : '\0', 0, NOTELL)
			)
			dmg /= 2;
	}
	if(dmg < 1)
		dmg = 1;
	/* madness damage reductions */
	if (mdef && youagr) {
		if (mdef->female && humanoid_torso(mdef->data) && roll_madness(MAD_SANCTITY)){
			dmg /= 4;
		}
		if (roll_madness(MAD_ARGENT_SHEEN)){
			dmg /= 6;
		}
		if ((is_spider(mdef->data)
			|| mdef->mtyp == PM_SPROW
			|| mdef->mtyp == PM_DRIDER
			|| mdef->mtyp == PM_PRIESTESS_OF_GHAUNADAUR
			|| mdef->mtyp == PM_AVATAR_OF_LOLTH
			) && roll_madness(MAD_ARACHNOPHOBIA)){
			dmg /= 4;
		}
		if (mdef->female && humanoid_upperbody(mdef->data) && roll_madness(MAD_ARACHNOPHOBIA)){
			dmg /= 2;
		}
		if (is_aquatic(mdef->data) && roll_madness(MAD_THALASSOPHOBIA)){
			dmg /= 10;
		}
	}
	return dmg;
}

/* helper to make a basic zap */
struct zapdata *
basiczap(zapdat, adtyp, ztyp, ndice)
struct zapdata * zapdat;
int adtyp;
int ztyp;
int ndice;
{
	if (!zapdat) {
		zapdat = &tzapdat;	/* use temporary global zapdata */
	}
	zapdat = memset(zapdat, 0, sizeof(struct zapdata));	/* clear old */
	zapdat->adtyp = adtyp;
	zapdat->damd = 6;
	zapdat->damn = ndice;
	zapdat->bonus = 0;
	zapdat->ztyp = ztyp;
	zapdat->affects_floor = 1;
	zapdat->directly_hits = 1;
	return zapdat;
}

void
zap(magr, sx, sy, dx, dy, range, zapdata)
struct monst * magr;		/* causer of zap, may be null */
int sx, sy;					/* current location of zap (passed sxsy is not hit) */
int dx, dy;					/* direction of zap */
int range;					/* range of zap. If 0, uses default of rn1(7,7). */
struct zapdata * zapdata;	/* lots of flags and data about the zap */
{
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	boolean shopdamage = FALSE;
	const char * fltxt = flash_type(zapdata->adtyp, zapdata->ztyp);
	coord save_bhitpos = bhitpos;	/* we shouldnt't set global bhitpos for some reason??? */
	int lsx, lsy;	/* last location of sxsy */
	struct rm *lev;
	struct monst * mdef;
	int dmg;	/* returned by zhit(), used by force() */

	/* modify range, if necessary */
	if (!range)
		range = rn1(7, 7);
	if (dx == 0 && dy == 0)
		range = 1;

	/* apply player's double-spell-size */
	if (youagr && Double_spell_size && (
			zapdata->ztyp == ZAP_SPELL ||
			zapdata->ztyp == ZAP_WAND)
		){
		zapdata->damn *= 1.5;
		zapdata->flat *= 1.5;
		if (range > 1)
			range *= 2;
	}

	/* intercept player zapping their swallower; RETURN EARLY */
	if (youagr && u.uswallow) {
		zhit(magr, u.ustuck, zapdata);
		if (!u.ustuck)
			u.uswallow = 0;
		return;
	}

	/* update your symbol? */
	if (!youagr) newsym(u.ux, u.uy);

	/* set zap glyph */
	tmp_at(DISP_BEAM, zapdir_to_glyph(dx, dy, zap_glyph_color(zapdata->adtyp)));

	/* do the main zap */
	while (range-- > 0) {
		lsx = sx; sx += dx;
		lsy = sy; sy += dy;

		/* check that zap is hitting in bounds, and also not stone? */
		if (isok(sx, sy) && ((lev = &levl[sx][sy]) && lev->typ)) {
			/* show zap */
			if (cansee(sx, sy)) {
				/* reveal/unreveal invisible monsters before tmp_at() */
				if ((mdef = m_at(sx, sy)) && !canspotmon(mdef))
					map_invisible(sx, sy);
				else if (!mdef && glyph_is_invisible(lev->glyph)) {
					unmap_object(sx, sy);
					newsym(sx, sy);
				}
				/* print zap segment */
				if (ZAP_POS(lev->typ) || (isok(lsx, lsy) && cansee(lsx, lsy)))
					tmp_at(sx, sy);
				delay_output(); /* wait a little */
			}

			/* hit() and miss() need bhitpos to match the target */
			bhitpos.x = sx, bhitpos.y = sy;

			/* Do zap-over-floor effects */
			if (zapdata->affects_floor)
				range += zap_over_floor(sx, sy, zapdata->adtyp, zapdata->ztyp, youagr, &shopdamage);

			/* there's a creature here */
			if (sx == u.ux && sy == u.uy)
				mdef = &youmonst;
			else
				mdef = m_at(sx, sy);

			if (mdef) {
#ifdef STEED
				if (u.usteed && !rn2(3) && mdef == &youmonst) mdef = u.usteed;
#endif
				youdef = (mdef == &youmonst);

				/* does it hit? */
				if (zap_hit(mdef, (youagr && zapdata->ztyp == ZAP_SPELL) ? zapdata->adtyp : 0, zapdata->phase_armor)
					|| zapdata->always_hits)
				{
					/* reduce range on hit */
					range -= 2;

					/* zap is hitting and is being reflected */
					if (zap_reflect(mdef, zapdata)) {
						/* message */
						if (youdef) {
							pline("%s hits you!", The(fltxt));
							if (Blind) {
								pline("For some reason you are not affected.");
							}
							else {
								shieldeff(sx, sy);
								(void)ureflects("But %s reflects from your %s!", "it");
							}
						}
						else if (cansee(sx, sy)) {
							hit(fltxt, mdef, ".");
							shieldeff(sx, sy);
							(void)mon_reflects(mdef, "But it reflects from %s %s!");
						}

						/* reflect and redirect */
						if (youdef && (
							(uwep && is_lightsaber(uwep) && litsaber(uwep) && activeFightingForm(FFORM_SHIEN) && (!uarm || is_light_armor(uarm))
								&& rnd(3) < FightingFormSkillLevel(FFORM_SHIEN)) ||
							(uwep && uwep->oartifact == ART_STAFF_OF_TWELVE_MIRRORS)
							)
							&& getdir((char *)0) && (u.dx || u.dy)
							){
							/* you choose direction */
							dx = u.dx;
							dy = u.dy;
							use_skill(P_SHIEN, 1);
							tmp_at(DISP_CHANGE, zapdir_to_glyph(dx, dy, zap_glyph_color(zapdata->adtyp)));
						}
						else if (!youdef && has_template(mdef, FRACTURED)){
							/* random direction */
							int i = rn2(8);
							dx = xdir[i];
							dy = ydir[i];
							tmp_at(DISP_CHANGE, zapdir_to_glyph(dx, dy, zap_glyph_color(zapdata->adtyp)));
						}
						else {
							dx = -dx;
							dy = -dy;
						}
					}
					/* zap is hitting and is not being reflected */
					else {
						if (zapdata->directly_hits) {
							zhit(magr, mdef, zapdata);
						}
						if (zapdata->single_target) {
							range = 0;
						}
					}
				}
				/* zap missed */
				else if (cansee(sx, sy)) {
					miss(fltxt, mdef);
				}

				/* effects on hit OR miss */

				/* defender notices */
				if (youagr && !youdef) mdef->mstrategy &= ~STRAT_WAITMASK;
				if (youdef)	nomul(0, NULL);

				/* lightning blinds */
				if (zapdata->adtyp == AD_ELEC && !resists_blnd(mdef)
					&& !(youagr && u.uswallow && mdef == u.ustuck)) {
					lightning_blind(mdef, d(zapdata->damn, 25));
				}

			}/*if mdef*/

			/* some zaps leave clouds behind */
			if (zapdata->leaves_clouds) {
				switch(zapdata->adtyp)
				{
				case AD_DRST:
					create_gas_cloud(sx, sy, 1, 8, youagr);
					break;
				default:{
					struct region_arg cloud_data;
					cloud_data.damage = 8;
					cloud_data.adtyp = zapdata->adtyp;
					create_generic_cloud(sx, sy, 1, &cloud_data, youagr);
				}
				}
			}

			/* trees are killed by death rays */
			if (lev->typ == TREE && zapdata->adtyp == AD_DEAD && (range >= 0)) {
				lev->typ = DEADTREE;
				lev->looted |= TREE_SWARM;
				if (cansee(sx, sy)) {
					pline("The tree withers!");
					newsym(sx, sy);
				}
				range = 0;
				if (youagr && u.sealsActive&SEAL_EDEN) unbind(SEAL_EDEN, TRUE);
			}

		}/* isok && !lev != stone */

		if ((!isok(sx, sy) || !ZAP_POS(lev->typ) || closed_door(sx, sy)) && (range >= 0)) {
			int bounce;
			uchar rmn;

			/* disintegrate terrain */
			/* maybe move this to dig.c? it's very very similar to zap_dig. */
			if (zapdata->adtyp == AD_DISN) {
				boolean shopdoor = FALSE, shopwall = FALSE;
				if (!isok(sx, sy)) {
					if (isok(sx-dx, sy-dy) && cansee(sx-dx, sy-dy))
						pline("The wall glows then fades.");
					range = 0;
				}
				else if (artifact_door(sx, sy)) {
					if (cansee(sx, sy))
						pline_The("door glows then fades.");
					range = 0;
				}
				else if (closed_door(sx, sy) || lev->typ == SDOOR) {
					/* might unbind otiax */
					if (closed_door(sx, sy) && !(levl[sx][sy].doormask&D_LOCKED) && youagr && u.sealsActive&SEAL_OTIAX) unbind(SEAL_OTIAX, TRUE);

					/* might be a shopkeeper's door */
					if (youagr && *in_rooms(sx, sy, SHOPBASE)) {
						add_damage(sx, sy, 400L);
						shopdoor = TRUE;
					}
					/* secret doors are silently revealed */
					if (lev->typ == SDOOR){
						lev->typ = DOOR;
					}
					if (cansee(sx, sy)) {
						pline_The("door is disintegrated!");
					}
					/* might anger watch */
					if (youagr) watch_dig((struct monst *)0, sx, sy, TRUE);

					lev->doormask = D_NODOOR;
					unblock_point(sx, sy); /* vision */
					vision_full_recalc = TRUE;
					range -= 1;
				}
				else if (IS_ROCK(lev->typ)) {
					if (!may_dig(sx, sy)){
						if (cansee(sx, sy))
							pline_The("rock glows then fades!");
						range = 0;
					}
					else if (IS_WALL(lev->typ)) {
						/* maybe unbind Anrealphus */
						if (youagr && u.sealsActive&SEAL_ANDREALPHUS) unbind(SEAL_ANDREALPHUS, TRUE);
						/* might be a shopkeeper's wall */
						if (youagr && *in_rooms(sx, sy, SHOPBASE)) {
							add_damage(sx, sy, 200L);
							shopwall = TRUE;
						}
						/* might anger watch */
						if (youagr) watch_dig((struct monst *)0, sx, sy, TRUE);
						if (level.flags.is_cavernous_lev && !in_town(sx, sy)) {
							lev->typ = CORR;
						}
						else {
							lev->typ = DOOR;
							lev->doormask = D_NODOOR;
						}
						range -= 2;
						unblock_point(sx, sy);
					}
					else if (IS_TREE(lev->typ)) {
						lev->typ = ROOM;
						if (youagr && u.sealsActive&SEAL_EDEN) unbind(SEAL_EDEN, TRUE);
						range -= 1;
						unblock_point(sx, sy);
					}
					else {	/* IS_ROCK but not IS_WALL */
						lev->typ = CORR;
						range -= 3;
						unblock_point(sx, sy);
					}
					/* vision */
					vision_full_recalc = TRUE;
				}
				if (shopdoor || shopwall)
					pay_for_damage(shopdoor ? "destroy" : "dig into", FALSE);
			}/*AD_DISN*/
			else {
				if (zapdata->no_hit_wall) {
					/* end zap on previous spot (right before the wall) */
					sx = lsx;
					sy = lsy;
					range = 0;
				}
				else if (zapdata->no_bounce) {
					/* end zap on current spot */
					range = 0;
				}
				else if (range >= 0) {
					bounce = 0;
					range--;

					if (range && isok(lsx, lsy) && cansee(lsx, lsy))
						pline("%s bounces!", The(fltxt));
					if (!dx || !dy || !rn2(20)) {
						dx = -dx;
						dy = -dy;
					}
					else {
						if (isok(sx, lsy) && ZAP_POS(rmn = levl[sx][lsy].typ) &&
							!closed_door(sx, lsy) &&
							(IS_ROOM(rmn) || (isok(sx + dx, lsy) &&
							ZAP_POS(levl[sx + dx][lsy].typ))))
							bounce = 1;
						if (isok(lsx, sy) && ZAP_POS(rmn = levl[lsx][sy].typ) &&
							!closed_door(lsx, sy) &&
							(IS_ROOM(rmn) || (isok(lsx, sy + dy) &&
							ZAP_POS(levl[lsx][sy + dy].typ))))
						if (!bounce || rn2(2))
							bounce = 2;

						switch (bounce) {
						case 0: dx = -dx; /* fall into... */
						case 1: dy = -dy; break;
						case 2: dx = -dx; break;
						}
						tmp_at(DISP_CHANGE, zapdir_to_glyph(dx, dy, zap_glyph_color(zapdata->adtyp)));
					}
				}/* bounce */
			}/* not AD_DISN */
		}/*hit wall*/
	}/*while range>0*/

	/* end glyphs */
	tmp_at(DISP_END, 0);

	/* some zaps have effects at their end */
	/* TODO: record colours in zapdata? Color currently standardized on AD_type */
	if (zapdata->explosive) {
		explode(sx, sy, zapdata->adtyp, 0, zapdamage(magr, (struct monst *)0, zapdata), adtyp_expl_color(zapdata->adtyp), 1 + !!(youagr &&Double_spell_size));
		if(youagr && u.explosion_up){
			int ex, ey;
			for(int i = 0; i < u.explosion_up; i++){
				ex = sx + rn2(3) - 1;
				ey = sy + rn2(3) - 1;
				if(isok(ex, ey) && ZAP_POS(levl[ex][ey].typ))
					explode(ex, ey, zapdata->adtyp, 0, zapdamage(magr, (struct monst *)0, zapdata), adtyp_expl_color(zapdata->adtyp), 1 + !!(Double_spell_size));
				else
					explode(sx, sy, zapdata->adtyp, 0, zapdamage(magr, (struct monst *)0, zapdata), adtyp_expl_color(zapdata->adtyp), 1 + !!(Double_spell_size));
			}
		}
	}
	if (zapdata->splashing) {
		long special_flags = 0L;
		if(youagr && GoatSpell && !GOAT_BAD && zapdata->adtyp == AD_ACID){
			zapdata->adtyp = AD_EACD;
			special_flags |= GOAT_SPELL;
			zapdata->damn *= 2;
			zapdata->damd += 2;
		}
		splash(sx, sy, dx, dy, zapdata->adtyp, 0, zapdamage(magr, (struct monst *)0, zapdata), adtyp_expl_color(zapdata->adtyp), special_flags);
	}

	/* calculate shop damage */
	if (shopdamage) {
		pay_for_damage(zapdata->adtyp == AD_FIRE ? "burn away" :
			zapdata->adtyp == AD_COLD ? "shatter" :
			zapdata->adtyp == AD_DEAD ? "disintegrate" : "destroy", FALSE);
	}
	/* restore old bhitpos */
	bhitpos = save_bhitpos;
	return;
}


/* prints messages and does all zap-hit effects */
/* CAN MODIFY ZAPDATA */
int
zhit(magr, mdef, zapdata)
struct monst * magr;	/* may be null */
struct monst * mdef;
struct zapdata * zapdata;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	boolean sho_shieldeff = FALSE;
	struct attack attk = { AT_NONE, zapdata->adtyp, 0, 0 };
	int dmg = zapdamage(magr, mdef, zapdata);
	int svddmg = dmg;	/* saved damage for golemeffects() */

	struct obj * otmp;
	char buf[BUFSZ];
	boolean havemsg = FALSE;
	boolean doshieldeff = FALSE;
	const char * fltxt = flash_type(zapdata->adtyp, zapdata->ztyp);

	/* macros to help put messages in the right place  */
#define addmsg(...) do{if(!havemsg){Sprintf(buf, __VA_ARGS__);havemsg=TRUE;}else{Strcat(buf, " "); Sprintf(eos(buf), __VA_ARGS__);}}while(0)
#define domsg() do{if((youagr || youdef || canseemon(mdef)) && dmg<*hp(mdef))\
	hit(fltxt, mdef, exclam(dmg)); \
	if(doshieldeff) shieldeff(bhitpos.x, bhitpos.y);\
	if(havemsg) pline1(buf);}while(0)

	/* do effects of zap */
	switch (zapdata->adtyp) {
	case AD_PHYS:
		if (Half_phys(mdef))
			dmg = (dmg + 1) / 2;
		domsg();
		return xdamagey(magr, mdef, &attk, dmg);
	case AD_MAGM:
		/* check resist */
		if (Magic_res(mdef)) {
			dmg = 0;
			doshieldeff = TRUE;
			if (youdef)
				addmsg("The missiles bounce off!");
		}
		domsg();
		if (youdef && dmg > 0)
			exercise(A_STR, FALSE);
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_HOLY:
		/* holy damage */
		if (hates_holy_mon(mdef)) {
			if (youdef) {
				addmsg("The holy missiles sear your flesh!");
			}
			dmg *= 2;
		}
		else if (hates_unholy_mon(mdef))
			dmg /= 3;
		domsg();
		if (youdef && dmg > 0)
			exercise(A_WIS, FALSE);
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_UNHY:
		/* holy damage */
		if (hates_unholy_mon(mdef)) {
			if (youdef) {
				addmsg("The unholy missiles sear your flesh!");
			}
			dmg *= 2;
		}
		else if (hates_holy_mon(mdef))
			dmg /= 3;
		domsg();
		if (youdef && dmg > 0)
			exercise(A_WIS, FALSE);
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_HLUH:
		/* holy damage */
		if (hates_unholy_mon(mdef) || hates_holy_mon(mdef)) {
			if (youdef) {
				addmsg("The corrupted missiles sear your flesh!");
			}
			if(hates_unholy_mon(mdef) && hates_holy_mon(mdef))
				dmg *= 3;
			else
				dmg *= 2;
		}
		domsg();
		if (youdef && dmg > 0)
			exercise(A_WIS, FALSE);
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_FIRE:
	case AD_EFIR:
		/* check resist / weakness */
		if (Fire_res(mdef)) {
			if (zapdata->adtyp == AD_EFIR) {
				dmg *= 0.5;
			}
			else {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You don't feel hot!");
				dmg = 0;
			}
		}
		else if (species_resists_cold(mdef)) {
			dmg *= 1.5;
		}
		domsg();
		golemeffects(mdef, AD_FIRE, svddmg);
		/* damage inventory */
		if (!UseInvFire_res(mdef)) {
			burnarmor(mdef, FALSE);
			if (!rn2(3)) (void)destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(3)) (void)destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(5)) (void)destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
		/* other */
		if (youdef) {
			burn_away_slime();
			melt_frozen_air();
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_COLD:
	case AD_ECLD:
		/* check resist / weakness */
		if (Cold_res(mdef)) {
			if (zapdata->adtyp == AD_ECLD) {
				dmg *= 0.5;
			}
			else {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You don't feel cold!");
				dmg = 0;
			}
		}
		else if (species_resists_fire(mdef)) {
			dmg *= 1.5;
		}
		domsg();
		golemeffects(mdef, AD_COLD, svddmg);
		/* damage inventory */
		if (!UseInvCold_res(mdef)) {
			if (!rn2(3)) (void)destroy_item(mdef, POTION_CLASS, AD_COLD);
		}
		/* other */
		if (youdef) {
			roll_frigophobia();
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_ELEC:
	case AD_EELC:
		/* check resist */
		if (Shock_res(mdef)) {
			if (zapdata->adtyp == AD_EELC) {
				dmg *= 0.5;
			}
			else {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You aren't shocked!");
				dmg = 0;
			}
		}
		domsg();
		golemeffects(mdef, AD_ELEC, svddmg);
		/* damage inventory */
		if (!UseInvShock_res(mdef)) {
			if (!rn2(3)) (void)destroy_item(mdef, WAND_CLASS, AD_ELEC);
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_ACID:
	case AD_EACD:
		/* check resist */
		if (Acid_res(mdef)) {
			if (zapdata->adtyp == AD_EACD) {
				dmg *= 0.5;
			}
			else {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You seem unaffected.");
				dmg = 0;
			}
		}
		/* extra effects vs player */
		if (youdef && dmg > 0) {
			addmsg("The acid burns!");
			exercise(A_STR, FALSE);
		}
		domsg();
		/* damage inventory */
		if (!UseInvAcid_res(mdef)) {
			if (!rn2(6)) (void)destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(6)) erode_obj(youdef ? uwep : MON_WEP(mdef), TRUE, TRUE);
			if (!rn2(6)) erode_armor(mdef, TRUE);
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_MADF:
		/* check resist / weakness */
		if (Fire_res(mdef) && Magic_res(mdef)) {
			doshieldeff = TRUE;
			if (youdef)
				addmsg("You don't feel hot!");
			dmg = 0;
		}
		else if (Fire_res(mdef)) {
			dmg -= dmg/2;
		}
		else if (species_resists_cold(mdef)) {
			dmg *= 1.5;
		}
		domsg();
		golemeffects(mdef, AD_FIRE, svddmg);
		/* damage inventory */
		if (!UseInvFire_res(mdef)) {
			burnarmor(mdef, FALSE);
			if (!rn2(3)) (void)destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(3)) (void)destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(5)) (void)destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
		/* other */
		if (youdef) {
			burn_away_slime();
			melt_frozen_air();
		}
		if(magr == mdef); //You can't share your madness with yourself
		else if(youdef){
			if(!save_vs_sanloss()){
				change_usanity(-1*d(3,6), TRUE);
			}
		}
		else if(youagr || magr->mtyp == PM_TWIN_SIBLING){
			if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || tp_sensemon(mdef) || !rn2(5)) && roll_generic_madness(FALSE)){
				//reset seen madnesses
				mdef->seenmadnesses = 0L;
				you_inflict_madness(mdef);
			}
		}
		else {
			if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
				if(!resist(mdef, '\0', 0, FALSE))
					mdef->mcrazed = TRUE;
			}
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_DRST:
		if (zapdata->ztyp == ZAP_SPELL) {
			/* the lethal "poison spray" spell */
			if (Poison_res(mdef)) {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("The poison doesn't seem to affect you.");
				dmg = 0;
			}
			else {
				dmg = *hp(mdef) + 1;
				if (youdef) {
					domsg();
					pline_The("poison was deadly...");
					killer_format = NO_KILLER_PREFIX;
					if (!u.uconduct.killer && !youagr){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(POISONING);
					return MM_DEF_LSVD;
				}
			}
			domsg();
			return xdamagey(magr, mdef, &attk, dmg);
		}
		else {
			/* other poison rays */
			if (youdef) {
				if (Poison_res(mdef)) dmg = 0; else dmg = 4;
				domsg();	/* gotta call before poisoned() */
				/* poisoned() deals the damage and checks resistance */
				poisoned("blast", A_STR, "poisoned blast", 15, FALSE);
			}
			else {
				if (Poison_res(mdef)) {
					doshieldeff = TRUE;
					dmg = 0;
				}
				domsg();	/* call after maybe reducing dmg to 0 */
			}
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_SLEE:
		/* does not actually do damage; */
		dmg = 0;
		{
		int sleeptime = zapdata->flat ? zapdata->flat : d(zapdata->damn, 25);
		/* asymmetric */
		if (youdef) {
			if (Sleep_res(mdef)) {
				doshieldeff = TRUE;
				sleeptime = 0;
				addmsg("You don't feel sleepy.");
			}
			domsg();
			if (sleeptime>0) {
				fall_asleep(-sleeptime, TRUE); /* sleep ray */
			}
		}
		else {
			domsg();
			(void)sleep_monst(mdef, sleeptime, zapdata->ztyp == ZAP_WAND ? WAND_CLASS : '\0');
		}
		/* rayguns are stunrays */
		if (zapdata->ztyp == ZAP_RAYGUN) {
			if (youdef) {
				make_stunned(HStun + sleeptime, TRUE);
			}
			else {
				mdef->mconf = TRUE;
				mdef->mstun = TRUE;
			}
		}
		/* dragons' sleep gas is hallucinogenic */
		if (magr && zapdata->ztyp == ZAP_BREATH &&
		    (is_true_dragon(magr->data) ||
		     youagr ? Race_if(PM_HALF_DRAGON) && u.ulevel >= 14 :
		     is_half_dragon(magr->data) && magr->data->mlevel >= 14)) {
			if (youdef) {
				make_hallucinated(HHallucination + sleeptime, TRUE, 0L);
			}
			else if (!mon_resistance(mdef, HALLUC_RES) && !mon_resistance(mdef, SLEEP_RES)){
				mdef->mberserk = TRUE;
				mdef->mconf = TRUE;
			}
		}
		}
		/* deal no damage */
		return xdamagey(magr, mdef, &attk, dmg);
	case AD_LASR:
		/* check resist / weakness */
		if (Fire_res(mdef)) {
			doshieldeff = TRUE;
			if (youdef)
				addmsg("You still feel some heat!");
			dmg = dmg/2;
		}
		domsg();
		golemeffects(mdef, AD_FIRE, svddmg);
		/* damage inventory */
		if (!InvFire_res(mdef)) {
			if (!rn2(3)) (void)destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(3)) (void)destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(5)) (void)destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
		/* other */
		if (youdef) {
			burn_away_slime();
			melt_frozen_air();
		}
		/* deal damage */
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_DRLI:
		/* check resist */
		if (Drain_res(mdef)) {
			doshieldeff = TRUE;
			if (youdef)
				addmsg("You aren't affected.");
			dmg = 0;
		}
		domsg();

		if(dmg > 0){
			/* drain levels */
			if (youdef) {
				losexp("life force drain", TRUE, FALSE, FALSE);
				dmg = 0;
			}
			else {
				dmg = d(2, 6);
				if (canseemon(mdef))
					pline("%s suddenly seems weaker!", Monnam(mdef));
				mdef->mhpmax -= dmg;
				if (mdef->m_lev == 0)
					dmg = mdef->mhp;
				else
					mdef->m_lev--;
				/* Automatic kill if drained past level 0 */
			}
		}
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_STAR:
		domsg();
		/* special antimagic effect */
		if (youdef)
			drain_en(dmg / 2);
		else
			mdef->mspec_used += dmg / 2;

		dmg = reduce_dmg(mdef, dmg, TRUE, FALSE);
		/* approximate as dmn/3 silver stars */
		if (dmg > 0) {
			int i;
			/* reduce by DR */
			for (i = zapdata->damn / 3; i > 0; i--) {
				dmg -= (youdef ? roll_udr(magr, ROLL_SLOT) : roll_mdr(mdef, magr, ROLL_SLOT));
			}
			/* deals silver-hating damage */
			if (hates_silver((youdef ? youracedata : mdef->data))) {
				for (i = zapdata->damn / 3; i > 0; i--) {
					dmg += rnd(20);
				}
			}
		}
		else {
			dmg = 1;
		}
		/* extra message for the silver */
		if (youdef && hates_silver(youracedata)) {
			if (noncorporeal(youracedata)) {
				pline("The silver stars sear you!");
			}
			else {
				pline("The silver stars sear your %s!", body_part(BODY_FLESH));
			}
		}

		return xdamagey(magr, mdef, &attk, dmg);

	case AD_DEAD:
		/* some creatures have special interactions with death beams */
		if (is_metroid(mdef->data)) {
			dmg = 0;
			domsg();	/* bud_metroid() prints messages */
			if (!youdef)
				bud_metroid(mdef);
			else {
				You("are irradiated with pure energy!");
				healup(d(3, 10), 0, FALSE, FALSE);
			}
			zapdata->single_target = TRUE; /* absorbs the beam */
		}
		else if (!youdef && is_delouseable(mdef->data)){
			dmg = 0;
			domsg();
			if (canseemon(mdef))
				pline("The parasite is killed!");
			delouse(mdef, AD_DEAD);
		}
		else if (mdef->mtyp == PM_DEATH) {
			dmg = 0;
			domsg();
			if (canseemon(mdef)) {
				pline("%s absorbs the deadly %s!", Monnam(mdef),
					zapdata->ztyp == ZAP_BREATH ? "blast" : "ray");
				pline("It seems even stronger than before.");
			}
			mdef->mhpmax += mdef->mhpmax / 2;
			if (mdef->mhpmax >= MAGIC_COOKIE)
				mdef->mhpmax = MAGIC_COOKIE - 1;
			mdef->mhp = mdef->mhpmax;
			zapdata->single_target = TRUE; /* absorbs the beam */
		}
		/* standard creatures */
		else {
			if (resists_death(mdef) || (youdef && u.sealsActive&SEAL_OSE)) {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You don't seem affected.");
				dmg = 0;
			}
			else if (Magic_res(mdef)) {
				doshieldeff = TRUE;
				if (youdef)
					addmsg("You aren't affected.");
				dmg = 0;
			}
			else {
				dmg = *hp(mdef) + 1;
			}
			domsg();
			if (dmg > 0) {
				if (youdef) {
					killer_format = KILLED_BY_AN;
					killer = flash_type(zapdata->adtyp, zapdata->ztyp);
					u.ugrave_arise = -3;
					done(DIED);
					return MM_DEF_LSVD; /* or, lifesaved */
				}
			}
		}
		return xdamagey(magr, mdef, &attk, dmg);

	case AD_DISN:
		if (Disint_res(mdef)) {
			doshieldeff = TRUE;
			if (youdef)
				addmsg("You are not disintegrated.");
			dmg = 0;
		}
		else if (is_rider(mdef->data)) {
			/* minor bug: armor doesn't protect them from disintegrating (and then reintegrating) */
			if (canseemon(mdef)) {
				addmsg("%s disintegrates.", Monnam(mdef));
				addmsg("%s body reintegrates before your %s!",
					s_suffix(Monnam(mdef)),
					(eyecount(youracedata) == 1) ?
					body_part(EYE) : makeplural(body_part(EYE)));
				addmsg("%s resurrects!", Monnam(mdef));
			}
			mdef->mhp = mdef->mhpmax;
			dmg = 0;
		}
		else {
			dmg = *hp(mdef) + 1;
		}
		domsg();

		if (dmg > 0)
		{
			if (/* armor must be in order of disintegration */
				(otmp = youdef ? uarms : which_armor(mdef, W_ARMS)) ||
				(otmp = youdef ? uarmc : which_armor(mdef, W_ARMC)) ||
				(otmp = youdef ? uarm : which_armor(mdef, W_ARM)) ||
				((otmp = youdef ? uarmu : which_armor(mdef, W_ARMU)) && objects[otmp->otyp].a_can > 0)
				) {
				/* armor protects */
				int i;
				for (i = max(1, dmg / 10); i > 0; i--) {

					/* player */
					if (otmp->spe > -1 * a_acdr(objects[(otmp)->otyp])){
						damage_item(otmp);
						if (i == 1) {
							if (youdef)
								Your("%s damaged by the beam.", aobjnam(otmp, "seem"));
							else if (canseemon(mdef))
								pline("%s %s is damaged!", s_suffix(Monnam(mdef)), distant_name(otmp, xname));
						}
					}
					else {
						if (youdef) {
							(void)destroy_arm(otmp);
						}
						else {
							if (canseemon(mdef))
								pline("%s %s is disintegrated!", s_suffix(Monnam(mdef)), distant_name(otmp, xname));
							m_useup(mdef, otmp);
						}
						i = 0;
					}
				}
			}/*armor*/
			else {

				if (youdef) {
					/* you are dead */
					killer_format = KILLED_BY_AN;
					killer = flash_type(zapdata->adtyp, zapdata->ztyp);
					/* when killed by disintegration breath, don't leave corpse */
					u.ugrave_arise = NON_PM;
					if (!u.uconduct.killer && !youagr){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DISINTEGRATED);
					return MM_DEF_LSVD; /* or, lifesaved */
				}
				else {
					/* monster's inventory gets hit by the disint too */
					struct obj *otmp2, *m_lsvd = mlifesaver(mdef);

					if (canseemon(mdef)) {
						if (!m_lsvd)
							pline("%s is disintegrated!", Monnam(mdef));
					}
#ifndef GOLDOBJ
					mdef->mgold = 0L;
#endif
					/* note: worn amulet of life saving must be preserved in order to operate */
					for (otmp = mdef->minvent; otmp; otmp = otmp2) {
						otmp2 = otmp->nobj;
						if (!(oresist_disintegration(otmp) || obj_resists(otmp, 5, 100) || otmp == m_lsvd)) {
							obj_extract_and_unequip_self(otmp);
							obfree(otmp, (struct obj *)0);
						}
					}
					return xdamagey(magr, mdef, &attk, *hp(mdef) + 1);
				}
			}/*!armor*/
		}/*!resisted*/

		return xdamagey(magr, mdef, &attk, 0);

	case AD_GOLD:
		domsg();
		if (/* armor must be in order of gilding */
			((otmp = youdef ? uarms : which_armor(mdef, W_ARMS)) && otmp->obj_material != GOLD) ||
			((otmp = youdef ? uarmc : which_armor(mdef, W_ARMC)) && otmp->obj_material != GOLD) ||
			((otmp = youdef ? uarm  : which_armor(mdef, W_ARM )) && otmp->obj_material != GOLD) ||
			((otmp = youdef ? uarmu : which_armor(mdef, W_ARMU)) && otmp->obj_material != GOLD && objects[otmp->otyp].a_can > 0)
			) {
			set_material(otmp, GOLD);
		}
		else {
			/* gild 10% of inventory */
			struct obj *itemp, *inext;
			for (itemp = (youdef ? invent : mdef->minvent); itemp; itemp = inext){
				inext = itemp->nobj;
				if (!rn2(10)) set_material(itemp, GOLD);
			}

			/* player */
			if (youdef && !Golded && !(Stone_resistance && youracedata->mtyp != PM_STONE_GOLEM)
				&& !is_gold(youracedata)
				&& !(poly_when_golded(youracedata) && polymon(PM_GOLD_GOLEM))
				) {
				Golded = 9;
				delayed_killer = "the breath of Mammon";
				killer_format = KILLED_BY;
				/* continue on to deal damage */
			}
			/* player clockworks turn to gold but don't die */
			if (youdef && uclockwork && u.clk_material != GOLD) {
				u.clk_material = GOLD;
			}
			/* monster */
			if (!youdef && !resists_ston(mdef) && !is_gold(mdef->data)) {
				minstagoldify(mdef, youagr);
				return !DEADMONSTER(mdef) ? MM_DEF_LSVD : MM_DEF_DIED; /* dead (or maybe lifesaved) */
			}
		}

		return xdamagey(magr, mdef, &attk, dmg);

	default:
		impossible("unhandled zap damage type %d", zapdata->adtyp);
		break;
	}
	return 0;
}

int
flash_hits_mon(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;	/* source of flash */
{
	int tmp, amt, res = 0, useeit = canseemon(mtmp) && !(mtmp->mundetected || mtmp->mappearance);

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    if (useeit) {
		pline_The("flash awakens %s.", mon_nam(mtmp));
		res = 1;
	    }
	} else if (mtmp->data->mlet != S_LIGHT) {
	    if (!resists_blnd(mtmp)) {
		tmp = dist2(otmp->ox, otmp->oy, mtmp->mx, mtmp->my);
		if (useeit) {
		    pline("%s is blinded by the flash!", Monnam(mtmp));
		    res = 1;
		}
		/* at this point, reveal them */
		mtmp->mundetected = 0;
		if (mtmp->m_ap_type)
			see_passive_mimic(mtmp);
		newsym(mtmp->mx, mtmp->my);
		if (mtmp->mtyp == PM_GREMLIN) {
		    /* Rule #1: Keep them out of the light. */
		    amt = otmp->otyp == WAN_LIGHT ? d(1 + otmp->spe, 4) :
		          rn2(min(mtmp->mhp,4));
		    pline("%s %s!", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "wails in agony" : "cries out in pain");
		    if ((mtmp->mhp -= amt) <= 0) {
			if (flags.mon_moving)
			    monkilled(mtmp, (char *)0, AD_BLND);
			else
			    killed(mtmp);
		    } else if (cansee(mtmp->mx,mtmp->my) && !canspotmon(mtmp)){
			map_invisible(mtmp->mx, mtmp->my);
		    }
			if(mtmp && amt > 0) mtmp->uhurtm = TRUE;
		}
		if (mtmp->mhp > 0) {
		    if (!flags.mon_moving) setmangry(mtmp);
		    if (tmp < 9 && !mtmp->isshk && rn2(4)) {
			if (rn2(4))
			    monflee(mtmp, rnd(100), FALSE, TRUE);
			else
			    monflee(mtmp, 0, FALSE, TRUE);
		    }
		    mtmp->mcansee = 0;
		    mtmp->mblinded = (tmp < 3) ? 0 : rnd(1 + 50/tmp);
		}
	    }
	}
	return res;
}

#endif /*OVL1*/
#ifdef OVLB

/*
 * burn scrolls and spellbooks on floor at position x,y
 * return the number of scrolls and spellbooks burned
 */
int
burn_floor_paper(x, y, give_feedback, u_caused)
int x, y;
boolean give_feedback;	/* caller needs to decide about visibility checks */
boolean u_caused;
{
	struct obj *obj, *obj2;
	long i, scrquan, delquan;
	char buf1[BUFSZ], buf2[BUFSZ];
	int cnt = 0;

	for (obj = level.objects[x][y]; obj; obj = obj2) {
	    obj2 = obj->nexthere;
	    if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS ||
			obj->otyp == SHEAF_OF_HAY) {
		if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL  || 
			obj->otyp == SCR_GOLD_SCROLL_OF_LAW || 
			obj_resists(obj, 0, 100))
		    continue;
		scrquan = obj->quan;	/* number present */
		delquan = 0;		/* number to destroy */
		for (i = scrquan; i > 0; i--)
		    if (!rn2(3)) delquan++;
		if (delquan) {
		    /* save name before potential delobj() */
		    if (give_feedback) {
			obj->quan = 1;
			Strcpy(buf1, (x == u.ux && y == u.uy) ?
				xname(obj) : distant_name(obj, xname));
			obj->quan = 2;
		    	Strcpy(buf2, (x == u.ux && y == u.uy) ?
				xname(obj) : distant_name(obj, xname));
			obj->quan = scrquan;
		    }
		    /* useupf(), which charges, only if hero caused damage */
		    if (u_caused) useupf(obj, delquan);
		    else if (delquan < scrquan) obj->quan -= delquan;
		    else delobj(obj);
		    cnt += delquan;
		    if (give_feedback) {
			if (delquan > 1)
			    pline("%ld %s burn.", delquan, buf2);
			else
			    pline("%s burns.", An(buf1));
		    }
		}
	    }
	}
	return cnt;
}

/* will zap/spell/breath attack score a hit a creature */
int
zap_hit(defender, adtyp, phase_armor)
struct monst * defender;
int adtyp;	// either a hero-cast AD_TYPE spell, or 0
boolean phase_armor;
{
    int chance = rn2(20);
    int spell_bonus = adtyp ? spell_hit_bonus(adtyp) : 0;
	int ac;

	if (defender == &youmonst){
		if (phase_armor)
			ac = base_uac();
		else
			ac = u.uac;
	}
	else
	{
		if (phase_armor)
			ac = base_mac(defender);
		else
			ac = find_mac(defender);
	}

    /* small chance for naked target to avoid being hit */
    if (!chance) return rnd(10) < ac+spell_bonus;

    /* very high armor protection does not achieve invulnerability */
	if (defender == &youmonst)
		ac = AC_VALUE(ac + u.uspellprot) - u.uspellprot;
	else
		ac = MONSTER_AC_VALUE(ac);

    return (3 - chance) < ac+spell_bonus;
}

#endif /*OVLB*/
#ifdef OVL0

/* caller should check !resists_blnd() before calling */
void
lightning_blind(mdef, blind_duration)
struct monst * mdef;
int blind_duration;
{
	if (Half_phys(mdef))
		blind_duration = (blind_duration + 1) / 2;

	if (mdef == &youmonst) {
		if (u.uvaul_duration)
			blind_duration = (blind_duration + 1) / 2;
		You(are_blinded_by_the_flash);
		make_blinded(blind_duration, FALSE);
		if (!Blind) Your1(vision_clears);
	}
	else {
		mdef->mcansee = 0;
		mdef->mblinded = min(127, blind_duration);
	}
	return;
}


struct monst *
delouse(mon, type)
struct monst *mon;
int type;
{
	struct obj *otmp;
	if(type == AD_STON){
		otmp = mksobj(STATUE, MKOBJ_NOINIT);
		otmp->corpsenm = mon->mtyp==PM_PARASITIZED_COMMANDER ? PM_PARASITIC_MASTER_MIND_FLAYER : PM_PARASITIC_MIND_FLAYER;
		fix_object(otmp);
		mpickobj(mon, otmp);
		if(which_armor(mon, W_ARMH)){
			struct obj *hlm = which_armor(mon, W_ARMH);
			m_lose_armor(mon, hlm);
		}
		//Equip it to the head slot
		mon->misc_worn_check |= W_ARMH;
		otmp->owornmask |= W_ARMH;
		update_mon_intrinsics(mon, otmp, TRUE, TRUE);
	}
	else if(type != AD_DGST){
		otmp = mksobj_at(CORPSE, mon->mx, mon->my, MKOBJ_NOINIT);
		otmp->corpsenm = mon->mtyp==PM_PARASITIZED_COMMANDER ? PM_PARASITIC_MASTER_MIND_FLAYER : PM_PARASITIC_MIND_FLAYER;
		fix_object(otmp);
	}
	
	if(mon->mtyp == PM_PARASITIZED_ANDROID){
		newcham(mon, PM_ANDROID, FALSE, FALSE);
		mon->m_lev = 7;
		mon->mhpmax = d(7,8);
		mon->mhp = min(mon->mhp, mon->mhpmax);
	}
	else if(mon->mtyp == PM_PARASITIZED_GYNOID){
		newcham(mon, PM_GYNOID, FALSE, FALSE);
		mon->m_lev = 14;
		mon->mhpmax = d(14,8);
		mon->mhp = min(mon->mhp, mon->mhpmax);
	}
	else if(mon->mtyp == PM_PARASITIZED_OPERATOR){
		newcham(mon, PM_OPERATOR, FALSE, FALSE);
		mon->m_lev = 7;
		mon->mhpmax = d(7,8);
		mon->mhp = min(mon->mhp, mon->mhpmax);
	}
	else if(mon->mtyp == PM_PARASITIZED_COMMANDER){
		newcham(mon, PM_COMMANDER, FALSE, FALSE);
		mon->m_lev = 3;
		mon->mhpmax = 20+rn2(4);
		mon->mhp = min(mon->mhp, mon->mhpmax);
		otmp = mksobj(SHACKLES, MKOBJ_NOINIT);
		set_material_gm(otmp, IRON);
		add_oprop(otmp, OPROP_ELECW);
		otmp->oeroded = 1;
		fix_object(otmp);
		(void) mpickobj(mon, otmp);
		mon->entangled_otyp = SHACKLES;
		mon->entangled_oid = otmp->o_id;
		mon->movement = 0;
	}
	else if(mon->mtyp == PM_PARASITIZED_DOLL){
		newcham(mon, PM_LIVING_DOLL, FALSE, FALSE);
		mon->m_lev = 15;
		mon->mhpmax = 60;
		mon->mhp = min(mon->mhp, mon->mhpmax);
	}
	set_template(mon, DELOUSED);
	untame(mon, 1);
	mon->mcanmove = 1;
	return mon;
}

struct monst *
delouse_tame(mon)
struct monst *mon;
{
	struct monst *mtmp;
	set_template(mon, 0);
	if(mon->mtyp == PM_COMMANDER){
		set_template(mon, M_GREAT_WEB);
	}
	if(mon->mtyp == PM_LIVING_DOLL){
		mon->mpeaceful = 1;
	} else {
		mtmp = tamedog(mon,(struct obj *) 0);
		if(mtmp){
			mon = mtmp;
			mon->mtame = 10;
			mon->mpeaceful = 1;
			mon->mcrazed = 1;
			EDOG(mon)->loyal = TRUE;
			EDOG(mon)->waspeaceful = TRUE;
			mon->mpeacetime = 0;
			newsym(mon->mx, mon->my);
		}
	}
	return mon;
}

void
melt_ice(x, y)
xchar x, y;
{
	struct rm *lev = &levl[x][y];
	struct obj *otmp;

	if (lev->typ == DRAWBRIDGE_UP)
	    lev->drawbridgemask &= ~DB_ICE;	/* revert to DB_MOAT */
	else {	/* lev->typ == ICE */
#ifdef STUPID
	    if (lev->icedpool == ICED_POOL) lev->typ = POOL;
	    if (lev->icedpool == ICED_PUDDLE) lev->typ = PUDDLE;
	    else lev->typ = MOAT;
#else
	    lev->typ = (lev->icedpool == ICED_POOL ? POOL :
			lev->icedpool == ICED_PUDDLE ? PUDDLE : MOAT);
#endif
	    lev->icedpool = 0;
	}
	obj_ice_effects(x, y, FALSE);
	if (lev->typ != PUDDLE)
		unearth_objs(x, y);
	if (Underwater) vision_recalc(1);
	newsym(x,y);
	if (cansee(x,y)) Norep("The ice crackles and melts.");
	if (lev->typ != PUDDLE && (otmp = boulder_at(x, y)) != 0) {
	    if (cansee(x,y)) pline("%s settles...", An(xname(otmp)));
	    do {
		obj_extract_self(otmp);	/* boulder isn't being pushed */
		if (!boulder_hits_pool(otmp, x, y, FALSE))
		    impossible("melt_ice: no pool?");
		/* try again if there's another boulder and pool didn't fill */
	    } while (is_pool(x,y, FALSE) && (otmp = boulder_at(x, y)) != 0);
	    newsym(x,y);
	}
	if (x == u.ux && y == u.uy)
		spoteffects(TRUE);	/* possibly drown, notice objects */
}

/* Burn floor scrolls, evaporate pools, etc...  in a single square.  Used
 * both for normal bolts of fire, cold, etc... and for fireballs.
 * Sets shopdamage to TRUE if a shop door is destroyed, and returns the
 * amount by which range is reduced (the latter is just ignored by fireballs)
 */
int
zap_over_floor(x, y, adtyp, olet, yours, shopdamage)
xchar x, y;
int adtyp, olet;
int yours;
boolean *shopdamage;
{
	struct monst *mon;
	struct rm *lev = &levl[x][y];
	int rangemod = 0;

	if(adtyp == AD_FIRE || adtyp == AD_MADF) {
	    struct trap *t = t_at(x, y);

	    if (t && t->ttyp == WEB && !Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)) {
		/* a burning web is too flimsy to notice if you can't see it */
		if (cansee(x,y)) Norep("A web bursts into flames!");
		(void) delfloortrap(t);
		if (cansee(x,y)) newsym(x,y);
	    }
	    if(IS_GRASS(lev->typ)){
		lev->typ = SOIL;
		if(cansee(x,y)) {
			pline("The grass burns away!");
			newsym(x,y);
		}
	    }
	    if(is_ice(x, y)) {
		melt_ice(x, y);
	    } else if(is_pool(x,y, FALSE)) {
		const char *msgtxt = "You hear hissing gas.";
		if(lev->typ != POOL || IS_PUDDLE(lev->typ)) {	/* MOAT or DRAWBRIDGE_UP */
		    if (cansee(x,y)) msgtxt = "Some water evaporates.";
		} else {
		    register struct trap *ttmp;

		    rangemod -= 3;
		    lev->typ = ROOM;
		    ttmp = maketrap(x, y, PIT);
		    if (ttmp) ttmp->tseen = 1;
		    if (cansee(x,y)) msgtxt = "The water evaporates.";
		}
		Norep("%s", msgtxt);
		if (lev->typ == ROOM) newsym(x,y);
	    } else if(IS_FOUNTAIN(lev->typ)) {
		    if (cansee(x,y))
			pline("Steam billows from the fountain.");
		    rangemod -= 1;
		    dryup(x, y, yours);
	    } else if (IS_PUDDLE(lev->typ)) {
		    rangemod -= 3;
		    lev->typ = ROOM;
		    if (cansee(x,y)) pline("The water evaporates.");
		    else You_hear("hissing gas.");
	    }
	}
	else if(adtyp == AD_COLD && (is_pool(x,y, TRUE) || is_lava(x,y))) {
		boolean lava = is_lava(x,y);
		boolean moat = (!lava && (lev->typ != POOL) &&
				(lev->typ != WATER) &&
				(lev->typ != PUDDLE) &&
				!Is_medusa_level(&u.uz) &&
				!Is_waterlevel(&u.uz));

		if (lev->typ == WATER) {
		    /* For now, don't let WATER freeze. */
		    if (cansee(x,y))
			pline_The("water freezes for a moment.");
		    else
			You_hear("a soft crackling.");
		    rangemod -= 1000;	/* stop */
		} else {
		    rangemod -= 3;
		    if (lev->typ == DRAWBRIDGE_UP) {
			lev->drawbridgemask &= ~DB_UNDER;  /* clear lava */
			lev->drawbridgemask |= (lava ? DB_FLOOR : DB_ICE);
		    } else {
			if (!lava)
			    lev->icedpool =
				    (lev->typ == POOL ? ICED_POOL :
				     lev->typ == PUDDLE ? ICED_PUDDLE : ICED_MOAT);
			lev->typ = (lava ? ROOM : ICE);
		    }
		    if (lev->icedpool != ICED_PUDDLE)
				bury_objs(x,y);
		    if(cansee(x,y)) {
			if(moat)
				Norep("The moat is bridged with ice!");
			else if(lava)
				Norep("The lava cools and solidifies.");
			else
				Norep("The water freezes.");
			newsym(x,y);
		    } else if(flags.soundok && !lava)
			You_hear("a crackling sound.");

		    if (x == u.ux && y == u.uy) {
			if (u.uinwater) {   /* not just `if (Underwater)' */
			    /* leave the no longer existent water */
			    u.uinwater = 0;
			    u.usubwater = 0;
			    u.uundetected = 0;
			    docrt();
			    vision_full_recalc = 1;
			} else if (u.utrap && u.utraptype == TT_LAVA) {
			    if (Passes_walls) {
				You("pass through the now-solid rock.");
			    } else {
				u.utrap = rn1(50,20);
				u.utraptype = TT_INFLOOR;
				You("are firmly stuck in the cooling rock.");
			    }
			}
		    } else if ((mon = m_at(x,y)) != 0) {
			/* probably ought to do some hefty damage to any
			   non-ice creature caught in freezing water;
			   at a minimum, eels are forced out of hiding */
			if (mon_resistance(mon,SWIMMING) && mon->mundetected) {
			    mon->mundetected = 0;
			    newsym(x,y);
			}
		    }
		}
		obj_ice_effects(x,y,TRUE);
	}
	else if (adtyp == AD_DRST) {
		if(flags.drgn_brth){
			create_gas_cloud(x, y, 1, 8, yours);
			// (void) create_gas_cloud(x, y, 1, 8, rn1(20, 5), yours);
		}
	}
	else if (adtyp == AD_ACID && levl[x][y].typ == IRONBARS && (flags.drgn_brth || !rn2(5))) {
	    if (cansee(x, y))
		pline_The("iron bars are dissolved!");
	    else
		You_hear(Hallucination ? "angry snakes!" : "a hissing noise.");
	    dissolve_bars(x, y);
	}
	if(closed_door(x, y)) {
		int new_doormask = -1;
		const char *see_txt = 0, *sense_txt = 0, *hear_txt = 0;
		rangemod = -1000;
		/* ALI - Artifact doors */
		if (artifact_door(x, y))
		    goto def_case;
		switch(adtyp) {
		case AD_FIRE:
		case AD_MADF:
		    new_doormask = D_NODOOR;
		    see_txt = "The door is consumed in flames!";
		    sense_txt = "smell smoke.";
		    break;
		case AD_COLD:
		    new_doormask = D_NODOOR;
		    see_txt = "The door freezes and shatters!";
		    sense_txt = "feel cold.";
		    break;
		case AD_DISN:
		    new_doormask = D_NODOOR;
		    see_txt = "The door disintegrates!";
		    hear_txt = "crashing wood.";
		    break;
		case AD_ELEC:
		    new_doormask = D_BROKEN;
		    see_txt = "The door splinters!";
		    hear_txt = "crackling.";
		    break;
		default:
		def_case:
		    if(cansee(x,y)) {
			pline_The("door absorbs the blast!");
		    } else You_feel("vibrations.");
		    break;
		}
		if (new_doormask >= 0) {	/* door gets broken */
		    if (*in_rooms(x, y, SHOPBASE)) {
			if (yours) {
			    add_damage(x, y, 400L);
			    *shopdamage = TRUE;
			} else	/* caused by monster */
			    add_damage(x, y, 0L);
		    }
		    lev->doormask = new_doormask;
		    unblock_point(x, y);	/* vision */
		    if (cansee(x, y)) {
			pline1(see_txt);
			newsym(x, y);
		    } else if (sense_txt) {
			You1(sense_txt);
		    } else if (hear_txt) {
			if (flags.soundok) You_hear1(hear_txt);
		    }
		    if (picking_at(x, y)) {
			stop_occupation();
			reset_pick();
		    }
		}
	}

	if(OBJ_AT(x, y) && (adtyp == AD_FIRE || adtyp == AD_MADF))
		if (burn_floor_paper(x, y, FALSE, yours) && couldsee(x, y)) {
		    newsym(x,y);
		    You("%s of smoke.",
			!Blind ? "see a puff" : "smell a whiff");
		}
	if(OBJ_AT(x, y) && adtyp == AD_WET && !is_lava(x,y))
		water_damage(level.objects[x][y], FALSE, FALSE, FALSE, 0);

	if ((mon = m_at(x,y)) != 0) {
		wakeup(mon, FALSE);
		if(mon->m_ap_type) seemimic(mon);
		if(yours && !flags.mon_moving) {
		    setmangry(mon);
		    if(mon->ispriest && *in_rooms(mon->mx, mon->my, TEMPLE))
			ghod_hitsu(mon);
		    if(mon->isshk && !*u.ushops)
			hot_pursuit(mon);
		}
	}
	return rangemod;
}

#endif /*OVL0*/
#ifdef OVL3

/*
 * Wrapper function that correctly handles each type of boulder-like object
 */

void
break_boulder(obj)
struct obj *obj;
{
	switch(obj->otyp){
		case BOULDER:
			fracture_rock(obj);
		break;
		case MASSIVE_STONE_CRATE:
			break_crate(obj);
		break;
		case MASS_OF_STUFF:
			separate_mass_of_stuff(obj, FALSE);
		break;
		case STATUE:
			break_statue(obj);
		break;
		default:
			impossible("unhandled boulder type being broken!");
			fracture_rock(obj);
		break;
	}
}

void
fracture_rock(obj)	/* fractured by pick-axe or wand of striking */
register struct obj *obj;		   /* no texts here! */
{
	int mat = obj->obj_material;
	int submat = obj->sub_material;
	/* A little Sokoban guilt... */
	if(In_sokoban(&u.uz) && !flags.mon_moving)
	    change_luck(-1);

	obj->otyp = ROCK;
	obj->quan = (long) rn1(60, 7);
	obj->oclass = GEM_CLASS;
	obj->known = FALSE;
	rem_all_ox(obj);	/* no names or other data */
	set_material_gm(obj, MINERAL);
	obj->owt = weight(obj);
	set_material(obj, mat);
	if(mat == GEMSTONE){
		if(submat)
			obj->otyp = submat;
		set_object_color(obj);
		fix_object(obj);
	}
	else {
		set_submat(obj, submat);
	}
	if (obj->where == OBJ_FLOOR) {
		obj_extract_self(obj);		/* move rocks back on top */
		place_object(obj, obj->ox, obj->oy);
		if(!does_block(obj->ox,obj->oy,&levl[obj->ox][obj->oy]))
			unblock_point(obj->ox,obj->oy);
		if(cansee(obj->ox,obj->oy))
		    newsym(obj->ox,obj->oy);
	}
}

/* handle statue hit by striking/force bolt/pick-axe */
boolean
break_statue(obj)
register struct obj *obj;
{
	/* [obj is assumed to be on floor, so no get_obj_location() needed] */
	struct trap *trap = t_at(obj->ox, obj->oy);
	struct obj *item;

	if (trap && trap->ttyp == STATUE_TRAP &&
		activate_statue_trap(trap, obj->ox, obj->oy, TRUE))
	    return FALSE;
	/* drop any objects contained inside the statue */
	while ((item = obj->cobj) != 0) {
	    obj_extract_self(item);
	    place_object(item, obj->ox, obj->oy);
	}
	if (Role_if(PM_ARCHEOLOGIST) && !flags.mon_moving && (obj->spe & STATUE_HISTORIC)) {
	    You_feel("guilty about damaging such a historic statue.");
	    adjalign(-1);
	}
	obj->spe = 0;
	fracture_rock(obj);
	return TRUE;
}

/* handle crate hit by striking/force bolt/pick-axe */
void
break_crate(obj)
register struct obj *obj;
{
	/* [obj is assumed to be on floor, so no get_obj_location() needed] */
	struct obj *item;

	/* drop any objects contained inside the crate */
	while ((item = obj->cobj) != 0) {
	    obj_extract_self(item);
	    place_object(item, obj->ox, obj->oy);
	}
	obj->spe = 0;
	fracture_rock(obj);
}

/* handle mass of stuff being hit by striking/force bolt/pick-axe or burried */
void
separate_mass_of_stuff(obj, burial)
register struct obj *obj;
boolean burial;
{
	/* [obj is assumed to be on floor, so no get_obj_location() needed] */
	struct obj *item;
	int x = obj->ox;
	int y = obj->oy;
	int i;
	struct obj *otmp;
	int mat = obj->obj_material;
	mkgold(d(100,9)+rn2(100), x, y);
	
	for(i = d(9,3); i > 0; i--){
		otmp = mkobj_at(WEAPON_CLASS, x, y, NO_MKOBJ_FLAGS);
		if(otmp){
			if(is_hard(otmp) && rn2(2))
				set_material(otmp, mat);
			if(obj->cursed)
				curse(otmp);
			if(obj->blessed)
				bless(otmp);
			if(burial)
				bury_an_obj(otmp);
			stackobj(otmp);
		}
	}
	for(i = d(9,3); i > 0; i--){
		otmp = mkobj_at(0, x, y, NO_MKOBJ_FLAGS);
		if(otmp){
			if((otmp->oclass == WEAPON_CLASS || otmp->oclass == ARMOR_CLASS || is_weptool(otmp)) && is_hard(otmp) && rn2(2))
				set_material(otmp, mat);
			if(obj->cursed)
				curse(otmp);
			if(obj->blessed)
				bless(otmp);
			if(burial)
				bury_an_obj(otmp);
			stackobj(otmp);
		}
	}
	for(i = d(9,3); i > 0; i--){
		otmp = mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
		if(obj->cursed)
			curse(otmp);
		if(obj->blessed)
			bless(otmp);
		if(otmp && burial)
			bury_an_obj(otmp);
		stackobj(otmp);
	}

	obj->spe = 0;
	fracture_rock(obj);
}

#endif /*OVL3*/
#ifdef OVL2

int
mm_resist(struct monst *mdef, struct monst *magr, int damage, int tell)
{
	int resisted;
	int alev, dlev;
	boolean youagr = &youmonst == magr;

#define LUCK_MODIFIER	if(Luck > 0) alev += rnd(Luck)/2; else if(Luck < 0) alev -= rnd(-1*Luck)/2;

	damage -= avg_spell_mdr(mdef);
	if(damage < 0)
		damage = 0;
	alev = mlev(magr);
	/* attack level */
	if(!mdef->mpeaceful || magr->mtame){
		LUCK_MODIFIER
	}
#undef LUCK_MODIFIER
	/* defense level */
	dlev = (int)mdef->m_lev;
	if(mdef->mcan)
		dlev /= 2;
	if (dlev > 50) dlev = 50;
	else if (dlev < 1) dlev = 1;
	
	if(mdef->mtame && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_STEEL)
		dlev += 1;

	int mons_mr = mdef->data->mr;
	if(mdef->mcan){
		if(mdef->mtyp == PM_ALIDER)
			mons_mr = 0;
		else
			mons_mr /= 2;
	}
	if(mdef->mtame && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WILL)
		mons_mr += 10;

	if(mdef->mtyp == PM_CHOKHMAH_SEPHIRAH) dlev+=u.chokhmah;
	resisted = rn2(100 + alev - dlev) < mons_mr;
	if (resisted) {
	    if (tell) {
			shieldeff(mdef->mx, mdef->my);
			pline("%s resists!", Monnam(mdef));
	    }
	    damage = (damage + 1) / 2;
	}

	if(youagr && mdef->female && humanoid_torso(mdef->data) && roll_madness(MAD_SANCTITY)){
		damage /= 4;
	}
	if(youagr && roll_madness(MAD_ARGENT_SHEEN)){
		damage /= 6;
	}
	if(youagr && (is_spider(mdef->data) 
		|| mdef->mtyp == PM_SPROW
		|| mdef->mtyp == PM_DRIDER
		|| mdef->mtyp == PM_PRIESTESS_OF_GHAUNADAUR
		|| mdef->mtyp == PM_AVATAR_OF_LOLTH
	) && roll_madness(MAD_ARACHNOPHOBIA)){
		damage /= 4;
	}
	if(youagr && mdef->female && humanoid_upperbody(mdef->data) && roll_madness(MAD_ARACHNOPHOBIA)){
		damage /= 2;
	}
	if(youagr && is_aquatic(mdef->data) && roll_madness(MAD_THALASSOPHOBIA)){
		damage /= 10;
	}
	
	if (damage) {
	    mdef->mhp -= damage;
	    if (mdef->mhp < 1) {
			if(m_using) monkilled(mdef, "", AD_RBRE);
			else killed(mdef);
	    }
	}
	return(resisted);
}

int
resist(mtmp, oclass, damage, tell)
struct monst *mtmp;
char oclass;
int damage, tell;
{
	int resisted;
	int alev, dlev;

#define LUCK_MODIFIER	if(Luck > 0) alev += rnd(Luck)/2; else if(Luck < 0) alev -= rnd(-1*Luck)/2;

	damage -= avg_spell_mdr(mtmp);
	if(damage < 0)
		damage = 0;
	/* attack level */
	switch (oclass) {
	    case WAND_CLASS:
			alev = 12;	 
			if(!flags.mon_moving){
				if(P_SKILL(P_WAND_POWER) > 1)
					alev += (P_SKILL(P_WAND_POWER)-1)*2;
				LUCK_MODIFIER
			}
		break;
	    case TOOL_CLASS:	/* instrument */
			alev = 10;
			if(!flags.mon_moving){
				alev += (ACURR(A_CHA)-11);
				alev += P_SKILL(P_MUSICALIZE)*2; //+0 to +8
				LUCK_MODIFIER
			}
		break;	
	    case GEM_CLASS:  /* artifact */
	    case WEAPON_CLASS:  /* artifact */
	    case CHAIN_CLASS:  /* artifact */
			alev = 45;
			if(!flags.mon_moving){
				LUCK_MODIFIER
			}
		break;
	    case SCROLL_CLASS:
			alev =  9;
			if(!flags.mon_moving){
				LUCK_MODIFIER
			}
		break;
	    case POTION_CLASS:
			alev =  6;
			if(!flags.mon_moving){
				LUCK_MODIFIER
			}
		break;
	    case RING_CLASS:/* conflict */
			alev =  5;
		break;
	    default:/* spell */
			alev = u.ulevel;
			alev += (ACURR(A_CHA)-11);
			LUCK_MODIFIER
		break;
	}
#undef LUCK_MODIFIER
	/* defense level */
	dlev = (int)mtmp->m_lev;
	if(mtmp->mcan)
		dlev /= 2;
	if (dlev > 50) dlev = 50;
	else if (dlev < 1) dlev = 1;
	
	if(mtmp->mtame && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_STEEL)
		dlev += 1;

	int mons_mr = mtmp->data->mr;
	if(mtmp->mcan){
		if(mtmp->mtyp == PM_ALIDER)
			mons_mr = 0;
		else
			mons_mr /= 2;
	}
	if(mtmp->mtame && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WILL)
		mons_mr += 10;

	if(mtmp->mtyp == PM_CHOKHMAH_SEPHIRAH) dlev+=u.chokhmah;
	resisted = rn2(100 + alev - dlev) < mons_mr;
	if (resisted) {
	    if (tell) {
			shieldeff(mtmp->mx, mtmp->my);
			pline("%s resists!", Monnam(mtmp));
	    }
	    damage = (damage + 1) / 2;
	}

	if(!flags.mon_moving && mtmp->female && humanoid_torso(mtmp->data) && roll_madness(MAD_SANCTITY)){
		damage /= 4;
	}
	if(!flags.mon_moving && roll_madness(MAD_ARGENT_SHEEN)){
		damage /= 6;
	}
	if(!flags.mon_moving && (is_spider(mtmp->data) 
		|| mtmp->mtyp == PM_SPROW
		|| mtmp->mtyp == PM_DRIDER
		|| mtmp->mtyp == PM_PRIESTESS_OF_GHAUNADAUR
		|| mtmp->mtyp == PM_AVATAR_OF_LOLTH
	) && roll_madness(MAD_ARACHNOPHOBIA)){
		damage /= 4;
	}
	if(!flags.mon_moving && mtmp->female && humanoid_upperbody(mtmp->data) && roll_madness(MAD_ARACHNOPHOBIA)){
		damage /= 2;
	}
	if(!flags.mon_moving && is_aquatic(mtmp->data) && roll_madness(MAD_THALASSOPHOBIA)){
		damage /= 10;
	}
	
	if (damage) {
	    mtmp->mhp -= damage;
	    if (mtmp->mhp < 1) {
		if(m_using) monkilled(mtmp, "", AD_RBRE);
		else killed(mtmp);
	    }
	}
	return(resisted);
}

/* returns WISH_ARTALLOW if the player is eligible to wish for an artifact at this time, otherwise 0
 * Although there is an ARTWISH_SPENT flag for the uevents, don't use it here -- just use how many
 *   artifacts the player has wished for, and keep track of how many (total) have been earned.
 */
int
allow_artwish()
{
	int n = 1;
	
	// n -= flags.descendant; 			// 'used' their first on their inheritance
	// n += u.uevent.qcalled;		// reaching the main dungeon branch of the quest
	//if(u.ulevel >= 7) n++;		// enough levels to be intimidating to marids/djinni
	n += (u.uevent.utook_castle & ARTWISH_EARNED);	// sitting on the castle throne
	n += (u.uevent.uunknowngod & ARTWISH_EARNED);	// sacrificing five artifacts to the priests of the unknown god
	n += (u.uevent.uconstellation & ARTWISH_SPENT);	// got an extra bonus artwish from the imperial elven ring

	n -= u.uconduct.wisharti;	// how many artifacts the player has wished for

	return ((n > 0) ? WISH_ARTALLOW : 0);
}

boolean
makewish(wishflags)
int wishflags;		// flags to change messages / effects
{
	char buf[BUFSZ];
	char bufcpy[BUFSZ];
	struct obj *otmp, nothing;
	int tries = 0;
	int wishreturn;

	nothing = zeroobj;  /* lint suppression; only its address matters */
	if (flags.verbose) You("may wish for an object.");
retry:
	getlin("For what do you wish?", buf);
	if(buf[0] == '\033') buf[0] = 0;
	/*
	 *  Note: if they wished for and got a non-object successfully,
	 *  otmp == &zeroobj.  That includes gold, or an artifact that
	 *  has been denied.  Wishing for "nothing" requires a separate
	 *  value to remain distinct.
	 */
	strcpy(bufcpy, buf);
	wishreturn = 0;
	otmp = readobjnam(buf, &wishreturn, wishflags);

	if (wishreturn & WISH_NOTHING)
	{
		/* explicitly wished for "nothing", presumeably attempting to retain wishless conduct */
		if (wishflags & WISH_VERBOSE)
			verbalize("As you wish.");
		return FALSE;
	}
	if (wishreturn & WISH_FAILURE)
	{
		/* wish could not be read */
		if (wishflags & WISH_VERBOSE)
			verbalize("I do not know of that which you speak.");
		else
			pline("Nothing fitting that description exists in the game.");
		if (++tries < 5)
			goto retry;
	}
	if (wishreturn & WISH_DENIED)
	{
		/* wish was read as an object that cannot be wished for */
		if (wishflags & WISH_VERBOSE)
			verbalize("That is beyond my power.");
		else
			pline("You cannot wish for that.");
		if (++tries < 5)
			goto retry;
	}
	if (wishreturn & WISH_MERCYRULE)
	{
		/* wish was denied due to mercy rule - you couldn't even pick it up if it WAS granted */
		if (wishflags & WISH_VERBOSE)
			verbalize("Such an artifact would refuse to lend you aid.");
		else
			pline("You cannot wish for an artifact that would refuse you.");
		if (++tries < 5)
			goto retry;
	}
	if (wishreturn & WISH_OUTOFJUICE)
	{
		/* wish was read as an artifact and you're out of artifact wishes  */
		if (wishflags & WISH_VERBOSE)
			verbalize("Alone, such artifacts are beyond my power.");
		else
			pline("You cannot wish for an artifact right now.");
		if (++tries < 5)
			goto retry;
	}
	if (wishreturn & WISH_ARTEXISTS)
	{
		/* wish was read as an artifact that has already been generated */
		if (wishflags & WISH_VERBOSE)
			verbalize("Such an artifact already has been created.");
		else
			pline("You cannot wish for an existing artifact.");
		if (++tries < 5)
			goto retry;
	}
	if (wishreturn & WISH_SUCCESS)
	{
		/* an allowable wish was read */
		if (wishflags & WISH_VERBOSE)
			verbalize("Done.");
	}
	if ((tries == 5) && !(wishreturn & (WISH_NOTHING | WISH_SUCCESS)))
	{
		/* no more tries
		 * removes vanilla behavior of random item
		 * will still consume the wish for lamps/wands, will NOT for candles/rings
		 * this is by-design, no real reason to punish the player,
		 * mostly for if they didn't know that X thing was unwishable
		 */
		pline1(thats_enough_tries);
		return FALSE;
	}

	/* KMH, conduct */
	u.uconduct.wishes++;

	if (otmp != &zeroobj) {

	    if (!flags.debug) {
			char llog[BUFSZ+20];
			Sprintf(llog, "wished for \"%s\"", mungspaces(bufcpy));
			livelog_write_string(llog);
	    }

	    /* The(aobjnam()) is safe since otmp is unidentified -dlc */
	    (void) hold_another_object(otmp, u.uswallow ?
				       "Oops!  %s out of your reach!" :
				       (Weightless ||
					Is_waterlevel(&u.uz) ||
					levl[u.ux][u.uy].typ < IRONBARS ||
					levl[u.ux][u.uy].typ >= ICE) ?
				       "Oops!  %s away from you!" :
				       "Oops!  %s to the floor!",
				       The(aobjnam(otmp,
					     Weightless || u.uinwater ?
						   "slip" : "drop")),
				       (const char *)0);
	    u.ublesscnt += rn1(100,50);  /* the gods take notice */
	}
	return TRUE;
}

boolean
dowand_refresh()
{
	struct obj *obj;
	const char wands[] = { WAND_CLASS, 0 };
	obj = getobj(wands, "charge");
	if(obj && obj->oclass == WAND_CLASS){
		obj->recharged = 0;
		obj->spe = (objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;
		return TRUE;
	}
	return FALSE;
}

#endif /*OVL2*/

/*zap.c*/
