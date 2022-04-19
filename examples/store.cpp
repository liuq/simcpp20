// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include <cstdio>

#include "simcpp20/simcpp20.hpp"
#include "simcpp20/resource.hpp"

simcpp20::event<> producer(simcpp20::simulation<> &sim,
                           simcpp20::store<int>& store) {
  co_await sim.timeout(3);
  co_await store.put(42);
}

simcpp20::event<> consumer(simcpp20::simulation<> &sim,
                           simcpp20::store<int>& store) {
  int val = co_await store.get();
  printf("[%.0f] val = %d\n", sim.now(), val);
}

int main() {
  simcpp20::simulation<> sim;
  simcpp20::store<int> store{sim};    
  producer(sim, store);
  consumer(sim, store);

  sim.run();
}
