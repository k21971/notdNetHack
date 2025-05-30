/*	SCCS Id: @(#)teleport.c	3.4	2003/08/11	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL boolean FDECL(tele_jump_ok, (int,int,int,int));
STATIC_DCL void NDECL(vault_tele);
STATIC_DCL boolean FDECL(rloc_pos_ok, (int,int,struct monst *));
STATIC_DCL void FDECL(mvault_tele, (struct monst *));

/*
 * Is (x,y) a good position of mtmp?  If mtmp is NULL, then is (x,y) good
 * for an object?
 *
 * This function will only look at mtmp->mdat, so makemon, mplayer, etc can
 * call it to generate new monster positions with fake monster structures.
 */
boolean
goodpos(x, y, mtmp, gpflags)
int x,y;
struct monst *mtmp;
unsigned gpflags;
{
	struct permonst *mdat = NULL;
	boolean ignorewater = ((gpflags & MM_IGNOREWATER) != 0);

	if (!isok(x, y)) return FALSE;

	/* in many cases, we're trying to create a new monster, which
	 * can't go on top of the player or any existing monster.
	 * however, occasionally we are relocating engravings or objects,
	 * which could be co-located and thus get restricted a bit too much.
	 * oh well.
	 */
	if (mtmp != &youmonst && x == u.ux && y == u.uy
#ifdef STEED
			&& (!u.usteed || mtmp != u.usteed)
#endif
			)
		return FALSE;

	if (mtmp) {
	    struct monst *mtmp2 = m_at(x,y);
		struct trap * ttmp = t_at(x, y);

	    /* Be careful with long worms.  A monster may be placed back in
	     * its own location.  Normally, if m_at() returns the same monster
	     * that we're trying to place, the monster is being placed in its
	     * own location.  However, that is not correct for worm segments,
	     * because all the segments of the worm return the same m_at().
	     * Actually we overdo the check a little bit--a worm can't be placed
	     * in its own location, period.  If we just checked for mtmp->mx
	     * != x || mtmp->my != y, we'd miss the case where we're called
	     * to place the worm segment and the worm's head is at x,y.
	     */
	    if (mtmp2 && (mtmp2 != mtmp || mtmp->wormno))
		return FALSE;

		/* Prevent monsters from teleporting to or being created on vivisection traps.
		 * Level generation must place monsters before placing the traps. */
		if (ttmp && ttmp->ttyp == VIVI_TRAP)
			return FALSE;

	    mdat = mtmp->data;
	    if (is_3dwater(x,y) && !ignorewater) {
			if(mtmp == &youmonst && level.flags.lethe)
				return FALSE;
			if (mtmp == &youmonst)
				/* not Amphibious -- teleporting into 3Dwater with limited breath can be very dangerous */
				return !!(Breathless && Waterproof && !(u.sealsActive&SEAL_OSE));
			else return (mon_resistance(mtmp,SWIMMING) || breathless_mon(mtmp) || amphibious_mon(mtmp));
	    } else if (is_pool(x,y, FALSE) && !ignorewater) {
			//The water level has 3d water and "pools" of water lining the bubbles as the only terrain.
			// Even if the pools wouldn't normally be considered OK, they have to be allowed as the only viable option.
			if(Is_waterlevel(&u.uz))
				return TRUE;
			if(mtmp == &youmonst && level.flags.lethe)
				return !!(Levitation || Flying || Wwalking);
			if (mtmp == &youmonst)
				return !!(Levitation || Flying || Wwalking ||
						((Swimming || Amphibious) && Waterproof && !(u.sealsActive&SEAL_OSE)));
			else	return (mon_resistance(mtmp,FLYING) || breathless_mon(mtmp) || mon_resistance(mtmp,SWIMMING) ||
								is_clinger(mdat) || amphibious_mon(mtmp));
	    } else if (mdat->mlet == S_EEL && !ignorewater && rn2(13)) {
			if (is_pool(x, y, TRUE))
				return (mdat->msize == MZ_TINY);
			return FALSE;
	    } else if (is_lava(x,y)) {
			if (mtmp == &youmonst)
				return !!(Levitation || Flying || likes_lava(youracedata));
			else
				return (mon_resistance(mtmp,FLYING) || likes_lava(mdat));
	    }
	    if (mon_resistance(mtmp,PASSES_WALLS) && may_passwall(x,y)) return TRUE;
	}
	if(Is_sigil(&u.uz) && levl[x][y].typ == AIR){
	    return FALSE;
	}
	if (!ACCESSIBLE(levl[x][y].typ)) {
		if (!(is_pool(x,y, FALSE) && ignorewater)) return FALSE;
	}
	if(In_quest(&u.uz) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && levl[x][y].typ == AIR){
		if(mtmp){
			return mtmp == &youmonst ? (Flying || Levitation) : (mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION));
		}
		else return FALSE;
	}

	if (closed_door(x, y) && (!mdat || !amorphous(mdat)))
		return FALSE;
	if (boulder_at(x, y) && (!mdat || !throws_rocks(mdat)))
		return FALSE;
	return TRUE;
}

    // int knix[8] = {2,  2,  1, -1, -2, -2,  1, -1};
    // int kniy[8] = {1, -1,  2,  2,  1, -1, -2, -2};
    // int camx[8] = {3,  3,  1, -1, -3, -3,  1, -1};
    // int camy[8] = {1, -1,  3,  3,  1, -1, -3, -3};
    // int zebx[8] = {3,  3,  2, -2, -3, -3,  2, -2};
    // int zeby[8] = {2, -2,  3,  3,  2, -2, -3, -3};
	
boolean
eofflin(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
#define TARGET_GOOD 15
    coord good[TARGET_GOOD], *good_ptr;
    int x, y, range, i;
    int xmin, xmax, ymin, ymax;
    struct monst fakemon = {0};	/* dummy monster */

    if (!mdat) {
#ifdef DEBUG
	pline("enexto() called with mdat==0");
#endif
	/* default to player's original monster type */
	mdat = &mons[u.umonster];
    }
    set_mon_data_core(&fakemon, mdat);	/* set up for goodpos */
    good_ptr = good;
    range = 3;
    /*
     * Walk around the border of the square with center (xx,yy) and
     * radius range.  Stop when we find at least one valid position.
     */
    do {
	xmin = max(1, xx-range);
	xmax = min(COLNO-1, xx+range);
	ymin = max(0, yy-range);
	ymax = min(ROWNO-1, yy+range);

	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymin, &fakemon, 0) && !linedup(u.ux,u.uy,x,ymin)) {
		good_ptr->x = x;
		good_ptr->y = ymin ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[TARGET_GOOD-1]) goto full;
	    }
	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymax, &fakemon, 0) && !linedup(u.ux,u.uy,x,ymax)) {
		good_ptr->x = x;
		good_ptr->y = ymax ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[TARGET_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmin, y, &fakemon, 0) && !linedup(u.ux,u.uy,xmin,y)) {
		good_ptr->x = xmin;
		good_ptr-> y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[TARGET_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmax, y, &fakemon, 0) && !linedup(u.ux,u.uy,xmax,y)) {
		good_ptr->x = xmax;
		good_ptr->y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[TARGET_GOOD-1]) goto full;
	    }
	range++;

	/* return if we've grown too big (nothing is valid) */
	if (range > ROWNO && range > COLNO){
		if(good_ptr != good) goto full;
		else return FALSE;
	}
    } while (good_ptr < &good[TARGET_GOOD-1]);

full:
    i = rn2((int)(good_ptr - good));
    cc->x = good[i].x;
    cc->y = good[i].y;
	if(!isok(cc->x, cc->y)){
		impossible("eofflin() attempting to return invalid coordinates as valid");
		return FALSE;
	}
    return TRUE;
}

boolean
eofflin_close(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
#define COORD_SIZE	8
    coord good[COORD_SIZE], *good_ptr;
    int x, y, i;
    int xmin, xmax, ymin, ymax;
    struct monst fakemon = {0};	/* dummy monster */
    int knix[COORD_SIZE] = {2,  2,  1, -1, -2, -2,  1, -1};
    int kniy[COORD_SIZE] = {1, -1,  2,  2,  1, -1, -2, -2};

    if (!mdat) {
#ifdef DEBUG
	pline("enexto() called with mdat==0");
#endif
	/* default to player's original monster type */
	mdat = &mons[u.umonster];
    }
    set_mon_data_core(&fakemon, mdat);	/* set up for goodpos */
    good_ptr = good;
	
	for(i = 0; i < COORD_SIZE; i++){
		x = u.ux + knix[i];
		y = u.uy + kniy[i];
		if(goodpos(x, y, &fakemon, 0)){
			good_ptr->x = x;
			good_ptr->y = y;
			good_ptr++;
		}
	}
	//No good spots
	if(!(good_ptr - good))
		return FALSE;
    i = rn2((int)(good_ptr - good));
    cc->x = good[i].x;
    cc->y = good[i].y;
	if(!isok(cc->x, cc->y)){
		impossible("eofflin_close() attempting to return invalid coordinates as valid");
		return FALSE;
	}
    return TRUE;
}

