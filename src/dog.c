/*	SCCS Id: @(#)dog.c	3.4	2002/09/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVLB

STATIC_DCL int NDECL(pet_type);

void
initedog(mtmp)
register struct monst *mtmp;
{
	if(!mtmp->mextra_p || !mtmp->mextra_p->edog_p)
		add_mx(mtmp, MX_EDOG);

	if(mtmp->mtyp == PM_SURYA_DEVA){
		struct monst *blade;
		for(blade = fmon; blade; blade = blade->nmon) if(blade->mtyp == PM_DANCING_BLADE && mtmp->m_id == blade->mvar_suryaID) break;
		if(blade && !blade->mtame) tamedog(blade, (struct obj *) 0);
	}
	
	mtmp->mtame = is_domestic(mtmp->data) ? 10 : 5;
	mtmp->mpeaceful = 1;
	mtmp->mavenge = 0;
	set_malign(mtmp); /* recalc alignment now that it's tamed */
	mtmp->mleashed = 0;
	mtmp->meating = 0;
	EDOG(mtmp)->droptime = 0;
	EDOG(mtmp)->dropdist = 10000;
	EDOG(mtmp)->apport = ACURR(A_CHA);
	EDOG(mtmp)->whistletime = 0;
	EDOG(mtmp)->hungrytime = 1000 + monstermoves;
	EDOG(mtmp)->ogoal.x = -1;	/* force error if used before set */
	EDOG(mtmp)->ogoal.y = -1;
	EDOG(mtmp)->abuse = 0;
	EDOG(mtmp)->revivals = 0;
	EDOG(mtmp)->mhpmax_penalty = 0;
	EDOG(mtmp)->killed_by_u = 0;
//ifdef BARD
	EDOG(mtmp)->friend = 0;
	EDOG(mtmp)->waspeaceful = 0;
//endif
	EDOG(mtmp)->loyal = 0;
	EDOG(mtmp)->dominated = 0;

	if (isok(mtmp->mx, mtmp->my))
		newsym(mtmp->mx, mtmp->my);
}

STATIC_OVL int
pet_type()
{
	if(Role_if(PM_MADMAN)){
		return PM_SECRET_WHISPERER;
	}
	if(Role_if(PM_ANACHRONOUNBINDER))
	    return (urole.petnum);
	else if(Role_if(PM_UNDEAD_HUNTER) && philosophy_index(u.ualign.god)){
		if(u.ualign.god == GOD_THE_COLLEGE){
			return PM_FLOATING_EYE;
		}
		else if(u.ualign.god == GOD_THE_CHOIR){
			return PM_PARASITIC_MIND_FLAYER;
		}
		else { //Defilement
			return PM_CENTIPEDE;
		}
	}
	else if(Race_if(PM_DROW)){
		if (Role_if(PM_HEALER)){
			return (PM_KNIGHT);
		}
		if(Role_if(PM_NOBLEMAN)){
			if(flags.initgend) return (PM_GIANT_SPIDER);
			else return (PM_SMALL_CAVE_LIZARD);
		}
		if(preferred_pet == 's')
			return (PM_CAVE_SPIDER);
		else if(preferred_pet == ':')
			return (PM_BABY_CAVE_LIZARD);
		else 
			return (rn2(3) ? PM_CAVE_SPIDER : PM_BABY_CAVE_LIZARD);
	}
	else if(Race_if(PM_HALF_DRAGON) && Role_if(PM_NOBLEMAN) && flags.initgend){
		return (PM_UNDEAD_KNIGHT);
		// if(flags.initgend) 
		// else 
	} else if(Race_if(PM_CHIROPTERAN))
		return PM_GIANT_BAT;
	else if (urole.petnum != NON_PM && urole.petnum != PM_LITTLE_DOG && urole.petnum != PM_KITTEN && urole.petnum != PM_PONY)
	    return (urole.petnum);
	else if(Race_if(PM_HALF_DRAGON))
		return urole.petnum == PM_PONY ? PM_RIDING_PSEUDODRAGON : PM_TINY_PSEUDODRAGON;
	else if(Race_if(PM_SALAMANDER))
		return PM_FIRE_SNAKE;
	else if (Role_if(PM_PIRATE)) {
		if (preferred_pet == 'B')
			return (PM_PARROT);
		else if(preferred_pet == 'Y')
			return PM_MONKEY;
		else
			return (rn2(2) ? PM_PARROT : PM_MONKEY);
	}
	else if (preferred_pet == 'c' || preferred_pet == 'f')
	    return (PM_KITTEN);
	else if (preferred_pet == 'd')
	    return (PM_LITTLE_DOG);
	else if (urole.petnum != NON_PM)
	    return (urole.petnum);
	else
	    return (rn2(2) ? PM_KITTEN : PM_LITTLE_DOG);
}

