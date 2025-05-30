/*	SCCS Id: @(#)priest.c	3.4	2002/11/06	*/
/* Copyright (c) Izchak Miller, Steve Linhart, 1989.		  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"

/* this matches the categorizations shown by enlightenment */
#define ALGN_SINNED	(-4)	/* worse than strayed */

#ifdef OVLB

STATIC_DCL boolean FDECL(histemple_at,(struct monst *,XCHAR_P,XCHAR_P));
STATIC_DCL boolean FDECL(has_shrine,(struct monst *));

static const char tools[] = { TOOL_CLASS, 0 };
/*
 * Move for priests and shopkeepers.  Called from shk_move() and pri_move().
 * Valid returns are  1: moved  0: didn't  -1: let m_move do it  -2: died.
 */
int
move_special(mtmp,in_his_shop,appr,uondoor,avoid,omx,omy,gx,gy)
register struct monst *mtmp;
boolean in_his_shop;
schar appr;
boolean uondoor,avoid;
register xchar omx,omy,gx,gy;
{
	register xchar nx,ny,nix,niy;
	register schar i;
	schar chcnt,cnt;
	coord poss[9];
	long info[9];
	long allowflags;
	struct obj *ib = (struct obj *)0;

	if(omx == gx && omy == gy)
		return(0);
	if(mtmp->mconf) {
		avoid = FALSE;
		appr = 0;
	}

	nix = omx;
	niy = omy;
	if (mtmp->isshk) allowflags = ALLOW_SSM;
	else allowflags = ALLOW_SSM | ALLOW_SANCT;
	if (mon_resistance(mtmp,PASSES_WALLS)) allowflags |= (ALLOW_ROCK|ALLOW_WALL);
	if (throws_rocks(mtmp->data)) allowflags |= ALLOW_ROCK;
	if (tunnels(mtmp->data)) allowflags |= ALLOW_DIG;
	if (!nohands(mtmp->data) && !verysmall(mtmp->data)) {
		allowflags |= OPENDOOR;
		if (m_carrying(mtmp, SKELETON_KEY)||m_carrying(mtmp, UNIVERSAL_KEY)) 
			allowflags |= BUSTDOOR;
	}
	if (species_busts_doors(mtmp->data)) allowflags |= BUSTDOOR;
	cnt = mfndpos(mtmp, poss, info, allowflags);

	/* this function ONLY moves, never attacks -- disallow any occupied squares */
	register schar j;
	chcnt = 0;
	for(i=0; i<cnt; i++) {
		if (info[i]&(ALLOW_U|ALLOW_M|ALLOW_TM)) {
			for(j=i+1; j<cnt; j++) {
				info[j-1] = info[j];
				poss[j-1] = poss[j];
			}
			chcnt++;
		}
	}
	cnt -= chcnt;

	if(mtmp->isshk && avoid && uondoor) { /* perhaps we cannot avoid him */
		for(i=0; i<cnt; i++)
		    if(!(info[i] & NOTONL)) goto pick_move;
		avoid = FALSE;
	}

#define GDIST(x,y)	(dist2(x,y,gx,gy))
pick_move:
	chcnt = 0;
	for(i=0; i<cnt; i++) {
		nx = poss[i].x;
		ny = poss[i].y;
		if((levl[nx][ny].typ == ROOM 
			|| levl[nx][ny].typ == GRASS 
			|| levl[nx][ny].typ == SOIL 
			|| levl[nx][ny].typ == SAND 
			) ||
			(mtmp->ispriest &&
			    levl[nx][ny].typ == ALTAR) ||
			(mtmp->isshk &&
			    (!in_his_shop || ESHK(mtmp)->following))) {
		    if(avoid && (info[i] & NOTONL))
			continue;
		    if((!appr && !rn2(++chcnt)) ||
			(appr && GDIST(nx,ny) < GDIST(nix,niy))) {
			    nix = nx;
			    niy = ny;
		    }
		}
	}
	if(mtmp->ispriest && avoid &&
			nix == omx && niy == omy && onlineu(omx,omy)) {
		/* might as well move closer as long it's going to stay
		 * lined up */
		avoid = FALSE;
		goto pick_move;
	}

	if(nix != omx || niy != omy) {
		remove_monster(omx, omy);
		place_monster(mtmp, nix, niy);
		newsym(nix,niy);
		if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
		    check_special_room(FALSE);
		if(ib) {
			if (cansee(mtmp->mx,mtmp->my))
			    pline("%s picks up %s.", Monnam(mtmp),
				distant_name(ib,doname));
			obj_extract_self(ib);
			(void) mpickobj(mtmp, ib);
		}
		return(1);
	}
	return(0);
}

#endif /* OVLB */

#ifdef OVL0