boolean
eonline(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
    int x, y, subx=-1, suby=-1, i, j, offset = rn2(8);
    int dx[8] = {0, 1,  0, -1, 1,  1, -1, -1};
    int dy[8] = {1, 0, -1,  0, 1, -1, -1,  1};
    struct monst fakemon = {0};	/* dummy monster */

    if (!mdat) {
#ifdef DEBUG
	pline("eonline() called with mdat==0");
#endif
	/* default to player's original monster type */
	mdat = &mons[u.umonster];
    }
	set_mon_data_core(&fakemon, mdat);	/* set up for goodpos */
	for(j = 0; j < 8; j++){
		x = xx;
		y = yy;
		for(i = 1; i < BOLT_LIM; i++){
			x += dx[(j+offset)%8];
			y += dy[(j+offset)%8];
			if(!isok(x,y) || !goodpos(x, y, &fakemon, 0) || IS_STWALL(levl[x][y].typ)){
				i--;
				x -= dx[(j+offset)%8];
				y -= dy[(j+offset)%8];
				if(i > BOLT_LIM/2){
					cc->x = x;
					cc->y = y;
					return TRUE;
				} else if(i > 1 && subx < 0){
					//found a sub-optimal location
					subx = x;
					suby = y;
				}
				break;
			}
		}
	}
	if(subx != -1){
		cc->x = subx;
		cc->y = suby;
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
 * "entity next to"
 *
 * Attempt to find a good place for the given monster type in the closest
 * position to (xx,yy).  Do so in successive square rings around (xx,yy).
 * If there is more than one valid positon in the ring, choose one randomly.
 * Return TRUE and the position chosen when successful, FALSE otherwise.
 */
boolean
enexto(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
	return enexto_core(cc, xx, yy, mdat, 0);
}

boolean
enexto_core(cc, xx, yy, mdat, entflags)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
unsigned entflags;
{
#define MAX_GOOD 15
    coord good[MAX_GOOD], *good_ptr;
    int x, y, range, i;
    int xmin, xmax, ymin, ymax;
    struct monst fakemon = {0};	/* dummy monster */

    if (!mdat) {
#ifdef DEBUG
	pline("enexto() called with mdat==0");
#endif
	/* default to player's original monster type */
	mdat = &mons[u.umonster];
    }
	set_mon_data_core(&fakemon, mdat);
    good_ptr = good;
    range = 1;
    /*
     * Walk around the border of the square with center (xx,yy) and
     * radius range.  Stop when we find at least one valid position.
     */
    do {
	xmin = max(1, xx-range);
	xmax = min(COLNO-1, xx+range);
	ymin = max(0, yy-range);
	ymax = min(ROWNO-1, yy+range);

	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymin, &fakemon, entflags)) {
		good_ptr->x = x;
		good_ptr->y = ymin ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymax, &fakemon, entflags)) {
		good_ptr->x = x;
		good_ptr->y = ymax ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmin, y, &fakemon, entflags)) {
		good_ptr->x = xmin;
		good_ptr-> y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmax, y, &fakemon, entflags)) {
		good_ptr->x = xmax;
		good_ptr->y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	range++;

	/* return if we've grown too big (nothing is valid) */
	if (range > ROWNO && range > COLNO) return FALSE;
    } while (good_ptr == good);

full:
    i = rn2((int)(good_ptr - good));
    cc->x = good[i].x;
    cc->y = good[i].y;
	if(!isok(cc->x, cc->y)){
		impossible("enexto() attempting to return invalid coordinates as valid");
		return FALSE;
	}
    return TRUE;
}

/*
 * "entity path to"
 *
 * Calls function `func(data, x, y)` on a number of locations.
 * Determines those locations from a starting point (`xx`,`yy`), pathing outwards up `r` moves away
 * `func` returns 0 if (x,y) is considered accessible (path can continue from that square), 1 otherwise
 */

#define EPATHTO_UNSEEN		0x0
#define EPATHTO_INACCESSIBLE	0x1
#define EPATHTO_DONE		0x2
#define EPATHTO_TAIL(n)		(0x3 + ((n) & 7))

#define EPATHTO_XY(x,y)		(((y) + 1) * COLNO + (x))
#define EPATHTO_Y(xy)		((xy) / COLNO - 1)
#define EPATHTO_X(xy)		((xy) % COLNO)

void
xpathto(r, xx, yy, func, data)
int r;
register xchar xx, yy;
int (*func)(genericptr_t, int, int);
genericptr_t data;
{
    int i, j, dir, xy, x, y;
    int path_len, postype;
    int first_col, last_col;
    int nd, n;
    unsigned char *map;
    static const int dirs[8] =
      /* N, S, E, W, NW, NE, SE, SW */
      { -COLNO, COLNO, 1, -1, -COLNO-1, -COLNO+1, COLNO+1, COLNO-1};
    map = (unsigned char *)alloc(COLNO * (ROWNO + 2) + 1);
    (void) memset((genericptr_t)map, EPATHTO_INACCESSIBLE, COLNO * (ROWNO + 2) + 1);
    for(i = 1; i < COLNO; i++)
	for(j = 0; j < ROWNO; j++)
	    map[EPATHTO_XY(i, j)] = EPATHTO_UNSEEN;
    map[EPATHTO_XY(xx, yy)] = EPATHTO_TAIL(0);
    if (func(data, xx, yy) == 0)
	nd = n = 1;
    else
	nd = n = 0;
    for(path_len = 0; path_len < r; path_len++)
    {
	/* x >= 1 && x <= COLNO-1 && y >= 0 && y <= ROWNO-1 */
	first_col = max(1, xx - path_len);
	last_col = min(COLNO - 1, xx + path_len);
	for(j = max(0, yy - path_len); j <= min(ROWNO - 1, yy + path_len); j++)
	    for(i = first_col; i <= last_col; i++)
		if (map[EPATHTO_XY(i, j)] == EPATHTO_TAIL(path_len)) {
		    map[EPATHTO_XY(i, j)] = EPATHTO_DONE;
		    for(dir = 0; dir < 8; dir++) {
			xy = EPATHTO_XY(i, j) + dirs[dir];
			if (map[xy] == EPATHTO_UNSEEN) {
			    x = EPATHTO_X(xy);
			    y = EPATHTO_Y(xy);
			    postype = func(data, x, y);
			    map[xy] = postype ? EPATHTO_INACCESSIBLE :
				    EPATHTO_TAIL(path_len + 1);
			    if (postype == 0)
				++n;
			}
		    }
		}
	if (nd == n)
	    break;	/* No more positions */
	else
	    nd = n;
    }
    free((genericptr_t)map);
}

/*
 * Check for restricted areas present in some special levels.  (This might
 * need to be augmented to allow deliberate passage in wizard mode, but
 * only for explicitly chosen destinations.)
 */
STATIC_OVL boolean
tele_jump_ok(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	if (dndest.nlx > 0) {
	    /* if inside a restricted region, can't teleport outside */
	    if (within_bounded_area(x1, y1, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy) &&
		!within_bounded_area(x2, y2, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy))
		return FALSE;
	    /* and if outside, can't teleport inside */
	    if (!within_bounded_area(x1, y1, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy) &&
		within_bounded_area(x2, y2, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy))
		return FALSE;
	}
	if (updest.nlx > 0) {		/* ditto */
	    if (within_bounded_area(x1, y1, updest.nlx, updest.nly,
						updest.nhx, updest.nhy) &&
		!within_bounded_area(x2, y2, updest.nlx, updest.nly,
						updest.nhx, updest.nhy))
		return FALSE;
	    if (!within_bounded_area(x1, y1, updest.nlx, updest.nly,
						updest.nhx, updest.nhy) &&
		within_bounded_area(x2, y2, updest.nlx, updest.nly,
						updest.nhx, updest.nhy))
		return FALSE;
	}
	return TRUE;
}

boolean
teleok(x, y, trapok)
register int x, y;
boolean trapok;
{
	if (!trapok && t_at(x, y)) return FALSE;
	if (!goodpos(x, y, &youmonst, 0)) return FALSE;
	if (!tele_jump_ok(u.ux, u.uy, x, y)) return FALSE;
	if (!in_out_region(x, y)) return FALSE;
	return TRUE;
}