struct monst *
make_familiar(otmp,x,y,quietly)
register struct obj *otmp;
xchar x, y;
boolean quietly;
{
	struct permonst *pm;
	struct monst *mtmp = 0;
	int chance, trycnt = 100;
	int mmflags = MM_EDOG|MM_IGNOREWATER;


	do {
	    if (otmp) {	/* figurine; otherwise spell */
			int mndx = otmp->corpsenm;
			pm = &mons[mndx];
			// No tame uniqs or nowish creatures
			if ((pm->geno & G_UNIQ) || is_unwishable(pm)){
				pline_The("figurine warps strangely!");
				pm = rndmonst(0,0);
			}
			/* activating a figurine provides one way to exceed the
			   maximum number of the target critter created--unless
			   it has a special limit (erinys, Nazgul) */
			if ((mvitals[mndx].mvflags & G_EXTINCT) && mbirth_limit(mndx) < MAXMONNO) {
				/* just printed "You <place> the figurine and it transforms." */
				if (!quietly)
					pline("... into a pile of dust.");
				break;	/* mtmp is null */
			}
			if(otmp->spe&FIGURINE_MALE){
				mmflags |= MM_MALE;
			}
			if(otmp->spe&FIGURINE_FEMALE){
				mmflags |= MM_FEMALE;
			}
	    } else if (!rn2(3)) {
			pm = &mons[pet_type()];
	    } else {
			pm = rndmonst(0,0);
			if (!pm) {
			  if (!quietly)
				There("seems to be nothing available for a familiar.");
			  break;
			}
			if(stationary(pm) || sessile(pm))
				continue;
		}

		mtmp = makemon(pm, x, y, mmflags);
		if (otmp && !mtmp) { /* monster was genocided or square occupied */
			if (!quietly)
			   pline_The("figurine writhes and then shatters into pieces!");
			break;
	    }
	} while (!mtmp && --trycnt > 0);

	if (!mtmp) return (struct monst *)0;

	if (is_pool(mtmp->mx, mtmp->my, FALSE) && minliquid(mtmp))
		return (struct monst *)0;

	initedog(mtmp);
	mtmp->msleeping = 0;
	if (otmp) { /* figurine; resulting monster might not become a pet */
		if(check_fig_template(otmp->spe, FIGURINE_PSEUDO)){
			set_template(mtmp, PSEUDONATURAL);
		}

		if(otmp->spe&FIGURINE_LOYAL){
			EDOG(mtmp)->loyal = TRUE;
		}
		else {
			chance = rn2(10);	/* 0==tame, 1==peaceful, 2==hostile */
			if (chance > 2) chance = otmp->blessed ? 0 : !otmp->cursed ? 1 : 2;
			/* 0,1,2:  b=80%,10,10; nc=10%,80,10; c=10%,10,80 */
			if (chance > 0 && 
				!(Role_if(PM_BARD) && rnd(20) < ACURR(A_CHA) && 
					!(is_animal(mtmp->data) || mindless_mon(mtmp)))
			){
				untame(mtmp, 1);	/* not tame after all */
				if (chance == 2) { /* hostile (cursed figurine) */
					if (!quietly)
					   You("get a bad feeling about this.");
					mtmp->mpeaceful = 0;
					set_malign(mtmp);
				}
			}
		}
	    /* if figurine has been named, give same name to the monster */
	    if (get_ox(otmp, OX_ENAM))
			mtmp = christen_monst(mtmp, ONAME(otmp));
	}
	set_malign(mtmp); /* more alignment changes */
	newsym(mtmp->mx, mtmp->my);

	/* must wield weapon immediately since pets will otherwise drop it */
	if (mtmp->mtame && mon_attacktype(mtmp, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
	return mtmp;
}

struct monst *
make_mad_eth(){
	struct monst *mtmp;
	mtmp = makemon(&mons[PM_ETHEREALOID], u.ux, u.uy, MM_ADJACENTOK|MM_EDOG);
	if(!mtmp) return (struct monst *) 0;
	mtmp = christen_monst(mtmp, plname);
	return mtmp;
	
	
}


struct monst *
makedog()
{
	register struct monst *mtmp;
#ifdef STEED
	register struct obj *otmp;
#endif
	const char *petname;
	int   pettype;
	static int petname_used = 0;
	if(Role_if(PM_MADMAN) && Race_if(PM_ETHEREALOID)) return make_mad_eth();

	if (preferred_pet == 'n' || Role_if(PM_ANACHRONONAUT) || Role_if(PM_UNDEAD_HUNTER)) return((struct monst *) 0);

	pettype = pet_type();
	if (pettype == PM_LITTLE_DOG)
		petname = dogname;
	else if (pettype == PM_PONY)
		petname = horsename;
	else if (pettype == PM_PARROT)
		petname = parrotname;
	else if (pettype == PM_MONKEY)
		petname = monkeyname;
	else if (pettype == PM_SECRET_WHISPERER)
		petname = whisperername;
	else if(is_spider(&mons[pettype]))
		petname = spidername;
	else if(is_dragon(&mons[pettype]))
		petname = dragonname;
	else if(mons[pettype].mlet == S_LIZARD)
		petname = lizardname;
#ifdef CONVICT
	else if (pettype == PM_SEWER_RAT)
		petname = ratname;
#endif /* CONVICT */
	else
		petname = catname;

	/* default pet names */
	if (!*petname && pettype == PM_LITTLE_DOG) {
	    /* All of these names were for dogs. */
	    if(Role_if(PM_CAVEMAN)) petname = "Slasher";   /* The Warrior */
	    if(Role_if(PM_SAMURAI)) petname = "Hachi";     /* Shibuya Station */
	    if(Role_if(PM_BARBARIAN)) petname = "Idefix";  /* Obelix */
	    if(Role_if(PM_RANGER)) petname = "Sirius";     /* Orion's dog */
	}

	if (!*petname && pettype == PM_KITTEN) {
	    if(Role_if(PM_NOBLEMAN)) petname = "Puss";   /* Puss in Boots */
	    if(Role_if(PM_WIZARD) && flags.female) petname = rn2(2) ? "Crookshanks" : "Greebo";     /* Hermione and Nanny Ogg */
	    if(Role_if(PM_WIZARD) && !flags.female) petname = rn2(2) ? "Tom Kitten" : "Mister";     /* Beatrix Potter and Harry Dresden */
	}

#ifdef CONVICT
	if (!*petname && pettype == PM_SEWER_RAT) {
	    if(Role_if(PM_CONVICT)) petname = "Nicodemus"; /* Rats of NIMH */
    }
#endif /* CONVICT */
	mtmp = makemon(&mons[pettype], u.ux, u.uy, pettype == PM_SECRET_WHISPERER ? MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH|MM_EDOG|MM_ESUM : MM_ADJACENTOK|MM_EDOG);

	if(!mtmp) return((struct monst *) 0); /* pets were genocided */
	
	if(pettype == PM_SECRET_WHISPERER){
		mark_mon_as_summoned(mtmp, &youmonst, ACURR(A_CHA) + 1, 0);
		for(int i = min(45, (Insight - mtmp->m_lev)); i > 0; i--){
			grow_up(mtmp, (struct monst *) 0);
			//Technically might grow into a genocided form.
			if(DEADMONSTER(mtmp))
				return((struct monst *) 0);
		}
		mtmp->mspec_used = 0;
	}
	
	if(mtmp->m_lev < mtmp->data->mlevel) mtmp->m_lev = mtmp->data->mlevel;
	
	if(pettype == PM_KNIGHT){
		struct obj *obj;
		mtmp->m_lev = 1;
		mon_adjust_speed(mtmp, -1, (struct obj *) 0, FALSE);
		obj = mongets(mtmp, CRAM_RATION, MKOBJ_NOINIT);
		if(obj){
			obj->quan = 3;
			fix_object(obj);
			fully_identify_obj(obj);
		}
		obj = mongets(mtmp, FOOD_RATION, MKOBJ_NOINIT);
		if(obj){
			obj->quan = 1;
			fix_object(obj);
			fully_identify_obj(obj);
		}
		obj = mongets(mtmp, HIGH_BOOTS, MKOBJ_NOINIT);
		if(obj) fully_identify_obj(obj);
		obj = mongets(mtmp, CHAIN_MAIL, MKOBJ_NOINIT);
		if(obj) fully_identify_obj(obj);
		obj = mongets(mtmp, GAUNTLETS, MKOBJ_NOINIT);
		if(obj) fully_identify_obj(obj);
		obj = mongets(mtmp, AMULET_OF_NULLIFY_MAGIC, MKOBJ_NOINIT);
		fully_identify_obj(obj);
		obj = mongets(mtmp, HELMET, MKOBJ_NOINIT);
		if(obj) fully_identify_obj(obj);
		obj = mongets(mtmp, KITE_SHIELD, MKOBJ_NOINIT);
		if(obj) fully_identify_obj(obj);
		obj = mongets(mtmp, LONG_SWORD, MKOBJ_NOINIT);
		if(obj){
			bless(obj);
			obj->spe = 1;
			fully_identify_obj(obj);
		}
		m_dowear(mtmp, TRUE);
		init_mon_wield_item(mtmp);
	}

	if(Role_if(PM_HEALER)){
		grow_up(mtmp, (struct monst *) 0);
		//Technically might grow into a genocided form.
		if(DEADMONSTER(mtmp))
			return((struct monst *) 0);
	}
	
	if(mtmp->m_lev) mtmp->mhpmax = 8*(mtmp->m_lev-1)+rnd(8);
	mtmp->mhp = mtmp->mhpmax;

#ifdef STEED
	/* Horses already wear a saddle */
	if ((pettype == PM_PONY || pettype == PM_GIANT_SPIDER || pettype == PM_SMALL_CAVE_LIZARD || pettype == PM_RIDING_PSEUDODRAGON)
		&& !!(otmp = mksobj(SADDLE, 0))
	) {
	    if (mpickobj(mtmp, otmp))
		panic("merged saddle?");
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->dknown = otmp->bknown = otmp->rknown = 1;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE, TRUE);
	}
#endif

	if (!petname_used++ && *petname)
		mtmp = christen_monst(mtmp, petname);

	initedog(mtmp);
	EDOG(mtmp)->loyal = TRUE;
	EDOG(mtmp)->waspeaceful = TRUE;
	mtmp->mpeacetime = 0;
	if(is_half_dragon(mtmp->data) && flags.HDbreath){
		mtmp->mvar_hdBreath = flags.HDbreath;
		set_mon_data(mtmp, mtmp->mtyp);
	}
	return(mtmp);
}

/* record `last move time' for all monsters prior to level save so that
   mon_arrive() can catch up for lost time when they're restored later */
void
update_mlstmv()
{
	struct monst *mon;

	/* monst->mlstmv used to be updated every time `monst' actually moved,
	   but that is no longer the case so we just do a blanket assignment */
	for (mon = fmon; mon; mon = mon->nmon)
	    if (!DEADMONSTER(mon)) mon->mlstmv = monstermoves;
}

void
losedogs()
{
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;

	while ((mtmp = mydogs) != 0) {
		mydogs = mtmp->nmon;
		mon_arrive(mtmp, TRUE);
	}

	for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel 
			&& mtmp->m_insight_level <= Insight
		    && !(mtmp->mtyp == PM_WALKING_DELIRIUM && BlockableClearThoughts)
		    && !(mtmp->mtyp == PM_STRANGER && !quest_status.touched_artifact)
		    && !((mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI) && mtmp->mvar_yellow_lifesaved)
		    && !(mtmp->mtyp == PM_TWIN_SIBLING && (mtmp->mvar_twin_lifesaved || !(u.specialSealsActive&SEAL_YOG_SOTHOTH)))
		) {
			mon_extract_from_list(mtmp, &migrating_mons);
		    mon_arrive(mtmp, FALSE);
		} else
		    mtmp0 = mtmp;
	}
}

void
mon_extract_from_list(mon, list_head)
struct monst *mon;
struct monst **list_head;
{
	struct monst *mtmp, *mtmp2, *mtmp0 = 0;
	for(mtmp = *list_head; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (mtmp == mon) {
			if(!mtmp0)
				*list_head = mtmp->nmon;
			else
				mtmp0->nmon = mtmp->nmon;
			break;
		} else
			mtmp0 = mtmp;
	}
}

