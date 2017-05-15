#ifndef TSPLIB_H_INCLUDED
#define TSPLIB_H_INCLUDED

typedef struct {
  char name[256], edge_weight_type[256], edge_weight_format[256];
  int size;
  double **cost_matrix;
  Point *points;
} Map;

void build_map(Map *map, double size);
void destroy_map(Map *map);
Map read_tsp_lib(char *file_name);

#endif