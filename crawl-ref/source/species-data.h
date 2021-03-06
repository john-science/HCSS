enum species_flag
{
    SPF_NONE        = 0,
    SPF_ELVEN       = 1 << 0, /// If this species counts as an elf
    SPF_DRACONIAN   = 1 << 1, /// If this is a draconian subspecies
    SPF_ORCISH      = 1 << 2, /// If this species is a kind of orc
    SPF_NO_HAIR     = 1 << 3, /// If members of the species are hairless
    SPF_SMALL_TORSO = 1 << 4, /// Torso is smaller than body
};
DEF_BITFIELD(species_flags, species_flag);

struct level_up_mutation
{
    mutation_type mut; ///< What mutation to give
    int mut_level; ///< How much to give
    int xp_level; ///< When to give it (if 1, is a starting mutation)
};

struct species_def
{
    const char* abbrev; ///< Two-letter abbreviation
    const char* name; ///< Main name
    const char* adj_name; ///< Adjectival form of name; if null, use name
    const char* genus_name; ///< Genus name; if null, use name
    species_flags flags; ///< Miscellaneous flags
    // The following three need to be 2 lines after the name for gen-apt.pl:
    int xp_mod; ///< Experience level modifier
    int hp_mod; ///< HP modifier (in tenths)
    int mp_mod; ///< MP modifier
    int mr_mod; ///< MR modifier (multiplied by XL for base MR)
    monster_type monster_species; ///< Corresponding monster (for display)
    habitat_type habitat; ///< Where it can live; HT_WATER -> no penalties
    undead_state_type undeadness; ///< What kind of undead (if any)
    size_type size; ///< Size of body
    int s, i, d; ///< Starting stats contribution
    set<stat_type> level_stats; ///< Which stats to gain on level-up
    int how_often; ///< When to level-up stats
    vector<level_up_mutation> level_up_mutations; ///< Mutations on level-up
    vector<string> verbose_fake_mutations; ///< Additional information on 'A'
    vector<string> terse_fake_mutations; ///< Additional information on '%'
    vector<job_type> recommended_jobs; ///< Which jobs are "good" for it
    vector<skill_type> recommended_weapons; ///< Which weapons types are "good"
};

