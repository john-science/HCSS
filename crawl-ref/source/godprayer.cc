#include "AppHdr.h"

#include "godprayer.h"

#include <cmath>

#include "artefact.h"
#include "butcher.h"
#include "coordit.h"
#include "database.h"
#include "describe-god.h"
#include "english.h"
#include "env.h"
#include "food.h"
#include "fprop.h"
#include "godabil.h"
#include "goditem.h"
#include "godpassive.h"
#include "hiscores.h"
#include "invent.h"
#include "itemprop.h"
#include "items.h"
#include "item_use.h"
#include "makeitem.h"
#include "message.h"
#include "notes.h"
#include "prompt.h"
#include "religion.h"
#include "shopping.h"
#include "spl-goditem.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "terrain.h"
#include "unwind.h"
#include "view.h"

string god_prayer_reaction()
{
    string result = uppercase_first(god_name(you.religion));
    const int rank = god_favour_rank(you.religion);
    if (crawl_state.player_is_dead())
        result += " was ";
    else
        result += " is ";
    result +=
        (rank == 7) ? "exalted by your worship" :
        (rank == 6) ? "extremely pleased with you" :
        (rank == 5) ? "greatly pleased with you" :
        (rank == 4) ? "most pleased with you" :
        (rank == 3) ? "pleased with you" :
        (rank == 2) ? "aware of your devotion"
                    : "noncommittal";
    result += ".";

    return result;
}

/**
 * Determine the god this game's ecumenical altar is for.
 * Replaces the ecumenical altar with the God's real altar.
 * Assumes you can worship at least one god (ie are not a
 * demigod), and that you're standing on the altar.
 *
 * @return The god this altar is for.
 */
static god_type _altar_identify_ecumenical_altar()
{
    god_type god;
    do
    {
        god = random_god();
    }
    while (!player_can_join_god(god));
    dungeon_terrain_changed(you.pos(), altar_for_god(god));
    return god;
}

static bool _pray_ecumenical_altar()
{
    if (yesno("You cannot tell which god this altar belongs to. Convert to "
              "them anyway?", false, 'n'))
    {
        {
            // Don't check for or charge a Gozag service fee.
            unwind_var<int> fakepoor(you.attribute[ATTR_GOLD_GENERATED], 0);

            god_type altar_god = _altar_identify_ecumenical_altar();
            mprf(MSGCH_GOD, "%s accepts your prayer!",
                            god_name(altar_god).c_str());
            you.turn_is_over = true;
            if (!you_worship(altar_god))
                join_religion(altar_god);
            else
                return true;
        }

        if (you_worship(GOD_RU))
            you.props[RU_SACRIFICE_PROGRESS_KEY] = 9999;
        else
            gain_piety(20, 1, false);

        mark_milestone("god.ecumenical", "prayed at an ecumenical altar.");
        return true;
    }
    else
    {
        canned_msg(MSG_OK);
        return false;
    }
}

/**
 * Attempt to convert to the given god.
 *
 * @return True if the conversion happened, false otherwise.
 */
static bool _try_god_conversion(god_type god, bool beogh_priest)
{
    ASSERT(god != GOD_NO_GOD);

    if (god == GOD_ECUMENICAL)
        return _pray_ecumenical_altar();

    if (you_worship(GOD_NO_GOD) || god != you.religion)
    {
        // consider conversion
        you.turn_is_over = true;
        // But if we don't convert then god_pitch
        // makes it not take a turn after all.
        god_pitch(god);
        if (you.turn_is_over && you_worship(GOD_BEOGH) && beogh_priest)
            spare_beogh_convert();
        return you.turn_is_over;
    }
    return false;
}

/**
 * Zazen.
 */
static void _zen_meditation()
{
    const mon_holy_type holi = you.holiness();
    mprf(MSGCH_PRAY,
         "You spend a moment contemplating the meaning of %s.",
         holi & MH_NONLIVING ? "existence" : holi & MH_UNDEAD ? "unlife" : "life");
}

/**
 * Pray. (To your god, or the god of the altar you're at, or to Beogh, if
 * you're an orc being preached at.)
 */
