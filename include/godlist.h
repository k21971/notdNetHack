/*	SCCS Id: @(#)godlist.h	3.4	1995/10/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef GODLIST_H
#define GODLIST_H

#define MINIONS(...) _MINIONS(__VA_ARGS__)
#define _MINIONS(...) {FIRST_TWENTY(dummy, ##__VA_ARGS__, \
	NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,\
	NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM,NON_PM)}
#define FIRST_TWENTY(dummy, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, \
	a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, ...) \
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, \
	a11, a12, a13, a14, a15, a16, a17, a18, a19, a20

#ifdef MAKEDEFS_C
/* in makedefs.c, all we care about is the list of names */
#define GOD_V2(name, alignment, holiness, minions) name

static const char *god_names[] = {

#else
/* full declarations */
#include "hack.h"
#include "artifact.h"
#include "gods.h"

#define GOD_V2(name, alignment, holiness, minions) \
	{name, alignment, holiness, minions}

/* oft-repeated minion lists */
#define Langels PM_JUSTICE_ARCHON,PM_SHIELD_ARCHON,PM_SWORD_ARCHON,PM_TRUMPET_ARCHON,PM_ANGEL,PM_WARDEN_ARCHON,PM_THRONE_ARCHON,PM_LIGHT_ARCHON
#define Ldevils PM_LEMURE,PM_IMP,PM_HORNED_DEVIL,PM_BARBED_DEVIL,PM_LEGION_DEVIL_GRUNT,PM_LEGION_DEVIL_SOLDIER, PM_BONE_DEVIL,PM_ICE_DEVIL,PM_FALLEN_ANGEL
#define Nangels PM_MOVANIC_DEVA,PM_MONADIC_DEVA,PM_ASTRAL_DEVA,PM_ANGEL,PM_GRAHA_DEVA,PM_SURYA_DEVA,PM_MAHADEVA
#define NElemen PM_AIR_ELEMENTAL,PM_WATER_ELEMENTAL,PM_FIRE_ELEMENTAL,PM_EARTH_ELEMENTAL,PM_MORTAI
#define Cangels PM_NOVIERE_ELADRIN,PM_BRALANI_ELADRIN,PM_FIRRE_ELADRIN,PM_SHIERE_ELADRIN,PM_ANGEL,PM_GHAELE_ELADRIN,PM_TULANI_ELADRIN
#define Cdemons PM_MANES,PM_QUASIT,PM_VROCK,PM_HEZROU,PM_NALFESHNEE,PM_MARILITH,PM_BALROG

NEARDATA const struct god base_godlist[] = {
#endif
/* dummy so all non-zero elements are interesting */
GOD_V2((const char *)0, 0, 0, MINIONS()),
/* archeologist */
GOD_V2("Quetzalcoatl",          A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_COUATL)
	),
GOD_V2("Camaxtli",              A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(NElemen)
	),
GOD_V2("Huhetotl",              A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* barbarian */
GOD_V2("Mitra",                 A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Crom",                  A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(NElemen)
	),
GOD_V2("Set",                   A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* caveman */
GOD_V2("Anu",                   A_LAWFUL, NEUTRAL_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("_Ishtar",               A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_COURE_ELADRIN, PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, PM_SON_OF_TYPHON)
	),
GOD_V2("Anshar",                A_CHAOTIC, NEUTRAL_HOLINESS,
	MINIONS(Cdemons)
	),
/* convict */
GOD_V2("Ilmater",               A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Grumbar",               A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(NElemen)
	),
GOD_V2("_Tymora",               A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),
/* healer, Greek */
GOD_V2("_Athena",               A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_MYRMIDON_HOPLITE,PM_MYRMIDON_LOCHIAS,PM_MYRMIDON_YPOLOCHAGOS,PM_DEMINYMPH,PM_MYRMIDON_LOCHAGOS,PM_SHIELD_ARCHON,PM_SWORD_ARCHON,PM_LIGHT_ARCHON)
	),
GOD_V2("Hermes",                A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_QUICKLING,PM_AIR_ELEMENTAL,PM_LIGHTNING_PARAELEMENTAL,PM_BANDERSNATCH,PM_MORTAI)
	),
GOD_V2("Poseidon",              A_CHAOTIC, NEUTRAL_HOLINESS,
	MINIONS(PM_FORD_ELEMENTAL,PM_WARHORSE,PM_NOVIERE_ELADRIN,PM_MARID,PM_WATER_ELEMENTAL,PM_ICE_PARAELEMENTAL,PM_LIGHTNING_PARAELEMENTAL,PM_UISCERRE_ELADRIN,PM_TITAN)
	),
/* acu */
GOD_V2("Resistance",                A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(Nangels)
	),
GOD_V2("Thoon",              A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),

/* knight, Irish */
GOD_V2("Lugh",                  A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_WARHORSE,PM_WHITE_UNICORN,PM_COUATL,PM_KI_RIN,PM_GIANT_EAGLE)
	),