void
mon_arrive_on_level(mon)
struct monst *mon;
{
	if(wizard) pline("arriving");
	mon_extract_from_list(mon, &migrating_mons);
	mon_arrive(mon, FALSE);
}

/* called from resurrect() in addition to losedogs() */
void
mon_arrive(mtmp, with_you)
struct monst *mtmp;
boolean with_you;
{
	struct trap *t;
	xchar xlocale, ylocale, xyloc, xyflags, wander;
	int num_segs;

	mtmp->nmon = fmon;
	mtmp->marriving = FALSE;
	fmon = mtmp;
	if (mtmp->isshk)
	    set_residency(mtmp, FALSE);
	if(mtmp->m_ap_type == M_AP_MONSTER && (BlockableClearThoughts || (!mtmp->iswiz && !(u.umadness&MAD_DELUSIONS)))){
		mtmp->m_ap_type = M_AP_NOTHING;
		mtmp->mappearance = 0;
	}
	num_segs = mtmp->wormno;
	/* baby long worms have no tail so don't use is_longworm() */
	if ((mtmp->mtyp == PM_LONG_WORM) &&
#ifdef DCC30_BUG
	    (mtmp->wormno = get_wormno(), mtmp->wormno != 0))
#else
	    (mtmp->wormno = get_wormno()) != 0)
#endif
	{
	    initworm(mtmp, num_segs);
	    /* tail segs are not yet initialized or displayed */
	} else mtmp->wormno = 0;

	/* some monsters might need to do something special upon arrival
	   _after_ the current level has been fully set up; see dochug() */
	mtmp->mstrategy |= STRAT_ARRIVE;

	/* make sure mnexto(rloc_to(set_apparxy())) doesn't use stale data */
	mtmp->mux = 0,  mtmp->muy = 0;
	xyloc	= mtmp->mtrack[0].x;
	xyflags = mtmp->mtrack[0].y;
	xlocale = mtmp->mtrack[1].x;
	ylocale = mtmp->mtrack[1].y;
	mtmp->mtrack[0].x = mtmp->mtrack[0].y = 0;
	mtmp->mtrack[1].x = mtmp->mtrack[1].y = 0;

#ifdef STEED
	if (mtmp == u.usteed)
	    return;	/* don't place steed on the map */
#endif
	if (with_you) {
	    /* When a monster accompanies you, sometimes it will arrive
	       at your intended destination and you'll end up next to
	       that spot.  This code doesn't control the final outcome;
	       goto_level(do.c) decides who ends up at your target spot
	       when there is a monster there too. */
	    if (!MON_AT(u.ux, u.uy) &&
		    !rn2(mtmp->mtame ? 10 : mtmp->mpeaceful ? 5 : 2)
		)
			rloc_to(mtmp, u.ux, u.uy);
	    else
			mnexto(mtmp);
		if(mtmp->mx == 0){
			if(wizard) pline("no room for buddy! Arriving later.");
			/*No room! place it once there's space.*/
			migrate_to_level(mtmp, ledger_no(&u.uz),
					 MIGR_RANDOM, (coord *)0);
			mtmp->marriving = TRUE;
			return;
		}
	    return;
	}
	/*
	 * The monster arrived on this level independently of the player.
	 * Its coordinate fields were overloaded for use as flags that
	 * specify its final destination.
	 */
	if (mtmp->mlstmv < monstermoves - 1L) {
	    /* heal monster for time spent in limbo */
	    long nmv = monstermoves - 1L - mtmp->mlstmv;

	    mon_catchup_elapsed_time(mtmp, nmv);
	    mtmp->mlstmv = monstermoves - 1L;

	    /* let monster move a bit on new level (see placement code below) */
	    wander = (xchar) min(nmv, 8);
	} else
	    wander = 0;

	switch (xyloc) {
	 case MIGR_APPROX_XY:	/* {x,y}locale set above */
		break;
	 case MIGR_EXACT_XY:	wander = 0;
		break;
	 case MIGR_NEAR_PLAYER:	xlocale = u.ux,  ylocale = u.uy;
		break;
	 case MIGR_STAIRS_UP:	xlocale = xupstair,  ylocale = yupstair;
		break;
	 case MIGR_STAIRS_DOWN:	xlocale = xdnstair,  ylocale = ydnstair;
		break;
	 case MIGR_LADDER_UP:	xlocale = xupladder,  ylocale = yupladder;
		break;
	 case MIGR_LADDER_DOWN:	xlocale = xdnladder,  ylocale = ydnladder;
		break;
	 case MIGR_SSTAIRS:	xlocale = sstairs.sx,  ylocale = sstairs.sy;
		break;
	 case MIGR_PORTAL:
		if (In_endgame(&u.uz)) {
		    /* there is no arrival portal for endgame levels */
		    /* BUG[?]: for simplicity, this code relies on the fact
		       that we know that the current endgame levels always
		       build upwards and never have any exclusion subregion
		       inside their TELEPORT_REGION settings. */
		    xlocale = rn1(updest.hx - updest.lx + 1, updest.lx);
		    ylocale = rn1(updest.hy - updest.ly + 1, updest.ly);
		    break;
		}
		/* find the arrival portal */
		for (t = ftrap; t; t = t->ntrap)
		    if (t->ttyp == MAGIC_PORTAL && t->dst.dlevel == mtmp->mtrack[2].x && t->dst.dnum == mtmp->mtrack[2].y) break;
		if (t) {
		    xlocale = t->tx,  ylocale = t->ty;
		    break;
		} else {
			//Note: thanks to the Female Half-Dragon Noble quest, this is no longer impossible
		    // impossible("mon_arrive: no corresponding portal?");
		} /*FALLTHRU*/
	 default:
	 case MIGR_RANDOM:	xlocale = ylocale = 0;
		    break;
	    }

	if (xlocale && wander) {
	    /* monster moved a bit; pick a nearby location */
	    /* mnearto() deals w/stone, et al */
	    char *r = in_rooms(xlocale, ylocale, 0);
	    if (r && *r) {
		coord c;
		/* somexy() handles irregular rooms */
		if (somexy(&rooms[*r - ROOMOFFSET], &c))
		    xlocale = c.x,  ylocale = c.y;
		else
		    xlocale = ylocale = 0;
	    } else {		/* not in a room */
		int i, j;
		i = max(1, xlocale - wander);
		j = min(COLNO-1, xlocale + wander);
		xlocale = rn1(j - i, i);
		i = max(0, ylocale - wander);
		j = min(ROWNO-1, ylocale + wander);
		ylocale = rn1(j - i, i);
	    }
	}	/* moved a bit */

	mtmp->mx = 0;	/*(already is 0)*/
	mtmp->my = xyflags;
	if (xlocale){
	    (void) mnearto(mtmp, xlocale, ylocale, FALSE);
		if(mtmp->mx == 0){
			if(wizard) pline("no room at local! Qrriving later.");
			/*No room! place it once there's space.*/
			migrate_to_level(mtmp, ledger_no(&u.uz),
					 MIGR_RANDOM, (coord *)0);
			mtmp->marriving = TRUE;
			return;
		}
	}
	else {
		if (!rloc(mtmp,TRUE)) {
			if(wizard) pline("no room! arriving later.");
			/*No room! place it once there's space.*/
			migrate_to_level(mtmp, ledger_no(&u.uz),
					 MIGR_RANDOM, (coord *)0);
			mtmp->marriving = TRUE;
			return;
		}
	}
	/* now that it's placed, we can resume timers (which may kill mtmp) */
	receive_timers(mtmp->timed);
}

