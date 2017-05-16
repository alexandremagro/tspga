#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "point.h"
#include "tsplib.h"
#include "genetic.h"

// Rand a number from 0 to 1
float rand_prob() {
  return  (float) rand() / (RAND_MAX);
}

// Rand a number from min to (min+len)
int rand_in(int min, int len) {
  int r;
  const int range = len - min;
  const int buckets = RAND_MAX / range;
  const int limit = buckets * range;

  /* Create equal size buckets all in a row, then fire randomly towards
   * the buckets until you land in one of them. All buckets are equally
   * likely. If you land off the end of the line of buckets, try again. */
  do {
    r = rand();
  } while (r >= limit);

  return min + (r / buckets);
}

void swap(void *a, void *b, size_t dataSize) {
  uint8_t t[dataSize];

  memcpy(t, a, dataSize);
  memcpy(a, b, dataSize);
  memcpy(b, t, dataSize);
}

// Aloca um array de Tours e retorna o array
Population alloc_population(int size) {
  Population p;

  p.size  = size;
  p.tours = malloc(size * sizeof(Tour));;

  return p;
}

void free_population(Population *population) {
  for (int i = 0; i < population->size; i++) {
    free(population->tours[i].points);
  }

  free(population->tours);
}

Tour copy_tour(Tour *src) {
  Tour dest;

  dest.points   = malloc(src->map->size * sizeof(Point));
  dest.map      = src->map;
  dest.distance = src->distance;
  dest.fitness  = src->fitness;

  for (int i = 0; i < src->map->size; i++)
    dest.points[i] = src->points[i];

  return dest;
}

void calc_tour(Tour *tour) {
  double distance = 0;

  for (int i = 1; i < tour->map->size; i++) {
    distance += tour->map->cost_matrix[tour->points[i]->id - 1][tour->points[i - 1]->id - 1];
  }

  tour->distance = distance;
  tour->fitness  = 1 / distance;
}

void calc_population(Population *population) {
  double highest_fitness = 0;

  for (int i = 0; i < population->size; i++) {
    if (population->tours[i].fitness > highest_fitness) {
      highest_fitness = population->tours[i].fitness;
      population->fittest = &population->tours[i];
    }
  }
}

// Gera um Tour para um objeto Map, calcula os valores e o retorna
Tour generate_random_tour(Map *map) {
  Tour tour;
  tour.points = malloc(map->size * sizeof(Point*));
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

/* SELECTIONS */

// Population should have length greater than one.
void roulette_wheel(Population *population, Population *selecteds) {
  int num_of_selected = 0;
  int *selected_indexes = calloc(selecteds->size, sizeof(int));

  double *normalized = calloc(population->size, sizeof(double));
  double sum   = 0;
  float acum   = 0;
  float choose = 0;

  if (population->size == 1) {
    selecteds->tours[0] = population->tours[0];
    return;
  }

  while (num_of_selected < selecteds->size) {

    // Calcula o total do fitness (sum), desconsiderando tours já selecionados
    for (int i = 0; i < population->size; i++) {
      bool already = false;

      for (int j = 0; j < num_of_selected; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }

      if (!already) {
        sum += population->tours[i].fitness;
      }
    }

    // Normalize probability
    for (int i = 0; i < population->size; i++) {
      bool already = false;

      for (int j = 0; j < num_of_selected; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }


      if (!already) {
        normalized[i] = (population->tours[i].fitness / sum) + acum;
        acum += normalized[i];
      }
    }

    // Roll the whell
    choose = rand_prob();

    // Get individual who normalized fitness is lower than magic choosed number.
    for (int i = 0; i < population->size; i++) {
      bool already = false;

      for (int j = 0; j < num_of_selected; j++) {
        if (selected_indexes[j] == i) {
          already = true;
          break;
        }
      }

      if (!already && choose < normalized[i]) {
        selecteds->tours[num_of_selected] = population->tours[i];
        num_of_selected++;
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

  new_tour.points = malloc(map_size * sizeof(Point*));
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

/* GENETIC ALGORITHM */

Population ga(Map *map, int pop_size, int elitism, float mutation_rate, int repetitions) {
  // Populacao Base (0) e População Evoluida (1)
  Population population_0 = alloc_population(pop_size);
  Population parents      = alloc_population(2);

  // Ending condition vars
  int repeated_fitness = 0;
  int cicles = 0;

  // Generated fist generation
  for (int i = 0; i < pop_size; i++)
    population_0.tours[i] = generate_random_tour(map);

  calc_population(&population_0);

  do {
    Population population_n = alloc_population(pop_size);

    // Elitism logic
    int i = 0;

    if (elitism) {
      population_n.tours[0] = copy_tour(population_0.fittest);
      i++;
    }
    for (; i < pop_size; i++) {
      // 2. Evaluation and Selections

      roulette_wheel(&population_0, &parents);

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

    free_population(&population_n);

    cicles++;

  } while (repeated_fitness < repetitions);

  free(parents.tours);

  return population_0;
}