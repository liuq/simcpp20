// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include <algorithm>
#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "fschuetz04/simcpp20.hpp"
#include "fschuetz04/resource.hpp"

simcpp20::event<> awaiter(simcpp20::simulation<> &sim, simcpp20::event<> ev,
                          double target, bool &finished) {
  REQUIRE(sim.now() == 0);
  co_await ev;
  REQUIRE(sim.now() == target);
  finished = true;
};

simcpp20::event<> awaiter_sequence(simcpp20::simulation<> &sim, simcpp20::event<> ev_1, simcpp20::event<> ev_2,
                          double target, bool &finished) {
  REQUIRE(sim.now() == 0);
  co_await ev_1;
  co_await ev_2;
  REQUIRE(sim.now() == target);
  finished = true;
};

TEST_CASE("boolean logic") {
  simcpp20::simulation<> sim;

  SECTION("any_of is not triggered when all events are never processed") {
    auto ev = sim.any_of({sim.event(), sim.event()});
    bool finished = false;
    awaiter(sim, ev, -1, finished);

    sim.run();

    REQUIRE(!finished);
  }

  SECTION("all_of is not triggered when one event is never processed") {
    auto ev = sim.all_of({sim.timeout(1), sim.event()});
    bool finished = false;
    awaiter(sim, ev, -1, finished);

    sim.run();

    REQUIRE(!finished);
  }

  double a = GENERATE(1, 2);
  auto ev_a = sim.timeout(a);
  auto ev_b = sim.timeout(3 - a);

  SECTION("any_of is triggered when the first event is processed") {
    auto ev = sim.any_of({ev_a, ev_b});
    bool finished = false;
    awaiter(sim, ev, 1, finished);

    sim.run();

    REQUIRE(finished);
  }

  SECTION("| is an alias for any_of") {
    auto ev = ev_a | ev_b;
    bool finished = false;
    awaiter(sim, ev, 1, finished);

    sim.run();

    REQUIRE(finished);
  }

  SECTION("all_of is triggered when all events are processed") {
    auto ev = sim.all_of({ev_a, ev_b});
    bool finished = false;
    awaiter(sim, ev, 2, finished);

    sim.run();

    REQUIRE(finished);
  }

  SECTION("& is an alias for any_of") {
    auto ev = ev_a & ev_b;
    bool finished = false;
    awaiter(sim, ev, 2, finished);

    sim.run();

    REQUIRE(finished);
  }
}

TEST_CASE("store resource") {
  simcpp20::simulation<> sim;
  simcpp20::store<int> store{sim}; 

  SECTION("store makes get() wait for put()") {
    auto ev = store.get();
    
    sim.run_until(2);
    REQUIRE(ev.pending());
    store.put(42);
    sim.run();

    REQUIRE(ev.processed());
    REQUIRE(ev.value() == 42);
    REQUIRE(store.size() == 0);
  }

  SECTION("store does not make put() wait for get()") {
    bool finished_put = false, finished_get = false;
    auto put_ev = store.put(42);    
    auto get_ev = store.get();
    awaiter(sim, put_ev, 0, finished_put);
    awaiter(sim, get_ev, 0, finished_get);

    sim.run();
    REQUIRE(put_ev.processed());
    REQUIRE(get_ev.processed());
    REQUIRE(get_ev.value() == 42);
    REQUIRE(store.size() == 0);
    REQUIRE(finished_put);
    REQUIRE(finished_get);
  }

  SECTION("aborted get() do not get the value") {
    auto ev = store.get();
    
    sim.run_until(2);
    REQUIRE(ev.pending());
    ev.abort();
    store.put(42);
    sim.run();

    REQUIRE(store.size() == 1);
    REQUIRE(ev.aborted());
  }
}

TEST_CASE("filtered store resource") {
  simcpp20::simulation<> sim;
  simcpp20::filtered_store<int> store{sim}; 

  SECTION("store makes get() wait for put()") {
    auto ev = store.get([](auto v) { return v >= 40; });
    
    sim.run_until(2);
    REQUIRE(ev.pending());
    store.put(42);
    sim.run();

    REQUIRE(ev.processed());
    REQUIRE(ev.value() == 42);
    REQUIRE(store.size() == 0);
  }

  SECTION("store does not make put() wait for get()") {
    bool finished_put = false, finished_get = false;
    auto put_ev = store.put(42);    
    auto get_ev = store.get([](auto v) { return v >= 40; });
    awaiter(sim, put_ev, 0, finished_put);
    awaiter(sim, get_ev, 0, finished_get);

    sim.run();
    REQUIRE(put_ev.processed());
    REQUIRE(get_ev.processed());
    REQUIRE(get_ev.value() == 42);
    REQUIRE(store.size() == 0);
    REQUIRE(finished_put);
    REQUIRE(finished_get);
  }

  SECTION("aborted get() do not get the value") {
    auto ev = store.get([](auto v) { return v >= 40; });
    
    sim.run_until(2);
    REQUIRE(ev.pending());
    ev.abort();
    store.put(42);
    sim.run();

    REQUIRE(store.size() == 1);
    REQUIRE(ev.aborted());
  }

  SECTION("older get() will get the value when available") {
    auto ev_1 = store.get([](auto v) { return v >= 40;}), ev_2 = store.get([](auto v) { return v < 0; });
    
    sim.run_until(2);
    REQUIRE(ev_1.pending());
    REQUIRE(ev_2.pending());
    store.put(42);
    sim.run();

    REQUIRE(store.size() == 0);
    REQUIRE(ev_2.pending());
    REQUIRE(store.waiting() == 1);
    REQUIRE(ev_1.processed());
    REQUIRE(ev_1.value() == 42);
  }

  SECTION("newer get() will get the value when available") {
    auto ev_1 = store.get([](auto v) { return v < 0; }), ev_2 = store.get([](auto v) { return v >= 40;});
    
    sim.run_until(2);
    REQUIRE(ev_1.pending());
    REQUIRE(ev_2.pending());
    store.put(42);
    sim.run();

    REQUIRE(store.size() == 0);
    REQUIRE(ev_1.pending());
    REQUIRE(store.waiting() == 1);
    REQUIRE(ev_2.processed());
    REQUIRE(ev_2.value() == 42);
  }
}