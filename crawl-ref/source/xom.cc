/**
 * @file
 * @brief All things Xomly
**/

#include "AppHdr.h"

#include "xom.h"

#include <algorithm>
#include <functional>

#include "abyss.h"
#include "acquire.h"
#include "act-iter.h"
#include "areas.h"
#include "artefact.h"
#include "cloud.h"
#include "coordit.h"
#include "database.h"
#ifdef WIZARD
#include "dbg-util.h"
#endif
#include "delay.h"
#include "directn.h"
#include "english.h"
#include "env.h"
#include "errors.h"
#include "goditem.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "item_use.h"
#include "losglobal.h"
#include "makeitem.h"
#include "message.h"
#include "misc.h"
#include "mon-behv.h"
#include "mon-death.h"
#include "mon-place.h"
#include "mon-poly.h"
#include "mon-tentacle.h"
#include "mutation.h"
#include "nearby-danger.h"
#include "notes.h"
#include "output.h"
#include "player-equip.h"
#include "player-stats.h"
#include "potion.h"
#include "prompt.h"
#include "religion.h"
#include "shout.h"
#include "spl-clouds.h"
#include "spl-goditem.h"
#include "spl-miscast.h"
#include "spl-monench.h"
#include "spl-transloc.h"
#include "stairs.h"
#include "stash.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "teleport.h"
#include "terrain.h"
#include "transform.h"
#include "traps.h"
#include "travel.h"
#include "unwind.h"
#include "viewchar.h"
#include "view.h"

#ifdef DEBUG_XOM
#    define DEBUG_RELIGION
#    define NOTE_DEBUG_XOM
#endif

#ifdef DEBUG_RELIGION
#    define DEBUG_DIAGNOSTICS
#    define DEBUG_GIFTS
#endif

static void _do_xom_event(xom_event_type event_type, int sever);
static int _xom_event_badness(xom_event_type event_type);

static bool _action_is_bad(xom_event_type action)
{
    return action > XOM_LAST_GOOD_ACT && action <= XOM_LAST_BAD_ACT;
}

// Spells later in the list require higher severity to have a chance of being
// selected.
static const vector<spell_type> _xom_random_spells =
{
    SPELL_SUMMON_BUTTERFLIES,
    SPELL_SUMMON_SMALL_MAMMAL,
    SPELL_CONFUSING_TOUCH,
    SPELL_CALL_CANINE_FAMILIAR,
    SPELL_OLGREBS_TOXIC_RADIANCE,
    SPELL_SUMMON_ICE_BEAST,
    SPELL_LEDAS_LIQUEFACTION,
    SPELL_CAUSE_FEAR,
    SPELL_INTOXICATE,
    SPELL_SHADOW_CREATURES,
    SPELL_SUMMON_MANA_VIPER,
    SPELL_STATUE_FORM,
    SPELL_DISPERSAL,
    SPELL_ENGLACIATION,
    SPELL_SUMMON_HYDRA,
    SPELL_MONSTROUS_MENAGERIE,
    SPELL_DISCORD,
    SPELL_DISJUNCTION,
    SPELL_CONJURE_BALL_LIGHTNING,
    SPELL_DRAGON_FORM,
    SPELL_SUMMON_HORRIBLE_THINGS,
    SPELL_SUMMON_DRAGON,
    SPELL_NECROMUTATION,
    SPELL_CHAIN_OF_CHAOS
};

static const char* xom_moods[] = {
    "a very special plaything of Xom.",
    "a special plaything of Xom.",
    "a plaything of Xom.",
    "a toy of Xom.",
    "a favourite toy of Xom.",
    "a beloved toy of Xom.",
    "Xom's teddy bear."
};

static const char *describe_xom_mood()
{
    const int mood = random2(7);
    ASSERT(mood >= 0);
    ASSERT((size_t) mood < ARRAYSZ(xom_moods));
    return xom_moods[mood];
}

const string describe_xom_favour()
{
    string favour;
    if (!you_worship(GOD_XOM))
        favour = "a very buggy toy of Xom.";
    else
        favour = describe_xom_mood();

    return favour;
}

#define XOM_SPEECH(x) x
static string _get_xom_speech(const string &key)
{
    string result = getSpeakString("Xom " + key);

    if (result.empty())
        result = getSpeakString("Xom " XOM_SPEECH("general effect"));

    if (result.empty())
        return "Xom makes something happen.";

    return result;
}

static bool _xom_is_bored()
{
    return you_worship(GOD_XOM) && !you.gift_timeout;
}

static bool _xom_feels_nasty()
{
    // Xom will only directly kill you with a bad effect if you're under
    // penance from him, or if he's bored.
    return you.penance[GOD_XOM] || _xom_is_bored();
}

bool xom_is_nice(int tension)
{
    if (player_under_penance(GOD_XOM))
        return false;

    if (you_worship(GOD_XOM))
    {
        // If you.gift_timeout is 0, then Xom is BORED. He HATES that.
        if (!you.gift_timeout)
            return false;

        // At high tension Xom is more likely to be nice, at zero
        // tension the opposite.
        const int tension_bonus
            = (tension == -1 ? 0 // :
// Xom needs to be less negative
//              : tension ==  0 ? -min(abs(HALF_MAX_PIETY - you.piety) / 2,
//                                     you.piety / 10)
                             : min((MAX_PIETY - you.piety) / 2,
                                   random2(tension)));

        const int effective_piety = you.piety + tension_bonus;
        ASSERT_RANGE(effective_piety, 0, MAX_PIETY + 1);

#ifdef DEBUG_XOM
        mprf(MSGCH_DIAGNOSTICS,
             "Xom: tension: %d, piety: %d -> tension bonus = %d, eff. piety: %d",
             tension, you.piety, tension_bonus, effective_piety);
#endif

        // Whether Xom is nice depends largely on his mood (== piety).
        return x_chance_in_y(effective_piety, MAX_PIETY);
    }
    else // CARD_XOM
        return coinflip();
}

void xom_tick()
{
    int sever = 0;
    if (!you_worship(GOD_XOM) && !you.penance[GOD_XOM])
        return;
    for (radius_iterator ri(you.pos(), LOS_RADIUS, C_SQUARE); ri; ++ri)
    {
            monster* mons = monster_at(*ri);
            if (!mons || !you.see_cell(*ri))
                continue;
            if (!xom_wants_to_help(mons))
                continue;
            sever += mons_threat_level(*mons) * 2 + 1;
    }

    if (!x_chance_in_y(sever, 150 + sever))
        return;

    xom_acts(sever);
}

static bool mon_nearby(function<bool(monster&)> filter)
{
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
        if (filter(**mi))
            return true;
    return false;
}

// Picks 100 random grids from the level and checks whether they've been
// marked as seen (explored) or known (mapped). If seen_only is true,
// grids only "seen" via magic mapping don't count. Returns the
// estimated percentage value of exploration.
static int _exploration_estimate(bool seen_only = false)
{
    int seen  = 0;
    int total = 0;
    int tries = 0;

    do
    {
        tries++;

        coord_def pos = random_in_bounds();
        if (!seen_only && env.map_knowledge(pos).known() || env.map_knowledge(pos).seen())
        {
            seen++;
            total++;
            continue;
        }

        bool open = true;
        if (cell_is_solid(pos) && !feat_is_closed_door(grd(pos)))
        {
            open = false;
            for (adjacent_iterator ai(pos); ai; ++ai)
            {
                if (map_bounds(*ai) && (!feat_is_opaque(grd(*ai))
                                        || feat_is_closed_door(grd(*ai))))
                {
                    open = true;
                    break;
                }
            }
        }

        if (open)
            total++;
    }
    while (total < 100 && tries < 1000);

    // If we didn't get any qualifying grids, there are probably so few
    // of them you've already seen them all.
    if (total == 0)
        return 100;

    if (total < 100)
        seen *= 100 / total;

    return seen;
}

static bool _transformation_check(const spell_type spell)
{
    transformation_type tran = TRAN_NONE;
    switch (spell)
    {
    case SPELL_BEASTLY_APPENDAGE:
        tran = TRAN_APPENDAGE;
        break;
    case SPELL_SPIDER_FORM:
        tran = TRAN_SPIDER;
        break;
    case SPELL_STATUE_FORM:
        tran = TRAN_STATUE;
        break;
    case SPELL_ICE_FORM:
        tran = TRAN_ICE_BEAST;
        break;
    case SPELL_HYDRA_FORM:
        tran = TRAN_HYDRA;
        break;
    case SPELL_DRAGON_FORM:
        tran = TRAN_DRAGON;
        break;
    case SPELL_NECROMUTATION:
        tran = TRAN_LICH;
        break;
    default:
        break;
    }

    if (tran == TRAN_NONE)
        return true;

    // Check whether existing enchantments/transformations, cursed
    // equipment or potential stat loss interfere with this
    // transformation.
    return transform(0, tran, true, true);
}

/// Try to choose a random player-castable spell.
static spell_type _choose_random_spell(int sever)
{
    const int spellenum = max(1, sever);
    vector<spell_type> ok_spells;
    const vector<spell_type> &spell_list = _xom_random_spells;
    for (int i = 0; i < min(spellenum, (int)spell_list.size()); ++i)
    {
        const spell_type spell = spell_list[i];
        if (!spell_is_useless(spell, true, true, true)
             && _transformation_check(spell))
        {
            ok_spells.push_back(spell);
        }
    }

    if (!ok_spells.size())
        return SPELL_NO_SPELL;
    return ok_spells[random2(ok_spells.size())];
}

/// Cast a random spell 'through' the player.
static void _xom_random_spell(int sever)
{
    const spell_type spell = _choose_random_spell(sever);
    if (spell == SPELL_NO_SPELL)
        return;

    god_speaks(GOD_XOM, _get_xom_speech("spell effect").c_str());

#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_RELIGION) || defined(DEBUG_XOM)
    mprf(MSGCH_DIAGNOSTICS,
         "_xom_makes_you_cast_random_spell(); spell: %d",
         spell);
#endif

    your_spells(spell, sever, false);
    const string note = make_stringf("cast spell '%s'", spell_title(spell));
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
}

