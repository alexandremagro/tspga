#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "point.h"
#include "tsplib.h"

void destroy_map(Map *map) {
  for (int i = 0; i < map->size; i++) {
    free(map->cost_matrix[i]);
  }

  free(map->cost_matrix);
  free(map->points);
}

Map read_tsp_lib(char *file_name) {
  FILE *file = fopen(file_name, "r");
  Map map;
  char line[256];

  // Verifica existencia do arquivo
  if (!file) {
    perror("File not found!\n");
    exit(-1);
  }

  // Obtém o nome da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "NAME%*[ :] %s\n", map.name) == 1)
      break;
  }

  // Obtém a dimensão da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "DIMENSION%*[ :] %d\n", &map.size) == 1)
      break;
  }

  // obtém o tipo da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "EDGE_WEIGHT_TYPE%*[ :] %s\n", map.edge_weight_type) == 1)
      break;
  }

  if (!strcmp(map.edge_weight_type, "EXPLICIT")) {
    fgets(line, 255, file);
    sscanf(line, "EDGE_WEIGHT_FORMAT%*[ :] %s\n", map.edge_weight_format);
  }

  // alocando os pontos e a matriz de custo

  map.points = malloc(map.size * sizeof(Point));
  map.cost_matrix = malloc(map.size * sizeof(double*));

  for (int i = 0; i < map.size; ++i)
    map.cost_matrix[i] = malloc(map.size * sizeof(double));

  // Se a matriz de custo NÃO estiver explicita...
  if (strcmp(map.edge_weight_type, "EXPLICIT")) {

    // pula para a sessão de dados...
    while (true) {
      fgets(line, 255, file);

      if (!strcmp(line, "NODE_COORD_SECTION\n"))
        break;
    }

    for (int i = 0; i < map.size; ++i) {
      Point point;

      fgets(line, 255, file);
      sscanf(line, "%d %f %f" , &point.id, &point.x, &point.y);

      map.points[i] = point;
    }

    if (!strcmp(map.edge_weight_type, "EUC_2D")) {
      float xd, yd;

      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j < map.size; ++j) {
          xd = map.points[i].x - map.points[j].x;
          yd = map.points[i].y - map.points[j].y;

          map.cost_matrix[i][j] = (int) (sqrt(xd * xd + yd * yd) + 0.5);
        }
      }
    }

    else if (!strcmp(map.edge_weight_type, "GEO")) {
      int deg;
      float *lat, *lon, min, q1, q2, q3;
      static double RRR = 6378.388;

      lat = malloc(map.size * sizeof(float));
      lon = malloc(map.size * sizeof(float));

      for(int i = 0; i < map.size; ++i) {
        deg = (int) map.points[i].x;
        min = map.points[i].x - deg;
        lat[i] = M_PI * (deg + 5.0 * min / 3.0) / 180.0;

        deg = (int) map.points[i].y;
        min = map.points[i].y - deg;
        lon[i] = M_PI * (deg + 5.0 * min / 3.0) / 180.0;
      }

      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j < map.cost_matrix[i][j]; ++j) {
          q1 = cos(lon[i] - lon[j]);
          q2 = cos(lat[i] - lat[j]);
          q3 = cos(lat[i] + lat[j]);

          map.cost_matrix[i][j] = (int) RRR * acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 1.0;
        }
      }

      free(lat);
      free(lon);
    }

    else if (!strcmp(map.edge_weight_type, "ATT")) {
      float xd, yd, rij;
      int tij;

      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j < map.size; ++j) {
          xd = map.points[i].x - map.points[j].x;
          yd = map.points[i].y - map.points[j].y;

          rij = sqrt((xd * xd + yd * yd) / 10.0);
          tij = (int) rij;

          map.cost_matrix[i][j] = (tij < rij) ? tij + 1 : tij;
        }
      }
    }
    else if (!strcmp(map.edge_weight_type, "CEIL_2D")) {
      float xd, yd;

      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j < map.size; ++j) {
          xd = map.points[i].x - map.points[j].x;
          yd = map.points[i].y - map.points[j].y;

          map.cost_matrix[i][j] = (int) ceil(sqrt(xd * xd + yd * yd) + 0.5);
        }
      }
    }
  }

  // Se a matriz de custo estiver explicita...
  else {
    while (true) {
      fgets(line, 256, file);

      // Não entendi
      // if (sscanf(linha, "%s", lixo) == EOF)
      //  return;

      if (!strcmp(line, "EDGE_WEIGHT_SECTION\n"))
        break;
    }

    if (!strcmp(map.edge_weight_format, "FULL_MATRIX")) {
      for(int i = 0; i < map.size; ++i)
        for(int j = 0; j < map.size; ++j)
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
    }

    else if (!strcmp(map.edge_weight_format, "UPPER_ROW")) {
      for (int i = 0; i < map.size; ++i) {
        for(int j = i + 1; j < map.size; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }

        map.cost_matrix[i][i] = 0;
      }
    }

    else if(!strcmp(map.edge_weight_format, "UPPER_DIAG_ROW")) {
      for(int i = 0; i < map.size; ++i) {
        for(int j = i; j < map.size; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }
      }
    }

    else if(!strcmp(map.edge_weight_format, "LOWER_DIAG_ROW")) {
      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j <= i; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }
      }
    }
  }

  fclose(file);

  return map;
}