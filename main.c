#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <float.h>

typedef struct {
  int id;
  float x, y;
} Point;

typedef struct {
  int size;
  double **cost_matrix;
  Point *points;
} Map;

typedef struct {
  double distance, fitness;
  Map *map;
  Point **points;
} Tour;

typedef struct {
  int size;
  Tour *tours;
  Tour *fittest;
} Population;

typedef struct {
  int dimension;
  char name[256], edge_weight_type[256], edge_weight_format[256];
  Map map;
} TSPInstance;

/* UTILS */

// Rand a number from 0 to 1
float rand_prob() {
  return  (float) rand() / (RAND_MAX);
}

// Rand a number from min to (min+len)
int rand_in(int min, int len) {
  return min + (rand() % len);
}

void swap(void *a, void *b, size_t dataSize) {
  void *t = malloc(dataSize);

  memcpy(t, a, dataSize);
  memcpy(a, b, dataSize);
  memcpy(b, t, dataSize);
  free(t);
}

/* PRINTS */

void plot_tour(Tour *tour) {
  for (int i = 0; i < tour->map->size; i++) {
    printf("%d,%.2f,%.2f ", tour->points[i]->id, tour->points[i]->x, tour->points[i]->y);
  }

  printf("\n");
}

/* EVALUATORS */

void calc_tour(Tour *tour) {
  double distance = 0;

  for (int i = 1; i < tour->map->size; i++) {
    distance += tour->map->cost_matrix[tour->points[i]->id - 1][tour->points[i - 1]->id - 1];
  }

  tour->distance = distance;
  tour->fitness  = 1 / distance;
}

void calc_population(Population *population) {
  Tour tour;
  double best_val = 0;
  int    best_pos = 0;

  for (int i = 0; i < population->size; i++) {
    if (population->tours[i].fitness > best_val) {
      best_val = population->tours[i].fitness;
      best_pos = i;
    }
  }

  population->fittest = &population->tours[best_pos];
}

/* CONSTRUCTORS */

// Aloca um array de Tours e retorna o array
Population build_population(int size) {
  Tour *tours = (Tour*) malloc(size * sizeof(Tour));
  Population p;

  p.size = size;
  p.tours = tours;

  return p;
}

// Aloca uma matriz de double
Map build_map(double size) {
  Map map;
  Point *points;
  double** matrix;

  points = (Point*) malloc(size * sizeof(Point));
  matrix = (double**) malloc(size * sizeof(double*));

  for (int i = 0; i < size; ++i) {
    matrix[i] = (double*) malloc(size * sizeof(double));
  }

  map.points = points;
  map.cost_matrix = matrix;
  map.size = size;

  return map;
}

// Gera um Tour para um objeto Map, calcula os valores e o retorna
Tour generate_random_tour(Map *map) {
  Tour tour;

  tour.points = (Point**) malloc(map->size * sizeof(Point*));
  tour.map    = map;

  // Copy populate
  for (int i = 0; i < map->size; i++) {
    tour.points[i] = &map->points[i];
  }

  // Randomiza um Tour (Fisher-Yates shuffle based)
  int j;
  for (int i = map->size-1; i > 0; i--) {
    // Pick a random index from 0 to i
    j = rand_in(0, i);
    swap(&tour.points[i], &tour.points[j], sizeof(Point*));
  }

  calc_tour(&tour);

  return tour;
}

Tour copy_tour(Tour *src) {
  Tour dest;

  dest.points = (Point**) malloc(src->map->size * sizeof(Point));
  dest.map = src->map;

  for (int i = 0; i < src->map->size; i++) {
    dest.points[i] = src->points[i];
  }

  calc_tour(&dest);

  return dest;
}

void destroy_map(Map *map) {
  for (int i = 0; i < map->size; i++) {
    free(map->cost_matrix[i]);
  }

  free(map->cost_matrix);
  free(map->points);
}

void destroy_tour(Tour *tour) {
  free(tour->points);
}

void destroy_population_tours(Population *population) {
  for (int i = 0; i < population->size; i++) {
    destroy_tour(&population->tours[i]);
  }
}

void destroy_population(Population *population) {
  free(population->tours);
}

/* SELECTIONS */

// Population should have length greater than one.
void roulette_wheel(Population population, Population *selecteds) {
  int selected_num = 0;
  int *selected_indexes = (int*) calloc(selecteds->size, sizeof(int));

  double *normalized = (double*) calloc(population.size, sizeof(double));
  double sum   = 0;
  float acum   = 0;
  float choose = 0;

  if (population.size == 1) {
    selecteds->tours[0] = population.tours[0];
    return;
  }

  while (selected_num < selecteds->size) {

    // Calculate total to probability
    for (int i=0; i<population.size; i++) {
      bool already = false;

      for (int j=0; j<selected_num; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }

      if (!already) {
        sum += population.tours[i].fitness;
      }
    }

    // Normalize probability
    for (int i=0; i<population.size; i++) {
      int already = false;

      for (int j=0; j<selected_num; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }

      if (!already) {
        normalized[i] = (population.tours[i].fitness / sum) + acum;
        acum += normalized[i];
      }
    }

    // Roll the whell
    choose = rand_prob();

    // Get individual who normalized fitness is lower than magic choosed number.
    for (int i=0; i<population.size; i++) {
      int already = false;

      for (int j=0; j<selected_num; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }

      if (!already && choose < normalized[i]) {
        selecteds->tours[selected_num] = population.tours[i];
        selected_num++;
        break;
      }
    }
  }

  // Free malloc
  free(normalized);
  free(selected_indexes);
}

