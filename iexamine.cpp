//
//  iexamine.cpp
//  Cataclysm
//
//  Livingstone
//

#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include <sstream>

void iexamine::none	(game *g, player *p, map *m, int examx, int examy) {
 g->add_msg("That is a %s.", m->tername(examx, examy).c_str());
};

void iexamine::gaspump(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn("Use the %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 item gas(g->itypes[itm_gasoline], g->turn);
 if (one_in(10 + p->dex_cur)) {
  g->add_msg("You accidentally spill the gasoline.");
  m->add_item(p->posx, p->posy, gas);
 } else {
  p->moves -= 300;
  g->handle_liquid(gas, false, true);
 }
 if (one_in(50)) {
  g->add_msg("With a clang and a shudder, the gas pump goes silent.");
  m->ter(examx, examy) = t_gas_pump_empty;
 }
}

void iexamine::elevator(game *g, player *p, map *m, int examx, int examy){
 if (!query_yn("Use the %s?",m->tername(examx, examy).c_str())) return;
 int movez = (g->levz < 0 ? 2 : -2);
 g->levz += movez;
 m->load(g, g->levx, g->levy, g->levz);
 g->update_map(p->posx, p->posy);
 for (int x = 0; x < SEEX * MAPSIZE; x++) {
  for (int y = 0; y < SEEY * MAPSIZE; y++) {
   if (m->ter(x, y) == t_elevator) {
    p->posx = x;
    p->posy = y;
   }
  }
 }
 g->refresh_all();
}

void iexamine::controls_gate(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn("Use the %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 g->open_gate(g,examx,examy, m->ter(examx,examy));
}

void iexamine::cardreader(game *g, player *p, map *m, int examx, int examy) {
 itype_id card_type = (m->ter(examx, examy) == t_card_science ? itm_id_science :
                       itm_id_military);
 if (p->has_amount(card_type, 1) && query_yn("Swipe your ID card?")) {
  p->moves -= 100;
  for (int i = -3; i <= 3; i++) {
   for (int j = -3; j <= 3; j++) {
    if (m->ter(examx + i, examy + j) == t_door_metal_locked)
     m->ter(examx + i, examy + j) = t_floor;
     }
  }
  for (int i = 0; i < g->z.size(); i++) {
   if (g->z[i].type->id == mon_turret) {
    g->z.erase(g->z.begin() + i);
    i--;
   }
  }
  g->add_msg("You insert your ID card.");
  g->add_msg("The nearby doors slide into the floor.");
  p->use_amount(card_type, 1);
 } else {
  bool using_electrohack = (p->has_amount(itm_electrohack, 1) &&
                            query_yn("Use electrohack on the reader?"));
  bool using_fingerhack = (!using_electrohack && p->has_bionic(bio_fingerhack) &&
                           p->power_level > 0 &&
                           query_yn("Use fingerhack on the reader?"));
  if (using_electrohack || using_fingerhack) {
   p->moves -= 500;
   p->practice("computer", 20);
   int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
   success += rng(-3, 3);
   if (using_fingerhack)
    success++;
   if (p->int_cur < 8)
    success -= rng(0, int((8 - p->int_cur) / 2));
    else if (p->int_cur > 8)
     success += rng(0, int((p->int_cur - 8) / 2));
     if (success < 0) {
      g->add_msg("You cause a short circuit!");
      if (success <= -5) {
       if (using_electrohack) {
        g->add_msg("Your electrohack is ruined!");
        p->use_amount(itm_electrohack, 1);
       } else {
        g->add_msg("Your power is drained!");
        p->charge_power(0 - rng(0, p->power_level));
       }
      }
      m->ter(examx, examy) = t_card_reader_broken;
     } else if (success < 6)
      g->add_msg("Nothing happens.");
      else {
       g->add_msg("You activate the panel!");
       g->add_msg("The nearby doors slide into the floor.");
       m->ter(examx, examy) = t_card_reader_broken;
       for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
         if (m->ter(examx + i, examy + j) == t_door_metal_locked)
          m->ter(examx + i, examy + j) = t_floor;
          }
       }
      }
  } else {
   g->add_msg("Looks like you need a %s.",g->itypes[card_type]->name.c_str());
  }
 }
}

void iexamine::rubble(game *g, player *p, map *m, int examx, int examy) {
 if (!(p->has_amount(itm_shovel, 1) || p->has_amount(itm_primitive_shovel, 1))) {
  g->add_msg("If only you had a shovel...");
  return;
 }
 const char *xname = m->tername(examx, examy).c_str();
 if (query_yn("Clear up that %s?",xname)) {
   // "Remove"
  p->moves -= 200;

   // "Replace"
  if(m->ter(examx,examy) == t_rubble) {
   item rock(g->itypes[itm_rock], g->turn);
   m->add_item(p->posx, p->posy, rock);
   m->add_item(p->posx, p->posy, rock);
  }

   // "Refloor"
  if (g->levz < 0) {
   m->ter(examx, examy) = t_rock_floor;
  } else {
   m->ter(examx, examy) = t_dirt;
  }

   // "Remind"
  g->add_msg("You clear up that %s.", xname);
 }
}