GOD_V2("_Brigit",               A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_FIRE_ELEMENTAL,PM_FIRE_VORTEX,PM_FIRRE_ELADRIN,PM_SURYA_DEVA)
	),
GOD_V2("Manannan Mac Lir",      A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_NAIAD,PM_NOVIERE_ELADRIN,PM_WATER_ELEMENTAL,PM_ICE_PARAELEMENTAL,PM_LIGHTNING_PARAELEMENTAL,PM_STORM_GIANT)
	),
/* monk, Chinese */
GOD_V2("Shan Lai Ching",        A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_OREAD,PM_EARTH_ELEMENTAL,PM_WATER_ELEMENTAL,PM_MAHADEVA)
	),
GOD_V2("Chih Sung-tzu",         A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_NAIAD,PM_WATER_ELEMENTAL,PM_MORTAI)
	),
GOD_V2("Huan Ti",               A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_SOLDIER,PM_TERRACOTTA_SOLDIER,PM_TULANI_ELADRIN,PM_LIGHT_ARCHON)
	),
/* noble, Romanian, sorta */
GOD_V2("God the Father",        A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("_Mother Earth",         A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(NElemen)
	),
GOD_V2("the Satan",             A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* pirate, Christian, sorta */
GOD_V2("the Lord",              A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("_the deep blue sea",    A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_NOVIERE_ELADRIN,PM_MARID,PM_WATER_ELEMENTAL,PM_ICE_PARAELEMENTAL,PM_LIGHTNING_PARAELEMENTAL,PM_MORTAI)
	),
GOD_V2("the Devil",             A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* rogue,  Nehwon */
GOD_V2("Issek",                 A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_WATER_ELEMENTAL,PM_ANGEL,PM_THRONE_ARCHON)
	),
GOD_V2("Mog",                   A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_GIANT_SPIDER,PM_MIRKWOOD_SPIDER,PM_ANGEL,PM_MAHADEVA)
	),
GOD_V2("Kos",                   A_CHAOTIC, NEUTRAL_HOLINESS,
	MINIONS(PM_ICE_VORTEX,PM_FIRE_VORTEX,PM_BRALANI_ELADRIN,PM_GHAELE_ELADRIN,PM_TULANI_ELADRIN)
	),
/* ranger, Roman */
GOD_V2("Apollo",                A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_PLAINS_CENTAUR,PM_MOUNTAIN_CENTAUR,PM_OREAD,PM_TRUMPET_ARCHON,PM_LIGHT_ARCHON)
	),
GOD_V2("_Latona",               A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_NAIAD,PM_OREAD,PM_EARTH_ELEMENTAL,PM_WATER_ELEMENTAL,PM_TITAN)
	),
GOD_V2("_Diana",                A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_PLAINS_CENTAUR,PM_NAIAD,PM_FOREST_CENTAUR,PM_TULANI_ELADRIN)
	),
/* samurai, Japanese */
GOD_V2("_Amaterasu Omikami",    A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Raijin",                A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(Nangels)
	),
GOD_V2("Susanowo",              A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),
/* tourist, Discworld */
GOD_V2("Blind Io",              A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("_The Lady",             A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(Nangels)
	),
GOD_V2("Offler",                A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_BABY_CROCODILE,PM_CROCODILE,PM_ZRUTY,PM_MARILITH,PM_AMMIT)
	),
/* bard, Thracian */
GOD_V2("Apollon",               A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_NAIAD,PM_MOVANIC_DEVA,PM_MONADIC_DEVA,PM_LIGHT_ARCHON)
	),
GOD_V2("Pan",                   A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_DRYAD,PM_DEMINYMPH,PM_MONADIC_DEVA,PM_GAE_ELADRIN)
	),
GOD_V2("Dionysus",              A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_NAIAD,PM_NOVIERE_ELADRIN,PM_DEMINYMPH,PM_GAE_ELADRIN)
	),
