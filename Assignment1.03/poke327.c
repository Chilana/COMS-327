#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include <endian.h>
// #include <sys/stat.h>
// #include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>

#include "heap.h"

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

typedef struct path
{
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef struct npc
{
  heap_node_t *hn;
  int32_t cost;
  uint8_t pos[2];
  uint8_t from[2];
  int dist;
  //pair_t pPos;
} npc_t;



typedef enum dim
{
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef uint8_t pair_t[num_dims];

#define MAP_X 80
#define MAP_Y 21
#define MIN_TREES 10
#define MIN_BOULDERS 10
#define TREE_PROB 95
#define BOULDER_PROB 95

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__((__packed__)) terrain_type
{
  ter_debug,
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
  ter_pc
} terrain_type_t;

typedef struct pc
{
  int x, y;
  char pc;
  bool placed;
  pair_t pPos;
} pc_t;

typedef struct map
{
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  uint8_t n, s, e, w;
  pc_t playc;
} map_t;

pc_t pC;

typedef struct queue_node
{
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int32_t path_cmp(const void *key, const void *with)
{
  return ((path_t *)key)->cost - ((path_t *)with)->cost;
}
/*static int32_t ter_cmp(const void *key, const void *with)
{
  return ((npc_t *)key)->cost - ((npc_t *)with)->cost;
}*/

static int32_t edge_penalty(uint8_t x, uint8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized)
  {
    for (y = 0; y < MAP_Y; y++)
    {
      for (x = 0; x < MAP_X; x++)
      {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x])
    {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y])
      {
        mapxy(x, y) = ter_path;
        heightxy(x, y) = 0;

        /*if(i == randTime && !m->playc.placed){
          mapxy(x, y) = ter_pc;
          m->playc.x = x;
          m->playc.y = y;
          m->playc.placed = true;
        }
        i++;*/
      }
      heap_delete(&h);
      // mapxy(pcX, pcY) = ter_pc;

      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1))))
    {
      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]]
                                               .hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]))))
    {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] - 1]
                                               .hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]))))
    {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] + 1]
                                               .hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1))))
    {
      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]]
                                               .hn);
    }
  }
}

static int build_paths(map_t *m)
{
  pair_t from, to;

  from[dim_x] = 1;
  to[dim_x] = MAP_X - 2;
  from[dim_y] = m->w;
  to[dim_y] = m->e;

  m->playc.placed = false;
  dijkstra_path(m, from, to);

  from[dim_y] = 1;
  to[dim_y] = MAP_Y - 2;
  from[dim_x] = m->n;
  to[dim_x] = m->s;

  dijkstra_path(m, from, to);

  return 0;
}

static int gaussian[5][5] = {
    {1, 4, 7, 4, 1},
    {4, 16, 26, 16, 4},
    {7, 26, 41, 26, 7},
    {4, 16, 26, 16, 4},
    {1, 4, 7, 4, 1}};
static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof(height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20)
  {
    do
    {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1)
    {
      head = tail = malloc(sizeof(*tail));
    }
    else
    {
      tail->next = malloc(sizeof(*tail));
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
  while (head)
  {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1])
    {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1])
    {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1])
    {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x])
    {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x])
    {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1])
    {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1])
    {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1])
    {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
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
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      for (s = t = p = 0; p < 5; p++)
      {
        for (q = 0; q < 5; q++)
        {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X)
          {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      for (s = t = p = 0; p < 5; p++)
      {
        for (q = 0; q < 5; q++)
        {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X)
          {
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
  do
  {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]) == ter_path) &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path)) ||
         ((mapxy(p[dim_x] + 2, p[dim_y]) == ter_path) &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path)) ||
         ((mapxy(p[dim_x], p[dim_y] - 1) == ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path)) ||
         ((mapxy(p[dim_x], p[dim_y] + 2) == ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path))) &&
        (((mapxy(p[dim_x], p[dim_y]) != ter_mart) &&
          (mapxy(p[dim_x], p[dim_y]) != ter_center) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_mart) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_center) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_mart) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_center) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x], p[dim_y]) != ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_path) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path))))
    {
      break;
    }
  } while (1);
}

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x], p[dim_y]) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]) = ter_mart;
  mapxy(p[dim_x], p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x], p[dim_y]) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]) = ter_center;
  mapxy(p[dim_x], p[dim_y] + 1) = ter_center;
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
  maxy = y + 1 <= MAP_Y ? y + 1 : MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1 : MAP_X;

  for (q = miny; q < maxy; q++)
  {
    for (p = minx; p < maxx; p++)
    {
      if (q != y || p != x)
      {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder)
        {
          r++;
        }
        else if (m->map[q][p] == ter_forest ||
                 m->map[q][p] == ter_tree)
        {
          t++;
        }
      }
    }
  }

  if (t == r)
  {
    return rand() & 1 ? ter_boulder : ter_tree;
  }
  else if (t > r)
  {
    if (rand() % 10)
    {
      return ter_tree;
    }
    else
    {
      return ter_boulder;
    }
  }
  else
  {
    if (rand() % 10)
    {
      return ter_boulder;
    }
    else
    {
      return ter_tree;
    }
  }
}

