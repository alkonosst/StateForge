#include <Arduino.h>
#include <unity.h>

#include "StateForge.h"

using namespace StateForge;

/* ---------------------------------------------------------------------------------------------- */
/*                                   State Machine construction                                   */
/* ---------------------------------------------------------------------------------------------- */
// Define the states
enum class States { Initial, State1, State2 };

// Define the events
enum class Events { Event1, Event2, Event3 };

// Define the context types
enum class Contexts { Context1, Context2 };

// Define the context classes
class Context1 : public Context {
  public:
  static constexpr size_t type = static_cast<size_t>(Contexts::Context1);
  uint32_t positive_int;

  Context1()
      : Context(type)
      , positive_int(0) {}
};

class Context2 : public Context {
  public:
  static constexpr size_t type = static_cast<size_t>(Contexts::Context2);
  int32_t negative_int;

  Context2()
      : Context(type)
      , negative_int(0) {}
};

// Define changing values for the contexts
#define CONTEXT1_ON_ENTER_VALUE 1000
#define CONTEXT1_ON_EXIT_VALUE  2000

#define CONTEXT2_ON_ENTER_VALUE -3000
#define CONTEXT2_ON_EXIT_VALUE  -4000

// Define hooks functions
void onEnter1(States from, Events event, States to, Context* const context) {
  TEST_ASSERT_NOT_NULL(context);
  TEST_ASSERT_TRUE(context->is<Context1>());

  Context1* const ctx = static_cast<Context1*>(context);
  ctx->positive_int   = CONTEXT1_ON_ENTER_VALUE;
}

void onExit1(States from, Events event, States to, Context* const context) {
  TEST_ASSERT_NOT_NULL(context);
  TEST_ASSERT_TRUE(context->is<Context1>());

  Context1* const ctx = static_cast<Context1*>(context);
  ctx->positive_int   = CONTEXT1_ON_EXIT_VALUE;
}

void onEnter2(States from, Events event, States to, Context* const context) {
  TEST_ASSERT_NOT_NULL(context);
  TEST_ASSERT_TRUE(context->is<Context2>());

  Context2* const ctx = static_cast<Context2*>(context);
  ctx->negative_int   = CONTEXT2_ON_ENTER_VALUE;
}

void onExit2(States from, Events event, States to, Context* const context) {
  TEST_ASSERT_NOT_NULL(context);
  TEST_ASSERT_TRUE(context->is<Context2>());

  Context2* const ctx = static_cast<Context2*>(context);
  ctx->negative_int   = CONTEXT2_ON_EXIT_VALUE;
}

// Define state transition functions
TranResult transitionTo1(States from, Events event, States to, Context* const context) {
  return TranResult::Change;
}

TranResult transitionTo2(States from, Events event, States to, Context* const context) {
  return TranResult::Change;
}

TranResult transitionToInit(States from, Events event, States to, Context* const context) {
  return TranResult::Change;
}

// Create context instances
Context1 context1;
Context2 context2;

// Define the state machine
// clang-format off
StateMachine<States, Events> sm(States::Initial,
  {
    {States::Initial, Events::Event1, States::State1,  nullptr,  transitionTo1,    nullptr,  nullptr},
    {States::State1,  Events::Event2, States::State2,  onEnter1, transitionTo2,    onExit1, &context1},
    {States::State2,  Events::Event3, States::Initial, onEnter2, transitionToInit, onExit2, &context2},
});
// clang-format on
/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------- */
/*                                           Unity tests                                          */
/* ---------------------------------------------------------------------------------------------- */
// Test 1: verify initial state is correct
void testInitialState() { TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState()); }

