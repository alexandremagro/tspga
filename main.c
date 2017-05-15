#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <float.h>

#include "point.h"
#include "tsplib.h"
#include "genetic.h"

/* PRINTS */

void plot_tour(Tour *tour) {
  for (int i = 0; i < tour->map->size; i++) {
    printf("%d,%.2f,%.2f ", tour->points[i]->id, tour->points[i]->x, tour->points[i]->y);
  }

  printf("\n");
}

void write_tour(char filename[], Tour *tour) {
  FILE *file = fopen(filename, "w");

  if (file == NULL) {
    printf("Error opening output file!\n");
    exit(1);
  }

  fprintf(file, "NAME: %s\n", tour->map->name);
  fprintf(file, "TYPE: TOUR\n");
  fprintf(file, "DIMENSION: %d\n", tour->map->size);
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
  bool elitism = true;
  int pop_size = 10;
  bool plot = false;
  int repetitions = 200;
  float mutation_rate = 0.02;

  Map map;
  Population best_population;

  time_t start_t, end_t;
  double diff_t;

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
  map = read_tsp_lib(argv[1]);

  // Running Generic Algorith
  time(&start_t);
  best_population = ga(&map, pop_size, elitism, mutation_rate, repetitions);
  time(&end_t);

  diff_t = difftime(end_t, start_t);

  // Plot
  if (plot)
    plot_tour(best_population.fittest);

  // Write
  write_tour(argv[2], best_population.fittest);

  // Finish
  destroy_population(&best_population);
  destroy_map(&map);

  return 0;
}