/* heal monster for time spent elsewhere */
void
mon_catchup_elapsed_time(mtmp, nmv)
struct monst *mtmp;
long nmv;		/* number of moves */
{
	int imv = 0;	/* avoid zillions of casts and lint warnings */
	if(mtmp->m_ap_type == M_AP_MONSTER && (BlockableClearThoughts || (!mtmp->iswiz && !(u.umadness&MAD_DELUSIONS)))){
		mtmp->m_ap_type = M_AP_NOTHING;
		mtmp->mappearance = 0;
		newsym(mtmp->mx, mtmp->my);
	}
	if(imprisoned(mtmp)) return;

#if defined(DEBUG) || defined(BETA)
	if (nmv < 0L) {			/* crash likely... */
	    panic("catchup from future time?");
	    /*NOTREACHED*/
	    return;
	} else if (nmv == 0L) {		/* safe, but should'nt happen */
	    impossible("catchup from now?");
	} else
#endif
	if (nmv >= LARGEST_INT)		/* paranoia */
	    imv = LARGEST_INT - 1;
	else
	    imv = (int)nmv;

	if(noactions(mtmp)){
		int i;
		for(i = imv; i > 0 && mtmp->entangled_oid; i--){
			mbreak_entanglement(mtmp);
			mescape_entanglement(mtmp);
		}
	}
	
	/* might stop being afraid, blind or frozen */
	/* set to 1 and allow final decrement in movemon() */
	if (mtmp->mblinded) {
	    if (imv >= (int) mtmp->mblinded) mtmp->mblinded = 1;
	    else mtmp->mblinded -= imv;
	}
	if (mtmp->mfrozen) {
	    if (imv >= (int) mtmp->mfrozen) mtmp->mfrozen = 1;
	    else mtmp->mfrozen -= imv;
	}
	if (mtmp->mequipping) {
	    if (imv >= (int) mtmp->mequipping) mtmp->mequipping = 1;
	    else mtmp->mequipping -= imv;
	}
	if (mtmp->mfleetim) {
	    if (imv >= (int) mtmp->mfleetim) mtmp->mfleetim = 1;
	    else mtmp->mfleetim -= imv;
	}

	/* might recover from temporary trouble */
	if (mtmp->mtrapped && rn2(imv + 1) > 40/2) mtmp->mtrapped = 0;
	if (mtmp->mconf    && rn2(imv + 1) > 50/2) mtmp->mconf = 0;
	if (mtmp->mstun    && rn2(imv + 1) > 10/2) mtmp->mstun = 0;

	/* might finish eating or be able to use special ability again */
	if (imv > mtmp->meating) mtmp->meating = 0;
	else mtmp->meating -= imv;
	if (imv > mtmp->mspec_used) mtmp->mspec_used = 0;
	else mtmp->mspec_used -= imv;

	/* reduce tameness for every 150 moves you are separated */
	if (get_mx(mtmp, MX_EDOG) && !(EDOG(mtmp)->loyal) && !(EDOG(mtmp)->dominated)
	 && !(Race_if(PM_VAMPIRE) && is_vampire(mtmp->data))
	 && !Is_town_level(&u.uz)
	) {
	    int wilder = (imv + 75) / 150;
		if(mtmp->mwait && !EDOG(mtmp)->friend) wilder = max(0, wilder - 11);
		if(P_SKILL(P_BEAST_MASTERY) > 1 && !EDOG(mtmp)->friend) wilder = max(0, wilder - (3*(P_SKILL(P_BEAST_MASTERY)-1) + 1));
#ifdef BARD
	    /* monsters under influence of Friendship song go wilder faster */
	    if (EDOG(mtmp)->friend)
		    wilder *= 150;
#endif
	    if (mtmp->mtame > wilder) mtmp->mtame -= wilder;	/* less tame */
	    else if (mtmp->mtame > rn2(wilder)) untame(mtmp, 1);  /* untame, peaceful */
	    else{
			untame(mtmp, 0);		/* hostile! */
			mtmp->mferal = 1;
		}
	}
	/* check to see if it would have died as a pet; if so, go wild instead
	 * of dying the next time we call dog_move()
	 */
	if (get_mx(mtmp, MX_EDOG) && (carnivorous(mtmp->data) || herbivorous(mtmp->data))){
	    struct edog *edog = EDOG(mtmp);
		if(!Is_town_level(&u.uz)) {
			if ((monstermoves > edog->hungrytime + 500 && mtmp->mhp < 3) ||
				(monstermoves > edog->hungrytime + 750)
			){
				mtmp->mtame = mtmp->mpeaceful = 0;		/* hostile! */
				mtmp->mferal = 1;
			}
		} else {
			if(edog->hungrytime < monstermoves + 500)
				edog->hungrytime = monstermoves + 500;
		}
	}

	if (!mtmp->mtame && mtmp->mleashed) {
	    /* leashed monsters should always be with hero, consequently
	       never losing any time to be accounted for later */
	    impossible("catching up for leashed monster?");
	    m_unleash(mtmp, FALSE);
	}

	/* recover lost hit points */
	if (mon_resistance(mtmp,REGENERATION)){
		if (mtmp->mhp + imv >= mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		else mtmp->mhp += imv;
	}
	if(!nonliving(mtmp->data)){
		imv = imv*(mtmp->m_lev + ACURR_MON(A_CHA, mtmp))/30;
		if (mtmp->mhp + imv >= mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		else mtmp->mhp += imv;
	}
}

#endif /* OVLB */
#ifdef OVL2

/* called when you move to another level */
void
keepdogs(pets_only, newlev, portal)
boolean pets_only;	/* true for ascension or final escape */
d_level *newlev;
boolean portal;
{
	register struct monst *mtmp, *mtmp2;
	register struct obj *obj;
	int num_segs;
	boolean stay_behind;
	boolean all_pets = FALSE;
	int pet_dist = P_SKILL(P_BEAST_MASTERY);
	int follow_dist;
	if(pet_dist < 1)
		pet_dist = 1;
	if(uwep && uwep->otyp == SHEPHERD_S_CROOK)
		pet_dist++;
	if(u.specialSealsActive&SEAL_COSMOS ||
		(uarmh && uarmh->oartifact == ART_CROWN_OF_THE_SAINT_KING) ||
		(uarmh && uarmh->oartifact == ART_HELM_OF_THE_DARK_LORD)
	) all_pets = TRUE;
	
	if(!all_pets) for(obj = invent; obj; obj=obj->nobj)
		if(obj->oartifact == ART_LYRE_OF_ORPHEUS){
			all_pets = TRUE;
			break;
		}
	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (DEADMONSTER(mtmp)) continue;
	    if (pets_only && !mtmp->mtame) continue;
		follow_dist = pet_dist;
		if(Race_if(PM_VAMPIRE)){
			if(is_vampire(mtmp->data)){
				follow_dist++;
				if(check_vampire(VAMPIRE_MASTERY))
					follow_dist++;
			}
			if(is_undead(mtmp->data)){
				follow_dist++;
			}
		}
	    if (((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp)) || 
			(mtmp->mtame && (all_pets ||
							(distmin(mtmp->mx, mtmp->my, u.ux, u.uy) <= follow_dist)
							|| (get_mx(mtmp, MX_EDOG) && EDOG(mtmp)->dominated) //Crystal skull monsters always follow
							// || (u.sealsActive&SEAL_MALPHAS && mtmp->mtyp == PM_CROW) //Allow distant crows to get left behind.
							)
							&& !(get_mx(mtmp, MX_ESUM) && !mtmp->mextra_p->esum_p->sticky)	// cannot be a summon marked as not-a-follower
			) ||
#ifdef STEED
			(mtmp == u.usteed) ||
#endif
		/* the wiz will level t-port from anywhere to chase
		   the amulet; if you don't have it, will chase you
		   only if in range. -3. */
			(u.uhave.amulet && mtmp->iswiz)
		/* All lurking hands follow between levels */
			|| (mtmp->mtyp == PM_LURKING_HAND || mtmp->mtyp == PM_BLASPHEMOUS_HAND)
		)
		&& ((!mtmp->msleeping && mtmp->mcanmove)
#ifdef STEED
		    /* eg if level teleport or new trap, steed has no control
		       to avoid following */
		    || (mtmp == u.usteed)
#endif
		    )
		/* monster won't follow if it hasn't noticed you yet */
		&& !(mtmp->mstrategy & STRAT_WAITFORU)) {
			stay_behind = FALSE;
			if (mtmp->mtame && mtmp->mwait && u.usteed != mtmp && (mtmp->mwait+100 > monstermoves)) {
				if (canspotmon(mtmp))
					pline("%s obediently waits for you to return.", Monnam(mtmp));
				stay_behind = TRUE;
			} else if (mon_has_amulet(mtmp)) {
				if (canspotmon(mtmp))
					pline("%s seems very disoriented for a moment.",
					Monnam(mtmp));
				stay_behind = TRUE;
			} else if (Role_if(PM_ANACHRONONAUT) && newlev 
				&& (In_quest(&u.uz) || In_quest(newlev))
				&& !(In_quest(&u.uz) && In_quest(newlev))
				&& stuck_in_time(mtmp)
			) {
				if (canspotmon(mtmp)){
					if(portal) pline("%s is unable to touch the portal!", Monnam(mtmp));
					else pline("%s seems very disoriented for a moment.", Monnam(mtmp));
				}
				stay_behind = TRUE;
			} else if (mtmp->mtame && (mtmp->mtrapped || mtmp->entangled_oid) && mtmp != u.usteed) {
				if (canspotmon(mtmp))
					pline("%s is still trapped.", Monnam(mtmp));
				stay_behind = TRUE;
			}
#ifdef STEED
			// if (mtmp == u.usteed) stay_behind = FALSE;
			if (mtmp == u.usteed && stay_behind) {
			    pline("%s vanishes from underneath you.", Monnam(mtmp));
				dismount_steed(DISMOUNT_VANISHED);
			}
#endif
			if (stay_behind) {
				if (mtmp->mleashed) {
					pline("%s leash suddenly comes loose.",
						humanoid(mtmp->data)
							? (mtmp->female ? "Her" : "His")
							: "Its");
					m_unleash(mtmp, FALSE);
				}
				if(mtmp->mtyp == PM_TWIN_SIBLING)
					migrate_to_level(mtmp, ledger_no(&u.uz), MIGR_EXACT_XY, (coord *)0);
				continue;
			}
			mtmp->mwait = 0L; //No longer waiting
			if (mtmp->isshk)
				set_residency(mtmp, TRUE);

			if (mtmp->wormno) {
				register int cnt;
				/* NOTE: worm is truncated to # segs = max wormno size */
				cnt = count_wsegs(mtmp);
				num_segs = min(cnt, MAX_NUM_WORMS - 1);
				wormgone(mtmp);
			} else num_segs = 0;

			/* set minvent's obj->no_charge to 0 */
			for(obj = mtmp->minvent; obj; obj = obj->nobj) {
				if (Has_contents(obj))
				picked_container(obj);	/* does the right thing */
				obj->no_charge = 0;
			}
			relmon(mtmp);
			newsym(mtmp->mx,mtmp->my);
			mtmp->mx = mtmp->my = 0; /* avoid mnexto()/MON_AT() problem */
			mtmp->wormno = num_segs;
			mtmp->mlstmv = monstermoves;
			mtmp->nmon = mydogs;
			mydogs = mtmp;
			summoner_gone(mtmp, TRUE);	/* has to be after being added to mydogs */
	    } else if (quest_status.touched_artifact && Race_if(PM_DROW) && !flags.initgend && Role_if(PM_NOBLEMAN) && mtmp->m_id == quest_status.leader_m_id) {
			mongone(mtmp);
			u.uevent.qcompleted = TRUE;
	    // } else if(u.uevent.qcompleted && mtmp->mtyp == PM_ORION){
			// mondied(mtmp);
	    } else if (mtmp->iswiz || 
			mtmp->mtyp == PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES || 
			mtmp->mtyp == PM_CENTER_OF_ALL || 
			mtmp->mtyp == PM_HUNGRY_DEAD ||
			mtmp->mtyp == PM_STRANGER ||
			mtmp->mtyp == PM_SUZERAIN ||
			mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH ||
			mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI ||
			mtmp->mtyp == PM_TWIN_SIBLING ||
			mtmp->mtame
		) {
			if (mtmp->mleashed) {
				/* this can happen if your quest leader ejects you from the
				   "home" level while a leashed pet isn't next to you */
				pline("%s leash goes slack.", s_suffix(Monnam(mtmp)));
				m_unleash(mtmp, FALSE);
			}
			/* we want to be able to find him when his next resurrection
			   chance comes up, but have him resume his present location
			   if player returns to this level before that time */
			migrate_to_level(mtmp, ledger_no(&u.uz), MIGR_EXACT_XY, (coord *)0);
	    }
	}
	/* any of your summons that *weren't* kept now disappear */
	summoner_gone(&youmonst, FALSE);
}

