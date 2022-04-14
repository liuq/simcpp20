// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#pragma once

#include <cstdint>
#include <queue>
#include <list>

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

  /**
   * @return size_t number of waiting events.
   */
  size_t waiting() {
    return evs.size();
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

template <typename Value, typename Time = double> class filtered_store {
public:
  filtered_store(simcpp20::simulation<Time> &sim) : sim{sim} {}

  /**
   * @tparam Args inferred types of the Value constructor.
   * @param args arguments for the constructor of the Value associated the event.
   * @return A new triggered event that confirms the effect of the put operation.
   */
  template <typename... Args>
  simcpp20::event<Time> put(Args &&...args) {
    auto ev = sim.event();
    list_.push_back(std::forward<Args>(args)...);
    ev.trigger();
    trigger_put();
    return ev;
  }

  /**
   * @return A new value event that could be triggered if there are values available in the queue or pending otherwise.
   */
  simcpp20::value_event<Value, Time> get(std::function<bool(const Value& v)> p) {
    auto ev = sim.template event<Value>();
    if (list_.size() > 0) {
      trigger_get(ev, p);
    } else {
      evs.push_back({ ev, p });
    }
    return ev;
  }

  /**
   * @return size_t number of stored elements.
   */

  size_t size() {
    return list_.size();
  }

  /**
   * @return size_t number of waiting events.
   */
  size_t waiting() {
    return evs.size();
  }

protected:
  void trigger_put() {
    // the only value candidate to be checked is the newly added one at the list back
    // some maintenance: get rid of aborted events, if any
    evs.erase(std::remove_if(evs.begin(), evs.end(), [](auto pair) { return pair.first.aborted(); }), evs.end());
    if (evs.size() == 0 || list_.size() == 0)
      return;
    for (auto it = evs.begin(); it != evs.end(); ++it) {
      auto ev = it->first;
      auto p = it->second;
      if (p(list_.back())) {
        ev.trigger(list_.back());
        it = evs.erase(it);
        list_.pop_back();
        break;
      }        
    }
  }

  void trigger_get(simcpp20::value_event<Value, Time>& ev, std::function<bool(const Value& v)> p) {
    // some maintenance: get rid of aborted events, if any
    evs.erase(std::remove_if(evs.begin(), evs.end(), [](auto pair) { return pair.first.aborted(); }), evs.end());
    auto it = std::find_if(list_.begin(), list_.end(), p);
    if (it != list_.end()) {
      ev.trigger(*it);
      it = list_.erase(it);
    } else {
      evs.push_back({ ev, p });
    }
  }

private:
  simcpp20::simulation<Time> &sim;
  std::list<std::pair<simcpp20::value_event<Value, Time>, std::function<bool(const Value&)>>> evs{};
  std::list<Value> list_;
};

} // namespace simcpp20