void iexamine::chainfence(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn("Climb %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 400;
 if (one_in(p->dex_cur)) {
  g->add_msg("You slip whilst climbing and fall down again");
 } else {
  p->moves += p->dex_cur * 10;
  p->posx = examx;
  p->posy = examy;
 }
}

void iexamine::tent(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn("Take down %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->ter(examx + i, examy + j) = t_dirt;
 g->add_msg("You take down the tent");
 item tent(g->itypes[itm_tent_kit], g->turn);
 m->add_item(examx, examy, tent);
}

void iexamine::shelter(game *g, player *p, map *m, int examx, int examy) {
 if (!query_yn("Take down %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->ter(examx + i, examy + j) = t_dirt;
 g->add_msg("You take down the shelter");
 item tent(g->itypes[itm_shelter_kit], g->turn);
 m->add_item(examx, examy, tent);
}

void iexamine::wreckage(game *g, player *p, map *m, int examx, int examy) {
 if (!(p->has_amount(itm_shovel, 1) || p->has_amount(itm_primitive_shovel, 1))) {
  g->add_msg("If only you had a shovel..");
  return;
 }

 if (query_yn("Clear up that wreckage?")) {
  p->moves -= 200;
  m->ter(examx, examy) = t_dirt;
  item chunk(g->itypes[itm_steel_chunk], g->turn);
  item scrap(g->itypes[itm_scrap], g->turn);
  item pipe(g->itypes[itm_pipe], g->turn);
  item wire(g->itypes[itm_wire], g->turn);
  m->add_item(examx, examy, chunk);
  m->add_item(examx, examy, scrap);
  if (one_in(5)) {
   m->add_item(examx, examy, pipe);
   m->add_item(examx, examy, wire); }
  g->add_msg("You clear the wreckage up");
 }
}

void iexamine::pit(game *g, player *p, map *m, int examx, int examy) {
 if(!p->has_amount(itm_2x4, 1)) {
  none(g, p, m, examx, examy);
  return;
 }
 if (query_yn("Place a plank over the pit?")) {
  p->use_amount(itm_2x4, 1);
  m->ter(examx, examy) = t_pit_covered;
  g->add_msg("You place a plank of wood over the pit");
 } else {
  g->add_msg("You need a plank of wood to do that");
 }
}

void iexamine::pit_spiked(game *g, player *p, map *m, int examx, int examy) {
 if(!p->has_amount(itm_2x4, 1)) {
  none(g, p, m, examx, examy);
  return;
 }
 if (query_yn("Place a plank over the pit?")) {
  p->use_amount(itm_2x4, 1);
  m->ter(examx, examy) = t_pit_spiked_covered;
  g->add_msg("You place a plank of wood over the pit");
 } else {
  g->add_msg("You need a plank of wood to do that");
 }
}

void iexamine::pit_covered(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Remove cover?")) {
  none(g, p, m, examx, examy);
  return;
 }
 item plank(g->itypes[itm_2x4], g->turn);
 g->add_msg("You remove the plank.");
 m->add_item(p->posx, p->posy, plank);
 m->ter(examx, examy) = t_pit;
}

void iexamine::pit_spiked_covered(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Remove cover?")) {
  none(g, p, m, examx, examy);
  return;
 }

 item plank(g->itypes[itm_2x4], g->turn);
 g->add_msg("You remove the plank.");
 m->add_item(p->posx, p->posy, plank);
 m->ter(examx, examy) = t_pit_spiked;
}

void iexamine::fence_post(game *g, player *p, map *m, int examx, int examy) {

 int ch = menu("Fence Construction:", "Rope Fence", "Wire Fence",
               "Barbed Wire Fence", "Cancel", NULL);
 switch (ch){
  case 1:{
   if (p->has_amount(itm_rope_6, 2)) {
    p->use_amount(itm_rope_6, 2);
    m->ter(examx, examy) = t_fence_rope;
    p->moves -= 200;
   } else
    g->add_msg("You need 2 six-foot lengths of rope to do that");
  } break;

  case 2:{
   if (p->has_amount(itm_wire, 2)) {
    p->use_amount(itm_wire, 2);
    m->ter(examx, examy) = t_fence_wire;
    p->moves -= 200;
   } else
    g->add_msg("You need 2 lengths of wire to do that!");
  } break;

  case 3:{
   if (p->has_amount(itm_wire_barbed, 2)) {
    p->use_amount(itm_wire_barbed, 2);
    m->ter(examx, examy) = t_fence_barbed;
    p->moves -= 200;
   } else
    g->add_msg("You need 2 lengths of barbed wire to do that!");
  } break;

  case 4:
  default:
   break;
 }
}

void iexamine::remove_fence_rope(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Remove %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }
 item rope(g->itypes[itm_rope_6], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter(examx, examy) = t_fence_post;
 p->moves -= 200;

}

void iexamine::remove_fence_wire(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Remove %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 item rope(g->itypes[itm_wire], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter(examx, examy) = t_fence_post;
 p->moves -= 200;
}

void iexamine::remove_fence_barbed(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Remove %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 item rope(g->itypes[itm_wire_barbed], g->turn);
 m->add_item(p->posx, p->posy, rope);
 m->add_item(p->posx, p->posy, rope);
 m->ter(examx, examy) = t_fence_post;
 p->moves -= 200;
}

void iexamine::slot_machine(game *g, player *p, map *m, int examx, int examy) {
 if (p->cash < 10)
  g->add_msg("You need $10 to play.");
 else if (query_yn("Insert $10?")) {
  do {
   if (one_in(5))
    popup("Three cherries... you get your money back!");
   else if (one_in(20)) {
    popup("Three bells... you win $50!");
    p->cash += 40;	// Minus the $10 we wagered
   } else if (one_in(50)) {
    popup("Three stars... you win $200!");
    p->cash += 190;
   } else if (one_in(1000)) {
    popup("JACKPOT!  You win $5000!");
    p->cash += 4990;
   } else {
    popup("No win.");
    p->cash -= 10;
   }
  } while (p->cash >= 10 && query_yn("Play again?"));
 }
}

void iexamine::bulletin_board(game *g, player *p, map *m, int examx, int examy) {
 basecamp *camp = m->camp_at(examx, examy);
 if (camp && camp->board_x() == examx && camp->board_y() == examy) {
  std::vector<std::string> options;
  options.push_back("Cancel");
  int choice = menu_vec(camp->board_name().c_str(), options) - 1;
 }
 else {
  bool create_camp = m->allow_camp(examx, examy);
  std::vector<std::string> options;
  if (create_camp)
   options.push_back("Create camp");
  options.push_back("Cancel");
 		// TODO: Other Bulletin Boards
  int choice = menu_vec("Bulletin Board", options) - 1;
  if (choice >= 0 && choice < options.size()) {
   if (options[choice] == "Create camp") {
  			// TODO: Allow text entry for name
    m->add_camp("Home", examx, examy);
   }
  }
 }
}

void iexamine::fault(game *g, player *p, map *m, int examx, int examy) {
 popup("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs.");
}

void iexamine::pedestal_wyrm(game *g, player *p, map *m, int examx, int examy) {
 if (!m->i_at(examx, examy).empty()) {
  none(g, p, m, examx, examy);
  return;
 }
 g->add_msg("The pedestal sinks into the ground...");
 m->ter(examx, examy) = t_rock_floor;
 g->add_event(EVENT_SPAWN_WYRMS, int(g->turn) + rng(5, 10));
}

void iexamine::pedestal_temple(game *g, player *p, map *m, int examx, int examy) {

 if (m->i_at(examx, examy).size() == 1 &&
     m->i_at(examx, examy)[0].type->id == itm_petrified_eye) {
  g->add_msg("The pedestal sinks into the ground...");
  m->ter(examx, examy) = t_dirt;
  m->i_at(examx, examy).clear();
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else if (p->has_amount(itm_petrified_eye, 1) &&
            query_yn("Place your petrified eye on the pedestal?")) {
  p->use_amount(itm_petrified_eye, 1);
  g->add_msg("The pedestal sinks into the ground...");
  m->ter(examx, examy) = t_dirt;
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else
  g->add_msg("This pedestal is engraved in eye-shaped diagrams, and has a large\
semi-spherical indentation at the top.");
}

void iexamine::fswitch(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Flip the %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

  p->moves -= 100;
  for (int y = examy; y <= examy + 5; y++) {
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    switch (m->ter(examx, examy)) {
     case t_switch_rg:
      if (m->ter(x, y) == t_rock_red)
       m->ter(x, y) = t_floor_red;
       else if (m->ter(x, y) == t_floor_red)
        m->ter(x, y) = t_rock_red;
        else if (m->ter(x, y) == t_rock_green)
         m->ter(x, y) = t_floor_green;
         else if (m->ter(x, y) == t_floor_green)
          m->ter(x, y) = t_rock_green;
          break;
     case t_switch_gb:
      if (m->ter(x, y) == t_rock_blue)
       m->ter(x, y) = t_floor_blue;
       else if (m->ter(x, y) == t_floor_blue)
        m->ter(x, y) = t_rock_blue;
        else if (m->ter(x, y) == t_rock_green)
         m->ter(x, y) = t_floor_green;
         else if (m->ter(x, y) == t_floor_green)
          m->ter(x, y) = t_rock_green;
          break;
     case t_switch_rb:
      if (m->ter(x, y) == t_rock_blue)
       m->ter(x, y) = t_floor_blue;
       else if (m->ter(x, y) == t_floor_blue)
        m->ter(x, y) = t_rock_blue;
        else if (m->ter(x, y) == t_rock_red)
         m->ter(x, y) = t_floor_red;
         else if (m->ter(x, y) == t_floor_red)
          m->ter(x, y) = t_rock_red;
          break;
     case t_switch_even:
      if ((y - examy) % 2 == 1) {
       if (m->ter(x, y) == t_rock_red)
        m->ter(x, y) = t_floor_red;
        else if (m->ter(x, y) == t_floor_red)
         m->ter(x, y) = t_rock_red;
         else if (m->ter(x, y) == t_rock_green)
          m->ter(x, y) = t_floor_green;
          else if (m->ter(x, y) == t_floor_green)
           m->ter(x, y) = t_rock_green;
           else if (m->ter(x, y) == t_rock_blue)
            m->ter(x, y) = t_floor_blue;
            else if (m->ter(x, y) == t_floor_blue)
             m->ter(x, y) = t_rock_blue;
             }
      break;
    }
   }
  }
  g->add_msg("You hear the rumble of rock shifting.");
  g->add_event(EVENT_TEMPLE_SPAWN, g->turn + 3);
}

void iexamine::flower_poppy(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Pick %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 g->add_msg("This flower has a heady aroma");
 if (!(p->is_wearing(itm_mask_filter)||p->is_wearing(itm_mask_gas) ||
       one_in(3)))  {
  g->add_msg("You fall asleep...");
  p->add_disease(DI_SLEEP, 1200, g);
  g->add_msg("Your legs are covered by flower's roots!");
  p->hurt(g,bp_legs, 0, 4);
  p->moves-=50;
 }
 m->ter(examx, examy) = t_dirt;
 m->spawn_item(examx, examy, g->itypes[itm_poppy_flower],0);
 m->spawn_item(examx, examy, g->itypes[itm_poppy_bud],0);
}

void iexamine::tree_apple(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Pick %s?",m->tername(examx, examy).c_str())) return;

 int num_apples = rng(1, p->skillLevel("survival"));
 if (num_apples >= 12)
  num_apples = 12;
 for (int i = 0; i < num_apples; i++)
  m->spawn_item(examx, examy, g->itypes[itm_apple], g->turn, 0);

 m->ter(examx, examy) = t_tree;

}

void iexamine::shrub_blueberry(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Pick %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 int num_blueberries = rng(1, p->skillLevel("survival"));

 if (num_blueberries >= 12)
  num_blueberries = 12;
 for (int i = 0; i < num_blueberries; i++)
  m->spawn_item(examx, examy, g->itypes[itm_blueberries], g->turn, 0);

 m->ter(examx, examy) = t_shrub;
}

void iexamine::shrub_wildveggies(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Pick %s?",m->tername(examx, examy).c_str())) return;

 p->assign_activity(g, ACT_FORAGE, 500 / (p->skillLevel("survival") + 1), 0);
 p->activity.placement = point(examx, examy);
 p->moves = 0;
}

void iexamine::recycler(game *g, player *p, map *m, int examx, int examy) {
 if(!query_yn("Use the %s?",m->tername(examx, examy).c_str())) {
  none(g, p, m, examx, examy);
  return;
 }

 if (m->i_at(examx, examy).size() > 0)
 {
  g->sound(examx, examy, 80, "Ka-klunk!");
  int num_metal = 0;
  for (int i = 0; i < m->i_at(examx, examy).size(); i++)
  {
   item *it = &(m->i_at(examx, examy)[i]);
   if (it->made_of(STEEL))
    num_metal++;
   m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
   i--;
  }
  if (num_metal > 0)
  {
   while (num_metal > 9)
   {
    m->spawn_item(p->posx, p->posy, g->itypes[itm_steel_lump], 0);
    num_metal -= 10;
   }
   do
   {
    m->spawn_item(p->posx, p->posy, g->itypes[itm_steel_chunk], 0);
    num_metal -= 3;
   } while (num_metal > 2);
  }
 }
 else g->add_msg("The recycler is empty.");
}

void iexamine::trap(game *g, player *p, map *m, int examx, int examy) {
 if (g->traps[m->tr_at(examx, examy)]->difficulty < 99 &&
     p->per_cur-p->encumb(bp_eyes) >= g->traps[m->tr_at(examx, examy)]->visibility &&
     query_yn("There is a %s there.  Disarm?",
              g->traps[m->tr_at(examx, examy)]->name.c_str())) {
      m->disarm_trap(g, examx, examy);
  }
}
