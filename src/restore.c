/*	SCCS Id: @(#)restore.c	3.4	2003/09/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "hashmap.h"
#include "tcap.h" /* for TERMLIB and ASCIIGRAPH */

#if defined(MICRO)
extern int dotcnt;	/* shared with save */
extern int dotrow;	/* shared with save */
#endif

#ifdef USE_TILES
extern void FDECL(substitute_tiles, (d_level *));       /* from tile.c */
#endif

#ifdef ZEROCOMP
static int NDECL(mgetc);
#endif
STATIC_DCL void NDECL(find_lev_obj);
STATIC_DCL void FDECL(restlevchn, (int));
STATIC_DCL void FDECL(restdamage, (int,BOOLEAN_P));
STATIC_DCL struct obj *FDECL(restobjchn, (int,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL struct monst *FDECL(restmonchn, (int,BOOLEAN_P));
STATIC_DCL struct fruit *FDECL(loadfruitchn, (int));
STATIC_DCL void FDECL(freefruitchn, (struct fruit *));
STATIC_DCL void FDECL(ghostfruit, (struct obj *));
STATIC_DCL boolean FDECL(restgamestate, (int, unsigned int *, unsigned int *));
STATIC_DCL void FDECL(restlevelstate, (unsigned int, unsigned int));
STATIC_DCL int FDECL(restlevelfile, (int,int));
STATIC_DCL void FDECL(reset_oattached_mids, (BOOLEAN_P));

/*
 * Save a mapping of IDs from ghost levels to the current level.  This
 * map is used by the timer routines when restoring ghost levels.
 */
#define N_PER_BUCKET 64
struct bucket {
    struct bucket *next;
    struct {
	unsigned gid;	/* ghost ID */
	unsigned nid;	/* new ID */
    } map[N_PER_BUCKET];
};

STATIC_DCL void NDECL(clear_id_mapping);
STATIC_DCL void FDECL(add_id_mapping, (unsigned, unsigned));

static int n_ids_mapped = 0;
static struct bucket *id_map = 0;


#ifdef AMII_GRAPHICS
void FDECL( amii_setpens, (int) );	/* use colors from save file */
extern int amii_numcolors;
#endif

#include "quest.h"

boolean restoring = FALSE;
boolean loading_mons = FALSE;
static NEARDATA struct fruit *oldfruit;
static NEARDATA long omoves;

#define Is_IceBox(o) ((o)->otyp == ICE_BOX ? TRUE : FALSE)

/* Recalculate level.objects[x][y], since this info was not saved. */
STATIC_OVL void
find_lev_obj()
{
	register struct obj *fobjtmp = (struct obj *)0;
	register struct obj *otmp;
	int x,y;

	for(x=0; x<COLNO; x++) for(y=0; y<ROWNO; y++)
		level.objects[x][y] = (struct obj *)0;

	/*
	 * Reverse the entire fobj chain, which is necessary so that we can
	 * place the objects in the proper order.  Make all obj in chain
	 * OBJ_FREE so place_object will work correctly.
	 */
	while ((otmp = fobj) != 0) {
		fobj = otmp->nobj;
		otmp->nobj = fobjtmp;
		otmp->where = OBJ_FREE;
		fobjtmp = otmp;
	}
	/* fobj should now be empty */

	/* Set level.objects (as well as reversing the chain back again) */
	while ((otmp = fobjtmp) != 0) {
		fobjtmp = otmp->nobj;
		place_object(otmp, otmp->ox, otmp->oy);
	}
}

/* Things that were marked "in_use" when the game was saved (ex. via the
 * infamous "HUP" cheat) get used up here.
 */
void
inven_inuse(quietly)
boolean quietly;
{
	register struct obj *otmp, *otmp2;

	for (otmp = invent; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
#ifndef GOLDOBJ
	    if (otmp->oclass == COIN_CLASS) {
		/* in_use gold is created by some menu operations */
		if (!otmp->in_use) {
		    impossible("inven_inuse: !in_use gold in inventory");
		}
		extract_nobj(otmp, &invent);
		otmp->in_use = FALSE;
		dealloc_obj(otmp);
	    } else
#endif /* GOLDOBJ */
	    if (otmp->in_use) {
		if (!quietly) pline("Finishing off %s...", xname(otmp));
		otmp->in_use = FALSE;
		useup(otmp);
	    }
	}
}

STATIC_OVL void
restlevchn(fd)
register int fd;
{
	int cnt;
	s_level	*tmplev, *x;

	sp_levchn = (s_level *) 0;
	mread(fd, (genericptr_t) &cnt, sizeof(int));
	for(; cnt > 0; cnt--) {

	    tmplev = (s_level *)alloc(sizeof(s_level));
	    mread(fd, (genericptr_t) tmplev, sizeof(s_level));
	    if(!sp_levchn) sp_levchn = tmplev;
	    else {

		for(x = sp_levchn; x->next; x = x->next);
		x->next = tmplev;
	    }
	    tmplev->next = (s_level *)0;
	}
}

STATIC_OVL void
restdamage(fd, ghostly)
int fd;
boolean ghostly;
{
	int counter;
	struct damage *tmp_dam;

	mread(fd, (genericptr_t) &counter, sizeof(counter));
	if (!counter)
	    return;
	tmp_dam = (struct damage *)alloc(sizeof(struct damage));
	while (--counter >= 0) {
	    char damaged_shops[5], *shp = (char *)0;

	    mread(fd, (genericptr_t) tmp_dam, sizeof(*tmp_dam));
	    if (ghostly)
		tmp_dam->when += (monstermoves - omoves);
	    Strcpy(damaged_shops,
		   in_rooms(tmp_dam->place.x, tmp_dam->place.y, SHOPBASE));
	    if (u.uz.dlevel) {
		/* when restoring, there are two passes over the current
		 * level.  the first time, u.uz isn't set, so neither is
		 * shop_keeper().  just wait and process the damage on
		 * the second pass.
		 */
		for (shp = damaged_shops; *shp; shp++) {
		    struct monst *shkp = shop_keeper(*shp);

		    if (shkp && inhishop(shkp) &&
			    repair_damage(shkp, tmp_dam, TRUE))
			break;
		}
	    }
	    if (!shp || !*shp) {
		tmp_dam->next = level.damagelist;
		level.damagelist = tmp_dam;
		tmp_dam = (struct damage *)alloc(sizeof(*tmp_dam));
	    }
	}
	free((genericptr_t)tmp_dam);
}

STATIC_OVL struct obj *
restobjchn(fd, ghostly, frozen)
register int fd;
boolean ghostly, frozen;
{
	register struct obj *otmp, *otmp2 = 0;
	register struct obj *first = (struct obj *)0;
	int endread;

	while(1) {
		mread(fd, (genericptr_t) &endread, sizeof(endread));
		if(endread == -1) break;
		otmp = newobj(0);
		if(!first) first = otmp;
		else otmp2->nobj = otmp;
		mread(fd, (genericptr_t) otmp, sizeof(struct obj));
		if(otmp->mp){
			otmp->mp = malloc(sizeof(struct mask_properties));
			mread(fd, (genericptr_t) otmp->mp, (unsigned) sizeof(struct mask_properties));
//			mread(fd, (genericptr_t) otmp->mp->mskacurr, (unsigned) sizeof(struct attribs));
//			mread(fd, (genericptr_t) otmp->mp->mskaexe, (unsigned) sizeof(struct attribs));
//			mread(fd, (genericptr_t) otmp->mp->mskamask, (unsigned) sizeof(struct attribs));
		}
		if(otmp->oextra_p){
			rest_oextra(otmp, fd, ghostly);
		}
		if (otmp->light) {
			rest_lightsource(LS_OBJECT, otmp, otmp->light, fd, ghostly);
		}
		if (otmp->timed) {
			rest_timers(TIMER_OBJECT, otmp, otmp->timed, fd, ghostly, monstermoves - omoves);
		}
		if (ghostly) {
		    unsigned nid = flags.ident++;
		    add_id_mapping(otmp->o_id, nid);
		    otmp->o_id = nid;
		}
		if (ghostly && otmp->otyp == SLIME_MOLD) ghostfruit(otmp);
		/* Ghost levels get object age shifted from old player's clock
		 * to new player's clock.  Assumption: new player arrived
		 * immediately after old player died.
		 */
		if (ghostly && !frozen && !age_is_relative(otmp))
		    otmp->age = monstermoves - omoves + otmp->age;

		/* get contents of a container or statue */
		if (Has_contents(otmp)) {
		    struct obj *otmp3;
		    otmp->cobj = restobjchn(fd, ghostly, Is_IceBox(otmp));
		    /* restore container back pointers */
		    for (otmp3 = otmp->cobj; otmp3; otmp3 = otmp3->nobj)
			otmp3->ocontainer = otmp;
		}
		if (otmp->bypass) otmp->bypass = 0;

		otmp2 = otmp;
	}
	if(first && otmp2->nobj){
		impossible("Restobjchn: error reading objchn.");
		otmp2->nobj = 0;
	}

	return(first);
}

STATIC_OVL struct monst *
restmonchn(fd, ghostly)
register int fd;
boolean ghostly;
{
	register struct monst *mtmp, *mtmp2 = 0;
	register struct monst *first = (struct monst *)0;
	int endread;
	struct permonst *monbegin;
	boolean moved;

	loading_mons = TRUE;
	/* get the original base address */
	mread(fd, (genericptr_t)&monbegin, sizeof(monbegin));
	moved = (monbegin != mons);
	/* re-generate index numbers of the permonst array */
	id_permonst();

	while(1) {
		mread(fd, (genericptr_t) &endread, sizeof(int));
		if(endread == -1) break;
		mtmp = malloc(sizeof(struct monst));
		if(!first) first = mtmp;
		else mtmp2->nmon = mtmp;
		mread(fd, (genericptr_t) mtmp, sizeof(struct monst));
		if (mtmp->mextra_p) {
			rest_mextra(mtmp, fd, ghostly);
		}
		/* debug code: detect, warn, and fix mismatches between mxtra booleans and structures
		* Remove this when isminion, etc are deprecated */
		if(mtmp->mtame    && !get_mx(mtmp, MX_EDOG))	{impossible("tame mon not edog"); mtmp->mtame = 0;}
		if(mtmp->isminion && !get_mx(mtmp, MX_EMIN))	{impossible("minion mon not emin"); mtmp->isminion = 0;}
		if(mtmp->ispriest && !get_mx(mtmp, MX_EPRI))	{impossible("priest mon not epri"); mtmp->ispriest = 0;}
		if(mtmp->isshk    && !get_mx(mtmp, MX_ESHK))	{impossible("shopkeeper mon not eshk"); mtmp->isshk = 0;}
		
		if (mtmp->light) {
			rest_lightsource(LS_MONSTER, mtmp, mtmp->light, fd, ghostly);
		}
		if (mtmp->timed) {
			rest_timers(TIMER_MONSTER, mtmp, mtmp->timed, fd, ghostly, monstermoves - omoves);
		}
		if (ghostly) {
			unsigned nid = flags.ident++;
			add_id_mapping(mtmp->m_id, nid);
			mtmp->m_id = nid;
		}
		if (mtmp->data) {
			set_mon_data(mtmp, mtmp->mtyp);
		}
		if (ghostly) {
			int mndx = monsndx(mtmp->data);
			if (propagate(mndx, TRUE, ghostly) == 0) {
				/* cookie to trigger purge in getbones() */
				mtmp->mhpmax = DEFUNCT_MONSTER;	
			}
		}
		if(mtmp->minvent) {
			struct obj *obj;
			mtmp->minvent = restobjchn(fd, ghostly, FALSE);
			/* restore monster back pointer */
			for (obj = mtmp->minvent; obj; obj = obj->nobj)
				obj->ocarry = mtmp;
		}
		if (mtmp->mw) {
			struct obj *obj;

			for(obj = mtmp->minvent; obj; obj = obj->nobj)
				if (obj->owornmask & W_WEP) break;
			if (obj) mtmp->mw = obj;
			else {
				MON_NOWEP(mtmp);
				impossible("bad monster weapon restore");
			}
		}
		if (mtmp->msw) {
			struct obj *obj;

			for(obj = mtmp->minvent; obj; obj = obj->nobj)
				if (obj->owornmask & W_SWAPWEP) break;
			if (obj) mtmp->msw = obj;
			else {
				MON_NOSWEP(mtmp);
				impossible("bad monster swap weapon restore");
			}
		}
		mtmp2 = mtmp;
	}
	if(first && mtmp2->nmon){
		impossible("Restmonchn: error reading monchn.");
		mtmp2->nmon = 0;
	}
	loading_mons = FALSE;
	return(first);
}

STATIC_OVL struct fruit *
loadfruitchn(fd)
int fd;
{
	register struct fruit *flist, *fnext;

	flist = 0;
	while (fnext = newfruit(),
	       mread(fd, (genericptr_t)fnext, sizeof *fnext),
	       fnext->fid != 0) {
		fnext->nextf = flist;
		flist = fnext;
	}
	dealloc_fruit(fnext);
	return flist;
}

STATIC_OVL void
freefruitchn(flist)
register struct fruit *flist;
{
	register struct fruit *fnext;

	while (flist) {
	    fnext = flist->nextf;
	    dealloc_fruit(flist);
	    flist = fnext;
	}
}

STATIC_OVL void
ghostfruit(otmp)
register struct obj *otmp;
{
	register struct fruit *oldf;

	for (oldf = oldfruit; oldf; oldf = oldf->nextf)
		if (oldf->fid == otmp->spe) break;

	if (!oldf) impossible("no old fruit?");
	else otmp->spe = fruitadd(oldf->fname);
}

STATIC_OVL
boolean
restgamestate(fd, stuckid, steedid)
register int fd;
unsigned int *stuckid, *steedid;	/* STEED */
{
	/* discover is actually flags.explore */
	boolean remember_discover = discover;
	struct obj *otmp;
	struct obj *bc_obj;
	int uid;
	int i;

	mread(fd, (genericptr_t) &uid, sizeof uid);
	if (uid != getuid()) {		/* strange ... */
	    /* for wizard mode, issue a reminder; for others, treat it
	       as an attempt to cheat and refuse to restore this file */
	    pline("Saved game was not yours.");
#ifdef WIZARD
	    if (!wizard)
#endif
		return FALSE;
	}

	mread(fd, (genericptr_t) &flags, sizeof(struct flag));
	flags.bypasses = 0;	/* never use the saved value of bypasses */
	has_loaded_bones = flags.end_around;
	flags.end_around = 2;
	if (remember_discover) discover = remember_discover;

	extern struct hashmap_s *itemmap;
	itemmap = malloc(sizeof(struct hashmap_s));
	hashmap_create(32, itemmap);

	role_init(FALSE);	/* Reset the initial role, race, gender, and alignment */
	
#ifdef AMII_GRAPHICS
	amii_setpens(amii_numcolors);	/* use colors from save file */
#endif
	mread(fd, (genericptr_t) &u, sizeof(struct you));
	mread(fd, (genericptr_t) &youmonst, sizeof(struct monst));
	if (youmonst.light)
		rest_lightsource(LS_MONSTER, &youmonst, youmonst.light, fd, FALSE);
	init_uasmon();
#ifdef CLIPPING
	cliparound(u.ux, u.uy);
#endif
	/* reload random monster*/
	{
	extern int monstr[];
	const char *tname;
	tname = mons[PM_SHAMBLING_HORROR].mname;
	mread(fd, (genericptr_t) &mons[PM_SHAMBLING_HORROR], sizeof(struct permonst));
	mons[PM_SHAMBLING_HORROR].mname = tname;
	monstr[PM_SHAMBLING_HORROR] = mstrength(&mons[PM_SHAMBLING_HORROR]);
	
	tname = mons[PM_STUMBLING_HORROR].mname;
	mread(fd, (genericptr_t) &mons[PM_STUMBLING_HORROR], sizeof(struct permonst));
	mons[PM_STUMBLING_HORROR].mname = tname;
	monstr[PM_STUMBLING_HORROR] = mstrength(&mons[PM_STUMBLING_HORROR]);
	
	tname = mons[PM_WANDERING_HORROR].mname;
	mread(fd, (genericptr_t) &mons[PM_WANDERING_HORROR], sizeof(struct permonst));
	mons[PM_WANDERING_HORROR].mname = tname;
	monstr[PM_WANDERING_HORROR] = mstrength(&mons[PM_WANDERING_HORROR]);

	/* all nameless horrors are set to difficulty 40 */
	monstr[PM_NAMELESS_HORROR] = 40;
	}
	
	/* Fix up the highest ranking eladrins */
	if(dungeon_topology.alt_tulani){
		unsigned short genotmp = mons[PM_TULANI_ELADRIN].geno;
		int common_caste;
		switch(dungeon_topology.alt_tulani){
			case GAE_CASTE:
				common_caste = PM_GAE_ELADRIN;
			break;
			case BRIGHID_CASTE:
				common_caste = PM_BRIGHID_ELADRIN;
			break;
			case UISCERRE_CASTE:
				common_caste = PM_UISCERRE_ELADRIN;
			break;
			case CAILLEA_CASTE:
				common_caste = PM_CAILLEA_ELADRIN;
			break;
		}
		mons[PM_TULANI_ELADRIN].geno = mons[common_caste].geno;
		mons[common_caste].geno = genotmp;
	}
	/* Fix up the alignment quest nemesi */
	mons[PM_OONA].mcolor = (u.oonaenergy == AD_FIRE) ? CLR_RED 
						 : (u.oonaenergy == AD_COLD) ? CLR_CYAN 
						 : (u.oonaenergy == AD_ELEC) ? HI_ZAP 
						 : CLR_WHITE;
	
	if(u.uhp <= 0 && (!Upolyd || u.mh <= 0)) {
	    u.ux = u.uy = 0;	/* affects pline() [hence You()] */
	    You("were not healthy enough to survive restoration.");
	    /* wiz1_level.dlevel is used by mklev.c to see if lots of stuff is
	     * uninitialized, so we only have to set it and not the other stuff.
	     */
	    wiz1_level.dlevel = 0;
	    u.uz.dnum = 0;
	    u.uz.dlevel = 1;
	    return(FALSE);
	}

	/* this stuff comes after potential aborted restore attempts */
	restore_gods(fd);
	restore_artifacts(fd);
	invent = restobjchn(fd, FALSE, FALSE);
	bc_obj = restobjchn(fd, FALSE, FALSE);
	while (bc_obj) {
		struct obj *nobj = bc_obj->nobj;
		if (bc_obj->owornmask) setworn(bc_obj, bc_obj->owornmask);
		bc_obj->nobj = (struct obj *)0;
		bc_obj = nobj;
	}
	for(i=0;i<10;i++) magic_chest_objs[i] = restobjchn(fd, FALSE, FALSE);
	migrating_objs = restobjchn(fd, FALSE, FALSE);
	migrating_mons = restmonchn(fd, FALSE);
	mread(fd, (genericptr_t) mvitals, sizeof(mvitals));

	/* this comes after inventory has been loaded */
	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->owornmask)
			setworn(otmp, otmp->owornmask);
	/* reset weapon so that player will get a reminder about "bashing"
	   during next fight when bare-handed or wielding an unconventional
	   item; for pick-axe, we aren't able to distinguish between having
	   applied or wielded it, so be conservative and assume the former */
	otmp = uwep;	/* `uwep' usually init'd by setworn() in loop above */
	uwep = 0;	/* clear it and have setuwep() reinit */
	setuwep(otmp);	/* (don't need any null check here) */
	if (!uwep || uwep->otyp == PICK_AXE || uwep->otyp == GRAPPLING_HOOK)
	    unweapon = TRUE;

	restore_dungeon(fd);
	restlevchn(fd);
	mread(fd, (genericptr_t) &moves, sizeof moves);
	mread(fd, (genericptr_t) &monstermoves, sizeof monstermoves);
	mread(fd, (genericptr_t) &quest_status, sizeof(struct q_score));
	mread(fd, (genericptr_t) spl_book,
				sizeof(struct spell) * (MAXSPELL + 1));
	restore_oracles(fd);
	if (u.ustuck)
		mread(fd, (genericptr_t) stuckid, sizeof (*stuckid));
#ifdef STEED
	if (u.usteed)
		mread(fd, (genericptr_t) steedid, sizeof (*steedid));
#endif
	mread(fd, (genericptr_t) pl_character, sizeof pl_character);

	mread(fd, (genericptr_t) pl_fruit, sizeof pl_fruit);
	mread(fd, (genericptr_t) &current_fruit, sizeof current_fruit);
	freefruitchn(ffruit);	/* clean up fruit(s) made by initoptions() */
	ffruit = loadfruitchn(fd);

	restnames(fd);
	restore_waterlevel(fd);
#ifdef RECORD_ACHIEVE
	mread(fd, (genericptr_t) &achieve, sizeof achieve);
#endif
#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)
	mread(fd, (genericptr_t) &realtime_data.realtime, 
			  sizeof realtime_data.realtime);
#endif
	/* must come after all mons & objs are restored */
	relink_mx((struct monst *)0);
	relink_ox((struct obj *)0);
#ifdef WHEREIS_FILE
        touch_whereis();
#endif
	return(TRUE);
}

/* update game state pointers to those valid for the current level (so we
 * don't dereference a wild u.ustuck when saving the game state, for instance)
 */
STATIC_OVL void
restlevelstate(stuckid, steedid)
unsigned int stuckid, steedid;	/* STEED */
{
	register struct monst *mtmp;

	if (stuckid) {
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			if (mtmp->m_id == stuckid) break;
		if (!mtmp){
			pline("Error recovery: Cannot find the monster ustuck.");
			u.ustuck = 0;
		} else {
			u.ustuck = mtmp;
		}
	}
#ifdef STEED
	if (steedid) {
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			if (mtmp->m_id == steedid) break;
		if (!mtmp){
			pline("Error recovery: Cannot find the monster usteed.");
			u.usteed = 0;
		} else {
			u.usteed = mtmp;
			remove_monster(mtmp->mx, mtmp->my);
		}
	}
#endif
}

/*ARGSUSED*/	/* fd used in MFLOPPY only */
STATIC_OVL int
restlevelfile(fd, ltmp)
register int fd;
int ltmp;
#if defined(macintosh) && (defined(__SC__) || defined(__MRC__))
# pragma unused(fd)
#endif
{
	register int nfd;
	char whynot[BUFSZ];

	nfd = create_levelfile(ltmp, whynot);
	if (nfd < 0) {
		/* BUG: should suppress any attempt to write a panic
		   save file if file creation is now failing... */
		panic("restlevelfile: %s", whynot);
	}
#ifdef MFLOPPY
	if (!savelev(nfd, ltmp, COUNT_SAVE)) {

		/* The savelev can't proceed because the size required
		 * is greater than the available disk space.
		 */
		pline("Not enough space on `%s' to restore your game.",
			levels);

		/* Remove levels and bones that may have been created.
		 */
		(void) close(nfd);
# ifdef AMIGA
		clearlocks();
# else
		eraseall(levels, alllevels);
		eraseall(levels, allbones);

		/* Perhaps the person would like to play without a
		 * RAMdisk.
		 */
		if (ramdisk) {
			/* PlaywoRAMdisk may not return, but if it does
			 * it is certain that ramdisk will be 0.
			 */
			playwoRAMdisk();
			/* Rewind save file and try again */
			(void) lseek(fd, (off_t)0, 0);
			(void) uptodate(fd, (char *)0);	/* skip version */
			return dorecover(fd);	/* 0 or 1 */
		} else {
# endif
			pline("Be seeing you...");
			terminate(EXIT_SUCCESS);
# ifndef AMIGA
		}
# endif
	}
#endif
	bufon(nfd);
	savelev(nfd, ltmp, WRITE_SAVE | FREE_SAVE);
	bclose(nfd);
	return(2);
}

int
dorecover(fd)
register int fd;
{
	unsigned int stuckid = 0, steedid = 0;	/* not a register */
	int ltmp;
	int rtmp;
	struct obj *otmp;

#ifdef STORE_PLNAME_IN_FILE
	mread(fd, (genericptr_t) plname, PL_NSIZ);
#endif

	restoring = TRUE;
	getlev(fd, 0, (int)0, FALSE);
	if (!restgamestate(fd, &stuckid, &steedid)) {
		display_nhwindow(WIN_MESSAGE, TRUE);
		savelev(-1, 0, FREE_SAVE);	/* discard current level */
		(void) close(fd);
		(void) delete_savefile();
		restoring = FALSE;
		return(0);
	}
	restlevelstate(stuckid, steedid);
#ifdef INSURANCE
	savestateinlock();
#endif
	rtmp = restlevelfile(fd, ledger_no(&u.uz));
	if (rtmp < 2) return(rtmp);  /* dorecover called recursively */

	/* these pointers won't be valid while we're processing the
	 * other levels, but they'll be reset again by restlevelstate()
	 * afterwards, and in the meantime at least u.usteed may mislead
	 * place_monster() on other levels
	 */
	u.ustuck = (struct monst *)0;
#ifdef STEED
	u.usteed = (struct monst *)0;
#endif

#ifdef MICRO
# ifdef AMII_GRAPHICS
	{
	extern struct window_procs amii_procs;
	if(windowprocs.win_init_nhwindows== amii_procs.win_init_nhwindows){
	    extern winid WIN_BASE;
	    clear_nhwindow(WIN_BASE);	/* hack until there's a hook for this */
	}
	}
# else
	clear_nhwindow(WIN_MAP);
# endif
	clear_nhwindow(WIN_MESSAGE);
	You("return to level %d in %s%s.",
		depth(&u.uz), dungeons[u.uz.dnum].dname,
		flags.debug ? " while in debug mode" :
		flags.explore ? " while in explore mode" : "");
	curs(WIN_MAP, 1, 1);
	dotcnt = 0;
	dotrow = 2;
	if (strncmpi("X11", windowprocs.name, 3))
    	  putstr(WIN_MAP, 0, "Restoring:");
#endif
	while(1) {
#ifdef ZEROCOMP
		if(mread(fd, (genericptr_t) &ltmp, sizeof ltmp) < 0)
#else
		if(read(fd, (genericptr_t) &ltmp, sizeof ltmp) != sizeof ltmp)
#endif
			break;
		getlev(fd, 0, ltmp, FALSE);
#ifdef MICRO
		curs(WIN_MAP, 1+dotcnt++, dotrow);
		if (dotcnt >= (COLNO - 1)) {
			dotrow++;
			dotcnt = 0;
		}
		if (strncmpi("X11", windowprocs.name, 3)){
		  putstr(WIN_MAP, 0, ".");
		}
		mark_synch();
#endif
		rtmp = restlevelfile(fd, ltmp);
		if (rtmp < 2) return(rtmp);  /* dorecover called recursively */
	}

#ifdef BSD
	(void) lseek(fd, 0L, 0);
#else
	(void) lseek(fd, (off_t)0, 0);
#endif
	(void) uptodate(fd, (char *)0);		/* skip version info */
#ifdef STORE_PLNAME_IN_FILE
	mread(fd, (genericptr_t) plname, PL_NSIZ);
#endif
	getlev(fd, 0, (int)0, FALSE);
	(void) close(fd);

	if (!wizard && !discover)
		(void) delete_savefile();
#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) assign_rogue_graphics(TRUE);
#endif
#ifdef USE_TILES
	substitute_tiles(&u.uz);
#endif
	restlevelstate(stuckid, steedid);
#ifdef MFLOPPY
	gameDiskPrompt();
#endif
	max_rank_sz(); /* to recompute mrank_sz (botl.c) */

	/* this comes after inventory has been loaded */
	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->owornmask)
			setworn(otmp, otmp->owornmask);
	/* take care of iron ball & chain */
	for(otmp = fobj; otmp; otmp = otmp->nobj)
		if(otmp->owornmask)
			setworn(otmp, otmp->owornmask);

	if ((uball && !uchain) || (uchain && !uball)) {
		impossible("either ball or chain lost in restgamestate");
		/* ugh, gotta fix it now to avoid further bugs... */
		setworn((struct obj *)0, W_CHAIN);
		setworn((struct obj *)0, W_BALL);
	}

	/* in_use processing must be after:
	 *    + The inventory has been read so that freeinv() works.
	 *    + The current level has been restored so billing information
	 *	is available.
	 */
	inven_inuse(FALSE);

	load_qtlist();	/* re-load the quest text info */
	reset_attribute_clock();
	/* Set up the vision internals, after levl[] data is loaded */
	/* but before docrt().					    */
	vision_reset();
	vision_full_recalc = 1;	/* recompute vision (not saved) */

	run_timers();	/* expire all timers that have gone off while away */
	docrt();
	restoring = FALSE;
	clear_nhwindow(WIN_MESSAGE);
	program_state.something_worth_saving++;	/* useful data now exists */

#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)

/* Start the timer here (realtime has already been set) */
#if defined(BSD) && !defined(POSIX_TYPES)
	(void) time((long *)&realtime_data.restoretime);
#else
	(void) time(&realtime_data.restoretime);
#endif

#endif /* RECORD_REALTIME || REALTIME_ON_BOTL */

	/* Success! */
	welcome(FALSE);
	return(1);
}

