// vim:set cindent:

#include <stdio.h>
#include "shim.h"
#include "inner.h"

class Outer
{
  private:
    Shim shim;

  public:
    Outer() { ShimCtor(&shim); }
   ~Outer() { ShimDtor(&shim); }
    
    void foo(int x)                 { ShimFoo(&shim, x); }
    void bar(int x, const char *s)  { ShimBar(&shim, x,s); }
};

class ExtImpl : public Inner
{
  public:
    ExtImpl() {}
 
    virtual ~ExtImpl()
    {
      printf(" -- ExtImpl::~ExtImpl [this=%p]\n", this);
    }

    virtual void foo(int x)
    {
      printf(" -- ExtImpl::foo [this=%p x=%d]\n", this, x);
    }

    virtual void bar(int x, const char *s)
    {
      printf(" -- ExtImpl::bar [this=%p x=%d s=\"%s\"]\n", this, x, s);
    }
};

int main()
{
  printf("\ntest use of our class directly:\n\n");
  {
    Outer outer;
    outer.foo(10);
    outer.bar(10, "hello");
  }

  printf("\n\ntest use of embedders class, interpreted as if it were our class:\n\n");
  {
    ExtImpl *ext = new ExtImpl();
    Outer *outer = reinterpret_cast<Outer *>(ext);
    outer->foo(5);
    outer->bar(5, "world");
    delete outer;
  }

  printf("\n\ntest use of our class, interpreted as if it were abstract class:\n\n");
  {
    Outer *outer = new Outer();
    Inner *inner = reinterpret_cast<Inner *>(outer);
    inner->foo(1);
    inner->bar(1, "crazy");
    delete inner;
  }

  printf("\n");
  return 0;
}
