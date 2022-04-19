// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include <cstdio>

#include "simcpp20/simcpp20.hpp"

simcpp20::event<> clock_proc(simcpp20::simulation<> &sim, char const *name,
                             double delay) {
  while (true) {
    printf("[%.0f] %s\n", sim.now(), name);
    co_await sim.timeout(delay);
  }
}

int main() {
  simcpp20::simulation<> sim;
  clock_proc(sim, "slow", 2);
  clock_proc(sim, "fast", 1);
  sim.run_until(5);
}
