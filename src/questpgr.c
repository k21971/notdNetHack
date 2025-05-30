/*	SCCS Id: @(#)questpgr.c	3.4	2000/05/05	*/
/*	Copyright 1991, M. Stephenson		  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*  quest-specific pager routines. */

extern const int monstr[];
#include "qtext.h"

#define QTEXT_FILE	"quest.dat"

/* #define DEBUG */	/* uncomment for debugging */

static void FDECL(Fread, (genericptr_t,int,int,dlb *));
STATIC_DCL struct qtmsg * FDECL(construct_qtlist, (long));
STATIC_DCL const char * NDECL(intermed);
STATIC_DCL const char * NDECL(neminame);
STATIC_DCL const char * NDECL(guardname);
STATIC_DCL const char * NDECL(homebase);
STATIC_DCL struct qtmsg * FDECL(msg_in, (struct qtmsg *,int));
STATIC_DCL void FDECL(convert_arg, (CHAR_P));
STATIC_DCL void NDECL(convert_line);
STATIC_DCL void FDECL(deliver_by_pline, (struct qtmsg *));
STATIC_DCL void FDECL(deliver_by_window, (struct qtmsg *,int));

static char	in_line[80], cvt_buf[64], out_line[128];
static struct	qtlists	qt_list;
static dlb	*msg_file;
/* used by ldrname() and neminame(), then copied into cvt_buf */
static char	nambuf[sizeof cvt_buf];

#ifdef DEBUG
static void NDECL(dump_qtlist);

static void
dump_qtlist()	/* dump the character msg list to check appearance */
{
	struct	qtmsg	*msg;
	long	size;

	for (msg = qt_list.chrole; msg->msgnum > 0; msg++) {
		pline("msgnum %d: delivery %c",
			msg->msgnum, msg->delivery);
		more();
		(void) dlb_fseek(msg_file, msg->offset, SEEK_SET);
		deliver_by_window(msg, NHW_TEXT);
	}
}
#endif /* DEBUG */

static void
Fread(ptr, size, nitems, stream)
genericptr_t	ptr;
int	size, nitems;
dlb	*stream;
{
	int cnt;

	if ((cnt = dlb_fread(ptr, size, nitems, stream)) != nitems) {

	    panic("PREMATURE EOF ON QUEST TEXT FILE! Expected %d bytes, got %d",
		    (size * nitems), (size * cnt));
	}
}

STATIC_OVL struct qtmsg *
construct_qtlist(hdr_offset)
long	hdr_offset;
{
	struct qtmsg *msg_list;
	int	n_msgs;

	(void) dlb_fseek(msg_file, hdr_offset, SEEK_SET);
	Fread(&n_msgs, sizeof(int), 1, msg_file);
	msg_list = (struct qtmsg *)
		alloc((unsigned)(n_msgs+1)*sizeof(struct qtmsg));

	/*
	 * Load up the list.
	 */
	Fread((genericptr_t)msg_list, n_msgs*sizeof(struct qtmsg), 1, msg_file);

	msg_list[n_msgs].msgnum = -1;
	return(msg_list);
}

void
load_qtlist()
{

	int	n_classes, i;
	char	qt_classes[N_HDR][LEN_HDR];
	long	qt_offsets[N_HDR];

	msg_file = dlb_fopen(QTEXT_FILE, RDBMODE);
	if (!msg_file)
	    panic("CANNOT OPEN QUEST TEXT FILE %s.", QTEXT_FILE);

	/*
	 * Read in the number of classes, then the ID's & offsets for
	 * each header.
	 */

	Fread(&n_classes, sizeof(int), 1, msg_file);
	Fread(&qt_classes[0][0], sizeof(char)*LEN_HDR, n_classes, msg_file);
	Fread(qt_offsets, sizeof(long), n_classes, msg_file);

	/*
	 * Now construct the message lists for quick reference later
	 * on when we are actually paging the messages out.
	 */

	qt_list.common = qt_list.chrole = (struct qtmsg *)0;

	for (i = 0; i < n_classes; i++) {
	    if (!strncmp(COMMON_ID, qt_classes[i], LEN_HDR))
	    	qt_list.common = construct_qtlist(qt_offsets[i]);
	    else if (!strncmp(urole.filecode, qt_classes[i], LEN_HDR))
	    	qt_list.chrole = construct_qtlist(qt_offsets[i]);
#if 0	/* UNUSED but available */
	    else if (!strncmp(urace.filecode, qt_classes[i], LEN_HDR))
	    	qt_list.chrace = construct_qtlist(qt_offsets[i]);
#endif
	}

	if (!qt_list.common || !qt_list.chrole)
	    impossible("load_qtlist: cannot load quest text.");
#ifdef DEBUG
	dump_qtlist();
#endif
	return;	/* no ***DON'T*** close the msg_file */
}

/* called at program exit */
void
unload_qtlist()
{
	if (msg_file)
	    (void) dlb_fclose(msg_file),  msg_file = 0;
	if (qt_list.common)
	    free((genericptr_t) qt_list.common),  qt_list.common = 0;
	if (qt_list.chrole)
	    free((genericptr_t) qt_list.chrole),  qt_list.chrole = 0;
	return;
}

short
quest_info(typ)
int typ;
{
	switch (typ) {
	    case 0:		return (urole.questarti);
	    case MS_LEADER:	return (urole.ldrnum);
	    case MS_NEMESIS:	return (urole.neminum);
	    case MS_GUARDIAN:	return (urole.guardnum);
	    default:		impossible("quest_info(%d)", typ);
	}
	return 0;
}

const char *
ldrname()	/* return your role leader's name */
{
	int i = urole.ldrnum;

	Sprintf(nambuf, "%s%s",
		type_is_pname(&mons[i]) ? "" : "the ",
		mons[i].mname);
	return nambuf;
}

STATIC_OVL const char *
intermed()	/* return your intermediate target string */
{
	return (urole.intermed);
}

boolean
is_primary_quest_artifact(otmp)
struct obj *otmp;
{
	return((boolean)(otmp->oartifact == urole.questarti || 
		(Race_if(PM_DROW) && (Role_if(PM_PRIEST) || Role_if(PM_ROGUE) || Role_if(PM_RANGER) || Role_if(PM_WIZARD)) && (
			 (otmp->oartifact == ART_TENTACLE_ROD && flags.stag) || 
			 (otmp->oartifact == ART_SILVER_STARLIGHT && flags.initgend && !flags.stag) || 
			 (otmp->oartifact == ART_DARKWEAVER_S_CLOAK && !flags.initgend && !flags.stag)
		))
	));
}

boolean
is_quest_artifact(otmp)
struct obj *otmp;
{
	return((boolean)(otmp->oartifact == urole.questarti || 
		(Race_if(PM_DROW) && (Role_if(PM_PRIEST) || Role_if(PM_ROGUE) || Role_if(PM_RANGER) || Role_if(PM_WIZARD)) && 
			(
			 (otmp->oartifact == ART_TENTACLE_ROD && flags.stag) || 
			 (otmp->oartifact == ART_CRESCENT_BLADE && flags.initgend && flags.stag) || 
			 (otmp->oartifact == ART_SILVER_STARLIGHT && flags.initgend && !flags.stag) || 
			 (otmp->oartifact == ART_WRATHFUL_SPIDER && flags.initgend && !flags.stag) || 
			 
			 (otmp->oartifact == ART_WEBWEAVER_S_CROOK && !flags.initgend && flags.stag) || 
			 (otmp->oartifact == ART_DARKWEAVER_S_CLOAK && !flags.initgend && !flags.stag) || 
			 (otmp->oartifact == ART_SPIDERSILK && !flags.initgend && !flags.stag)
			)
		) ||
		(Race_if(PM_ELF) && (Role_if(PM_RANGER) || Role_if(PM_WIZARD) || Role_if(PM_PRIEST) || Role_if(PM_NOBLEMAN)) && 
			(otmp->oartifact == ART_BELTHRONDING || otmp->oartifact == ART_PALANTIR_OF_WESTERNESSE)
		)
	));
}

STATIC_OVL const char *
neminame()	/* return your role nemesis' name */
{
	int i = urole.neminum;

	Sprintf(nambuf, "%s%s",
		type_is_pname(&mons[i]) ? "" : "the ",
		mons[i].mname);
	return nambuf;
}

STATIC_OVL const char *
guardname()	/* return your role leader's guard monster name */
{
	int i = urole.guardnum;

	return(mons[i].mname);
}

STATIC_OVL const char *
homebase()	/* return your role leader's location */
{
	return(urole.homebase);
}

STATIC_OVL struct qtmsg *
msg_in(qtm_list, msgnum)
struct qtmsg *qtm_list;
int	msgnum;
{
	struct qtmsg *qt_msg;

	for (qt_msg = qtm_list; qt_msg->msgnum > 0; qt_msg++)
	    if (qt_msg->msgnum == msgnum) return(qt_msg);

	return((struct qtmsg *)0);
}

