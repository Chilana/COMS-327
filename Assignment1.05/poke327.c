#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <ncurses/curses.h>


#include "heap.h"

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

#define MAP_X              80
#define MAP_Y              21
#define MIN_TREES          10
#define MIN_BOULDERS       10
#define TREE_PROB          95
#define BOULDER_PROB       95
#define WORLD_SIZE         401

#define MIN_TRAINERS     7
#define ADD_TRAINER_PROB 60

#define MOUNTAIN_SYMBOL       '%'
#define BOULDER_SYMBOL        '0'
#define TREE_SYMBOL           '4'
#define FOREST_SYMBOL         '^'
#define GATE_SYMBOL           '#'
#define PATH_SYMBOL           '#'
#define POKEMART_SYMBOL       'M'
#define POKEMON_CENTER_SYMBOL 'C'
#define TALL_GRASS_SYMBOL     ':'
#define SHORT_GRASS_SYMBOL    '.'
#define WATER_SYMBOL          '~'
#define ERROR_SYMBOL          '&'

#define DIJKSTRA_PATH_MAX (INT_MAX / 2)

#define PC_SYMBOL       '@'
#define HIKER_SYMBOL    'h'
#define RIVAL_SYMBOL    'r'
#define EXPLORER_SYMBOL 'e'
#define SENTRY_SYMBOL   's'
#define PACER_SYMBOL    'p'
#define SWIMMER_SYMBOL  'm'
#define WANDERER_SYMBOL 'w'

#define esc 27

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_boulder,
  ter_tree,
  ter_path,
  ter_mart,
  ter_center,
  ter_grass,
  ter_clearing,
  ter_mountain,
  ter_forest,
  ter_water,
  ter_gate,
  num_terrain_types,
  ter_debug
} terrain_type_t;

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

typedef struct pc {
} pc_t;

typedef struct npc {
  character_type_t ctype;
  movement_type_t mtype;
  pair_t dir;
} npc_t;

typedef struct character {
  npc_t *npc;
  pc_t *pc;
  pair_t pos;
  char symbol;
  int next_turn;
  int seq_num;
  int fought;
} character_t;

typedef struct map {
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  character_t *cmap[MAP_Y][MAP_X];
  heap_t turn;
  int8_t n, s, e, w;
} map_t;

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;



typedef struct world {
  map_t *world[WORLD_SIZE][WORLD_SIZE];
  pair_t cur_idx;
  map_t *cur_map;
  /* Place distance maps in world, not map, since *
   * we only need one pair at any given time.     */
  int hiker_dist[MAP_Y][MAP_X];
  int rival_dist[MAP_Y][MAP_X];
  character_t pc;
  int char_seq_num;
} world_t;

/* Even unallocated, a WORLD_SIZE x WORLD_SIZE array of pointers is a very *
 * large thing to put on the stack.  To avoid that, world is a global.     */
world_t world;

static pair_t all_dirs[8] = {
  { -1, -1 },
  { -1,  0 },
  { -1,  1 },
  {  0, -1 },
  {  0,  1 },
  {  1, -1 },
  {  1,  0 },
  {  1,  1 },
};

/* Just to make the following table fit in 80 columns */
#define IM DIJKSTRA_PATH_MAX
/* Swimmers are not allowed to move onto paths in general, and this *
 * is governed by the swimmer movement code.  However, paths over   *
 * or adjacent to water are bridges.  They can't have inifinite     *
 * movement cost, or it throws a wrench into the turn queue.        */
int32_t move_cost[num_character_types][num_terrain_types] = {
//  boulder,tree,path,mart,center,grass,clearing,mountain,forest,water,gate
  { IM, IM, 10, 10, 10, 20, 10, IM, IM, IM, 10 },
  { IM, IM, 10, 50, 50, 15, 10, 15, 15, IM, IM },
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM },
  { IM, IM,  7, IM, IM, IM, IM, IM, IM,  7, IM },
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM },
};
#undef IM

#define rand_dir(dir) {     \
  int _i = rand() & 0x7;    \
  dir[0] = all_dirs[_i][0]; \
  dir[1] = all_dirs[_i][1]; \
}

#define is_adjacent(pos, ter)                                     \
  ((world.cur_map->map[pos[dim_y] - 1][pos[dim_x] - 1] == ter) || \
   (world.cur_map->map[pos[dim_y] - 1][pos[dim_x]    ] == ter) || \
   (world.cur_map->map[pos[dim_y] - 1][pos[dim_x] + 1] == ter) || \
   (world.cur_map->map[pos[dim_y]    ][pos[dim_x] - 1] == ter) || \
   (world.cur_map->map[pos[dim_y]    ][pos[dim_x] + 1] == ter) || \
   (world.cur_map->map[pos[dim_y] + 1][pos[dim_x] - 1] == ter) || \
   (world.cur_map->map[pos[dim_y] + 1][pos[dim_x]    ] == ter) || \
   (world.cur_map->map[pos[dim_y] + 1][pos[dim_x] + 1] == ter))

#define is_PCadjacent(pos)                                         \
  ((world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] - 1] != NULL && world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] - 1]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x]    ] != NULL && world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] ]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1] != NULL && world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y]    ][pos[dim_x] - 1] != NULL && world.cur_map->cmap[pos[dim_y]   ][pos[dim_x] - 1]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] - 1] != NULL && world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] - 1]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x]    ] != NULL && world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] ]->fought == 0) || \
   (world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] + 1] != NULL && world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] + 1]->fought == 0))