void
teleds(nux, nuy, allow_drag)
register int nux,nuy;
boolean allow_drag;
{
	boolean ball_active = (Punished && uball->where != OBJ_FREE),
		ball_still_in_range = FALSE;

	/* If they have to move the ball, then drag if allow_drag is true;
	 * otherwise they are teleporting, so unplacebc().  
	 * If they don't have to move the ball, then always "drag" whether or
	 * not allow_drag is true, because we are calling that function, not
	 * to drag, but to move the chain.  *However* there are some dumb
	 * special cases:
	 *    0				 0
	 *   _X  move east       ----->  X_
	 *    @				  @
	 * These are permissible if teleporting, but not if dragging.  As a
	 * result, drag_ball() needs to know about allow_drag and might end
	 * up dragging the ball anyway.  Also, drag_ball() might find that
	 * dragging the ball is completely impossible (ball in range but there's
	 * rock in the way), in which case it teleports the ball on its own.
	 */
	if (ball_active) {
	    if (!carried(uball) && distmin(nux, nuy, uball->ox, uball->oy) <= 2)
		ball_still_in_range = TRUE; /* don't have to move the ball */
	    else {
		/* have to move the ball */
		if (!allow_drag || distmin(u.ux, u.uy, nux, nuy) > 1) {
		    /* we should not have dist > 1 and allow_drag at the same
		     * time, but just in case, we must then revert to teleport.
		     */
		    allow_drag = FALSE;
		    unplacebc();
		}
	    }
	}
	if(u.utraptype != TT_SALIVA)
		u.utrap = 0;
	u.ustuck = 0;
	u.ux0 = u.ux;
	u.uy0 = u.uy;

	if (hides_under(youracedata))
		u.uundetected = OBJ_AT(nux, nuy);
	else if (is_underswimmer(youracedata))
		u.uundetected = is_pool(nux, nuy, FALSE);
	else {
		u.uundetected = 0;
		/* mimics stop being unnoticed */
		if (youracedata->mlet == S_MIMIC)
		    youmonst.m_ap_type = M_AP_NOTHING;
	}

	if (u.uswallow) {
		u.uswldtim = u.uswallow = 0;
		if (Punished && !ball_active) {
		    /* ensure ball placement, like unstuck */
		    ball_active = TRUE;
		    allow_drag = FALSE;
		}
		docrt();
	}
	if (ball_active) {
	    if (ball_still_in_range || allow_drag) {
		int bc_control;
		xchar ballx, bally, chainx, chainy;
		boolean cause_delay;

		if (drag_ball(nux, nuy, &bc_control, &ballx, &bally,
				    &chainx, &chainy, &cause_delay, allow_drag))
		    move_bc(0, bc_control, ballx, bally, chainx, chainy);
	    }
	}
	/* must set u.ux, u.uy after drag_ball(), which may need to know
	   the old position if allow_drag is true... */
	u.lastmoved = monstermoves;
	u.ux = nux;
	u.uy = nuy;
	fill_pit(u.ux0, u.uy0);
	if (ball_active) {
	    if (!ball_still_in_range && !allow_drag)
		placebc();
	}
	initrack(); /* teleports mess up tracking monsters without this */
	update_player_regions();
#ifdef STEED
	/* Move your steed, too */
	if (u.usteed) {
		u.usteed->mx = nux;
		u.usteed->my = nuy;
	}
#endif
	/*
	 *  Make sure the hero disappears from the old location.  This will
	 *  not happen if she is teleported within sight of her previous
	 *  location.  Force a full vision recalculation because the hero
	 *  is now in a new location.
	 */
	newsym(u.ux0,u.uy0);
	see_monsters();
	vision_full_recalc = 1;
	nomul(0, NULL);
	vision_recalc(0);	/* vision before effects */
	if(u.usubwater && !is_3dwater(u.ux, u.uy)){
		u.usubwater = 0;
		vision_recalc(2);	/* unsee old position */
		vision_full_recalc = 1;
		doredraw();
	}
	spoteffects(TRUE);
	invocation_message();
}

boolean
safe_teleds(allow_drag)
boolean allow_drag;
{
	register int nux, nuy, tcnt = 0;

	do {
		nux = rnd(COLNO-1);
		nuy = rn2(ROWNO);
	} while (!teleok(nux, nuy, (boolean)(tcnt > 200)) && ++tcnt <= 400);

	if (tcnt <= 400) {
		teleds(nux, nuy, allow_drag);
		return TRUE;
	} else
		return FALSE;
}

STATIC_OVL void
vault_tele()
{
	register struct mkroom *croom = search_special(VAULT);
	coord c;

	if (croom && somexy(croom, &c) && teleok(c.x,c.y,FALSE)) {
		teleds(c.x,c.y,FALSE);
		return;
	}
	tele();
}

boolean
teleport_pet(mtmp, force_it)
register struct monst *mtmp;
boolean force_it;
{
	register struct obj *otmp;

#ifdef STEED
	if (mtmp == u.usteed)
		return (FALSE);
#endif

	if (mtmp->mleashed) {
	    otmp = get_mleash(mtmp);
	    if (!otmp) {
		impossible("%s is leashed, without a leash.", Monnam(mtmp));
		goto release_it;
	    }
	    if (otmp->cursed && !force_it) {
		yelp(mtmp);
		return FALSE;
	    } else {
		Your("leash goes slack.");
 release_it:
		m_unleash(mtmp, FALSE);
		return TRUE;
	    }
	}
	return TRUE;
}

boolean
tele()
{
	coord cc;

	/* Disable teleportation in stronghold && Vlad's Tower */
	if (notel_level()) {
#ifdef WIZARD
		if (!wizard) {
#endif
		    pline("A mysterious force prevents you from teleporting!");
		    return TRUE;
#ifdef WIZARD
		}
#endif
	}

	/* don't show trap if "Sorry..." */
	if (!Blinded) make_blinded(0L,FALSE);

	if
#ifdef WIZARD
	(
#endif
	 (u.uhave.amulet || On_W_tower_level(&u.uz)
#ifdef STEED
	  || (u.usteed && mon_has_amulet(u.usteed))
#endif
	 )
#ifdef WIZARD
	 && (!wizard) )
#endif
	{
		You_feel("disoriented for a moment.");
		(void) safe_teleds(FALSE);/*teleportation still functions as an escape*/
		return TRUE;
	}
	if ((Teleport_control && !Stunned)
#ifdef WIZARD
			    || wizard
#endif
	) {
	    if (unconscious()) {
			pline("Being unconscious, you cannot control your teleport.");
	    } else {
#ifdef STEED
		    char buf[BUFSZ];
		    if (u.usteed) Sprintf(buf," and %s", mon_nam(u.usteed));
#endif
		    pline("To what position do you%s want to be teleported?",
#ifdef STEED
				u.usteed ? buf :
#endif
			   "");
		    cc.x = u.ux;
		    cc.y = u.uy;
		    if (getpos(&cc, TRUE, "the desired position") < 0)
			return FALSE;	/* abort */
		    /* possible extensions: introduce a small error if
		       magic power is low; allow transfer to solid rock */
		    if (teleok(cc.x, cc.y, FALSE)) {
			teleds(cc.x, cc.y, FALSE);
			return TRUE;
		    }
		    pline("Sorry...");
		}
	}

	(void) safe_teleds(FALSE);
	return TRUE;
}