void
trickery(reason)
char *reason;
{
	int *c = (int *)0;
	pline("Strange, this map is not as I remember it.");
	pline("Somebody is trying some trickery here...");
	pline("This game is void.");
	killer = reason;
	*c = 1;
	done(TRICKED);
}

void
getlev(fd, pid, lev, ghostly)
int fd, pid;
int lev;
boolean ghostly;
{
	register struct trap *trap;
	register struct monst *mtmp;
	branch *br;
	int hpid;
	int dlvl;
	int x, y;
#ifdef TOS
	short tlev;
#endif

	if (ghostly)
	    clear_id_mapping();

#if defined(MSDOS) || defined(OS2)
	setmode(fd, O_BINARY);
#endif
	/* Load the old fruit info.  We have to do it first, so the
	 * information is available when restoring the objects.
	 */
	if (ghostly) oldfruit = loadfruitchn(fd);

	/* First some sanity checks */
	mread(fd, (genericptr_t) &hpid, sizeof(hpid));
/* CHECK:  This may prevent restoration */
#ifdef TOS
	mread(fd, (genericptr_t) &tlev, sizeof(tlev));
	dlvl=tlev&0x00ff;
#else
	mread(fd, (genericptr_t) &dlvl, sizeof(dlvl));
#endif
	if ((pid && pid != hpid) || (lev && dlvl != lev)) {
	    char trickbuf[BUFSZ];

	    if (pid && pid != hpid)
		Sprintf(trickbuf, "PID (%d) doesn't match saved PID (%d)!",
			hpid, pid);
	    else
		Sprintf(trickbuf, "This is level %d, not %d!", dlvl, lev);
#ifdef WIZARD
	    if (wizard) pline1(trickbuf);
#endif
	    trickery(trickbuf);
	}

#ifdef RLECOMP
	{
		short	i, j;
		uchar	len;
		struct rm r;
		
#if defined(MAC)
		/* Suppress warning about used before set */
		(void) memset((genericptr_t) &r, 0, sizeof(r));
#endif
		i = 0; j = 0; len = 0;
		while(i < ROWNO) {
		    while(j < COLNO) {
			if(len > 0) {
			    levl[j][i] = r;
			    len -= 1;
			    j += 1;
			} else {
			    mread(fd, (genericptr_t)&len, sizeof(uchar));
			    mread(fd, (genericptr_t)&r, sizeof(struct rm));
			}
		    }
		    j = 0;
		    i += 1;
		}
	}
#else
	mread(fd, (genericptr_t) levl, sizeof(levl));
#endif	/* RLECOMP */

	mread(fd, (genericptr_t)&omoves, sizeof(omoves));
	mread(fd, (genericptr_t)&upstair, sizeof(stairway));
	mread(fd, (genericptr_t)&dnstair, sizeof(stairway));
	mread(fd, (genericptr_t)&upladder, sizeof(stairway));
	mread(fd, (genericptr_t)&dnladder, sizeof(stairway));
	mread(fd, (genericptr_t)&sstairs, sizeof(stairway));
	mread(fd, (genericptr_t)&updest, sizeof(dest_area));
	mread(fd, (genericptr_t)&dndest, sizeof(dest_area));
	mread(fd, (genericptr_t)&level.flags, sizeof(level.flags));
	mread(fd, (genericptr_t)&level.lastmove, sizeof(level.lastmove));
	mread(fd, (genericptr_t)doors, sizeof(doors));
	mread(fd, (genericptr_t)&altarindex, sizeof(int));
	mread(fd, (genericptr_t)altars, sizeof(altars));
	rest_rooms(fd);		/* No joke :-) */
	if (nroom)
	    doorindex = rooms[nroom - 1].fdoor + rooms[nroom - 1].doorct;
	else
	    doorindex = 0;

	fmon = restmonchn(fd, ghostly);

	rest_worm(fd);	/* restore worm information */
	ftrap = 0;
	while (trap = newtrap(),
	       mread(fd, (genericptr_t)trap, sizeof(struct trap)),
	       trap->tx != 0) {	/* need "!= 0" to work around DICE 3.0 bug */
		/* if there's a stale pointer, we need to reload the old saved ammo */
		if (trap->ammo) {
			struct obj * obj;
			trap->ammo = restobjchn(fd, ghostly, FALSE);
			/* restore trap back pointer */
			for (obj = trap->ammo; obj; obj = obj->nobj)
				obj->otrap = trap;
		}
		trap->ntrap = ftrap;
		ftrap = trap;
	}
	dealloc_trap(trap);
	fobj = restobjchn(fd, ghostly, FALSE);
	find_lev_obj();
	/* restobjchn()'s `frozen' argument probably ought to be a callback
	   routine so that we can check for objects being buried under ice */
	level.buriedobjlist = restobjchn(fd, ghostly, FALSE);
	billobjs = restobjchn(fd, ghostly, FALSE);
	rest_engravings(fd);

	/* reset level.monsters for new level */
	for (x = 0; x < COLNO; x++)
	    for (y = 0; y < ROWNO; y++)
		level.monsters[x][y] = (struct monst *) 0;
	for (mtmp = level.monlist; mtmp; mtmp = mtmp->nmon) {
	    if (mtmp->isshk)
		set_residency(mtmp, FALSE);
	    place_monster(mtmp, mtmp->mx, mtmp->my);
	    if (mtmp->wormno) place_wsegs(mtmp);
	}
	restdamage(fd, ghostly);

	rest_regions(fd, ghostly);
	if (ghostly) {
	    /* Now get rid of all the temp fruits... */
	    freefruitchn(oldfruit),  oldfruit = 0;

	    if (lev > ledger_no(&challenge_level) &&
			lev < ledger_no(&stronghold_level) && xdnstair == 0) {
		coord cc;

		mazexy(&cc);
		xdnstair = cc.x;
		ydnstair = cc.y;
		levl[cc.x][cc.y].typ = STAIRS;
	    }

	    br = Is_branchlev(&u.uz);
	    if (br //&& u.uz.dlevel == 1
			) {
			d_level * ltmp = branchlev_other_end(br, &u.uz);

			switch(br->type) {
			case BR_STAIR:
			case BR_NO_END1:
			case BR_NO_END2: /* OK to assign to sstairs if it's not used */
				assign_level(&sstairs.tolev, ltmp);
				break;		
			case BR_PORTAL:
				{
				register struct trap *ttmp;
				/* find the portal on the level that links to the correct dungeon, we will correct which dlevel it goes to */
				for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
					if (ttmp->ttyp == MAGIC_PORTAL && ttmp->dst.dnum == ltmp->dnum)
					break;
				if (!ttmp) panic("getlev: need portal but none found");
				assign_level(&ttmp->dst, ltmp);
				}
				break;
			}
	    }
	}

	/* must come after all mons & objs are restored */
	relink_mx((struct monst *)0);
	relink_ox((struct obj *)0);
	reset_oattached_mids(ghostly);

	/* regenerate animals while on another level */
	if (u.uz.dlevel) {
	    register struct monst *mtmp2;

	    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
			mtmp2 = mtmp->nmon;
			if (ghostly) {
				/* reset peaceful/malign relative to new character */
				if(!mtmp->isshk)
					/* shopkeepers will reset based on name */
					mtmp->mpeaceful = peace_minded(mtmp);
				set_malign(mtmp);
			} else if (monstermoves > omoves){
				mon_catchup_elapsed_time(mtmp, monstermoves - omoves);

				/* update shape-changers in case protection against
				   them is different now than when the level was saved */
				restore_cham(mtmp);
			}
	    }
		//Note: in case there's a bones file or other fail.
		if(u.silver_flame_z.dnum == u.uz.dnum && u.silver_flame_z.dlevel == u.uz.dlevel
		 && ((!(u.s_f_x) && !(u.s_f_y)) || (!isok(u.s_f_x, u.s_f_y) || !ZAP_POS(levl[u.s_f_x][u.s_f_y].typ)))
		){
			int limit = 1000;
			do {
				u.s_f_x = rnd(COLNO-1);
				u.s_f_y = rn2(ROWNO);
			} while((!isok(u.s_f_x, u.s_f_y) || !ZAP_POS(levl[u.s_f_x][u.s_f_y].typ)) && limit-- > 0);
		}
	}

	if (ghostly)
	    clear_id_mapping();
}