/* madman */
GOD_V2("Zo-Kalar",              A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Lobon",                 A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(Nangels)
	),
GOD_V2("Tamash",                A_CHAOTIC, NEUTRAL_HOLINESS,
	MINIONS(PM_HOMUNCULUS,PM_COURE_ELADRIN,PM_ANGEL,PM_WARDEN_ARCHON,PM_CAILLEA_ELADRIN)
	),
/* valkyrie, Norse */
GOD_V2("Tyr",                   A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_ARCADIAN_AVENGER,PM_SHIELD_ARCHON,PM_ANGEL,PM_WARDEN_ARCHON,PM_THRONE_ARCHON)
	),
GOD_V2("Odin",                  A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_WARHORSE,PM_ARCADIAN_AVENGER,PM_VALKYRIE,PM_SURYA_DEVA)
	),
GOD_V2("Loki",                  A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(PM_HELL_HOUND_PUP,PM_HELL_HOUND,PM_FIRE_GIANT,PM_BRIGHID_ELADRIN)
	),
GOD_V2("Skadi",                 A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_WINTER_WOLF_CUB,PM_WINTER_WOLF,PM_FROST_GIANT,PM_WHITE_DRAGON,PM_TITAN)
	),
/* wizard, Egyptian */
GOD_V2("Ptah",                  A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_CROCODILE,PM_SERPENT_NECKED_LIONESS,PM_PHARAOH)
	),
GOD_V2("Thoth",                 A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_PARROT,PM_GOLDEN_NAGA,PM_ASTRAL_DEVA,PM_GRAHA_DEVA,PM_PHARAOH)
	),
GOD_V2("Anhur",                 A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(PM_VROCK,PM_MARILITH,PM_ANUBITE,PM_AMMIT)
	),
/* elf ranger */
GOD_V2("Orome",                 A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_PLAINS_CENTAUR,PM_FOREST_CENTAUR,PM_MOUNTAIN_CENTAUR,PM_HUNTER,PM_TITAN)
	),
GOD_V2("_Yavanna",              A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_GRAY_UNICORN,PM_EARTH_ELEMENTAL,PM_WOOD_TROLL,PM_TITAN)
	),
GOD_V2("Tulkas",                A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),
/* elf priestess */
GOD_V2("_Varda Elentari",       A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_WHITE_UNICORN,PM_AIR_ELEMENTAL,PM_ASTRAL_DEVA,PM_ANGEL,PM_GRAHA_DEVA,PM_LIGHT_ARCHON)
	),
GOD_V2("_Vaire",                A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_GRAY_UNICORN,PM_MOVANIC_DEVA,PM_ASTRAL_DEVA,PM_MAHADEVA)
	),
GOD_V2("_Nessa",                A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),
/* elf priest */
GOD_V2("Manwe Sulimo",           A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_WHITE_UNICORN,PM_AIR_ELEMENTAL,PM_ANGEL,PM_SURYA_DEVA,PM_GIANT_EAGLE,PM_THRONE_ARCHON)
	),
GOD_V2("Mandos",                 A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_GRAY_UNICORN,PM_MOVANIC_DEVA,PM_ASTRAL_DEVA,PM_GRAHA_DEVA,PM_MAHADEVA)
	),
GOD_V2("Lorien",                 A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(Cangels)
	),
/* anachrononaut */
GOD_V2("_Ilsensine",             A_LAWFUL, UNHOLY_HOLINESS,
	MINIONS(PM_MIND_FLAYER,PM_BRAIN_GOLEM,PM_SEMBLANCE,PM_MASTER_MIND_FLAYER)
	),
/* hedrow shared */
GOD_V2("Eddergud",               A_LAWFUL, UNHOLY_HOLINESS,
	MINIONS(PM_DROW_MUMMY,PM_EDDERKOP,PM_EDDERKOP,PM_EDDERKOP,PM_DROW_ALIENIST,PM_EMBRACED_DROWESS)
	),
GOD_V2("Vhaeraun",               A_NEUTRAL, UNHOLY_HOLINESS,
	MINIONS(PM_HEDROW_WARRIOR,PM_PHASE_SPIDER,PM_MIRKWOOD_ELDER)
	),
GOD_V2("_Lolth",                 A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(PM_GIANT_SPIDER,PM_MIRKWOOD_SPIDER,PM_MIRKWOOD_ELDER)
	),
