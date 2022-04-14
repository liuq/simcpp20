// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include <cstdio>

#include "fschuetz04/simcpp20.hpp"
#include "fschuetz04/resource.hpp"

simcpp20::event<> producer(simcpp20::simulation<> &sim,
                           simcpp20::filtered_store<int>& store) {
  for (int i = 0; i < 10; ++i) {
    co_await sim.timeout(1);
    co_await store.put(i);  
  }
}

simcpp20::event<> consumer(simcpp20::simulation<> &sim,
                           simcpp20::filtered_store<int>& store) {
  int val = co_await store.get([](const int& v) { return v >= 5; });
  printf("[%.0f] val = %d\n", sim.now(), val);
}

int main() {
  simcpp20::simulation<> sim;
  simcpp20::filtered_store<int> store{sim};    
  producer(sim, store);
  consumer(sim, store);

  sim.run();
}
