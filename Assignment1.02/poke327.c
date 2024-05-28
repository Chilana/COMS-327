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
#include<stdbool.h>

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
#define BOULDER_COST 10000
#define TREE_COST 10000
#define PATH_COST 10
#define rPMART_COST 10
#define rPCNTR_COST 50
#define rSGRASS_COST 10

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

typedef struct map
{
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  uint8_t n, s, e, w;
} map_t;

typedef struct queue_node
{
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int32_t path_cmp(const void *key, const void *with)
{
  return ((path_t *)key)->cost - ((path_t *)with)->cost;
}

int c
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
  int pcX, pcY;

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
  int placed = 0;
  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;
    int randTime = rand() % 20;
    int i = 0;
    
   if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x])
    {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y])
      {
        mapxy(x, y) = ter_path;
        heightxy(x, y) = 0;
        if(i == randTime && placed == 0){
          mapxy(x, y) = ter_pc;
          pcX = x;
          pcY = y;
          placed += 1;
        }
        i++;
      }
      heap_delete(&h);
      return;
    }
    // add to the poroity queue if adjecnt to the point im at if the map coordinate(q.contains) is porcessed don't add.
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
  
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof(height));

  
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

static int new_map(map_t *m, uint8_t n, uint8_t s, uint8_t e, uint8_t w, int x, int y)
{
  int manhattan_dist, calc;
  smooth_height(m);
  map_terrain(m, n, s, e, w);
  place_boulders(m);
  place_trees(m);
  build_paths(m);
  if (x == 0)
  {
    for (int i = 0; i < MAP_Y; i++)
    {
      m->map[i][0] = ter_boulder;
    }
  }
  if (x == 400)
  {
    for (int i = 0; i < MAP_Y; i++)
    {
      m->map[i][MAP_X - 1] = ter_boulder;
    }
  }
  if (y == 0)
  {
    for (int i = 0; i < MAP_X; i++)
    {
      m->map[0][i] = ter_boulder;
    }
  }
  if (y == 400)
  {
    for (int i = 0; i < MAP_X; i++)
    {
      m->map[MAP_Y - 1][i] = ter_boulder;
    }
  }
  manhattan_dist = abs(x - 200) + abs(x - 200);
  if (x == 200 && y == 200)
  {
    place_pokemart(m);
    place_center(m);
  }
  else if (manhattan_dist < 200)
  {
    calc = ((-45 * manhattan_dist) / 200 + 50) / 100;
    if (1 + rand() % 100 <= calc)
    {
      place_pokemart(m);
    }
    if (1 + rand() % 100 <= calc)
    {
      place_center(m);
    }
  }
  else
  {
    if (1 + rand() % 100 <= 5)
    {
      place_pokemart(m);
    }
    if (1 + rand() % 100 <= 5)
    {
      place_pokemart(m);
    }
  }

  return 0;
}

static void print_map(map_t *m)
{
  int x, y;
  int default_reached = 0;

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
        putchar('#');
        break;
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
    putchar('\n');
  }

  if (default_reached)
  {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}