#endif /* OVL2 */
#ifdef OVLB

void
migrate_to_level(mtmp, tolev, xyloc, cc)
	register struct monst *mtmp;
	int tolev;	/* destination level */
	xchar xyloc;	/* MIGR_xxx destination xy location: */
	coord *cc;	/* optional destination coordinates */
{
	register struct obj *obj;
	d_level new_lev;
	xchar xyflags;
	int num_segs = 0;	/* count of worm segments */

	/* dead monsters cannot migrate -- they must die where they stood */
	if (DEADMONSTER(mtmp))
		return;
		
	if (mtmp->isshk)
	    set_residency(mtmp, TRUE);

	if (mtmp->wormno) {
	    register int cnt;
	  /* **** NOTE: worm is truncated to # segs = max wormno size **** */
	    cnt = count_wsegs(mtmp);
	    num_segs = min(cnt, MAX_NUM_WORMS - 1);
	    wormgone(mtmp);
	}

	/* set minvent's obj->no_charge to 0 */
	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if (Has_contents(obj))
		picked_container(obj);	/* does the right thing */
	    obj->no_charge = 0;
	}

	if (mtmp->mleashed) {
		mtmp->mtame--;
		if (!mtmp->mtame) untame(mtmp, 1);
		m_unleash(mtmp, TRUE);
	}

	/* summons don't persist away from their summoner, or if they're flagged to not be able to follow */
	/* although your summons can travel between levels with you, they cannot do so independently of you */
	if (get_mx(mtmp, MX_ESUM) && (!mtmp->mextra_p->esum_p->sticky || mtmp->mextra_p->esum_p->summoner)) {
		monvanished(mtmp);
		return;	/* return early -- mtmp is gone. */
	}
	/* likewise, a summoner leaving affects its summons */
	summoner_gone(mtmp, TRUE);

	/* timers pause processing while mon is migrating */
	migrate_timers(mtmp->timed);

	relmon(mtmp);
	mtmp->nmon = migrating_mons;
	migrating_mons = mtmp;
	newsym(mtmp->mx,mtmp->my);

	new_lev.dnum = ledger_to_dnum(tolev);
	new_lev.dlevel = ledger_to_dlev(tolev);
	/* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as */
	/* destination codes (setup flag bits before altering mx or my) */
	xyflags = (depth(&new_lev) < depth(&u.uz));	/* 1 => up */
	if (In_W_tower(mtmp->mx, mtmp->my, &u.uz)) xyflags |= 2;
	mtmp->wormno = num_segs;
	mtmp->mlstmv = monstermoves;
	if (xyloc == MIGR_PORTAL) {
		mtmp->mtrack[2].x = u.uz.dlevel;
		mtmp->mtrack[2].y = u.uz.dnum;
	}
	mtmp->mtrack[1].x = cc ? cc->x : mtmp->mx;
	mtmp->mtrack[1].y = cc ? cc->y : mtmp->my;
	mtmp->mtrack[0].x = xyloc;
	mtmp->mtrack[0].y = xyflags;
	mtmp->mux = new_lev.dnum;
	mtmp->muy = new_lev.dlevel;
	mtmp->mx = mtmp->my = 0;	/* this implies migration */
}

#endif /* OVLB */
#ifdef OVL1