/* Clear all structures for object and monster ID mapping. */
STATIC_OVL void
clear_id_mapping()
{
    struct bucket *curr;

    while ((curr = id_map) != 0) {
	id_map = curr->next;
	free((genericptr_t) curr);
    }
    n_ids_mapped = 0;
}

/* Add a mapping to the ID map. */
STATIC_OVL void
add_id_mapping(gid, nid)
    unsigned gid, nid;
{
    int idx;

    idx = n_ids_mapped % N_PER_BUCKET;
    /* idx is zero on first time through, as well as when a new bucket is */
    /* needed */
    if (idx == 0) {
	struct bucket *gnu = (struct bucket *) alloc(sizeof(struct bucket));
	gnu->next = id_map;
	id_map = gnu;
    }

    id_map->map[idx].gid = gid;
    id_map->map[idx].nid = nid;
    n_ids_mapped++;
}

/*
 * Global routine to look up a mapping.  If found, return TRUE and fill
 * in the new ID value.  Otherwise, return false and return -1 in the new
 * ID.
 */
boolean
lookup_id_mapping(gid, nidp)
    unsigned gid, *nidp;
{
    int i;
    struct bucket *curr;

    if (n_ids_mapped)
	for (curr = id_map; curr; curr = curr->next) {
	    /* first bucket might not be totally full */
	    if (curr == id_map) {
		i = n_ids_mapped % N_PER_BUCKET;
		if (i == 0) i = N_PER_BUCKET;
	    } else
		i = N_PER_BUCKET;

	    while (--i >= 0)
		if (gid == curr->map[i].gid) {
		    *nidp = curr->map[i].nid;
		    return TRUE;
		}
	}

    return FALSE;
}