static const map<species_type, species_def> species_data =
{

{ SP_DEEP_ELF, {
    "DE",
    "Deep Elf", "Elven", "Elf",
    SPF_ELVEN,
    -1, -2, 2, 4,
    MONS_ELF,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    5, 12, 10, // 27
    { STAT_INT }, 4,
    { { MUT_MAGIC_ATTUNEMENT, 1, 1 }, },
    {},
    {},
    { JOB_WIZARD, JOB_SUMMONER,
      JOB_FIRE_ELEMENTALIST, JOB_ICE_ELEMENTALIST, JOB_AIR_ELEMENTALIST,
      JOB_EARTH_ELEMENTALIST },
    { SK_SHORT_BLADES, SK_BOWS },
} },

{ SP_BASE_DRACONIAN, {
    "Dr",
    "Draconian", nullptr, nullptr,
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, },
    {},
    {},
    { JOB_TRANSMUTER, JOB_FIRE_ELEMENTALIST,
      JOB_ICE_ELEMENTALIST, JOB_AIR_ELEMENTALIST, JOB_EARTH_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_RED_DRACONIAN, {
    "Dr",
    "Red Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_RED_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_HEAT_RESISTANCE, 1, 7 }, },
    { "You can breathe blasts of fire." },
    { "breathe fire" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_WHITE_DRACONIAN, {
    "Dr",
    "White Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_WHITE_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_COLD_RESISTANCE, 1, 7 }, },
    { "You can breathe waves of freezing cold.",
      "You can buffet flying creatures when you breathe cold." },
    { "breathe frost" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_GREEN_DRACONIAN, {
    "Dr",
    "Green Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_GREEN_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_POISON_RESISTANCE, 1, 7 },
      { MUT_STINGER, 1, 14 }, },
    { "You can breathe blasts of noxious fumes." },
    { "breathe noxious fumes" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_YELLOW_DRACONIAN, {
    "Dr",
    "Yellow Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_YELLOW_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_ACIDIC_BITE, 1, 14 }, },
    { "You can spit globs of acid.", "You are resistant to acid." },
    { "spit acid", "acid resistance" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_GREY_DRACONIAN, {
    "Dr",
    "Grey Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_GREY_DRACONIAN,
    HT_AMPHIBIOUS, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_UNBREATHING, 1, 7 }, },
    { "You can walk through water." },
    { "walk through water" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_BLACK_DRACONIAN, {
    "Dr",
    "Black Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_BLACK_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, { MUT_SHOCK_RESISTANCE, 1, 7 },
      { MUT_BIG_WINGS, 1, 14 }, },
    { "You can breathe wild blasts of lightning." },
    { "breathe lightning" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_PURPLE_DRACONIAN, {
    "Dr",
    "Purple Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 6,
    MONS_PURPLE_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, },
    { "You can breathe bolts of dispelling energy." },
    { "breathe power" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_MOTTLED_DRACONIAN, {
    "Dr",
    "Mottled Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_MOTTLED_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, },
    { "You can spit globs of burning liquid.",
      "You can ignite nearby creatures when you spit burning liquid." },
    { "breathe sticky flame splash" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_PALE_DRACONIAN, {
    "Dr",
    "Pale Draconian", "Draconian", "Draconian",
    SPF_DRACONIAN,
    -1, 1, 0, 3,
    MONS_PALE_DRACONIAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_COLD_BLOODED, 1, 1 }, },
    { "You can breathe blasts of scalding, opaque steam." },
    { "breathe steam" },
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_FORMICID, {
    "Fo",
    "Formicid", nullptr, "Ant",
    SPF_NONE,
    1, 0, 0, 4,
    MONS_FORMICID,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    12, 7, 6, // 25
    { STAT_STR, STAT_INT }, 4,
    { { MUT_ANTENNAE, 3, 1 }, },
    { "You are under a permanent stasis effect.",
      "You can dig through walls and to a lower floor.",
      "Your four strong arms can wield two-handed weapons with a shield." },
    { "permanent stasis", "dig shafts and tunnels", "four strong arms" },
    { JOB_FIGHTER, JOB_EARTH_ELEMENTALIST, },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS },
} },

{ SP_GARGOYLE, {
    "Gr",
    "Gargoyle", nullptr, nullptr,
    SPF_NO_HAIR,
    0, -2, 0, 3,
    MONS_GARGOYLE,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    11, 8, 5, // 24
    { STAT_STR, STAT_INT }, 4,
    { { MUT_ROT_IMMUNITY, 1, 1 }, { MUT_NEGATIVE_ENERGY_RESISTANCE, 1, 1 },
      { MUT_PETRIFICATION_RESISTANCE, 1, 1 }, { MUT_SHOCK_RESISTANCE, 1, 1 },
      { MUT_UNBREATHING, 1, 1 }, { MUT_BIG_WINGS, 1, 14 }, },
    { "You are resistant to torment." },
    {},
    { JOB_FIGHTER, JOB_GLADIATOR, JOB_MONK,
      JOB_FIRE_ELEMENTALIST, JOB_ICE_ELEMENTALIST, JOB_EARTH_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_BOWS },
} },

{ SP_GNOLL, {
    "Gn",
    "Gnoll", nullptr, nullptr,
    SPF_NONE,
    0, 0, 0, 3,
    MONS_GNOLL,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    8, 8, 8, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 5,
    {},
    {},
    {},
    { JOB_SKALD, JOB_FIGHTER, JOB_ARCANE_MARKSMAN, JOB_WANDERER },
    { SK_SHORT_BLADES, SK_MACES_FLAILS, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_MOUNTAIN_DWARF, {
    "MD",
    "Mountain Dwarf", "Dwarven", "Dwarf",
    SPF_NONE,
    0, 0, 0, 3,
    MONS_DEEP_DWARF,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR }, 5,
    { {MUT_STURDY_FRAME, 1, 1 }, {MUT_WILD_MAGIC, 1, 6}, {MUT_WILD_MAGIC, 1, 12},
      {MUT_WILD_MAGIC, 1, 18},},
    {},
    {},
    { JOB_FIGHTER,
      JOB_EARTH_ELEMENTALIST, JOB_FIRE_ELEMENTALIST, },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS },
} },

{ SP_HUMAN, {
    "Hu",
    "Human", nullptr, nullptr,
    SPF_NONE,
    2, 0, 0, 3,
    MONS_HUMAN,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    8, 8, 8, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    {},
    {},
    {},
    { JOB_FIRE_ELEMENTALIST, JOB_ICE_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_KOBOLD, {
    "Ko",
    "Kobold", nullptr, nullptr,
    SPF_NONE,
    1, -2, 0, 3,
    MONS_KOBOLD,
    HT_LAND, US_ALIVE, SIZE_SMALL,
    6, 11, 11, // 28
    { STAT_DEX, STAT_INT }, 5,
    {{ MUT_EVOLUTION, 1, 2 }, },
    {},
    {},
    { JOB_ARCANE_MARKSMAN, JOB_ENCHANTER,
      JOB_SUMMONER, JOB_WIZARD },
    { SK_SHORT_BLADES, SK_MACES_FLAILS },
} },

{ SP_MERFOLK, {
    "Mf",
    "Merfolk", "Merfolkian", nullptr,
    SPF_NONE,
    0, 0, 0, 3,
    MONS_MERFOLK,
    HT_WATER, US_ALIVE, SIZE_MEDIUM,
    8, 7, 9, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 5,
    {},
    { "You revert to your normal form in water.",
      "You are very nimble and swift while swimming.",
      "You are very stealthy in the water." },
    { "change form in water", "swift swim", "stealthy swim" },
    { JOB_GLADIATOR, JOB_SKALD, JOB_TRANSMUTER, JOB_SUMMONER,
      JOB_ICE_ELEMENTALIST },
    { SK_POLEARMS },
} },

{ SP_MINOTAUR, {
    "Mi",
    "Minotaur", nullptr, nullptr,
    SPF_NONE,
    -1, 1, -1, 3,
    MONS_MINOTAUR,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    12, 5, 5, // 22
    { STAT_STR, STAT_DEX }, 4,
    { { MUT_HORNS, 2, 1 }, },
    { "You reflexively headbutt those who attack you in melee." },
    { "retaliatory headbutt" },
    { JOB_FIGHTER, JOB_GLADIATOR, JOB_HUNTER, JOB_MONK, },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_NAGA, {
    "Na",
    "Naga", nullptr, nullptr,
    SPF_SMALL_TORSO,
    0, 2, 0, 5,
    MONS_NAGA,
    HT_LAND, US_ALIVE, SIZE_LARGE,
    10, 8, 6, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_ACUTE_VISION, 1, 1 }, { MUT_SLOW, 2, 1 },  { MUT_DEFORMED, 1, 1 },
      { MUT_SPIT_POISON, 1, 1 },  { MUT_POISON_RESISTANCE, 1, 1 },
      { MUT_CONSTRICTING_TAIL, 1, 7 } },
    { "You cannot wear boots." },
    {},
    { JOB_TRANSMUTER, JOB_ENCHANTER, JOB_FIRE_ELEMENTALIST,
      JOB_ICE_ELEMENTALIST, JOB_WARPER, JOB_WIZARD },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS, SK_BOWS },
} },

{ SP_OGRE, {
    "Og",
    "Ogre", "Ogreish", nullptr,
    SPF_NONE,
    0, 3, 0, 4,
    MONS_OGRE,
    HT_LAND, US_ALIVE, SIZE_LARGE,
    12, 7, 5, // 24
    { STAT_STR }, 3,
    { { MUT_TOUGH_SKIN, 1, 1 }, },
    {},
    {},
    { JOB_HUNTER, JOB_ARCANE_MARKSMAN, JOB_WIZARD,
      JOB_FIRE_ELEMENTALIST },
    { SK_MACES_FLAILS },
} },

{ SP_OCTOPODE, {
    "Op",
    "Octopode", "Octopoid", "Octopus",
    SPF_NO_HAIR,
    0, -1, 0, 3,
    MONS_OCTOPODE,
    HT_WATER, US_ALIVE, SIZE_MEDIUM,
    7, 10, 7, // 24
    { STAT_STR, STAT_INT, STAT_DEX }, 5,
    { { MUT_CAMOUFLAGE, 1, 1 }, { MUT_GELATINOUS_BODY, 1, 1 }, },
    { "You cannot wear most types of armour.",
      "You are very stealthy in the water." },
    { "almost no armour", "stealthy swim" },
    { JOB_TRANSMUTER, JOB_WIZARD, JOB_ASSASSIN,
      JOB_FIRE_ELEMENTALIST, JOB_ICE_ELEMENTALIST, JOB_EARTH_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_SPRIGGAN, {
    "Sp",
    "Spriggan", nullptr, nullptr,
    SPF_NONE,
    -1, -3, 1, 7,
    MONS_SPRIGGAN,
    HT_LAND, US_ALIVE, SIZE_LITTLE,
    4, 9, 11, // 24
    { STAT_INT, STAT_DEX }, 5,
    { { MUT_FAST, 3, 1 }, { MUT_ACUTE_VISION, 1, 1 }, },
    {},
    {},
    { JOB_ARTIFICER, JOB_WARPER, JOB_ENCHANTER, JOB_EARTH_ELEMENTALIST,
      JOB_ARCANE_MARKSMAN },
    { SK_SHORT_BLADES },
} },

{ SP_TENGU, {
    "Te",
    "Tengu", nullptr, nullptr,
    SPF_NO_HAIR,
    0, -2, 1, 3,
    MONS_TENGU,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    8, 8, 9, // 25
    { STAT_STR, STAT_INT, STAT_DEX }, 4,
    { { MUT_BEAK, 1, 1 }, { MUT_TALONS, 3, 1 }, { MUT_CLAWS, 1, 1 }, { MUT_HORNS, 1, 1 },
      { MUT_TENGU_FLIGHT, 1, 5 }, },
    {},
    {},
    { JOB_WIZARD, JOB_SUMMONER, JOB_ASSASSIN,
      JOB_FIRE_ELEMENTALIST, JOB_AIR_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_TROLL, {
    "Tr",
    "Troll", "Trollish", nullptr,
    SPF_NONE,
    -1, 3, -1, 3,
    MONS_TROLL,
    HT_LAND, US_ALIVE, SIZE_LARGE,
    15, 4, 5, // 24
    { STAT_STR }, 3,
    { { MUT_TOUGH_SKIN, 1, 1 }, { MUT_REGENERATION, 1, 1 }, { MUT_CLAWS, 1, 1 },
      { MUT_CLAWS, 1, 11 }, { MUT_CLAWS, 1, 21 },},
    {},
    {},
    { JOB_FIGHTER, JOB_MONK, JOB_HUNTER, JOB_WARPER,
      JOB_EARTH_ELEMENTALIST, JOB_WIZARD },
    { SK_UNARMED_COMBAT, SK_MACES_FLAILS },
} },

{ SP_VINE_STALKER, {
    "VS",
    "Vine Stalker", "Vine", "Vine",
    SPF_NONE,
    0, -3, 1, 5,
    MONS_VINE_STALKER,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 9, // 27
    { STAT_STR, STAT_DEX }, 4,
    { { MUT_FANGS, 2, 1 }, { MUT_FANGS, 1, 8 },
      { MUT_MANA_SHIELD, 1, 1 }, { MUT_ANTIMAGIC_BITE, 1, 1 },
      { MUT_NO_DEVICE_HEAL, 3, 1 }, { MUT_ROT_IMMUNITY, 1, 1 },
      { MUT_REGENERATION, 1, 4 }, { MUT_REGENERATION, 1, 12 }, },
    {},
    {},
    { JOB_FIGHTER, JOB_ASSASSIN, JOB_ENCHANTER,
      JOB_ICE_ELEMENTALIST },
    { SK_SHORT_BLADES, SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_SLUDGE_ELF, {
    "SE",
    "Sludge Elf", "Elven", "Elf",
    SPF_ELVEN,
    0, -1, 1, 3,
    MONS_ELF,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    8, 8, 8, // 24
    { STAT_INT, STAT_DEX }, 4,
    {},
    {},
    {},
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_LAVA_ORC, {
    "LO",
    "Lava Orc", "Orcish", "Orc",
    SPF_ORCISH | SPF_NO_HAIR,
    -1, 1, 0, 3,
    MONS_LAVA_ORC,
    HT_AMPHIBIOUS_LAVA, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_INT, STAT_DEX }, 5,
    {},
    {},
    {},
    {}, // not a starting race
    {}, // not a starting race
} },

{ SP_HIGH_ELF, {
    "HE",
    "High Elf", "Elven", "Elf",
    SPF_ELVEN,
    -1, -1, 1, 4,
    MONS_ELF,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    7, 11, 10, // 28
    { STAT_INT, STAT_DEX }, 3,
    {},
    {},
    {},
    { JOB_SKALD, JOB_WIZARD, JOB_FIRE_ELEMENTALIST,
      JOB_ICE_ELEMENTALIST, JOB_AIR_ELEMENTALIST },
    { SK_SHORT_BLADES, SK_BOWS },
} },

{ SP_DEEP_DWARF, {
    "DD",
    "Deep Dwarf", "Dwarven", "Dwarf",
    SPF_NONE,
    -1, 2, 0, 6,
    MONS_DEEP_DWARF,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    11, 8, 8, // 27
    { STAT_STR, STAT_INT }, 4,
    { { MUT_NO_REGENERATION, 1, 1 }, { MUT_PASSIVE_MAPPING, 1, 1 },
      { MUT_NEGATIVE_ENERGY_RESISTANCE, 1, 14 }, },
    { "You are resistant to damage.",
      "You can recharge devices by infusing magical energy." },
    { "damage resistance", "recharge devices" },
    { JOB_FIGHTER, JOB_EARTH_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES },
} },

{ SP_HILL_ORC, {
    "HO",
    "Hill Orc", "Orcish", "Orc",
    SPF_ORCISH,
    0, 1, 0, 3,
    MONS_ORC,
    HT_LAND, US_ALIVE, SIZE_MEDIUM,
    10, 8, 6, // 24
    { STAT_STR }, 5,
    {},
    {},
    {},
    { JOB_FIGHTER, JOB_MONK, JOB_FIRE_ELEMENTALIST, },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS },
} },

{ SP_CENTAUR, {
    "Ce",
    "Centaur", nullptr, nullptr,
    SPF_SMALL_TORSO,
    -1, 1, 0, 3,
    MONS_CENTAUR,
    HT_LAND, US_ALIVE, SIZE_LARGE,
    10, 7, 4, // 21
    { STAT_STR, STAT_DEX }, 4,
    { { MUT_TOUGH_SKIN, 1, 1 }, { MUT_FAST, 2, 1 },  { MUT_DEFORMED, 1, 1 },
      { MUT_HOOVES, 3, 1 }, },
    {},
    {},
    { JOB_FIGHTER, JOB_GLADIATOR, JOB_HUNTER, JOB_ARCANE_MARKSMAN, JOB_WARPER },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_GNOME, {
    "No",
    "Gnome", nullptr, nullptr,
    SPF_NONE,
    1, -1, 0, 3,
    MONS_GNOME,
    HT_LAND, US_ALIVE, SIZE_SMALL,
    9, 6, 9, // 24
    { STAT_DEX }, 5,
    { { MUT_MUTATION_RESISTANCE, 1, 1 }, },
    {},
    {},
    { JOB_FIGHTER, JOB_SKALD, JOB_HUNTER, },
    { SK_SHORT_BLADES, SK_AXES },
} },

{ SP_FROGTAUR, {
    "Fr",
    "Frogtaur", nullptr, "Frog",
    SPF_NO_HAIR,
    0, 0, 0, 3,
    MONS_FROGTAUR,
    HT_WATER, US_ALIVE, SIZE_MEDIUM,
    6, 6, 6, // 18
    { STAT_STR, STAT_INT, STAT_DEX }, 5,
    { { MUT_SLOW, 1, 1 }, { MUT_HOP, 1, 1}, {MUT_HOP, 1, 13}, },
    { "You can swim through water.", },
    { "swims", },
    { JOB_FIGHTER, JOB_SKALD, JOB_SUMMONER, JOB_ICE_ELEMENTALIST },
    { SK_MACES_FLAILS, SK_AXES, SK_POLEARMS,
      SK_BOWS },
} },

{ SP_GHOUL, {
    "Gh",
    "Ghoul", "Ghoulish", nullptr,
    SPF_NO_HAIR,
    0, 1, -1, 3,
    MONS_GHOUL,
    HT_LAND, US_HUNGRY_DEAD, SIZE_MEDIUM,
    11, 3, 4, // 18
    { STAT_STR }, 5,
    { { MUT_NEGATIVE_ENERGY_RESISTANCE, 3, 1 },
      { MUT_TORMENT_RESISTANCE, 1, 1 },
      { MUT_INHIBITED_REGENERATION, 1, 1 }, { MUT_COLD_RESISTANCE, 1, 1 },
      { MUT_CLAWS, 1, 1 }, { MUT_UNBREATHING, 1, 1 }, },
    { "You restore health by killing enemies." },
    { "hp from kills" },
    { JOB_GLADIATOR, JOB_MONK,
      JOB_ICE_ELEMENTALIST, JOB_EARTH_ELEMENTALIST },
    { SK_UNARMED_COMBAT, SK_BOWS },
} },

// Ideally this wouldn't be necessary...
{ SP_UNKNOWN, { // Line 1: enum
    "??", // Line 2: abbrev
    "Yak", nullptr, nullptr, // Line 3: name, genus name, adjectival name
    SPF_NONE, // Line 4: flags
    0, 0, 0, 0, // Line 5: XP, HP, MP, MR (gen-apt.pl needs them here!)
    MONS_PROGRAM_BUG, // Line 6: equivalent monster type
    HT_LAND, US_ALIVE, SIZE_MEDIUM, // Line 7: habitat, life, size
    0, 0, 0, // Line 8: str, int, dex
    set<stat_type>(), 28, // Line 9: str gain, int gain, dex gain, frequency
    {}, // Line 10: Mutations
    {}, // Line 11: Fake mutations
    {}, // Line 12: Fake mutations
    {}, // Line 13: Recommended jobs
    {}, // Line 14: Recommended weapons
} }
};