char
temple_occupied(array)
register char *array;
{
	register char *ptr;

	for (ptr = array; *ptr; ptr++)
		if (rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
			return(*ptr);
	return('\0');
}

aligntyp
temple_alignment(roomno)
int roomno;
{
	char *ptr;
	coord *shrine_spot;
	struct rm *lev;

	shrine_spot = shrine_pos(roomno);
	if(!IS_ALTAR(levl[shrine_spot->x][shrine_spot->y].typ)){
		shrine_spot = find_shrine_altar(roomno);
	}
	return (a_align(shrine_spot->x, shrine_spot->y));
}

int
temple_god(int roomno)
{
	char *ptr;
	coord *shrine_spot;
	struct rm *lev;

	shrine_spot = shrine_pos(roomno);
	if(!IS_ALTAR(levl[shrine_spot->x][shrine_spot->y].typ)){
		shrine_spot = find_shrine_altar(roomno);
	}
	return (god_at_altar(shrine_spot->x, shrine_spot->y));
}
#endif /* OVL0 */
#ifdef OVLB

STATIC_OVL boolean
histemple_at(priest, x, y)
register struct monst *priest;
register xchar x, y;
{
	return((boolean)((EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE)) &&
	       on_level(&(EPRI(priest)->shrlevel), &u.uz)));
}

/*
 * pri_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
pri_move(priest)
register struct monst *priest;
{
	register xchar gx,gy,omx,omy;
	schar temple;
	boolean avoid = TRUE;

	omx = priest->mx;
	omy = priest->my;

	if(!histemple_at(priest, omx, omy)) return(-1);

	temple = EPRI(priest)->shroom;

	gx = EPRI(priest)->shrpos.x;
	gy = EPRI(priest)->shrpos.y;

	gx += rn1(3,-1);	/* mill around the altar */
	gy += rn1(3,-1);

	if(!priest->mpeaceful || priest->mberserk ||
	   (Conflict && !resist(priest, RING_CLASS, 0, 0))) {
		if(monnear(priest, u.ux, u.uy) && !priest->mflee) {
			if(Displaced)
				Your("displaced image doesn't fool %s!",
					mon_nam(priest));
			(void) mattacku(priest);
			return(0);
		} else if(index(u.urooms, temple)) {
			/* chase player if inside temple & can see him */
			if(!is_blind(priest) && m_canseeu(priest)) {
				gx = u.ux;
				gy = u.uy;
			}
			avoid = priest->mflee;
		}
	} else if(Invis) avoid = FALSE;

	return(move_special(priest,FALSE,TRUE,FALSE,avoid,omx,omy,gx,gy));
}