/// Map out the level.
static void _xom_magic_mapping(int sever)
{
    god_speaks(GOD_XOM, _get_xom_speech("divination").c_str());

    // power isn't relevant at present, but may again be, someday?
    const int power = stepdown_value(sever, 10, 10, 40, 45);
    magic_mapping(5 + power, 50 + random2avg(power * 2, 2), false);
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1,
                   "divination: magic mapping"), true);
}

/// Detect items across the level.
static void _xom_detect_items(int sever)
{
    god_speaks(GOD_XOM, _get_xom_speech("divination").c_str());

    if (detect_items(sever) == 0)
        canned_msg(MSG_DETECT_NOTHING);
    else
        mpr("You detect items!");

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1,
                   "divination: detect items"), true);
}

/// Detect creatures across the level.
static void _xom_detect_creatures(int sever)
{
    god_speaks(GOD_XOM, _get_xom_speech("divination").c_str());

    const int prev_detected = count_detected_mons();
    const int num_creatures = detect_creatures(sever);

    if (num_creatures == 0)
        canned_msg(MSG_DETECT_NOTHING);
    else if (num_creatures == prev_detected)
    {
        // This is not strictly true. You could have cast Detect
        // Creatures with a big enough fuzz that the detected glyph is
        // still on the map when the original one has been killed. Then
        // another one is spawned, so the number is the same as before.
        // There's no way we can check this, however.
        mpr("You detect no further creatures.");
    }
    else
        mpr("You detect creatures!");

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1,
                   "divination: detect creatures"), true);
}

static void _try_brand_switch(const int item_index)
{
    if (item_index == NON_ITEM)
        return;

    item_def &item(mitm[item_index]);

    if (item.base_type != OBJ_WEAPONS)
        return;

    if (is_unrandom_artefact(item))
        return;

    // Only do it some of the time.
    if (one_chance_in(3))
        return;

    if (get_weapon_brand(item) == SPWPN_NORMAL)
        return;

    if (is_random_artefact(item))
        artefact_set_property(item, ARTP_BRAND, SPWPN_CHAOS);
    else
        item.brand = SPWPN_CHAOS;
}

static void _xom_make_item(object_class_type base, int subtype, int power)
{
    god_acting gdact(GOD_XOM);

    int thing_created = items(true, base, subtype, power, 0, GOD_XOM);

    if (thing_created == NON_ITEM)
    {
        god_speaks(GOD_XOM, "\"No, never mind.\"");
        return;
    }

    _try_brand_switch(thing_created);

    static char gift_buf[100];
    snprintf(gift_buf, sizeof(gift_buf), "god gift: %s",
             mitm[thing_created].name(DESC_PLAIN).c_str());
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, gift_buf), true);

    canned_msg(MSG_SOMETHING_APPEARS);
    move_item_to_grid(&thing_created, you.pos());

    if (thing_created == NON_ITEM) // if it fell into lava
        simple_god_message(" snickers.", GOD_XOM);

    stop_running();
}

/// Xom's 'acquirement'. A gift for the player
static void _xom_acquirement(int /*sever*/)
{
    god_speaks(GOD_XOM, _get_xom_speech("general gift").c_str());

    const object_class_type types[] =
    {
        OBJ_WEAPONS, OBJ_ARMOUR, OBJ_JEWELLERY,  OBJ_BOOKS,
        OBJ_STAVES,  OBJ_WANDS,  OBJ_MISCELLANY,
        OBJ_MISSILES
    };
    const object_class_type force_class = RANDOM_ELEMENT(types);

    int item_index = NON_ITEM;
    if (!acquirement(force_class, GOD_XOM, false, &item_index)
        || item_index == NON_ITEM)
    {
        god_speaks(GOD_XOM, "\"No, never mind.\"");
        return;
    }

    _try_brand_switch(item_index);

    const string note = make_stringf("god gift: %s",
                                     mitm[item_index].name(DESC_PLAIN).c_str());
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);

    stop_running();
    more();
}

/// Create a random item and give it to the player.
static void _xom_random_item(int sever)
{
    god_speaks(GOD_XOM, _get_xom_speech("general gift").c_str());
    _xom_make_item(OBJ_RANDOM, OBJ_RANDOM, sever * 3);
    more();
}

static bool _choose_mutatable_monster(const monster& mon)
{
    return mon.alive() && mon.can_safely_mutate()
           && !mon.submerged();
}

static bool _choose_enchantable_monster(const monster& mon)
{
    return mon.alive() && !mon.wont_attack()
           && !mons_immune_magic(mon);
}

static bool _is_chaos_upgradeable(const item_def &item,
                                  const monster* mon)
{
    // Since Xom is a god, he is capable of changing randarts, but not
    // other artefacts.
    if (is_unrandom_artefact(item))
        return false;

    // Staves can't be changed either, since they don't have brands in the way
    // other weapons do.
    if (item.base_type == OBJ_STAVES || item.base_type == OBJ_RODS)
    {
        return false;
    }

    // Only upgrade permanent items, since the player should get a
    // chance to use the item if he or she can defeat the monster.
    if (item.flags & ISFLAG_SUMMONED)
        return false;

    // Blessed weapons are protected, being gifts from good gods.
    if (is_blessed(item))
        return false;

    // God gifts are protected -- but not his own!
    if (item.orig_monnum < 0)
    {
        god_type iorig = static_cast<god_type>(-item.orig_monnum);
        if (iorig > GOD_NO_GOD && iorig < NUM_GODS && iorig != GOD_XOM)
            return false;
    }

    // Leave branded items alone, since this is supposed to be an
    // upgrade.
    if (item.base_type == OBJ_MISSILES)
    {
        // Don't make boulders of chaos.
        if (item.sub_type == MI_LARGE_ROCK)
        {
            return false;
        }

        if (get_ammo_brand(item) == SPMSL_NORMAL)
            return true;
    }
    else
    {
        // If the weapon is a launcher, and the monster is either out
        // of ammo or is carrying javelins, then don't bother upgrading
        // the launcher.
        if (is_range_weapon(item)
            && (mon->inv[MSLOT_MISSILE] == NON_ITEM
                || !has_launcher(mitm[mon->inv[MSLOT_MISSILE]])))
        {
            return false;
        }

        if (get_weapon_brand(item) == SPWPN_NORMAL)
            return true;
    }

    return false;
}

static bool _choose_chaos_upgrade(const monster& mon)
{
    // Only choose monsters that will attack.
    if (!mon.alive() || mons_attitude(mon) != ATT_HOSTILE
        || mons_is_fleeing(mon))
    {
        return false;
    }

    if (mons_itemuse(mon) < MONUSE_STARTING_EQUIPMENT)
        return false;

    // Holy beings are presumably protected by another god, unless
    // they're gifts from a chaotic god.
    if (mon.is_holy() && !is_chaotic_god(mon.god))
        return false;

    // God gifts from good gods will be protected by their god from
    // being given chaos weapons, while other gods won't mind the help
    // in their servants' killing the player.
    if (is_good_god(mon.god))
        return false;

    // Beogh presumably doesn't want Xom messing with his orcs, even if
    // it would give them a better weapon.
    if (mons_genus(mon.type) == MONS_ORC
        && (mon.is_priest() || coinflip()))
    {
        return false;
    }

    mon_inv_type slots[] = {MSLOT_WEAPON, MSLOT_ALT_WEAPON, MSLOT_MISSILE};

    // NOTE: Code assumes that the monster will only be carrying one
    // missile launcher at a time.
    bool special_launcher = false;
    for (int i = 0; i < 3; ++i)
    {
        const mon_inv_type slot = slots[i];
        const int          midx = mon.inv[slot];

        if (midx == NON_ITEM)
            continue;
        const item_def &item(mitm[midx]);

        // The monster already has a chaos weapon. Give the upgrade to
        // a different monster.
        if (is_chaotic_item(item))
            return false;

        if (_is_chaos_upgradeable(item, &mon))
        {
            if (item.base_type != OBJ_MISSILES)
                return true;

            // If, for some weird reason, a monster is carrying a bow
            // and javelins, then branding the javelins is okay, since
            // they won't be fired by the bow.
            if (!special_launcher || !has_launcher(item))
                return true;
        }

        if (is_range_weapon(item))
        {
            // If the launcher alters its ammo, then branding the
            // monster's ammo won't be an upgrade.
            int brand = get_weapon_brand(item);
            if (brand == SPWPN_FLAMING || brand == SPWPN_FREEZING
                || brand == SPWPN_VENOM)
            {
                special_launcher = true;
            }
        }
    }

    return false;
}

static void _do_chaos_upgrade(item_def &item, const monster* mon)
{
    ASSERT(item.base_type == OBJ_MISSILES
           || item.base_type == OBJ_WEAPONS);
    ASSERT(!is_unrandom_artefact(item));

    bool seen = false;
    if (mon && you.can_see(*mon) && item.base_type == OBJ_WEAPONS)
    {
        seen = true;

        description_level_type desc = mon->friendly() ? DESC_YOUR :
                                                        DESC_THE;
        string msg = apostrophise(mon->name(desc));

        msg += " ";

        msg += item.name(DESC_PLAIN, false, false, false);

        msg += " is briefly surrounded by a scintillating aura of "
               "random colours.";

        mpr(msg);
    }

    const int brand = (item.base_type == OBJ_WEAPONS) ? (int) SPWPN_CHAOS
                                                      : (int) SPMSL_CHAOS;

    if (is_random_artefact(item))
    {
        artefact_set_property(item, ARTP_BRAND, brand);

        if (seen)
            artefact_learn_prop(item, ARTP_BRAND);
    }
    else
    {
        item.brand = brand;

        if (seen)
            set_ident_flags(item, ISFLAG_KNOW_TYPE);

        // Make sure it's visibly special.
        if (!(item.flags & ISFLAG_COSMETIC_MASK))
            item.flags |= ISFLAG_GLOWING;

        // Make the pluses more like a randomly generated ego item.
        if (item.base_type == OBJ_WEAPONS)
            item.plus  += random2(5);
    }
}

