struct Foo;
struct FooMethods;

struct FooMethods
{
  void (* dtor)(struct Foo * const);
  int  (* meth1)(struct Foo * const);
  int  (* meth2)(struct Foo *, const char *);
  int  (* meth3)(struct Foo *, int, const char *);
  int  (* meth4)(struct Foo *, int, const char *, char);
  void (* dummy1)(struct Foo *);
  void (* dummy2)(struct Foo *);
};

struct Foo
{
  struct FooMethods *vptr;
  int x;
};

static void foo_dtor(struct Foo * const self)
{
  printf("foo_dtor [self=%p]\n", self);
}

static int foo_meth1(struct Foo *self)
{
  printf("foo_meth1 [self=%p]\n", self);
}

static int foo_meth2(struct Foo *self, const char *s)
{
  printf("foo_meth2 [self=%p s=\"%s\"]\n", self, s);
}

static int foo_meth3(struct Foo *self, int x, const char *s)
{
  printf("foo_meth3 [self=%p x=%d s=\"%s\"]\n", self, x, s);
}

static int foo_meth4(struct Foo *self, int x, const char *s, char c)
{
  printf("foo_meth4 [self=%p x=%d s=\"%s\" c=%c]\n", self, x, s, c);
}

static void foo_dummy(struct Foo *self)
{
  printf("foo_dummy [self=%p]\n", self);
}

static struct FooMethods foo_methods = {
  &foo_dtor,
  &foo_dummy,
  &foo_meth1,
  &foo_meth2,
  &foo_meth3,
  &foo_meth4,
};

struct Foo *CreateFoo()
{
  struct Foo *f = (struct Foo *) malloc(sizeof(struct Foo));
  f->vptr = &foo_methods;
  return f;
}
