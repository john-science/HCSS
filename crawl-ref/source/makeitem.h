/**
 * @file
 * @brief Item creation routines.
**/

#ifndef MAKEITEM_H
#define MAKEITEM_H

#include "itemprop-enum.h"

static const int NO_AGENT = -1;

int create_item_named(string name, coord_def pos, string *error);

int items(bool allow_uniques, object_class_type force_class, int force_type,
          int item_level, int force_ego = 0, int agent = NO_AGENT);

void item_colour(item_def &item);

jewellery_type get_random_ring_type();
jewellery_type get_random_amulet_type();
void item_set_appearance(item_def &item);

bool is_weapon_brand_ok(int type, int brand, bool strict);
bool is_armour_brand_ok(int type, int brand, bool strict);
bool is_missile_brand_ok(int type, int brand, bool strict);

void reroll_brand(item_def &item, int item_level);

bool is_high_tier_wand(int type);

void squash_plusses(int item_slot);
#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_TESTS)
void makeitem_tests();
#endif
#endif