/* return quality of food; the lower the better */
/* fungi will eat even tainted food */
int
dogfood(mon,obj)
struct monst *mon;
register struct obj *obj;
{
	boolean carni = carnivorous(mon->data);
	boolean herbi = herbivorous(mon->data);
	boolean starving;

	if (is_quest_artifact(obj) || obj_resists(obj, 0, 100))
	    return (obj->cursed ? TABU : APPORT);

	switch(obj->oclass) {
	case FOOD_CLASS:
	    if (obj->otyp == CORPSE &&
		((touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
		 || is_rider(&mons[obj->corpsenm])))
		    return TABU;

	    /* Ghouls only eat old corpses... yum! */
	    if (mon->mtyp == PM_GHOUL){
		return (obj->otyp == CORPSE && obj->corpsenm != PM_ACID_BLOB &&
		  peek_at_iced_corpse_age(obj) + 5*rn1(20,10) <= monstermoves) ?
			DOGFOOD : TABU;
	    }
	    /* vampires only "eat" very fresh corpses ... 
	     * Assume meat -> blood
	     */
	    if (is_vampire(mon->data)) {
		return (obj->otyp == CORPSE &&
		  has_blood(&mons[obj->corpsenm]) && !obj->oeaten &&
	    	  peek_at_iced_corpse_age(obj) + 5 >= monstermoves) ?
				DOGFOOD : TABU;
	    }

	    if (!carni && !herbi)
		    return (obj->cursed ? UNDEF : APPORT);

	    /* a starving pet will eat almost anything */
	    starving = (get_mx(mon, MX_EDOG) && EDOG(mon)->mhpmax_penalty);

	    switch (obj->otyp) {
		case TRIPE_RATION:
		case MEATBALL:
		case MEAT_RING:
		case MEAT_STICK:
		case MASSIVE_CHUNK_OF_MEAT:
		    return (carni ? DOGFOOD : MANFOOD);
		case EGG:
		    if (obj->corpsenm != NON_PM && touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
			return POISON;
		    return (carni ? CADAVER : MANFOOD);
		case CORPSE:
rock:
		   if ((peek_at_iced_corpse_age(obj) + 50L <= monstermoves
					    && obj->corpsenm != PM_LIZARD
					    && obj->corpsenm != PM_BABY_CAVE_LIZARD
					    && obj->corpsenm != PM_SMALL_CAVE_LIZARD
					    && obj->corpsenm != PM_CAVE_LIZARD
					    && obj->corpsenm != PM_LARGE_CAVE_LIZARD
					    && obj->corpsenm != PM_LICHEN
					    && obj->corpsenm != PM_BEHOLDER
					    && mon->data->mlet != S_FUNGUS) ||
			(mons[obj->corpsenm].mflagsa == mon->data->mflagsa && !(mindless_mon(mon) || is_animal(mon->data))) ||
			(polyfodder(obj) && !resists_poly(mon->data) && (!(mindless_mon(mon) || is_animal(mon->data)) || goodsmeller(mon->data))) ||
			(acidic(&mons[obj->corpsenm]) && !resists_acid(mon)) ||
			(freezing(&mons[obj->corpsenm]) && !resists_cold(mon)) ||
			(burning(&mons[obj->corpsenm]) && !resists_fire(mon)) ||
			(poisonous(&mons[obj->corpsenm]) &&
						!resists_poison(mon))||
			 (touch_petrifies(&mons[obj->corpsenm]) &&
			  !resists_ston(mon)))
			return POISON;
		    else if (vegan(&mons[obj->corpsenm]))
			return (herbi ? CADAVER : MANFOOD);
		    else return (carni ? CADAVER : MANFOOD);
		case CLOVE_OF_GARLIC:
		    return (is_undead(mon->data) ? TABU :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));
		case TIN:
		    return (metallivorous(mon->data) ? ACCFOOD : TABU);
		case APPLE:
		case CARROT:
		    return (herbi ? DOGFOOD : starving ? ACCFOOD : MANFOOD);
		case BANANA:
		    return ((mon->data->mlet == S_YETI) ? DOGFOOD :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));

                case K_RATION:
		case C_RATION:
                case CRAM_RATION:
		case LEMBAS_WAFER:
		case FOOD_RATION:
		    if (is_human(mon->data) ||
		        is_elf(mon->data) ||
			is_dwarf(mon->data) ||
			is_gnome(mon->data) ||
			is_orc(mon->data))
		        return ACCFOOD; 

		default:
		    if (starving) return ACCFOOD;
		    return (obj->otyp > SLIME_MOLD ?
			    (carni ? ACCFOOD : MANFOOD) :
			    (herbi ? ACCFOOD : MANFOOD));
	    }
	default:
	    if (obj->otyp == AMULET_OF_STRANGULATION ||
			obj->otyp == RIN_SLOW_DIGESTION)
			return TABU;
	    if (hates_silver(mon->data) &&
		obj_is_material(obj, SILVER))
			return(TABU);
	    if (hates_iron(mon->data) &&
		is_iron_obj(obj))
			return(TABU);
	    if (hates_unholy_mon(mon) &&
		is_unholy(obj))
			return(TABU);
	    if (hates_unholy_mon(mon) &&
		obj_is_material(obj, GREEN_STEEL))
			return(TABU);
	    if (hates_unblessed_mon(mon) &&
		(is_unholy(obj) || obj->blessed))
			return(TABU);
		if (is_vampire(mon->data) &&
		obj->otyp == POT_BLOOD && !((touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon)) || is_rider(&mons[obj->corpsenm])))
			return DOGFOOD;
	    if (herbi && !carni && (obj->otyp == SHEAF_OF_HAY || obj->otyp == SEDGE_HAT))
			return CADAVER;
	    if ((mon->mtyp == PM_GELATINOUS_CUBE || mon->mtyp == PM_ANCIENT_OF_CORRUPTION) && is_organic(obj))
			return(ACCFOOD);
	    if (metallivorous(mon->data) && is_metallic(obj) && (is_rustprone(obj) || mon->mtyp != PM_RUST_MONSTER)) {
		/* Non-rustproofed ferrous based metals are preferred. */
		return((is_rustprone(obj) && !obj->oerodeproof) ? DOGFOOD :
			ACCFOOD);
	    }
	    if(!obj->cursed && obj->oclass != BALL_CLASS &&
						obj->oclass != CHAIN_CLASS)
		return(APPORT);
	    /* fall into next case */
	case ROCK_CLASS:
	    return(UNDEF);
	}
}

/* return whether the given monster can eat the food */
/* fungi will eat even tainted food */
boolean
is_edible_mon(mon,obj)
struct monst *mon;
register struct obj *obj;
{
	boolean carni = carnivorous(mon->data);
	boolean herbi = herbivorous(mon->data);
	boolean metal = metallivorous(mon->data);
	boolean magic = magivorous(mon->data);

	if (is_quest_artifact(obj) || obj_resists(obj, 0, 100))
	    return 0;

	if (obj->otyp == CORPSE && is_rider(&mons[obj->corpsenm]))
		return 0;
	
	if (hates_unholy_mon(mon) && is_unholy(obj))
		return 0;
	if (hates_unholy_mon(mon) && obj->obj_material == GREEN_STEEL)
		return 0;
	if (hates_unblessed_mon(mon) && (is_unholy(obj) || obj->blessed))
		return 0;

	if(metal){
		if(hates_silver(mon->data) && obj->obj_material == SILVER)
			return 0;
	    if (hates_iron(mon->data) && is_iron_obj(obj))
			return 0;
		if(mon->mtyp == PM_RUST_MONSTER)
			return is_rustprone(obj);
		else return is_metallic(obj);
	}

	if(magic) return incantifier_edible(obj);

	switch(obj->oclass) {
	case FOOD_CLASS:

	    /* Ghouls only eat old corpses... yum! */
	    if (mon->mtyp == PM_GHOUL){
		return (obj->otyp == CORPSE && obj->corpsenm != PM_ACID_BLOB &&
		  peek_at_iced_corpse_age(obj) + 5*rn1(20,10) <= monstermoves) ?
			1 : 0;
	    }
	    /* vampires only "eat" very fresh corpses ... 
	     * Assume meat -> blood
	     */
	    if (is_vampire(mon->data)) {
		return (obj->otyp == CORPSE &&
		  has_blood(&mons[obj->corpsenm]) && !obj->oeaten &&
	    	  peek_at_iced_corpse_age(obj) + 5 >= monstermoves) ?
				1 : 0;
	    }

	    if (!carni && !herbi)
		    return 0;

	    switch (obj->otyp) {
		case TRIPE_RATION:
		case MEATBALL:
		case MEAT_RING:
		case MEAT_STICK:
		case MASSIVE_CHUNK_OF_MEAT:
		case EGG:
		    return carni;
		case CORPSE:
rock:
		   if (peek_at_iced_corpse_age(obj) + 50L <= monstermoves
				&& obj->corpsenm != PM_LIZARD
				&& obj->corpsenm != PM_BABY_CAVE_LIZARD
				&& obj->corpsenm != PM_SMALL_CAVE_LIZARD
				&& obj->corpsenm != PM_CAVE_LIZARD
				&& obj->corpsenm != PM_LARGE_CAVE_LIZARD
				&& obj->corpsenm != PM_LICHEN
				&& obj->corpsenm != PM_BEHOLDER
				&& mon->data->mlet != S_FUNGUS
			)
				return 0;
		    else if (vegan(&mons[obj->corpsenm]))
				return herbi;
		    else return carni;
		case CLOVE_OF_GARLIC:
		    return is_undead(mon->data) ? 0 : herbi;
		case TIN:
		    return 0;
		case APPLE:
		case CARROT:
		    return herbi;
		case BANANA:
		    return herbi;

		case K_RATION:
		case C_RATION:
		case CRAM_RATION:
		case LEMBAS_WAFER:
		case FOOD_RATION:
			return herbi || carni; 

		default:
		    return (obj->otyp > SLIME_MOLD ? carni : herbi);
	    }
	default:
		if (is_vampire(mon->data) &&
		obj->otyp == POT_BLOOD && !((touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon)) || is_rider(&mons[obj->corpsenm])))
			return 1;
	    if (herbi && !carni && (obj->otyp == SHEAF_OF_HAY || obj->otyp == SEDGE_HAT))
			return 1;
	    if ((mon->mtyp == PM_GELATINOUS_CUBE || mon->mtyp == PM_ANCIENT_OF_CORRUPTION) && is_organic(obj))
			return 1;
	}
	return 0;
}