// Test 2: verify state transition from Initial to State1
void testStateTransition1() {
  // Dispatch the correct event to transition from Initial to State1
  TranResult result = sm.dispatch(Events::Event1);

  TEST_ASSERT_EQUAL(TranResult::Change, result);
  TEST_ASSERT_EQUAL(States::State1, sm.getCurrentState());

  // Dispatch incorrect events to stay in State1 (Event1 and Event3)
  result = sm.dispatch(Events::Event1);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::State1, sm.getCurrentState());

  result = sm.dispatch(Events::Event3);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::State1, sm.getCurrentState());
}

// Test 3: verify state transition from State1 to State2
void testStateTransition2() {
  // Dispatch the correct event to transition from State1 to State2
  TranResult result = sm.dispatch(Events::Event2);

  TEST_ASSERT_EQUAL(TranResult::Change, result);
  TEST_ASSERT_EQUAL(States::State2, sm.getCurrentState());

  // Dispatch incorrect events to stay in State2 (Event1 and Event2)
  result = sm.dispatch(Events::Event1);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::State2, sm.getCurrentState());

  result = sm.dispatch(Events::Event2);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::State2, sm.getCurrentState());
}

// Test 4: verify state transition from State2 to Initial
void testStateTransition3() {
  // Dispatch the correct event to transition from State2 to Initial
  TranResult result = sm.dispatch(Events::Event3);

  TEST_ASSERT_EQUAL(TranResult::Change, result);
  TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState());

  // Dispatch incorrect events to stay in Initial (Event2 and Event3)
  result = sm.dispatch(Events::Event2);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState());

  result = sm.dispatch(Events::Event3);
  TEST_ASSERT_EQUAL(TranResult::NotFound, result);
  TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState());
}

// Test 5: verify state reset
void testStateReset() {
  // Advance the state machine to State2 and then reset it
  sm.dispatch(Events::Event1);
  sm.dispatch(Events::Event2);

  sm.resetState();
  TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState());
}

/** Test 6:
 * - do a full cycle of state transitions
 * - verify for context null pointer and correct type at each state
 * - verify the context values at each state
 */
void testStateContexts() {
  // State Initial
  TEST_ASSERT_NULL(sm.getContext(States::Initial, Events::Event1, States::State1));

  // State State1
  sm.dispatch(Events::Event1);
  TEST_ASSERT_EQUAL(States::State1, sm.getCurrentState());

  // -> State1 on enter
  TEST_ASSERT_EQUAL(CONTEXT1_ON_ENTER_VALUE, context1.positive_int);

  // State State2
  sm.dispatch(Events::Event2);
  TEST_ASSERT_EQUAL(States::State2, sm.getCurrentState());

  // -> State1 on exit
  TEST_ASSERT_EQUAL(CONTEXT1_ON_EXIT_VALUE, context1.positive_int);

  // -> State2 on enter
  TEST_ASSERT_EQUAL(CONTEXT2_ON_ENTER_VALUE, context2.negative_int);

  // State Initial
  sm.dispatch(Events::Event3);
  TEST_ASSERT_EQUAL(States::Initial, sm.getCurrentState());

  // -> State2 on exit
  TEST_ASSERT_EQUAL(CONTEXT2_ON_EXIT_VALUE, context2.negative_int);
}

// Test 7: verify the context type of a pointer
void testContextType() {
  // Get the context 1 pointer and verify its type
  Context* const ctx_ptr = sm.getContext(States::State1, Events::Event2, States::State2);

  TEST_ASSERT_NOT_NULL(ctx_ptr);

  TEST_ASSERT_TRUE(ctx_ptr->is<Context1>());
  TEST_ASSERT_FALSE(ctx_ptr->is<Context2>());
}
/* ---------------------------------------------------------------------------------------------- */

void setup() {
  delay(2000);
  UNITY_BEGIN();

  RUN_TEST(testInitialState);
  RUN_TEST(testStateTransition1);
  RUN_TEST(testStateTransition2);
  RUN_TEST(testStateTransition3);
  RUN_TEST(testStateReset);
  RUN_TEST(testStateContexts);
  RUN_TEST(testContextType);

  UNITY_END();
}

void loop() {}