int
dotele()
{
	struct trap *trap;

	trap = t_at(u.ux, u.uy);
	if (trap && (!trap->tseen || (trap->ttyp != TELEP_TRAP && trap->ttyp != LEVEL_TELEP)))
		trap = 0;

	if (trap) {
		if (trap->once) {
			pline("This is a vault teleport, usable once only.");
			if (yn("Jump in?") == 'n')
				trap = 0;
			else {
				deltrap(trap);
				newsym(u.ux, u.uy);
			}
		}
		if (trap){
			You("%s onto the teleportation trap.",
			    locomotion(&youmonst, "jump"));
			if(trap->ttyp == LEVEL_TELEP){
				level_tele_trap(trap, TRUE);
				return MOVE_STANDARD;
			}
		}
	}
	if (!trap) {
	    boolean castit = FALSE;
	    register int sp_no = 0, energy = 0;

	    if (!Teleportation || (u.ulevel < (Role_if(PM_WIZARD) ? 8 : 12)
					&& !mon_resistance(&youmonst,TELEPORT))) {
		/* Try to use teleport away spell. */
		if (objects[SPE_TELEPORT_AWAY].oc_name_known && !Confusion)
		    for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
			if (spl_book[sp_no].sp_id == SPE_TELEPORT_AWAY) {
				castit = TRUE;
				break;
			}
#ifdef WIZARD
		if (!wizard) {
#endif
		    if (!castit) {
			if (!Teleportation)
			    You("don't know that spell.");
			else You("are not able to teleport at will.");
			return MOVE_CANCELLED;
		    }
#ifdef WIZARD
		}
#endif
	    }

	    if ((!Race_if(PM_INCANTIFIER) && u.uhunger <= 100*get_uhungersizemod()) || ACURR(A_STR) < 6) {
#ifdef WIZARD
		if (!wizard) {
#endif
			You("lack the strength %s.",
			    castit ? "for a teleport spell" : "to teleport");
			return MOVE_STANDARD;
#ifdef WIZARD
		}
#endif
	    }

	    energy = objects[SPE_TELEPORT_AWAY].oc_level * 7 / 2 - 2;
	    if (u.uen <= energy) {
#ifdef WIZARD
		if (wizard)
			energy = u.uen;
		else
#endif
		{
			You("lack the energy %s.",
			    castit ? "for a teleport spell" : "to teleport");
			return MOVE_STANDARD;
		}
	    }

	    if (check_capacity(
			"Your concentration falters from carrying so much."))
		return MOVE_STANDARD;

	    if (castit) {
		exercise(A_WIS, TRUE);
		if (spelleffects(sp_no, TRUE, 0))
			return MOVE_STANDARD;
		else
#ifdef WIZARD
		    if (!wizard)
#endif
			return MOVE_CANCELLED;
	    } else {
		losepw(energy);
		flags.botl = 1;
	    }
	}

	if (next_to_u()) {
		if (trap && trap->once) vault_tele();
		else tele();
		(void) next_to_u();
	} else {
		You1(shudder_for_moment);
		return MOVE_INSTANT;
	}
	if (!trap) morehungry(100*get_uhungersizemod());
	return MOVE_STANDARD;
}