#endif /* OVL1 */
#ifdef OVLB

void
enough_dogs(numdogs)
int numdogs;
{
	// finds weakest pet, and if there's more than 6 pets that count towards your limit
	// it sets the weakest friendly
	struct monst *curmon = 0, *weakdog = 0;
	int witches = 0, familiars = 0, vampires = 0;
	for(curmon = fmon; curmon; curmon = curmon->nmon){
		if(curmon->mtame && !(EDOG(curmon)->friend) && !(EDOG(curmon)->loyal) && !(EDOG(curmon)->dominated) && !is_suicidal(curmon->data)
			&& !curmon->mspiritual && !(get_timer(curmon->timed, DESUMMON_MON) && !(get_mx(curmon, MX_ESUM) && curmon->mextra_p->esum_p->permanent))
		){
			if(is_witch_mon(curmon)){
				if(witches >= familiars)
					numdogs++;
				witches++;
			}
			else if(curmon->mtyp == PM_WITCH_S_FAMILIAR){
				if(familiars >= witches)
					numdogs++;
				familiars++;
			}
			else if(is_vampire(curmon->data) && check_vampire(VAMPIRE_THRALLS)){
				//50% (if count is odd)
				if(vampires&1)
					numdogs++;
				vampires++;
			}
			else
				numdogs++;
			if(!weakdog) weakdog = curmon;
			if(weakdog->m_lev > curmon->m_lev)
				weakdog = curmon; /* The weakest pet is stronger than the current pet */
			else if(weakdog->m_lev == curmon->m_lev){  /* Do we need tiebreakers? */
				if(weakdog->mtame > curmon->mtame)
					weakdog = curmon; /* Tiebreaker 1: the weakest pet is more tame than the current pet */
				else if(weakdog->mtame == curmon->mtame){ /* Do we need another tiebreaker? */
					int weakdog_mlev = mon_max_lev(weakdog);
					int curmon_mlev = mon_max_lev(curmon);
					if(weakdog_mlev > curmon_mlev)
						weakdog = curmon; /* Tiebreaker 2: the weakest pet has greater potential than the current pet */
					else if(weakdog_mlev == curmon_mlev){
						extern int monstr[];
						if(monstr[weakdog->mtyp] > monstr[curmon->mtyp])
							weakdog = curmon; /* Tiebreaker 3: the weakest pet has greater starting strength than the current pet */
					}
				}
			}
		}
	}
	if(weakdog && numdogs > dog_limit()) EDOG(weakdog)->friend = 1;
}

void
vanish_dogs()
{
	// if there's a spiritual pet that isn't already marked for vanishing,
	// give it 5 turns before it disappears.
	struct monst *weakdog, *curmon;
	int numdogs;
	do {
		weakdog = (struct monst *)0;
		numdogs = 0;
		for(curmon = fmon; curmon; curmon = curmon->nmon){
			if(curmon->mspiritual && !get_timer(curmon->timed, DESUMMON_MON)){ /* assumes no pets that are both spiritual and permanently summoned */
				numdogs++;
				if(!weakdog) weakdog = curmon;
				if(weakdog->m_lev > curmon->m_lev) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
			}
		}
		if(weakdog && numdogs > dog_limit()) start_timer(5L, TIMER_MONSTER, DESUMMON_MON, (genericptr_t)weakdog);
	} while(weakdog && numdogs > dog_limit());
}

int
dog_limit()
{
	int n = ACURR(A_CHA)/3;
	if(u.ufirst_know)
		n += 3;
	if(uring_art(ART_SHARD_FROM_MORGOTH_S_CROWN))
		n += 6;
	return n;
}

void
average_dogs()
{
	struct monst *mon;
	double totalmax = 0, totalcurrent = 0;
	double percent;
	for(mon = fmon; mon; mon = mon->nmon){
		if(mon->mtame && !DEADMONSTER(mon)){
			totalmax += mon->mhpmax;
			totalcurrent += mon->mhp;
		}
	}
	if(Upolyd){
		totalmax += u.mhmax;
		totalcurrent += u.mh;
	} else {
		totalmax += u.uhpmax;
		totalcurrent += u.uhp;
	}
	percent = totalcurrent/totalmax;
	
	for(mon = fmon; mon; mon = mon->nmon){
		if(mon->mtame && !DEADMONSTER(mon)){
			mon->mhp = (int)(percent * mon->mhpmax);//Chops
			totalmax -= mon->mhpmax;
			totalcurrent -= mon->mhp;
			
			percent = totalcurrent/totalmax; //Will creep up over time, due to the effects of the chop
			if(mon->mhp <= 0)
				mondied(mon);
		}
	}
	if(Upolyd){
		if(u.mh != totalcurrent){
			u.mh = max(1, totalcurrent); //Remainder goes to you.  The max(1, limit should never come into play, but we should be nice and never kill the player
			flags.botl = 1;
		}
	} else if(u.uhp != totalcurrent){
		u.uhp = max(1, totalcurrent);
		flags.botl = 1;
	}
}

struct monst *
tamedog(mtmp, obj)
struct monst *mtmp;
struct obj *obj;
{
	return tamedog_core(mtmp, obj, FALSE);
}