static monster_type _xom_random_demon(int sever)
{
    const int roll = random2(1000 - (MAX_PIETY - sever) * 5);
#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "_xom_random_demon(); sever = %d, roll: %d",
         sever, roll);
#endif
    monster_type dct = (roll >= 340) ? RANDOM_DEMON_COMMON
                                     : RANDOM_DEMON_LESSER;

    monster_type demon = MONS_PROGRAM_BUG;

    if (dct == RANDOM_DEMON_COMMON && one_chance_in(10))
        demon = MONS_CHAOS_SPAWN;
    else
        demon = summon_any_demon(dct);

    return demon;
}

static bool _player_is_dead()
{
    return you.hp <= 0
        || is_feat_dangerous(grd(you.pos()))
        || you.did_escape_death();
}

static void _note_potion_effect(potion_type pot)
{
    string potion_name = potion_type_name(static_cast<int>(pot));

    string potion_msg = "potion effect ";

    potion_msg += ("(" + potion_name + ")");

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, potion_msg), true);
}


/// Feed the player a notionally-good potion effect.
static void _xom_do_potion(int /*sever*/)
{
    potion_type pot = POT_CURING;
    do
    {
        pot = random_choose_weighted(10, POT_HEAL_WOUNDS,
                                     10, POT_MAGIC,
                                     10, POT_HASTE,
                                     10, POT_MIGHT,
                                     10, POT_INVISIBILITY,
                                     10, POT_BERSERK_RAGE);
    }
    while (!get_potion_effect(pot)->can_quaff()); // ugh

    god_speaks(GOD_XOM, _get_xom_speech("potion effect").c_str());

    if (pot == POT_BERSERK_RAGE)
        you.berserk_penalty = NO_BERSERK_PENALTY;

    if (pot == POT_INVISIBILITY)
        you.attribute[ATTR_INVIS_UNCANCELLABLE] = 1;

    _note_potion_effect(pot);

    get_potion_effect(pot)->effect(true, 80);
}

static void _confuse_monster(monster* mons, int sever)
{
    if (mons->check_clarity(false))
        return;
    if (mons->holiness() & (MH_NONLIVING | MH_PLANT))
        return;

    const bool was_confused = mons->confused();
    if (mons->add_ench(mon_enchant(ENCH_CONFUSION, 0,
          &menv[ANON_FRIENDLY_MONSTER], random2(sever) * 10)))
    {
        if (was_confused)
            simple_monster_message(*mons, " looks more confused.");
        else
            simple_monster_message(*mons, " looks confused.");
    }
}

static void _xom_confuse_monsters(int sever)
{
    bool spoke = false;
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
    {
        if (mi->wont_attack() || one_chance_in(20))
            continue;

        // Only give this message once.
        if (!spoke)
            god_speaks(GOD_XOM, _get_xom_speech("confusion").c_str());
        spoke = true;

        _confuse_monster(*mi, sever);
    }

    if (spoke)
    {
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "confuse monster(s)"),
                  true);
    }
}

/// Post a passel of pals to the player.
static void _xom_send_allies(int sever)
{
    // The number of allies is dependent on severity, though heavily
    // randomised.
    int numdemons = sever;
    for (int i = 0; i < 3; i++)
        numdemons = random2(numdemons + 1);
    numdemons = min(numdemons + 2, 16);

    // Limit number of demons by experience level.
    const int maxdemons = (you.experience_level);
    if (numdemons > maxdemons)
        numdemons = maxdemons;

    int num_actually_summoned = 0;

    for (int i = 0; i < numdemons; ++i)
    {
        monster_type mon_type = _xom_random_demon(sever);

        mgen_data mg(mon_type, BEH_FRIENDLY, you.pos(), MHITYOU, MG_FORCE_BEH);
        mg.set_summoned(&you, 3, MON_SUMM_AID, GOD_XOM);

        // Even though the friendlies are charged to you for accounting,
        // they should still show as Xom's fault if one of them kills you.
        mg.non_actor_summoner = "Xom";

        if (create_monster(mg))
            num_actually_summoned++;
    }

    if (num_actually_summoned)
    {
        god_speaks(GOD_XOM, _get_xom_speech("multiple summons").c_str());

        const string note = make_stringf("summons %d friendly demon%s",
                                         num_actually_summoned,
                                         num_actually_summoned > 1 ? "s" : "");
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
    }
}

/// Send a single pal to the player's aid, hopefully.
static void _xom_send_one_ally(int sever)
{
    const monster_type mon_type = _xom_random_demon(sever);

    mgen_data mg(mon_type, BEH_FRIENDLY, you.pos(), MHITYOU, MG_FORCE_BEH);
    mg.set_summoned(&you, 6, MON_SUMM_AID, GOD_XOM);

    mg.non_actor_summoner = "Xom";

    if (monster *summons = create_monster(mg))
    {
        god_speaks(GOD_XOM, _get_xom_speech("single summon").c_str());

        const string note = make_stringf("summons friendly %s",
                                         summons->name(DESC_PLAIN).c_str());
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
    }
}

/**
 * Try to polymorph the given monster. If 'helpful', hostile monsters will
 * (try to) turn into weaker ones, and friendly monsters into stronger ones;
 * if (!helpful), the reverse is true.
 *
 * @param mons      The monster in question.
 * @param helpful   Whether to try to be helpful.
 */
static void _xom_polymorph_monster(monster &mons, bool helpful)
{
    god_speaks(GOD_XOM,
               helpful ? _get_xom_speech("good monster polymorph").c_str()
                       : _get_xom_speech("bad monster polymorph").c_str());

    const bool see_old = you.can_see(mons);
    const string old_name = see_old ? mons.full_name(DESC_PLAIN)
                                    : "something unseen";

    const bool powerup = !(mons.wont_attack() ^ helpful);
    monster_polymorph(&mons, RANDOM_MONSTER,
                      powerup ? PPT_MORE : PPT_LESS);

    const bool see_new = you.can_see(mons);

    if (see_old || see_new)
    {
        const string new_name = see_new ? mons.full_name(DESC_PLAIN)
                                        : "something unseen";

        string note = make_stringf("polymorph %s -> %s",
                                   old_name.c_str(), new_name.c_str());

#ifdef NOTE_DEBUG_XOM
        note += " (";
        note += (powerup ? "upgrade" : "downgrade");
        note += ")";
#endif
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
    }
}

/// Find a monster to poly.
static monster* _xom_mons_poly_target()
{
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
        if (_choose_mutatable_monster(**mi) && !mons_is_firewood(**mi))
            return *mi;
    return nullptr;
}

/// Try to polymporph a nearby monster into something weaker... or stronger.
static void _xom_polymorph_nearby_monster(bool helpful)
{
    monster* mon = _xom_mons_poly_target();
    if (mon)
        _xom_polymorph_monster(*mon, helpful);
}

/// Try to polymporph a nearby monster into something weaker.
static void _xom_good_polymorph(int /*sever*/)
{
    _xom_polymorph_nearby_monster(true);
}

/// Try to polymporph a nearby monster into something stronger.
static void _xom_bad_polymorph(int /*sever*/)
{
    _xom_polymorph_nearby_monster(false);
}

bool swap_monsters(monster* m1, monster* m2)
{
    monster& mon1(*m1);
    monster& mon2(*m2);

    const bool mon1_caught = mon1.caught();
    const bool mon2_caught = mon2.caught();

    mon1.swap_with(m2);

    if (mon1_caught && !mon2_caught)
    {
        check_net_will_hold_monster(&mon2);
        mon1.del_ench(ENCH_HELD, true);

    }
    else if (mon2_caught && !mon1_caught)
    {
        check_net_will_hold_monster(&mon1);
        mon2.del_ench(ENCH_HELD, true);
    }

    return true;
}

/// Which monsters, if any, can Xom currently swap with the player?
static vector<monster*> _rearrangeable_pieces()
{
    vector<monster* > mons;
    if (player_stair_delay() || monster_at(you.pos()))
        return mons;

    for (monster_near_iterator mi(&you, LOS_NO_TRANS); mi; ++mi)
    {
        if (!mons_is_tentacle_or_tentacle_segment(mi->type))
            mons.push_back(*mi);
    }
    return mons;
}

// Swap places with a random monster and, depending on severity, also
// between monsters. This can be pretty bad if there are a lot of
// hostile monsters around.
static void _xom_rearrange_pieces(int sever)
{
    vector<monster*> mons = _rearrangeable_pieces();
    if (mons.empty())
        return;

    god_speaks(GOD_XOM, _get_xom_speech("rearrange the pieces").c_str());

    const int num_mons = mons.size();

    // Swap places with a random monster.
    monster* mon = mons[random2(num_mons)];
    swap_with_monster(mon);

    // Sometimes confuse said monster.
    if (coinflip())
        _confuse_monster(mon, sever);

    if (num_mons > 1 && x_chance_in_y(sever, 70))
    {
        bool did_message = false;
        const int max_repeats = min(num_mons / 2, 8);
        const int repeats     = min(random2(sever / 10) + 1, max_repeats);
        for (int i = 0; i < repeats; ++i)
        {
            const int mon1 = random2(num_mons);
            int mon2 = mon1;
            while (mon1 == mon2)
                mon2 = random2(num_mons);

            if (swap_monsters(mons[mon1], mons[mon2]))
            {
                if (!did_message)
                {
                    mpr("Some monsters swap places.");
                    did_message = true;
                }
                if (one_chance_in(4))
                    _confuse_monster(mons[mon1], sever);
                if (one_chance_in(4))
                    _confuse_monster(mons[mon2], sever);
            }
        }
    }
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "swap monsters"), true);
}

