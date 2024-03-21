# Ant Optimization

This is an implementation of the Max-Min-Ant-Optimization as described by [1].

It implements a Max-Min Ant System and applies it to a sequential-ordering problem.


## Findings

Testing during development was done on the [ESC25](problems/ESC25.sop)-problem with `-O1 -debug` compiler-flags

|No|Approach|Time for 10k rounds|Result|Notes|
|--|--------|-------------------|------|-----|
|#1|1 thread for all|3_300ms|Optimal||
|#2|1 thread per ant|11_000ms|Optimal||

Optimization ideas:
- Let ants wander in batches (because #2 was one thread for each ant -> way slower than one thread for all ants), maybe 50 ants / thread?
- Do staggered exploration
  - Let k colonys compete for n rounds
  - Take the pheromone of the best
  - Repeat
- Maybe do staggered exploration but merge pheromone based on results? 
- Find a way to let ant wander AND update pheromone in an isolated thread without interfering
- pthreads survive single optimize() call

## Writing paper

Online latex: overleaf.hrz.tu-chemnitz.de
Paper: Document decisions
Include referenced code snippets

One optimize() call is an "iteration"



## Sources

[1] M. Dorigo, M. Birattari and T. Stutzle, "Ant colony optimization," in IEEE Computational Intelligence Magazine, vol. 1, no. 4, pp. 28-39, Nov. 2006, doi: 10.1109/MCI.2006.329691. keywords: {Ant colony optimization;Bridges;Insects;Fluctuations;Computational intelligence;Computational and artificial intelligence;Competitive intelligence;Problem-solving;Animals;Guidelines} available at [https://ieeexplore.ieee.org/abstract/document/4129846](https://ieeexplore.ieee.org/abstract/document/4129846)