void free_surrond_fight(pair_t pos){             

  if(world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] - 1] != NULL) {
    world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] - 1]->fought = 1;
  }
  else if(world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x]] != NULL)
   world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x]    ]->fought = 1; 
  else if(world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1] != NULL){
   world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1]->fought = 1;
  }
  else if(world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1] != NULL){
   world.cur_map->cmap[pos[dim_y] - 1][pos[dim_x] + 1]->fought = 1; 
  }
  else if(world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] - 1] != NULL){
   world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] - 1]->fought = 1; 
  }
  else if(world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x]    ] != NULL){
   world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x]    ]->fought = 1; 
  }
  else if(world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] + 1] != NULL){
   world.cur_map->cmap[pos[dim_y] + 1][pos[dim_x] + 1]->fought = 1;
  }
}

void pathfind(map_t *m);

uint32_t can_see(map_t *m, character_t *voyeur, character_t *exhibitionist)
{
  /* Application of Bresenham's Line Drawing Algorithm.  If we can draw a   *
   * line from v to e without intersecting any foreign terrain, then v can  *
   * see * e.  Unfortunately, Bresenham isn't symmetric, so line-of-sight   *
   * based on this approach is not reciprocal (Helmholtz Reciprocity).      *
   * This is a very real problem in roguelike games, and one we're going to *
   * ignore for now.  Algorithms that are symmetrical are far more          *
   * expensive.                                                             */

  /* Adapted from rlg327.  For the purposes of poke327, can swimmers see    *
   * the PC adjacent to water or on a bridge?  v is always a swimmer, and e *
   * is always the player character.                                        */

  pair_t first, second;
  pair_t del, f;
  int16_t a, b, c, i;

  first[dim_x] = voyeur->pos[dim_x];
  first[dim_y] = voyeur->pos[dim_y];
  second[dim_x] = exhibitionist->pos[dim_x];
  second[dim_y] = exhibitionist->pos[dim_y];

  if (second[dim_x] > first[dim_x]) {
    del[dim_x] = second[dim_x] - first[dim_x];
    f[dim_x] = 1;
  } else {
    del[dim_x] = first[dim_x] - second[dim_x];
    f[dim_x] = -1;
  }

  if (second[dim_y] > first[dim_y]) {
    del[dim_y] = second[dim_y] - first[dim_y];
    f[dim_y] = 1;
  } else {
    del[dim_y] = first[dim_y] - second[dim_y];
    f[dim_y] = -1;
  }

  if (del[dim_x] > del[dim_y]) {
    a = del[dim_y] + del[dim_y];
    c = a - del[dim_x];
    b = c - del[dim_x];
    for (i = 0; i <= del[dim_x]; i++) {
      if (((mappair(first) != ter_water) && (mappair(first) != ter_path)) &&
          i && (i != del[dim_x])) {
        return 0;
      }
      first[dim_x] += f[dim_x];
      if (c < 0) {
        c += a;
      } else {
        c += b;
        first[dim_y] += f[dim_y];
      }
    }
    return 1;
  } else {
    a = del[dim_x] + del[dim_x];
    c = a - del[dim_y];
    b = c - del[dim_y];
    for (i = 0; i <= del[dim_y]; i++) {
      if (((mappair(first) != ter_water) && (mappair(first) != ter_path)) &&
          i && (i != del[dim_y])) {
        return 0;
      }
      first[dim_y] += f[dim_y];
      if (c < 0) {
        c += a;
      } else {
        c += b;
        first[dim_x] += f[dim_x];
      }
    }
    return 1;
  }

  return 1;
}

