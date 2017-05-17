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

Population alloc_population(int size);
void free_population(Population *population);
Population ga(Map *cities, int pop_size, int elitism, float mutation_rate, int repetitions);

#endif