static int map_terrain(map_t *m, uint8_t n, uint8_t s, uint8_t e, uint8_t w)
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

  memset(&m->map, 0, sizeof(m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++)
  {
    do
    {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0)
    {
      type = ter_grass;
    }
    else if (i == num_grass)
    {
      type = ter_clearing;
    }
    else if (i == num_grass + num_clearing)
    {
      type = ter_mountain;
    }
    else if (i == num_grass + num_clearing + num_mountain)
    {
      type = ter_forest;
    }
    else if (i == num_grass + num_clearing + num_mountain + num_forest)
    {
      type = ter_water;
    }
    m->map[y][x] = type;
    if (i == 0)
    {
      head = tail = malloc(sizeof(*tail));
    }
    else
    {
      tail->next = malloc(sizeof(*tail));
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
  while (head)
  {
    x = head->x;
    y = head->y;
    i = m->map[y][x];

    if (x - 1 >= 0 && !m->map[y][x - 1])
    {
      if ((rand() % 100) < 80)
      {
        m->map[y][x - 1] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x])
    {
      if ((rand() % 100) < 20)
      {
        m->map[y - 1][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x])
    {
      if ((rand() % 100) < 20)
      {
        m->map[y + 1][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1])
    {
      if ((rand() % 100) < 80)
      {
        m->map[y][x + 1] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
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

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1)
      {
        mapxy(x, y) = type = border_type(m, x, y);
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  mapxy(n, 0) = ter_path;
  mapxy(n, 1) = ter_path;
  mapxy(s, MAP_Y - 1) = ter_path;
  mapxy(s, MAP_Y - 2) = ter_path;
  mapxy(0, w) = ter_path;
  mapxy(1, w) = ter_path;
  mapxy(MAP_X - 1, e) = ter_path;
  mapxy(MAP_X - 2, e) = ter_path;

  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++)
  {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest && m->map[y][x] != ter_path)
    {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++)
  {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path &&
        m->map[y][x] != ter_water)
    {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

static int new_map(map_t *m)
{
  smooth_height(m);
  map_terrain(m,
              1 + rand() % (MAP_X - 2), 1 + rand() % (MAP_X - 2),
              1 + rand() % (MAP_Y - 2), 1 + rand() % (MAP_Y - 2));
  place_boulders(m);
  place_trees(m);
  build_paths(m);
  place_pokemart(m);
  place_center(m);

  return 0;
}

static void print_map(map_t *m)
{
  int x, y;
  int default_reached = 0;
  int i = 0;
  int r = rand() % 30;

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      switch (m->map[y][x])
      {
      case ter_boulder:
      case ter_mountain:
        putchar('%');
        break;
      case ter_tree:
      case ter_forest:
        putchar('^');
        break;
      case ter_path:
        i++;
        if (i == r)
        {
          putchar('@');
          pC.pPos[dim_y] = y;
          pC.pPos[dim_x] = x;
          m->map[y][x] = '@';
          m->map[y][x] = ter_pc;
          break;
        }
        else
        {
          putchar('#');
          break;
        }
      case ter_mart:
        putchar('M');
        break;
      case ter_center:
        putchar('C');
        break;
      case ter_grass:
        putchar(':');
        break;
      case ter_clearing:
        putchar('.');
        break;
      case ter_water:
        putchar('~');
        break;
      case ter_pc:
        putchar('@');
        break;
      default:
        default_reached = 1;
        break;
      }
    }
    printf("\n");
  }

  if (default_reached)
  {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}

/*void dijikstra(map_t *m, npc_t *n[MAP_Y][MAP_X]){

  heap_t h;
  npc_t *nNode, *neigh;
  int tempDistance;
  //heap_node_t hn;
  heap_init(&h, ter_cmp, NULL);

  for(int y = 1; y < MAP_Y; y++){
    for(int x = 1; y < MAP_X; x++){
      if(m->map[y][x] != '@'){
        heap_insert(&h, &n[y][x]);
      }
    }
  }

  while(h.size != 0){
    nNode = heap_remove_min(&h);

    if(nNode[y + 1][x + 1])
      tempDistance = nNode->cost + nNode[y + 1][x + 1].cost;
      if(tempDistance < neigh->cost){
        neigh->cost = tempDistance;
        neigh = nNode;
      }
  }
}*/
int32_t terCost(map_t *m, int yP, int xP, char type)
{
  //printf("%d", m->map[yP][xP]);

  //printf("z");
  switch (m->map[yP][xP])
  {
  case ter_debug:
    break;
  case ter_boulder:
    return (int32_t)10000;
    break;
  case ter_mountain:
    if (type == 'r')
    {

      return (int32_t)10000;
    }
    else if (type == 'h')
    {

      return (int32_t)15;
    }
    break;
  case ter_tree:
    return (int32_t)10000;
    break;
  case ter_forest:
    if (type == 'r')
      return 10000;
    else if (type == 'h')
      return (int32_t)15;
    break;
  case ter_path:
    return (int32_t)10;
    break;
  case ter_mart:
    return (int32_t)50;
    break;
  case ter_center:
    return (int32_t)50;
    break;
  case ter_grass:
    if (type == 'h')
      return (int32_t)15;
    else if (type == 'r')
      return (int32_t)20;
    break;
  case ter_clearing:
    return (int32_t)10;
    break;
  case ter_water:
    return (int32_t)10000;
    break;
  case ter_pc:
    return (int32_t)0;
    break;
  }

  return 0;
}

static void print_npc(map_t *m, char type)
{
  // printf("%c", type);
  static heap_t h;
  static npc_t npc[MAP_Y][MAP_X], *p;
  //heap_delete(&h);
  //static pair_t from;
  // static pair_t from;
  //heap_delete(&h);
  heap_init(&h, path_cmp, NULL);

  // pair_t from, to;
  static uint32_t initialized = 0;

  if (!initialized)
  {
    initialized = 1;
    for (int y = 0; y < MAP_Y; y++)
    {
      for (int x = 0; x < MAP_X; x++)
      {
        npc[y][x].pos[dim_y] = y;
        npc[y][x].pos[dim_x] = x;
      }
    }
  }
  
  for (int y = 0; y < MAP_Y; y++)
  {
    for (int x = 0; x < MAP_X; x++)
    {
      npc[y][x].cost = 10000;
    }
  }

  

  // printf("%d %dPENISINPOIENOIENFE", from[dim_y], from[dim_y]);
  npc[pC.pPos[dim_y]][pC.pPos[dim_x]].cost = 0;

  for (int y = 0; y < MAP_Y; y++)
  {
    for (int x = 0; x < MAP_X; x++)
    {
      npc[y][x].hn = heap_insert(&h, &npc[y][x]); ////#########
    }
  }
  // heap_delete(&h);
  // print_heap(&h, "notes.txt");

  /*from[dim_y] = m->playc.y;
  from[dim_x] = m->playc.x;
  to[dim_x];
  to[dim_y];
  int tempdistance;*/

  while ((p = heap_remove_min(&h)))
  {
    //printf("h");
    p->hn = NULL;
    int32_t cost = terCost(m, p->pos[dim_y], p->pos[dim_x], type);
    //printf("%d", cost);

    if ((npc[p->pos[dim_y] - 1][p->pos[dim_x]].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x]].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x]].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }
    
    ////////////////////////////////////////////////////////////blocp end
    if ((npc[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y]][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }
    if ((npc[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y]][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }
    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x]].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x]].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x]].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }
    if ((npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }
    if ((npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }
    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             cost))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          cost;
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
    }
    
  }

    /*if ((npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost+
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }

    if ((npc[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y]][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }

    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
    }
  
    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }

    if ((npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }

    if ((npc[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&
        (npc[p->pos[dim_y]][p->pos[dim_x] + 1].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }

    if ((npc[p->pos[dim_y] + 1][p->pos[dim_x]].hn) &&
        (npc[p->pos[dim_y] + 1][p->pos[dim_x]].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] + 1][p->pos[dim_x]].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((npc[p->pos[dim_y] - 1][p->pos[dim_x]].hn) &&
        (npc[p->pos[dim_y] - 1][p->pos[dim_x]].cost >
         npc[p->pos[dim_y]][p->pos[dim_x]].cost +
             terCost(m, p->pos[dim_y], p->pos[dim_x], type)))
    {
      npc[p->pos[dim_y] - 1][p->pos[dim_x]].cost =
          npc[p->pos[dim_y]][p->pos[dim_x]].cost +
          terCost(m, p->pos[dim_y], p->pos[dim_x], type);
      heap_decrease_key_no_replace(&h,
                                   npc[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }*/
  

  // printf("/n z");
  heap_delete(&h);

  // dijkstra_path(m, from, to);

  for (int y = 1; y < MAP_Y - 1; y++)
  {
    for (int x = 1; x < MAP_X - 1; x++)
    {
      switch (m->map[y][x])
      {
      case ter_pc:
        printf("@  ");
        break;
      case ter_boulder:
        printf("   ");
      case ter_mountain:
        if (type == 'r')
          printf("   ");
        else if (type == 'h')
          printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_tree:
        printf("   ");
      case ter_forest:
        if (type == 'r')
          printf("   ");
        else if (type == 'h')
          printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_path:
        printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_mart:
        printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_center:
        printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_grass:
        printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_clearing:
        printf("%d ", npc[y][x].cost % 100);
        break;
      case ter_water:
        printf("   ");
        break;

      default:
        break;
      }
    }
    printf("\n");
  }
}

int main(int argc, char *argv[])
{
  map_t d;
  struct timeval tv;
  uint32_t seed;
  char hiker = 'h';
  char rival = 'r';
  if (argc == 2)
  {
    seed = atoi(argv[1]);
  }
  else
  {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  srand(seed);

  new_map(&d);
  print_map(&d);
  print_npc(&d, rival);
  print_npc(&d, hiker);

  // printf("%d, %d \n", d.playc.x, d.playc.y);

  return 0;
}
