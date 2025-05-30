/*	SCCS Id: @(#)dig.c	3.4	2003/03/23	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* #define DEBUG */	/* turn on for diagnostics */

#ifdef OVLB

extern const int monstr[];

static NEARDATA boolean did_dig_msg;

STATIC_DCL void NDECL(fakerocktrap);
STATIC_DCL void NDECL(openfakedoor);
STATIC_DCL boolean NDECL(rm_waslit);
STATIC_DCL void FDECL(mkcavepos, (XCHAR_P,XCHAR_P,int,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void FDECL(mkcavearea, (BOOLEAN_P));
STATIC_DCL int FDECL(dig_typ, (struct obj *,XCHAR_P,XCHAR_P));
STATIC_DCL int NDECL(dig);

/* Indices returned by dig_typ() */
#define DIGTYP_UNDIGGABLE 0
#define DIGTYP_ROCK       1
#define DIGTYP_STATUE     2
#define DIGTYP_BOULDER    3
#define DIGTYP_CRATE      4
#define DIGTYP_MS_O_STF   5
#define DIGTYP_DOOR       6
#define DIGTYP_TREE       7
#define DIGTYP_BARS       8
#define DIGTYP_BRIDGE     9


STATIC_OVL boolean
rm_waslit()
{
    register xchar x, y;

    if(levl[u.ux][u.uy].typ == ROOM && levl[u.ux][u.uy].waslit)
	return(TRUE);
    for(x = u.ux-2; x < u.ux+3; x++)
	for(y = u.uy-1; y < u.uy+2; y++)
	    if(isok(x,y) && levl[x][y].waslit) return(TRUE);
    return(FALSE);
}

/* Change level topology.  Messes with vision tables and ignores things like
 * boulders in the name of a nice effect.  Vision will get fixed up again
 * immediately after the effect is complete.
 */
STATIC_OVL void
mkcavepos(x, y, dist, waslit, rockit)
    xchar x,y;
    int dist;
    boolean waslit, rockit;
{
    register struct rm *lev;

    if(!isok(x,y)) return;
    lev = &levl[x][y];

    if(rockit) {
	register struct monst *mtmp;

	if(IS_ROCK(lev->typ)) return;
	if(t_at(x, y)) return; /* don't cover the portal */
	if ((mtmp = m_at(x, y)) != 0)	/* make sure crucial monsters survive */
	    if(!mon_resistance(mtmp,PASSES_WALLS)) return;
    } else if(lev->typ == ROOM) return;

    unblock_point(x,y);	/* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if(dist < 3) lev->lit = (rockit ? FALSE : TRUE);
    if(waslit) lev->waslit = (rockit ? FALSE : TRUE);
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 3 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;
    lev->typ = (rockit ? STONE : ROOM);
    if(dist >= 3)
	impossible("mkcavepos called with dist %d", dist);
    if(Blind)
	feel_location(x, y);
    else newsym(x,y);

	if (rockit)
		block_point(x, y);
	else
		unblock_point(x, y);
}

STATIC_OVL void
mkcavearea(rockit)
register boolean rockit;
{
    int dist;
    xchar xmin = u.ux, xmax = u.ux;
    xchar ymin = u.uy, ymax = u.uy;
    register xchar i;
    register boolean waslit = rm_waslit();

    if(rockit) pline("Crash!  The ceiling collapses around you!");
    else pline("A mysterious force %s cave around you!",
	     (levl[u.ux][u.uy].typ == CORR) ? "creates a" : "extends the");
    display_nhwindow(WIN_MESSAGE, TRUE);

    for(dist = 1; dist <= 2; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist < 2) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkcavepos(i, ymin, dist, waslit, rockit);
		mkcavepos(i, ymax, dist, waslit, rockit);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkcavepos(xmin, i, dist, waslit, rockit);
	    mkcavepos(xmax, i, dist, waslit, rockit);
	}

	flush_screen(1);	/* make sure the new glyphs shows up */
	delay_output();
    }

    if(!rockit && levl[u.ux][u.uy].typ == CORR) {
	levl[u.ux][u.uy].typ = ROOM;
	if(waslit) levl[u.ux][u.uy].waslit = TRUE;
	newsym(u.ux, u.uy); /* in case player is invisible */
    }

    vision_full_recalc = 1;	/* everything changed */
}

/* When digging into location <x,y>, what are you actually digging into? */
STATIC_OVL int
dig_typ(otmp, x, y)
struct obj *otmp;
xchar x, y;
{
	boolean ispick = is_pick(otmp),
		is_saber = is_lightsaber(otmp),
		is_seismic = (otmp->otyp == SEISMIC_HAMMER && otmp->ovar1_charges > 0),
		is_axe = is_axe(otmp);

	return ((ispick||is_saber||is_seismic) && sobj_at(STATUE, x, y) ? DIGTYP_STATUE :
		(ispick||is_saber||is_seismic) && sobj_at(BOULDER, x, y) ? DIGTYP_BOULDER :
		(ispick||is_saber||is_seismic) && sobj_at(MASSIVE_STONE_CRATE, x, y) ? DIGTYP_CRATE :
		(ispick||is_saber||is_seismic) && sobj_at(MASS_OF_STUFF, x, y) ? DIGTYP_MS_O_STF :
		(closed_door(x, y) ||
			levl[x][y].typ == SDOOR) ? DIGTYP_DOOR :
		IS_TREES(levl[x][y].typ) ?
			((ispick && !(is_axe || is_saber || is_seismic)) ? DIGTYP_UNDIGGABLE : DIGTYP_TREE) :
		(ispick || is_seismic) && IS_ROCK(levl[x][y].typ) &&
			(!level.flags.arboreal || IS_WALL(levl[x][y].typ)) ?
			DIGTYP_ROCK : 
		is_saber && (levl[x][y].typ == DRAWBRIDGE_DOWN || is_drawbridge_wall(x, y) >= 0) ?
			DIGTYP_BRIDGE : 
		is_saber && IS_WALL(levl[x][y].typ) ?
			DIGTYP_ROCK : 
		(is_saber || is_seismic) && levl[x][y].typ == IRONBARS ? DIGTYP_BARS : 
			DIGTYP_UNDIGGABLE);
}

boolean
is_digging()
{
	if (occupation == dig) {
	    return TRUE;
	}
	return FALSE;
}

#define BY_YOU		(&youmonst)
#define BY_OBJECT	((struct monst *)0)

boolean
dig_check(madeby, verbose, x, y)
	struct monst	*madeby;
	boolean		verbose;
	int		x, y;
{
	struct trap *ttmp = t_at(x, y);
	const char *verb = 
	    (madeby != BY_YOU || !uwep || is_pick(uwep)) ? "dig in" :
	    	is_lightsaber(uwep) ? "cut" :
	    	"chop";

	if (On_stairs(x, y)) {
	    if (x == xdnladder || x == xupladder) {
		if(verbose) pline_The("ladder resists your effort.");
	    } else if(verbose) pline_The("stairs are too hard to %s.", verb);
	    return(FALSE);
	/* ALI - Artifact doors, from Slash'em */
	} else if (IS_DOOR(levl[x][y].typ) && artifact_door(x, y)) {
	    if(verbose) pline_The("%s here is too hard to dig in.",
				  surface(x,y));
	    return(FALSE);
	} else if (IS_THRONE(levl[x][y].typ) && madeby != BY_OBJECT) {
	    if(verbose) pline_The("throne is too hard to break apart.");
	    return(FALSE);
	} else if (IS_ALTAR(levl[x][y].typ) && (madeby != BY_OBJECT ||
		Is_astralevel(&u.uz) || Is_sanctum(&u.uz) || (Role_if(PM_EXILE) && Is_nemesis(&u.uz)))
	) {
	    if(verbose) pline_The("altar is too hard to break apart.");
	    return(FALSE);
	} else if (Weightless) {
	    if(verbose) You("cannot %s thin air.", verb);
	    return(FALSE);
	} else if (Is_waterlevel(&u.uz)) {
	    if(verbose) pline_The("water splashes and subsides.");
	    return(FALSE);
	} else if ((IS_ROCK(levl[x][y].typ) && levl[x][y].typ != SDOOR &&
		      (levl[x][y].wall_info & W_NONDIGGABLE) != 0)
		|| (ttmp &&
		      (ttmp->ttyp == MAGIC_PORTAL || !Can_dig_down(&u.uz)))) {
	    if(verbose) pline_The("%s here is too hard to %s.",
				  surface(x,y), verb);
	    return(FALSE);
	} else if (boulder_at(x, y)) {
	    if(verbose) There("isn't enough room to %s here.", verb);
	    return(FALSE);
	} else if (madeby == BY_OBJECT &&
		    /* the block against existing traps is mainly to
		       prevent broken wands from turning holes into pits */
		    (ttmp || is_pool(x,y, TRUE) || is_lava(x,y))) {
	    /* digging by player handles pools separately */
	    return FALSE;
	}
	return(TRUE);
}

STATIC_OVL int
dig()
{
	register xchar dpx = digging.pos.x, dpy = digging.pos.y;
	register struct rm *lev = &levl[dpx][dpy];
	register boolean ispick = ((uwep && ((is_pick(uwep) || (is_lightsaber(uwep) && litsaber(uwep))) || (uwep->otyp == SEISMIC_HAMMER))) || (uarmg && is_pick(uarmg)));
	int bonus;
	struct obj *digitem = ((uwep && ((is_pick(uwep) || (is_lightsaber(uwep) && litsaber(uwep))) || (uwep->otyp == SEISMIC_HAMMER)))) ? uwep : 
		(uwep && is_axe(uwep) && IS_TREES(lev->typ)) ? uwep :
		(uarmg && is_pick(uarmg)) ? uarmg : uwep;
	const char *verb =
	    (!uwep || is_pick(uwep)) ? "dig into" :
		    is_lightsaber(uwep) ? "cut through" :
		    (uwep->otyp == SEISMIC_HAMMER) ? "smash through" :
		    "chop through";
	/* perhaps a nymph stole your pick-axe while you were busy digging */
	/* or perhaps you teleported away */
	/* WAC allow lightsabers */
	if (u.uswallow || !(ispick || (digitem && is_axe(digitem))) ||
	    !on_level(&digging.level, &u.uz) ||
	    ((digging.down ? (dpx != u.ux || dpy != u.uy)
			   : (distu(dpx,dpy) > 2)))
	) return MOVE_CANCELLED;
	
	if (digging.down) {
		if(!dig_check(BY_YOU, TRUE, u.ux, u.uy)) return MOVE_CANCELLED;
	} else { /* !digging.down */
		if (IS_TREES(lev->typ) && !may_dig(dpx,dpy) &&
			dig_typ(digitem, dpx, dpy) == DIGTYP_TREE
		) {
			pline("This tree seems to be petrified.");
			return MOVE_CANCELLED;
		}
	    /* ALI - Artifact doors from Slash'em */
		if ((IS_ROCK(lev->typ) && !may_dig(dpx,dpy) &&
	    		dig_typ(digitem, dpx, dpy) == DIGTYP_ROCK) ||
			(IS_DOOR(lev->typ) && artifact_door(dpx, dpy))
		) {
			pline("This %s is too hard to %s.",
				IS_DOOR(lev->typ) ? "door" : "wall", verb);
			return MOVE_CANCELLED;
		}
	}
	if(Fumbling &&
		/* Can't exactly miss holding a lightsaber to the wall */
		!is_lightsaber(digitem) &&
		!rn2(3)) {
	    switch(rn2(3)) {
	    case 0:
		if(!welded(digitem) && !(digitem->owornmask)) {
		    You("fumble and drop your %s.", xname(digitem));
		    dropx(digitem);
		} else {
#ifdef STEED
		    if (u.usteed)
			Your("%s %s and %s %s!",
			     xname(digitem),
			     otense(digitem, "bounce"), otense(digitem, "hit"),
			     mon_nam(u.usteed));
		    else
#endif
			pline("Ouch!  Your %s %s and %s you!",
			      xname(digitem),
			      otense(digitem, "bounce"), otense(digitem, "hit"));
#ifdef STEED
		    if (u.usteed) set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
			else
#endif
			if (!(uarmf && uarmf->otyp == find_jboots())) set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		}
		break;
	    case 1:
		pline("Bang!  You hit with the broad side of %s!",
		      the(xname(digitem)));
		break;
	    default: Your("swing misses its mark.");
		break;
	    }
	    return MOVE_CANCELLED;
	}

	bonus = 10 + rn2(5) + abon() +
			   digitem->spe - greatest_erosion(digitem) + u.udaminc + aeshbon();

	if(uarmg && uarmg->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(uarmg, IEA_INC_DAM))
		bonus += uarmg->spe;

	if (Race_if(PM_DWARF))
	    bonus *= 2;
	if (digitem->oartifact == ART_GREAT_CLAWS_OF_URDLEN)
	    bonus *= 4;
	if (lev->typ == DEADTREE)
	    bonus *= 2;
	if (is_lightsaber(digitem) && !IS_TREES(lev->typ))
	    bonus -= 11; /* Melting a hole takes longer */
	if ((digitem->otyp == SEISMIC_HAMMER) && digitem->ovar1_charges-- > 0)
	    bonus += 1000; /* Smashes through */

	digging.effort += bonus;

	if (digging.down) {
		register struct trap *ttmp;

		if (digging.effort > 250) {
		    (void) dighole(FALSE);
		    (void) memset((genericptr_t)&digging, 0, sizeof digging);
		    return MOVE_FINISHED_OCCUPATION;	/* done with digging */
		}

		if (digging.effort <= 50 ||
		    is_lightsaber(digitem) ||
		    ((ttmp = t_at(dpx,dpy)) != 0 &&
			(ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT ||
			 ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)))
		    return MOVE_STANDARD;

		if (IS_ALTAR(lev->typ)) {
		    altar_wrath(dpx, dpy);
		    angry_priest();
		}

		if (dighole(TRUE)) {	/* make pit at <u.ux,u.uy> */
		    digging.level.dnum = 0;
		    digging.level.dlevel = -1;
		}
		return MOVE_FINISHED_OCCUPATION;
	}

	if (digging.effort > 100) {
		register const char *digtxt = (char *) 0, *dmgtxt = (const char*) 0;
		register struct obj *obj;
		register boolean shopedge = *in_rooms(dpx, dpy, SHOPBASE);

		if ((obj = sobj_at(STATUE, dpx, dpy)) != 0) {
			struct obj *bobj;
			
			if (break_statue(obj))
				digtxt = "The statue shatters.";
			else
				/* it was a statue trap; break_statue()
				 * printed a message and updated the screen
				 */
				digtxt = (char *)0;
			if ((bobj = boulder_at(dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
		} else if ((obj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			struct obj *bobj;

			break_boulder(obj);
			if ((bobj = boulder_at(dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
			digtxt = "The boulder falls apart.";
		} else if ((obj = sobj_at(MASS_OF_STUFF, dpx, dpy)) != 0) {
			struct obj *bobj;

			break_boulder(obj);
			if ((bobj = boulder_at(dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
			digtxt = "The mass of stuff comes unstuck.";
		} else if ((obj = sobj_at(MASSIVE_STONE_CRATE, dpx, dpy)) != 0) {
			struct obj *bobj;

			break_boulder(obj);
			if ((bobj = boulder_at(dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
			digtxt = "The crate falls open.";
		} else if (lev->typ == DRAWBRIDGE_DOWN) {
		    destroy_drawbridge(dpx, dpy);
			spoteffects(FALSE);
		} else if ((is_drawbridge_wall(dpx, dpy) >= 0)) {
		    int x = dpx, y = dpy;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&x, &y);
		    destroy_drawbridge(x, y);
			spoteffects(FALSE);
		} else if (lev->typ == STONE || lev->typ == SCORR ||
				IS_TREES(lev->typ) || lev->typ == IRONBARS) {
			if(Is_earthlevel(&u.uz)) {
			    if(digitem->blessed && !rn2(3)) {
				mkcavearea(FALSE);
				goto cleanup;
			    } else if((digitem->cursed && !rn2(4)) ||
					  (!digitem->blessed && !rn2(6))) {
				mkcavearea(TRUE);
				goto cleanup;
			    }
			}
			if (IS_TREES(lev->typ)) {
				int numsticks;
				struct obj *staff;
				if(digitem->otyp == SEISMIC_HAMMER)
					digtxt = "You splinter the tree.";
				else
					digtxt = "You cut down the tree.";
			    lev->typ = SOIL;
			    if (!(lev->looted & TREE_LOOTED) && !rn2(5)){
					if(!In_neu(&u.uz) &&
						u.uz.dnum != chaos_dnum &&
						!Is_medusa_level(&u.uz) &&
						!(In_quest(&u.uz) && (Role_if(PM_NOBLEMAN) ||
						Race_if(PM_DROW) || 
						(Race_if(PM_ELF) && (Role_if(PM_RANGER) || Role_if(PM_PRIEST) || Role_if(PM_NOBLEMAN) || Role_if(PM_WIZARD)))
						)) &&
						u.uz.dnum != tower_dnum
					) (void) rnd_treefruit_at(dpx, dpy);
				}
				for(numsticks = d(2,4)-1; numsticks > 0; numsticks--){
					staff = mksobj_at(rn2(2) ? QUARTERSTAFF : CLUB, dpx, dpy, MKOBJ_NOINIT);
					set_material_gm(staff, WOOD);
					staff->spe = 0;
					staff->cursed = staff->blessed = FALSE;
				}
				if(!flags.mon_moving && u.sealsActive&SEAL_EDEN) unbind(SEAL_EDEN,TRUE);
			} else if (lev->typ == IRONBARS) {
				if(digitem->otyp == SEISMIC_HAMMER)
					digtxt = "You shatter the bars.";
				else
					digtxt = "You cut through the bars.";
				break_iron_bars(dpx, dpy, FALSE);
			} else {
				struct obj *otmp;
				if(!is_lightsaber(digitem)){
					if(!Is_belial_level(&u.uz) && !rn2(20)){
						otmp = mksobj_at(BOULDER, dpx, dpy, MKOBJ_NOINIT);
						otmp->owt = weight(otmp);
					} else {
						if(Is_belial_level(&u.uz)){
							otmp = mksobj_at(DROVEN_DAGGER, dpx, dpy, MKOBJ_NOINIT);
							set_material_gm(otmp, OBSIDIAN_MT);
						} else {
							otmp = mksobj_at(ROCK, dpx, dpy, MKOBJ_NOINIT);
						}
						set_obj_quan(otmp, rn1(20, 20));
					}
				}
				if(digitem->otyp == SEISMIC_HAMMER)
					digtxt = "You succeed in smashing some rock.";
				else
					digtxt = "You succeed in cutting away some rock.";
			    lev->typ = CORR;
			}
		} else if(IS_WALL(lev->typ)) {
			struct obj *otmp;
			if(IS_WALL(lev->typ) && u.sealsActive&SEAL_ANDREALPHUS) unbind(SEAL_ANDREALPHUS,TRUE);
			if(shopedge) {
			    add_damage(dpx, dpy, 10L * ACURRSTR);
			    dmgtxt = "damage";
			}
			if (level.flags.is_maze_lev) {
			    lev->typ = ROOM;
			} else if (level.flags.is_cavernous_lev &&
				   !in_town(dpx, dpy)) {
			    lev->typ = CORR;
			} else {
			    lev->typ = DOOR;
			    lev->doormask = D_NODOOR;
			}
			if(!is_lightsaber(digitem)){
				if(Is_belial_level(&u.uz)){
					otmp = mksobj_at(DROVEN_DAGGER, dpx, dpy, MKOBJ_NOINIT);
					set_material_gm(otmp, OBSIDIAN_MT);
				} else {
					otmp = mksobj_at(ROCK, dpx, dpy, MKOBJ_NOINIT);
				}
				set_obj_quan(otmp, rn1(20, 20));
			}
			digtxt = "You make an opening in the wall.";
		} else if(lev->typ == SDOOR) {
			cvt_sdoor_to_door(lev);	/* ->typ = DOOR */
			digtxt = "You break through a secret door!";
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else if(closed_door(dpx, dpy)) {
			digtxt = "You break through the door.";
			if(!(lev->doormask&D_LOCKED) && u.sealsActive&SEAL_OTIAX) unbind(SEAL_OTIAX, TRUE);
			if(shopedge) {
			    add_damage(dpx, dpy, 400L);
			    dmgtxt = "break";
			}
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else return MOVE_CANCELLED; /* statue or boulder got taken */

		if(!does_block(dpx,dpy,&levl[dpx][dpy]))
		    unblock_point(dpx,dpy);	/* vision:  can see through */
		if(Blind)
		    feel_location(dpx, dpy);
		else
		    newsym(dpx, dpy);
		if(digtxt && !digging.quiet) pline1(digtxt); /* after newsym */
		if(dmgtxt)
		    pay_for_damage(dmgtxt, FALSE);

		if(Is_earthlevel(&u.uz) && !rn2(3)) {
		    register struct monst *mtmp;

		    switch(rn2(2)) {
		      case 0:
			mtmp = makemon(&mons[PM_EARTH_ELEMENTAL],
					dpx, dpy, NO_MM_FLAGS);
			break;
		      default:
			mtmp = makemon(&mons[PM_XORN],
					dpx, dpy, NO_MM_FLAGS);
			break;
		    }
		    if(mtmp) pline_The("debris from your digging comes to life!");
		}
		if(IS_DOOR(lev->typ) && (lev->doormask & D_TRAPPED)) {
			lev->doormask = D_NODOOR;
			b_trapped("door", 0);
			newsym(dpx, dpy);
		}
cleanup:
		digging.lastdigtime = moves;
		digging.quiet = FALSE;
		digging.level.dnum = 0;
		digging.level.dlevel = -1;
		return MOVE_FINISHED_OCCUPATION;
	} else {		/* not enough effort has been spent yet */
		static const char *const d_target[10] = {
			"", "rock", "statue", "boulder", "crate", "mass", "door", "tree", "bars", "chains"
		};
		int dig_target = dig_typ(digitem, dpx, dpy);
		
		if(digitem && is_lightsaber(digitem)) digitem->age -= 100;
		
		if (IS_WALL(lev->typ) || dig_target == DIGTYP_DOOR) {
		    if(*in_rooms(dpx, dpy, SHOPBASE)) {
			pline("This %s seems too hard to %s.",
			      IS_DOOR(lev->typ) ? "door" : "wall", verb);
			return MOVE_CANCELLED;
		    }
		} else if (!IS_ROCK(lev->typ) && dig_target == DIGTYP_ROCK)
		    return MOVE_CANCELLED; /* statue or boulder got taken */
		if(!did_dig_msg) {
		    if (is_lightsaber(digitem)) You("burn steadily through the %s.",
			d_target[dig_target]);
		    else
		    You("hit the %s with all your might.",
			d_target[dig_target]);
		    did_dig_msg = TRUE;
		}
	}
	return MOVE_STANDARD;
}

/* When will hole be finished? Very rough indication used by shopkeeper. */
int
holetime()
{
	if(occupation != dig || !*u.ushops) return(-1);
	return ((250 - digging.effort) / 20);
}

/* Return typ of liquid to fill a hole with, or ROOM, if no liquid nearby */
schar
fillholetyp(x,y)
int x, y;
{
    register int x1, y1;
    int lo_x = max(1,x-1), hi_x = min(x+1,COLNO-1),
	lo_y = max(0,y-1), hi_y = min(y+1,ROWNO-1);
    int pool_cnt = 0, moat_cnt = 0, lava_cnt = 0;

    for (x1 = lo_x; x1 <= hi_x; x1++)
	for (y1 = lo_y; y1 <= hi_y; y1++)
	    if (levl[x1][y1].typ == POOL)
		pool_cnt++;
	    else if (levl[x1][y1].typ == MOAT ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_MOAT))
		moat_cnt++;
	    else if (levl[x1][y1].typ == LAVAPOOL ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_LAVA))
		lava_cnt++;
    pool_cnt /= 3;		/* not as much liquid as the others */

    if (lava_cnt > moat_cnt + pool_cnt && rn2(lava_cnt + 1))
	return LAVAPOOL;
    else if ((moat_cnt > 0 && rn2(moat_cnt + 1)) ||
			(Is_paradise(&u.uz) && !rn2(10)))
	return MOAT;
    else if (pool_cnt > 0 && rn2(pool_cnt + 1))
	return POOL;
    else
	return ROOM;
}

void
digactualhole(x, y, madeby, ttyp, forceknown, msgs)
register int	x, y;
struct monst	*madeby;
int ttyp;
boolean forceknown;
boolean msgs;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);	/* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);
	boolean wont_fall = Levitation || Flying;

	if (u.utrap && u.utraptype == TT_INFLOOR) u.utrap = 0;

	/* these furniture checks were in dighole(), but wand
	   breaking bypasses that routine and calls us directly */
	if (IS_FOUNTAIN(lev->typ)) {
	    dogushforth(FALSE);
	    SET_FOUNTAIN_WARNED(x,y);		/* force dryup */
	    dryup(x, y, madeby_u);
		if(!flags.mon_moving && u.sealsActive&SEAL_EDEN) unbind(SEAL_EDEN,TRUE);
	    return;
	} else if (IS_FORGE(lev->typ)) {
	    breakforge(x, y);
	    return;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
	    breaksink(x, y);
	    return;
#endif
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(x, y) >= 0)) {
	    int bx = x, by = y;
	    /* if under the portcullis, the bridge is adjacent */
	    (void) find_drawbridge(&bx, &by);
	    destroy_drawbridge(bx, by);
	    return;
	}

	if (ttyp != PIT && !Can_dig_down(&u.uz)) {
	    impossible("digactualhole: can't dig %s on this level.",
		       defsyms[trap_to_defsym(ttyp)].explanation);
	    ttyp = PIT;
	}

	/* maketrap() might change it, also, in this situation,
	   surface() returns an inappropriate string for a grave */
	if (IS_GRAVE(lev->typ))
	    Strcpy(surface_type, "grave");
	else
	    Strcpy(surface_type, surface(x,y));
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[x][y];
	ttmp->tseen = (forceknown || madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	if (ttyp == PIT) {

	    if(madeby_u) {
		You("dig a pit in the %s.", surface_type);
		if (shopdoor) pay_for_damage("ruin", FALSE);
	    } else if (!madeby_obj && canseemon(madeby) && msgs)
		pline("%s digs a pit in the %s.", Monnam(madeby), surface_type);
	    else if (cansee(x, y) && flags.verbose && msgs)
		pline("A pit appears in the %s.", surface_type);

	    if(at_u) {
		if (!wont_fall) {
		    if (!Passes_walls)
			u.utrap = rn1(4,2);
		    u.utraptype = TT_PIT;
		    vision_full_recalc = 1;	/* vision limits change */
		} else
		    u.utrap = 0;
		if (oldobjs != newobjs)	/* something unearthed */
			(void) pickup(1);	/* detects pit */
	    } else if(mtmp && !DEADMONSTER(mtmp)) {
			if(mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION)) {
				if(canseemon(mtmp))
				pline("%s %s over the pit.", Monnam(mtmp),
								 (mon_resistance(mtmp,FLYING)) ?
								 "flies" : "floats");
			} else if(mtmp != madeby)
				(void) mintrap(mtmp);
	    }
	} else {	/* was TRAPDOOR now a HOLE*/

	    if(madeby_u)
		You("dig a hole through the %s.", surface_type);
	    else if(!madeby_obj && canseemon(madeby) && msgs)
		pline("%s digs a hole through the %s.",
		      Monnam(madeby), surface_type);
	    else if(cansee(x, y) && flags.verbose && msgs)
		pline("A hole appears in the %s.", surface_type);

	    if (at_u) {
		if (!u.ustuck && !wont_fall && !next_to_u()) {
		    You("are jerked back by your pet!");
		    wont_fall = TRUE;
		}

		/* Floor objects get a chance of falling down.  The case where
		 * the hero does NOT fall down is treated here.  The case
		 * where the hero does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
		    if (newobjs)
			impact_drop((struct obj *)0, x, y, 0, madeby_u);
		    if (oldobjs != newobjs)
			(void) pickup(1);
		    if (shopdoor && madeby_u) pay_for_damage("ruin", FALSE);

		} else {
		    d_level newlevel;

		    if (*u.ushops && madeby_u)
			shopdig(1); /* shk might snatch pack */
		    /* handle earlier damage, eg breaking wand of digging */
		    else if (madeby_u) pay_for_damage("dig into", TRUE);

		    fall_through(TRUE);
		    /* Earlier checks must ensure that the destination
		     * level exists and is in the present dungeon.
		     */
		}
	    } else {
		if (shopdoor && madeby_u) pay_for_damage("ruin", FALSE);
		if (newobjs)
		    impact_drop((struct obj *)0, x, y, 0, madeby_u);
		if (mtmp && !DEADMONSTER(mtmp)) {
		     /*[don't we need special sokoban handling here?]*/
		    if (mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION) ||
		        mtmp->mtyp == PM_WUMPUS ||
			(mtmp->wormno && count_wsegs(mtmp) > 5) ||
			mtmp->data->msize >= MZ_HUGE) return;
		    if (mtmp == u.ustuck)	/* probably a vortex */
			    return;		/* temporary? kludge */

		    if (teleport_pet(mtmp, FALSE)) {
			d_level tolevel;

			if (Is_stronghold(&u.uz)) {
			    assign_level(&tolevel, &valley_level);
			} else if (Is_botlevel(&u.uz)) {
			    if (canseemon(mtmp) && msgs)
				pline("%s avoids the trap.", Monnam(mtmp));
			    return;
			} else {
			    get_level(&tolevel, depth(&u.uz) + 1);
			}
			if (mtmp->isshk) make_angry_shk(mtmp, 0, 0);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
		    }
		}
	    }
	}
}

void
openactualdoor(x, y, madeby, ttyp)
register int	x, y;
struct monst	*madeby;
int ttyp;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);	/* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);
	boolean wont_fall = Levitation || Flying;

	if (u.utrap && u.utraptype == TT_INFLOOR) u.utrap = 0;

	if (ttyp == TRAPDOOR && !Can_dig_down(&u.uz)) {
	    impossible("openactaldoor: can't open %s on this level.",
		       defsyms[trap_to_defsym(ttyp)].explanation);
	    ttyp = PIT;
	}

	/* maketrap() might change it, also, in this situation,
	   surface() returns an inappropriate string for a grave */
	if (IS_GRAVE(lev->typ))
	    Strcpy(surface_type, "grave");
	else
	    Strcpy(surface_type, surface(x,y));
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[x][y];
	ttmp->tseen = (madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	pline("A trapdoor opens in the %s.", surface_type);
	if (ttyp == PIT) {

	    if(madeby_u) {
			if (shopdoor) pay_for_damage("ruin", FALSE);
	    }
	    if(at_u) {
		if (!wont_fall) {
		    if (!Passes_walls)
			u.utrap = rn1(4,2);
		    u.utraptype = TT_PIT;
		    vision_full_recalc = 1;	/* vision limits change */
		} else
		    u.utrap = 0;
		if (oldobjs != newobjs)	/* something unearthed */
			(void) pickup(1);	/* detects pit */
	    } else if(mtmp) {
		if(mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION)) {
		    if(canseemon(mtmp))
			pline("%s %s over the trapdoor.", Monnam(mtmp),
						     (mon_resistance(mtmp,FLYING)) ?
						     "flies" : "floats");
		} else if(mtmp != madeby)
		    (void) mintrap(mtmp);
	    }
	} else {	/* was TRAPDOOR now a HOLE*/

	    if (at_u) {
		if (!u.ustuck && !wont_fall && !next_to_u()) {
		    You("are jerked back by your pet!");
		    wont_fall = TRUE;
		}

		/* Floor objects get a chance of falling down.  The case where
		 * the hero does NOT fall down is treated here.  The case
		 * where the hero does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
		    if (newobjs)
			impact_drop((struct obj *)0, x, y, 0, madeby_u);
		    if (oldobjs != newobjs)
			(void) pickup(1);
		    if (shopdoor && madeby_u) pay_for_damage("ruin", FALSE);

		} else {
		    d_level newlevel;

		    if (*u.ushops && madeby_u)
			shopdig(1); /* shk might snatch pack */
		    /* handle earlier damage, eg breaking wand of digging */
		    else if (madeby_u) pay_for_damage("dig into", TRUE);

		    You("fall through...");
		    /* Earlier checks must ensure that the destination
		     * level exists and is in the present dungeon.
		     */
		    newlevel.dnum = u.uz.dnum;
		    newlevel.dlevel = u.uz.dlevel + 1;
		    goto_level(&newlevel, FALSE, TRUE, FALSE);
		    /* messages for arriving in special rooms */
		    spoteffects(FALSE);
		}
	    } else {
		if (shopdoor && madeby_u) pay_for_damage("ruin", FALSE);
		if (newobjs)
		    impact_drop((struct obj *)0, x, y, 0, madeby_u);
		if (mtmp) {
		     /*[don't we need special sokoban handling here?]*/
		    if (mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION) ||
		        mtmp->mtyp == PM_WUMPUS ||
			(mtmp->wormno && count_wsegs(mtmp) > 5) ||
			mtmp->data->msize >= MZ_HUGE) return;
		    if (mtmp == u.ustuck)	/* probably a vortex */
			    return;		/* temporary? kludge */

		    if (teleport_pet(mtmp, FALSE)) {
			d_level tolevel;

			if (Is_stronghold(&u.uz)) {
			    assign_level(&tolevel, &valley_level);
			} else if (Is_botlevel(&u.uz)) {
			    if (canseemon(mtmp))
				pline("%s avoids the trap.", Monnam(mtmp));
			    return;
			} else {
			    get_level(&tolevel, depth(&u.uz) + 1);
			}
			if (mtmp->isshk) make_angry_shk(mtmp, 0, 0);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
		    }
		}
	    }
	}
}

void
openactualrocktrap(x, y, madeby)
register int	x, y;
struct monst	*madeby;
{
	int ttyp = ROCKTRAP;
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);	/* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);

	if (u.utrap && u.utraptype == TT_INFLOOR) u.utrap = 0;
	
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	ttmp->tseen = (madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	pline("A trapdoor opens in the %s.", ceiling(x,y));
	if(madeby_u) {
		if (shopdoor) pay_for_damage("ruin", FALSE);
	}
	if(at_u) {
		u.utrap = 0;
		dotrap(ttmp, 0);
	} else if(mtmp) {
		(void) mintrap(mtmp);
	}
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
dighole(pit_only)
boolean pit_only;
{
	register struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) ||
	   /* ALI - artifact doors from slash'em */
	   (IS_DOOR(levl[u.ux][u.uy].typ) && artifact_door(u.ux, u.uy)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("%s here is too hard to dig in.", surface(u.ux,u.uy));

	} else if (is_pool(u.ux, u.uy, TRUE) || is_lava(u.ux, u.uy)) {
		pline_The("%s sloshes furiously for a moment, then subsides.",
			is_lava(u.ux, u.uy) ? "lava" : "water");
		wake_nearby();	/* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (pit_only) {
		    pline_The("drawbridge seems too hard to dig through.");
		    return FALSE;
		} else {
		    int x = u.ux, y = u.uy;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&x, &y);
		    destroy_drawbridge(x, y);
		    return TRUE;
		}

	} else if ((boulder_here = boulder_at(u.ux, u.uy)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
		    rn2(2)) {
			pline_The("%s settles into the pit.", xname(boulder_here) );
			ttmp->ttyp = PIT;	 /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
			pline("KADOOM! The %s falls in!", xname(boulder_here) );
			(void) delfloortrap(ttmp);
		}
		delobj(boulder_here);
		return TRUE;

	} else if (IS_GRAVE(lev->typ)) {
	    digactualhole(u.ux, u.uy, BY_YOU, PIT, FALSE, TRUE);
	    dig_up_grave(u.ux, u.uy);
	    return TRUE;
	} else if (IS_SEAL(lev->typ)) {
	    // digactualhole(u.ux, u.uy, BY_YOU, PIT, FALSE, TRUE);
	    break_seal(u.ux, u.uy);
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
			pline_The("%s here is too hard to dig in.",
			      surface(u.ux, u.uy));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(u.ux, u.uy);

		pline("As you dig, the hole fills with %s!",
		      typ == LAVAPOOL ? "lava" : "water");
		if (!Levitation && !Flying) {
		    if (typ == LAVAPOOL)
			(void) lava_effects(TRUE);
		    else if (!Wwalking)
			(void) drown();
		}
		return TRUE;

	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
		pline_The("throne is too hard to break apart.");

	} else if (IS_ALTAR(lev->typ)) {
		pline_The("altar is too hard to break apart.");

	} else {
		typ = fillholetyp(u.ux,u.uy);

		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(u.ux, u.uy, BY_YOU, PIT, FALSE, TRUE);
		else
			digactualhole(u.ux, u.uy, BY_YOU, HOLE, FALSE, TRUE);

		return TRUE;
	}

	return FALSE;
}

STATIC_DCL
void
openfakedoor()
{
	d_level dtmp;
	char msgbuf[BUFSZ];
	int newlevel = dunlev(&u.uz);
	
	if(!Blind && !(Levitation || Flying)) pline("A trap door opens up under you!");
	
	if(Levitation || Flying){
		if(!In_sokoban(&u.uz)) return;
		else {
			pline("Powerful winds force you down!");
			losehp(rnd(2), "dangerous winds", KILLED_BY);
		}
	}
	
	if(*u.ushops) shopdig(1);

	if (Is_stronghold(&u.uz)) {
		find_hell(&dtmp);
	} else {
		do {
			newlevel++;
		} while(!rn2(5) && newlevel < dunlevs_in_dungeon(&u.uz));
		
		dtmp.dnum = u.uz.dnum;
		dtmp.dlevel = newlevel;
	}
	Sprintf(msgbuf, "The hole in the %s above you closes up.",
		ceiling(u.ux,u.uy));
	schedule_goto(&dtmp, FALSE, TRUE, 0,
			  (char *)0, !Blind ? msgbuf : (char *)0, 0, 0);
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
opentrapdoor(pit_only)
boolean pit_only;
{
	d_level dtmp;
	int newlevel = dunlev(&u.uz);
	struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);
	
	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL)) ||
	   /* ALI - artifact doors from slash'em */
	   (IS_DOOR(levl[u.ux][u.uy].typ) && artifact_door(u.ux, u.uy)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("%s here refuses to open.", surface(u.ux,u.uy));
		
		return FALSE;
	} else if (is_pool(u.ux, u.uy, TRUE) || is_lava(u.ux, u.uy)) {
		You("can't open a door into a liquid!");
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (lev->typ == DBWALL ) {
			open_drawbridge(u.ux, u.uy);
			return TRUE;
		} else {
		    pline_The("drawbridge is already open.");
		    return FALSE;
		}

	} else if ((boulder_here = boulder_at(u.ux, u.uy)) != 0) {
		/*
		 * digging makes a hole, but the boulder immediately
		 * fills it.  Final outcome:  no hole, no boulder.
		 */
		pline("KADOOM! The %s falls in!", xname(boulder_here) );
		(void) delfloortrap(ttmp);
		delobj(boulder_here);
		return TRUE;
	} else if (IS_GRAVE(lev->typ)) {
		/* You succeed in opening the grave.*/
	    openactualdoor(u.ux, u.uy, BY_YOU, PIT);
	    dig_up_grave(u.ux, u.uy);
	    return TRUE;
	} else if (IS_SEAL(lev->typ)) {
	    // openactualdoor(u.ux, u.uy, BY_YOU, PIT);
	    break_seal(u.ux, u.uy);
	    return TRUE;
	} else if (IS_DOOR(lev->typ)) {
		if(lev->doormask == D_NODOOR){
			goto goodspot;
		} else if(lev->doormask & D_BROKEN){
			pline("The door is broken open.");
			return FALSE;
		} else if(lev->doormask & D_ISOPEN){
			pline("The door is already open.");
			return FALSE;
		} else {
			pline("The door opens.");
			lev->doormask &= ~(D_CLOSED|D_LOCKED);
			lev->doormask |= D_ISOPEN;
			if (Blind)
			feel_location(u.ux,u.uy);	/* the hero knows she opened it  */
			else
			newsym(u.ux,u.uy);
			unblock_point(u.ux,u.uy);		/* vision: new see through there */
		}
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			if(!pit_only){
				openfakedoor();
				return TRUE;
			} else {
				pline_The("%s here refuses to open.", surface(u.ux,u.uy));
				return FALSE;
			}
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(u.ux, u.uy);

		pline("Though the trapdoor opens, it is immediately filled with %s!",
		      typ == LAVAPOOL ? "lava" : "water");
		if (!Levitation && !Flying) {
		    if (typ == LAVAPOOL)
			(void) lava_effects(TRUE);
		    else if (!Wwalking)
			(void) drown();
		}
		return TRUE;

	} else if (IS_FOUNTAIN(lev->typ)) {
		if(!pit_only){
			openfakedoor();
			return TRUE;
		} else {
			pline_The("%s here refuses to open.", surface(u.ux,u.uy));
			return FALSE;
		}
	} else if (IS_FORGE(lev->typ)) {
		if(!pit_only){
			openfakedoor();
			return TRUE;
		} else {
			pline_The("%s here refuses to open.", surface(u.ux,u.uy));
			return FALSE;
		}
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
		if(!pit_only){
			openfakedoor();
			return TRUE;
		} else {
			pline_The("%s here refuses to open.", surface(u.ux,u.uy));
			return FALSE;
		}
#endif
	} else if (IS_THRONE(lev->typ)) {
		if(!pit_only){
			openfakedoor();
			return TRUE;
		} else {
			pline_The("%s here refuses to open.", surface(u.ux,u.uy));
			return FALSE;
		}
	} else if (IS_ALTAR(lev->typ)) {
		if(!pit_only){
			openfakedoor();
			return TRUE;
		} else {
			pline_The("%s here refuses to open.", surface(u.ux,u.uy));
			return FALSE;
		}
	} else {
goodspot:
		typ = fillholetyp(u.ux,u.uy);
		
		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}
		/* finally we get to make a hole */
		if (nohole || pit_only)
			openactualdoor(u.ux, u.uy, BY_YOU, PIT);
		else
			openactualdoor(u.ux, u.uy, BY_YOU, TRAPDOOR);

		return TRUE;
	}

	return FALSE;
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
opennewdoor(x,y)
int x,y;
{
	struct rm *lev = &levl[x][y];
	schar typ;

	if (/* ALI - artifact doors from slash'em */
	   (IS_DOOR(levl[x][y].typ) && artifact_door(x, y)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("%s here refuses to open.", IS_DOOR(levl[x][y].typ) ? "door":"wall");
		
		return FALSE;
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(x, y) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (lev->typ == DBWALL ) {
			open_drawbridge(x, y);
			return TRUE;
		} else {
		    pline_The("drawbridge is already open.");
		    return FALSE;
		}
	} else if (IS_GRAVE(lev->typ)) {
		/* You succeed in opening the grave.*/
	    openactualdoor(x, y, BY_YOU, PIT);
	    dig_up_grave(x, y);
	    return TRUE;
	} else if (IS_SEAL(lev->typ)) {
	    // openactualdoor(x, y, BY_YOU, PIT);
	    break_seal(x,y);
	    return TRUE;
	} else if (IS_DOOR(lev->typ)) {
		if(lev->doormask == D_NODOOR){
			goto badspot;
		} else if(lev->doormask & D_BROKEN){
			pline("The door is broken open.");
			return FALSE;
		} else if(lev->doormask & D_ISOPEN){
			pline("The door is already open.");
			return FALSE;
		} else {
			pline("The door opens.");
			lev->doormask &= ~(D_CLOSED|D_LOCKED);
			lev->doormask |= D_ISOPEN;
			if (Blind)
				feel_location(x,y);	/* the hero knows she opened it  */
			else
				newsym(x,y);
			unblock_point(x,y);		/* vision: new see through there */
		}
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP || ZAP_POS(lev->typ)) {
badspot:
		pline("Doors can only exist in solid surfaces!");
		return FALSE;
	} else {
		lev->typ = DOOR;
		pline("A new door opens.");
		lev->doormask = D_ISOPEN;
		if (Blind)
			feel_location(x,y);	/* the hero knows she opened it  */
		else
			newsym(x,y);
		unblock_point(x,y);		/* vision: new see through there */
		return TRUE;
	}
	return FALSE;
}

STATIC_DCL
void
fakerocktrap()
{
	int dmg = d(2,6); /* should be std ROCK dmg? */
	struct obj *otmp;

	otmp = mksobj_at(ROCK, u.ux, u.uy, NO_MKOBJ_FLAGS);
	set_obj_quan(otmp, 1);

	pline("A trap door in %s opens and %s falls on your %s!",
	  the(ceiling(u.ux,u.uy)),
	  an(xname(otmp)),
	  body_part(HEAD));

	if (uarmh) {
	if(is_hard(uarmh)) {
		pline("Fortunately, you are wearing a hard helmet.");
		dmg = 2;
	} else if (flags.verbose) {
		Your("%s does not protect you.", xname(uarmh));
	}
	}

	if (!Blind) otmp->dknown = 1;
	stackobj(otmp);
	newsym(u.ux,u.uy);	/* map the rock */

	losehp(dmg, "falling rock", KILLED_BY_AN);
	exercise(A_STR, FALSE);
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
openrocktrap()
{
	struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	schar typ;

	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL)) ||
	   /* ALI - artifact doors from slash'em */
	   (IS_DOOR(levl[u.ux][u.uy].typ) && artifact_door(u.ux, u.uy)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("%s here refuses to open.", ceiling(u.ux,u.uy));
		
		return FALSE;
	} else if (is_pool(u.ux, u.uy, TRUE) || is_lava(u.ux, u.uy)) {
		fakerocktrap();
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (lev->typ == DBWALL ) {
			open_drawbridge(u.ux, u.uy);
			return TRUE;
		} else {
		    pline_The("drawbridge is already open.");
		    return FALSE;
		}

	} else if (IS_GRAVE(lev->typ)) {
		fakerocktrap();
	    return TRUE;
	} else if (IS_SEAL(lev->typ)) {
		fakerocktrap();
	    return TRUE;
	} else if (IS_DOOR(lev->typ)) {
		if(lev->doormask == D_NODOOR){
			openactualrocktrap(u.ux, u.uy, BY_YOU);
			return TRUE;
		} else if(lev->doormask & D_BROKEN){
			pline("The door is broken open.");
			return FALSE;
		} else if(lev->doormask & D_ISOPEN){
			pline("The door is already open.");
			return FALSE;
		} else {
			pline("The door opens.");
			lev->doormask &= ~(D_CLOSED|D_LOCKED);
			lev->doormask |= D_ISOPEN;
			if (Blind)
				feel_location(u.ux,u.uy);	/* the hero knows she opened it  */
			else
				newsym(u.ux,u.uy);
			unblock_point(u.ux,u.uy);		/* vision: new see through there */
		}
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		fakerocktrap();
	    return TRUE;
	} else if (IS_FOUNTAIN(lev->typ)) {
		fakerocktrap();
	    return TRUE;
	} else if (IS_FORGE(lev->typ)) {
		fakerocktrap();
	    return TRUE;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
		fakerocktrap();
	    return TRUE;
#endif
	} else if (IS_THRONE(lev->typ)) {
		fakerocktrap();
	    return TRUE;
	} else if (IS_ALTAR(lev->typ)) {
		fakerocktrap();
	    return TRUE;
	} else {
		openactualrocktrap(u.ux, u.uy, BY_YOU);
		return TRUE;
	}

	return FALSE;
}

void
dig_up_grave(x,y)
int x, y;
{
	struct obj *otmp;
	boolean revenancer = (uarmg && uarmg->oartifact == ART_CLAWS_OF_THE_REVENANCER);
	
	if(Race_if(PM_VAMPIRE) || revenancer){
		/* Wise to find allies */
		exercise(A_WIS, TRUE);
	} else {
		/* Grave-robbing is frowned upon... */
		exercise(A_WIS, FALSE);
	}
	IMPURITY_UP(u.uimp_graverobbery)
	if (Role_if(PM_ARCHEOLOGIST)) {
	    adjalign(-sgn(u.ualign.type)*3);
		u.ualign.sins++;
		change_hod(1);
	    You_feel("like a despicable grave-robber!");
	} else if (Role_if(PM_SAMURAI)) {
        if(!(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_NINJA)){
		    adjalign(-sgn(u.ualign.type)*10);//stiffer penalty
			u.ualign.sins++;
			change_hod(1);
		    You("disturb the honorable dead!");
        } else {
			adjalign(10);
			You("disturb the honorable dead!");
        }
	} else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
	    adjalign(-sgn(u.ualign.type)*2);
	    You("have violated the sanctity of this grave!");
	}

	switch (rn2(5)) {
	case 0:
	case 1:
	    You("unearth a corpse.");
	    if (!!(otmp = mk_tt_object(CORPSE, x, y)))
	    	otmp->age -= 100;		/* this is an *OLD* corpse */;
	    break;
	case 2:
		if(!Race_if(PM_VAMPIRE) && !revenancer){
			if (!Blind) pline(Hallucination ? "Dude!  The living dead!" :
				"The grave's owner is very upset!");
			(void) makemon(mkclass(S_ZOMBIE, Inhell ? G_HELL : G_NOHELL), x, y, NO_MM_FLAGS);
		} else{
			You("unearth a minion.");
			(void) initedog(makemon(mkclass(S_ZOMBIE, Inhell ? G_HELL : G_NOHELL), x, y, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH));
		}
	break;
	case 3:
		if(!Race_if(PM_VAMPIRE) && !revenancer){
			if (!Blind) pline(Hallucination ? "I want my mummy!" :
				"You've disturbed a tomb!");
			(void) makemon(mkclass(S_MUMMY, Inhell ? G_HELL : G_NOHELL), x, y, NO_MM_FLAGS);
		} else {
			You("recruit a follower from this tomb.");
			(void) initedog(makemon(mkclass(S_MUMMY, Inhell ? G_HELL : G_NOHELL), x, y, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH));
		}
    break;
	default:
	    /* No corpse */
	    pline_The("grave seems unused.  Strange....");
	    break;
	}
	levl[x][y].typ = ROOM;
	del_engr_ward_at(x, y);
	newsym(x,y);
	return;
}

void
hell_vault_items(x,y,type)
int x, y, type;
{
    struct obj *container;
	container = mksobj_at(CHEST, x, y, MKOBJ_NOINIT);
	set_material_gm(container, METAL);
	container->olocked = TRUE;
	container->otrapped = TRUE;
	for(int i = d(9,4); i > 0; i--)
		mkhellvaultitem_cnt(container, type, TRUE);
	bury_an_obj(container);
}

void
levi_spawn_items(x,y, levi)
int x, y;
struct monst *levi;
{
    struct obj *container;
	struct obj *otmp;
	container = mksobj(CHEST, MKOBJ_NOINIT);
	set_material_gm(container, levi->mtyp == PM_LEVIATHAN ? BONE : METAL);
	container->olocked = TRUE;
	container->otrapped = TRUE;
	for(int i = 5; i > 0; i--){
		otmp = mksobj(levi->mtyp == PM_LEVIATHAN ? WAGE_OF_GLUTTONY : WAGE_OF_ENVY, MKOBJ_NOINIT);
		add_to_container(container, otmp);
	}
	for(int i = 3; i > 0; i--){
		otmp = mksobj(levi->mtyp == PM_LEVIATHAN ? WAGE_OF_ENVY : WAGE_OF_LUST, MKOBJ_NOINIT);
		add_to_container(container, otmp);
	}
	otmp = mksobj(WAGE_OF_PRIDE, MKOBJ_NOINIT);
	add_to_container(container, otmp);
	for(int i = 3*9; i > 0; i--)
		mkhellvaultitem_cnt(container, VN_N_PIT_FIEND, FALSE);
	if(levi->mtyp == PM_LEVIATHAN) mpickobj(levi, container);
	else place_object(container, x, y);
}

void
break_seal(x,y)
int x, y;
{
	int i, j;
    struct trap *ttmp;
    struct obj *otmp;
    struct monst *mon;
    register struct rm *lev;
	int mid = PM_LAMB;
	if(cansee(x, y)){
		pline("The hellish seal shatters and dissolves into magenta fog!");
	}
    pline_The("floor shakes violently under you!");
	if(distmin(u.ux, u.uy, x, y) <= 2){
		pline_The("walls around you begin to bend and crumble!");
	}
	for(i=-2; i < 3; i++){
		for(j=-2; j < 3; j++){
			if(!isok(x+i,y+j))
				continue;
			//Skip the center point
			if(i || j){
				ttmp = t_at(x+i,y+j);
				lev = &levl[x+i][y+j];
				//Clear traps
				if(ttmp && ttmp->ttyp != MAGIC_PORTAL){
					deltrap(ttmp);
				}
				//Clear boulders
				while ((otmp = boulder_at(x+i, y+j)) != 0)
					break_boulder(otmp);

				if(!IS_FEATURE(lev->typ)){
					/* fake out saved state */
					lev->seenv = 0;
					lev->doormask = 0;
					lev->lit = TRUE;
					lev->horizontal = FALSE;
					lev->typ = ROOM;
					unblock_point(x+i,y+j);	/* make sure vision knows this location is open */
				}

				newsym(x+i,y+j);
			}
		}
	}

	//Decode the vault monster's ID
	switch(levl[x][y].vaulttype){
		case VN_AKKABISH:
			mid = PM_AKKABISH_TANNIN;
		break;
		case VN_SHALOSH:
			mid = PM_SHALOSH_TANNAH;
		break;
		case VN_NACHASH:
			mid = PM_NACHASH_TANNIN;
		break;
		case VN_KHAAMNUN:
			mid = PM_KHAAMNUN_TANNIN;
		break;
		case VN_RAGLAYIM:
			mid = PM_RAGLAYIM_TANNIN;
		break;
		case VN_TERAPHIM:
			mid = PM_TERAPHIM_TANNAH;
		break;
		case VN_SARTAN:
			mid = PM_SARTAN_TANNIN;
		break;
		case VN_A_O_BLESSINGS:
			mid = PM_ANCIENT_OF_BLESSINGS;
		break;
		case VN_A_O_VITALITY:
			mid = PM_ANCIENT_OF_VITALITY;
		break;
		case VN_A_O_CORRUPTION:
			mid = PM_ANCIENT_OF_CORRUPTION;
		break;
		case VN_A_O_BURNING_WASTES:
			mid = PM_ANCIENT_OF_THE_BURNING_WASTES;
		break;
		case VN_A_O_THOUGHT:
			mid = PM_ANCIENT_OF_THOUGHT;
		break;
		case VN_A_O_DEATH:
			mid = PM_ANCIENT_OF_DEATH;
		break;
		case VN_APOCALYPSE:
			mid = PM_APOCALYPSE_ANGEL;
		break;
		case VN_HARROWER:
			mid = PM_HARROWER_OF_ZARIEL;
		break;
		case VN_MAD_ANGEL:
			switch(rn2(6)){
				case 0:
					mid = PM_THRONE_ARCHON;
				break;
				case 1:
					mid = PM_LIGHT_ARCHON;
				break;
				case 2:
					mid = PM_SURYA_DEVA;
				break;
				case 3:
					mid = PM_MAHADEVA;
				break;
				//Note: regardless of which one is the "common" variety each has a chance of being in the vault
				case 4:
					if(dungeon_topology.eprecursor_typ == PRE_DRACAE && rn2(2)){
						mid = PM_DRACAE_ELADRIN;
					} else
						mid = rn2(2) ? PM_GAE_ELADRIN : PM_TULANI_ELADRIN;
				break;
				case 5:
					if(dungeon_topology.eprecursor_typ == PRE_DRACAE && rn2(2)){
						mid = PM_DRACAE_ELADRIN;
					} else 
						mid = !rn2(3) ? PM_BRIGHID_ELADRIN : rn2(2) ? PM_UISCERRE_ELADRIN : PM_CAILLEA_ELADRIN;
				break;
			}
		break;
		case VN_JRT:
			mid = PM_JRT_NETJER;
		break;
		case VN_N_PIT_FIEND:
			mid = PM_NESSIAN_PIT_FIEND;
		break;
		case VN_SHAYATEEN:
			mid = PM_SHAYATEEN;
		break;
		default:
			impossible("Unhandled vault number %d.", levl[x][y].vaulttype);
			mid = PM_LAMB;
		break;
	}
	
	if(mid == PM_ANCIENT_OF_DEATH){
		mon = makemon(&mons[mid], x, y, MM_ADJACENTOK|MM_NOCOUNTBIRTH|NO_MINVENT);
		if(mon){
			otmp = mksobj(SCYTHE, MKOBJ_NOINIT);
			set_material_gm(otmp, GREEN_STEEL);
			add_oprop(otmp, OPROP_UNHYW);
			add_oprop(otmp, OPROP_DRANW);
			add_oprop(otmp, OPROP_VORPW);
			set_obj_size(otmp, MZ_LARGE);
			otmp->spe = 8;
			curse(otmp);
			
			(void) mpickobj(mon, otmp);
			m_dowear(mon, TRUE);
			init_mon_wield_item(mon);
			m_level_up_intrinsic(mon);
		}
	}
	else if(mid == PM_NESSIAN_PIT_FIEND){
		mon = makemon(&mons[mid], x, y, MM_ADJACENTOK|MM_NOCOUNTBIRTH|NO_MINVENT);
#define NPF_ARMOR(typ) 	otmp = mongets(mon, typ, MKOBJ_NOINIT);\
			add_oprop(otmp, OPROP_UNHY);\
			add_oprop(otmp, OPROP_AXIO);\
			set_material_gm(otmp, GREEN_STEEL);\
			otmp->spe = 9;\
			curse(otmp);
		if(mon){
			NPF_ARMOR(SPEED_BOOTS)
			(void)mongets(mon, CLOAK_OF_MAGIC_RESISTANCE, MKOBJ_NOINIT);
			NPF_ARMOR(PLATE_MAIL)
			NPF_ARMOR(GAUNTLETS_OF_POWER)
			NPF_ARMOR(HELMET)
			otmp = mongets(mon, MACE, MKOBJ_NOINIT);
			add_oprop(otmp, OPROP_UNHYW);
			add_oprop(otmp, OPROP_AXIOW);
			set_material_gm(otmp, GREEN_STEEL);
			otmp->spe = 9;
			curse(otmp);
			m_dowear(mon, TRUE);
			init_mon_wield_item(mon);
			m_level_up_intrinsic(mon);
		}
#undef NPF_ARMOR
	}
	else if(mid == PM_SHAYATEEN){
		mon = makemon(&mons[mid], x, y, MM_ADJACENTOK|MM_NOCOUNTBIRTH|NO_MINVENT);
#define SHAY_WEAPON(typ) 	otmp = mongets(mon, typ, MKOBJ_NOINIT);\
			add_oprop(otmp, OPROP_UNHYW);\
			add_oprop(otmp, OPROP_ANARW);\
			add_oprop(otmp, !rn2(4) ? OPROP_DRANW : !rn2(3) ? OPROP_ACIDW : !rn2(2) ? OPROP_FIREW : OPROP_COLDW);\
			otmp->spe = 6;\
			curse(otmp);
		if(mon){
			SHAY_WEAPON(BATTLE_AXE)
			SHAY_WEAPON(BATTLE_AXE)
			SHAY_WEAPON(BATTLE_AXE)
			SHAY_WEAPON(BATTLE_AXE)
			m_dowear(mon, TRUE);
			init_mon_wield_item(mon);
			m_level_up_intrinsic(mon);
		}
#undef NPF_ARMOR
	}
	else {
		mon = makemon(&mons[mid], x, y, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
	}

	if(mon){
		mon->mpeaceful = FALSE;
		set_malign(mon);
		give_hell_vault_trophy(levl[x][y].vaulttype);
		if(levl[x][y].vaulttype == VN_MAD_ANGEL){
			set_template(mon, MAD_TEMPLATE);
			mon->m_lev += (mon->data->mlevel)/2;
		}
		else if(levl[x][y].vaulttype == VN_JRT){
			set_template(mon, POISON_TEMPLATE);
			mon->m_lev = 30;
		}
		mon->mhpmax = max(4, 8*mon->m_lev);
		mon->mhp = mon->mhpmax;
		if(mid == PM_TULANI_ELADRIN || mid == PM_GAE_ELADRIN){
			if(dungeon_topology.eprecursor_typ == PRE_POLYP && rn2(2))
				mon->ispolyp = TRUE;
		}
		mon->mspec_used = 0;
		if(is_ancient(mon)) mon->mvar_ancient_breath_cooldown = 1L; //Don't breathe at the end of this turn.
		switch(levl[x][y].vaulttype){
			case VN_AKKABISH:
			case VN_SARTAN:
			case VN_RAGLAYIM:
			case VN_SHAYATEEN:
			case VN_N_PIT_FIEND:
				/* stay adjacent */
				break;

			case VN_TERAPHIM:
			case VN_MAD_ANGEL:
			case VN_JRT:
				mofflin_close(mon);
				break;

			case VN_NACHASH:
			case VN_KHAAMNUN:
			case VN_A_O_BLESSINGS:
			case VN_A_O_VITALITY:
			case VN_A_O_CORRUPTION:
			case VN_A_O_BURNING_WASTES:
			case VN_A_O_THOUGHT:
			case VN_A_O_DEATH:
			case VN_HARROWER:
			case VN_SHALOSH:
			default:
				mofflin(mon);
				break;
			case VN_APOCALYPSE:
				rloc(mon, TRUE);
				break;
		}
		if(canseemon(mon)){
			pline("%s appears from the magenta fog!", Monnam(mon));
		}
	}

	hell_vault_items(x,y,levl[x][y].vaulttype);
	levl[x][y].seenv = 0;
	levl[x][y].lit = TRUE;
	levl[x][y].horizontal = FALSE;
	levl[x][y].vaulttype = 0;
	levl[x][y].typ = ROOM;
}

int
use_pick_axe(obj)
struct obj *obj;
{
	boolean ispick;
	char dirsyms[12];
	char qbuf[QBUFSZ];
	register char *dsp = dirsyms;
	register int rx, ry;
	int res = MOVE_CANCELLED;
	register const char *sdp, *verb;

	if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

	/* Check tool */
	if (obj != uwep && obj != uarmg) {
	    if (!wield_tool(obj, "swing")) return MOVE_CANCELLED;
	    else res = MOVE_STANDARD;
	}
	if(Straitjacketed){
		//Tool: axe or pickaxe
		You("can't swing a tool while your %s are bound!", makeplural(body_part(ARM)));
		return MOVE_CANCELLED;
	}
	ispick = is_pick(obj);
	verb = ispick ? "dig" : "chop";

	if (u.utrap && u.utraptype == TT_WEB) {
	    pline("%s you can't %s while entangled in a web.",
		  /* res==MOVE_CANCELLED => no prior message;
		     res==MOVE_STANDARD => just got "You now wield a pick-axe." message */
		  res == MOVE_CANCELLED ? "Unfortunately," : "But", verb);
	    return res;
	}
	if (u.utrap && u.utraptype == TT_SALIVA) {
	    pline("%s you can't %s while glued down by saliva.",
		  /* res==MOVE_CANCELLED => no prior message;
		     res==MOVE_STANDARD => just got "You now wield a pick-axe." message */
		  res == MOVE_CANCELLED ? "Unfortunately," : "But", verb);
	    return res;
	}

	while(*sdp) {
		(void) movecmd(*sdp);	/* sets u.dx and u.dy and u.dz */
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		/* Include down even with axe, so we have at least one direction */
		if (u.dz > 0 ||
		    (u.dz == 0 && isok(rx, ry) &&
		     dig_typ(obj, rx, ry) != DIGTYP_UNDIGGABLE))
			*dsp++ = *sdp;
		sdp++;
	}
	*dsp = 0;
	Sprintf(qbuf, "In what direction do you want to %s? [%s]", verb, dirsyms);
	if(!getdir(qbuf))
		return(res);

	if((obj->otyp == HUNTER_S_AXE || obj->otyp == HUNTER_S_LONG_AXE) && !u.dx && !u.dy && !u.dz){
		return use_hunter_axe(obj);
	}
	else if(obj->otyp == CHURCH_PICK && !u.dx && !u.dy && !u.dz){
		return use_church_pick(obj);
	}

	res &= ~MOVE_CANCELLED;
	res |= use_pick_axe2(obj);
	return res; /*Pickaxe might be an attack. It will not cancel*/
}

/* general dig through doors/etc. function
 * Handles pickaxes/lightsabers/axes
 * called from doforce and use_pick_axe
 */
/* MRKR: use_pick_axe() is split in two to allow autodig to bypass */
/*       the "In what direction do you want to dig?" query.        */
/*       use_pick_axe2() uses the existing u.dx, u.dy and u.dz    */

int
use_pick_axe2(obj) 
struct obj *obj;
{
	register int rx, ry;
	register struct rm *lev;
	int dig_target, digtyp;
	boolean ispick = is_pick(obj);
	const char *verbing = ispick ? "digging" :
		(obj->otyp == SEISMIC_HAMMER) ? "smashing" :
		is_lightsaber(obj) ? "cutting" :
		"chopping";
#define PICK_TYP	0
#define SABER_TYP	1
#define AXE_TYP		2
#define HAMMER_TYP	3
	/* 0 = pick, 1 = lightsaber, 2 = axe, 3 = hammer */
	digtyp = ((is_pick(obj)) ? PICK_TYP :
		is_lightsaber(obj) ? SABER_TYP : 
		(obj->otyp == SEISMIC_HAMMER) ? HAMMER_TYP : 
		AXE_TYP);

	if (u.uswallow && attack2(u.ustuck)) {
		;  /* return(1) */
	} else if (Underwater) {
		pline("Turbulence torpedoes your %s attempts.", verbing);
	} else if(u.dz < 0) {
		if(Levitation)
		    if (digtyp == SABER_TYP)
				pline_The("ceiling is too hard to cut through.");
		    else
				You("don't have enough leverage.");
		else
			You_cant("reach the %s.",ceiling(u.ux,u.uy));
	} else if(!u.dx && !u.dy && !u.dz) {
		/* NOTREACHED for lightsabers/axes called from doforce */
		char buf[BUFSZ];
		int dam;

		dam = rnd(2) + dbon(obj, &youmonst) + obj->spe;
		if (dam <= 0) dam = 1;
		You("hit yourself with %s.", yname(obj));
		Sprintf(buf, "%s own %s", uhis(),
				OBJ_NAME(objects[obj->otyp]));
		losehp(dam, buf, KILLED_BY);
		flags.botl=1;
		return MOVE_STANDARD;
	} else if(u.dz == 0) {
		if(Stunned || (Confusion && !rn2(5))) confdir();
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(!isok(rx, ry)) {
			if (digtyp == SABER_TYP) pline("Your %s bounces off harmlessly.",
				aobjnam(obj, (char *)0));
			else if (digtyp == HAMMER_TYP) pline("Clunk!");
			else pline("Clash!");
			return MOVE_STANDARD;
		}
		lev = &levl[rx][ry];
		if(MON_AT(rx, ry) && attack2(m_at(rx, ry)))
			return MOVE_ATTACKED;
		dig_target = dig_typ(obj, rx, ry);
		if (dig_target == DIGTYP_UNDIGGABLE) {
			/* ACCESSIBLE or POOL */
			struct trap *trap = t_at(rx, ry);

			if (trap && trap->ttyp == WEB) {
			    if (!trap->tseen) {
					seetrap(trap);
					There("is a spider web there!");
			    }
				if(digtyp == SABER_TYP){
					Your("%s through in the web.",
					aobjnam(obj, "burn"));
					if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
						deltrap(trap);
						newsym(rx, ry);
					}
				} else {
					Your("%s entangled in the web.",
					aobjnam(obj, "become"));
					/* you ought to be able to let go; tough luck */
					/* (maybe `move_into_trap()' would be better) */
					nomul(-d(2,2), "entangled in a web");
					nomovemsg = "You pull free.";
				}
			} else if (lev->typ == IRONBARS) {
			    pline("Clang!");
			    wake_nearby_noisy();
			} else if (IS_TREES(lev->typ))
			    You("need an axe to cut down a tree.");
			else if (IS_ROCK(lev->typ))
			    You("need a pick to dig rock.");
			else if (!ispick && (sobj_at(STATUE, rx, ry)
					     || sobj_at(BOULDER, rx, ry)
					     || sobj_at(MASS_OF_STUFF, rx, ry)
			)) {
			    boolean vibrate = !rn2(3);
			    pline("Sparks fly as you whack the %s.%s",
				sobj_at(STATUE, rx, ry) ? "statue" : sobj_at(MASS_OF_STUFF, rx, ry) ? "mass of stuff" : "boulder",
				vibrate ? " The axe-handle vibrates violently!" : "");
			    if (vibrate) losehp(2, "axing a hard object", KILLED_BY);
			}
			else
			    You("swing your %s through thin air.",
				aobjnam(obj, (char *)0));
		} else {
			static const char * const d_action[10][4] = {
			    {"swinging",			"slicing the air",			"swinging",					"swinging"},
			    {"digging",				"cutting through the wall",	"cutting",					"digging"},
			    {"chipping the statue",	"cutting the statue",		"chipping the statue",		"smashing the statue"},
			    {"hitting the boulder",	"cutting through the boulder","hitting the boulder",	"smashing the boulder"},
			    {"chipping the crate",	"cutting through the crate","chipping the crate",		"smashing the crate"},
			    {"chipping the mass",	"cutting through the mass", "chipping the mass",			"smashing the mass"},
			    {"hitting the door",	"burning through the door",	"chopping at the door",		"smashing the door"},
			    {"hitting the tree",	"razing the tree",			"cutting down the tree",	"smashing the tree"},
			    {"hitting the bars",	"cutting through the bars",	"chopping the bars",		"smashing the bars"},
			    {"hitting the drawbridge","cutting through the drawbridge chains","chopping at the drawbridge","smashing the drawbridge"}
			};
			did_dig_msg = FALSE;
			digging.quiet = FALSE;
			if (digging.pos.x != rx || digging.pos.y != ry ||
			    !on_level(&digging.level, &u.uz) || digging.down) {
			    if (flags.autodig &&
				dig_target == DIGTYP_ROCK && !digging.down &&
				digging.pos.x == u.ux &&
				digging.pos.y == u.uy &&
				(moves <= digging.lastdigtime+2 &&
				 moves >= digging.lastdigtime)) {
				/* avoid messages if repeated autodigging */
				did_dig_msg = TRUE;
				digging.quiet = TRUE;
			    }
			    digging.down = digging.chew = FALSE;
			    digging.warned = FALSE;
			    digging.pos.x = rx;
			    digging.pos.y = ry;
			    assign_level(&digging.level, &u.uz);
			    digging.effort = 0;
			    if (!digging.quiet)
				You("start %s.", d_action[dig_target][digtyp]);
			} else {
			    You("%s %s.", digging.chew ? "begin" : "continue",
					d_action[dig_target][digtyp]);
			    digging.chew = FALSE;
			}
			set_occupation(dig, verbing, 0);
		}
	} else if (Weightless || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
		You("swing your %s through thin air.", aobjnam(obj, (char *)0));
	} else if (!can_reach_floor()) {
		You_cant("reach the %s.", surface(u.ux,u.uy));
	} else if (is_pool(u.ux, u.uy, FALSE) || is_lava(u.ux, u.uy)) {
		/* Monsters which swim also happen not to be able to dig */
		You("cannot stay under%s long enough.",
				is_pool(u.ux, u.uy, TRUE) ? "water" : " the lava");
	} else if (IS_PUDDLE(levl[u.ux][u.uy].typ)) {
		Your("%s against the water's surface.", aobjnam(obj, "splash"));
		wake_nearby();
	} else if (digtyp == AXE_TYP) {
		Your("%s merely scratches the %s.",
				aobjnam(obj, (char *)0), surface(u.ux,u.uy));
		u_wipe_engr(3);
	} else {
		if (digging.pos.x != u.ux || digging.pos.y != u.uy ||
			!on_level(&digging.level, &u.uz) || !digging.down) {
		    digging.chew = FALSE;
		    digging.down = TRUE;
		    digging.warned = FALSE;
		    digging.pos.x = u.ux;
		    digging.pos.y = u.uy;
		    assign_level(&digging.level, &u.uz);
		    digging.effort = 0;
		    You("start %s downward.", verbing);
		    if (*u.ushops) shopdig(0);
		} else
		    You("continue %s downward.", verbing);
		did_dig_msg = FALSE;
		set_occupation(dig, verbing, 0);
	}
	return MOVE_STANDARD;
}

/*
 * Town Watchmen frown on damage to the town walls, trees or fountains.
 * It's OK to dig holes in the ground, however.
 * If mtmp is assumed to be a watchman, a watchman is found if mtmp == 0
 * zap == TRUE if wand/spell of digging, FALSE otherwise (chewing)
 */
void
watch_dig(mtmp, x, y, zap)
    struct monst *mtmp;
    xchar x, y;
    boolean zap;
{
	struct rm *lev = &levl[x][y];

	if (in_town(x, y) &&
	    (closed_door(x, y) || lev->typ == SDOOR ||
	     IS_WALL(lev->typ) || IS_FOUNTAIN(lev->typ) || IS_FORGE(lev->typ) || IS_TREE(lev->typ))) {
	    if (!mtmp) {
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if ((mtmp->mtyp == PM_WATCHMAN ||
			 mtmp->mtyp == PM_WATCH_CAPTAIN) &&
			!is_blind(mtmp) && m_canseeu(mtmp) &&
			couldsee(mtmp->mx, mtmp->my) && mtmp->mpeaceful)
			break;
		}
	    }

	    if (mtmp) {
		if(zap || digging.warned) {
		    verbalize("Halt, vandal!  You're under arrest!");
		    (void) angry_guards(!(flags.soundok));
		} else {
		    const char *str;

		    if (IS_DOOR(lev->typ))
			str = "door";
		    else if (IS_TREE(lev->typ))
			str = "tree";
		    else if (IS_ROCK(lev->typ))
			str = "wall";
		    else
			str = "fountain";
		    verbalize("Hey, stop damaging that %s!", str);
		    digging.warned = TRUE;
		}
		if (is_digging())
		    stop_occupation();
	    }
	}
}

#endif /* OVLB */
#ifdef OVL0

/* Return TRUE if monster died, FALSE otherwise.  Called from m_move(). */
boolean
mdig_tunnel(mtmp)
register struct monst *mtmp;
{
	register struct rm *here;
	struct obj *otmp;

	here = &levl[mtmp->mx][mtmp->my];
	if (here->typ == SDOOR)
	    cvt_sdoor_to_door(here);	/* ->typ = DOOR */

	/* Eats away door if present & closed or locked */
	if (closed_door(mtmp->mx, mtmp->my)) {
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
			add_damage(mtmp->mx, mtmp->my, 0L);
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */
	    if (here->doormask & D_TRAPPED) {
			here->doormask = D_NODOOR;
			if (mb_trapped(mtmp)) {	/* mtmp is killed */
				newsym(mtmp->mx, mtmp->my);
				return TRUE;
			}
	    } else {
			if (!rn2(3) && flags.verbose)	/* not too often.. */
				You_feel("an unexpected draft.");
			here->doormask = D_BROKEN;
	    }
	    newsym(mtmp->mx, mtmp->my);
	    return FALSE;
	} else if (!IS_ROCK(here->typ) && !IS_TREE(here->typ) ) /* no dig */
	    return FALSE;

	/* Only rock, trees, and walls fall through to this point. */
	if ((here->wall_info & W_NONDIGGABLE) != 0) {
	    impossible("mdig_tunnel:  %s at (%d,%d) is undiggable",
		       (IS_WALL(here->typ) ? "wall" : "stone"),
		       (int) mtmp->mx, (int) mtmp->my);
	    return FALSE;	/* still alive */
	}

	if (IS_WALL(here->typ)) {
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
	    if (flags.soundok && flags.verbose && !rn2(5))
		You_hear("crashing rock.");
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    if (level.flags.is_maze_lev) {
		here->typ = ROOM;
	    } else if (level.flags.is_cavernous_lev &&
		       !in_town(mtmp->mx, mtmp->my)) {
		here->typ = CORR;
	    } else {
		here->typ = DOOR;
		here->doormask = D_NODOOR;
	    }
		if(mtmp->mtyp == PM_MANY_TALONED_THING){
			struct engr *oep = engr_at(mtmp->mx, mtmp->my);
			if(!oep){
				make_engr_at(mtmp->mx, mtmp->my,
				 "", 0L, DUST);
				oep = engr_at(mtmp->mx, mtmp->my);
			}
			if(oep && (oep->ward_type == DUST || oep->ward_type == ENGR_BLOOD || oep->ward_type == ENGRAVE || oep->ward_type == MARK)){
				oep->ward_id = CLAW_MARKS;
				oep->halu_ward = 1;
				oep->ward_type = ENGRAVE;
				oep->complete_wards = 1;
			}
		}
		if(Is_belial_level(&u.uz)){
			otmp = mksobj_at(DROVEN_DAGGER, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
			set_material_gm(otmp, OBSIDIAN_MT);
		} else {
			otmp = mksobj_at(ROCK, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
		}
		set_obj_quan(otmp, rn1(20, 20));
	} else if (IS_TREE(here->typ)) {
		int numsticks;
	    here->typ = ROOM;
		for(numsticks = d(2,4)-1; numsticks > 0; numsticks--){
			otmp = mksobj_at(rn2(2) ? QUARTERSTAFF : CLUB, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
			set_material_gm(otmp, WOOD);
			otmp->spe = 0;
			otmp->cursed = otmp->blessed = FALSE;
		}
	} else {
	    here->typ = CORR;
	    if (!Is_belial_level(&u.uz) && !rn2(20)) mksobj_at(BOULDER, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
		else {
			if(Is_belial_level(&u.uz)){
				otmp = mksobj_at(DROVEN_DAGGER, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
				set_material_gm(otmp, OBSIDIAN_MT);
			} else {
				otmp = mksobj_at(ROCK, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
			}
			set_obj_quan(otmp, rn1(20, 20));
		}
	}
	newsym(mtmp->mx, mtmp->my);
	if (!sobj_at(BOULDER, mtmp->mx, mtmp->my))
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */

	return FALSE;
}

/* fill in corridors around previous position, create corridors out of rock around current position */
void
mworldshape(mtmp, oldx, oldy)
struct monst * mtmp;
int oldx;
int oldy;
{
	int tx, ty;
	/* fill in around prev position */
	for (tx = max(0, oldx-1); tx < min(COLNO, oldx+1); tx++)
	for (ty = max(0, oldy-1); ty < min(ROWNO, oldy+1); ty++)
		if (levl[tx][ty].typ == CORR) {
			levl[tx][ty].typ = STONE;
			block_point(tx, ty);
		}
	/* dig out around new position */
	for (tx = max(0, mtmp->mx-1); tx < min(COLNO, mtmp->mx+1); tx++)
	for (ty = max(0, mtmp->my-1); ty < min(ROWNO, mtmp->my+1); ty++)
		if (levl[tx][ty].typ == STONE && may_dig(tx, ty)) {
			levl[tx][ty].typ = CORR;
			unblock_point(tx, ty);
		}
}

#endif /* OVL0 */
#ifdef OVL3

/* digging via wand zap or spell cast */
/* parameters < 0 indicate "use defaults" */
void
zap_dig(zx, zy, digdepth)
register int zx, zy, digdepth;
{
	struct rm *room;
	struct monst *mtmp;
	struct obj *otmp;
	boolean shopdoor, shopwall, maze_dig, cavesafe = FALSE;
	/*
	 * Original effect (approximately):
	 * from CORR: dig until we pierce a wall
	 * from ROOM: pierce wall and dig until we reach
	 * an ACCESSIBLE place.
	 * Currently: dig for digdepth positions;
	 * also down on request of Lennart Augustsson.
	 */

	if (u.uswallow) {
	    mtmp = u.ustuck;

	    if (!is_whirly(mtmp->data)) {
		if (is_animal(mtmp->data))
		    You("pierce %s %s wall!",
			s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
		if(mtmp->mtyp == PM_JUIBLEX 
			|| mtmp->mtyp == PM_LEVIATHAN
			|| mtmp->mtyp == PM_METROID_QUEEN
												) mtmp->mhp = (int)(.75*mtmp->mhp + 1); 
		else mtmp->mhp = 1;		/* almost dead */
		expels(mtmp, mtmp->data, !is_animal(mtmp->data));
	    }
	    return;
	} /* swallowed */

	if (u.dz) {
	    if (!Weightless && !Is_waterlevel(&u.uz) && !Underwater) {
		if (u.dz < 0 || On_stairs(u.ux, u.uy)) {
		    if (On_stairs(u.ux, u.uy))
			pline_The("beam bounces off the %s and hits the %s.",
			      (u.ux == xdnladder || u.ux == xupladder) ?
			      "ladder" : "stairs", ceiling(u.ux, u.uy));
		    You("loosen a rock from the %s.", ceiling(u.ux, u.uy));
		    pline("It falls on your %s!", body_part(HEAD));
		    losehp(rnd((uarmh && is_hard(uarmh)) ? 2 : 6),
			   "falling rock", KILLED_BY_AN);
		    otmp = mksobj_at(ROCK, u.ux, u.uy, MKOBJ_NOINIT);
		    if (otmp) {
			(void)xname(otmp);	/* set dknown, maybe bknown */
			stackobj(otmp);
		    }
		    newsym(u.ux, u.uy);
		} else {
		    watch_dig((struct monst *)0, u.ux, u.uy, TRUE);
		    (void) dighole(FALSE);
		}
	    }
	    return;
	} /* up or down */

	/* normal case: digging across the level */
	shopdoor = shopwall = FALSE;
	maze_dig = level.flags.is_maze_lev && !Is_earthlevel(&u.uz);
	if(zx<0) zx = u.ux + u.dx;
	if(zy<0) zy = u.uy + u.dy;
	if(digdepth<=0) digdepth = rn1(18, 8);
	tmp_at(DISP_BEAM, cmap_to_glyph(S_digbeam));
	while (--digdepth >= 0) {
	    if (!isok(zx,zy)) break;
	    room = &levl[zx][zy];
	    tmp_at(zx,zy);
	    delay_output();	/* wait a little bit */
		if(closed_door(zx, zy) && !(levl[zx][zy].doormask&D_LOCKED) && u.sealsActive&SEAL_OTIAX) unbind(SEAL_OTIAX, TRUE);
	    if (closed_door(zx, zy) || room->typ == SDOOR) {
			/* ALI - Artifact doors from slash'em*/
			if (artifact_door(zx, zy)) {
				if (cansee(zx, zy))
				pline_The("door glows then fades.");
				break;
			}
			if (*in_rooms(zx,zy,SHOPBASE)) {
				add_damage(zx, zy, 400L);
				shopdoor = TRUE;
			}
			if (room->typ == SDOOR)
				room->typ = DOOR;
			else if (cansee(zx, zy))
				pline_The("door is razed!");
			watch_dig((struct monst *)0, zx, zy, TRUE);
			room->doormask = D_NODOOR;
			unblock_point(zx,zy); /* vision */
			digdepth -= 2;
			if (maze_dig) break;
	    } else if (maze_dig) {
			if (IS_WALL(room->typ)) {
				if (!(room->wall_info & W_NONDIGGABLE)) {
				if (*in_rooms(zx,zy,SHOPBASE)) {
					add_damage(zx, zy, 200L);
					shopwall = TRUE;
				}
				room->typ = ROOM;
				unblock_point(zx,zy); /* vision */
				} else if (!Blind)
				pline_The("wall glows then fades.");
				break;
			} else if (IS_TREE(room->typ)) { /* check trees before stone */
				if (!(room->wall_info & W_NONDIGGABLE)) {
					room->typ = ROOM;
					unblock_point(zx,zy); /* vision */
				} else if (!Blind)
					pline_The("tree shudders but is unharmed.");
				break;
			} else if (room->typ == STONE || room->typ == SCORR) {
				if (!(room->wall_info & W_NONDIGGABLE)) {
					room->typ = CORR;
					unblock_point(zx,zy); /* vision */
				} else if (!Blind)
					pline_The("rock glows then fades.");
				break;
			}
	    } else if (IS_ROCK(room->typ)) {
			if (!may_dig(zx,zy)) break;
			if (IS_WALL(room->typ) || room->typ == SDOOR) {
				if(IS_WALL(room->typ) && u.sealsActive&SEAL_ANDREALPHUS) unbind(SEAL_ANDREALPHUS,TRUE);
				if (*in_rooms(zx,zy,SHOPBASE)) {
				add_damage(zx, zy, 200L);
				shopwall = TRUE;
				}
				watch_dig((struct monst *)0, zx, zy, TRUE);
				if (level.flags.is_cavernous_lev && !in_town(zx, zy)) {
				room->typ = CORR;
				} else {
				room->typ = DOOR;
				room->doormask = D_NODOOR;
				}
				digdepth -= 2;
			} else if (IS_TREE(room->typ)) {
				room->typ = ROOM;
				digdepth -= 2;
				if(!flags.mon_moving && u.sealsActive&SEAL_EDEN) unbind(SEAL_EDEN,TRUE);
			} else {	/* IS_ROCK but not IS_WALL or SDOOR */
				room->typ = CORR;
				digdepth--;
				if(Is_earthlevel(&u.uz) && !cavesafe) {
					if(!rn2(3)) {
						mkcavearea(FALSE);
						digdepth=0;
					} else if(rn2(2)) {
						mkcavearea(TRUE);
						digdepth=0;
					} else cavesafe = TRUE;
				}
			}
			unblock_point(zx,zy); /* vision */
	    }
	    zx += u.dx;
	    zy += u.dy;
	} /* while */
	tmp_at(DISP_END,0);	/* closing call */
	if (shopdoor || shopwall)
	    pay_for_damage(shopdoor ? "destroy" : "dig into", FALSE);
	return;
}

/* move objects from fobj/nexthere lists to buriedobjlist, keeping position */
/* information */
struct obj *
bury_an_obj(otmp)
	struct obj *otmp;
{
	struct obj *otmp2;
	boolean under_ice;

#ifdef DEBUG
	pline("bury_an_obj: %s", xname(otmp));
#endif
	if (otmp == uball)
		unpunish();
	/* after unpunish(), or might get deallocated chain */
	otmp2 = otmp->nexthere;
	/*
	 * obj_resists(,0,0) prevents Rider corpses from being buried.
	 * It also prevents The Amulet and invocation tools from being
	 * buried.  Since they can't be confined to bags and statues,
	 * it makes sense that they can't be buried either, even though
	 * the real reason there (direct accessibility when carried) is
	 * completely different.
	 */
	if (otmp == uchain || obj_resists(otmp, 0, 0))
		return(otmp2);

	if (otmp->otyp == LEASH && otmp->leashmon != 0)
		o_unleash(otmp);

	if (obj_is_burning(otmp) && otmp->otyp != POT_OIL)
		end_burn(otmp, TRUE);

	obj_extract_self(otmp);

	under_ice = is_ice(otmp->ox, otmp->oy);
	if (otmp->otyp == ROCK && !under_ice) {
		/* merges into burying material */
		obfree(otmp, (struct obj *)0);
		return(otmp2);
	}
	/*
	 * Start a rot on organic material.  Not corpses -- they
	 * are already handled.
	 */
	if (otmp->otyp == CORPSE) {
	    ;		/* should cancel timer if under_ice */
	} else if ((under_ice ? otmp->oclass == POTION_CLASS : is_organic(otmp))
		&& !obj_resists(otmp, 0, 100)) {
	    (void) start_timer((under_ice ? 0L : 250L) + (long)rnd(250),
			       TIMER_OBJECT, ROT_ORGANIC, (genericptr_t)otmp);
	} else if(otmp->otyp == HOLY_SYMBOL_OF_THE_BLACK_MOTHE && !Infuture){
	    (void) start_timer(250L + (long)rnd(250), TIMER_OBJECT, ROT_ORGANIC, (genericptr_t)otmp);
	}
	add_to_buried(otmp);
	return(otmp2);
}

void
bury_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	if(level.objects[x][y] != (struct obj *)0)
		pline("bury_objs: at %d, %d", x, y);
#endif
	for (otmp = level.objects[x][y]; otmp; otmp = otmp2)
		otmp2 = bury_an_obj(otmp);

	/* don't expect any engravings here, but just in case */
	del_engr_ward_at(x, y);
	newsym(x, y);
}

/* move objects from buriedobjlist to fobj/nexthere lists */
void
unearth_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	pline("unearth_objs: at %d, %d", x, y);
#endif
	for (otmp = level.buriedobjlist; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (otmp->ox == x && otmp->oy == y) {
		    obj_extract_self(otmp);
		    if (otmp->timed)
			(void) stop_timer(ROT_ORGANIC, otmp->timed);
		    place_object(otmp, x, y);
		    stackobj(otmp);
		}
	}
	del_engr_ward_at(x, y);
	newsym(x, y);
}

/*
 * The organic material has rotted away while buried.  As an expansion,
 * we could add add partial damage.  A damage count is kept in the object
 * and every time we are called we increment the count and reschedule another
 * timeout.  Eventually the object rots away.
 *
 * This is used by buried objects other than corpses.  When a container rots
 * away, any contents become newly buried objects.
 */
/* ARGSUSED */
void
rot_organic(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
	struct obj *obj = (struct obj *) arg;
	
	while (Has_contents(obj)) {
	    /* We don't need to place contained object on the floor
	       first, but we do need to update its map coordinates. */
	    obj->cobj->ox = obj->ox,  obj->cobj->oy = obj->oy;
	    /* Everything which can be held in a container can also be
	       buried, so bury_an_obj's use of obj_extract_self insures
	       that Has_contents(obj) will eventually become false. */
	    (void)bury_an_obj(obj->cobj);
	}
	if(obj->otyp == HOLY_SYMBOL_OF_THE_BLACK_MOTHE){
		struct engr *oep = engr_at(obj->ox, obj->oy);
		if(!oep){
			make_engr_at(obj->ox, obj->oy,
			 "", 0L, DUST);
			oep = engr_at(obj->ox, obj->oy);
		}
		if(oep){
			oep->ward_id = HOOFPRINT;
			oep->halu_ward = 1;
			oep->ward_type = BURN;
			oep->complete_wards = 1;
		}
		if(cansee(obj->ox, obj->oy))
			pline_The("%s boils to black mist!", surface(obj->ox, obj->oy));
		makemon(&mons[PM_MOUTH_OF_THE_GOAT], obj->ox, obj->oy, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
		/* Spreading the Goat's influence is worth credit. Intentionally not affected by the usual diminishing returns on credit gain. */
		if (u.shubbie_atten) {
			u.shubbie_credit += 30;
			u.shubbie_devotion += 30;
		}
	}
	if(obj->otyp == BRAINROOT){
		struct monst *mon = makemon(&mons[PM_BRAINBLOSSOM_PATCH], obj->ox, obj->oy, MM_NOCOUNTBIRTH);
		if(mon && canspotmon(mon)){
			pline("%s grows back from the roots!", Monnam(mon));
		}
	}
	obj_extract_self(obj);
	obfree(obj, (struct obj *) 0);
}

/*
 * Called when a corpse has rotted completely away.
 */
void
rot_corpse(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
	xchar x = 0, y = 0;
	struct obj *obj = (struct obj *) arg;
	boolean on_floor = obj->where == OBJ_FLOOR,
		in_invent = obj->where == OBJ_INVENT;

	get_obj_location(obj, &x, &y, 0);

	if (in_invent) {
	    if (flags.verbose) {
		char *cname = corpse_xname(obj, FALSE);
		Your("%s%s %s away%c",
		     obj == uwep ? "wielded " : nul, cname,
		     otense(obj, "rot"), obj == uwep ? '!' : '.');
	    }
	    if (obj == uwep) {
			uwepgone();	/* now bare handed */
			stop_occupation();
	    } else if (obj == uswapwep) {
			uswapwepgone();
			stop_occupation();
	    } else if (obj == uquiver) {
			uqwepgone();
			stop_occupation();
	    }
	} else if (obj->where == OBJ_MINVENT && obj->owornmask) {
	    if (obj == MON_WEP(obj->ocarry)) {
			setmnotwielded(obj->ocarry,obj);
			MON_NOWEP(obj->ocarry);
	    }
		if (obj == MON_SWEP(obj->ocarry)) {
			setmnotwielded(obj->ocarry,obj);
			MON_NOSWEP(obj->ocarry);
	    }
	}
	if(on_floor){
		if(u.ublood_smithing && has_blood(&mons[obj->corpsenm])){
			if(obj->corpsenm == PM_INDEX_WOLF){
				//Guaranteed chunk
				struct obj *otmp = mksobj_at(CRYSTAL, x, y, MKOBJ_NOINIT);
				if(otmp){
					set_material_gm(otmp, HEMARGYOS);
					otmp->spe = 3;
				}
			}
			else if(obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS){
				//Guaranteed mass
				struct obj *otmp = mksobj_at(CRYSTAL, x, y, MKOBJ_NOINIT);
				if(otmp){
					set_material_gm(otmp, HEMARGYOS);
					otmp->spe = 4;
				}
			}
			else {
				int out_of = 20;
				if(active_glyph(EYE_THOUGHT) && active_glyph(LUMEN))
					out_of = 7;
				else if(active_glyph(EYE_THOUGHT))
					out_of = 10;
				else if(active_glyph(LUMEN))
					out_of = 13;
				if(!rn2(out_of)){
					struct obj *otmp = mksobj_at(CRYSTAL, x, y, MKOBJ_NOINIT);
					if(otmp){
						set_material_gm(otmp, HEMARGYOS);
						if(monstr[obj->corpsenm] >= 20)
							otmp->spe = 4;
						else if(monstr[obj->corpsenm] >= 14)
							otmp->spe = 3;
						else if(monstr[obj->corpsenm] >= 5)
							otmp->spe = 2;
						else
							otmp->spe = 1;
					}
				}
			}
		}
		if((active_glyph(LUMEN) || (!u.veil && u.ualign.god == GOD_THE_CHOIR)) && !obj->researched && !mindless(&mons[obj->corpsenm]) && !is_animal(&mons[obj->corpsenm])){
			int out_of = 100;
			if(active_glyph(EYE_THOUGHT) && active_glyph(LUMEN))
				out_of = 33;
			else if(active_glyph(EYE_THOUGHT))
				out_of = 50;
			else if(active_glyph(LUMEN))
				out_of = 66;
			if(!rn2(out_of)){
				mksobj_at(PARASITE, x, y, NO_MKOBJ_FLAGS);
			}
		}
		if(check_preservation(PRESERVE_ROT_TRIGGER)){
			int out_of = check_preservation(PRESERVE_MAX) ? 300 : check_preservation(PRESERVE_GAIN_DR_2) ? 600 : 900;
			if(check_rot(ROT_KIN))
				out_of /= 2;
			out_of -= u.uimpurity;
			out_of -= u.uimp_rot;
			if(active_glyph(EYE_THOUGHT) && active_glyph(DEFILEMENT))
				out_of /= 3;
			else if(active_glyph(EYE_THOUGHT))
				out_of /= 2;
			else if(active_glyph(DEFILEMENT))
				out_of = 2*out_of/3;

			if(active_glyph(LUMEN) && active_glyph(ROTTEN_EYES))
				out_of /= 2;
			else if(active_glyph(LUMEN) || active_glyph(ROTTEN_EYES))
				out_of = 3*out_of/4;

			if(!obj->researched)
				out_of /= 2;
			if(!obj->odrained)
				out_of /= 3;
			if(out_of < 2 || !rn2(out_of)){
				struct obj *crys = mksobj(CRYSTAL, NO_MKOBJ_FLAGS);
				if(crys){
					set_material_gm(crys, FLESH);
					crys->spe = rn2(7);
					place_object(crys, x, y);
				}
			}
			if(u.silvergrubs && !rn2(20)){
				set_silvergrubs(FALSE);
			}
			if(check_rot(ROT_KIN) && !mindless(&mons[obj->corpsenm]) && !is_animal(&mons[obj->corpsenm]) && (u.silvergrubs || !rn2(100)) && !(mvitals[PM_SILVERGRUB].mvflags&G_GONE && !In_quest(&u.uz))){
				set_silvergrubs(TRUE);
				makemon(&mons[PM_SILVERGRUB], x, y, NO_MM_FLAGS);
			}
		}
	}

	if(x && couldsee(x, y) && distmin(x, y, u.ux, u.uy) <= BOLT_LIM/2){
		IMPURITY_UP(u.uimp_rot)
	}
	// rot_organic(arg, timeout); //This is not for corpses, it is for buried containers.
	obj_extract_self(obj);
	obfree(obj, (struct obj *) 0);
	
	if (on_floor) newsym(x, y);
	else if (in_invent) update_inventory();
}

/* return TRUE if digging succeeded, FALSE otherwise */
//digs a hole at the specified x and y.
boolean
digfarhole(pit_only, x, y, yours)
boolean pit_only;
int x;
int y;
boolean yours;
{
	struct trap *ttmp = t_at(x, y);
	struct rm *lev = &levl[x][y];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

    if (!isok(x,y)) return FALSE; // if out of bounds or whatever.
	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) ||
	   /* ALI - artifact doors from slash'em */
	   (IS_DOOR(levl[x][y].typ) && artifact_door(x, y)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
			if(cansee(x, y))
				pline_The("dust over the %s swirls in the wind.", surface(x,y));

	} else if (is_pool(x, y, TRUE) || is_lava(x, y)) {
		pline_The("%s sloshes furiously for a moment, then subsides.",
			is_lava(x, y) ? "lava" : "water");
		wake_nearby();	/* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(x, y) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (pit_only) {
			if(cansee(x, y))
			    pline_The("dust over the drawbridge swirls in the wind.");
		    return FALSE;
		} else {
		    int bx = x, by = y;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&bx, &by);
		    destroy_drawbridge(bx, by);
		    return TRUE;
		}

	} else if ((boulder_here = boulder_at(x, y)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
		    rn2(2)) {
			pline_The("%s settles into the pit.", xname(boulder_here));
			ttmp->ttyp = PIT;	 /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
			pline("KADOOM! The %s falls in!", xname(boulder_here));
			(void) delfloortrap(ttmp);
		}
		delobj(boulder_here);
		return TRUE;

	} else if (IS_GRAVE(lev->typ)) {
	    digactualhole(x, y, yours ? BY_YOU : (struct monst *)0, PIT, FALSE, TRUE);
	    dig_up_grave(x,y);
	    return TRUE;
	} else if (IS_SEAL(lev->typ)) {
	    // digactualhole(x, y, yours ? BY_YOU : (struct monst *)0, PIT, FALSE, TRUE);
	    break_seal(x,y);
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(x,y);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
			if(cansee(x, y))
				pline_The("dust over the %s swirls in the wind.", surface(x,y));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 far_liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(x, y);

			if(cansee(x, y))
				pline_The("hole fills with %s!",
			      typ == LAVAPOOL ? "lava" : "water");
		if (!Levitation && !Flying && x==u.ux && y==u.uy) {
		    if (typ == LAVAPOOL)
			(void) lava_effects(TRUE);
		    else if (!Wwalking)
			(void) drown();
		}
		return TRUE;

	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
		if(cansee(x, y))
			pline_The("dust over the throne swirls in the wind.");
	} else if (IS_ALTAR(lev->typ)) {
		if(cansee(x, y))
			pline_The("dust over the altar swirls in the wind.");
	} else {
		typ = fillholetyp(x,y);

		if (typ != ROOM) {
			lev->typ = typ;
			goto far_liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(x, y, yours ? BY_YOU : (struct monst *)0, PIT, FALSE, TRUE);
		else
			digactualhole(x, y, yours ? BY_YOU : (struct monst *)0, HOLE, FALSE, TRUE);

		return TRUE;
	}

	return FALSE;
}

#if 0
void
bury_monst(mtmp)
struct monst *mtmp;
{
#ifdef DEBUG
	pline("bury_monst: %s", mon_nam(mtmp));
#endif
	if(canseemon(mtmp)) {
	    if(mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION)) {
		pline_The("%s opens up, but %s is not swallowed!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
		return;
	    } else
	        pline_The("%s opens up and swallows %s!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
	}

	mtmp->mburied = TRUE;
	wakeup(mtmp, TRUE);			/* at least give it a chance :-) */
	newsym(mtmp->mx, mtmp->my);
}

void
bury_you()
{
#ifdef DEBUG
	pline("bury_you");
#endif
    if (!Levitation && !Flying) {
	if(u.uswallow)
	    You_feel("a sensation like falling into a trap!");
	else
	    pline_The("%s opens beneath you and you fall in!",
		  surface(u.ux, u.uy));

	u.uburied = TRUE;
	if(!Strangled && !Breathless) Strangled = 6;
	under_ground(1);
    }
}

void
unearth_you()
{
#ifdef DEBUG
	pline("unearth_you");
#endif
	u.uburied = FALSE;
	under_ground(0);
	if(!uamul || uamul->otyp != AMULET_OF_STRANGULATION)
		Strangled = 0;
	vision_recalc(0);
}

void
escape_tomb()
{
#ifdef DEBUG
	pline("escape_tomb");
#endif
	if ((Teleportation || mon_resistance(&youmonst,TELEPORT)) &&
	    (Teleport_control || rn2(3) < Luck+2)) {
		You("attempt a teleport spell.");
		(void) dotele();	/* calls unearth_you() */
	} else if(u.uburied) { /* still buried after 'port attempt */
		boolean good;

		if(amorphous(youracedata) || Passes_walls ||
		   noncorporeal(youracedata) || unsolid(youracedata) ||
		   (tunnels(youracedata) && !needspick(youracedata))) {

		    You("%s up through the %s.",
			(tunnels(youracedata) && !needspick(youracedata)) ?
			 "try to tunnel" : (amorphous(youracedata)) ?
			 "ooze" : "phase", surface(u.ux, u.uy));

		    if(tunnels(youracedata) && !needspick(youracedata))
			good = dighole(TRUE);
		    else good = TRUE;
		    if(good) unearth_you();
		}
	}
}

void
bury_obj(otmp)
struct obj *otmp;
{

#ifdef DEBUG
	pline("bury_obj");
#endif
	if(cansee(otmp->ox, otmp->oy))
	   pline_The("objects on the %s tumble into a hole!",
		surface(otmp->ox, otmp->oy));

	bury_objs(otmp->ox, otmp->oy);
}
#endif

#ifdef DEBUG
int
wiz_debug_cmd() /* in this case, bury everything at your loc and around */
{
	int x, y;

	for (x = u.ux - 1; x <= u.ux + 1; x++)
	    for (y = u.uy - 1; y <= u.uy + 1; y++)
		if (isok(x,y)) bury_objs(x,y);
	return 0;
}

#endif /* DEBUG */
#endif /* OVL3 */

/*dig.c*/