/// Try to find a nearby hostile monster with an animateable weapon.
static monster* _find_monster_with_animateable_weapon()
{
    vector<monster* > mons_wpn;
    for (monster_near_iterator mi(&you, LOS_NO_TRANS); mi; ++mi)
    {
        if (mi->wont_attack() || mi->is_summoned()
            || mons_itemuse(**mi) < MONUSE_STARTING_EQUIPMENT
            || (mi->flags & MF_HARD_RESET))
        {
            continue;
        }

        const int mweap = mi->inv[MSLOT_WEAPON];
        if (mweap == NON_ITEM)
            continue;

        const item_def weapon = mitm[mweap];

        if (weapon.base_type == OBJ_WEAPONS
            && !(weapon.flags & ISFLAG_SUMMONED)
            && weapon.quantity == 1
            && !is_range_weapon(weapon)
            && !is_special_unrandom_artefact(weapon)
            && get_weapon_brand(weapon) != SPWPN_DISTORTION)
        {
            mons_wpn.push_back(*mi);
        }
    }
    if (mons_wpn.empty())
        return nullptr;
    return mons_wpn[random2(mons_wpn.size())];
}

static void _xom_animate_monster_weapon(int sever)
{
    // Pick a random monster...
    monster* mon = _find_monster_with_animateable_weapon();
    if (!mon)
        return;

    god_speaks(GOD_XOM, _get_xom_speech("animate monster weapon").c_str());

    // ...and get its weapon.
    const int wpn = mon->inv[MSLOT_WEAPON];
    ASSERT(wpn != NON_ITEM);

    const int dur = min(2 + (random2(sever) / 5), 6);

    mgen_data mg(MONS_DANCING_WEAPON, BEH_FRIENDLY, mon->pos(), mon->mindex());
    mg.set_summoned(&you, dur, SPELL_TUKIMAS_DANCE, GOD_XOM);

    mg.non_actor_summoner = "Xom";

    monster *dancing = create_monster(mg);

    if (!dancing)
        return;

    // Make the monster unwield its weapon.
    mon->unequip(*(mon->mslot_item(MSLOT_WEAPON)), false, true);
    mon->inv[MSLOT_WEAPON] = NON_ITEM;

    mprf("%s %s dances into the air!",
         apostrophise(mon->name(DESC_THE)).c_str(),
         mitm[wpn].name(DESC_PLAIN).c_str());

    destroy_item(dancing->inv[MSLOT_WEAPON]);

    dancing->inv[MSLOT_WEAPON] = wpn;
    mitm[wpn].set_holding_monster(*dancing);
    dancing->colour = mitm[wpn].get_colour();
}

static void _xom_shuffle_mutations(bool penance)
{
    if (!you.can_safely_mutate())
        return;

    god_speaks(GOD_XOM, _get_xom_speech("random mutations").c_str());

    delete_all_mutations("Xom's power");

    const int num_tries = 2 + random2avg(you.experience_level * 3 / 2 + 1, 2);

    const string note = make_stringf("give %smutation%s",
#ifdef NOTE_DEBUG_XOM
            "random ",
#else
             "",
#endif
             num_tries > 1 ? "s" : "");

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
    mpr("Your body is suffused with distortional energy.");

    bool failMsg = true;

    for (int i = num_tries; i > 0; --i)
    {
        if (!mutate(penance ? RANDOM_BAD_MUTATION : RANDOM_MUTATION,
                    penance ?  "Xom's mischief" : "Xom's grace",
                    failMsg, false, true, false, MUTCLASS_NORMAL))
        {
            failMsg = false;
        }
    }
}

/**
 * Have Xom throw divine lightning.
 */
static void _xom_throw_divine_lightning(int /*sever*/)
{
    god_speaks(GOD_XOM, "The area is suffused with divine lightning!");

    bolt beam;

    beam.flavour      = BEAM_ELECTRICITY;
    beam.glyph        = dchar_glyph(DCHAR_FIRED_BURST);
    beam.damage       = dice_def(3, 30);
    beam.target       = you.pos();
    beam.name         = "blast of lightning";
    beam.colour       = LIGHTCYAN;
    beam.thrower      = KILL_MISC;
    beam.source_id    = MID_NOBODY;
    beam.aux_source   = "Xom's lightning strike";
    beam.ex_size      = 2;
    beam.is_explosion = true;

    beam.explode(true, true);

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "divine lightning"), true);
}

/// Xom hurls fireballs at your foes!
static void _xom_destruction(int sever, bool real)
{
    bool rc = false;

    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
    {
        if (mons_is_projectile(**mi)
            || mons_is_tentacle_or_tentacle_segment(mi->type)
            || one_chance_in(3))
        {
            continue;
        }

        // Skip adjacent monsters, and skip non-hostile monsters if not feeling nasty.
        if (real
            && (adjacent(you.pos(), mi->pos())
                || mi->wont_attack() && !_xom_feels_nasty()))
        {
            continue;
        }

        if (!real)
        {
            if (!rc)
                god_speaks(GOD_XOM, _get_xom_speech("fake destruction").c_str());
            rc = true;
            backlight_monster(*mi);
            continue;
        }

        bolt beam;

        beam.flavour      = BEAM_FIRE;
        beam.glyph        = dchar_glyph(DCHAR_FIRED_BURST);
        beam.damage       = dice_def(2, 4 + sever / 10);
        beam.target       = mi->pos();
        beam.name         = "fireball";
        beam.colour       = RED;
        beam.thrower      = KILL_MISC;
        beam.source_id    = MID_NOBODY;
        beam.aux_source   = "Xom's destruction";
        beam.ex_size      = 1;
        beam.is_explosion = true;

        // Only give this message once.
        if (!rc)
            god_speaks(GOD_XOM, _get_xom_speech("destruction").c_str());
        rc = true;

        beam.explode();
    }

    if (rc)
    {
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1,
                       real ? "destruction" : "fake destruction"), true);
    }
}

static void _xom_real_destruction(int sever) { _xom_destruction(sever, true); }

static void _xom_enchant_monster(bool helpful)
{
    monster* mon = choose_random_nearby_monster(0, _choose_enchantable_monster);
    if (!mon)
        return;

    god_speaks(GOD_XOM,
               helpful ? _get_xom_speech("good enchant monster").c_str()
                       : _get_xom_speech("bad enchant monster").c_str());

    beam_type ench;

    if (helpful) // To the player, not the monster.
    {
        static const beam_type enchantments[] =
        {
            BEAM_PETRIFY,
            BEAM_SLOW,
            BEAM_PARALYSIS,
            BEAM_ENSLAVE,
        };
        ench = RANDOM_ELEMENT(enchantments);
    }
    else
    {
        static const beam_type enchantments[] =
        {
            BEAM_HASTE,
            BEAM_MIGHT,
            BEAM_AGILITY,
            BEAM_RESISTANCE,
        };
        ench = RANDOM_ELEMENT(enchantments);
    }

    enchant_actor_with_flavour(mon, 0, ench);

    // Take a note.
    const string note = make_stringf("enchant monster %s",
                                     helpful ? "(good)" : "(bad)");
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
}

static void _xom_good_enchant_monster(int /*sever*/) {
    _xom_enchant_monster(true);
}
static void _xom_bad_enchant_monster(int /*sever*/) {
    _xom_enchant_monster(false);
}

/// Toss some fog around the player. Helping...?
static void _xom_fog(int /*sever*/)
{
    big_cloud(CLOUD_RANDOM_SMOKE, &you, you.pos(), 50, 8 + random2(8));
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "fog"), true);
    god_speaks(GOD_XOM, _get_xom_speech("cloud").c_str());
}

static void _get_hand_type(string &hand, bool &can_plural)
{
    hand       = "";
    can_plural = true;

    vector<string> hand_vec;
    vector<bool>   plural_vec;
    bool           plural;

    hand_vec.push_back(you.hand_name(false, &plural));
    plural_vec.push_back(plural);

    if (you.species != SP_NAGA || form_changed_physiology())
    {
        if (item_def* item = you.slot_item(EQ_BOOTS))
        {
            hand_vec.emplace_back(item->name(DESC_BASENAME, false, false, false));
            plural = false; // "pair of boots" is singular
        }
        else
            hand_vec.push_back(you.foot_name(false, &plural));
        plural_vec.push_back(plural);
    }

    if (you.form == TRAN_SPIDER)
    {
        hand_vec.emplace_back("mandible");
        plural_vec.push_back(true);
    }
    else if (you.species != SP_OCTOPODE && !you.get_mutation_level(MUT_BEAK)
          || form_changed_physiology())
    {
        hand_vec.emplace_back("nose");
        plural_vec.push_back(false);
    }

    if (you.form == TRAN_BEHOLDER || you.species != SP_OCTOPODE
           && !form_changed_physiology())
    {
        hand_vec.emplace_back("ear");
        plural_vec.push_back(true);
    }

    if (!form_changed_physiology() && you.species != SP_OCTOPODE)
    {
        hand_vec.emplace_back("elbow");
        plural_vec.push_back(true);
    }

    ASSERT(hand_vec.size() == plural_vec.size());
    ASSERT(!hand_vec.empty());

    const unsigned int choice = random2(hand_vec.size());

    hand       = hand_vec[choice];
    can_plural = plural_vec[choice];
}

static void _xom_miscast(const int max_level)
{
    ASSERT_RANGE(max_level, 0, 4);

    const char* speeches[4] =
    {
        XOM_SPEECH("zero miscast effect"),
        XOM_SPEECH("minor miscast effect"),
        XOM_SPEECH("medium miscast effect"),
        XOM_SPEECH("major miscast effect"),
    };

    const char* causes[4] =
    {
        "the mischief of Xom",
        "the capriciousness of Xom",
        "the capriciousness of Xom",
        "the severe capriciousness of Xom"
    };

    const char* speech_str = speeches[max_level];
    const char* cause_str  = causes[max_level];

    const int level = (1 + random2(max_level));

    // Take a note.
    const char* levels[4] = { "harmless", "mild", "medium", "severe" };
    const auto school = spschools_type::exponent(random2(SPTYP_LAST_EXPONENT + 1));
    string desc = make_stringf("%s %s miscast", levels[level],
                               spelltype_short_name(school));
#ifdef NOTE_DEBUG_XOM
    if (nasty)
        desc += " (Xom was nasty)";
#endif
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, desc), true);

    string hand_str;
    bool   can_plural;

    _get_hand_type(hand_str, can_plural);

    // If Xom's not being nasty, then prevent spell miscasts from
    // killing the player.
    const int lethality_margin  = random_range(1, 4);

    god_speaks(GOD_XOM, _get_xom_speech(speech_str).c_str());

    MiscastEffect(&you, nullptr, GOD_MISCAST + GOD_XOM,
                  (spschool_flag_type)school, level, cause_str, NH_DEFAULT,
                  lethality_margin, hand_str, can_plural);
}

