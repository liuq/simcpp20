// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include <cstdio>

#include "simcpp20/simcpp20.hpp"

struct ev_type;
using ev_inner = simcpp20::value_event<ev_type>;
struct ev_type {
  ev_inner ev;
#ifdef CLANG_COMPILER
  ev_type(const ev_inner& ev) : ev(ev) {}
#endif
};

simcpp20::event<> party(simcpp20::simulation<> &sim, const char *name,
                        simcpp20::value_event<ev_type> my_event, double delay) {
  while (true) {
    auto their_event = (co_await my_event).ev;
    printf("[%.0f] %s\n", sim.now(), name);
    co_await sim.timeout(delay);
    my_event = sim.event<ev_type>();
    their_event.trigger(my_event);
  }
}

int main() {
  simcpp20::simulation<> sim;
  auto pong_event = sim.event<ev_type>();
  auto ping_event = sim.timeout<ev_type>(0, pong_event);
  party(sim, "ping", ping_event, 1);
  party(sim, "pong", pong_event, 2);
  sim.run_until(8);
}
