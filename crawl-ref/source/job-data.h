enum weapon_choice
{
    WCHOICE_NONE,   ///< No weapon choice
    WCHOICE_PLAIN,  ///< Normal weapon choice
    WCHOICE_GOOD,   ///< Chooses from "good" weapons
    WCHOICE_RANGED, ///< Choice of ranged weapon
};

struct job_def
{
    const char* abbrev; ///< Two-letter abbreviation
    const char* name; ///< Long name
    int s, i, d; ///< Starting Str, Dex, and Int
    vector<species_type> recommended_species; ///< Which species are good at it
    /// Guaranteed starting equipment. Uses vault spec syntax, with the plus:,
    /// charges:, q:, and ego: tags supported.
    vector<string> equipment;
    weapon_choice wchoice; ///< how the weapon is chosen, if any
    vector<pair<skill_type, int>> skills; ///< starting skills
};

static const map<job_type, job_def> job_data =
{

{ JOB_AIR_ELEMENTALIST, {
    "AE", "Air Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_TENGU, SP_BASE_DRACONIAN, SP_NAGA, },
    { "robe", "book of Air" },
    WCHOICE_NONE,
    { { SK_AIR_MAGIC, 4 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_ARCANE_MARKSMAN, {
    "AM", "Arcane Marksman",
    3, 5, 4,
    { SP_FORMICID, SP_DEEP_ELF, SP_KOBOLD, SP_SPRIGGAN, SP_TROLL, },
    { "robe", "book of Arcane Marksmanship" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 1 }, { SK_DODGING, 2 },
      { SK_HEXES, 4 }, { SK_WEAPON, 2 }, },
} },

{ JOB_ARTIFICER, {
    "Ar", "Artificer",
    4, 3, 5,
    { SP_GNOME, SP_KOBOLD, SP_SPRIGGAN, SP_BASE_DRACONIAN, },
    { "dagger", "leather armour", "wand of flame charges:15",
      "wand of enslavement charges:15", "wand of iceblast charges:1" },
    WCHOICE_NONE,
    { { SK_EVOCATIONS, 3 }, { SK_DODGING, 2 }, { SK_FIGHTING, 1 },
      { SK_WEAPON, 1 }, { SK_STEALTH, 1 }, },
} },

{ JOB_ASSASSIN, {
    "As", "Assassin",
    3, 3, 6,
    { SP_TROLL, SP_SPRIGGAN, SP_VINE_STALKER, },
    { "dagger plus:2", "robe", "cloak", "tomahawk q:96",
      "tomahawk ego:dispersal q:5" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 2 }, { SK_DODGING, 1 }, { SK_STEALTH, 4 },
      { SK_THROWING, 2 }, { SK_WEAPON, 2 }, },
} },

{ JOB_EARTH_ELEMENTALIST, {
    "EE", "Earth Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_SPRIGGAN, SP_GARGOYLE, SP_VINE_STALKER,
      SP_OCTOPODE, },
    { "book of Geomancy", "robe", },
    WCHOICE_NONE,
    { { SK_TRANSMUTATIONS, 2 }, { SK_EARTH_MAGIC, 4 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, }
} },

{ JOB_ENCHANTER, {
    "En", "Enchanter",
    0, 7, 5,
    { SP_DEEP_ELF, SP_KOBOLD, SP_SPRIGGAN, SP_NAGA, },
    { "dagger plus:1", "robe plus:1", "book of Maledictions" },
    WCHOICE_NONE,
    { { SK_WEAPON, 1 }, { SK_HEXES, 4 },
      { SK_DODGING, 2 }, { SK_STEALTH, 3 }, },
} },

{ JOB_FIGHTER, {
    "Fi", "Fighter",
    8, 0, 4,
    { SP_MOUNTAIN_DWARF, SP_CENTAUR, SP_TROLL, SP_MINOTAUR, SP_GARGOYLE, SP_GNOLL },
    { "scale mail", "shield", "potion of augmentation" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 3 }, { SK_SHIELDS, 3 }, { SK_ARMOUR, 3 },
      { SK_WEAPON, 2 } },
} },

{ JOB_FIRE_ELEMENTALIST, {
    "FE", "Fire Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_MOUNTAIN_DWARF, SP_NAGA, SP_TENGU, SP_GARGOYLE, },
    { "robe", "book of Flames" },
    WCHOICE_NONE,
    { { SK_FIRE_MAGIC, 4 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_GLADIATOR, {
    "Gl", "Gladiator",
    7, 0, 5,
    { SP_MOUNTAIN_DWARF, SP_CENTAUR, SP_MERFOLK, SP_MINOTAUR, SP_GARGOYLE, SP_GNOLL},
    { "leather armour", "helmet", "tomahawk q:12",
      "tomahawk ego:dispersal q:5" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 2 }, { SK_THROWING, 2 }, { SK_DODGING, 3 },
      { SK_WEAPON, 3}, },
} },

{ JOB_HUNTER, {
    "Hu", "Hunter",
    4, 3, 5,
    { SP_MOUNTAIN_DWARF, SP_CENTAUR, SP_GNOME, SP_KOBOLD, SP_OGRE, SP_TROLL, SP_GNOLL},
    { "dagger", "leather armour" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 2 }, { SK_DODGING, 2 }, { SK_STEALTH, 1 },
      { SK_WEAPON, 4 }, },
} },

{ JOB_ICE_ELEMENTALIST, {
    "IE", "Ice Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_MERFOLK, SP_NAGA, SP_BASE_DRACONIAN,
      SP_GARGOYLE, },
    { "robe", "book of Frost" },
    WCHOICE_NONE,
    { { SK_ICE_MAGIC, 4 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_MONK, {
    "Mo", "Monk",
    3, 2, 7,
    { SP_MOUNTAIN_DWARF, SP_TROLL, SP_MERFOLK, SP_GARGOYLE, },
    { "robe" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_WEAPON, 3 }, { SK_DODGING, 3 },
      { SK_STEALTH, 2 }, {SK_INVOCATIONS, 3} },
} },

{ JOB_SKALD, {
    "Sk", "Skald",
    4, 4, 4,
    { SP_GNOME, SP_MERFOLK, SP_BASE_DRACONIAN, SP_GNOLL},
    { "leather armour", "book of Battle" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 2 }, { SK_ARMOUR, 1 }, { SK_DODGING, 1 },
      { SK_CHARMS, 4 }, { SK_WEAPON, 2 }, },
} },

{ JOB_SUMMONER, {
    "Su", "Summoner",
    0, 7, 5,
    { SP_DEEP_ELF, SP_MOUNTAIN_DWARF, SP_VINE_STALKER, SP_MERFOLK, SP_TENGU, },
    { "robe", "book of Callings" },
    WCHOICE_NONE,
    { { SK_SUMMONINGS, 5 }, { SK_DODGING, 2 },
      { SK_STEALTH, 2 }, },
} },

{ JOB_TRANSMUTER, {
    "Tm", "Transmuter",
    2, 5, 5,
    { SP_NAGA, SP_MERFOLK, SP_BASE_DRACONIAN, SP_TROLL, },
    { "robe", "book of Changes" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 1 }, { SK_UNARMED_COMBAT, 3 }, { SK_DODGING, 2 },
      { SK_TRANSMUTATIONS, 3 }, },
} },

{ JOB_WANDERER, {
    "Wn", "Wanderer",
    0, 0, 0, // Randomised
    { SP_MOUNTAIN_DWARF, SP_SPRIGGAN, SP_MERFOLK, SP_BASE_DRACONIAN,
      SP_HUMAN, },
    { }, // Randomised
    WCHOICE_NONE,
    { }, // Randomised
} },

{ JOB_WARPER, {
    "Wr", "Warper",
    3, 5, 4,
    { SP_SPRIGGAN, SP_CENTAUR, SP_BASE_DRACONIAN, },
    { "leather armour", "book of Spatial Translocations", "scroll of blinking",
      "tomahawk ego:dispersal q:5" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 2 }, { SK_ARMOUR, 1 }, { SK_DODGING, 2 },
      { SK_TRANSLOCATIONS, 3 }, { SK_THROWING, 1 },
      { SK_WEAPON, 2 }, },
} },

{ JOB_WIZARD, {
    "Wz", "Hedge Wizard",
    -1, 10, 3,
    { SP_DEEP_ELF, SP_NAGA, SP_BASE_DRACONIAN, SP_OCTOPODE, SP_HUMAN, },
    { "robe", "hat", "book of Minor Magic" },
    WCHOICE_NONE,
    { { SK_DODGING, 2 }, { SK_STEALTH, 2 },
      { SK_TRANSLOCATIONS, 1 }, { SK_SUMMONINGS, 1 }, { SK_AIR_MAGIC, 1 },
      { SK_TRANSMUTATIONS, 1 }, { SK_EARTH_MAGIC, 1 }, { SK_HEXES, 1 },
      { SK_NECROMANCY, 1 }, { SK_FIRE_MAGIC, 1 },
    },
} },

{ JOB_CONJURER, {
    "Cj", "Conjurer",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_HEALER, {
    "He", "Healer",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_PRIEST, {
    "Pr", "Priest",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_STALKER, {
    "St", "Stalker",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_VENOM_MAGE, {
    "VM", "Venom Mage",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

};