boolean
level_tele()
{
	register int newlev;
	d_level newlevel;
	const char *escape_by_flying = 0;	/* when surviving dest of -N */
	char buf[BUFSZ];
	boolean force_dest = FALSE;
	if (iflags.debug_fuzzer)
		goto random_levtport;

	if ((u.uhave.amulet || In_endgame(&u.uz) || In_sokoban(&u.uz) || In_void(&u.uz))
#ifdef WIZARD
						&& !wizard
#endif
							) {
	    You_feel("very disoriented for a moment.");
	    return TRUE;
	}
	if(In_spire(&u.uz) && !Is_sigil(&u.uz)){
		//All roads lead to rome
		You_feel("reoriented.");
	    	schedule_goto(&sigil_level, FALSE, FALSE, 0, (char *)0, (char *)0, 0, 0);
		return TRUE;
	}
	if ((Teleport_control && !Stunned)
#ifdef WIZARD
	   || wizard
#endif
		) {
	    char qbuf[BUFSZ];
	    int trycnt = 0;

	    Strcpy(qbuf, "To what level do you want to teleport?");
	    do {
		if (++trycnt == 2) {
#ifdef WIZARD
			if (wizard) Strcat(qbuf, " [type a number or ? for a menu]");
			else
#endif
			Strcat(qbuf, " [type a number]");
		}
		getlin(qbuf, buf);
		if (!strcmp(buf,"\033")) {	/* cancelled */
			return FALSE;
		} else if (!strcmp(buf,"*")) {
			goto random_levtport;
		} else if (Confusion && rnl(100) >= 20) {
			pline("Oops...");
			goto random_levtport;
		}
#ifdef WIZARD
		if (wizard && !strcmp(buf,"?")) {
		    schar destlev = 0;
		    int destdnum = 0;

		    if (print_dungeon(TRUE, FALSE, &destlev, &destdnum)) {
			newlevel.dnum = (xchar) destdnum;
			newlevel.dlevel = destlev;
			newlev = depth(&newlevel);
			if (In_endgame(&newlevel) && !In_endgame(&u.uz)) {
				Sprintf(buf,
				    "Destination is earth level");
				if (!u.uhave.amulet) {
					struct obj *obj;
					obj = mksobj(AMULET_OF_YENDOR, NO_MKOBJ_FLAGS);
					if (obj) {
						obj = addinv(obj);
						Strcat(buf, " with the amulet");
					}
				}
				assign_level(&newlevel, &earth_level);
				pline("%s.", buf);
			}
			force_dest = TRUE;
		    } else return TRUE;
		} else
#endif
		if ((newlev = lev_by_name(buf)) == 0) newlev = atoi(buf);
	    } while (!force_dest && (!newlev && !digit(buf[0]) &&
		     (buf[0] != '-' || !digit(buf[1])) &&
		     trycnt < 10));

	    /* no dungeon escape via this route */
	    if (newlev == 0 && !force_dest) {
			if (trycnt >= 10)
				goto random_levtport;
			if (Is_nowhere(&u.uz)) return FALSE;
			else if (Race_if(PM_ETHEREALOID)) {
				flags.phasing = FALSE;
				u.old_lev.uz = u.uz;
				u.old_lev.ux = u.ux;
				u.old_lev.uy = u.uy;
				goto_level(&nowhere_level, FALSE, FALSE, FALSE);
				return TRUE;
			}
			if (yesno("Go to Nowhere.  Are you sure?", iflags.paranoid_quit) != 'y') return FALSE;
			You("%s in agony as your body begins to warp...",
				is_silent(youracedata) ? "writhe" : "scream");
			display_nhwindow(WIN_MESSAGE, FALSE);
			You("cease to exist.");
			if (invent) Your("possessions land on the %s with a thud.",
					surface(u.ux, u.uy));
			killer_format = NO_KILLER_PREFIX;
			killer = "committed suicide";
			done(DIED);
			pline("An energized cloud of dust begins to coalesce.");
			Your("body rematerializes%s.", invent ?
				", and you gather up all your possessions" : "");
			return TRUE;
	    }

	    /* if in Knox and the requested level > 0, stay put.
	     * we let negative values requests fall into the "heaven" loop.
	     */
	    if (!force_dest && Is_knox(&u.uz) && newlev > 0) {
			You1(shudder_for_moment);
			return TRUE;
	    }
	    /* if in Quest, the player sees "Home 1", etc., on the status
	     * line, instead of the logical depth of the level.  controlled
	     * level teleport request is likely to be relativized to the
	     * status line, and consequently it should be incremented to
	     * the value of the logical depth of the target level.
	     *
	     * we let negative values requests fall into the "heaven" loop.
	     */
		if(!force_dest && (In_quest(&u.uz) || In_tower(&u.uz) || In_law(&u.uz) || In_neu(&u.uz) || In_cha(&u.uz))){
			boolean rangeRestricted = TRUE;
			int urlev = dungeons[u.uz.dnum].depth_start + u.uz.dlevel - 1;
			if (In_quest(&u.uz) && newlev > 0){
				newlev = newlev + dungeons[u.uz.dnum].depth_start - 1;
				if(u.uhave.questart) rangeRestricted = FALSE;
			} else if (In_tower(&u.uz)) {
				if(newlev > 4) newlev = 4;
				newlev = dungeons[u.uz.dnum].depth_start + 4 - newlev;
				if(mvitals[PM_VLAD_THE_IMPALER].died > 0) rangeRestricted = FALSE;
			} else if (In_law(&u.uz)) {
				if (newlev > 13) newlev = 13;
				newlev = dungeons[u.uz.dnum].depth_start + (path1_level.dlevel - newlev);
				if(mvitals[PM_ARSENAL].died > 0) rangeRestricted = FALSE;
			} else if (In_neu(&u.uz)) {
				struct obj *pobj;
				newlev =  newlev + dungeons[neutral_dnum].depth_start - 1;
				for(pobj = invent; pobj; pobj=pobj->nobj){
					if(pobj->oartifact == ART_SILVER_KEY || pobj->oartifact == ART_HAND_MIRROR_OF_CTHYLLA){
						rangeRestricted = FALSE;
						break;
					}
				}
			} else if (In_cha(&u.uz)){
				newlev = newlev + dungeons[u.uz.dnum].depth_start - 1;
				if(In_FF_quest(&u.uz)){
					if(mvitals[PM_CHAOS].died > 0) rangeRestricted = FALSE;
				} else if(In_mithardir_quest(&u.uz)){
					if(u.ufirst_life && u.ufirst_sky && u.ufirst_light) rangeRestricted = FALSE;
				} else if(In_mordor_quest(&u.uz)){
					if(mvitals[PM_LUNGORTHIN].died > 0) rangeRestricted = FALSE;
				}
			}
#ifdef WIZARD
			if (rangeRestricted && wizard && yn("Respect range restrictions?") == 'n')
				rangeRestricted = FALSE;
#endif
			if (rangeRestricted) {
				if(urlev <= 0){ /*Level 0 is skipped*/
					if(newlev < urlev) newlev = urlev-2;
					else if(newlev > urlev){
						if(urlev < 0) newlev = urlev;
						else newlev = 1;
					} else newlev = urlev-1; /*station keeping*/
				} else {
					if(newlev < urlev) newlev = urlev-1;
					else if(newlev > urlev) newlev = urlev+1;
				}
			}
		}
	} else { /* involuntary level tele */
 random_levtport:
	    newlev = random_teleport_level();
	    if (newlev == depth(&u.uz)) {
			tele();
			You1(shudder_for_moment);
			return TRUE;
	    }
	}

	if (!next_to_u()) {
		You1(shudder_for_moment);
		return TRUE;
	}
#ifdef WIZARD
	if (In_endgame(&u.uz)) {	/* must already be wizard */
	    int llimit = dunlevs_in_dungeon(&u.uz);

	    if (newlev >= 0 || newlev <= -llimit || (force_dest && newlevel.dnum != u.uz.dnum)) {
			You_cant("get there from here.");
			return TRUE;
	    }
	    newlevel.dnum = u.uz.dnum;
	    newlevel.dlevel = llimit + newlev;
	    schedule_goto(&newlevel, FALSE, FALSE, 0, (char *)0, (char *)0, 0, 0);
	    return TRUE;
	}
#endif

	killer = 0;		/* still alive, so far... */
	
	if (iflags.debug_fuzzer && newlev < 0)
       		goto random_levtport;
	
	if (newlev < 0 && dungeons[u.uz.dnum].depth_start-1 > newlev && !force_dest) {
		if (yesno("There will be no return. Are you sure?", iflags.paranoid_quit) != 'y') return FALSE;
		if (*u.ushops0) {
		    /* take unpaid inventory items off of shop bills */
		    in_mklev = TRUE;	/* suppress map update */
		    u_left_shop(u.ushops0, TRUE);
		    /* you're now effectively out of the shop */
		    *u.ushops0 = *u.ushops = '\0';
		    in_mklev = FALSE;
		}
		if (newlev <= -10) {
			You("arrive in heaven.");
			verbalize("Thou art early, but we'll admit thee.");
			killer_format = NO_KILLER_PREFIX;
			killer = "went to heaven prematurely";
		} else if (newlev == -9) {
			You_feel("deliriously happy. ");
			pline("(In fact, you're on Cloud 9!) ");
			display_nhwindow(WIN_MESSAGE, FALSE);
		} else
			You("are now high above the clouds...");

		if (killer) {
		    ;		/* arrival in heaven is pending */
		} else if (Levitation) {
		    escape_by_flying = "float gently down to earth";
		} else if (Flying) {
		    escape_by_flying = "fly down to the ground";
		} else {
		    pline("Unfortunately, you don't know how to fly.");
		    You("plummet a few thousand feet to your death.");
		    Sprintf(buf,
			  "teleported out of the dungeon and fell to %s death",
			    uhis());
		    killer = buf;
		    killer_format = NO_KILLER_PREFIX;
		}
	}
	// if(newlev<0) newlev++;//No dungeon's displayed level crosses 0 anymore //Dungeon levels should skip from 1 to -1, 0 should always be nowhere.
	
	if (killer) {	/* the chosen destination was not survivable */
	    d_level lsav;

	    /* set specific death location; this also suppresses bones */
	    lsav = u.uz;	/* save current level, see below */
	    u.uz.dnum = 0;	/* main dungeon */
	    u.uz.dlevel = (newlev <= -10) ? -10 : 0;	/* heaven or surface */
	    done(DIED);
	    /* can only get here via life-saving (or declining to die in
	       explore|debug mode); the hero has now left the dungeon... */
	    escape_by_flying = "find yourself back on the surface";
	    u.uz = lsav;	/* restore u.uz so escape code works */
	}

	/* calls done(ESCAPED) if newlevel==0 */
	if (escape_by_flying) {
	    You("%s.", escape_by_flying);
	    newlevel.dnum = 0;		/* specify main dungeon */
	    newlevel.dlevel = 0;	/* escape the dungeon */
	    /* [dlevel used to be set to 1, but it doesn't make sense to
		teleport out of the dungeon and float or fly down to the
		surface but then actually arrive back inside the dungeon] */
	} else if (u.uz.dnum == challenge_level.dnum &&
	    newlev >= dungeons[u.uz.dnum].depth_start +
						dunlevs_in_dungeon(&u.uz)) {
#ifdef WIZARD
	    if (!(wizard && force_dest))
#endif
	    find_hell(&newlevel);
	} else {
	    /* if invocation did not yet occur, teleporting into
	     * the last level of Gehennom is forbidden.
	     */
#ifdef WIZARD
	    if (!(wizard)) {
#endif
	    if (Inhell && !u.uevent.gehennom_entered && /*seal off gehennom untill you formally enter it by descending the valley of the dead staircase.*/
			newlev >= (dungeons[u.uz.dnum].depth_start)){
				newlev = dungeons[u.uz.dnum].depth_start;
				pline("Sorry...");
		}
		else if (Inhell && !u.uevent.invoked && /*seal off square level and sanctum untill you perform the invocation.*/
			newlev >= (dungeons[u.uz.dnum].depth_start +
					dunlevs_in_dungeon(&u.uz) - 2)) {
		newlev = dungeons[u.uz.dnum].depth_start +
					dunlevs_in_dungeon(&u.uz) - 3;
		pline("Sorry...");
	    }
#ifdef WIZARD
	    }
#endif
	    /* no teleporting out of quest dungeon */
	    if (In_quest(&u.uz) && newlev < depth(&qstart_level))
		newlev = depth(&qstart_level);
	    /* the player thinks of levels purely in logical terms, so
	     * we must translate newlev to a number relative to the
	     * current dungeon.
	     */
#ifdef WIZARD
	    if (!(wizard && force_dest))
#endif
	    get_level(&newlevel, newlev);
	}
	schedule_goto(&newlevel, FALSE, FALSE, 0, (char *)0, (char *)0, 0, 0);
	/* in case player just read a scroll and is about to be asked to
	   call it something, we can't defer until the end of the turn */
	if (u.utotype && !flags.mon_moving) deferred_goto();
	return TRUE;
}

boolean
branch_tele()
{
	int i, num_ok_dungeons, last_ok_dungeon = 0;
	d_level newlev;
	extern int n_dgns; /* from dungeon.c */
	winid tmpwin = create_nhwindow(NHW_MENU);
	anything any;

	any.a_void = 0;	/* set all bits to zero */
	start_menu(tmpwin);
	/* use index+1 (cant use 0) as identifier */
	for (i = num_ok_dungeons = 0; i < n_dgns; i++) {
	if (!dungeons[i].dunlev_ureached) continue;
	if(!strcmp(dungeons[i].dname,"Nowhere")) continue;
	any.a_int = i+1;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
		 dungeons[i].dname, MENU_UNSELECTED);
	num_ok_dungeons++;
	last_ok_dungeon = i;
	}
	end_menu(tmpwin, "Open a portal to which dungeon?");
	if (num_ok_dungeons > 1) {
	/* more than one entry; display menu for choices */
	menu_item *selected;
	int n;

	n = select_menu(tmpwin, PICK_ONE, &selected);
	if (n <= 0) {
		destroy_nhwindow(tmpwin);
		return FALSE;
	}
	i = selected[0].item.a_int - 1;
	free((genericptr_t)selected);
	} else
	i = last_ok_dungeon;	/* also first & only OK dungeon */
	destroy_nhwindow(tmpwin);

	/*
	 * i is now index into dungeon structure for the new dungeon.
	 * Find the closest level in the given dungeon, open
	 * a use-once portal to that dungeon and go there.
	 * The closest level is either the entry or dunlev_ureached.
	 */
	newlev.dnum = i;
	if(dungeons[i].depth_start >= depth(&u.uz))
	newlev.dlevel = dungeons[i].entry_lev;
	else
	newlev.dlevel = dungeons[i].dunlev_ureached;
	if(u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev) ||  In_void(&u.uz) || In_void(&newlev) || Is_nowhere(&u.uz) ||
	   newlev.dnum == u.uz.dnum) {
	You_feel("very disoriented for a moment.");
	} else {
	if(u.usteed && mon_has_amulet(u.usteed)){
		dismount_steed(DISMOUNT_VANISHED);
	}
	if(!Blind) You("are surrounded by a shimmering sphere!");
	else You_feel("weightless for a moment.");
	goto_level(&newlev, FALSE, FALSE, FALSE);
	}
	return TRUE;
}