/* exclusively for when there is an altar to a god */
void
priestini(lvl, sroom, sx, sy, sanctum)
d_level	*lvl;
struct mkroom *sroom;
int sx, sy;
int sanctum;   /* is it the seat of the high priest? */
{
	struct monst *priest;
	struct obj *otmp;
	int cnt;

	if(MON_AT(sx+1, sy))
		(void) rloc(m_at(sx+1, sy), FALSE); /* insurance */
	priest = god_priest(a_gnum(sx, sy), sx, sy, sanctum);
	if (priest) {
		add_mx(priest, MX_EPRI);
		EPRI(priest)->shroom = (sroom - rooms) + ROOMOFFSET;
		EPRI(priest)->shralign = a_align(sx, sy);
		EPRI(priest)->godnum = a_gnum(sx, sy);
		switch(EPRI(priest)->godnum){
			case GOD_THE_COLLEGE:
				EPRI(priest)->godnum = GOD_PTAH;
			break;
			case GOD_THE_CHOIR:
				EPRI(priest)->godnum = GOD_THOTH;
			break;
			case GOD_DEFILEMENT:
				EPRI(priest)->godnum = GOD_ANHUR;
			break;
		}
		EPRI(priest)->shrpos.x = sx;
		EPRI(priest)->shrpos.y = sy;
		assign_level(&(EPRI(priest)->shrlevel), lvl);
		priest->mtrapseen = ~0;	/* traps are known */
		if(In_mordor_depths(&u.uz)){
			priest->ispriest = 1;
			priest->msleeping = 0;
		    (void) mpickobj(priest, mksobj(SPE_FIREBALL, MKOBJ_NOINIT));
		} else if(Is_bridge_temple(&u.uz)){
			priest->ispriest = 1;
			priest->msleeping = 0;
			priest->mpeaceful = 0;
		} else {
			priest->mpeaceful = 1;
			priest->ispriest = 1;
			priest->msleeping = 0;
		}
		set_malign(priest); /* mpeaceful or murderability may have changed */

		/* now his/her goodies... */
		if(sanctum && EPRI(priest)->shralign == A_NONE &&
		     on_level(&sanctum_level, &u.uz)) {
			(void) mongets(priest, AMULET_OF_YENDOR, NO_MKOBJ_FLAGS);
		}
		/* 2 to 4 spellbooks */
		for (cnt = rn1(3,2); cnt > 0; --cnt) {
		    (void) mpickobj(priest, mkobj(SPBOOK_CLASS, FALSE));
		}
		/* robe [via makemon()] */
		if (rn2(2) && (otmp = which_armor(priest, W_ARMC)) != 0) {
		    if (p_coaligned(priest))
			uncurse(otmp);
		    else
			curse(otmp);
		}
	}
	if(In_quest(&u.uz) && u.uz.dlevel == nemesis_level.dlevel && Role_if(PM_EXILE)){
		int qpm = NON_PM;
		const int *minions;
		if(a_align(sx, sy) == A_LAWFUL){
			makemon(&mons[roles[flags.panLgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panLgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panLgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panLgod].guardnum], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panLgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panLgod].malenum : 
				roles[flags.panLgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panLgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panLgod].malenum : 
				roles[flags.panLgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			minions = god_minions(align_to_god(A_LAWFUL));
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
		} else if(a_align(sx, sy) == A_CHAOTIC){
			makemon(&mons[roles[flags.panCgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panCgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panCgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panCgod].guardnum], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panCgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panCgod].malenum : 
				roles[flags.panCgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panCgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panCgod].malenum : 
				roles[flags.panCgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			minions = god_minions(align_to_god(A_CHAOTIC));
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
		} else if(a_align(sx, sy) == A_NEUTRAL){
			makemon(&mons[roles[flags.panNgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panNgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panNgod].guardnum], sx, sy, MM_ADJACENTOK);
			makemon(&mons[roles[flags.panNgod].guardnum], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panNgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panNgod].malenum : 
				roles[flags.panNgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			qpm = (roles[flags.panNgod].femalenum == NON_PM || !rn2(2)) ? 
				roles[flags.panNgod].malenum : 
				roles[flags.panNgod].femalenum;
			makemon(&mons[qpm], sx, sy, MM_ADJACENTOK);
			minions = god_minions(align_to_god(A_NEUTRAL));
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
			makemon(&mons[minions[0]], sx, sy, MM_ADJACENTOK);
		}
	}
}

/*
 * Specially aligned monsters are named specially.
 *	- aligned priests with ispriest and high priests have shrines
 *		they retain ispriest and epri when polymorphed
 *	- aligned priests without ispriest and Angels are roamers
 *		they retain isminion and access epri as emin when polymorphed
 *		(coaligned Angels are also created as minions, but they
 *		use the same naming convention)
 *	- minions do not have ispriest but have isminion and emin
 *	- caller needs to inhibit Hallucination if it wants to force
 *		the true name even when under that influence
 */
char *
priestname(mon, pname)
register struct monst *mon;
char *pname;		/* caller-supplied output buffer */
{
	const char *what = Hallucination ? rndmonnam() : mon->data->mname;
	int align = (get_mx(mon, MX_EPRI) ? EPRI(mon)->shralign : get_mx(mon, MX_EMIN) ? EMIN(mon)->min_align : 0);
	int godnum = (get_mx(mon, MX_EPRI) ? EPRI(mon)->godnum : get_mx(mon, MX_EMIN) ? EMIN(mon)->godnum : GOD_NONE);
	const char *godnm = godname(godnum);

	Strcpy(pname, "the ");
	if (mon->minvis) Strcat(pname, "invisible ");
	if ((mon->ispriest && mon->mtyp != PM_HIGH_SHAMAN && mon->mtyp != PM_KOBOLD_HIGH_SHAMAN) || mon->mtyp == PM_ALIGNED_PRIEST || mon->mtyp == PM_ANGEL)
	{
		if (mon->mtame && mon->mtyp == PM_ANGEL)
			Strcat(pname, "guardian ");
		if (mon->mtyp == PM_ANGEL) {
			Strcat(pname, what);
			Strcat(pname, " ");
		}
		if (mon->mtyp != PM_ANGEL) {
			if (!mon->ispriest && mon->malign > 0 && align == u.ualign.type)
				Strcat(pname, "renegade ");
			if (mon->mtyp == PM_HIGH_PRIEST)
				Strcat(pname, "high ");
			if (mon->mtyp == PM_ELDER_PRIEST)
				Strcat(pname, "elder ");
			if (Hallucination)
				Strcat(pname, "poohbah ");
			else if (mon->female)
				Strcat(pname, "priestess ");
			else
				Strcat(pname, "priest ");
		}
		Strcat(pname, "of ");
		/* Astral Call bugfix by Patric Muller, tweaked by CM*/
		if (mon->mtyp == PM_HIGH_PRIEST && !Hallucination &&
		            Is_astralevel(&u.uz) && distu(mon->mx, mon->my) > 2) {
			Strcat(pname, "a whole faith");
//			Strcat(pname, "?");
		} else {
	 		Strcat(pname, godnm);
		}
		return(pname);
	}
	Strcat(pname, what);
	if(!strstr(what,godnm)){
		Strcat(pname, " of ");
		Strcat(pname, godnm);
	}
	return(pname);
}

boolean
p_coaligned(priest)
struct monst *priest;
{
	return((boolean)(u.ualign.type == ((int)EPRI(priest)->shralign) && (!(EPRI(priest)->godnum) || u.ugodbase[UGOD_CURRENT] == EPRI(priest)->godnum)));
}

STATIC_OVL boolean
has_shrine(pri)
struct monst *pri;
{
	int x, y;

	if(!pri)
		return(FALSE);
	x = EPRI(pri)->shrpos.x;
	y = EPRI(pri)->shrpos.y;
	if (!IS_ALTAR(levl[x][y].typ) || !(a_shrine(x, y)))
		return(FALSE);
	return((boolean)(EPRI(pri)->shralign == a_align(x, y)));
}

struct monst *
findpriest(roomno)
char roomno;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if(mtmp->ispriest && (EPRI(mtmp)->shroom == roomno) &&
	       histemple_at(mtmp,mtmp->mx,mtmp->my))
		return(mtmp);
	}
	return (struct monst *)0;
}

/* called from check_special_room() when the player enters the temple room */
void
intemple(roomno)
register int roomno;
{
	register struct monst *priest = findpriest((char)roomno);
	boolean tended = (priest != (struct monst *)0);
	boolean shrined, sanctum, can_speak;
	const char *msg1, *msg2;
	char buf[BUFSZ];

	if(!temple_occupied(u.urooms0)) {
	    if(tended) {
			shrined = has_shrine(priest);
			sanctum = ( (priest->mtyp == PM_HIGH_PRIEST || priest->mtyp == PM_ELDER_PRIEST) &&
				   (Is_sanctum(&u.uz) || In_endgame(&u.uz)));
			can_speak = (priest->mcanmove && priest->mnotlaugh && !priest->msleeping &&
					 flags.soundok && priest->mtyp != PM_BLASPHEMOUS_LURKER);
			if (can_speak) {
				unsigned save_priest = priest->ispriest;
				/* don't reveal the altar's owner upon temple entry in
				   the endgame; for the Sanctum, the next message names
				   Moloch so suppress the "of Moloch" for him here too */
				if (sanctum && !Hallucination) priest->ispriest = 0;
				pline("%s intones:",
				canseemon(priest) ? Monnam(priest) : "A nearby voice");
				priest->ispriest = save_priest;
			}
			msg2 = 0;
			if(sanctum && Is_sanctum(&u.uz)) {
				if(priest->mpeaceful) {
					msg1 = "Infidel, you have entered Moloch's Sanctum!";
					msg2 = "Be gone!";
					priest->mpeaceful = 0;
					set_malign(priest);
				} else msg1 = "You desecrate this place by your presence!";
			} else if(Is_lolth_level(&u.uz) && !(EPRI(priest) && EPRI(priest)->godnum == GOD_LOLTH)) {
				struct monst *mtmp;
				msg1 = "Pilgrim, you enter a sacred place.";
				msg2 = "...You will die in sight of your worthless \"god\"!";
				priest->mpeaceful = 0;
				set_mon_data(priest, PM_LILITU);
				priest->m_lev = 18;
				priest->mhp = 8*17 + rn2(8);
				mongets(priest, CRYSTAL_BOOTS, NO_MKOBJ_FLAGS);
				mongets(priest, CRYSTAL_PLATE_MAIL, NO_MKOBJ_FLAGS);
				mongets(priest, CRYSTAL_GAUNTLETS, NO_MKOBJ_FLAGS);
				mongets(priest, CRYSTAL_HELM, NO_MKOBJ_FLAGS);
				mongets(priest, CRYSTAL_SWORD, NO_MKOBJ_FLAGS);
				mongets(priest, CRYSTAL_SWORD, NO_MKOBJ_FLAGS);
				newsym(priest->mx, priest->my);
				set_malign(priest);
				priest->mspec_used = 0;
				mtmp = makemon(&mons[PM_LILITU], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
				mtmp = makemon(&mons[PM_LILITU], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
				mtmp = makemon(&mons[PM_INCUBUS], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
				mtmp = makemon(&mons[PM_SUCCUBUS], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
				mtmp = makemon(&mons[PM_UNEARTHLY_DROW], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
				mtmp = makemon(&mons[PM_YOCHLOL], priest->mx, priest->my, MM_ADJACENTOK);
				if(mtmp) mtmp->mspec_used = 0;
			} else if(In_quest(&u.uz) && Role_if(PM_EXILE) && u.uz.dlevel == nemesis_level.dlevel) {
				if(priest->mpeaceful) {
					msg1 = "Your existence is blasphemy!";
					msg2 = "No more shall you challenge the Most High!";
					priest->mpeaceful = 0;
					set_malign(priest);
				} else msg1 = "You desecrate this place by your presence!";
			} else if(In_quest(&u.uz) && Role_if(PM_PRIEST) && u.uz.dlevel == qlocate_level.dlevel) {
				if(priest->mpeaceful) {
					msg1 = 0;
					msg2 = 0;
					msg1 = "You are not welcome here!";
				} else msg1 = "You desecrate this place by your presence!";
				if(canseemon(priest)) pline("As you approach, the priest suddenly putrefies and melts into a foul-smelling liquid mass!");
				set_mon_data(priest, PM_DARKNESS_GIVEN_HUNGER);
				priest->mpeaceful = 0;
				set_malign(priest);
			} else if(is_sarnath_god(EPRI(priest)->godnum) && (u.detestation_ritual&Align2ritual(EPRI(priest)->shralign))) {
				if(priest->mpeaceful) {
					msg1 = 0;
					msg2 = 0;
					msg1 = "Infidel! Traitor! You desecrate this place by your presence!";
				} else msg1 = "You desecrate this place by your presence!";
				priest->mpeaceful = 0;
				set_malign(priest);
			} else if(In_moloch_temple(&u.uz)) {
				if(priest->mpeaceful) {
					msg1 = "Infidel, you have entered Moloch's hidden temple!";
					msg2 = "Be gone!";
					priest->mpeaceful = 0;
					set_malign(priest);
				} else msg1 = "You desecrate this place by your presence!";
			} else if(Is_astralevel(&u.uz) && Role_if(PM_ANACHRONONAUT) && EPRI(priest)->shralign == A_LAWFUL) {
				if(priest->mpeaceful) {
					priest->mpeaceful = 0;
					set_malign(priest);
				}
				msg1 = "I SEE YOU!";
				set_mon_data(priest, PM_LUGRIBOSSK);
				//Assumes High Priest is 25, Lugribossk is 27 (archon)
				priest->m_lev = 27;
				priest->mhp = hd_size(priest->data)*26 + rn2(hd_size(priest->data));
				priest->mhpmax = priest->mhp;
				do_clear_area(priest->mx,priest->my, 4, set_lit, (genericptr_t)0);
				do_clear_area(u.ux,u.uy, 4, set_lit, (genericptr_t)0);
				doredraw();
				newsym(priest->mx, priest->my);
			} else if(Is_astralevel(&u.uz) && Role_if(PM_UNDEAD_HUNTER) && quest_status.moon_close) {
				if(priest->mpeaceful) {
					priest->mpeaceful = 0;
					set_malign(priest);
				}
				msg1 = "Owooooo!";
				set_mon_data(priest, PM_HIGH_PRIEST_WOLF);
				priest->m_lev = 50;
				priest->mhp = hd_size(priest->data)*49 + rn2(hd_size(priest->data));
				priest->mhpmax = priest->mhp;
				newsym(priest->mx, priest->my);
			} else {
				Sprintf(buf, "Pilgrim, you enter a %s place!",
					!shrined ? "desecrated" : "sacred");
				msg1 = buf;
			}
			if (can_speak) {
				if(msg1) verbalize1(msg1);
				if(msg2) verbalize1(msg2);
			}
			if(Is_lolth_level(&u.uz) && !(EPRI(priest) && EPRI(priest)->godnum == GOD_LOLTH)){
				pline("Rivers of blood pour from the altar!");
				if(!Is_astralevel(&u.uz)) {
					int gx = EPRI(priest)->shrpos.x;
					int gy = EPRI(priest)->shrpos.y;
					a_align(gx, gy) = A_CHAOTIC;
					a_gnum(gx, gy) = GOD_LOLTH;
				}
				make_doubtful(888L, TRUE);
				EPRI(priest)->godnum = GOD_LOLTH;
				EPRI(priest)->shralign = A_CHAOTIC;
			}
			else if(!sanctum) {
				/* !tended -> !shrined */
				if (!shrined || !p_coaligned(priest) ||
					u.ualign.record <= ALGN_SINNED)
				You("have a%s forbidding feeling...",
					(!shrined) ? "" : " strange");
				else You("experience a strange sense of peace.");
			}
		} else {
			int godnum = temple_god(roomno);
			if(philosophy_index(godnum))
				return;
			switch(rn2(3)) {
			  case 0: You("have an eerie feeling..."); break;
			  case 1: You_feel("like you are being watched."); break;
			  default: pline("A shiver runs down your %s.",
				body_part(SPINE)); break;
			}
			if(Is_astralevel(&u.uz) && Role_if(PM_ANACHRONONAUT) && temple_alignment(roomno) == A_LAWFUL) {
				struct monst *mtmp;

				if(mvitals[PM_MAANZECORIAN].born > 0 || !(mtmp = makemon(&mons[PM_MAANZECORIAN],u.ux,u.uy,MM_ADJACENTOK)))
					return;
				if (!Blind || sensemon(mtmp))
					pline("An enormous ghostly mind flayer appears next to you!");
				else You("sense a presence close by!");
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
				//Assumes Maanzecorian is 27 (archon)
				mtmp->m_lev = 27;
				mtmp->mhpmax = 8*26 + rn2(8);
				mtmp->mhp = mtmp->mhpmax;
				do_clear_area(mtmp->mx,mtmp->my, 4, set_lit, (genericptr_t)0);
				do_clear_area(u.ux,u.uy, 4, set_lit, (genericptr_t)0);
				doredraw();
				if(flags.verbose) You("are frightened to death, and unable to move.");
				nomul(-4, "being frightened to death");
				nomovemsg = "You regain your composure.";
			} else if(!rn2(5)) {
				struct monst *mtmp;

				if(!(mtmp = makemon(&mons[PM_GHOST],u.ux,u.uy,NO_MM_FLAGS)))
				return;
				if (!Blind || sensemon(mtmp))
				pline("An enormous ghost appears next to you!");
				else You("sense a presence close by!");
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
				if(flags.verbose)
				You("are frightened to death, and unable to move.");
				nomul(-3, "being frightened to death");
				nomovemsg = "You regain your composure.";
			}
		}
    }
}

void
priest_talk(priest)
register struct monst *priest;
{
	if (!get_mx(priest, MX_EPRI)) {
		verbalize("Would thou likest to repent?");
		return;
	}

	boolean coaligned = p_coaligned(priest);
	boolean strayed = (u.ualign.record < 0);
	char class_list[MAXOCLASSES+2];
	int seenSeals;

	if(!priest->mnotlaugh){
	    pline("%s is laughing uncontrollably!",
				Monnam(priest));
		return;
	}
	
	seenSeals = countCloseSigns(priest);
	EPRI(priest)->signspotted = max(seenSeals, EPRI(priest)->signspotted);
	if(seenSeals > 1){
		EPRI(priest)->pbanned = TRUE;
		if(seenSeals == 4){
			verbalize("Foul heretic!");
			makemon(&mons[PM_DAAT_SEPHIRAH], u.ux, u.uy, MM_ADJACENTOK);
			priest->mpeaceful=0;
		} else if(seenSeals == 5){
			coord mm;
			verbalize("Foul heretic! The Lord's servants shall humble you!");
			priest->mpeaceful=0;
			summon_god_minion(EPRI(priest)->godnum, FALSE);
			makemon(&mons[PM_DAAT_SEPHIRAH], u.ux, u.uy, MM_ADJACENTOK);
			makemon(&mons[PM_DAAT_SEPHIRAH], u.ux, u.uy, MM_ADJACENTOK);
		} else if(seenSeals >= 6){
			coord mm;
			verbalize("Foul heretic! The Lord's servants shall humble you!");
			priest->mpeaceful=0;
			summon_god_minion(EPRI(priest)->godnum, FALSE);
			summon_god_minion(EPRI(priest)->godnum, FALSE);
			summon_god_minion(EPRI(priest)->godnum, FALSE);
			makemon(&mons[PM_DAAT_SEPHIRAH], u.ux, u.uy, MM_ADJACENTOK);
			makemon(&mons[PM_DAAT_SEPHIRAH], u.ux, u.uy, MM_ADJACENTOK);
			/* Create swarm near down staircase (hinders return to level) */
			mm.x = xdnstair;
			mm.y = ydnstair;
			makemon(&mons[PM_DAAT_SEPHIRAH], mm.x, mm.y, MM_ADJACENTOK);
			makemon(&mons[PM_DAAT_SEPHIRAH], mm.x, mm.y, MM_ADJACENTOK);
			makeketer(&mm);
			/* Create swarm near up staircase (hinders retreat from level) */
			mm.x = xupstair;
			mm.y = yupstair;
			makemon(&mons[PM_DAAT_SEPHIRAH], mm.x, mm.y, MM_ADJACENTOK);
			makemon(&mons[PM_DAAT_SEPHIRAH], mm.x, mm.y, MM_ADJACENTOK);
			makeketer(&mm);
		}
	}
	
	if(EPRI(priest)->pbanned && seenSeals){
		verbalize("Your kind are anathema.");
		return;
	}
	if(EPRI(priest)->pbanned){
		verbalize("You have been excommunicated.");
		return;
	}
	if(seenSeals){
		verbalize("I do not offer blessings to your kind.");
		return;
	}
	if(Misotheism){
		pline("%s looks extremely confused, and does not reply.", Monnam(priest));
		return;
	}
	
	/* KMH, conduct */
	u.uconduct.gnostic++;

	if(priest->mflee || (!priest->ispriest && coaligned && strayed)) {
	    pline("%s doesn't want anything to do with you!",
				Monnam(priest));
	    return;
	}

	/* priests don't chat unless peaceful and in their own temple */
	if(!histemple_at(priest,priest->mx,priest->my) ||
		 !priest->mpeaceful || !priest->mcanmove || priest->msleeping) {
	    static const char * const cranky_msg[3] = {
		"Thou wouldst have words, eh?  I'll give thee a word or two!",
		"Talk?  Here is what I have to say!",
		"Pilgrim, I would speak no longer with thee."
	    };

	    if(!priest->mcanmove || priest->msleeping) {
		pline("%s breaks out of %s reverie!",
		      Monnam(priest), mhis(priest));
		priest->mfrozen = priest->msleeping = 0;
		priest->mcanmove = 1;
	    }
	    verbalize1(cranky_msg[rn2(3)]);
	    return;
	}

	/* you desecrated the temple and now you want to chat? */
	if(priest->mpeaceful && *in_rooms(priest->mx, priest->my, TEMPLE) &&
		  !has_shrine(priest)) {
	    verbalize("Begone!  Thou desecratest this holy place with thy presence.");
	    priest->mpeaceful = 0;
	    return;
	}
#ifndef GOLDOBJ
	if(!u.ugold) {
	    if(coaligned && !strayed) {
		if(uclockwork && u.uhs >= WEAK &&
		   yn("Shall I wind your clockwork, brother?") == 'y'){
			struct obj *key;
			Strcpy(class_list, tools);
			key = getobj(class_list, "wind with");
			start_clockwinding(key, priest, 10);
		} else if (priest->mgold > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
		    pline("%s gives you %s for an ale.", Monnam(priest),
			(priest->mgold == 1L) ? "one bit" : "two bits");
		    if (priest->mgold > 1L)
			u.ugold = 2L;
		    else
			u.ugold = 1L;
		    priest->mgold -= u.ugold;
		    flags.botl = 1;
#else
	if(!money_cnt(invent)) {
	    if(coaligned && !strayed) {
                long pmoney = money_cnt(priest->minvent);
		if(uclockwork && u.uhs >= WEAK &&
		   yn("Shall I wind your clockwork, brother?") == 'y'){
			struct obj *key;
			Strcpy(class_list, tools);
			key = getobj(class_list, "wind with");
			if (!key){
				pline(Never_mind);
				return;
			}
			start_clockwinding(key, priest, 10);
		} else if (pmoney > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
		    pline("%s gives you %s for an ale.", Monnam(priest),
			(pmoney == 1L) ? "one bit" : "two bits");
		     money2u(priest, pmoney > 1L ? 2 : 1);
#endif
		} else
		    pline("%s preaches the virtues of poverty.", Monnam(priest));
		exercise(A_WIS, TRUE);
	    } else
		pline("%s is not interested.", Monnam(priest));
	    return;
	} else if(uclockwork &&
	   yn("Shall I wind your clockwork, pilgrim?") == 'y'){
		struct obj *key;
		int turns = 0;
		
		Strcpy(class_list, tools);
		key = getobj(class_list, "wind with");
		if (!key){
			pline1(Never_mind);
			return;
		}
		turns = ask_turns(priest, u.ulevel*10+100, 0);
		if(!turns){
			pline1(Never_mind);
			return;
		}
		start_clockwinding(key, priest, turns);
	} else {
	    long offer;

	    pline("%s asks you for a contribution for the temple.",
			Monnam(priest));
	    if((offer = bribe(priest)) == 0) {
		verbalize("Thou shalt regret thine action!");
		if(coaligned) adjalign(-1);
	    } else if(offer < (u.ulevel * 200)) {
		if(u.sealsActive&SEAL_AMON) unbind(SEAL_AMON,TRUE);
#ifndef GOLDOBJ
		if(u.ugold > (offer * 2L)) verbalize("Cheapskate.");
#else
		if(money_cnt(invent) > (offer * 2L)) verbalize("Cheapskate.");
#endif
		else {
		    verbalize("I thank thee for thy contribution.");
		    /*  give player some token  */
		    exercise(A_WIS, TRUE);
		}
	    } else if(offer < (u.ulevel * 400)) {
		if(u.sealsActive&SEAL_AMON) unbind(SEAL_AMON,TRUE);
		verbalize("Thou art indeed a pious individual.");
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L))
#else
		if(money_cnt(invent) < (offer * 2L))
#endif
		{
		    if (coaligned && u.ualign.record <= ALGN_SINNED)
			adjalign(1);
		    verbalize("I bestow upon thee a blessing.");
		    incr_itimeout(&HClairvoyant, rn1(500,500));
		}
	    } else if(offer < (u.ulevel * 600) &&
		      u.ublessed < 20 &&
		      (u.ublessed < 9 || !rn2(u.ublessed))) {
		if(u.sealsActive&SEAL_AMON) unbind(SEAL_AMON,TRUE);
		verbalize("Thy devotion has been rewarded.");
		if (!(HProtection & INTRINSIC))  {
			HProtection |= FROMOUTSIDE;
			if (!u.ublessed)  u.ublessed = rn1(3, 2);
		} else u.ublessed++;
	    } else {
		if(u.sealsActive&SEAL_AMON) unbind(SEAL_AMON,TRUE);
		verbalize("Thy selfless generosity is deeply appreciated.");
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L) && coaligned)
#else
		if(money_cnt(invent) < (offer * 2L) && coaligned)
#endif
		{
		    if(strayed && (moves - u.ucleansed) > 5000L) {
			u.ualign.record = 0; /* cleanse thee */
			u.ucleansed = moves;
		    } else {
			adjalign(2);
		    }
		}
	    }
	}
}

struct monst *
mk_roamer(ptr, alignment, x, y, peaceful)
register struct permonst *ptr;
aligntyp alignment;
xchar x, y;
boolean peaceful;
{
	register struct monst *roamer;
	register boolean coaligned = (u.ualign.type == alignment);

	//if ptr is null don't make a monster
	if(!ptr)
		return((struct monst *)0);

	if (ptr->mtyp != PM_ALIGNED_PRIEST && ptr->mtyp != PM_ANGEL)
		return((struct monst *)0);
	
	if (MON_AT(x, y)) (void) rloc(m_at(x, y), FALSE);	/* insurance */

	if (!(roamer = makemon(ptr, x, y, NO_MM_FLAGS)))
		return((struct monst *)0);

	add_mx(roamer, MX_EMIN);
	EMIN(roamer)->min_align = alignment;
	int gnum = align_to_god(alignment);
	if(gnum == GOD_THE_COLLEGE)
		gnum = GOD_PTAH;
	else if(gnum == GOD_THE_CHOIR)
		gnum = GOD_THOTH;
	else if(gnum == GOD_DEFILEMENT)
		gnum = GOD_ANHUR;
	EMIN(roamer)->godnum = gnum;

	/* Binders, on astral, should be beset by a wide variety of gods' angels -- overwrite godnum */
	if (Role_if(PM_EXILE) && Is_astralevel(&u.uz) && alignment != A_NONE) {
		do {
			int gnum = rnd(MAX_GOD);
			if(gnum == GOD_THE_COLLEGE)
				gnum = GOD_PTAH;
			else if(gnum == GOD_THE_CHOIR)
				gnum = GOD_THOTH;
			else if(gnum == GOD_DEFILEMENT)
				gnum = GOD_ANHUR;
			EMIN(roamer)->godnum = gnum;
		} while(galign(EMIN(roamer)->godnum) != alignment);
	}

	roamer->isminion = TRUE;
	roamer->mtrapseen = ~0;		/* traps are known */
	roamer->mpeaceful = peaceful;
	roamer->msleeping = 0;
	set_malign(roamer); /* peaceful may have changed */

	/* MORE TO COME */
	return(roamer);
}

void
reset_hostility(roamer)
register struct monst *roamer;
{
	if(!(roamer->isminion && (roamer->mtyp == PM_ALIGNED_PRIEST ||
				  roamer->mtyp == PM_ANGEL)))
	        return;

	int alignment = (get_mx(roamer, MX_EPRI) ? EPRI(roamer)->shralign : get_mx(roamer, MX_EMIN) ? EMIN(roamer)->min_align : 0);

	if (alignment != u.ualign.type) {
		untame(roamer, 0);
	    set_malign(roamer);
	}
	newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(mon, x, y)
struct monst *mon;	/* if non-null, <mx,my> overrides <x,y> */
xchar x, y;
{
	register char roomno;
	register struct monst *priest;

	if (mon) {
	    if (is_minion(mon->data) || is_rider(mon->data)) return FALSE;
	    x = mon->mx, y = mon->my;
	}
	if (u.ualign.record <= ALGN_SINNED)	/* sinned or worse */
	    return FALSE;
	if ((roomno = temple_occupied(u.urooms)) == 0 ||
		roomno != *in_rooms(x, y, TEMPLE))
	    return FALSE;
	if ((priest = findpriest(roomno)) == 0)
	    return FALSE;
	return (boolean)(has_shrine(priest) &&
			 p_coaligned(priest) &&
			 priest->mpeaceful);
}

void
ghod_hitsu(priest)	/* when attacking "priest" in his temple */
struct monst *priest;
{
	int x, y, ax, ay, roomno = (int)temple_occupied(u.urooms);
	struct mkroom *troom;
	static long last_smite = 0L;

	if (!roomno || !has_shrine(priest))
		return;

	/* max 1 smite per global turn */
	if (last_smite >= monstermoves)
		return;

	ax = x = EPRI(priest)->shrpos.x;
	ay = y = EPRI(priest)->shrpos.y;
	troom = &rooms[roomno - ROOMOFFSET];

	if((u.ux == x && u.uy == y) || !linedup(u.ux, u.uy, x, y)) {
	    if(IS_DOOR(levl[u.ux][u.uy].typ)) {

		if(u.ux == troom->lx - 1) {
		    x = troom->hx;
		    y = u.uy;
		} else if(u.ux == troom->hx + 1) {
		    x = troom->lx;
		    y = u.uy;
		} else if(u.uy == troom->ly - 1) {
		    x = u.ux;
		    y = troom->hy;
		} else if(u.uy == troom->hy + 1) {
		    x = u.ux;
		    y = troom->ly;
		}
	    } else {
		switch(rn2(4)) {
		case 0:  x = u.ux; y = troom->ly; break;
		case 1:  x = u.ux; y = troom->hy; break;
		case 2:  x = troom->lx; y = u.uy; break;
		default: x = troom->hx; y = u.uy; break;
		}
	    }
	    if(!linedup(u.ux, u.uy, x, y)) return;
	}

	switch(rn2(3)) {
	case 0:
	    pline("%s roars in anger:  \"Thou shalt suffer!\"",
			a_gname_at(ax, ay));
	    break;
	case 1:
	    pline("%s voice booms:  \"How darest thou harm my servant!\"",
			s_suffix(a_gname_at(ax, ay)));
	    break;
	default:
	    pline("%s roars:  \"Thou dost profane my shrine!\"",
			a_gname_at(ax, ay));
	    break;
	}

	zap(priest, x, y, sgn(tbx), sgn(tby), rn1(7, 7), basiczap(0, AD_ELEC, ZAP_SPELL, 6));

	exercise(A_WIS, FALSE);
	last_smite = monstermoves;
}

void
angry_priest()
{
	register struct monst *priest;
	int x, y;

	if ((priest = findpriest(temple_occupied(u.urooms))) != 0) {
	    wakeup(priest, TRUE);
	    /*
	     * If the altar has been destroyed or converted, let the
	     * priest run loose.
	     * (When it's just a conversion and there happens to be
	     *	a fresh corpse nearby, the priest ought to have an
	     *	opportunity to try converting it back; maybe someday...)
	     */
		x = EPRI(priest)->shrpos.x;
		y = EPRI(priest)->shrpos.y;
	    if (!IS_ALTAR(levl[x][y].typ) || (a_gnum(x, y) != EPRI(priest)->godnum)) {
		priest->ispriest = 0;		/* now a roamer */
		priest->isminion = 1;		/* but still aligned */
		/* this overloads the `shroom' field, which is now clobbered */
		EPRI(priest)->renegade = 0;
	    }
	}
}

/*
 * When saving bones, find priests that aren't on their shrine level,
 * and remove them.   This avoids big problems when restoring bones.
 */
void
clearpriests()
{
    register struct monst *mtmp, *mtmp2;

    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	mtmp2 = mtmp->nmon;
	if (!DEADMONSTER(mtmp) && mtmp->ispriest && !on_level(&(EPRI(mtmp)->shrlevel), &u.uz))
	    mongone(mtmp);
    }
}


#endif /* OVLB */

/*priest.c*/