static void move_hiker_func(character_t *c, pair_t dest)
{
  int min;
  int base;
  int i;
  
  base = rand() & 0x7;

  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
  min = INT_MAX;
  
  for (i = base; i < 8 + base; i++) {
    if ((world.hiker_dist[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                         [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] <=
         min) &&
        !world.cur_map->cmap[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                            [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] &&
        c->pos[dim_x] + all_dirs[i & 0x7][dim_x] != 0 &&
        c->pos[dim_x] + all_dirs[i & 0x7][dim_x] != MAP_X - 1 &&
        c->pos[dim_y] + all_dirs[i & 0x7][dim_y] != 0 &&
        c->pos[dim_y] + all_dirs[i & 0x7][dim_y] != MAP_Y - 1) {
      dest[dim_x] = c->pos[dim_x] + all_dirs[i & 0x7][dim_x];
      dest[dim_y] = c->pos[dim_y] + all_dirs[i & 0x7][dim_y];
      min = world.hiker_dist[dest[dim_y]][dest[dim_x]];
    }
  }
}

static void move_rival_func(character_t *c, pair_t dest)
{
  int min;
  int base;
  int i;
  
  base = rand() & 0x7;

  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
  min = INT_MAX;
  
  for (i = base; i < 8 + base; i++) {
    if ((world.rival_dist[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                         [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] <
         min) &&
        !world.cur_map->cmap[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                            [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] &&
        c->pos[dim_x] + all_dirs[i & 0x7][dim_x] != 0 &&
        c->pos[dim_x] + all_dirs[i & 0x7][dim_x] != MAP_X - 1 &&
        c->pos[dim_y] + all_dirs[i & 0x7][dim_y] != 0 &&
        c->pos[dim_y] + all_dirs[i & 0x7][dim_y] != MAP_Y - 1) {
      dest[dim_x] = c->pos[dim_x] + all_dirs[i & 0x7][dim_x];
      dest[dim_y] = c->pos[dim_y] + all_dirs[i & 0x7][dim_y];
      min = world.rival_dist[dest[dim_y]][dest[dim_x]];
    }
  }
}

static void move_pacer_func(character_t *c, pair_t dest)
{
  terrain_type_t t;
  
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  t = world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                        [c->pos[dim_x] + c->npc->dir[dim_x]];

  if ((t != ter_path && t != ter_grass && t != ter_clearing) ||
      world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    c->npc->dir[dim_x] *= -1;
    c->npc->dir[dim_y] *= -1;
  }

  if ((t == ter_path || t == ter_grass || t == ter_clearing) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_wanderer_func(character_t *c, pair_t dest)
{
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] !=
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) ||
      world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    rand_dir(c->npc->dir);
  }

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] ==
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_sentry_func(character_t *c, pair_t dest)
{
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
}

static void move_explorer_func(character_t *c, pair_t dest)
{
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if ((move_cost[char_other][world.cur_map->map[c->pos[dim_y] +
                                                c->npc->dir[dim_y]]
                                               [c->pos[dim_x] +
                                                c->npc->dir[dim_x]]] ==
       DIJKSTRA_PATH_MAX) ||
      world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    rand_dir(c->npc->dir);
  }

  if ((move_cost[char_other][world.cur_map->map[c->pos[dim_y] +
                                                c->npc->dir[dim_y]]
                                               [c->pos[dim_x] +
                                                c->npc->dir[dim_x]]] !=
       DIJKSTRA_PATH_MAX) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_swimmer_func(character_t *c, pair_t dest)
{
  map_t *m = world.cur_map;
  pair_t dir; 

  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if (is_adjacent(world.pc.pos, ter_water) &&
      can_see(world.cur_map, c, &world.pc)) {
    /* PC is next to this body of water; swim to the PC */

    dir[dim_x] = world.pc.pos[dim_x] - c->pos[dim_x];
    if (dir[dim_x]) {
      dir[dim_x] /= abs(dir[dim_x]);
    }
    dir[dim_y] = world.pc.pos[dim_y] - c->pos[dim_y];
    if (dir[dim_y]) {
      dir[dim_y] /= abs(dir[dim_y]);
    }

    if ((m->map[dest[dim_y] + dir[dim_y]]
               [dest[dim_x] + dir[dim_x]] == ter_water) ||
        ((m->map[dest[dim_y] + dir[dim_y]]
                [dest[dim_x] + dir[dim_x]] == ter_path) &&
         is_adjacent(((pair_t){ (dest[dim_x] + dir[dim_x]),
                                (dest[dim_y] + dir[dim_y]) }), ter_water))) {
      dest[dim_x] += dir[dim_x];
      dest[dim_y] += dir[dim_y];
    } else if ((m->map[dest[dim_y]][dest[dim_x] + dir[dim_x]] == ter_water) ||
               ((m->map[dest[dim_y]][dest[dim_x] + dir[dim_x]] == ter_path) &&
                is_adjacent(((pair_t){ (dest[dim_x] + dir[dim_x]),
                                       (dest[dim_y]) }), ter_water))) {
      dest[dim_x] += dir[dim_x];
    } else if ((m->map[dest[dim_y] + dir[dim_y]][dest[dim_x]] == ter_water) ||
               ((m->map[dest[dim_y] + dir[dim_y]][dest[dim_x]] == ter_path) &&
                is_adjacent(((pair_t){ (dest[dim_x]),
                                       (dest[dim_y] + dir[dim_y]) }),
                            ter_water))) {
      dest[dim_y] += dir[dim_y];
    }
  } else {
    /* PC is elsewhere.  Keep doing laps. */
    dir[dim_x] = c->npc->dir[dim_x];
    dir[dim_y] = c->npc->dir[dim_y];
    if ((m->map[dest[dim_y] + dir[dim_y]]
                    [dest[dim_x] + dir[dim_x]] != ter_water) ||
        !((m->map[dest[dim_y] + dir[dim_y]]
                 [dest[dim_x] + dir[dim_x]] == ter_path) &&
          is_adjacent(((pair_t) { dest[dim_x] + dir[dim_x],
                                  dest[dim_y] + dir[dim_y] }), ter_water))) {
      rand_dir(dir);
    }

    if ((m->map[dest[dim_y] + dir[dim_y]]
                    [dest[dim_x] + dir[dim_x]] == ter_water) ||
        ((m->map[dest[dim_y] + dir[dim_y]]
                [dest[dim_x] + dir[dim_x]] == ter_path) &&
         is_adjacent(((pair_t) { dest[dim_x] + dir[dim_x],
                                 dest[dim_y] + dir[dim_y] }), ter_water))) {
      dest[dim_x] += dir[dim_x];
      dest[dim_y] += dir[dim_y];
    }
  }

  if (m->cmap[dest[dim_y]][dest[dim_x]]) {
    /* Occupied.  Just be patient. */
    dest[dim_x] = c->pos[dim_x];
    dest[dim_y] = c->pos[dim_y];
  }
}

static void move_pc_func(character_t *c, pair_t dest)
{
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
}

void (*move_func[num_movement_types])(character_t *, pair_t) = {
  move_hiker_func,
  move_rival_func,
  move_pacer_func,
  move_wanderer_func,
  move_sentry_func,
  move_explorer_func,
  move_swimmer_func,
  move_pc_func,
};

void rand_pos(pair_t pos)
{
  pos[dim_x] = (rand() % (MAP_X - 2)) + 1;
  pos[dim_y] = (rand() % (MAP_Y - 2)) + 1;
}

void new_hiker()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.hiker_dist[pos[dim_y]][pos[dim_x]] >= DIJKSTRA_PATH_MAX ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_hiker;
  c->npc->mtype = move_hiker;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->symbol = HIKER_SYMBOL;
  c->next_turn = 0;
  c->seq_num = world.char_seq_num++;
  heap_insert(&world.cur_map->turn, c);
  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c;
}

void new_rival()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] >= DIJKSTRA_PATH_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_rival;
  c->npc->mtype = move_rival;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->symbol = RIVAL_SYMBOL;
  c->next_turn = 0;
  c->seq_num = world.char_seq_num++;
  heap_insert(&world.cur_map->turn, c);
  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c;
}

void new_swimmer()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.cur_map->map[pos[dim_y]][pos[dim_x]] != ter_water ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_swimmer;
  c->npc->mtype = move_swim;
  rand_dir(c->npc->dir);
  c->symbol = SWIMMER_SYMBOL;
  c->next_turn = 0;
  c->seq_num = world.char_seq_num++;
  heap_insert(&world.cur_map->turn, c);
  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c;
}

void new_char_other()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] >= DIJKSTRA_PATH_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_other;
  switch (rand() % 4) {
  case 0:
    c->npc->mtype = move_pace;
    c->symbol = PACER_SYMBOL;
    break;
  case 1:
    c->npc->mtype = move_wander;
    c->symbol = WANDERER_SYMBOL;
    break;
  case 2:
    c->npc->mtype = move_sentry;
    c->symbol = SENTRY_SYMBOL;
    break;
  case 3:
    c->npc->mtype = move_explore;
    c->symbol = EXPLORER_SYMBOL;
    break;
  }
  rand_dir(c->npc->dir);
  c->next_turn = 0;
  c->seq_num = world.char_seq_num++;
  heap_insert(&world.cur_map->turn, c);
  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c;
}

