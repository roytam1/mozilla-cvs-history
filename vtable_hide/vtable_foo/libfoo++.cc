#include <stdio.h>
#include <stdlib.h>

class Foo
{
public:
  static Foo sFoo;

  Foo() {}
  virtual ~Foo()
    {
      printf("~Foo [this=%p]\n", this);
    }
  virtual int meth1()
    {
      if ( *((int *) this) == *((int *) &sFoo) )
        printf("vptrs match\n");
      printf("meth1 [this=%p]\n", this);
    }
  virtual int meth2(const char *s)
    { printf("meth2 [this=%p s=\"%s\"]\n", this, s); }
  virtual int meth3(int x, const char *s)
    { printf("meth3 [this=%p x=%d s=\"%s\"]\n", this, x, s); }
  virtual int meth4(int x, const char *s, char c)
    { printf("meth4 [this=%p x=%d s=\"%s\" c=%c]\n", this, x, s, c); }
};

Foo Foo::sFoo;

extern "C" Foo *
CreateFoo()
{
/*
  Foo *f = (Foo *) malloc(sizeof(Foo));

  new (f) Foo();

  return f;
*/
  return new Foo();
}