STATIC_OVL void
convert_arg(c)
char c;
{
	register const char *str;

	switch (c) {

	    case 'p':	str = plname;
			break;
	    case 'c':	str = (flags.female && urole.name.f) ?
	    			urole.name.f : urole.name.m;
			break;
	    case 'r':	str = rank_of(u.ulevel, Role_switch, flags.female);
			break;
	    case 'R':	str = rank_of(MIN_QUEST_LEVEL, Role_switch,
	    			flags.female);
			break;
	    case 's':	str = (flags.female) ? "sister" : "brother";
			break;
	    case 'S':	str = (flags.female) ? "daughter" : "son";
			break;
	    case 'l':	str = ldrname();
			break;
	    case 'i':	str = intermed();
			break;
	    case 'o':	str = the(artiname(urole.questarti));
			break;
	    case 'n':	str = neminame();
			break;
	    case 'g':	str = guardname();
			break;
	    case 'G':	str = gtitle(u.ugodbase[UGOD_ORIGINAL]);
			break;
	    case 'h':	str = (flags.female) ?  urace.individual.f : urace.individual.m;
			break;
	    case 'H':	str = homebase();
			break;
	    case 'a':	str = align_str(galign(u.ugodbase[UGOD_ORIGINAL]));
			break;
	    case 'A':	str = align_str(u.ualign.type);
			break;
	    case 'd':	str = align_gname(galign(u.ugodbase[UGOD_ORIGINAL]));
			break;
	    case 'D':	str = align_gname(A_LAWFUL);
			break;
	    case 'C':	str = "chaotic";
			break;
	    case 'N':	str = "neutral";
			break;
	    case 'L':	str = "lawful";
			break;
	    case 'x':	str = Blind ? "sense" : "see";
			break;
	    case 'Z':	str = dungeons[0].dname;
			break;
	    case '%':	str = "%";
			break;
	     default:	str = "";
			break;
	}
	Strcpy(cvt_buf, str);
}

STATIC_OVL void
convert_line()
{
	char *c, *cc;
	char xbuf[BUFSZ];

	cc = out_line;
	for (c = xcrypt(in_line, xbuf); *c; c++) {

	    *cc = 0;
	    switch(*c) {

		case '\r':
		case '\n':
			*(++cc) = 0;
			return;

		case '%':
			if (*(c+1)) {
			    convert_arg(*(++c));
			    switch (*(++c)) {

					/* insert "a"/"an" prefix */
				case 'A': Strcat(cc, An(cvt_buf));
				    cc += strlen(cc);
				    continue; /* for */
				case 'a': Strcat(cc, an(cvt_buf));
				    cc += strlen(cc);
				    continue; /* for */

					/* capitalize */
				case 'C': cvt_buf[0] = highc(cvt_buf[0]);
				    break;

					/* pluralize */
				case 'P': cvt_buf[0] = highc(cvt_buf[0]);
				case 'p': Strcpy(cvt_buf, makeplural(cvt_buf));
				    break;

					/* append possessive suffix */
				case 'S': cvt_buf[0] = highc(cvt_buf[0]);
				case 's': Strcpy(cvt_buf, s_suffix(cvt_buf));
				    break;

					/* strip any "the" prefix */
				case 't': if (!strncmpi(cvt_buf, "the ", 4)) {
					Strcat(cc, &cvt_buf[4]);
					cc += strlen(cc);
					continue; /* for */
				    }
				    break;

				default: --c;	/* undo switch increment */
				    break;
			    }
			    Strcat(cc, cvt_buf);
			    cc += strlen(cvt_buf);
			    break;
			}	/* else fall through */

		default:
			*cc++ = *c;
			break;
	    }
	}
	if (cc >= out_line + sizeof out_line)
	    panic("convert_line: overflow");
	*cc = 0;
	return;
}

STATIC_OVL void
deliver_by_pline(qt_msg)
struct qtmsg *qt_msg;
{
	long	size;

	for (size = 0; size < qt_msg->size; size += (long)strlen(in_line)) {
	    (void) dlb_fgets(in_line, 80, msg_file);
	    convert_line();
	    pline1(out_line);
	}

}

STATIC_OVL void
deliver_by_window(qt_msg, how)
struct qtmsg *qt_msg;
int how;
{
	long	size;
	winid datawin = create_nhwindow(how);

	for (size = 0; size < qt_msg->size; size += (long)strlen(in_line)) {
	    (void) dlb_fgets(in_line, 80, msg_file);
	    convert_line();
	    putstr(datawin, 0, out_line);
	}
	display_nhwindow(datawin, TRUE);
	destroy_nhwindow(datawin);
}

