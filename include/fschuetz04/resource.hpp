// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#pragma once

#include <cstdint>
#include <queue>
#include <iostream>

#include "fschuetz04/simcpp20.hpp"

namespace simcpp20 {

/**
 * Used to create a (discrete) shared resource.
 *
 * To create a new instance, default-initialize the class:
 *
 *     simcpp20::resource<> resource;
 *
 * @tparam Time Type used for simulation time.
 */
template <class Time = double> class resource {
public:
  resource(simcpp20::simulation<Time> &sim, uint64_t available)
      : sim{sim}, available_{available} {}

  simcpp20::event<Time> request() {
    auto ev = sim.event();
    evs.push(ev);
    trigger_evs();
    return ev;
  }

  void release() {
    ++available_;
    trigger_evs();
  }

  uint64_t available() { return available_; }

private:
  std::queue<simcpp20::event<Time>> evs{};
  simcpp20::simulation<Time> &sim;
  uint64_t available_;

  void trigger_evs() {
    while (available() > 0 && evs.size() > 0) {
      auto ev = evs.front();
      evs.pop();
      if (ev.aborted()) {
        continue;
      }

      ev.trigger();
      --available_;
    }
  }
};

/**
 * Used to create a (discrete) shared store for a given type.
 *
 * To create a new instance, initialize the class passing
 * the type of the stored values.
 *
 *     simcpp20::resource<int> store;
 *
 * @tparam Value Type used for stored values.
 * @tparam Time Type used for simulation time.
 */

template <typename Value, typename Time = double> class store {
public:
  store(simcpp20::simulation<Time> &sim) : sim{sim} {}


  /**
   * @tparam Args inferred types of the Value constructor.
   * @param args arguments for the constructor of the Value associated the event.
   * @return A new triggered event that confirms the effect of the put operation.
   */
  template <typename... Args>
  simcpp20::event<Time> put(Args &&...args) {
    auto ev = sim.event();
    queue_.push(std::forward<Args>(args)...);
    ev.trigger();
    trigger_get();
    return ev;
  }

  /**
   * @return A new value event that could be triggered if there are values available in the queue or pending otherwise.
   */
  simcpp20::value_event<Value, Time> get() {
    auto ev = sim.template event<Value>();
    if (queue_.size() > 0) {
      ev.trigger(queue_.front());
      queue_.pop();
    } else {
      evs.push(ev);
    }
    return ev;
  }

  /**
   * @return size_t number of stored elements.
   */

  size_t size() {
    return queue_.size();
  }

protected:
  void trigger_get() {
    while (evs.size() > 0 && queue_.size() > 0) {
      auto ev = evs.front();
      evs.pop();
      if (ev.aborted()) {
        continue;
      }
      ev.trigger(queue_.front());
      queue_.pop();
    }
  }

private:
  simcpp20::simulation<Time> &sim;
  std::queue<simcpp20::value_event<Value, Time>> evs{};
  std::queue<Value> queue_;
};

} // namespace simcpp20