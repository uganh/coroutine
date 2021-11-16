#include "Coroutine.h"

static constexpr std::size_t CO_STACK_SIZE = 4096;

thread_local Coroutine Coroutine::co_main;
thread_local Coroutine *Coroutine::co_curr = &co_main;

/*
 * An entry function of each created coroutine
 */
void CO_ENTRY Coroutine::co_entry(CO_ENTRY_PARAM) {
  co_curr->invoke(co_curr->raw_fn_args);
  Coroutine *co_prev = co_curr;
  co_curr = co_prev->co_caller;
  co_prev->co_caller = nullptr;
  co_prev->_status = DEAD;
  co_curr->_status = RUNNING;
  
  /* Switch context here */
#ifdef _MSC_VER
  SwitchToFiber(co_curr->fiber);
#endif
}

bool Coroutine::resume(void) {
  if (_status != SUSPENDED) {
    return false;
  }

  co_caller = co_curr;
  co_curr = this;

  co_caller->_status = NORMAL;
  _status = RUNNING;

#ifdef _MSC_VER
  if (!fiber) {
    fiber = CreateFiber(CO_STACK_SIZE, &Coroutine::co_entry, nullptr);
  }

  SwitchToFiber(fiber);
#else
  if (!stack) {
    /* Allocate a new stack for this context */
    stack = ::operator new(CO_STACK_SIZE);

    getcontext(&ctx);
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = CO_STACK_SIZE;
    ctx.uc_link = &co_caller->ctx;

    /*
     * When this context is later actived by swapcontext(), the function entry
     * is called. When this function returns, the successor context is actived.
     * If the successor context pointer is NULL, the thread exits.
     */
    makecontext(&ctx, &Coroutine::co_entry, 0);
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
    co_curr = co_prev->co_caller;
    co_prev->co_caller = nullptr;
    co_prev->_status = SUSPENDED;
    co_curr->_status = RUNNING;
#ifdef _MSC_VER
    SwitchToFiber(co_curr->fiber);
#else
    swapcontext(&co_prev->ctx, &co_curr->ctx);
#endif
  } else {
    throw std::exception("Suspending the main coroutine is not allowed");
  }
}