void
com_pager(msgnum)
int	msgnum;
{
	struct qtmsg *qt_msg;
	
	if (!(qt_msg = msg_in(qt_list.common, msgnum))) {
		impossible("com_pager: message %d not found.", msgnum);
		return;
	}
	
	(void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
	if (qt_msg->delivery == 'p') deliver_by_pline(qt_msg);
	else if (msgnum == 1 || msgnum == 199) deliver_by_window(qt_msg, NHW_MENU);
	else		     deliver_by_window(qt_msg, NHW_TEXT);
	return;
}

void
qt_pager(msgnum)
int	msgnum;
{
	struct qtmsg *qt_msg;

	if (!(qt_msg = msg_in(qt_list.chrole, msgnum))) {
		impossible("qt_pager: message %d not found.", msgnum);
		return;
	}

	(void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
	if (qt_msg->delivery == 'p' && strcmp(windowprocs.name, "X11"))
		deliver_by_pline(qt_msg);
	else	deliver_by_window(qt_msg, NHW_TEXT);
	return;
}

void
string_pager(char *string)
{
	long	size;
	winid datawin = create_nhwindow(NHW_TEXT);
	char buf[BUFSZ];
	int c = 0;
	for(int i = 0; string[i]; i++){
		if(string[i] != '\n'){
			buf[c] = string[i];
			c++;
		}
		else {
			buf[c] = '\0';
			putstr(datawin, 0, (const char *)&buf);
			c = 0;
		}
	}
	if(c > 0){
		buf[c] = '\0';
		putstr(datawin, 0, (const char *)&buf);
	}
	display_nhwindow(datawin, TRUE);
	destroy_nhwindow(datawin);
}

struct permonst *
qt_montype(int x, int y)
{
	if(Role_if(PM_ANACHRONONAUT)){
		switch(rn2(7)){
			case 0:
				if(rn2(5)) return &mons[PM_CHANGED];
				else return &mons[PM_WARRIOR_CHANGED];
			break;
			case 1:
				if(rn2(2)) return &mons[PM_BRAIN_GOLEM];
				else if(rn2(2)) return &mons[PM_UMBER_HULK];
				else if(rn2(2)) return &mons[PM_MIND_FLAYER];
				else return &mons[PM_MASTER_MIND_FLAYER];
			break;
			case 2:
				if(rn2(100)) return &mons[PM_EDDERKOP];
				else return &mons[PM_EMBRACED_DROWESS];
			break;
			case 3:
				if(rn2(5)) return mkclass(S_UMBER, G_NOHELL|G_HELL);
				else return &mons[PM_DOPPELGANGER];
			break;
			case 4:
				if(rn2(2)) return mkclass(S_VAMPIRE, G_NOHELL|G_HELL);
				else if(rn2(2)) return mkclass(S_ZOMBIE, G_NOHELL|G_HELL);
				else if(rn2(2)) return mkclass(S_WRAITH, G_NOHELL|G_HELL);
				else if(rn2(3)) return mkclass(S_LICH, G_NOHELL|G_HELL);
				else if(Is_qstart(&u.uz) || rn2(3)) return &mons[PM_DARKNESS_GIVEN_HUNGER];
				else return &mons[PM_DREAD_SERAPH];
			break;
			case 5:
				if(rn2(2)) return &mons[PM_DEEP_ONE];
				else if(rn2(2)) return &mons[PM_DEEPER_ONE];
				else if(rn2(4)) return &mons[PM_DEEP_DWELLER];
				else return &mons[PM_DEEPEST_ONE];
			break;
			case 6:
				if(rn2(3)) return &mons[PM_PHANTASM];
				else if(rn2(3)) return &mons[PM_NEVERWAS];
				else if(rn2(3)) return &mons[PM_CARCOSAN_COURTIER];
				else if(rn2(3)) return &mons[PM_INTONER];
				else return &mons[PM_BLACK_FLOWER];
			break;
		}
	} else if(Role_if(PM_ANACHRONOUNBINDER)){
		switch(rn2(3)){
			case 0:
				return &mons[PM_ADVERSARY];
			break;
			case 1:
				if(rn2(4)) return &mons[PM_NEOTHELID];
				else return mkclass(S_WORM, G_NOHELL|G_HELL);
			break;
			case 2:
				if(rn2(4)) return &mons[PM_VILLITHID];
				else return mkclass(S_VAMPIRE, G_NOHELL|G_HELL);
			break;
			//case 3:
			//	if(rn2(2)) return &mons[PM_ADVERSARY];
			//	else return &mons[PM_ILLITHID_LARVA];
		//	break;
			default:
				return &mons[PM_VILLITHID];
				impossible("No monster?");
			break;
		}
	 } else if(Pantheon_if(PM_SALAMANDER)){
		if(on_level(&u.uz,&qstart_level) && (flags.questprogress < 1 || u.uevent.qcompleted)) return &mons[PM_LONG_WORM_TAIL];
		if(on_level(&u.uz,&qstart_level)) return &mons[PM_EFREET_OVERSEER];
		switch(rn2(3)){
			case 0:
				return &mons[PM_EFREET];
			break;
			case 1:
				if(!rn2(3)) return &mons[PM_EFREET_GUARDIAN];
				else return &mons[PM_SALAMANDER_SLAVE];
			break;
			case 2:
				if(rn2(2)) return &mons[PM_EFREET_MERCHANT];
				else return &mons[PM_FIRE_ELEMENTAL];
			break;
			case 3:
				if(rn2(3)) return &mons[PM_EFREET_OVERSEER];
				else return &mons[PM_SALAMANDER_SLAVE];
			break;
			default:
				return &mons[PM_EFREET];
				impossible("No monster?");
			break;
		}
	} else if(Role_if(PM_CAVEMAN)){
		//Very placeholdery
		switch(rnd(12)){
			case 1:
				return &mons[PM_SABER_TOOTHED_CAT];
			break;
			case 2:
				return &mons[PM_TYRANNOSAURUS];
			break;
			case 3:
				return &mons[PM_TRICERATOPS];
			break;
			case 4:
				return &mons[PM_DIPLODOCUS];
			break;
			case 5:
				return &mons[PM_TITANOTHERE];
			break;
			case 6:
				return &mons[PM_BALUCHITHERIUM];
			break;
			case 7:
				return &mons[PM_MASTODON];
			break;
			case 8:
				return &mons[PM_RED_DRAGON];
			break;
			case 9:
				return &mons[PM_GREEN_DRAGON];
			break;
			case 10:
				return &mons[PM_BLUE_DRAGON];
			break;
			case 11:
				return &mons[PM_WHITE_DRAGON];
			break;
			case 12:
				return &mons[PM_BLACK_DRAGON];
			break;
		}
	} else if(Role_if(PM_CONVICT) && ((Is_qlocate(&u.uz) && rn2(2)) || (u.uz.dlevel > qlocate_level.dlevel))){
		int qpm;
		if(rn2(5)){
			qpm = PM_QUASIT;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(S_IMP, G_HELL));
		}
		qpm = PM_DAUGHTER_OF_BEDLAM;
		if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
			return (&mons[qpm]);
		return (mkclass(S_DEMON, G_HELL));
	} else if(Role_if(PM_MADMAN) && (u.uz.dlevel >= qlocate_level.dlevel)){
		int qpm;
		if(rn2(5)){
			qpm = PM_LARGE_CAT;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(S_WORM, G_NOHELL));
		}
		qpm = PM_CONTAMINATED_PATIENT;
		if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
			return (&mons[qpm]);
		return (mkclass(S_BAT, G_NOHELL|G_HELL));
	} else if(Role_if(PM_MONK)){
		int qpm;
		int monks_of_kaen[] = {PM_XORN_MONK, PM_DAO_LAO_GUI_MONK, PM_ZHI_REN_MONK, PM_XUENU_MONK};
		if(rn2(3)){
			qpm = rn2(3) ? PM_EARTH_ELEMENTAL : PM_TERRACOTTA_SOLDIER;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(S_ELEMENTAL, G_NOHELL));
		}
		if(rn2(2)){
			qpm = PM_XORN;
			if (qpm != NON_PM && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			//Fall through in this (impossible?) case
		}
		qpm = ROLL_FROM(monks_of_kaen);
		if (qpm != NON_PM && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
			return (&mons[qpm]);
		//Should be impossible:
		return (mkclass(S_YETI, G_NOHELL));
	} else if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
		int qpm;
		int duergar[] = {PM_DUERGAR, PM_DUERGAR_STONEGUARD, PM_DUERGAR_DEBILITATOR};
		int drow_etc[] = {PM_HOUSELESS_DROW, PM_HOUSELESS_DROW, PM_HOUSELESS_DROW, PM_HOUSELESS_DROW,
					PM_SPROW, PM_SPROW, PM_DRIDER, PM_DROW_MUMMY,
					PM_HEDROW_ZOMBIE, PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_HEDROW_WIZARD,
					PM_DROW_MATRON, PM_PEN_A_MENDICANT, PM_MENDICANT_SPROW, PM_MENDICANT_DRIDER,
					PM_Y_CULTIST_MATRON, PM_Y_CULTIST_WIZARD, PM_Y_CULTIST_FIGHTER, PM_Y_CULTIST,
					PM_Y_CULTIST_PATRON, PM_Y_CULTIST_WIZARD, PM_Y_CULTIST_FIGHTER, PM_Y_CULTIST,
					PM_ALLIANCE_VANGUARD, PM_ANULO, PM_ANULO, PM_DWARF,
					PM_DWARF, PM_WOODLAND_ELF, PM_WOODLAND_ELF, PM_GREEN_ELF,
					PM_GREEN_ELF, PM_PEASANT, PM_PEASANT, PM_GOBLIN,
					PM_HOBGOBLIN, PM_BUGBEAR, PM_BUGBEAR, PM_HOBBIT,
					PM_GNOME, PM_GNOME, PM_HILL_GIANT, PM_HILL_GIANT,
					PM_MINOTAUR, PM_MINOTAUR_PRIESTESS, PM_ETTIN,
					PM_DEMINYMPH
			};
		if(!rn2(3)){
			qpm = ROLL_FROM(duergar);
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(S_HUMANOID, G_NOHELL));
		}
		if(rn2(2)){
			if(u.uevent.qcompleted)
				qpm = rn2(5) ? ROLL_FROM(duergar) : rn2(3) ? PM_MIND_FLAYER : PM_MASTER_MIND_FLAYER;
			else
				qpm = rn2(5) ? PM_KUO_TOA : rn2(3) ? PM_KUO_TOA_WHIP : rn2(3) ? PM_CHUUL : PM_ELDER_CHUUL;
			if (qpm != NON_PM && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			//Fall through in this (impossible?) case
		}
		qpm = ROLL_FROM(drow_etc);
		if (qpm != NON_PM && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
			return (&mons[qpm]);
		//Should be impossible:
		return (mkclass(S_HUMAN, G_NOHELL));
	} else if(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON) && flags.initgend
		&& ((Is_qlocate(&u.uz) && rn2(2)) || (u.uz.dlevel < qlocate_level.dlevel))
	){
		int qpm;
		if(rn2(5)){
			qpm = PM_WOLF;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(S_DOG, G_NOHELL));
		}
		qpm = PM_CANDLE_TREE;
		if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
			return (&mons[qpm]);
		return (mkclass(S_DOG, G_NOHELL));
	} else if(Role_if(PM_UNDEAD_HUNTER)
		&& ((u.uz.dlevel < qlocate_level.dlevel && quest_status.time_doing_quest < (mvitals[PM_INDEX_WOLF].died ? UH_QUEST_TIME_1 : UH_QUEST_TIME_0))
			|| (u.uz.dlevel == qstart_level.dlevel && quest_status.time_doing_quest < UH_QUEST_TIME_2)
			|| (mvitals[PM_MOON_S_CHOSEN].died)
		)
	){
		if(quest_status.moon_close && *in_rooms(x, y, MORGUE)){
			return mkclass(rn2(100) ? S_ZOMBIE : S_LICH, G_NOHELL|G_HELL);
		}
		return mvitals[PM_MOON_S_CHOSEN].died ? (u.uz.dlevel <= qlocate_level.dlevel ? &mons[PM_FOG_CLOUD] : &mons[PM_MIST_CLOUD]) : &mons[PM_LONG_WORM_TAIL];
	} else {
		int qpm;
		if(Race_if(PM_DROW) && !flags.initgend && Role_if(PM_NOBLEMAN) && on_level(&u.uz, &qstart_level)) return &mons[PM_LONG_WORM_TAIL];
		else if(Race_if(PM_DROW) && flags.initgend && Role_if(PM_NOBLEMAN) && Is_nemesis(&u.uz) && !rn2(4)) 
			return !(mvitals[PM_MIND_FLAYER].mvflags & G_GONE && !In_quest(&u.uz)) ? &mons[PM_MIND_FLAYER] : mkclass(S_UMBER, G_NOHELL);
		else if(In_quest(&u.uz) && Race_if(PM_DWARF) && 
			urole.neminum == PM_BOLG && Is_qlocate(&u.uz) && 
			!((mvitals[PM_SMAUG].mvflags & G_GONE && !In_quest(&u.uz)) || mvitals[PM_SMAUG].died > 0)
		) return &mons[PM_LONG_WORM_TAIL];
		
		if(rn2(5)){
		  if(Role_if(PM_EXILE)){
			switch(rn2(4)){
				case 0:
					qpm = roles[flags.panLgod].guardnum;
				break;
				case 1:
					qpm = roles[flags.panNgod].guardnum;
				break;
				case 2:
					qpm = roles[flags.panCgod].guardnum;
				break;
				case 3:
					qpm = urole.enemy1num;
				break;
			}
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(urole.enemy1sym, G_NOHELL|G_HELL));
		  } else {
			qpm = urole.enemy1num;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(urole.enemy1sym, G_NOHELL|G_HELL));
		  }
		}
		if(Role_if(PM_EXILE)){
			switch(rn2(4)){
				case 0:
					qpm = (roles[flags.panLgod].femalenum == NON_PM || !rn2(2)) ? 
						roles[flags.panLgod].malenum : 
						roles[flags.panLgod].femalenum;
				break;
				case 1:
					qpm = (roles[flags.panNgod].femalenum == NON_PM || !rn2(2)) ? 
						roles[flags.panNgod].malenum : 
						roles[flags.panNgod].femalenum;
				break;
				case 2:
					qpm = (roles[flags.panCgod].femalenum == NON_PM || !rn2(2)) ? 
						roles[flags.panCgod].malenum : 
						roles[flags.panCgod].femalenum;
				break;
				case 3:
					qpm = urole.enemy1num;
				break;
			}
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(urole.enemy1sym, G_NOHELL|G_HELL));
		} else {
			qpm = urole.enemy2num;
			if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GONE && !In_quest(&u.uz)))
				return (&mons[qpm]);
			return (mkclass(urole.enemy2sym, G_NOHELL|G_HELL));
		}
	}
	return &mons[PM_GNOME];
}