void place_characters()
{
  int num_trainers = 3;

  //Always place a hiker and a rival, then place a random number of others
  new_hiker();
  new_rival();
  new_swimmer();
  do {
    //higher probability of non- hikers and rivals
    switch(rand() % 10) {
    case 0:
      new_hiker();
      break;
    case 1:
     new_rival();
      break;
    case 2:
      new_swimmer();
      break;
    default:
      new_char_other();
      break;
    }
  } while (++num_trainers < MIN_TRAINERS ||
           ((rand() % 100) < ADD_TRAINER_PROB));
}

int32_t cmp_char_turns(const void *key, const void *with)
{
  return ((((character_t *) key)->next_turn ==
           ((character_t *) with)->next_turn)  ?
          (((character_t *) key)->seq_num -
           ((character_t *) with)->seq_num)    :
          (((character_t *) key)->next_turn -
           ((character_t *) with)->next_turn));
}

void delete_character(void *v)
{
  if (v == &world.pc) {
    free(world.pc.pc);
  } else {
    free(((character_t *) v)->npc);
    free(v);
  }
}

void init_pc()
{
  int x, y;

  do {
    x = rand() % (MAP_X - 2) + 1;
    y = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[y][x] != ter_path);

  world.pc.pos[dim_x] = x;
  world.pc.pos[dim_y] = y;
  world.pc.symbol = PC_SYMBOL;
  world.pc.pc = malloc(sizeof (*world.pc.pc));

  world.cur_map->cmap[y][x] = &world.pc;
  world.pc.next_turn = 0;

  world.pc.seq_num = world.char_seq_num++;

  heap_insert(&world.cur_map->turn, &world.pc);
}

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(int8_t x, int8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        /* Don't overwrite the gate */
        if (x != to[dim_x] || y != to[dim_y]) {
          mapxy(x, y) = ter_path;
          heightxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1)))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1)))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

static int build_paths(map_t *m)
{
  pair_t from, to;

  /*  printf("%d %d %d %d\n", m->n, m->s, m->e, m->w);*/

  if (m->e != -1 && m->w != -1) {
    from[dim_x] = 1;
    to[dim_x] = MAP_X - 2;
    from[dim_y] = m->w;
    to[dim_y] = m->e;

    dijkstra_path(m, from, to);
  }

  if (m->n != -1 && m->s != -1) {
    from[dim_y] = 1;
    to[dim_y] = MAP_Y - 2;
    from[dim_x] = m->n;
    to[dim_x] = m->s;

    dijkstra_path(m, from, to);
  }

  if (m->e == -1) {
    if (m->s == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->w == -1) {
    if (m->s == -1) {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->n == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->s == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }

    dijkstra_path(m, from, to);
  }

  return 0;
}

static int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};

static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof (height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */

  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
  do {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x] + 2, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] - 1) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] + 2) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path)))   &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path)))) {
          break;
    }
  } while (1);
}

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

/* Chooses tree or boulder for border cell.  Choice is biased by dominance *
 * of neighboring cells.                                                   */
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y)
{
  int32_t p, q;
  int32_t r, t;
  int32_t miny, minx, maxy, maxx;
  
  r = t = 0;
  
  miny = y - 1 >= 0 ? y - 1 : 0;
  maxy = y + 1 <= MAP_Y ? y + 1: MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1: MAP_X;

  for (q = miny; q < maxy; q++) {
    for (p = minx; p < maxx; p++) {
      if (q != y || p != x) {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder) {
          r++;
        } else if (m->map[q][p] == ter_forest ||
                   m->map[q][p] == ter_tree) {
          t++;
        }
      }
    }
  }
  
  if (t == r) {
    return rand() & 1 ? ter_boulder : ter_tree;
  } else if (t > r) {
    if (rand() % 10) {
      return ter_tree;
    } else {
      return ter_boulder;
    }
  } else {
    if (rand() % 10) {
      return ter_boulder;
    } else {
      return ter_tree;
    }
  }
}