/* CROSSOVER */

// Parent1 and Parent2 should to be the same length
// Parent1 and Parent1 should have length greater than zero.
Tour crossover(Tour parent1, Tour parent2, float mutation_rate) {
  if (parent1.map != parent2.map)
    exit(-1);

  int map_size = parent1.map->size;
  Tour new_tour;

  new_tour.points = (Point**) malloc(map_size * sizeof(Point*));
  new_tour.map = parent1.map;

  int start = rand_in(0, map_size);
  int end   = rand_in(0, map_size);

  // Se os pais tiverem mais de 3 cromossomos, selecionar um intervalo diferente de 1 e TUDO...
  if (map_size > 3) {
    while (fabs(end - start) < 2 || fabs(end - start) == map_size - 1) {
      end = rand_in(0, map_size);
    }
  }

  if (start > end) { swap(&start, &end, sizeof(int)); }

  // seleciona cromossomos do parent1
  for (int i = start; i < end; i++) {
    new_tour.points[i] = parent1.points[i];
  }

  // completa com os cromossomos do parent2
  int j_stop = 0;
  for (int i = 0; i < map_size; i++) {

    if (i >= start && i < end) {
      continue;
    } else {
      for (int j = j_stop; j < map_size; j++) {
        bool already = false;

        for (int z = start; z < end; z++) {
          if (parent2.points[j]->id == new_tour.points[z]->id) {
            already = true;
            break;
          }
        }

        if (!already) {
          new_tour.points[i] = parent2.points[j];
          j_stop = j + 1;
          break;
        }
      }
    }
  }

  // Mutation
  if (rand_prob() < mutation_rate) {
    int gene1 = 0, gene2 = 0;

    while (gene1 == gene2) {
      gene1 = rand_in(0, map_size);
      gene2 = rand_in(0, map_size);
    }

    swap(&new_tour.points[gene1], &new_tour.points[gene2], sizeof(Point*));
  }

  calc_tour(&new_tour);

  return new_tour;
}

/* INPUT */