struct permonst *
archipelago_montype()
{
	int chance = rn2(100);
	if(chance < 10){
		if(Is_arcboss_level(&u.uz)) return &mons[PM_JELLYFISH];
		return &mons[PM_PARROT];
	}
	if(chance < 30){
		if(Is_arcboss_level(&u.uz)) return &mons[PM_JELLYFISH];
		return &mons[PM_MONKEY];
	}
	if(chance < 40)
		return &mons[PM_WATER_MOCCASIN];
	if(chance < 60)
		return &mons[PM_JELLYFISH];
	if(chance < 90)
		return mkclass(S_LIZARD, G_NOHELL);
	if(chance < 100)
		return mkclass(S_SNAKE, G_NOHELL);
	return (struct permonst *) 0;

}
struct permonst *
dismalswamp_montype()
{
	int chance = rn2(100);
	if(chance < 40)
		return mkclass(S_KOBOLD, G_NOHELL);
	if(chance < 60)
		return mkclass(S_SNAKE, G_NOHELL);
	if(chance < 70)
		return &mons[PM_LEPRECHAUN];
	if(chance < 80)
		return mkclass(S_BAT, G_NOHELL);
	if(chance < 90)
		return mkclass(S_RODENT, G_NOHELL);
	if(chance < 100)
		return mkclass(S_DOG, G_NOHELL);
	return (struct permonst *) 0;

}
struct permonst *
blackforest_montype()
{
	int chance = rn2(100);
	if(chance < 10)
		return &mons[PM_COYOTE];
	if(chance < 30)
		return &mons[PM_BUGBEAR];
	if(chance < 50)
		return &mons[PM_RAZORVINE];
	if(chance < 60)
		return &mons[PM_GIANT_BAT];
	if(chance < 70)
		return &mons[PM_WOLF_CUB];
	if(chance < 80)
		return &mons[PM_WEEPING_WILLOW];
	if(chance < 90)
		return &mons[PM_WEREJACKAL];
	if(chance < 100)
		return &mons[PM_BLACK_UNICORN];
	return (struct permonst *) 0;


}
struct permonst *
icecaves_montype()
{
	int chance = rn2(100);
	if(chance < 10)
		return &mons[PM_CHILL_BUG];
	if(chance < 30)
		return &mons[PM_WHITE_NAGA_HATCHLING];
	if(chance < 45)
		return &mons[PM_BABY_WHITE_DRAGON];
	if(chance < 60)
		return &mons[PM_BLUE_JELLY];
	if(chance < 80)
		return &mons[PM_BROWN_MOLD];
	if(chance < 90)
		return &mons[PM_ICE_VORTEX];
	if(chance < 100)
		return &mons[PM_WHITE_NAGA];
	return (struct permonst *) 0;
}