static int map_terrain(map_t *m, int8_t n, int8_t s, int8_t e, int8_t w)
{
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
  int num_grass, num_clearing, num_mountain, num_forest, num_water, num_total;
  terrain_type_t type;
  int added_current = 0;
  
  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_water = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest + num_water;

  memset(&m->map, 0, sizeof (m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0) {
      type = ter_grass;
    } else if (i == num_grass) {
      type = ter_clearing;
    } else if (i == num_grass + num_clearing) {
      type = ter_mountain;
    } else if (i == num_grass + num_clearing + num_mountain) {
      type = ter_forest;
    } else if (i == num_grass + num_clearing + num_mountain + num_forest) {
      type = ter_water;
    }
    m->map[y][x] = type;
    if (i == 0) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m->map[y][x];
    
    if (x - 1 >= 0 && !m->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x - 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y - 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y + 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x + 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1) {
        mapxy(x, y) = border_type(m, x, y);
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  if (n != -1) {
    mapxy(n,         0        ) = ter_gate;
    mapxy(n,         1        ) = ter_gate;
  }
  if (s != -1) {
    mapxy(s,         MAP_Y - 1) = ter_gate;
    mapxy(s,         MAP_Y - 2) = ter_gate;
  }
  if (w != -1) {
    mapxy(0,         w        ) = ter_gate;
    mapxy(1,         w        ) = ter_gate;
  }
  if (e != -1) {
    mapxy(MAP_X - 1, e        ) = ter_gate;
    mapxy(MAP_X - 2, e        ) = ter_gate;
  }

  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest &&
        m->map[y][x] != ter_path   &&
        m->map[y][x] != ter_gate) {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;
  
  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path     &&
        m->map[y][x] != ter_water    &&
        m->map[y][x] != ter_gate) {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

// New map expects cur_idx to refer to the index to be generated.  If that
// map has already been generated then the only thing this does is set
// cur_map.
static int new_map()
{
  int d, p;
  int e, w, n, s;
  int x, y;
  
  if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]]) {
    world.cur_map = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]];
    return 0;
  }

  world.cur_map                                             =
    world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]] =
    malloc(sizeof (*world.cur_map));

  smooth_height(world.cur_map);
  
  if (!world.cur_idx[dim_y]) {
    n = -1;
  } else if (world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]) {
    n = world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]->s;
  } else {
    n = 1 + rand() % (MAP_X - 2);
  }
  if (world.cur_idx[dim_y] == WORLD_SIZE - 1) {
    s = -1;
  } else if (world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]) {
    s = world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]->n;
  } else  {
    s = 1 + rand() % (MAP_X - 2);
  }
  if (!world.cur_idx[dim_x]) {
    w = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]) {
    w = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]->e;
  } else {
    w = 1 + rand() % (MAP_Y - 2);
  }
  if (world.cur_idx[dim_x] == WORLD_SIZE - 1) {
    e = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]) {
    e = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]->w;
  } else {
    e = 1 + rand() % (MAP_Y - 2);
  }
  
  map_terrain(world.cur_map, n, s, e, w);
     
  place_boulders(world.cur_map);
  place_trees(world.cur_map);
  build_paths(world.cur_map);
  d = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
       abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
  p = d > 200 ? 5 : (50 - ((45 * d) / 200));
  //  printf("d=%d, p=%d\n", d, p);
  if ((rand() % 100) < p || !d) {
    place_pokemart(world.cur_map);
  }
  if ((rand() % 100) < p || !d) {
    place_center(world.cur_map);
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      world.cur_map->cmap[y][x] = NULL;
    }
  }

  heap_init(&world.cur_map->turn, cmp_char_turns, delete_character);

  init_pc();
  pathfind(world.cur_map);
  place_characters();

  return 0;
}

static void print_map()
{
  int x, y;
  int default_reached = 0;

  //printf("\n\n\n");

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (world.cur_map->cmap[y][x] != NULL)
      {
        mvprintw(y + 3, x, "%c", world.cur_map->cmap[y][x]->symbol);
        if(world.cur_map->cmap[y][x] != &world.pc){
          world.cur_map->cmap[y][x]->fought = 0;
        }
      }
      else
      {
        switch (world.cur_map->map[y][x])
        {
        case ter_boulder:
          // putchar(BOULDER_SYMBOL);
          attron(COLOR_PAIR(5));
          mvprintw(y + 3, x, "%c", BOULDER_SYMBOL);
          attroff(COLOR_PAIR(5));
          break;
        case ter_mountain:
          // putchar(MOUNTAIN_SYMBOL);
          attron(COLOR_PAIR(7));
          mvprintw(y + 3, x, "%c", MOUNTAIN_SYMBOL);
          attroff(COLOR_PAIR(7));
          break;
        case ter_tree:
          attron(COLOR_PAIR(5));
          mvprintw(y + 3, x, "%c", TREE_SYMBOL);
          attroff(COLOR_PAIR(5));
          break;
        case ter_forest:
          attron(COLOR_PAIR(1));
          mvprintw(y + 3, x, "%c", FOREST_SYMBOL);
          attroff(COLOR_PAIR(1));
          break;
        case ter_path:
          attron(COLOR_PAIR(2));
          mvprintw(y + 3, x, "%c", PATH_SYMBOL);
          attroff(COLOR_PAIR(2));
          break;
        case ter_gate:
          attron(COLOR_PAIR(2));
          mvprintw(y + 3, x, "%c", GATE_SYMBOL);
          attroff(COLOR_PAIR(2));
          break;
        case ter_mart:
          attron(COLOR_PAIR(6));
          mvprintw(y + 3, x, "%c", POKEMART_SYMBOL);
          attroff(COLOR_PAIR(6));
          break;
        case ter_center:
          attron(COLOR_PAIR(6));
          mvprintw(y + 3, x, "%c", POKEMON_CENTER_SYMBOL);
          attroff(COLOR_PAIR(6));
          break;
        case ter_grass:
          attron(COLOR_PAIR(4));
          mvprintw(y + 3, x, "%c", TALL_GRASS_SYMBOL);
          attroff(COLOR_PAIR(4));
          break;
        case ter_clearing:
          attron(COLOR_PAIR(4));
          mvprintw(y + 3, x, "%c", SHORT_GRASS_SYMBOL);
          attroff(COLOR_PAIR(4));
          break;
        case ter_water:
          attron(COLOR_PAIR(3));
          mvprintw(y + 3, x, "%c", WATER_SYMBOL);
          attroff(COLOR_PAIR(3));
          break;
        default:
          mvprintw(y + 3, x, "%c", ERROR_SYMBOL);
          default_reached = 1;
          break;
        }
      }
      refresh();
    }
  }

  if (default_reached)
  {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}