void
domagicportal(ttmp)
register struct trap *ttmp;
{
	struct d_level target_level;

	if (!next_to_u()) {
		You1(shudder_for_moment);
		return;
	}

	/* if landed from another portal, do nothing */
	/* problem: level teleport landing escapes the check */
	if (!on_level(&u.uz, &u.uz0)) return;

	You("activated a magic portal!");

	/* prevent the poor shnook, whose amulet was stolen while in
	 * the endgame, from accidently triggering the portal to the
	 * next level, and thus losing the game
	 */
	if (In_endgame(&u.uz) && !u.uhave.amulet) {
	    You_feel("dizzy for a moment, but nothing happens...");
	    return;
	}

	if (In_endgame(&u.uz) && Role_if(PM_ANACHRONOUNBINDER)) {
		int missing_items = acu_asc_items_check();
		if (missing_items) {
			You("stop yourself from entering.");
			acu_asc_items_warning(missing_items);
			return;
		}
	}
	
	//The exit portal tries to return fem half dragon nobles to where they entered the quest.
	if(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON) && flags.initgend && In_quest(&u.uz)){
	    int i;
	    extern int n_dgns; /* from dungeon.c */
		int painting;
		//Look for return info
	    for (i = 0; i < n_dgns; i++)
			if(dungeons[i].dunlev_ureturn)
				break;
		if(i < n_dgns){
			target_level.dnum = i;
			target_level.dlevel = dungeons[i].dunlev_ureturn;
			dungeons[i].dunlev_ureturn = 0;
			painting = PAINTING_OUT;
		} else {
			//Possibly we branchported in.
			target_level = ttmp->dst;
			painting = FALSE;
		}
		schedule_goto(&target_level, FALSE, FALSE, painting,
				  painting ? "You find yourself next to a scrap of canvas." :
				  "You feel dizzy for a moment, but the sensation passes.",
				  (char *)0, 0, 0);
	} else {
		target_level = ttmp->dst;
		if(Role_if(PM_UNDEAD_HUNTER)
			&& In_endgame(&u.uz)
			&& Is_astralevel(&target_level)
			&& quest_status.moon_close
		){
			if(!philosophy_index(u.ualign.god)){
				pline("A blinding wall of silver-green light fills your consciousness, but you feel something beyond the wall pull you through.");
			}
			else if(research_incomplete()){
				char buf[BUFSZ];
				You("feel dizzy for a moment, and a blinding wall of silver-green light fills your consciousness.");
				You_feel("strangely peaceful, as though you were a rock skipping along the surface of a pond.");
				pline("You materialize high above the clouds...");
				//The game appears unwinnable (The Player could arrange to fail this check yet still have the ability to make the game winnable, but tough luck for them in that case)
				if(Levitation){
					pline("...and float gently down to earth.");
				}
				else if(Flying){
					You("fly down to the ground.");
				}
				else {
					pline("Unfortunately, you don't know how to fly.");
					You("plummet a few thousand feet to your death.");
					Sprintf(buf,
					  "was blocked from entering the heavenly temple and fell to %s death",
						uhis());
					killer = buf;
					killer_format = NO_KILLER_PREFIX;
					done(DIED);
					You("find yourself back on the surface.");
				}
				done(ESCAPED); //On surface :(
			}
			else {
				pline("A blinding wall of silver-green light fills your consciousness.");
				if(((u.ualign.type == A_LAWFUL && known_glyph(ROTTEN_EYES)) || active_glyph(ROTTEN_EYES)) && reanimation_count() >= 6){
					pline("...Actually, it's more of a maze, and your many eyes can see the way through!");
				}
				else if(((u.ualign.type == A_CHAOTIC && known_glyph(DEFILEMENT)) || active_glyph(DEFILEMENT)) && defile_count() >= 6){
					pline("The wall doesn't seem to notice you, or impede your progress in any way.");
				}
				else if(((u.ualign.type == A_NEUTRAL && known_glyph(LUMEN)) || active_glyph(LUMEN)) && parasite_count() >= 6){
					pline("Your parasites lift their voices in song, and the wall opens to allow you through!");
				}
			}
		}
		schedule_goto(&target_level, FALSE, FALSE, TRUE,
				  "You feel dizzy for a moment, but the sensation passes.",
				  (char *)0, 0, 0);
	}
	if(ttmp && ttmp->special){
		/*With appologies to "Through the Gates of the Silver Key" by H. P. Lovecraft. */
		pline("You have paused in your travels for a time, in a strange place where place and time have no meaning.");
		pline("There is no distinction between your past and present selves, just as there is no distinction between your future and present. There is only the immutable %s.", plname);
		pline("Light filters down from a sky of no color in baffling, contradictory directions, and plays almost sentiently over a line of gigantic hexagonal pedestals.");
		pline("Surmounting these pedestals are cloaked, ill-defined Shapes, and another Shape, too, which occupies no pedestal.");
		pline("It speaks to you without sound or language, and you find yourself at your destination!");
		pline("A seal is engraved into your mind!");
		u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
	}
}

void
tele_trap(trap)
struct trap *trap;
{
	if (In_endgame(&u.uz) || Antimagic) {
		if (Antimagic)
			shieldeff(u.ux, u.uy);
		You_feel("a wrenching sensation.");
	} else if (!next_to_u()) {
		You1(shudder_for_moment);
	} else if (trap->once) {
		deltrap(trap);
		trap = 0;
		newsym(u.ux,u.uy);	/* get rid of trap symbol */
		vault_tele();
	} else
		tele();

	if(trap && trap->special){
		/*With appologies to "Through the Gates of the Silver Key" by H. P. Lovecraft. */
		pline("You have paused in your travels for a time, in a strange place where place and time have no meaning.");
		pline("There is no distinction between your past and present selves, just as there is no distinction between your future and present. There is only the immutable %s.", plname);
		pline("Light filters down from a sky of no color in baffling, contradictory directions, and plays almost sentiently over a line of gigantic hexagonal pedestals.");
		pline("Surmounting these pedestals are cloaked, ill-defined Shapes, and another Shape, too, which occupies no pedestal.");
		pline("It speaks to you without sound or language, and you find yourself at your destination!");
		pline("A seal is engraved into your mind!");
		u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
	}
}

void
level_tele_trap(struct trap *trap, boolean force)
{
	You("%s onto a level teleport trap!",
		      Levitation ? (const char *)"float" :
				  locomotion(&youmonst, "step"));
	if (Antimagic && !force) {
	    shieldeff(u.ux, u.uy);
	}
	if ((Antimagic && !force) || In_endgame(&u.uz)) {
	    You_feel("a wrenching sensation.");
	    return;
	}
	if (!Blind)
	    You("are momentarily blinded by a flash of light.");
	else
	    You("are momentarily disoriented.");

	if(trap && trap->special){
		/*With appologies to "Through the Gates of the Silver Key" by H. P. Lovecraft. */
		pline("You have paused in your travels for a time, in a strange place where place and time have no meaning.");
		pline("There is no distinction between your past and present selves, just as there is no distinction between your future and present. There is only the immutable %s.", plname);
		pline("Light filters down from a sky of no color in baffling, contradictory directions, and plays almost sentiently over a line of gigantic hexagonal pedestals.");
		pline("Surmounting these pedestals are cloaked, ill-defined Shapes, and another Shape, too, which occupies no pedestal.");
		pline("It speaks to you without sound or language, and you find yourself at your destination!");
		pline("A seal is engraved into your mind!");
		u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
	}

	deltrap(trap);
	newsym(u.ux,u.uy);	/* get rid of trap symbol */
	level_tele();
}

/* check whether monster can arrive at location <x,y> via Tport (or fall) */
STATIC_OVL boolean
rloc_pos_ok(x, y, mtmp)
register int x, y;		/* coordinates of candidate location */
struct monst *mtmp;
{
	register int xx, yy;

