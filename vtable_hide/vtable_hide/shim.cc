// vim:set cindent:

#include <stddef.h>
#include <stdio.h>
#include "shim.h"
#include "inner.h"

class InnerImpl : public Inner
{
  public:
    InnerImpl() {}

    virtual ~InnerImpl()
    {
      printf("InnerImpl::~InnerImpl [this=%p]\n", this);
      ShimDtor((Shim *) this);
    }

    virtual void foo(int x)
    {
      printf("InnerImpl::foo [this=%p x=%d]\n", this, x);
      ShimFoo((Shim *) this, x);
    }

    virtual void bar(int x, const char *s)
    {
      printf("InnerImpl::bar [this=%p x=%d s=\"%s\"]\n", this, x, s);
      ShimBar((Shim *) this, x, s);
    }
};

// placement new
inline void *operator new(size_t size, Shim *s) { return s; }

static Shim
GetOurImpl()
{
  Shim ourImpl;
  new (&ourImpl) InnerImpl();
  return ourImpl;
}
static Shim gOurImpl = GetOurImpl();
#define IS_EXTERNAL_IMPL(self) (self->vptr != gOurImpl.vptr)

void ShimCtor(Shim *self)
{
  printf("ShimCtor [self=%p]\n", self);

  new (self) InnerImpl();

  /*
   * initialize our members after placement new since sizeof InnerImpl
   * may be bigger than sizeof(void *)
   */

  self->data = 0;
  self->length = 1;
  self->flags = 2;
}

void ShimDtor(Shim *self)
{
  printf("ShimDtor [self=%p]\n", self);

  if (IS_EXTERNAL_IMPL(self))
  {
    ((Inner *) self)->~Inner();
    return;
  }

  printf(" ++ running our dtor!\n");
}

void ShimFoo(Shim *self, int x)
{
  printf("ShimFoo [self=%p x=%d]\n", self, x);

  if (IS_EXTERNAL_IMPL(self))
  {
    ((Inner *) self)->foo(x);
    return;
  }

  printf(" ++ running our foo!\n");
}

void ShimBar(Shim *self, int x, const char *s)
{
  printf("ShimBar [self=%p x=%d s=\"%s\"]\n", self, x, s);

  if (IS_EXTERNAL_IMPL(self))
  {
    ((Inner *) self)->bar(x, s);
    return;
  }

  printf(" ++ running our bar!\n");
}
