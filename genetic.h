#ifndef GENETIC_H_INCLUDED
#define GENETIC_H_INCLUDED

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

float rand_prob();
int rand_in(int min, int max);
void swap(void *a, void *b, size_t dataSize);
Population build_population(int size);
void destroy_population(Population *population);
Tour copy_tour(Tour *src);
Tour generate_random_tour(Map *map);
void calc_tour(Tour *tour);
void calc_population(Population *population);
void roulette_wheel(Population *population, Population *selecteds);
Tour crossover(Tour parent1, Tour parent2, float mutation_rate);
Population ga(Map *cities, int pop_size, int elitism, float mutation_rate, int repetitions);

#endif