static void _xom_minor_miscast(int sever)
{
    _xom_miscast(1);
}
static void _xom_major_miscast(int sever)
{
    _xom_miscast(2);
}
static void _xom_critical_miscast(int sever)
{
    _xom_miscast(3);
}

static void _xom_chaos_upgrade(int /*sever*/)
{
    monster* mon = choose_random_nearby_monster(0, _choose_chaos_upgrade);

    if (!mon)
        return;

    god_speaks(GOD_XOM, _get_xom_speech("chaos upgrade").c_str());

    mon_inv_type slots[] = {MSLOT_WEAPON, MSLOT_ALT_WEAPON, MSLOT_MISSILE};

    bool rc = false;
    for (int i = 0; i < 3 && !rc; ++i)
    {
        item_def* const item = mon->mslot_item(slots[i]);
        if (item && _is_chaos_upgradeable(*item, mon))
        {
            _do_chaos_upgrade(*item, mon);
            rc = true;
        }
    }
    ASSERT(rc);

    // Wake the monster up.
    behaviour_event(mon, ME_ALERT, &you);

    if (rc)
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "chaos upgrade"), true);
}

static void _xom_player_confusion_effect(int sever)
{
    const bool conf = you.confused();

    if (!confuse_player(5 + random2(3), true))
        return;

    god_speaks(GOD_XOM, _get_xom_speech("confusion").c_str());
    mprf(MSGCH_WARN, "You are %sconfused.",
         conf ? "more " : "");

    // At higher severities, Xom is less likely to confuse surrounding
    // creatures.
    bool mons_too = false;
    if (random2(sever) < 30)
    {
        for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
        {
            if (random2(sever) > 30)
                continue;
            _confuse_monster(*mi, sever);
            mons_too = true;
        }
    }

    // Take a note.
    string conf_msg = "confusion";
    if (mons_too)
        conf_msg += " (+ monsters)";
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, conf_msg), true);
}

static bool _valid_floor_grid(coord_def pos)
{
    if (!in_bounds(pos))
        return false;

    return grd(pos) == DNGN_FLOOR;
}

bool move_stair(coord_def stair_pos, bool away, bool allow_under)
{
    if (!allow_under)
        ASSERT(stair_pos != you.pos());

    dungeon_feature_type feat = grd(stair_pos);
    ASSERT(feat_stair_direction(feat) != CMD_NO_CMD);

    coord_def begin, towards;

    bool stairs_moved = false;
    if (away)
    {
        // If the staircase starts out under the player, first shove it
        // onto a neighbouring grid.
        if (allow_under && stair_pos == you.pos())
        {
            coord_def new_pos(stair_pos);
            // Loop twice through all adjacent grids. In the first round,
            // only consider grids whose next neighbour in the direction
            // away from the player is also of type floor. If we didn't
            // find any matching grid, try again without that restriction.
            for (int tries = 0; tries < 2; ++tries)
            {
                int adj_count = 0;
                for (adjacent_iterator ai(stair_pos); ai; ++ai)
                    if (grd(*ai) == DNGN_FLOOR
                        && (tries || _valid_floor_grid(*ai + *ai - stair_pos))
                        && one_chance_in(++adj_count))
                    {
                        new_pos = *ai;
                    }

                if (!tries && new_pos != stair_pos)
                    break;
            }

            if (new_pos == stair_pos)
                return false;

            if (!slide_feature_over(stair_pos, new_pos, true))
                return false;

            stair_pos = new_pos;
            stairs_moved = true;
        }

        begin   = you.pos();
        towards = stair_pos;
    }
    else
    {
        // Can't move towards player if it's already adjacent.
        if (adjacent(you.pos(), stair_pos))
            return false;

        begin   = stair_pos;
        towards = you.pos();
    }

    ray_def ray;
    if (!find_ray(begin, towards, ray, opc_solid_see))
    {
        mprf(MSGCH_ERROR, "Couldn't find ray between player and stairs.");
        return stairs_moved;
    }

    // Don't start off under the player.
    if (away)
        ray.advance();

    bool found_stairs = false;
    int  past_stairs  = 0;
    while (in_bounds(ray.pos()) && you.see_cell(ray.pos())
           && !cell_is_solid(ray.pos()) && ray.pos() != you.pos())
    {
        if (ray.pos() == stair_pos)
            found_stairs = true;
        if (found_stairs)
            past_stairs++;
        ray.advance();
    }
    past_stairs--;

    if (!away && cell_is_solid(ray.pos()))
    {
        // Transparent wall between stair and player.
        return stairs_moved;
    }

    if (away && !found_stairs)
    {
        if (cell_is_solid(ray.pos()))
        {
            // Transparent wall between stair and player.
            return stairs_moved;
        }

        mprf(MSGCH_ERROR, "Ray didn't cross stairs.");
    }

    if (away && past_stairs <= 0)
    {
        // Stairs already at edge, can't move further away.
        return stairs_moved;
    }

    if (!in_bounds(ray.pos()) || ray.pos() == you.pos())
        ray.regress();

    while (!you.see_cell(ray.pos()) || grd(ray.pos()) != DNGN_FLOOR)
    {
        ray.regress();
        if (!in_bounds(ray.pos()) || ray.pos() == you.pos()
            || ray.pos() == stair_pos)
        {
            // No squares in path are a plain floor.
            return stairs_moved;
        }
    }

    ASSERT(stair_pos != ray.pos());

    string stair_str = feature_description_at(stair_pos, false, DESC_THE, false);

    mprf("%s slides %s you!", stair_str.c_str(),
         away ? "away from" : "towards");

    // Animate stair moving.
    const feature_def &feat_def = get_feature_def(feat);

    bolt beam;

    beam.range   = INFINITE_DISTANCE;
    beam.flavour = BEAM_VISUAL;
    beam.glyph   = feat_def.symbol();
    beam.colour  = feat_def.colour();
    beam.source  = stair_pos;
    beam.target  = ray.pos();
    beam.name    = "STAIR BEAM";
    beam.draw_delay = 50; // Make beam animation slower than normal.

    beam.aimed_at_spot = true;
    beam.fire();

    // Clear out "missile trails"
    viewwindow();

    if (!swap_features(stair_pos, ray.pos(), false, false))
    {
        mprf(MSGCH_ERROR, "_move_stair(): failed to move %s",
             stair_str.c_str());
        return stairs_moved;
    }

    return true;
}

static void _xom_cloud_trail(int /*sever*/)
{
    you.duration[DUR_CLOUD_TRAIL] = random_range(300, 600);
    you.props[XOM_CLOUD_TRAIL_TYPE_KEY] =
        // 80% chance of a useful trail
        random_choose_weighted(20, CLOUD_CHAOS,
                               10, CLOUD_MAGIC_TRAIL,
                               5,  CLOUD_MIASMA,
                               5,  CLOUD_PETRIFY,
                               5,  CLOUD_MUTAGENIC,
                               5,  CLOUD_NEGATIVE_ENERGY);

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "cloud trail"), true);

    const string speech = _get_xom_speech("cloud trail");
    god_speaks(GOD_XOM, speech.c_str());
}

static void _xom_statloss(int /*sever*/)
{
    const string speech = _get_xom_speech("draining or torment");

    const stat_type stat = static_cast<stat_type>(random2(NUM_STATS));
    int loss = 1 + random2(3);
    if (you.stat(stat) <= loss)
        return;

    god_speaks(GOD_XOM, speech.c_str());
    lose_stat(stat, loss);

    const char* sstr[3] = { "Str", "Int", "Dex" };
    const string note = make_stringf("stat loss: -%d %s (%d/%d)",
                                     loss, sstr[stat], you.stat(stat),
                                     you.max_stat(stat));

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
}

static void _xom_draining(int /*sever*/)
{
    const string speech = _get_xom_speech("draining or torment");
    god_speaks(GOD_XOM, speech.c_str());

    drain_player(100, true);

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "draining"), true);
}

static void _xom_torment(int /*sever*/)
{
    const string speech = _get_xom_speech("draining or torment");
    god_speaks(GOD_XOM, speech.c_str());

    torment_player(0, TORMENT_XOM);

    const string note = make_stringf("torment (%d/%d hp)", you.hp, you.hp_max);
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
}

static monster* _xom_summon_hostile(monster_type hostile)
{
    return create_monster(mgen_data::hostile_at(hostile, true, you.pos())
                          .set_summoned(nullptr, 4, MON_SUMM_WRATH, GOD_XOM)
                          .set_non_actor_summoner("Xom"));
}

static void _xom_summon_hostiles(int sever)
{
    int num_summoned = 0;
    const bool shadow_creatures = one_chance_in(3);

    if (shadow_creatures)
    {
        // Small number of shadow creatures.
        int count = 1 + random2(4);
        for (int i = 0; i < count; ++i)
            if (_xom_summon_hostile(RANDOM_MOBILE_MONSTER))
                num_summoned++;
    }
    else
    {
        // The number of demons is dependent on severity, though heavily
        // randomised.
        int numdemons = sever;
        for (int i = 0; i < 3; ++i)
            numdemons = random2(numdemons + 1);
        numdemons = min(numdemons + 1, 14);

        // Limit number of demons by experience level.
        if (!you.penance[GOD_XOM])
        {
            const int maxdemons = (you.experience_level / 2);
            if (numdemons > maxdemons)
                numdemons = maxdemons;
        }

        for (int i = 0; i < numdemons; ++i)
            if (_xom_summon_hostile(_xom_random_demon(sever)))
                num_summoned++;
    }

    if (num_summoned > 0)
    {
        const string note = make_stringf("summons %d hostile %s%s",
                                         num_summoned,
                                         shadow_creatures ? "shadow creature"
                                                          : "demon",
                                         num_summoned > 1 ? "s" : "");
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);

        const string speech = _get_xom_speech("hostile monster");
        god_speaks(GOD_XOM, speech.c_str());
    }
}