STATIC_OVL void
reset_oattached_mids(ghostly)
boolean ghostly;
{
    struct obj *otmp;
    unsigned oldid, nid;
    for (otmp = fobj; otmp; otmp = otmp->nobj) {
	if (ghostly && get_ox(otmp, OX_EMON)) {
	    struct monst *mtmp = EMON(otmp);
	    mtmp->m_id = 0;
		/* pet's owner died! They are no longer tame. */
		if (mtmp->mextra_p) {
			void * mextra_bundle = mtmp+1;
			unbundle_mextra(mtmp, mextra_bundle);
			untame(mtmp, 0);
			rem_ox(otmp, OX_EMON);
			save_mtraits(otmp, mtmp);
		}
	}
	if (ghostly && get_ox(otmp, OX_EMID)) {
		oldid = otmp->oextra_p->emid_p[0];
	    if (lookup_id_mapping(oldid, &nid))
			otmp->oextra_p->emid_p[0] = nid;
	    else
			rem_ox(otmp, OX_EMID);
	}
    }
}

#ifdef ZEROCOMP
#define RLESC '\0'	/* Leading character for run of RLESC's */

#ifndef ZEROCOMP_BUFSIZ
#define ZEROCOMP_BUFSIZ BUFSZ
#endif
static NEARDATA unsigned char inbuf[ZEROCOMP_BUFSIZ];
static NEARDATA unsigned short inbufp = 0;
static NEARDATA unsigned short inbufsz = 0;
static NEARDATA short inrunlength = -1;
static NEARDATA int mreadfd;

