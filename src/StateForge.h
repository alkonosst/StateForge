/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <initializer_list>
#include <type_traits>
#include <vector>

namespace StateForge {

enum class TranResult { Change, NoChange, Reset, NotFound, InvalidContext };

struct Context {
  virtual ~Context() = default;

  Context(size_t derived_type)
      : derived_type(derived_type) {}
  Context(const Context&)            = default;
  Context& operator=(const Context&) = default;
  Context(Context&&)                 = delete;
  Context& operator=(Context&&)      = delete;

  size_t derived_type;

  template <typename Type>
  bool is() const {
    // Check if Type is not Context
    static_assert(!std::is_same<Type, Context>::value, "is() template type must not be Context!");

    // Check if Type is derived from Context
    static_assert(std::is_base_of<Context, Type>::value,
      "is() template type must be derived from Context!");

    // If Type has not the type member, it will fail to compile (no static_assert needed)
    return derived_type == Type::type;
  }
};

template <typename StateType, typename EventType>
struct Transition {
  StateType from;
  EventType event;
  StateType to;

  std::function<void(StateType from, EventType event, StateType to, Context* const context)>
    on_enter;
  std::function<TranResult(StateType from, EventType event, StateType to, Context* const context)>
    on_transition;
  std::function<void(StateType from, EventType event, StateType to, Context* const context)>
    on_exit;

  Context* const context;
};

template <typename StateType, typename EventType>
class StateMachine {
  static_assert(std::is_enum<StateType>::value, "StateType must be an enum class!");
  static_assert(std::is_enum<EventType>::value, "EventType must be an enum class!");

  public:
  StateMachine(StateType initial_state,
    std::initializer_list<Transition<StateType, EventType>> transitions)
      : _initial_state(initial_state)
      , _current_state(initial_state)
      , _transitions(transitions) {}

  TranResult dispatch(EventType event) {
    for (const auto& transition : _transitions) {
      if (transition.from == _current_state && transition.event == event) {
        TranResult result;

        if (transition.on_enter)
          transition.on_enter(transition.from, transition.event, transition.to, transition.context);

        if (transition.on_transition)
          result = transition.on_transition(transition.from,
            transition.event,
            transition.to,
            transition.context);

        if (transition.on_exit)
          transition.on_exit(transition.from, transition.event, transition.to, transition.context);

        switch (result) {
          case TranResult::Change: _current_state = transition.to; return result;
          case TranResult::Reset: _current_state = _initial_state; return result;
          default: return result;
        }
      }
    }

    return TranResult::NotFound;
  }

  StateType getCurrentState() { return _current_state; }
  void resetState() { _current_state = _initial_state; }

  private:
  StateType _initial_state;
  StateType _current_state;
  std::vector<Transition<StateType, EventType>> _transitions;
};

} // namespace StateForge