	if (!goodpos(x, y, mtmp, 0)) return FALSE;
	/*
	 * Check for restricted areas present in some special levels.
	 *
	 * `xx' is current column; if 0, then `yy' will contain flag bits
	 * rather than row:  bit #0 set => moving upwards; bit #1 set =>
	 * inside the Wizard's tower.
	 */
	xx = mtmp->mx;
	yy = mtmp->my;
	if (!xx) {
	    /* no current location (migrating monster arrival) */
	    if (dndest.nlx && On_W_tower_level(&u.uz))
		return ((yy & 2) != 0) ^	/* inside xor not within */
		       !within_bounded_area(x, y, dndest.nlx, dndest.nly,
						  dndest.nhx, dndest.nhy);
	    if (updest.lx && (yy & 1) != 0)	/* moving up */
		return (within_bounded_area(x, y, updest.lx, updest.ly,
						  updest.hx, updest.hy) &&
		       (!updest.nlx ||
			!within_bounded_area(x, y, updest.nlx, updest.nly,
						   updest.nhx, updest.nhy)));
	    if (dndest.lx && (yy & 1) == 0)	/* moving down */
		return (within_bounded_area(x, y, dndest.lx, dndest.ly,
						  dndest.hx, dndest.hy) &&
		       (!dndest.nlx ||
			!within_bounded_area(x, y, dndest.nlx, dndest.nly,
						   dndest.nhx, dndest.nhy)));
	} else {
	    /* current location is <xx,yy> */
	    if (!tele_jump_ok(xx, yy, x, y)) return FALSE;
	}
	/* <x,y> is ok */
	return TRUE;
}

/*
 * rloc_to()
 *
 * Pulls a monster from its current position and places a monster at
 * a new x and y.  If oldx is 0, then the monster was not in the levels.monsters
 * array.  However, if oldx is 0, oldy may still have a value because mtmp is a
 * migrating_mon.  Worm tails are always placed randomly around the head of
 * the worm.
 */
void
rloc_to(mtmp, x, y)
struct monst *mtmp;
register int x, y;
{
	register int oldx = mtmp->mx, oldy = mtmp->my;
	boolean resident_shk = mtmp->isshk && inhishop(mtmp);

	if (x == mtmp->mx && y == mtmp->my)	/* that was easy */
		return;

	if (oldx) {				/* "pick up" monster */
	    if (mtmp->wormno)
		remove_worm(mtmp);
	    else {
		remove_monster(oldx, oldy);
		newsym(oldx, oldy);		/* update old location */
	    }
	}

	place_monster(mtmp, x, y);		/* put monster down */
	update_monster_region(mtmp);

	if (mtmp->wormno)			/* now put down tail */
		place_worm_tail_randomly(mtmp, x, y);

	if (u.ustuck == mtmp) {
		if (u.uswallow) {
			u.ux = x;
			u.uy = y;
			docrt();
		} else	u.ustuck = 0;
	}

	newsym(x, y);				/* update new location */
	set_apparxy(mtmp);			/* orient monster */

	/* shopkeepers will only teleport if you zap them with a wand of
	   teleportation or if they've been transformed into a jumpy monster;
	   the latter only happens if you've attacked them with polymorph */
	if (resident_shk && !inhishop(mtmp)) make_angry_shk(mtmp, oldx, oldy);
}

/* place a monster at a random location, typically due to teleport */
/* return TRUE if successful, FALSE if not */
boolean
rloc(mtmp, suppress_impossible)
struct monst *mtmp;	/* mx==0 implies migrating monster arrival */
boolean suppress_impossible;
{
	register int x, y, trycount;

#ifdef STEED
	if (mtmp == u.usteed) {
	    tele();
	    return TRUE;
	}
#endif

	if (mtmp->iswiz && mtmp->mx) {	/* Wizard, not just arriving */
	    if (!In_W_tower(u.ux, u.uy, &u.uz))
		x = xupstair,  y = yupstair;
	    else if (!xdnladder)	/* bottom level of tower */
		x = xupladder,  y = yupladder;
	    else
		x = xdnladder,  y = ydnladder;
	    /* if the wiz teleports away to heal, try the up staircase,
	       to block the player's escaping before he's healed
	       (deliberately use `goodpos' rather than `rloc_pos_ok' here) */
	    if (goodpos(x, y, mtmp, 0))
		goto found_xy;
	}

	trycount = 0;
	do {
	    x = rn1(COLNO-3,2);
	    y = rn2(ROWNO);
	    if ((trycount < 500) ? rloc_pos_ok(x, y, mtmp)
				 : goodpos(x, y, mtmp, 0))
		goto found_xy;
	} while (++trycount < 1000);

	/* last ditch attempt to find a good place */
	for (x = 2; x < COLNO - 1; x++)
	    for (y = 0; y < ROWNO; y++)
		if (goodpos(x, y, mtmp, 0))
		    goto found_xy;

	/* level either full of monsters or somehow faulty */
	if (!suppress_impossible)
		impossible("rloc(): couldn't relocate monster");
	return FALSE;

 found_xy:
	rloc_to(mtmp, x, y);
	return TRUE;
}

STATIC_OVL void
mvault_tele(mtmp)
struct monst *mtmp;
{
	register struct mkroom *croom = search_special(VAULT);
	coord c;

	if (croom && somexy(croom, &c) &&
				goodpos(c.x, c.y, mtmp, 0)) {
		rloc_to(mtmp, c.x, c.y);
		return;
	}
	(void) rloc(mtmp, TRUE);
}

boolean
tele_restrict(mon)
struct monst *mon;
{
	if (notel_level()) {
		if (canseemon(mon))
		    pline("A mysterious force prevents %s from teleporting!",
			mon_nam(mon));
		return TRUE;
	}
	return FALSE;
}

void
mtele_trap(mtmp, trap, in_sight)
struct monst *mtmp;
struct trap *trap;
int in_sight;
{
	char *monname;

	if (tele_restrict(mtmp)) return;
	else if(resists_magm(mtmp)){
		shieldeff(mtmp->mx, mtmp->my);
		return;
	} else if (teleport_pet(mtmp, FALSE)) {
	    /* save name with pre-movement visibility */
	    monname = Monnam(mtmp);

	    /* Note: don't remove the trap if a vault.  Other-
	     * wise the monster will be stuck there, since
	     * the guard isn't going to come for it...
	     */
	    if (trap->once) mvault_tele(mtmp);
	    else (void) rloc(mtmp, TRUE);

	    if (in_sight) {
		if (canseemon(mtmp))
		    pline("%s seems disoriented.", monname);
		else
		    pline("%s suddenly disappears!", monname);
		seetrap(trap);
	    }
	}
}

/* return 0 if still on level, 3 if not */
int
mlevel_tele_trap(mtmp, trap, force_it, in_sight)
struct monst *mtmp;
struct trap *trap;
boolean force_it;
int in_sight;
{
	int tt = trap->ttyp;
	struct permonst *mptr = mtmp->data;

	if (mtmp == u.ustuck)	/* probably a vortex */
	    return 0;		/* temporary? kludge */
	else if(resists_magm(mtmp)){
		shieldeff(mtmp->mx, mtmp->my);
		return 0;
	} else if(Role_if(PM_ANACHRONONAUT) && tt == MAGIC_PORTAL 
		&& (In_quest(&u.uz) || In_quest(&trap->dst))
		&& !(In_quest(&u.uz) && In_quest(&trap->dst))
		&& stuck_in_time(mtmp)
	){
		shieldeff(mtmp->mx, mtmp->my);
		return 0;
	} else if (teleport_pet(mtmp, force_it)) {
	    d_level tolevel;
	    int migrate_typ = MIGR_RANDOM;

	    if ((tt == HOLE || tt == TRAPDOOR)) {
		if (Is_stronghold(&u.uz)) {
		    assign_level(&tolevel, &valley_level);
		} else if (Is_botlevel(&u.uz)) {
		    if (in_sight && trap->tseen)
			pline("%s avoids the %s.", Monnam(mtmp),
			(tt == HOLE) ? "hole" : "trap");
		    return 0;
		} else {
		    get_level(&tolevel, depth(&u.uz) + 1);
		}
	    } else if (tt == MAGIC_PORTAL) {
		if (In_endgame(&u.uz) &&
		    (mon_has_amulet(mtmp) || is_home_elemental(mptr))) {
		    if (in_sight && mptr->mlet != S_ELEMENTAL) {
			pline("%s seems to shimmer for a moment.",
							Monnam(mtmp));
			seetrap(trap);
		    }
		    return 0;
		} else if(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON) && flags.initgend && In_quest(&u.uz)){
			int i;
			extern int n_dgns; /* from dungeon.c */
			//Look for return info
			for (i = 0; i < n_dgns; i++)
				if(dungeons[i].dunlev_ureturn)
					break;
			if(i < n_dgns){
				tolevel.dnum = i;
				tolevel.dlevel = dungeons[i].dunlev_ureturn;
			} else {
				assign_level(&tolevel, &trap->dst);
				migrate_typ = MIGR_PORTAL;
			}
		} else {
		    assign_level(&tolevel, &trap->dst);
		    migrate_typ = MIGR_PORTAL;
		}
	    } else { /* (tt == LEVEL_TELEP) */
		int nlev;

		if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
		    if (in_sight)
			pline("%s seems very disoriented for a moment.",
				Monnam(mtmp));
		    return 0;
		}
		nlev = random_teleport_level();
		if (nlev == depth(&u.uz)) {
		    if (in_sight)
			pline("%s shudders for a moment.", Monnam(mtmp));
		    return 0;
		}
		get_level(&tolevel, nlev);
	    }

	    if (in_sight) {
		pline("Suddenly, %s disappears out of sight.", mon_nam(mtmp));
		seetrap(trap);
	    }
	    migrate_to_level(mtmp, ledger_no(&tolevel),
			     migrate_typ, (coord *)0);
	    return 3;	/* no longer on this level */
	}
	return 0;
}