struct permonst *
chaos_montype()
{
	if(on_level(&chaosf_level,&u.uz)){
		int chance = d(1,100);
		if(chance < 40){
			return mkclass(S_ZOMBIE, G_NOHELL);
		}
		else if(chance < 50){
			return !(mvitals[PM_HOBGOBLIN].mvflags & G_GONE) ? &mons[PM_HOBGOBLIN] : mkclass(S_ORC, G_NOHELL);
		}
		else{
			return !(mvitals[PM_GOBLIN].mvflags & G_GONE) ? &mons[PM_GOBLIN] : mkclass(S_ORC, G_NOHELL);
		}
	}
	else if(on_level(&chaosm_level,&u.uz)){
		//Mordor orcs, orc shaman, orc-captain, ogre, rock troll, stone golem, clay golem
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_STONE_GOLEM].mvflags & G_GONE) ? &mons[PM_STONE_GOLEM] : mkclass(S_GOLEM, G_NOHELL);
		}
		else if(chance < 30){
			return !(mvitals[PM_CLAY_GOLEM].mvflags & G_GONE) ? &mons[PM_CLAY_GOLEM] : mkclass(S_GOLEM, G_NOHELL);
		}
		else if(chance < 40){
			return !(mvitals[PM_ROCK_TROLL].mvflags & G_GONE) ? &mons[PM_ROCK_TROLL] : mkclass(S_TROLL, G_NOHELL);
		}
		else if(chance < 50){
			return !(mvitals[PM_ORC_CAPTAIN].mvflags & G_GONE) ? &mons[PM_ORC_CAPTAIN] : mkclass(S_ORC, G_NOHELL);
		}
		else if(chance < 60){
			return !(mvitals[PM_ORC_SHAMAN].mvflags & G_GONE) ? &mons[PM_ORC_SHAMAN] : mkclass(S_ORC, G_NOHELL);
		}
		else if(chance < 70){
			return !(mvitals[PM_OGRE].mvflags & G_GONE) ? &mons[PM_OGRE] : mkclass(S_OGRE, G_NOHELL);
		}
		else{
			return !(mvitals[PM_MORDOR_ORC].mvflags & G_GONE) ? &mons[PM_MORDOR_ORC] : mkclass(S_ORC, G_NOHELL);
		}
	}
	else if(on_level(&chaost_level,&u.uz)){
//		purple worm, mind flayer, white dragon, black light, beholders
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_WHITE_DRAGON].mvflags & G_GONE) ? &mons[PM_WHITE_DRAGON] : mkclass(S_DRAGON, G_NOHELL);
		}
		else if(chance < 20){
			return !(mvitals[PM_PURPLE_WORM].mvflags & G_GONE) ? &mons[PM_PURPLE_WORM] : mkclass(S_WORM, G_NOHELL);
		}
		else if(chance < 30){
			return !(mvitals[PM_BEHOLDER].mvflags & G_GONE) ? &mons[PM_BEHOLDER] : mkclass(S_UMBER, G_NOHELL);
		}
		else if(chance < 65){
			return !(mvitals[PM_MIND_FLAYER].mvflags & G_GONE) ? &mons[PM_MIND_FLAYER] : mkclass(S_UMBER, G_NOHELL);
		}
		else{
			return !(mvitals[PM_BLACK_LIGHT].mvflags & G_GONE) ? &mons[PM_BLACK_LIGHT] : mkclass(S_LIGHT, G_NOHELL);
		}
	}
	else if(on_level(&chaoss_level,&u.uz)){
//		purple worm, mind flayer, GREEN dragon, black light, beholders
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_GREEN_DRAGON].mvflags & G_GONE) ? &mons[PM_GREEN_DRAGON] : mkclass(S_DRAGON, G_NOHELL);
		}
		else if(chance < 20){
			return !(mvitals[PM_PURPLE_WORM].mvflags & G_GONE) ? &mons[PM_PURPLE_WORM] : mkclass(S_WORM, G_NOHELL);
		}
		else if(chance < 30){
			return !(mvitals[PM_BEHOLDER].mvflags & G_GONE) ? &mons[PM_BEHOLDER] : mkclass(S_UMBER, G_NOHELL);
		}
		else if(chance < 65){
			return !(mvitals[PM_MIND_FLAYER].mvflags & G_GONE) ? &mons[PM_MIND_FLAYER] : mkclass(S_UMBER, G_NOHELL);
		}
		else{
			return !(mvitals[PM_BLACK_LIGHT].mvflags & G_GONE) ? &mons[PM_BLACK_LIGHT] : mkclass(S_LIGHT, G_NOHELL);
		}
	}
	else if(on_level(&chaosfrh_level,&u.uz)){
		//vampire lord, vampire, earth elemental, zombie
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_VAMPIRE_LORD].mvflags & G_GONE) ? &mons[PM_VAMPIRE_LORD] : mkclass(S_VAMPIRE, G_NOHELL);
		}
		else if(chance < 20){
			return !(mvitals[PM_EARTH_ELEMENTAL].mvflags & G_GONE) ? &mons[PM_EARTH_ELEMENTAL] : mkclass(S_ELEMENTAL, G_NOHELL);
		}
		else if(chance < 30){
			return mkclass(S_ZOMBIE, G_NOHELL);
		}
		else if(chance < 65){
			return !(mvitals[PM_VAMPIRE].mvflags & G_GONE) ? &mons[PM_VAMPIRE] : mkclass(S_VAMPIRE, G_NOHELL);
		}
		else{
			return mkclass(S_ZOMBIE, G_NOHELL);
		}
	}
	else if(on_level(&chaosffh_level,&u.uz)){
		//fire giant, salamander, imps, fire elemtal
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_FIRE_GIANT].mvflags & G_GONE) ? &mons[PM_FIRE_GIANT] : mkclass(S_GIANT, G_NOHELL);
		}
		else if(chance < 20){
			return !(mvitals[PM_FIRE_ELEMENTAL].mvflags & G_GONE) ? &mons[PM_FIRE_ELEMENTAL] : mkclass(S_ELEMENTAL, G_NOHELL);
		}
		else if(chance < 30){
			return !(mvitals[PM_SALAMANDER].mvflags & G_GONE) ? &mons[PM_SALAMANDER] : mkclass(S_LIZARD, G_NOHELL);
		}
		else{
			return !(mvitals[PM_IMP].mvflags & G_GONE) ? &mons[PM_IMP] : mkclass(S_IMP, G_NOHELL);
		}
	}
	else if(on_level(&chaossth_level,&u.uz)){
		//winter wolf, water troll, python, water moccasin, water elemental
		int chance = d(1,100);
		if(chance < 10){
			return !(mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE) ? &mons[PM_WATER_ELEMENTAL] : mkclass(S_ELEMENTAL, G_NOHELL);
		}
		else if(chance < 30){
			return !(mvitals[PM_WINTER_WOLF].mvflags & G_GONE) ? &mons[PM_WINTER_WOLF] : mkclass(S_DOG, G_NOHELL);
		}
		else if(chance < 50){
			return !(mvitals[PM_PYTHON].mvflags & G_GONE) ? &mons[PM_PYTHON] : mkclass(S_SNAKE, G_NOHELL);
		}
		else if(chance < 70){
			return !(mvitals[PM_WATER_TROLL].mvflags & G_GONE) ? &mons[PM_WATER_TROLL] : mkclass(S_TROLL, G_NOHELL);
		}
		else{
			return !(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE) ? &mons[PM_WATER_MOCCASIN] : mkclass(S_SNAKE, G_NOHELL);
		}
	}
	else if(on_level(&chaosvth_level,&u.uz)){
		//iron golem, air elemental
		int chance = d(1,8);
		if(chance == 1){
			return !(mvitals[PM_AIR_ELEMENTAL].mvflags & G_GONE) ? &mons[PM_AIR_ELEMENTAL] : mkclass(S_ELEMENTAL, G_NOHELL);
		}
		else if(chance == 2){
			return !(mvitals[PM_IRON_GOLEM].mvflags & G_GONE) ? &mons[PM_IRON_GOLEM] : mkclass(S_GOLEM, G_NOHELL);
		}
		else if(chance == 3 && !rn2(10)){
			return !(mvitals[PM_DOOM_KNIGHT].mvflags & G_GONE) ? &mons[PM_DOOM_KNIGHT] : (struct permonst *)0;
		}
		else{
			return (struct permonst *)0;
		}
	}
	else if(on_level(&chaose_level,&u.uz)){
		return (struct permonst *)0;
	}
	else return (struct permonst *)0;
}