int main(int argc, char *argv[])
{
  map_t *p[401][401];
  map_t d;
  struct timeval tv;
  uint32_t seed;
  char input;
  int curr_x, curr_y, j, i;
  int fly_x = 0;
  int fly_y = 0;

  if (argc == 2)
  {
    seed = atoi(argv[1]);
  }
  else
  {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  for (j = 0; j < 401; ++j)
  {
    for (i = 0; i < 401; ++i)
    {
      p[j][i] = NULL;
    }
  }

  printf("Using seed: %u\n", seed);
  srand(seed);

  new_map(&d, 1 + rand() % (MAP_X - 2), 1 + rand() % (MAP_X - 2), 1 + rand() % (MAP_Y - 2), 1 + rand() % (MAP_Y - 2), 200, 200);
  print_map(&d);
  curr_x = curr_y = 200;
  p[curr_x][curr_y] = &d;

  while (1)
  {
    printf("Currently at (%d, %d) Enter move command: ", (curr_x - 200), (curr_y - 200) * -1);
  
    scanf(" %c", &input);
    if (input == 'q')
    {
      break;
    }
    map_t *m;
    if (input == 'n' && curr_y != 0)
    {
      curr_y--;
      if (!p[curr_x][curr_y])
      {
        m = malloc(sizeof(map_t));
        new_map(m, (curr_y == 0 || !p[curr_x][curr_y - 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y - 1]->s,
                p[curr_x][curr_y + 1]->n,
                (curr_x == 400 || !p[curr_x + 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x + 1][curr_y]->w,
                (curr_x == 0 || !p[curr_x - 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x - 1][curr_y]->e, curr_x, curr_y);
        p[curr_x][curr_y] = m;
      }
      else
      {
        m = p[curr_x][curr_y];
      }
    }
    else if (input == 's' && curr_y != 400)
    {
      curr_y++;
      if (!p[curr_x][curr_y])
      {
        m = malloc(sizeof(map_t));
        new_map(m, p[curr_x][curr_y - 1]->s,
                (curr_y == 400 || !p[curr_x][curr_y + 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y + 1]->n,
                (curr_x == 400 || !p[curr_x + 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x + 1][curr_y]->w,
                (curr_x == 0 || !p[curr_x - 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x - 1][curr_y]->e, curr_x, curr_y);
        p[curr_x][curr_y] = m;
      }
      else
      {
        m = p[curr_x][curr_y];
      }
    }
    else if (input == 'e' && curr_x != 400)
    {
      curr_x++;
      if (!p[curr_x][curr_y])
      {
        m = malloc(sizeof(map_t));
        new_map(m, (curr_y == 0 || !p[curr_x][curr_y - 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y - 1]->s,
                (curr_y == 400 || !p[curr_x][curr_y + 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y + 1]->n,
                (curr_x == 400 || !p[curr_x + 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x + 1][curr_y]->w,
                p[curr_x - 1][curr_y]->e, curr_x, curr_y);
        p[curr_x][curr_y] = m;
      }
      else
      {
        m = p[curr_x][curr_y];
      }
    }
    else if (input == 'w' && curr_x != 0)
    {
      curr_x--;
      if (!p[curr_x][curr_y])
      {
        m = malloc(sizeof(map_t));
        new_map(m, (curr_y == 0 || !p[curr_x][curr_y - 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y - 1]->s,
                (curr_y == 400 || !p[curr_x][curr_y + 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y + 1]->n,
                p[curr_x + 1][curr_y]->w,
                (curr_x == 0 || !p[curr_x - 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x - 1][curr_y]->e, curr_x, curr_y);
        p[curr_x][curr_y] = m;
      }
      else
      {
        m = p[curr_x][curr_y];
      }
    }
    else if (input == 'f')
    {
      if (scanf(" %d %d", &fly_x, &fly_y) == 2)
      {
        if (fly_x >= -200 && fly_x <= 200 && fly_y >= -200 && fly_y <= 200)
        {
          curr_x = fly_x + 200;
          curr_y = ((-1 * fly_y) + 200);
          if (!p[curr_x][curr_y])
          {

            m = malloc(sizeof(map_t));

            new_map(m, (curr_y == 0 || !p[curr_x][curr_y - 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y - 1]->s,
                    (curr_y == 400 || !p[curr_x][curr_y + 1]) ? 1 + rand() % (MAP_X - 2) : p[curr_x][curr_y + 1]->n,
                    (curr_x == 400 || !p[curr_x + 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x + 1][curr_y]->e,
                    (curr_x == 0 || !p[curr_x - 1][curr_y]) ? 1 + rand() % (MAP_Y - 2) : p[curr_x - 1][curr_y]->w, curr_x, curr_y);

            p[curr_x][curr_y] = m;
          }
          else
          {
            m = p[curr_x][curr_y];
          }
        }
      }
    }
    if(input == 'q' || input == 'f' || input == 's' || input == 'n' || input == 'e' || input == 'w')
       print_map(m);
    else{
      printf("Please enter a vaild character\n");
    }
  }

  

  for (j = 0; j < 401; j++)
  {
    for (i = 0; i < 401; i++)
    {
      if (!p[j][i] || (j == 200 && i == 200))
      {
        continue;
      }
      free(p[j][i]);
    }
  }

  return 0;
}