TSPInstance read_tsp_lib(char *file_name) {
  FILE *file = fopen(file_name, "r");
  TSPInstance instance;
  Map map;

  int dimension;
  char line[256];

  // Verifica existencia do arquivo
  if (!file) {
    perror("File not found!\n");
    exit(-1);
  }

  // Obtém o nome da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "NAME%*[ :] %s\n", instance.name) == 1)
      break;
  }

  // Obtém a dimensão da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "DIMENSION%*[ :] %d\n", &instance.dimension) == 1)
      break;
  }

  // obtém o tipo da instância
  while (true) {
    fgets(line, 255, file);

    if (sscanf(line, "EDGE_WEIGHT_TYPE%*[ :] %s\n", instance.edge_weight_type) == 1)
      break;
  }

  if (!strcmp(instance.edge_weight_type, "EXPLICIT")) {
    fgets(line, 255, file);
    sscanf(line, "EDGE_WEIGHT_FORMAT%*[ :] %s\n", instance.edge_weight_format);
  }

  // construindo a matrix com a dimensão informada

  map = build_map(instance.dimension);

  // Se a matriz de custo NÃO estiver explicita...
  if (strcmp(instance.edge_weight_type, "EXPLICIT")) {

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

    if (!strcmp(instance.edge_weight_type, "EUC_2D")) {
      float xd, yd;

      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j < map.size; ++j) {
          xd = map.points[i].x - map.points[j].x;
          yd = map.points[i].y - map.points[j].y;

          map.cost_matrix[i][j] = (int) (sqrt(xd * xd + yd * yd) + 0.5);
        }
      }
    }

    else if (!strcmp(instance.edge_weight_type, "GEO")) {
      int deg;
      float *lat, *lon, min, q1, q2, q3;
      static double RRR = 6378.388;

      lat = (float*) malloc(map.size * sizeof(float));
      lon = (float*) malloc(map.size * sizeof(float));

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

    else if (!strcmp(instance.edge_weight_type, "ATT")) {
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
    else if (!strcmp(instance.edge_weight_type, "CEIL_2D")) {
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

    if (!strcmp(instance.edge_weight_format, "FULL_MATRIX")) {
      for(int i = 0; i < map.size; ++i)
        for(int j = 0; j < map.size; ++j)
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
    }

    else if (!strcmp(instance.edge_weight_format, "UPPER_ROW")) {
      for (int i = 0; i < map.size; ++i) {
        for(int j = i + 1; j < map.size; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }

        map.cost_matrix[i][i] = 0;
      }
    }

    else if(!strcmp(instance.edge_weight_format, "UPPER_DIAG_ROW")) {
      for(int i = 0; i < map.size; ++i) {
        for(int j = i; j < map.size; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }
      }
    }

    else if(!strcmp(instance.edge_weight_format, "LOWER_DIAG_ROW")) {
      for (int i = 0; i < map.size; ++i) {
        for (int j = 0; j <= i; ++j) {
          fscanf(file, "%lf", &map.cost_matrix[i][j]);
          map.cost_matrix[j][i] = map.cost_matrix[i][j];
        }
      }
    }
  }

  fclose(file);

  instance.map = map;

  return instance;
}

/* GENETIC ALGORITHM */

Population ga(Map cities, int pop_size, int elitism, float mutation_rate, int repetitions) {
  // Populacao Base (0) e População Evoluida (1)
  Population population_0 = build_population(pop_size);
  Population population_n = build_population(pop_size);
  Population parents      = build_population(2);

  // Ending condition vars
  int repeated_fitness = 0;
  int cicles = 0;

  // Generated fist generation
  for (int i = 0; i < pop_size; i++)
    population_0.tours[i] = generate_random_tour(&cities);

  calc_population(&population_0);

  // Save initial distance
  double initial_distance = population_0.fittest->distance;

  do {
    // Elitism logic
    int offset = 0;

    if (elitism) {
      population_n.tours[0] = copy_tour(population_0.fittest);

      offset = 1;
    }

    for (int i = offset; i < pop_size; i++) {
      // 2. Evaluation and Selections
      roulette_wheel(population_0, &parents);

      // 3. Evolve: Crossover and Mutation
      population_n.tours[i] = crossover(parents.tours[0], parents.tours[1], mutation_rate);
    }

    calc_population(&population_n);

    if (population_0.fittest->fitness != population_n.fittest->fitness)
      repeated_fitness = 0;
    else
      repeated_fitness++;

    // 4. Repeat
    swap(&population_0, &population_n, sizeof(Population));

    destroy_population_tours(&population_n);

    cicles++;

  } while (repeated_fitness < repetitions);

  destroy_population(&population_n);
  destroy_population(&parents);

  return population_0;
}

void write_tour(char filename[], TSPInstance *instance, Tour *tour) {
  FILE *file = fopen(filename, "w");

  if (file == NULL) {
    printf("Error opening output file!\n");
    exit(1);
  }

  fprintf(file, "NAME: %s\n", instance->name);
  fprintf(file, "TYPE: TOUR\n");
  fprintf(file, "DIMENSION: %d\n", instance->dimension);
  fprintf(file, "DISTANCE: %.2f\n", tour->distance);
  fprintf(file, "TOUR_SECTION\n");

  for (int i = 0; i < tour->map->size; i++) {
    fprintf(file, "%d\n", tour->points[i]->id);
  }

  fprintf(file, "EOF");
  fclose(file);
}

/* MAIN */

int main(int argc, char *argv[]) {
  int elitism = true;
  int pop_size = 10;
  int plot = false;
  int repetitions = 200;
  float mutation_rate = 0.02;

  TSPInstance instance;
  Population best_population;

  // init
  if (argc < 3) {
    printf("USE: %s <input filename> <output filename> [OPTIONS]\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  %-18s %s\n", "-p, --plot", "plot an array of ID,X,Y of the best tour");
    printf("  %-18s %s\n", "-m, --mutation", "Set mutation rate [0.02]");
    printf("  %-18s %s\n", "-s, --size", "Set population size [10]");
    printf("  %-18s %s\n", "-r, --repetitions", "How many times repeat the best way? [200]");
    exit(-1);
  }

  // reading optional params
  if (argc > 3) {
    int argument_param = 0;

    for (int i = 0; i < argc; i++) {
      // param arguments

      if (argument_param == 1) {
        sscanf(argv[i], "%d", &repetitions);
        argument_param = 0;
        continue;
      }

      if (argument_param == 2) {
        sscanf(argv[i], "%f", &mutation_rate);
        argument_param = 0;
        continue;
      }

      if (argument_param == 3) {
        sscanf(argv[i], "%d", &pop_size);
        argument_param = 0;
        continue;
      }

      // params

      if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--plot")) {
        plot = true;
      }

      if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mutation")) {
        argument_param = 2;
        continue;
      }

      if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--size")) {
        argument_param = 3;
        continue;
      }

      if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--repetitions")) {
        argument_param = 1;
        continue;
      }
    }
  }

  // Reading TspLib
  instance = read_tsp_lib(argv[1]);

  // Running Generic Algorith
  best_population = ga(instance.map, pop_size, elitism, mutation_rate, repetitions);

  // Plot
  if (plot)
    plot_tour(best_population.fittest);

  // Write
  write_tour(argv[2], &instance, best_population.fittest);

  // Finish
  destroy_population_tours(&best_population);
  destroy_population(&best_population);
  destroy_map(&instance.map);

  return 0;
}