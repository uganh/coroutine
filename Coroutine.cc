#include "Coroutine.h"

/*
 * An entry function of each created coroutine
 */
void Coroutine::entry(void) {
  co_curr->func();
  co_curr->state = DEAD;
  co_curr->co_caller->state = RUNNING;
  co_curr = co_curr->co_caller;
}

Coroutine::Coroutine(std::function<void(void)> &&func) :
  state(func ? SUSPENDED : RUNNING /* This is main coroutine */),
  func(std::move(func)),
  co_caller(nullptr) {
  ctx.uc_stack.ss_sp = nullptr;
}

bool Coroutine::resume(void) {
  if (state != SUSPENDED) {
    return false;
  }

  co_caller = co_curr;
  co_curr = this;

  co_caller->state = NORMAL;
  state = RUNNING;

  if (func && !ctx.uc_stack.ss_sp) {
    getcontext(&ctx);

    /* Allocate a new stack for this context */
    ctx.uc_stack.ss_sp = ::operator new(4096);
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

  return true;
}

void Coroutine::yield(void) {
  if (co_curr->co_caller) {
    Coroutine *co_prev = co_curr;
    co_curr = co_curr->co_caller;
    co_prev->state = SUSPENDED;
    co_curr->state = RUNNING;
    swapcontext(&co_prev->ctx, &co_curr->ctx);
  }
  /* TODO: Raise exception */
}