// The world is global because of its size, so init_world is parameterless
void init_world()
{
  world.cur_idx[dim_x] = world.cur_idx[dim_y] = WORLD_SIZE / 2;
  world.char_seq_num = 0;
  new_map();
}

void delete_world()
{
  int x, y;

  for (y = 0; y < WORLD_SIZE; y++)
  {
    for (x = 0; x < WORLD_SIZE; x++)
    {
      if (world.world[y][x])
      {
        free(world.world[y][x]);
        world.world[y][x] = NULL;
      }
    }
  }
}

#define ter_cost(x, y, c) move_cost[c][m->map[y][x]]

static int32_t hiker_cmp(const void *key, const void *with)
{
  return (world.hiker_dist[((path_t *)key)->pos[dim_y]]
                          [((path_t *)key)->pos[dim_x]] -
          world.hiker_dist[((path_t *)with)->pos[dim_y]]
                          [((path_t *)with)->pos[dim_x]]);
}

static int32_t rival_cmp(const void *key, const void *with) {
  return (world.rival_dist[((path_t *) key)->pos[dim_y]]
                          [((path_t *) key)->pos[dim_x]] -
          world.rival_dist[((path_t *) with)->pos[dim_y]]
                          [((path_t *) with)->pos[dim_x]]);
}

void pathfind(map_t *m)
{
  heap_t h;
  uint32_t x, y;
  static path_t p[MAP_Y][MAP_X], *c;
  static uint32_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      world.hiker_dist[y][x] = world.rival_dist[y][x] = DIJKSTRA_PATH_MAX;
    }
  }
  world.hiker_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 
    world.rival_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 0;

  heap_init(&h, hiker_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_hiker) != DIJKSTRA_PATH_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);

  heap_init(&h, rival_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_rival) != DIJKSTRA_PATH_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