/* hedrow noble */
GOD_V2("_Ver'tas",               A_LAWFUL, UNHOLY_HOLINESS,
	MINIONS(PM_HEDROW_WARRIOR,PM_BEHOLDER,PM_ANGEL,PM_EYE_OF_DOOM)
	),
GOD_V2("_Pen'a",                 A_NEUTRAL, HOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("Keptolo",                A_NEUTRAL, UNHOLY_HOLINESS,
	MINIONS(PM_HEDROW_WARRIOR,PM_PHASE_SPIDER,PM_MIRKWOOD_ELDER)
	),
GOD_V2("Ghaunadaur",             A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(PM_HEDROW_WARRIOR,PM_DRIDER,PM_HEDROW_WIZARD,PM_SHOGGOTH,PM_PRIESTESS_OF_GHAUNADAUR,PM_PRIEST_OF_GHAUNADAUR)
	),
/* Drow [+noble] shared  */
GOD_V2("_Eilistraee",            A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_ELF_LORD,PM_DROW_MATRON,PM_HEDROW_BLADEMASTER,PM_ANGEL)
	),
GOD_V2("_Kiaransali",            A_NEUTRAL, UNHOLY_HOLINESS,
	MINIONS(PM_HEDROW_ZOMBIE,PM_DROW_MUMMY,PM_VAMPIRE,PM_MASTER_LICH)
	),
//GOD_V2("_Lolth",                 A_CHAOTIC, UNHOLY_HOLINESS, // Lolth repeated
//	MINIONS(PM_SPROW,PM_YOCHLOL,PM_ANGEL,PM_MARILITH)
//	),
/* Binder */
GOD_V2("Yaldabaoth",             A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("the void",               A_VOID, VOID_HOLINESS,
	MINIONS(NON_PM)
	),
GOD_V2("_Pistis Sophia",         A_CHAOTIC, HOLY_HOLINESS,
	MINIONS(PM_ANGEL,PM_GRAHA_DEVA,PM_TULANI_ELADRIN,PM_SURYA_DEVA,PM_THRONE_ARCHON,PM_LIGHT_ARCHON)
	),
/* Dwarf */
GOD_V2("Mahal",                  A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_ROCK_MOLE,PM_DWARF,PM_ANGEL,PM_THRONE_ARCHON)
	),
GOD_V2("Holashner",              A_NEUTRAL, UNHOLY_HOLINESS,
	MINIONS(PM_ROCK_MOLE,PM_MIND_FLAYER,PM_PURPLE_WORM)
	),
GOD_V2("Armok",                  A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* Gnome */
GOD_V2("Kurtulmak",              A_LAWFUL, UNHOLY_HOLINESS,
	MINIONS(Ldevils)
	),
GOD_V2("Garl Glittergold",       A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_ARCADIAN_AVENGER,PM_MOVANIC_DEVA,PM_MONADIC_DEVA,PM_ASTRAL_DEVA,PM_GRAHA_DEVA,PM_SURYA_DEVA,PM_MAHADEVA)
	),
GOD_V2("Urdlen",                 A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(PM_ROCK_MOLE,PM_LONG_WORM,PM_EARTH_ELEMENTAL,PM_PURPLE_WORM)
	),
/* Half dragon noble */
GOD_V2("Gwyn, Lord of Sunlight",           A_LAWFUL, HOLY_HOLINESS,
	MINIONS(PM_UNDEAD_KNIGHT,PM_GARGOYLE,PM_WINGED_GARGOYLE,PM_WARRIOR_OF_SUNLIGHT)
	),
GOD_V2("_Gwynevere, Princess of Sunlight", A_NEUTRAL, HOLY_HOLINESS,
	MINIONS(PM_UNDEAD_MAIDEN,PM_UNDEAD_KNIGHT,PM_BLUE_SENTINEL,PM_KNIGHT_OF_THE_PRINCESS_S_GUARD,PM_DEATH_KNIGHT,PM_PISACA)
	),
GOD_V2("Dark Sun Gwyndolin",               A_CHAOTIC, NEUTRAL_HOLINESS,
	MINIONS(PM_BLUE_SENTINEL,PM_SNAKE,PM_PYTHON,PM_GARGOYLE,PM_DARKMOON_KNIGHT)
	),
GOD_V2("_Velka, Goddess of Sin",           A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(PM_UNDEAD_REBEL,PM_CROW,PM_RAVEN,PM_TENGU,PM_PARDONER,PM_OCCULTIST)
	),