static void _xom_noise(int /*sever*/)
{
    // Ranges from shout to shatter volume.
    const int noisiness = 12 + random2(19);

    god_speaks(GOD_XOM, _get_xom_speech("noise").c_str());
    // Xom isn't subject to silence.
    fake_noisy(noisiness, you.pos());

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "noise"), true);
}

static bool _mon_valid_blink_victim(const monster& mon)
{
    return !mon.wont_attack()
            && !mon.no_tele()
            && !mons_is_projectile(mon);
}

static void _xom_blink_monsters(int /*sever*/)
{
    int blinks = 0;
    // Sometimes blink towards the player, sometimes randomly. It might
    // end up being helpful instead of dangerous, but Xom doesn't mind.
    const bool blink_to_player = _xom_feels_nasty() || coinflip();
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
    {
        if (blinks >= 5)
            break;

        if (!_mon_valid_blink_victim(**mi) || coinflip())
            continue;

        // Only give this message once.
        if (!blinks)
            god_speaks(GOD_XOM, _get_xom_speech("blink monsters").c_str());

        if (blink_to_player)
            blink_other_close(*mi, you.pos());
        else
            monster_blink(*mi, false);

        blinks++;
    }

    if (blinks)
    {
        take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "blink monster(s)"),
                  true);
    }
}

static void _xom_cleaving(int sever)
{
    god_speaks(GOD_XOM, _get_xom_speech("cleaving").c_str());

    you.increase_duration(DUR_CLEAVE, 5 + random2(sever));

    if (const item_def* const weapon = you.weapon())
    {
        const bool axe = item_attack_skill(*weapon) == SK_AXES;
        mprf(MSGCH_DURATION,
             "%s %s sharp%s", weapon->name(DESC_YOUR).c_str(),
             conjugate_verb("look", weapon->quantity > 1).c_str(),
             (axe) ? " (like it always does)." : ".");
    }
    else
    {
        mprf(MSGCH_DURATION, "%s",
             you.hands_act("look", "sharp.").c_str());
    }

    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "cleaving"), true);
}


static void _handle_accidental_death(const int orig_hp,
    const FixedVector<uint8_t, NUM_MUTATIONS> &orig_mutation,
    const transformation_type orig_form)
{
    // Did ouch() return early because the player died from the Xom
    // effect, even though neither is the player under penance nor is
    // Xom bored?
    if (!you.did_escape_death()
        && you.escaped_death_aux.empty()
        && !_player_is_dead())
    {
        // The player is fine.
        return;
    }

    string speech_type = XOM_SPEECH("accidental homicide");

    const dungeon_feature_type feat = grd(you.pos());

    switch (you.escaped_death_cause)
    {
        case NUM_KILLBY:
        case KILLED_BY_LEAVING:
        case KILLED_BY_WINNING:
        case KILLED_BY_QUITTING:
            speech_type = XOM_SPEECH("weird death");
            break;

        case KILLED_BY_LAVA:
        case KILLED_BY_WATER:
            if (!is_feat_dangerous(feat))
                speech_type = "weird death";
            break;

        default:
            if (is_feat_dangerous(feat))
                speech_type = "weird death";
        break;
    }

    canned_msg(MSG_YOU_DIE);
    god_speaks(GOD_XOM, _get_xom_speech(speech_type).c_str());
    god_speaks(GOD_XOM, _get_xom_speech("resurrection").c_str());

    int pre_mut_hp = you.hp;
    if (you.hp <= 0)
        you.hp = 9999; // avoid spurious recursive deaths if heavily rotten

    // If any mutation has changed, death was because of it.
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        if (orig_mutation[i] > you.mutation[i])
            mutate((mutation_type)i, "Xom's lifesaving", true, true, true);
        else if (orig_mutation[i] < you.mutation[i])
            delete_mutation((mutation_type)i, "Xom's lifesaving", true, true, true);
    }

    if (pre_mut_hp <= 0)
        set_hp(min(orig_hp, you.hp_max));

    if (orig_form != you.form)
    {
        dprf("Trying emergency untransformation.");
        you.transform_uncancellable = false;
        transform(10, orig_form, true);
    }

    if (is_feat_dangerous(feat) && !crawl_state.game_is_sprint())
        you_teleport_now();
}

/**
 * Try to choose an action for Xom to take
 * @param sever         The intended magnitude of the action.
 * @return              An action for Xom to take, e.g. XOM_BAD_NOISE.
 */
static xom_event_type _xom_choose_random_action(int sever)
{
    int destruction_chance = 0;
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
    {
        if (mons_is_projectile(**mi)
            || mons_is_tentacle_or_tentacle_segment(mi->type))
        {
            continue;
        }

        // Skip adjacent monsters and non-hostile monsters
        if (!adjacent(you.pos(), mi->pos()) && !mi->wont_attack())
        {
            destruction_chance+= 5;
        }
    }

    int lightning_chance = 0;
    if (player_in_a_dangerous_place())
    {
        // Make sure there's at least one enemy within the lightning radius.
        for (radius_iterator ri(you.pos(), 2, C_SQUARE, LOS_SOLID, true); ri;
             ++ri)
        {
            const monster *mon = monster_at(*ri);
            if (mon && !mon->wont_attack())
                lightning_chance++;
        }
    }

    xom_event_type action = random_choose_weighted(
        200, XOM_GOOD_POTION,
        _choose_random_spell(sever) != SPELL_NO_SPELL ? 100 : 0, XOM_GOOD_SPELL,
        !you.get_mutation_level(MUT_NO_LOVE) ? 100 : 0, XOM_GOOD_SINGLE_ALLY,
        _find_monster_with_animateable_weapon()
        && !you.get_mutation_level(MUT_NO_LOVE) ? 50 : 0, XOM_GOOD_ANIMATE_MON_WPN,
        !you.duration[DUR_CLOUD_TRAIL] ? 50 : 0, XOM_GOOD_CLOUD_TRAIL,
        mon_nearby(_choose_enchantable_monster) ? 50 + sever / 4 : 0, XOM_GOOD_ENCHANT_MONSTER,
        mon_nearby([](monster& mon){ return !mon.wont_attack(); }) ? 30 : 0, XOM_GOOD_CONFUSION,
        !you.get_mutation_level(MUT_NO_LOVE) ? 30 : 0, XOM_GOOD_ALLIES,
        _xom_mons_poly_target() != nullptr ? 30 : 0, XOM_GOOD_POLYMORPH,
        destruction_chance, XOM_GOOD_DESTRUCTION,
        !cloud_at(you.pos()) ? 20 + sever / 4 : 0, XOM_GOOD_FOG,
        lightning_chance * sever / 5, XOM_GOOD_LIGHTNING,
        20, XOM_GOOD_CLEAVING,
        100 - sever, XOM_BAD_NOISE,
        100, XOM_BAD_ENCHANT_MONSTER,
        mon_nearby(_mon_valid_blink_victim) ? 100 : 0, XOM_BAD_BLINK_MONSTERS,
        _rearrangeable_pieces().size() ? 50 : 0, XOM_BAD_SWAP_MONSTERS,
        30 + sever / 5, XOM_BAD_POLYMORPH,
        20, XOM_BAD_CONFUSION,
        30 + sever / 5, XOM_BAD_SUMMON_HOSTILES,
        10, XOM_BAD_MISCAST_MAJOR,
        10, XOM_BAD_DRAINING,
        10, XOM_BAD_STATLOSS,
        10, XOM_BAD_TORMENT,
        5,  XOM_BAD_CHAOS_UPGRADE,
        5,  XOM_BAD_CHAOS_CLOUD,
        1 + sever / 5, XOM_BAD_MISCAST_CRITICAL);

    return action;
}

/**
 * Try to choose an action for Xom to take.
 *
 * @param niceness      Whether the action should be 'good' for the player.
 * @param sever         The intended magnitude of the action.
 * @param tension       How much danger we think the player's currently in.
 * @return              An bad action for Xom to take, e.g. XOM_DID_NOTHING.
 */
xom_event_type xom_choose_action(int sever)
{
    sever = max(1, sever);

    if (_player_is_dead())
    {
        // This should only happen if the player used wizard mode to
        // escape death from deep water or lava.
        ASSERT(you.wizard);
        ASSERT(!you.did_escape_death());
        if (is_feat_dangerous(grd(you.pos())))
            mprf(MSGCH_DIAGNOSTICS, "Player is standing in deadly terrain, skipping Xom act.");
        else
            mprf(MSGCH_DIAGNOSTICS, "Player is already dead, skipping Xom act.");
        return XOM_PLAYER_DEAD;
    }

    // Bad mojo. (this loop, that is)
    while (true)
    {
        const xom_event_type action = _xom_choose_random_action(sever);
        if (action != XOM_DID_NOTHING)
            return action;
    }

    die("This should never happen.");
}

/**
 * Execute the specified Xom Action.
 *
 * @param action        The action type in question; e.g. XOM_BAD_NOISE.
 * @param sever         The severity of the action.
 */