void pray(bool allow_conversion)
{
    const god_type altar_god = feat_altar_god(grd(you.pos()));
    const bool beogh_priest = env.level_state & LSTATE_BEOGH
        && can_convert_to_beogh();
    const god_type target_god = beogh_priest ? GOD_BEOGH : altar_god;

    // only successful prayer takes time
    you.turn_is_over = false;
    // Try to pray to an altar or beogh (if either is possible)
    if (allow_conversion
        && target_god != GOD_NO_GOD
        && you.religion != target_god)
    {
        _try_god_conversion(target_god, beogh_priest);
        return;
    }

    ASSERT(!you.turn_is_over);
    // didn't convert to anyone.
    if (you_worship(GOD_NO_GOD))
    {
        // wasn't considering following a god; just meditating.
        if (altar_god == GOD_NO_GOD)
            _zen_meditation();
        return;
    }

    mprf(MSGCH_PRAY, "You offer a %sprayer to %s.",
         you.cannot_speak() ? "silent " : "",
         god_name(you.religion).c_str());

    if (you_worship(GOD_XOM))
        mprf(MSGCH_GOD, "%s", getSpeakString("Xom prayer").c_str());
    else if (you_worship(GOD_GOZAG))
        mprf(MSGCH_GOD, "%s", getSpeakString("Gozag prayer").c_str());
    else if (player_under_penance())
        simple_god_message(" demands penance!");
    else
        mprf(MSGCH_PRAY, you.religion, "%s", god_prayer_reaction().c_str());

    dprf("piety: %d (-%d)", you.piety, you.piety_hysteresis);
}

int zin_tithe(const item_def& item, int quant, bool quiet, bool converting)
{
    int taken = 0;
    int due = quant += you.attribute[ATTR_TITHE_BASE];
    if (due > 0)
    {
        int tithe = due / 10;
        due -= tithe * 10;
        // Those high enough in the hierarchy get to reap the benefits.
        // You're never big enough to be paid, the top is not having to pay
        // (and even that at 200 piety, for a brief moment until it decays).
        tithe = min(tithe,
                    (you.penance[GOD_ZIN] + MAX_PIETY - you.piety) * 2 / 3);
        if (tithe <= 0)
        {
            // update the remainder anyway
            you.attribute[ATTR_TITHE_BASE] = due;
            return 0;
        }
        taken = tithe;
        you.attribute[ATTR_DONATIONS] += tithe;
        mprf("You pay a tithe of %d gold.", tithe);

        if (item.plus == 1) // seen before worshipping Zin
        {
            tithe = 0;
            simple_god_message(" ignores your late donation.");
        }
        // A single scroll can give you more than D:1-18, Lair and Orc
        // together, limit the gains. You're still required to pay from
        // your sudden fortune, yet it's considered your duty to the Church
        // so piety is capped. If you want more piety, donate more!
        //
        // Note that the stepdown is not applied to other gains: it would
        // be simpler, yet when a monster combines a number of gold piles
        // you shouldn't be penalized.
        int denom = 2;
        if (item.props.exists(ACQUIRE_KEY)) // including "acquire any" in vaults
        {
            tithe = stepdown_value(tithe, 10, 10, 50, 50);
            dprf("Gold was acquired, reducing gains to %d.", tithe);
        }
        else
        {
            if (player_in_branch(BRANCH_ORC) && !converting)
            {
                // Another special case: Orc gives simply too much compared to
                // other branches.
                denom *= 2;
            }
            // Avg gold pile value: 10 + depth/2.
            tithe *= 47;
            denom *= 20 + env.absdepth0;
        }
        gain_piety(tithe * 3, denom);
    }
    you.attribute[ATTR_TITHE_BASE] = due;
    return taken;
}

enum jiyva_slurp_results
{
    JS_NONE = 0,
    JS_FOOD = 1 << 0,
    JS_HP   = 1 << 1,
    JS_MP   = 1 << 2,
};

struct slurp_gain
{
    int jiyva_bonus;
    piety_gain_t piety_gain;

    slurp_gain(int bonus, piety_gain_t gain)
        : jiyva_bonus(bonus), piety_gain(gain)
    {
    }
};