struct monst *
tamedog_core(mtmp, obj, enhanced)
struct monst *mtmp;
struct obj *obj;
int enhanced;
{
	struct monst *curmon, *weakdog = (struct monst *) 0;
	/* The Wiz, Medusa and the quest nemeses aren't even made peaceful. || mtmp->mtyp == PM_MEDUSA */
	if (is_untamable(mtmp->data) || mtmp->notame || mtmp->iswiz
		|| (mtmp->mtyp == urole.neminum)
	) return((struct monst *)0);

	/* worst case, at least it'll be peaceful. */
	if(!obj || (!is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP)){
		mtmp->mpeaceful = 1;
		mtmp->mtraitor  = 0;	/* No longer a traitor */
		set_malign(mtmp);
	}

	/* pacify monster cannot tame */
	if (obj && obj->otyp == SPE_PACIFY_MONSTER){
		mtmp->mpeacetime = max(mtmp->mpeacetime, ACURR(A_CHA));
		return((struct monst *)0);
	}

	if((flags.moonphase == FULL_MOON || flags.moonphase == HUNTING_MOON)
		&& night() && rn2(6) && obj 
		&& !is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP
		&& obj->oclass != SPBOOK_CLASS && obj->oclass != SCROLL_CLASS
		&& mtmp->data->mlet == S_DOG
	) return((struct monst *)0);

#ifdef CONVICT
    if (!enhanced && Role_if(PM_CONVICT) && (is_domestic(mtmp->data) && obj
		&& !is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP
		&& obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS)
	) {
        /* Domestic animals are wary of the Convict */
        pline("%s still looks wary of you.", Monnam(mtmp));
        return((struct monst *)0);
    }
#endif
	/* If we cannot tame it, at least it's no longer afraid. */
	mtmp->mflee = 0;
	mtmp->mfleetim = 0;

	/* make grabber let go now, whether it becomes tame or not */
	if (mtmp == u.ustuck) {
	    if (u.uswallow)
		expels(mtmp, mtmp->data, TRUE);
	    else if (!(sticks(&youmonst)))
		unstuck(mtmp);
	}

	/* feeding it treats makes it tamer */
	if (mtmp->mtame && obj 
		&& !is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP
		&& obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS
	) {
	    int tasty;

	    if (mtmp->mcanmove && !mtmp->mconf && !mtmp->meating &&
		((tasty = dogfood(mtmp, obj)) == DOGFOOD ||
		 (tasty <= ACCFOOD && EDOG(mtmp)->hungrytime <= monstermoves))) {
		/* pet will "catch" and eat this thrown food */
		if (canseemon(mtmp)) {
		    boolean big_corpse = (obj->otyp == CORPSE &&
					  obj->corpsenm >= LOW_PM &&
				mons[obj->corpsenm].msize > mtmp->data->msize);
		    pline("%s catches %s%s",
			  Monnam(mtmp), the(xname(obj)),
			  !big_corpse ? "." : ", or vice versa!");
		} else if (cansee(mtmp->mx,mtmp->my))
		    pline("%s.", Tobjnam(obj, "stop"));

		/* Don't stuff ourselves if we know better */
		if (is_animal(mtmp->data) || mindless_mon(mtmp))
		{
		/* dog_eat expects a floor object */
		place_object(obj, mtmp->mx, mtmp->my);
		    if (dog_eat(mtmp, obj, mtmp->mx, mtmp->my, FALSE) == 2
		        && rn2(4))
		    {
		        /* You choked your pet, you cruel, cruel person! */
		        You_feel("guilty about losing your pet like this.");
				adjalign(-15);
				if(!Infuture){
					if(!Role_if(PM_ANACHRONOUNBINDER)) godlist[u.ualign.god].anger++;
					change_hod(5);
				}
		    }
		}
		/* eating might have killed it, but that doesn't matter here;
		   a non-null result suppresses "miss" message for thrown
		   food and also implies that the object has been deleted */
		return mtmp;
	    } else
		return (struct monst *)0;
	}

	if (mtmp->mtame || (!mtmp->mcanmove && !mtmp->moccupation && !enhanced) ||
	    /* monsters with conflicting structures cannot be tamed */
	    mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion ||
	    mtmp->mtyp == urole.neminum ||
	    (!enhanced && is_demon(mtmp->data) && !is_demon(youracedata)) ||
	    (obj
			&& !is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP 
			&& obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS 
			&& dogfood(mtmp, obj) >= MANFOOD
		)
	) return (struct monst *)0;

	if (mtmp->m_id == quest_status.leader_m_id)
	    return((struct monst *)0);

	/* before officially taming the target, check how many pets there are and untame one if there are too many */
	if(!(obj && obj->oclass == SCROLL_CLASS && Confusion)){
		enough_dogs(1);
	}
#ifdef RECORD_ACHIEVE
	//Taming Oona counts as completing the law quest
	if(mtmp->mtyp == PM_OONA)
		give_law_trophy();
#endif
	/* add the pet component */
	add_mx(mtmp, MX_EDOG);
	initedog(mtmp);
	if(obj && obj->otyp == SPE_CHARM_MONSTER){
		mtmp->mpeacetime = 1;
	}

	if (obj && !is_instrument(obj) && obj->otyp != DOLL_OF_FRIENDSHIP 
		&& obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS
	){		/* thrown food */
	    /* defer eating until the edog extension has been set up */
	    place_object(obj, mtmp->mx, mtmp->my);	/* put on floor */
	    /* devour the food (might grow into larger, genocided monster) */
	    if (dog_eat(mtmp, obj, mtmp->mx, mtmp->my, TRUE) == 2)
		return mtmp;		/* oops, it died... */
	    /* `obj' is now obsolete */
	}

	newsym(mtmp->mx, mtmp->my);
	if (mon_attacktype(mtmp, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
	return(mtmp);
}

/* untames mtmp, if it was tame. Sets mtmp->mpeaceful whether or not mtmp was tame */
void
untame(mtmp, be_peaceful)
struct monst * mtmp;
boolean be_peaceful;
{
	rem_mx(mtmp, MX_EDOG);
	mtmp->mtame = 0;
	mtmp->mpeaceful = be_peaceful;
	if (u.usteed == mtmp) {
		dismount_steed(DISMOUNT_THROWN);
	}
	newsym(mtmp->mx, mtmp->my);
	return;
}

struct monst *
make_pet_minion(mtyp,godnum, is_summoned)
int mtyp;
int godnum;
boolean is_summoned;
{
    register struct monst *mon;
    register struct monst *mtmp2;
	mon = makemon_full(&mons[mtyp], u.ux, u.uy, is_summoned ? MM_ESUM : NO_MM_FLAGS, -1, god_faction(godnum));
    if (!mon) return 0;
    /* now tame that puppy... */
	add_mx(mon, MX_EDOG);
    initedog(mon);
    newsym(mon->mx, mon->my);
    mon->mpeaceful = 1;
	EDOG(mon)->loyal = 1;
    set_malign(mon);
    mon->mtame = 10;
    /* this section names the creature "of ______" */
	add_mx(mon, MX_EMIN);
	mon->isminion = TRUE;
	EMIN(mon)->min_align = galign(godnum);
	EMIN(mon)->godnum = godnum;
    return mon;
}

/*
 * Called during pet revival or pet life-saving.
 * If you killed the pet, it revives wild.
 * If you abused the pet a lot while alive, it revives wild.
 * If you abused the pet at all while alive, it revives untame.
 * If the pet wasn't abused and was very tame, it might revive tame.
 */
void
wary_dog(mtmp, was_dead)
struct monst *mtmp;
boolean was_dead;
{
    struct edog *edog;
    boolean quietly = was_dead;

    mtmp->meating = 0;
    if (!mtmp->mtame) return;
    edog = get_mx(mtmp, MX_EDOG) ? EDOG(mtmp) : 0;

    /* if monster was starving when it died, undo that now */
    if (edog && edog->mhpmax_penalty) {
	mtmp->mhpmax += edog->mhpmax_penalty;
	mtmp->mhp += edog->mhpmax_penalty;	/* heal it */
	edog->mhpmax_penalty = 0;
    }

    if (edog && (edog->killed_by_u == 1 || edog->abuse > 2)) {
      boolean be_peaceful = edog->abuse >= 0 && edog->abuse < 10 && !rn2(edog->abuse + 1);
      untame(mtmp, be_peaceful);
      edog = (struct edog *)0;
	if(!quietly && cansee(mtmp->mx, mtmp->my)) {
	    if (haseyes(youracedata)) {
		if (haseyes(mtmp->data))
			pline("%s %s to look you in the %s.",
				Monnam(mtmp),
				mtmp->mpeaceful ? "seems unable" :
					    "refuses",
				body_part(EYE));
		else 
			pline("%s avoids your gaze.",
				Monnam(mtmp));
	    }
	}
    } else {
	/* chance it goes wild anyway - Pet Semetary */
	if (!(edog && (edog->loyal || edog->dominated)) 
		&& !(mtmp->mtame && is_vampire(mtmp->data) && check_vampire(VAMPIRE_THRALLS))
		&& !rn2(mtmp->mtame)
	) {
      untame(mtmp, 0);
      edog = (struct edog *)0;
	}
    }
    if (!mtmp->mtame) {
	newsym(mtmp->mx, mtmp->my);
	/* a life-saved monster might be leashed;
	   don't leave it that way if it's no longer tame */
	if (mtmp->mleashed) m_unleash(mtmp, TRUE);
    }

    /* if its still a pet, start a clean pet-slate now */
    if (edog && mtmp->mtame) {
		edog->revivals++;
		edog->killed_by_u = 0;
		edog->abuse = 0;
		edog->ogoal.x = edog->ogoal.y = -1;
		if (was_dead || edog->hungrytime < monstermoves + 500L)
		    edog->hungrytime = monstermoves + 500L;
		if (was_dead) {
		    edog->droptime = 0L;
		    edog->dropdist = 10000;
		    edog->whistletime = 0L;
		    edog->apport = ACURR(A_CHA);
		} /* else lifesaved, so retain current values */
    }
}

void
abuse_dog(mtmp)
struct monst *mtmp;
{
	if (!mtmp->mtame) return;

	if (Aggravate_monster || Conflict) mtmp->mtame /=2;
	else mtmp->mtame--;

	if (mtmp->mtame && get_mx(mtmp, MX_EDOG))
	    EDOG(mtmp)->abuse++;

	if (!mtmp->mtame && mtmp->mleashed)
	    m_unleash(mtmp, TRUE);

	/* don't make a sound if pet is in the middle of leaving the level */
	/* newsym isn't necessary in this case either */
	if (mtmp->mx != 0) {
	    if (mtmp->mtame && rn2(mtmp->mtame)) yelp(mtmp);
	    else growl(mtmp);	/* give them a moment's worry */
		/* Give monster a chance to betray you now */
	    if (mtmp->mtame) betrayed(mtmp);
	}
	/* untame if mtame = 0 */
	if (!mtmp->mtame) untame(mtmp, 0);
}

#endif /* OVLB */

/*dog.c*/