/* orc noble */
GOD_V2("Ilneval",                A_LAWFUL, UNHOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("_Luthic",                A_NEUTRAL, UNHOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("Gruumsh",                A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS()
	),
/* Orc noble, elf */
GOD_V2("_Vandria",               A_LAWFUL, HOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("Corellon",               A_NEUTRAL, HOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("_Sehanine",              A_CHAOTIC, HOLY_HOLINESS,
	MINIONS()
	),
/* prc noble, human */
GOD_V2("Saint Cuthbert",         A_LAWFUL, HOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("Helm",                   A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS()
	),
GOD_V2("_Mask",                  A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS()
	),
/* salamander rebel*/
GOD_V2("Utu",        A_LAWFUL, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Kossuth",         A_NEUTRAL, NEUTRAL_HOLINESS,
	MINIONS(NElemen)
	),
GOD_V2("Garyx",             A_CHAOTIC, UNHOLY_HOLINESS,
	MINIONS(Cdemons)
	),
/* chaos quest */
GOD_V2("The Silence",            A_NONE, UNHOLY_HOLINESS,
	MINIONS()
	),
GOD_V2("Chaos",                  A_NONE, UNHOLY_HOLINESS,
	MINIONS(PM_GOBLIN,PM_WATER_ELEMENTAL,PM_FIRE_ELEMENTAL,PM_EARTH_ELEMENTAL,PM_AIR_ELEMENTAL,PM_MIND_FLAYER,PM_VAMPIRE,PM_PURPLE_WORM)
	),
//GOD_V2("Deep Chaos",              A_NONE, UNHOLY_HOLINESS,
//	MINIONS(PM_LICH,PM_MARILITH,PM_KRAKEN,PM_GREEN_DRAGON)
//	),
/* unaligned */
GOD_V2("Moloch",                 A_NONE, UNHOLY_HOLINESS,
	MINIONS()// randomly between Ldevils and Cdemons
	),
GOD_V2("an alien god",           A_NONE, NEUTRAL_HOLINESS,
	MINIONS()
	),
GOD_V2("_the Black Mother",      A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_SMALL_GOAT_SPAWN, PM_GOAT_SPAWN,PM_DEMINYMPH,PM_SWIRLING_MIST,PM_DUST_STORM,PM_ICE_STORM,PM_THUNDER_STORM,PM_FIRE_STORM,PM_GIANT_GOAT_SPAWN,PM_SHOGGOTH,PM_DARK_YOUNG,PM_BLESSED)
	),
GOD_V2("Nodens",                 A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_MOUNTAIN_CENTAUR,PM_WATER_ELEMENTAL,PM_NIGHTGAUNT,PM_TITAN)
	),
GOD_V2("_Bast",                  A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_KITTEN, PM_HOUSECAT, PM_LYNX, PM_LARGE_CAT, PM_JAGUAR, PM_PANTHER, PM_TIGER, PM_SERPENT_NECKED_LIONESS, PM_SABER_TOOTHED_CAT, PM_DISPLACER_BEAST, PM_SON_OF_TYPHON, PM_HELLCAT)
	),
GOD_V2("the Dread Fracture",     A_NONE, UNHOLY_HOLINESS,
	MINIONS(PM_AMM_KAMEREL,PM_FREEZING_SPHERE,PM_WRAITH,PM_SHADE,PM_HUDOR_KAMEREL,PM_ICE_PARAELEMENTAL,PM_SHARAB_KAMEREL,PM_DARKNESS_GIVEN_HUNGER,PM_FALLEN_ANGEL,PM_SHAYATEEN)
	),
GOD_V2("Yog-Sothoth",            A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_AOA_DROPLET,PM_HOOLOOVOO,PM_AOA,PM_UVUUDAUM)
	),
GOD_V2("Bokrug, the water-lizard", A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_BEING_OF_IB, PM_PRIEST_OF_IB)
	),
GOD_V2("the Silver Flame", A_NONE, HOLY_HOLINESS,
	MINIONS(Langels)
	),
GOD_V2("Nyarlathotep", A_NONE, NEUTRAL_HOLINESS,
	MINIONS(PM_HUNTING_HORROR)
	),
/* the great terminator */
GOD_V2((const char *)0, 0, 0, MINIONS())
};

#endif /*GODLIST_H*/
/*godlist.h*/