static int
mgetc()
{
    if (inbufp >= inbufsz) {
	inbufsz = read(mreadfd, (genericptr_t)inbuf, sizeof inbuf);
	if (!inbufsz) {
	    if (inbufp > sizeof inbuf)
		error("EOF on file #%d.\n", mreadfd);
	    inbufp = 1 + sizeof inbuf;  /* exactly one warning :-) */
	    return -1;
	}
	inbufp = 0;
    }
    return inbuf[inbufp++];
}

void
minit()
{
    inbufsz = 0;
    inbufp = 0;
    inrunlength = -1;
}

int
mread(fd, buf, len)
int fd;
genericptr_t buf;
register unsigned len;
{
    /*register int readlen = 0;*/
    if (fd < 0) error("Restore error; mread attempting to read file %d.", fd);
    mreadfd = fd;
    while (len--) {
	if (inrunlength > 0) {
	    inrunlength--;
	    *(*((char **)&buf))++ = '\0';
	} else {
	    register short ch = mgetc();
	    if (ch < 0) return -1; /*readlen;*/
	    if ((*(*(char **)&buf)++ = (char)ch) == RLESC) {
		inrunlength = mgetc();
	    }
	}
	/*readlen++;*/
    }
    return 0; /*readlen;*/
}

#else /* ZEROCOMP */

void
minit()
{
    return;
}

void
mread(fd, buf, len)
register int fd;
register genericptr_t buf;
register unsigned int len;
{
	register int rlen;

#if defined(BSD) || defined(ULTRIX)
	rlen = read(fd, buf, (int) len);
	if(rlen != len){
#else /* e.g. SYSV, __TURBOC__ */
	rlen = read(fd, buf, (unsigned) len);
	if((unsigned)rlen != len){
#endif
		pline("Read %d instead of %u bytes.", rlen, len);
		if(restoring) {
			(void) close(fd);
			(void) delete_savefile();
			error("Error restoring old game.");
		}
		panic("Error reading level file.");
	}
}
#endif /* ZEROCOMP */

/*restore.c*/
