#ifndef CHARACTER_H
# define CHARACTER_H

# include <cstdint>
# include <string>

# include "pair.h"

#define DIJKSTRA_PATH_MAX (INT_MAX / 2)
#define NO_NPCS 50

using namespace std;

typedef enum __attribute__ ((__packed__)) movement_type {
  move_hiker,
  move_rival,
  move_pace,
  move_wander,
  move_sentry,
  move_explore,
  move_swim,
  move_pc,
  num_movement_types
} movement_type_t;

typedef enum __attribute__ ((__packed__)) character_type {
  char_pc,
  char_hiker,
  char_rival,
  char_swimmer,
  char_other,
  num_character_types
} character_type_t;

extern const char *char_type_name[num_character_types];

class character {
 public:
  virtual ~character() {}
  pair_t pos;
  char symbol;
  int next_turn;
  int seq_num;
};


class Pokemon : public character {
 public:
  int id, x, y, iv;
  int level;
  char name[30];
  char moves[2][15];
  int HP; 
  int attack, defense, speed, specialAttack, specialDefence;
  bool defeated;
  bool shiny;
  Pokemon(){
    iv = rand() % 16;
    if(rand() % 8192 == 0){
      shiny = 1;
    }
    else{
      shiny = 0;
    }
  }
  
 
  int calcHP(int base, int iv, int level){
      HP = ((((base + iv) * 2) * level) / 100) + level + 10;
      return ((((base + iv) * 2) * level) / 100) + level + 10;
  }
  int calc_other_stat(int base, int iv, int level){
    attack = ((((base + iv) * 2) * level) / 100) + 5;
    defense = ((((base + iv) * 2) * level) / 100) + 5;
    speed = ((((base + iv) * 2) * level) / 100) + 5;
    specialAttack = ((((base + iv) * 2) * level) / 100) + 5;
    specialDefence = ((((base + iv) * 2) * level) / 100) + 5;
    return ((((base + iv) * 2) * level) / 100) + 5;
  }
  /*
  moves
  pokemon_moves[id].pokemon_id = pokemon.species_id
  pokemon_moves.pokemon_move_method_id = 1
  
  */
};

class npc : public character {
 public:
  character_type_t ctype;
  movement_type_t mtype;
  int defeated;
  pair_t dir;
  Pokemon poke[4];
};

class pc : public character {
  public:
    Pokemon poke;
};

/* character is defined in poke327.h to allow an instance of character
 * in world without including character.h in poke327.h                 */

int32_t cmp_char_turns(const void *key, const void *with);
void delete_character(void *v);

extern void (*move_func[num_movement_types])(character *, pair_t);

int pc_move(char);
bool is_pc(character *c);
int level();

#endif