void xom_take_action(xom_event_type action, int sever)
{
    const int  orig_hp       = you.hp;
    const transformation_type orig_form = you.form;
    const FixedVector<uint8_t, NUM_MUTATIONS> orig_mutation = you.mutation;
    const bool was_bored = _xom_is_bored();

    const bool bad_effect = _action_is_bad(action);

    if (was_bored && bad_effect && Options.note_xom_effects)
        take_note(Note(NOTE_MESSAGE, 0, 0, "XOM is BORED!"), true);

    // actually take the action!
    {
        god_acting gdact(GOD_XOM);
        _do_xom_event(action, sever);
    }

    // If we got here because Xom was bored, reset gift timeout according
    // to the badness of the effect.
    if (bad_effect && _xom_is_bored())
    {
        const int badness = _xom_event_badness(action);
        const int interest = random2avg(badness * 60, 2);
        you.gift_timeout   = min(interest, 255);
        //updating piety status line
        you.redraw_title = true;
#if defined(DEBUG_RELIGION) || defined(DEBUG_XOM)
        mprf(MSGCH_DIAGNOSTICS, "badness: %d, new interest: %d",
             badness, you.gift_timeout);
#endif
    }

    _handle_accidental_death(orig_hp, orig_mutation, orig_form);

    if (you_worship(GOD_XOM) && one_chance_in(5))
    {
        const string old_xom_favour = describe_xom_favour();
        you.piety = random2(MAX_PIETY + 1);
        you.redraw_title = true; // redraw piety/boredom display
        const string new_xom_favour = describe_xom_favour();
        if (was_bored || old_xom_favour != new_xom_favour)
        {
            const string msg = "You are now " + new_xom_favour;
            god_speaks(you.religion, msg.c_str());
        }
#ifdef NOTE_DEBUG_XOM
        const string note = string("reroll piety: ") + you.piety;
        take_note(Note(NOTE_MESSAGE, 0, 0, note), true);
#endif
    }
    else if (was_bored)
    {
        // If we didn't reroll at least mention the new favour
        // now that it's not "BORING thing" anymore.
        const string new_xom_favour = describe_xom_favour();
        const string msg = "You are now " + new_xom_favour;
        god_speaks(you.religion, msg.c_str());
    }
}

/**
 * Let Xom take an action, probably.
 *
 * @param sever         The intended magnitude of the action.
 * @param nice          Whether the action should be 'good' for the player.
 *                      If MB_MAYBE, determined by xom's whim.
 *                      May be overridden.
 * @param tension       How much danger we think the player's currently in.
 * @return              Whichever action Xom took, or XOM_DID_NOTHING.
 */
xom_event_type xom_acts(int sever, bool debug)
{
#if defined(DEBUG_RELIGION) || defined(DEBUG_XOM)
    if (!debug)
    {
        // This probably seems a bit odd, but we really don't want to display
        // these when doing a heavy-duty wiz-mode debug test: just ends up
        // as message spam and the player doesn't get any further information
        // anyway. (jpeg)

        // these numbers (sever, tension) may be modified later...
        mprf(MSGCH_DIAGNOSTICS, "xom_acts(%d)", sever);

        static char xom_buf[100];
        snprintf(xom_buf, sizeof(xom_buf), "xom_acts(%d)", sever);
        take_note(Note(NOTE_MESSAGE, 0, 0, xom_buf), true);
    }
#endif

    const xom_event_type action = xom_choose_action(sever);
    if (!debug)
        xom_take_action(action, sever);

    return action;
}

static bool _death_is_funny(const kill_method_type killed_by)
{
    switch (killed_by)
    {
    // The less original deaths are considered boring.
    case KILLED_BY_MONSTER:
    case KILLED_BY_BEAM:
    case KILLED_BY_CLOUD:
    case KILLED_BY_FREEZING:
    case KILLED_BY_BURNING:
    case KILLED_BY_SELF_AIMED:
    case KILLED_BY_SOMETHING:
    case KILLED_BY_TRAP:
        return false;
    default:
        // All others are fun (says Xom).
        return true;
    }
}

void xom_death_message(const kill_method_type killed_by)
{
    if (!you_worship(GOD_XOM) && (!you.worshipped[GOD_XOM] || coinflip()))
        return;

    const int death_tension = get_tension(GOD_XOM);

    // "Normal" deaths with only down to -2 hp and comparatively low tension
    // are considered particularly boring.
    if (!_death_is_funny(killed_by) && you.hp >= -1 * random2(3)
        && death_tension <= random2(10))
    {
        god_speaks(GOD_XOM, _get_xom_speech("boring death").c_str());
    }
    // Unusual methods of dying, really low hp, or high tension make
    // for funny deaths.
    else if (_death_is_funny(killed_by) || you.hp <= -10
             || death_tension >= 20)
    {
        god_speaks(GOD_XOM, _get_xom_speech("laughter").c_str());
    }

    // All others just get ignored by Xom.
}

/**
 * The Xom teleportation train takes you on instant
 * teleportation to a few random areas, stopping if either
 * an area is dangerous to you or randomly.
 */
static void _xom_bad_teleport(int sever)
{
    god_speaks(GOD_XOM,
               _get_xom_speech("teleportation journey").c_str());

    int count = 0;
    do
    {
        you_teleport_now();
        search_around();
        more();
        if (count++ >= 7 + random2(5))
            break;
    }
    while (x_chance_in_y(3, 4) && !player_in_a_dangerous_place());
    maybe_update_stashes();

    // Take a note.
    const string note = make_stringf("%d-stop teleportation journey%s", count,
#ifdef NOTE_DEBUG_XOM
             badness == 3 ? " (dangerous)" : "");
#else
    "");
#endif
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, note), true);
}

/// Place a one-tile chaos cloud on the player, with minor spreading.
static void _xom_chaos_cloud(int /*sever*/)
{
    check_place_cloud(CLOUD_CHAOS, you.pos(), 3 + random2(12)*3,
                      nullptr, random_range(5,15));
    take_note(Note(NOTE_XOM_EFFECT, you.piety, -1, "chaos cloud"),
              true);
    god_speaks(GOD_XOM, _get_xom_speech("cloud").c_str());
}

struct xom_effect_count
{
    string effect;
    int    count;

    xom_effect_count(string e, int c) : effect(e), count(c) {};
};

/// A type of action Xom can take.
struct xom_event
{
    /// Wizmode name for the event.
    const char* name;
    /// The event action.
    function<void(int sever)> action;
    /**
     * Rough estimate of how hard a Xom effect hits the player,
     * scaled between 10 (harmless) and 50 (disastrous). Reduces boredom.
     */
    int badness_10x;
};

//have Xom do something when you reach a new level, sometimes
void xom_new_level_effect()
{
    if (!you_worship(GOD_XOM))
        return;
    if (one_chance_in(8))
    {
        switch (random2(3))
        {
            case 0:
                _xom_detect_creatures(20 + random2(100));
                break;
            case 1:
                _xom_detect_items(20 + random2(100));
                break;
            case 2:
                _xom_magic_mapping(20 + random2(100));
                break;
            default:
                break;
        }
        you.attribute[ATTR_XOM_GIFT_XP] +=
        2 * (exp_needed(you.experience_level + 1)
                - exp_needed(you.experience_level)) / (1 + random2(4));
    }
    else if (one_chance_in(10))
    {
        give_xom_gift(50);
    }
}

void give_xom_gift(int acq_chance)
{
    if (!you_worship(GOD_XOM))
        return;
    if (x_chance_in_y(acq_chance, 100))
        _xom_acquirement(5 + random2(you.experience_level* 7));
    else
        _xom_random_item(5 + random2(you.experience_level* 7));
    you.attribute[ATTR_XOM_GIFT_XP] +=
        2 * (exp_needed(you.experience_level + 1)
                - exp_needed(you.experience_level)) / (1 + random2(min(you.experience_level, 4)));
}

bool xom_wants_to_help(monster* mon)
{
    return !mon->friendly()
            && you.see_cell(mon->pos())
            && !mons_is_firewood(*mon)
            && !mon->submerged()
            && !mons_is_projectile(mon->type);
}

//delete all the player's mutations and give them a bunch of new ones
//set the timeout on this effect to somewhat over 1 level's worth of exp
void xom_mutate_player(bool penance)
{
    _xom_shuffle_mutations(penance);
    if (penance)
        return;
    you.attribute[ATTR_XOM_MUT_XP] +=
        ((10 + random2(10)) * (exp_needed(you.experience_level + 1)
                - exp_needed(you.experience_level))) / 4;
}