void
rlocos_at(x, y)
int x, y;
{
	int try_limit = 4000;
	struct obj *lobj = 0;
	
	while(level.objects[x][y] && (lobj != level.objects[x][y] || try_limit-- > 0))
		rloco(level.objects[x][y]);
}

void
rloco(obj)
struct obj *obj;
{
	if (obj->otyp == CORPSE && is_rider(&mons[obj->corpsenm])) {
	    if (revive_corpse(obj, REVIVE_MONSTER)) return;
	}
	if (obj->otyp == MAGIC_CHEST && obj->obolted)
		return;

	obj_extract_self(obj);
	randomly_place_obj(obj);
}

void
randomly_place_obj(obj)
struct obj *obj;
{
	xchar tx, ty, otx, oty;
	boolean restricted_fall;
	int try_limit = 4000;

	if(obj->where != OBJ_FREE){
		impossible("obj not free in randomly_place_obj! (no action taken)");
		return;
	}
	otx = obj->ox;
	oty = obj->oy;
	restricted_fall = (otx == 0 && dndest.lx);
	do {
	    tx = rn1(COLNO-3,2);
	    ty = rn2(ROWNO);
	    if (!--try_limit) break;
	} while (!goodpos(tx, ty, (struct monst *)0, 0) ||
		/* bug: this lacks provision for handling the Wizard's tower */
		 (restricted_fall &&
		  (!within_bounded_area(tx, ty, dndest.lx, dndest.ly,
						dndest.hx, dndest.hy) ||
		   (dndest.nlx &&
		    within_bounded_area(tx, ty, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy)))));

	if (flooreffects(obj, tx, ty, "fall")) {
	    return;
	} else if (otx == 0 && oty == 0) {
	    ;	/* fell through a trap door; no update of old loc needed */
	} else {
	    if (costly_spot(otx, oty)
	      && (!costly_spot(tx, ty) ||
		  !index(in_rooms(tx, ty, 0), *in_rooms(otx, oty, 0)))) {
		if (costly_spot(u.ux, u.uy) &&
			    index(u.urooms, *in_rooms(otx, oty, 0)))
		    addtobill(obj, FALSE, FALSE, FALSE);
		else (void)stolen_value(obj, otx, oty, FALSE, FALSE);
	    }
	    newsym(otx, oty);	/* update old location */
	}
	place_object(obj, tx, ty);
	newsym(tx, ty);
}

/* Returns an absolute depth */
int
random_teleport_level()
{
	int nlev, max_depth, min_depth,
	    cur_depth = (int)depth(&u.uz);

	/* enter from bottom going up*/
	boolean inverted = (dungeons[u.uz.dnum].entry_lev == dungeons[u.uz.dnum].num_dunlevs);

	if (!rn2(5) || Is_knox(&u.uz))
	    return cur_depth;

	/* What I really want to do is as follows:
	 * -- If in a dungeon that goes down, the new level is to be restricted
	 *    to [top of parent, bottom of current dungeon]
	 * -- If in a dungeon that goes up, the new level is to be restricted
	 *    to [top of current dungeon, bottom of parent]
	 * -- If in a quest dungeon or similar dungeon entered by portals,
	 *    the new level is to be restricted to [top of current dungeon,
	 *    bottom of current dungeon]
	 * The current behavior is not as sophisticated as that ideal, but is
	 * still better what we used to do, which was like this for players
	 * but different for monsters for no obvious reason.  Currently, we
	 * must explicitly check for special dungeons.  We check for Knox
	 * above; endgame is handled in the caller due to its different
	 * message ("disoriented").
	 * --KAA
	 * 3.4.2: explicitly handle quest here too, to fix the problem of
	 * monsters sometimes level teleporting out of it into main dungeon.
	 * Also prevent monsters reaching the Sanctum prior to invocation.
	 */
	min_depth = (In_quest(&u.uz) || In_law(&u.uz) || In_neu(&u.uz) || In_cha(&u.uz)) 
		? dungeons[u.uz.dnum].depth_start : 1;
	max_depth = dunlevs_in_dungeon(&u.uz) +
			(dungeons[u.uz.dnum].depth_start - 1);
	/* can't reach the Sanctum OR square level if the invocation hasn't been performed */
	if (Inhell && !u.uevent.invoked) max_depth -= 2;

	/* Get a random value relative to the current dungeon */
	if (!inverted) {
		/* Range is min_depth to current+3, current not counting */
		nlev = rn2(cur_depth + 3 - min_depth) + min_depth;
		if (nlev >= cur_depth) nlev++;
	}
	else {
		/* Range is current-3 to max_depth, current not counting */
		nlev = max_depth - rn2(max_depth - cur_depth + 3);
		if (nlev <= cur_depth) nlev--;
	}

	if (nlev > max_depth) {
	    nlev = max_depth;
	    /* teleport up if already on bottom */
	    if (Is_botlevel(&u.uz)) nlev -= rnd(3);
	}
	if (nlev < min_depth) {
	    nlev = min_depth;
	    if (nlev == cur_depth) {
	        nlev += rnd(3);
	        if (nlev > max_depth)
		    nlev = max_depth;
	    }
	}
	return nlev;
}

/* you teleport a monster (via wand, spell, or poly'd q.mechanic attack);
   return false iff the attempt fails */
boolean
u_teleport_mon(mtmp, give_feedback)
struct monst *mtmp;
boolean give_feedback;
{
	coord cc;
	int illrgrd = FALSE;//teleport will be fatal to target due to ill-regard equipment)
	
	if(mtmp->mtyp == PM_CREATURE_IN_THE_ICE){
	    if (give_feedback){
			shieldeff(mtmp->mx, mtmp->my);
			pline("Your magic has no effect on %s!", mon_nam(mtmp));
		}
		return FALSE;
	}
	
	if(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP){
		illrgrd = TRUE;
	}

	if (mtmp->ispriest && *in_rooms(mtmp->mx, mtmp->my, TEMPLE)) {
	    if (give_feedback)
		pline("%s resists your magic!", Monnam(mtmp));
	    return FALSE;
	} else if (notel_level() && u.uswallow && mtmp == u.ustuck) {
	    if (give_feedback)
		You("are no longer inside %s!", mon_nam(mtmp));
	    unstuck(mtmp);
	    (void) rloc(mtmp, FALSE);
	} else if ((is_rider(mtmp->data) || (mon_resistance(mtmp, TELEPORT_CONTROL) && !mtmp->mflee))
		&& rn2(13) && enexto(&cc, u.ux, u.uy, mtmp->data)
	){
		if(illrgrd && canspotmon(mtmp)) pline("%s vanishes out of the equipment that imprisons %s.", Monnam(mtmp), himherit(mtmp));
	    rloc_to(mtmp, cc.x, cc.y);
	} else {
		if(illrgrd && canspotmon(mtmp)) pline("%s vanishes out of the equipment that imprisons %s.", Monnam(mtmp), himherit(mtmp));
	    (void) rloc(mtmp, FALSE);
	}
	
	if(illrgrd){
		if(Is_illregrd(&u.uz)){
			u.uevent.uaxus_foe = 1;
			pline("An alarm sounds!");
			aggravate();
		}
		if(!DEADMONSTER(mtmp)){
			if(canspotmon(mtmp)) pline("Unfortunately, that equipment was the only thing keeping %s %s.", himherit(mtmp), nonliving(mtmp->data) ? "intact" : "alive");
			mondied(mtmp);
		}
	}
	return TRUE;
}

/*teleport.c*/