struct permonst *
chaos2_montype()
{
	int diff = (u.ulevel+level_difficulty())/2;
	if(on_level(&elshava_level,&u.uz)){
		if(rn2(3))
			return !(mvitals[PM_FOG_CLOUD].mvflags & G_GONE) ? &mons[PM_FOG_CLOUD] : mkclass(S_VORTEX, G_NOHELL);
		else if(!rn2(2))
			return !(mvitals[PM_PIRANHA].mvflags & G_GONE) ? &mons[PM_PIRANHA] : mkclass(S_EEL, G_NOHELL);
		else if(rn2(3))
			return mkclass(S_EEL, G_NOHELL);
		else if(rn2(2))
			return !(mvitals[PM_COURE_ELADRIN].mvflags & G_GONE) ? &mons[PM_COURE_ELADRIN] : mkclass(S_CHA_ANGEL, G_NOHELL);
		else
			return !(mvitals[PM_NOVIERE_ELADRIN].mvflags & G_GONE) ? &mons[PM_NOVIERE_ELADRIN] : mkclass(S_CHA_ANGEL, G_NOHELL);
	} else if(In_mithardir_desert(&u.uz)){
		if(G_C_INST(mons[PM_DESERT_SEER].geno) < Insight && !rn2(10)){
			if(!toostrong(PM_DESERT_SEER, diff+5))
				return &mons[PM_DESERT_SEER];
		}
		if(rn2(3))
			return !(mvitals[PM_DUST_VORTEX].mvflags & G_GONE) ? &mons[PM_DUST_VORTEX] : mkclass(S_ZOMBIE, G_NOHELL);
		else if(rn2(2))
			return !(mvitals[PM_ALABASTER_ELF].mvflags & G_GONE) ? &mons[PM_ALABASTER_ELF] : mkclass(S_ZOMBIE, G_NOHELL);
		else if(rn2(2))
			return !(mvitals[PM_WEREWOLF].mvflags & G_GONE) ? &mons[PM_WEREWOLF] : mkclass(S_ZOMBIE, G_NOHELL);
		else if(rn2(3))
			return !(mvitals[PM_ALABASTER_ELF_ELDER].mvflags & G_GONE) ? &mons[PM_ALABASTER_ELF_ELDER] : mkclass(S_ZOMBIE, G_NOHELL);
		else if(!rn2(3))
			return !(mvitals[PM_SENTINEL_OF_MITHARDIR].mvflags & G_GONE) ? &mons[PM_SENTINEL_OF_MITHARDIR] : mkclass(S_GOLEM, G_NOHELL);
		else if(rn2(2))
			return !(mvitals[PM_WRAITHWORM].mvflags & G_GONE) ? &mons[PM_WRAITHWORM] : mkclass(S_SNAKE, G_NOHELL);
		else 
			return !(mvitals[PM_LIVING_MIRAGE].mvflags & G_GONE) ? &mons[PM_LIVING_MIRAGE] : mkclass(S_PUDDING, G_NOHELL);
	} else if(In_mithardir_catacombs(&u.uz)){
		if(rn2(2))
			return mkclass(S_BLOB, G_NOHELL);
		else if(rn2(3))
			return mkclass(S_PUDDING, G_NOHELL);
		else if(!rn2(3))
			return mkclass(S_WRAITH, G_NOHELL);
		else if(rn2(2))
			return !(mvitals[PM_WRAITHWORM].mvflags & G_GONE) ? &mons[PM_SENTINEL_OF_MITHARDIR] : mkclass(S_SNAKE, G_NOHELL);
		else if(rn2(3) || !In_mithardir_terminus(&u.uz))
			return !(mvitals[PM_SENTINEL_OF_MITHARDIR].mvflags & G_GONE) ? &mons[PM_SENTINEL_OF_MITHARDIR] : mkclass(S_GOLEM, G_NOHELL);
		else 
			return !(mvitals[PM_ASPECT_OF_THE_SILENCE].mvflags & G_GONE) ? &mons[PM_ASPECT_OF_THE_SILENCE] : mkclass(S_ZOMBIE, G_NOHELL);
	}
	return !(mvitals[PM_FOG_CLOUD].mvflags & G_GONE) ? &mons[PM_FOG_CLOUD] : mkclass(S_VORTEX, G_NOHELL);
}
#define	FOREST_INSIGHT	{ \
		if(!rn2(66)){\
			if (moves > 9000 && !(mvitals[PM_INCARNATOR_MAGGOT].mvflags & G_GONE) && !toostrong(PM_INCARNATOR_MAGGOT, diff+5)) \
				return &mons[PM_INCARNATOR_MAGGOT]; \
		}\
}
#define	DEPTHS_INSIGHT	{ \
		if(!rn2(20)){\
			if (!(mvitals[PM_INCARNATOR_MAGGOT].mvflags & G_GONE) && !toostrong(PM_INCARNATOR_MAGGOT, diff+5)) \
				return &mons[PM_INCARNATOR_MAGGOT]; \
		}\
		if(check_insight()){ \
			if(G_C_INST(mons[PM_NAMELESS_GNAWER].geno) < u.uinsight && !toostrong(PM_NAMELESS_GNAWER, diff+5)) \
				return &mons[PM_NAMELESS_GNAWER]; \
		} \
}
struct permonst *
chaos3_montype()
{
	int diff = (u.ulevel+level_difficulty())/2;
	if(In_mordor_forest(&u.uz)){
		FOREST_INSIGHT
		switch(rn2(20)){
			case 0:
			case 1:
			case 2:
			case 3: return &mons[PM_WOODLAND_ELF];
			case 4:
			case 5:
			case 6: return &mons[PM_GREEN_ELF];
			case 7:
			case 8:
			case 9: return &mons[PM_GREY_ELF];
			case 10: return &mons[rn2(2) ? PM_ELF_LORD : PM_ELF_LADY];
			case 11: return &mons[PM_COURE_ELADRIN];
			case 12: return &mons[PM_NOVIERE_ELADRIN];
			case 13: return &mons[PM_QUICKLING];
			case 14: return &mons[PM_DRYAD];
			case 15: return &mons[PM_LEOCROTTA];
			case 16: return &mons[PM_HORSE];
			case 17: return &mons[rn2(2) ? PM_GRAY_UNICORN : PM_WHITE_UNICORN];
			case 18: return &mons[PM_BLACK_UNICORN];
			case 19: return &mons[PM_DREAM_QUASIELEMENTAL];
		}
	} else if(Is_ford_level(&u.uz)){
		FOREST_INSIGHT
		return ford_montype(0);
	} else if(In_mordor_fields(&u.uz)){
		FOREST_INSIGHT
		switch(rn2(8)){
			case 0: return &mons[PM_ROTHE];
			case 1: return &mons[PM_MUMAK_CALF];
			case 2: return &mons[PM_MUMAK];
			case 3: return &mons[PM_PONY];
			case 4: return &mons[PM_HORSE];
			case 5: return &mons[PM_WARHORSE];
			case 6: return &mons[PM_FOG_CLOUD];
			case 7: return &mons[PM_BARROW_WIGHT];
		}
	} else if(In_mordor_buildings(&u.uz)){
		switch(rn2(20)){
			case 1:
			case 2:
			case 3:
			case 4:
			case 5: return &mons[PM_MORDOR_ORC];
			case 6:
			case 7:
			case 8: return &mons[PM_ORC_SHAMAN];
			case 9:
			case 10: return &mons[PM_ORC_CAPTAIN];
			case 11: return &mons[PM_QUASIT];
			case 12:
			case 13: return &mons[PM_OGRE];
			case 14: return &mons[PM_OGRE_LORD];
			case 15: return &mons[PM_MUMAK_CALF];
			case 16: return &mons[PM_MUMAK];
			case 17:
			case 18: return &mons[PM_MORDOR_ORC_ELITE];
			case 19: return &mons[rn2(2) ? PM_SHADE : PM_NAZGUL];
		}
	} else if(Is_spider_cave(&u.uz)){
		switch(rn2(9)){
			case 0: 
			case 1: 
			case 2: 
			case 3: return &mons[PM_CAVE_SPIDER];
			case 4: 
			case 5: 
			case 6: return &mons[PM_GIANT_SPIDER];
			case 7: return &mons[PM_MIRKWOOD_SPIDER];
			case 8: return mkclass(S_SPIDER, G_NOHELL|G_HELL);
		}
	} else if(
		on_level(&u.uz, &mordor_depths_1_level)
		|| on_level(&u.uz, &mordor_depths_2_level)
	){
		DEPTHS_INSIGHT
		switch(rn2(on_level(&u.uz, &mordor_depths_2_level) ? 30 : 20)){
			case 1:
			case 2:
			case 3:
			case 4:
			case 5: return &mons[PM_MORDOR_ORC];
			case 6:
			case 7:
			case 8: return &mons[PM_ORC_SHAMAN];
			case 9:
			case 10: return &mons[PM_ORC_CAPTAIN];
			case 11: return &mons[PM_QUASIT];
			case 12:
			case 13: return &mons[PM_OGRE];
			case 14: return &mons[PM_OGRE_LORD];
			case 15: return &mons[PM_MUMAK_CALF];
			case 16: return &mons[PM_MUMAK];
			case 17:
			case 18:
			case 19: return &mons[PM_MORDOR_ORC_ELITE];
			case 20:
			case 21:
			case 22:
			case 23: return &mons[PM_FLAMING_SPHERE];
			case 24:
			case 25: return &mons[PM_FIRE_VORTEX];
			case 26:
			case 27: return &mons[PM_FIRE_ELEMENTAL];
			case 28:
			case 29: return &mons[PM_MORDOR_SHAMAN];
		}
	} else if(on_level(&u.uz, &mordor_depths_3_level)){
		DEPTHS_INSIGHT
		switch(rn2(20)){
			case 0:
			case 1:
			case 2: return &mons[PM_MORDOR_ORC];
			case 3: return &mons[PM_ORC_SHAMAN];
			case 4:
			case 5: return &mons[PM_ORC_CAPTAIN];
			case 6: return &mons[PM_QUASIT];
			case 7:
			case 8:
			case 9:
			case 10: return &mons[PM_MORDOR_ORC_ELITE];
			case 11:
			case 12:
			case 13: return &mons[PM_FLAMING_SPHERE];
			case 14:
			case 15: return &mons[PM_FIRE_VORTEX];
			case 16:
			case 17: return &mons[PM_FIRE_ELEMENTAL];
			case 18:
			case 19: return &mons[PM_MORDOR_SHAMAN];
		}
	} else if(on_level(&u.uz, &borehole_1_level)){
		DEPTHS_INSIGHT
		switch(rn2(4)){
			case 0:
			case 1:
			case 2: return &mons[PM_MORDOR_ORC];
			case 3: return &mons[PM_MORDOR_ORC_ELITE];
		}
	} else if(on_level(&u.uz, &borehole_2_level)){
		DEPTHS_INSIGHT
		switch(rn2(6)){
			case 0:
			case 1:
			case 2: return &mons[PM_MORDOR_ORC];
			case 3:
			case 4: return &mons[PM_MORDOR_ORC_ELITE];
			case 5: return &mons[PM_ANGBAND_ORC];
		}
	} else if(on_level(&u.uz, &borehole_3_level)){
		DEPTHS_INSIGHT
		switch(rn2(10)){
			case 0:
			case 1:
			case 2:
			case 3: return &mons[PM_MORDOR_ORC];
			case 4:
			case 5:
			case 6: return &mons[PM_MORDOR_ORC_ELITE];
			case 7:
			case 8: return &mons[PM_ANGBAND_ORC];
			case 9: return &mons[PM_INVIDIAK];
		}
	} else if(on_level(&u.uz, &borehole_4_level)){
		DEPTHS_INSIGHT
	}
	return (struct permonst *)0;
}


struct permonst *
ford_montype(leftright)
int leftright;
{
	if(!leftright)
		leftright = rn2(2) ? 1 : -1;
	
	if(leftright < 0){
		switch(rn2(20)){
			case 0:
			case 1:
			case 2:
			case 3: return &mons[PM_WOODLAND_ELF];
			case 4:
			case 5:
			case 6: return &mons[PM_GREEN_ELF];
			case 7:
			case 8:
			case 9: return &mons[PM_GREY_ELF];
			case 10: return &mons[rn2(2) ? PM_ELF_LORD : PM_ELF_LADY];
			case 11: return &mons[PM_COURE_ELADRIN];
			case 12: return &mons[PM_NOVIERE_ELADRIN];
			case 13: return &mons[PM_QUICKLING];
			case 14: return &mons[PM_DRYAD];
			case 15: return &mons[PM_LEOCROTTA];
			case 16: return &mons[PM_HORSE];
			case 17: return &mons[rn2(2) ? PM_GRAY_UNICORN : PM_WHITE_UNICORN];
			case 18: return &mons[PM_BLACK_UNICORN];
			case 19: return &mons[PM_DREAM_QUASIELEMENTAL];
		}
	} else {
		switch(rn2(15)){
			case 0:
			case 1:
			case 2: return &mons[PM_HILL_ORC];
			case 3:
			case 4:
			case 5: return &mons[PM_MORDOR_ORC];
			case 6:
			case 7:
			case 8: return &mons[PM_URUK_HAI];
			case 9:
			case 10:
			case 11: return &mons[PM_ORC_SHAMAN];
			case 12: return &mons[PM_ORC_CAPTAIN];
			case 13: return &mons[PM_URUK_CAPTAIN];
			case 14: return &mons[PM_QUASIT];
		}
	}
	return &mons[PM_HILL_ORC];
}