static const map<xom_event_type, xom_event> xom_events = {
    { XOM_DID_NOTHING, { "nothing" }},
    { XOM_GOOD_POTION, { "potion", _xom_do_potion }},
    { XOM_GOOD_MAGIC_MAPPING, { "nothing" }},
    { XOM_GOOD_DETECT_CREATURES, { "nothing" }},
    { XOM_GOOD_DETECT_ITEMS, { "nothing" }},
    { XOM_GOOD_SPELL, { "tension spell", _xom_random_spell }},
    { XOM_GOOD_CONFUSION, { "confuse monsters", _xom_confuse_monsters }},
    { XOM_GOOD_SINGLE_ALLY, { "single ally", _xom_send_one_ally }},
    { XOM_GOOD_ANIMATE_MON_WPN, { "animate monster weapon",
                                  _xom_animate_monster_weapon }},
    { XOM_GOOD_RANDOM_ITEM, { "nothing" }},
    { XOM_GOOD_ACQUIREMENT, { "nothing" }},
    { XOM_GOOD_ALLIES, { "summon allies", _xom_send_allies }},
    { XOM_GOOD_POLYMORPH, { "good polymorph", _xom_good_polymorph }},
    { XOM_GOOD_TELEPORT, { "nothing" }},
    { XOM_GOOD_MUTATION, { "nothing" }},
    { XOM_GOOD_LIGHTNING, { "lightning", _xom_throw_divine_lightning }},
    { XOM_GOOD_SCENERY, { "nothing" }},
    { XOM_GOOD_SNAKES, { "nothing" }},
    { XOM_GOOD_DESTRUCTION, { "mass fireball", _xom_real_destruction }},
    { XOM_GOOD_FAKE_DESTRUCTION, { "nothing", }},
    { XOM_GOOD_ENCHANT_MONSTER, { "good enchant monster",
                                  _xom_good_enchant_monster }},
    { XOM_GOOD_FOG, { "fog", _xom_fog }},
    { XOM_GOOD_CLOUD_TRAIL, { "cloud trail", _xom_cloud_trail }},
    { XOM_GOOD_CLEAVING, { "cleaving", _xom_cleaving }},

    { XOM_BAD_MISCAST_PSEUDO, { "nothing" }},
    { XOM_BAD_MISCAST_HARMLESS, { "nothing" }},
    { XOM_BAD_MISCAST_MINOR, { "minor miscast", _xom_minor_miscast, 10}},
    { XOM_BAD_MISCAST_MAJOR, { "major miscast", _xom_major_miscast, 20}},
    { XOM_BAD_MISCAST_CRITICAL, { "critical miscast",
                                    _xom_critical_miscast, 45}},
    { XOM_BAD_NOISE, { "noise", _xom_noise, 10 }},
    { XOM_BAD_ENCHANT_MONSTER, { "bad enchant monster",
                                 _xom_bad_enchant_monster, 10}},
    { XOM_BAD_BLINK_MONSTERS, { "blink monsters", _xom_blink_monsters, 10}},
    { XOM_BAD_CONFUSION, { "confuse player", _xom_player_confusion_effect, 13}},
    { XOM_BAD_SWAP_MONSTERS, { "swap monsters", _xom_rearrange_pieces, 20 }},
    { XOM_BAD_CHAOS_UPGRADE, { "chaos upgrade", _xom_chaos_upgrade, 20}},
    { XOM_BAD_TELEPORT, { "bad teleportation", _xom_bad_teleport, -1}},
    { XOM_BAD_POLYMORPH, { "bad polymorph", _xom_bad_polymorph, 30}},
    { XOM_BAD_MOVING_STAIRS, { "nothing" }},
    { XOM_BAD_CLIMB_STAIRS, { "nothing" }},
    { XOM_BAD_MUTATION, { "nothing" }},
    { XOM_BAD_SUMMON_HOSTILES, { "summon hostiles", _xom_summon_hostiles, 35}},
    { XOM_BAD_STATLOSS, { "statloss", _xom_statloss, 23}},
    { XOM_BAD_DRAINING, { "draining", _xom_draining, 23}},
    { XOM_BAD_TORMENT, { "torment", _xom_torment, 23}},
    { XOM_BAD_CHAOS_CLOUD, { "chaos cloud", _xom_chaos_cloud, 20}},
    { XOM_BAD_BANISHMENT, { "nothing" }},
    { XOM_BAD_PSEUDO_BANISHMENT, {"nothing" }},
};

static void _do_xom_event(xom_event_type event_type, int sever)
{
    const xom_event *event = map_find(xom_events, event_type);
    if (event && event->action)
        event->action(sever);
}

static int _xom_event_badness(xom_event_type event_type)
{
    if (event_type == XOM_BAD_TELEPORT)
        return player_in_a_dangerous_place() ? 3 : 1;

    const xom_event *event = map_find(xom_events, event_type);
    if (event)
        return div_rand_round(event->badness_10x, 10);
    return 0;
}

string xom_effect_to_name(xom_event_type effect)
{
    const xom_event *event = map_find(xom_events, effect);
    return event ? event->name : "bugginess";
}

/// Basic sanity checks on xom_events.
void validate_xom_events()
{
    string fails;
    set<string> action_names;

    for (int i = 0; i < XOM_LAST_REAL_ACT; ++i)
    {
        const xom_event_type event_type = static_cast<xom_event_type>(i);
        const xom_event *event = map_find(xom_events, event_type);
        if (!event)
        {
            fails += make_stringf("Xom event %d has no associated data!\n", i);
            continue;
        }

        if (action_names.count(event->name))
            fails += make_stringf("Duplicate name '%s'!\n", event->name);
        action_names.insert(event->name);

        if (_action_is_bad(event_type))
        {
            if ((event->badness_10x < 10 || event->badness_10x > 50)
                && event->badness_10x != -1) // implies it's special-cased
            {
                fails += make_stringf("'%s' badness %d outside 10-50 range.\n",
                                      event->name, event->badness_10x);
            }
        } else if (event->badness_10x)
        {
            fails += make_stringf("'%s' is not bad, but has badness!\n",
                                  event->name);
        }

        if (event_type != XOM_DID_NOTHING && !event->action)
            fails += make_stringf("No action for '%s'!\n", event->name);
    }

    dump_test_fails(fails, "xom-data");
}

#ifdef WIZARD
static bool _sort_xom_effects(const xom_effect_count &a,
                              const xom_effect_count &b)
{
    if (a.count == b.count)
        return a.effect < b.effect;

    return a.count > b.count;
}

static string _list_exploration_estimate()
{
    int explored = 0;
    int mapped   = 0;
    for (int k = 0; k < 10; ++k)
    {
        mapped   += _exploration_estimate(false);
        explored += _exploration_estimate(true);
    }
    mapped /= 10;
    explored /= 10;

    return make_stringf("mapping estimate: %d%%\nexploration estimate: %d%%\n",
                        mapped, explored);
}

// Loops over the entire piety spectrum and calls xom_acts() multiple
// times for each value, then prints the results into a file.
// TODO: Allow specification of niceness, tension, and boredness.
void debug_xom_effects()
{
    // Repeat N times.
    const int N = prompt_for_int("How many iterations over the "
                                 "entire piety range? ", true);

    if (N == 0)
    {
        canned_msg(MSG_OK);
        return;
    }

    FILE *ostat = fopen("xom_debug.stat", "w");
    if (!ostat)
    {
        mprf(MSGCH_ERROR, "Can't write 'xom_debug.stat'. Aborting.");
        return;
    }

    const int real_piety    = you.piety;
    const god_type real_god = you.religion;
    you.religion            = GOD_XOM;
    const int tension       = get_tension(GOD_XOM);

    fprintf(ostat, "---- STARTING XOM DEBUG TESTING ----\n");
    fprintf(ostat, "%s\n", dump_overview_screen(false).c_str());
    fprintf(ostat, "%s\n", screenshot().c_str());
    fprintf(ostat, "%s\n", _list_exploration_estimate().c_str());
    fprintf(ostat, "%s\n", mpr_monster_list().c_str());
    fprintf(ostat, " --> Tension: %d\n", tension);

    if (player_under_penance(GOD_XOM))
        fprintf(ostat, "You are under Xom's penance!\n");
    else if (_xom_is_bored())
        fprintf(ostat, "Xom is BORED.\n");
    fprintf(ostat, "\nRunning %d times through entire mood cycle.\n", N);
    fprintf(ostat, "---- OUTPUT EFFECT PERCENTAGES ----\n");

    vector<xom_event_type>          mood_effects;
    vector<vector<xom_event_type>>  all_effects;
    vector<string>                  moods;
    vector<int>                     mood_good_acts;

    string old_mood = "";
    string     mood = "";

    // Add an empty list to later add all effects to.
    all_effects.push_back(mood_effects);
    moods.emplace_back("total");
    mood_good_acts.push_back(0); // count total good acts

    int mood_good = 0;
    for (int p = 0; p <= MAX_PIETY; ++p)
    {
        you.piety     = p;
        int sever     = abs(p - HALF_MAX_PIETY);
        mood          = describe_xom_mood();
        if (old_mood != mood)
        {
            if (old_mood != "")
            {
                all_effects.push_back(mood_effects);
                mood_effects.clear();
                mood_good_acts.push_back(mood_good);
                mood_good_acts[0] += mood_good;
                mood_good = 0;
            }
            moods.push_back(mood);
            old_mood = mood;
        }

        // Repeat N times.
        for (int i = 0; i < N; ++i)
        {
            const xom_event_type result = xom_acts(sever, true);

            mood_effects.push_back(result);
            all_effects[0].push_back(result);

            if (result <= XOM_LAST_GOOD_ACT)
                mood_good++;
        }
    }
    all_effects.push_back(mood_effects);
    mood_effects.clear();
    mood_good_acts.push_back(mood_good);
    mood_good_acts[0] += mood_good;

    const int num_moods = moods.size();
    vector<xom_effect_count> xom_ec_pairs;
    for (int i = 0; i < num_moods; ++i)
    {
        mood_effects    = all_effects[i];
        const int total = mood_effects.size();

        if (i == 0)
            fprintf(ostat, "\nTotal effects (all piety ranges)\n");
        else
            fprintf(ostat, "\nMood: You are %s\n", moods[i].c_str());

        fprintf(ostat, "GOOD%7.2f%%\n",
                (100.0 * (float) mood_good_acts[i] / (float) total));
        fprintf(ostat, "BAD %7.2f%%\n",
                (100.0 * (float) (total - mood_good_acts[i]) / (float) total));

        sort(mood_effects.begin(), mood_effects.end());

        xom_ec_pairs.clear();
        xom_event_type old_effect = XOM_DID_NOTHING;
        int count      = 0;
        for (int k = 0; k < total; ++k)
        {
            if (mood_effects[k] != old_effect)
            {
                if (count > 0)
                {
                    xom_ec_pairs.emplace_back(xom_effect_to_name(old_effect),
                                              count);
                }
                old_effect = mood_effects[k];
                count = 1;
            }
            else
                count++;
        }

        if (count > 0)
            xom_ec_pairs.emplace_back(xom_effect_to_name(old_effect), count);

        sort(xom_ec_pairs.begin(), xom_ec_pairs.end(), _sort_xom_effects);
        for (const xom_effect_count &xec : xom_ec_pairs)
        {
            fprintf(ostat, "%7.2f%%    %s\n",
                    (100.0 * xec.count / total),
                    xec.effect.c_str());
        }
    }
    fprintf(ostat, "---- FINISHED XOM DEBUG TESTING ----\n");
    fclose(ostat);
    mpr("Results written into 'xom_debug.stat'.");

    you.piety    = real_piety;
    you.religion = real_god;
}
#endif // WIZARD
