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

void write_tour(char filename[], Tour *tour, double diff_t) {
  FILE *file = fopen(filename, "w");

  if (file == NULL) {
    printf("Error opening output file!\n");
    exit(1);
  }

  fprintf(file, "NAME: %s\n", tour->map->name);
  fprintf(file, "TYPE: TOUR\n");
  fprintf(file, "DIMENSION: %d\n", tour->map->size);
  fprintf(file, "DISTANCE: %.2f\n", tour->distance);
  fprintf(file, "TIME: %f\n", diff_t);
  fprintf(file, "TOUR_SECTION\n");

  for (int i = 0; i < tour->map->size; i++) {
    fprintf(file, "%d\n", tour->points[i]->id);
  }

  fprintf(file, "EOF");
  fclose(file);
}

/* MAIN */

int main(int argc, char *argv[]) {
  bool output = false;
  bool freeze = false;
  bool elitism = true;
  int pop_size = 16;
  bool plot = false;
  int repetitions = 200;
  float mutation_rate = 0.02;
  char *output_filename;

  Map map;
  Population best_population;

  clock_t start_t, end_t;
  double diff_t;

  // init
  if (argc < 2) {
    printf("USE: %s <input filename> [OPTIONS]\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  %-18s %s\n", "-o, --output", "default output to TSP Lib");
    printf("  %-18s %s\n", "-p, --plot", "plot an array of ID,X,Y of the best tour");
    printf("  %-18s %s\n", "-m, --mutation", "Set mutation rate [0.02]");
    printf("  %-18s %s\n", "-s, --size", "Set population size [16]");
    printf("  %-18s %s\n", "-r, --repetitions", "How many times repeat the best way? [200]");
    printf("  %-18s %s\n", "-f, --freeze", "Deny seed random");
    exit(-1);
  }

  // reading optional params
  if (argc > 2) {
    int argument_param = 0;

    for (int i = 0; i < argc; i++) {
      // param arguments

      if (argument_param == 1) {
        output = true;
        output_filename = malloc(strlen(argv[i]) + 1);
        strcpy(output_filename, argv[i]);

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

      if (argument_param == 4) {
        sscanf(argv[i], "%d", &repetitions);
        argument_param = 0;
        continue;
      }

      // params

      if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
        output = true;
        argument_param = 1;
        continue;
      }

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
        argument_param = 4;
        continue;
      }

      if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--freeze")) {
        freeze = true;
      }
    }
  }

  if (!freeze)
    srand(time(NULL));

  // Reading TspLib
  map = read_tsp_lib(argv[1]);

  // Running Generic Algorith
  start_t = clock();
  best_population = ga(&map, pop_size, elitism, mutation_rate, repetitions);
  end_t = clock();

  diff_t = (double) (end_t - start_t) / CLOCKS_PER_SEC;

  // Plot
  if (plot)
    plot_tour(best_population.fittest);

  // Write
  if (output) {
    write_tour(output_filename, best_population.fittest, diff_t);
    free(output_filename);
  }

  // Finish
  free_population(&best_population);
  free_map(&map);

  return 0;
}