struct permonst *
neutral_montype()
{
	if(u.uz.dnum == neutral_dnum && u.uz.dlevel < sum_of_all_level.dlevel){
		int chance = rn2(100);
		int diff = (u.ulevel+level_difficulty())/2;
		if(Is_gatetown(&u.uz))
			return (struct permonst *)0;
		switch(rn2(5)){
			case 0:
				if(rn2(2)) return &mons[PM_HORSE];
				else return mkclass(S_UNICORN, G_NOHELL);
			break;
			case 1:
				if(chance < 10 && !toostrong(PM_ARGENACH_RILMANI, diff))
					return &mons[PM_ARGENACH_RILMANI];
				if(chance < 30 && !toostrong(PM_CUPRILACH_RILMANI, diff))
					return &mons[PM_CUPRILACH_RILMANI];
				if(chance < 60 && !toostrong(PM_FERRUMACH_RILMANI, diff))
					return &mons[PM_FERRUMACH_RILMANI];
				if(chance < 70)
					return mkclass(S_GOLEM, G_NOHELL);
				return &mons[PM_PLUMACH_RILMANI];
			break;
			case 2:
				if(chance < 5 && !toostrong(PM_ARA_KAMEREL, diff))
					return &mons[PM_ARA_KAMEREL];
				if(chance < 15 && !toostrong(PM_SHARAB_KAMEREL, diff))
					return &mons[PM_SHARAB_KAMEREL];
				return &mons[PM_AMM_KAMEREL];
			break;
			case 3:
				if(rn2(4)) return mkclass(S_QUADRUPED, G_NOHELL);
				return &mons[PM_SHATTERED_ZIGGURAT_CULTIST];
			break;
			case 4:
				if(on_level(&spire_level, &u.uz) && !rn2(4))
					return mkclass(S_GOLEM, G_NOHELL);
				return &mons[PM_PLAINS_CENTAUR];
			break;
		}
	}
	if(on_level(&sum_of_all_level,&u.uz)){
		int chance = rn2(100);
		int diff = (u.ulevel+level_difficulty())/2;
		if(chance < 5 && !toostrong(PM_AURUMACH_RILMANI, diff))
			return &mons[PM_AURUMACH_RILMANI];
		if(chance < 15 && !toostrong(PM_ARGENACH_RILMANI, diff))
			return &mons[PM_ARGENACH_RILMANI];
		if(chance < 35 && !toostrong(PM_CUPRILACH_RILMANI, diff))
			return &mons[PM_CUPRILACH_RILMANI];
		if(chance < 65 && !toostrong(PM_FERRUMACH_RILMANI, diff))
			return &mons[PM_FERRUMACH_RILMANI];
		if(chance < 75)
			return mkclass(S_GOLEM, G_NOHELL);
		return &mons[PM_PLUMACH_RILMANI];
	}
	if(on_level(&rlyeh_level,&u.uz)){
		int chance = d(1,100);
		int num = 0;
		if(rn2(20)) return (struct permonst *)0; /*GREATLY reduce monster generation*/
		if(chance < 2){
			for (num = d(2,3); num >= 0; num--) {
				if (!(mvitals[PM_HUNTING_HORROR].mvflags & G_GONE))
					makemon(&mons[PM_HUNTING_HORROR], xupstair, yupstair, MM_ADJACENTOK);
				else
					makemon(mkclass(S_UMBER, G_NOHELL|G_HELL), xupstair, yupstair, MM_ADJACENTOK);
			}
			return (struct permonst *)0;
		}
		else if(chance < 6){
			for (num = d(2,4); num >= 0; num--) {
				if (!(mvitals[PM_BYAKHEE].mvflags & G_GONE))
					makemon(&mons[PM_BYAKHEE], xupstair, yupstair, MM_ADJACENTOK);
				else
					makemon(mkclass(S_UMBER, G_NOHELL|G_HELL), xupstair, yupstair, MM_ADJACENTOK);
			}
			return (struct permonst *)0;
		}
		else if(chance < 8){
			if (!(mvitals[PM_SHOGGOTH].mvflags & G_GONE))
				makemon(&mons[PM_SHOGGOTH], xupstair, yupstair, MM_ADJACENTOK);
			else
				makemon(mkclass(S_BLOB, G_NOHELL|G_HELL), xupstair, yupstair, MM_ADJACENTOK);
			return (struct permonst *)0;
		}
		else if(chance < 10){ /*Deep ones can't be genocided*/
			makemon(&mons[PM_DEEPEST_ONE], xupstair, yupstair, MM_ADJACENTOK);
			for (num = rnd(4); num >= 0; num--) makemon(&mons[PM_DEEPER_ONE], xupstair, yupstair, MM_ADJACENTOK);
			for (num = rn1(2,1); num >= 0; num--) makemon(&mons[PM_DEEP_ONE], xupstair, yupstair, MM_ADJACENTOK);
			return (struct permonst *)0;
		}
		else if(chance < 30){
			for (num = rnd(3); num >= 0; num--) {
				if (!(mvitals[PM_MASTER_MIND_FLAYER].mvflags & G_GONE))
					makemon(&mons[PM_MASTER_MIND_FLAYER], xupstair, yupstair, MM_ADJACENTOK);
				else
					makemon(mkclass(S_UMBER, G_NOHELL|G_HELL), xupstair, yupstair, MM_ADJACENTOK);
			}
			return (struct permonst *)0;
		}
		else if(chance < 50){
			for (num = rn1(2,2); num >= 0; num--) {
				if (!(mvitals[PM_MIND_FLAYER].mvflags & G_GONE))
					makemon(&mons[PM_MIND_FLAYER], xupstair, yupstair, MM_ADJACENTOK);
				else
					makemon(mkclass(S_UMBER, G_NOHELL|G_HELL), xupstair, yupstair, MM_ADJACENTOK);
			}
			return (struct permonst *)0;
		}
		else if(chance < 70){/*Deep ones can't be genocided*/
			for (num = rnd(6); num >= 0; num--) makemon(&mons[PM_DEEPER_ONE], xupstair, yupstair, MM_ADJACENTOK);
			return (struct permonst *)0;
		}
		else{/*Deep ones can't be genocided*/
			for (num = rn1(4,3); num >= 0; num--) makemon(&mons[PM_DEEP_ONE], xupstair, yupstair, MM_ADJACENTOK);
			return (struct permonst *)0;
		}
	}
	return (struct permonst *)0;
}

