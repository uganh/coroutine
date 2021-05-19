#include <cassert>

#include "Coroutine.h"

thread_local Coroutine Coroutine::co_main;
thread_local Coroutine *Coroutine::co_curr = &co_main;

/*
 * An entry function of each created coroutine
 */
void Coroutine::entry(void) {
  co_curr->func();
  co_curr->state = DEAD;
  co_curr->co_caller->state = RUNNING;
  co_curr = co_curr->co_caller;

  /* Switch context here */
}

Coroutine::Coroutine(std::function<void(void)> &&func) :
  state(func ? SUSPENDED : RUNNING /* This is main coroutine */),
  func(std::move(func)),
  co_caller(nullptr),
#ifdef _MSC_VER
  fiber(nullptr) {
  if (!func) {
    fiber = ConvertThreadToFiber(nullptr);
  }
#else
  stack(nullptr) {
#endif
}

bool Coroutine::resume(void) {
  if (state != SUSPENDED) {
    return false;
  }

  assert(func);

  co_caller = co_curr;
  co_curr = this;

  co_caller->state = NORMAL;
  state = RUNNING;

#ifdef _MSC_VER
  if (!fiber) {
    fiber = CreateFiber(
      4096, reinterpret_cast<LPFIBER_START_ROUTINE>(entry), nullptr);
  }

  SwitchToFiber(fiber);
#else
  if (!stack) {
    /* Allocate a new stack for this context */
    stack = ::operator new(4096);

    getcontext(&ctx);
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = 4096;
    ctx.uc_link = &co_caller->ctx;

    /*
     * When this context is later actived by swapcontext(), the function entry
     * is called. When this function returns, the successor context is actived.
     * If the successor context pointer is NULL, the thread exits.
     */
    makecontext(&ctx, entry, 0);
  }

  /*
   * The swapcontext() function saves the current context, and then activates
   * the context of another
   */
  swapcontext(&co_caller->ctx, &ctx);
#endif

  return true;
}

void Coroutine::yield(void) {
  if (co_curr->co_caller) {
    Coroutine *co_prev = co_curr;
    co_curr = co_curr->co_caller;
    co_prev->state = SUSPENDED;
    co_curr->state = RUNNING;
#ifdef _MSC_VER
    SwitchToFiber(co_curr->fiber);
#else
    swapcontext(&co_prev->ctx, &co_curr->ctx);
#endif
  } else {
    /* TODO: Throw exception */
  }
}