void print_hiker_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.hiker_dist[y][x] == DIJKSTRA_PATH_MAX) {
        printf("   ");
      } else {
        printf(" %02d", world.hiker_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void print_rival_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.rival_dist[y][x] == DIJKSTRA_PATH_MAX ||
          world.rival_dist[y][x] < 0) {
        printf("   ");
      } else {
        printf(" %02d", world.rival_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void print_character(character_t *c)
{
  //int xpos = c->pos[dim_x] - pc
  printf("%c: <%d,%d> %d (%d)\n", c->symbol, c->pos[dim_x],
         c->pos[dim_y], c->next_turn, c->seq_num);
}

char getTerChar(int x, int y)
{

  switch(world.cur_map->map[y][x])
  {
  case ter_boulder:
    return BOULDER_SYMBOL;
     break;
  case ter_mountain:
    return MOUNTAIN_SYMBOL;
     break;
  case ter_tree:
    return TREE_SYMBOL;
     break;
  case ter_forest:
    return FOREST_SYMBOL;
     break;
  case ter_path:
    return PATH_SYMBOL;
     break;
  case ter_gate:
    return GATE_SYMBOL;
     break;
  case ter_mart:
    return POKEMART_SYMBOL;
     break;
  case ter_center:
    return POKEMON_CENTER_SYMBOL;
     break;
  case ter_grass:
    return TALL_GRASS_SYMBOL;
     break;
  case ter_clearing:
    return SHORT_GRASS_SYMBOL;
     break;
  case ter_water:
    return WATER_SYMBOL;
     break;
  default:
    return 'P';
  }
}

void freePc(int y, int x){
  world.cur_map->cmap[y][x] = NULL;
}

void putPc(int y, int x){
  world.cur_map->cmap[y][x] = &world.pc;
}
void game_loop()
{
  WINDOW *pokebat = newwin(1, 30, 1, 0);
  character_t *c;
  pair_t d;
  print_map();
  
  
  while (getch() != 'q')
  {
    c = heap_remove_min(&world.cur_map->turn);
    //mvprintw(1, 0, "%c", c->symbol);
    int pcX = world.pc.pos[dim_x];
    int pcY = world.pc.pos[dim_y];
    int once = 0;
    if(is_PCadjacent(world.pc.pos) && once == 0){
        mvwprintw(pokebat, 0, 0, "Oh shit");
        wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                free_surrond_fight(world.pc.pos);
                break;
              }
            }
      
    }

    // curter = world.cur_map->map[pcY][pcX];
    //c = heap_remove_min(&world.cur_map->turn);
    if(c == &world.pc){
      if (getch() == 'y' || getch() == '7')
      {
        if (world.cur_map->map[pcY - 1][pcX - 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY - 1][pcX - 1]] != DIJKSTRA_PATH_MAX)
          {
          if (world.cur_map->cmap[pcY - 1][pcX - 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                world.cur_map->cmap[pcY - 1][pcX - 1]->fought = 1;
                wrefresh(pokebat);
                refresh();
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX - 1;
            world.pc.pos[dim_y] = pcY - 1;
            mvaddch(pcY + 2, pcX - 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
          }
        // print_map();
        }
     }
      else if (getch() == '8' || getch() == 'k')
      {
        if (world.cur_map->map[pcY - 1][pcX] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY - 1][pcX]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY - 1][pcX] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY - 1][pcX]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX;
            world.pc.pos[dim_y] = pcY - 1;
            mvaddch(pcY + 2, pcX, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '9' || getch() == 'u')
      {
        if (world.cur_map->map[pcY - 1][pcX + 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY - 1][pcX + 1]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY - 1][pcX + 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY - 1][pcX + 1]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX + 1;
            world.pc.pos[dim_y] = pcY - 1;
            mvaddch(pcY + 2, pcX + 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '6' || getch() == 'l')
      {
        if (world.cur_map->map[pcY][pcX + 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY][pcX + 1]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY][pcX + 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY][pcX + 1]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX + 1;
            world.pc.pos[dim_y] = pcY;
            mvaddch(pcY + 3, pcX + 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '3' || getch() == 'n')
      {
        if (world.cur_map->map[pcY + 1][pcX + 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY + 1][pcX + 1]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY + 1][pcX + 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY + 1][pcX + 1]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX + 1;
            world.pc.pos[dim_y] = pcY + 1;
            mvaddch(pcY + 4, pcX + 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '2' || getch() == 'j')
      {
        if (world.cur_map->map[pcY + 1][pcX] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY + 1][pcX]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY + 1][pcX] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY + 1][pcX]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX;
            world.pc.pos[dim_y] = pcY + 1;
            mvaddch(pcY + 4, pcX, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '1' || getch() == 'b')
      {
        if (world.cur_map->map[pcY + 1][pcX - 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY + 1][pcX - 1]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY + 1][pcX - 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY + 1][pcX - 1]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX - 1;
            world.pc.pos[dim_y] = pcY + 1;
            mvaddch(pcY + 4, pcX - 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }

      else if (getch() == '4' || getch() == 'h')
      {
        if (world.cur_map->map[pcY][pcX - 1] != ter_gate && move_cost[char_pc][world.cur_map->map[pcY][pcX - 1]] != DIJKSTRA_PATH_MAX)
        {
          if (world.cur_map->cmap[pcY][pcX - 1] != NULL)
          {
            mvwprintw(pokebat, 0, 0, "Oh shit");
            wrefresh(pokebat);
            while (1)
            {
              if (getch() == esc)
              {
                for (int i = 0; i < 30; i++)
                {
                  mvwaddstr(pokebat, 0, i, " ");
                }
                wrefresh(pokebat);
                refresh();
                world.cur_map->cmap[pcY][pcX - 1]->fought = 1;
                break;
              }
            }
          }
          else
          {
            freePc(pcY, pcX);
            world.pc.pos[dim_x] = pcX - 1;
            world.pc.pos[dim_y] = pcY;
            mvaddch(pcY + 3, pcX - 1, '@');
            mvaddch(pcY + 3, pcX, getTerChar(pcX, pcY));
            putPc(world.pc.pos[dim_y], world.pc.pos[dim_x]);
            refresh();
            // print_map();
          }
        }
      }
      else if (getch() == '>')
      {
        if (getTerChar(pcX, pcY) == POKEMART_SYMBOL || getTerChar(pcX, pcY) == POKEMON_CENTER_SYMBOL){
          //new window Ig
          character_t *v;
          //int i = 0;
          WINDOW *pokeCent = newwin(15, 30, 0, 30);
          mvwprintw(pokeCent, 1, 0, "Welcome to the pokeplaces");
          //mvwprintw(pokeCent, 2, 0,"Lemme know what you tryna get i got gas");
          wrefresh(pokeCent);

          while(1){
            if(getch() == '<'){
              for(int i = 0; i < 15; i++){
                for(int j = 0; j < 30; j++){
                  mvwaddstr(pokeCent, i, j, " ");
                }
              }
              //mvwaddstr(pokeCent,i, j, " ");
              //delwin(pokeCent);
              wrefresh(pokeCent);
              for(int y = 0; y < 15; y++){
                for(int x = 30; x < 60; x++){
                  if(world.cur_map->cmap[y][x] != NULL){
                    v = world.cur_map->cmap[y][x];
                    mvprintw(y + 3, x, "%c", v->symbol);
                  }
                  else{
                    mvprintw(y + 3, x, "%c", getTerChar(x, y));
                  }
                }
              }
              refresh();
              break;
            }
          }
          /*c = heap_peek_min(&world.cur_map->turn);
          while(1){
            mvwprintw(npcDisp, i, 0, "%c: <%d,%d> %d (%d)\n", c->symbol, c->pos[dim_x],
            c->pos[dim_y], c->next_turn, c->seq_num);

          }*/
        }
      }
      else if(getch() == '5' || getch() == ' ' || getch() == '.'){
        character_t *t;
        pair_t b;
        //t = heap_remove_min(&world.cur_map->turn);
        //int count = t->seq_num 
        for(int y = 0; y < MAP_Y; y++){
          for(int x = 0; x < MAP_X; x++){
            if(world.cur_map->cmap[y][x] != NULL && world.cur_map->cmap[y][x] != &world.pc){
              t = world.cur_map->cmap[y][x];
              move_func[t->npc->mtype](t, b);
              int cX, cY;
              cX = t->pos[dim_x];
              cY = t->pos[dim_y];
              
              world.cur_map->cmap[t->pos[dim_y]][t->pos[dim_x]] = NULL;
              world.cur_map->cmap[b[dim_y]][b[dim_x]] = t;
              t->next_turn += move_cost[t->npc->ctype][world.cur_map->map[b[dim_y]]
                                                                        [b[dim_x]]];
              t->pos[dim_y] = b[dim_y];
              t->pos[dim_x] = b[dim_x];

              mvaddch(t->pos[dim_y] + 3, t->pos[dim_x], t->symbol);
              mvaddch(cY + 3, cX, getTerChar(cX, cY));
              refresh();
            }
          }
        }
      }
      else if (getch() == 't')
      {
        WINDOW *tDisp = newwin(24, 50, 0, 15);
        character_t *d;//, *l;
        char ydir[10];
        char hdir[10];
        int xD, yD;
        int i = 4;
        mvwprintw(tDisp, i - 1, 0, "List of Trainers on the map:");
        for(int y = 0; y < MAP_Y; y++){
          for(int x = 0; x < MAP_X; x++){
            if(world.cur_map->cmap[y][x] != NULL && world.cur_map->cmap[y][x] != &world.pc){
              d = world.cur_map->cmap[y][x];

              xD = d->pos[dim_x] - pcX;
              yD = d->pos[dim_y] - pcY;
              if (xD > 0)
              {
                strcpy(hdir, "east");
              }
              else
              {
                strcpy(hdir, "west");
                xD *= -1;
              }
              if (yD > 0)
              {
                strcpy(ydir, "south");
              }
              else
              {
                strcpy(ydir, "north");
                yD *= -1;
              }
              mvwprintw(tDisp, i, 0, "%c, %d %s and %d %s ", d->symbol, yD, ydir,
                        xD, hdir);
              i += 1;
              wrefresh(tDisp);

            }
          }
        }
        //mvwprintw(tDisp, 10, 0, "Works");
        //wrefresh(tDisp);
        
        //refresh();
        while (1)
        {
          if (getch() == esc)
          {
            for (int i = 0; i < 24; i++)
            {
              for (int j = 0; j < 50; j++)
              {
                mvwaddstr(tDisp, i, j, " ");
              }
            }
            // mvwaddstr(pokeCent,i, j, " ");
            //delwin(tDisp);
            //wrefresh(tDisp);
            for (int y = 0; y < 21; y++)
            {
              for (int x = 15; x < 65; x++)
              {
                //if(world.cur_map->cmap[y][x] != NULL){
                //  l = world.cur_map->cmap[y][x];
                //  mvprintw(y + 3, x, "%c", l->symbol);
                //}
                if(world.cur_map->cmap[y][x] == &world.pc){
                  mvprintw(y + 3, x, "%c", '@');
                }
                else{
                  mvprintw(y + 3, x, "%c", getTerChar(x, y));
                }
              }
            }
            refresh();
            break;
          }
        }
      }
      c->next_turn += move_cost[char_pc][world.cur_map->map[c->pos[dim_y]]
                                                           [c->pos[dim_x]]];
      pathfind(world.cur_map);
    }
    else{

      //mvprintw(0, 0, "BRO");
      if(c->fought == 0){
        move_func[c->npc->mtype](c, d);
        world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
        world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;
        c->next_turn += move_cost[c->npc->ctype][world.cur_map->map[d[dim_y]]
                                                                  [d[dim_x]]];
        
        int xc = c->pos[dim_x];
        int yc = c->pos[dim_y];
        if(world.cur_map->cmap[d[dim_y]][d[dim_x]] != &world.pc){
          c->pos[dim_y] = d[dim_y];
          c->pos[dim_x] = d[dim_x];
          mvaddch(c->pos[dim_y] + 3, c->pos[dim_x], c->symbol);
          mvaddch(yc + 3, xc, getTerChar(xc, yc));
          refresh();
        }
      }
      else{
        move_wanderer_func(c, d);
        world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
        world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;
        c->next_turn += move_cost[c->npc->ctype][world.cur_map->map[d[dim_y]]
                                                                 [d[dim_x]]];

        int xc = c->pos[dim_x];
        int yc = c->pos[dim_y];
        if(world.cur_map->cmap[d[dim_y]][d[dim_x]] != &world.pc){
          c->pos[dim_y] = d[dim_y];
          c->pos[dim_x] = d[dim_x];
          mvaddch(yc + 3, xc, getTerChar(xc, yc));
          mvaddch(c->pos[dim_y] + 3, c->pos[dim_x], c->symbol);
          refresh();
        }
      }
    }

    heap_insert(&world.cur_map->turn, c);
    //
    //  c = heap_remove_min(&world.cur_map->turn);
    // print_character(c);
    /*else if (c == &world.pc)
    { // && getch() == '5' || getch() == ' ' || getch() == '.'
      // print_map();
      // usleep(250000);
      c->next_turn += move_cost[char_pc][world.cur_map->map[c->pos[dim_y]]
                                                           [c->pos[dim_x]]];
    }
    else
    {
      move_func[c->npc->mtype](c, d);
      world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
      world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;
      c->next_turn += move_cost[c->npc->ctype][world.cur_map->map[d[dim_y]]
                                                                 [d[dim_x]]];
      c->pos[dim_y] = d[dim_y];
      c->pos[dim_x] = d[dim_x];
    }
    heap_insert(&world.cur_map->turn, c);
    
    else if(getch() == '5' || getch() == ' ' || getch() == '.'){
      character_t *p;
      while(((p = heap_remove_min(&world.cur_map->turn)) && p != &world.pc){
          move_func[c->npc->mtype](c, d);
      world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
      world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;
      c->next_turn += move_cost[c->npc->ctype][world.cur_map->map[d[dim_y]]
                                                                 [d[dim_x]]];
      c->pos[dim_y] = d[dim_y];
      c->pos[dim_x] = d[dim_x];
      heap_insert(&world.cur_map->turn, c);
      }
    }*/
    
  }
}

int main(int argc, char *argv[])
{
  struct timeval tv;
  uint32_t seed;

  if (argc == 2) {
    seed = atoi(argv[1]);
  } else {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  srand(seed);


  initscr();
  keypad(stdscr, TRUE);
  raw();
  cbreak();
  noecho();
  start_color();
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_WHITE, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_BLACK);
  init_pair(5, COLOR_RED, COLOR_BLACK);
  init_pair(6, COLOR_CYAN, COLOR_BLACK);
  init_color(COLOR_MAGENTA, 128, 128, 128);
  init_pair(7, COLOR_MAGENTA, COLOR_BLACK);

  init_world();

//  WINDOW *npcDisp = newwin(10, 20, 5, 20);
  
  

  game_loop();

  delete_world();

  endwin();

  printf("But how are you going to be the very best if you quit?\n");
  
  return 0;
}