struct permonst *
law_montype()
{
	if(on_level(&path1_level,&u.uz)){
		int chance = d(1,100);
		if(chance < 10){
			return mkclass(S_DEMON, G_NOHELL|G_HELL);
		}
		else if(chance < 60){
			return mkclass(S_IMP, 0);
		}
		else if(chance < 70 && mvitals[PM_OONA].died == 0){
			if(u.oonaenergy == AD_FIRE){
				return rn2(2) ? &mons[PM_FLAMING_SPHERE] : &mons[PM_FIRE_VORTEX];
			}
			if(u.oonaenergy == AD_COLD){
				return rn2(2) ? &mons[PM_FREEZING_SPHERE] : &mons[PM_ICE_VORTEX];
			}
			if(u.oonaenergy == AD_ELEC){
				return rn2(2) ? &mons[PM_SHOCKING_SPHERE] : &mons[PM_ENERGY_VORTEX];
			}
		}
		else{ /*Can't be genocided*/
			chance = d(1,31);
			if(chance == 1){
				return &mons[PM_QUINON];
			}
			else if(chance <= 3){
				return &mons[PM_QUATON];
			}
			else if(chance <= 7){
				return &mons[PM_TRITON];
			}
			else if(chance <= 15){
				return &mons[PM_DUTON];
			}
			else{
				return &mons[PM_MONOTON];
			}
		}
	}
	else if(on_level(&path2_level,&u.uz)){
		int chance = d(1,100);
		if(chance < 2){
			return mkclass(S_DEMON, G_NOHELL|G_HELL);
		}
		else if(chance < 40){
			return mkclass(S_IMP, 0);
		}
		else if(chance < 60 && mvitals[PM_OONA].died == 0){
			if(u.oonaenergy == AD_FIRE){
				return rn2(2) ? &mons[PM_FLAMING_SPHERE] : &mons[PM_FIRE_VORTEX];
			}
			if(u.oonaenergy == AD_COLD){
				return rn2(2) ? &mons[PM_FREEZING_SPHERE] : &mons[PM_ICE_VORTEX];
			}
			if(u.oonaenergy == AD_ELEC){
				return rn2(2) ? &mons[PM_SHOCKING_SPHERE] : &mons[PM_ENERGY_VORTEX];
			}
		}
		else{
			chance = d(1,31);
			if(chance == 1){
				return &mons[PM_QUINON];
			}
			else if(chance <= 3){
				return &mons[PM_QUATON];
			}
			else if(chance <= 7){
				return &mons[PM_TRITON];
			}
			else if(chance <= 15){
				return &mons[PM_DUTON];
			}
			else{
				return &mons[PM_MONOTON];
			}
		}
	}
	else if(on_level(&path3_level,&u.uz)){
		int chance = d(1,100);
		if(chance < 20){
			return mkclass(S_IMP, 0);
		}
		else if(chance < 40 && mvitals[PM_OONA].died == 0){
			if(u.oonaenergy == AD_FIRE){
				return rn2(2) ? &mons[PM_FLAMING_SPHERE] : &mons[PM_FIRE_VORTEX];
			}
			if(u.oonaenergy == AD_COLD){
				return rn2(2) ? &mons[PM_FREEZING_SPHERE] : &mons[PM_ICE_VORTEX];
			}
			if(u.oonaenergy == AD_ELEC){
				return rn2(2) ? &mons[PM_SHOCKING_SPHERE] : &mons[PM_ENERGY_VORTEX];
			}
		}
		else{
			chance = d(1,31);
			if(chance == 1){
				return &mons[PM_QUINON];
			}
			else if(chance <= 3){
				return &mons[PM_QUATON];
			}
			else if(chance <= 7){
				return &mons[PM_TRITON];
			}
			else if(chance <= 15){
				return &mons[PM_DUTON];
			}
			else{
				return &mons[PM_MONOTON];
			}
		}
	}
	else if(Is_illregrd(&u.uz)){
		int chance = d(1,31);
		if(chance == 1){
			return &mons[PM_QUINON];
		}
		else if(chance <= 3){
			return &mons[PM_QUATON];
		}
		else if(chance <= 7){
			return &mons[PM_TRITON];
		}
		else if(chance <= 15){
			return &mons[PM_DUTON];
		}
		else{
			return &mons[PM_MONOTON];
		}
	}
	else if(on_level(&arcadia1_level,&u.uz)){
		int chance = d(1,100);
		int diff = (u.ulevel+level_difficulty())/2;
		if(check_insight()){
			if(!night() && !toostrong(PM_TETTIGON_LEGATUS, diff+5))
				return &mons[PM_TETTIGON_LEGATUS];
			else if(night() && !toostrong(PM_LUMINESCENT_SWARM, diff+5))
				return &mons[PM_LUMINESCENT_SWARM];
		}
		//No insight event or too strong
		if(chance <= 20){
			return !(mvitals[PM_KILLER_BEE].mvflags & G_GONE) ? &mons[PM_KILLER_BEE] : mkclass(S_ANT, 0);
		}
		else if(chance <= 40){
			return !(mvitals[PM_SOLDIER_ANT].mvflags & G_GONE) ? &mons[PM_SOLDIER_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 70){
			return !(mvitals[PM_GIANT_ANT].mvflags & G_GONE) ? &mons[PM_GIANT_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 90){
			chance = d(1,6);
			if(chance <= 5) return !(mvitals[PM_DWARF].mvflags & G_GONE) ? &mons[PM_DWARF] : mkclass(S_HUMANOID, 0);
			else if(chance <= 9) return !(mvitals[PM_DWARF_LORD].mvflags & G_GONE) ? &mons[PM_DWARF_LORD] : mkclass(S_HUMANOID, 0);
		}
		else if(chance <= 99){
			chance = d(1,31);
			if(chance == 1){
				return &mons[PM_QUINON];
			}
			else if(chance <= 3){
				return &mons[PM_QUATON];
			}
			else if(chance <= 7){
				return &mons[PM_TRITON];
			}
			else if(chance <= 15){
				return &mons[PM_DUTON];
			}
			else{
				return &mons[PM_MONOTON];
			}
		} else {
			return &mons[PM_ARCADIAN_AVENGER];
		}
	}
	else if(on_level(&arcadia2_level,&u.uz)){
		int chance = d(1,100);
		if(chance <= 20){
			return !(mvitals[PM_KILLER_BEE].mvflags & G_GONE) ? &mons[PM_KILLER_BEE] : mkclass(S_ANT, 0);
		}
		else if(chance <= 40){
			return !(mvitals[PM_SOLDIER_ANT].mvflags & G_GONE) ? &mons[PM_SOLDIER_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 70){
			return !(mvitals[PM_GIANT_ANT].mvflags & G_GONE) ? &mons[PM_GIANT_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 90){
			chance = d(1,6);
			if(chance <= 5) return !(mvitals[PM_DWARF].mvflags & G_GONE) ? &mons[PM_DWARF] : mkclass(S_HUMANOID, 0);
			else if(chance <= 9) return !(mvitals[PM_DWARF_LORD].mvflags & G_GONE) ? &mons[PM_DWARF_LORD] : mkclass(S_HUMANOID, 0);
		}
		else if(chance <= 98){
			chance = d(1,6);
			if(chance <= 5) return !(mvitals[PM_SOLDIER].mvflags & G_GONE) ? &mons[PM_SOLDIER] : mkclass(S_HUMAN, 0);
			else return !(mvitals[PM_SERGEANT].mvflags & G_GONE) ? &mons[PM_SERGEANT] : mkclass(S_HUMAN, 0);
		} else {
			return &mons[PM_ARCADIAN_AVENGER];
		}
	}
	else if(on_level(&arcadia3_level,&u.uz)){
		int chance = d(1,100);
		if(chance <= 20){
			return !(mvitals[PM_KILLER_BEE].mvflags & G_GONE) ? &mons[PM_KILLER_BEE] : mkclass(S_ANT, 0);
		}
		else if(chance <= 40){
			return !(mvitals[PM_SOLDIER_ANT].mvflags & G_GONE) ? &mons[PM_SOLDIER_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 70){
			return !(mvitals[PM_GIANT_ANT].mvflags & G_GONE) ? &mons[PM_GIANT_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 90){
			chance = d(1,6);
			if(chance <= 5) return !(mvitals[PM_SOLDIER].mvflags & G_GONE) ? &mons[PM_SOLDIER] : mkclass(S_HUMAN, 0);
			else return !(mvitals[PM_SERGEANT].mvflags & G_GONE) ? &mons[PM_SERGEANT] : mkclass(S_HUMAN, 0);
		} else {
			return &mons[PM_ARCADIAN_AVENGER];
		}
	}
	else if(on_level(&arcward_level,&u.uz) || on_level(&arcfort_level,&u.uz)){
		int chance = d(1,100);
		if(chance <= 20){
			return !(mvitals[PM_KILLER_BEE].mvflags & G_GONE) ? &mons[PM_KILLER_BEE] : mkclass(S_ANT, 0);
		}
		else if(chance <= 50){
			return !(mvitals[PM_SOLDIER_ANT].mvflags & G_GONE) ? &mons[PM_SOLDIER_ANT] : mkclass(S_ANT, 0);
		}
		else if(chance <= 80){
			chance = d(1,6);
			if(chance <= 5) return !(mvitals[PM_SOLDIER].mvflags & G_GONE) ? &mons[PM_SOLDIER] : mkclass(S_HUMAN, 0);
			else return !(mvitals[PM_SERGEANT].mvflags & G_GONE) ? &mons[PM_SERGEANT] : mkclass(S_HUMAN, 0);
		} else {
			return &mons[PM_ARCADIAN_AVENGER];
		}
	} else if(mvitals[PM_OONA].died == 0){
		if(u.oonaenergy == AD_FIRE){
			return rn2(2) ? &mons[PM_FLAMING_SPHERE] : &mons[PM_FIRE_VORTEX];
		}
		if(u.oonaenergy == AD_COLD){
			return rn2(2) ? &mons[PM_FREEZING_SPHERE] : &mons[PM_ICE_VORTEX];
		}
		if(u.oonaenergy == AD_ELEC){
			return rn2(2) ? &mons[PM_SHOCKING_SPHERE] : &mons[PM_ENERGY_VORTEX];
		}
	}
	return (struct permonst *)0;
}

struct permonst *
moon_montype()
{
	int chance = rnd(100);
	if(chance > 92)
		return &mons[PM_MOON_ENTITY_MANIPALP];
	if(chance > 88)
		return &mons[PM_MOON_ENTITY_TONGUE];
	if(chance > 84)
		return &mons[PM_MOON_ENTITY_EYE_CLUSTER];
	if(chance > 82)
		return &mons[PM_FOETID_ANGEL];
	if(chance > 72)
		return &mons[PM_AETHER_WOLF];
	if(chance > 62)
		return &mons[PM_MIST_WOLF];
	if(chance > 50)
		return &mons[PM_MOON_FLEA];
	return (struct permonst *)0;
}
/*questpgr.c*/
