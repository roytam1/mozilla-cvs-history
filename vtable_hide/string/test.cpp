#include <stdio.h>
#include "nsStrRef.h"

void func(const char *p)
{
    printf("--%s--\n", p);
}

void concat_and_print(const nsStrRef &a, const nsStrRef &b)
{
    nsStrRef r(a + b);
    printf("%s\n", r.get());
}

int main()
{
    nsStrRef a("howdy");
    NS_STR_NAMED_LITERAL(b, "howdy");
    printf("%s %s %d\n", a.get(), b.get(), a.Equals(b));

    nsStrRef d;
    d.Assign('x');
    printf("%s\n", d.get());

    nsStrRef r(Substring(a, 1, 3));
    printf("%s\n", r.get());

    nsStrDependentRef c("foopity");
    r = a + nsStrRef("/balance/") + b + c + nsStrDependentRef("bob");
    printf("%s\n", r.get());

    func(nsStrRef("xyz").get());

    nsStrDependentRef aa("foo"), bb("bar");
    concat_and_print(aa, bb);

    nsStrAutoBuf a_buf;

    a_buf = Substring(aa, 1, 2) + r;
    printf("1:%s\n", a_buf.get());

    a_buf = NS_STR_LITERAL("$w$") + a_buf + NS_STR_LITERAL("$you$");
    printf("2:%s\n", a_buf.get());

    a_buf.Append(NS_STR_LITERAL("--") + r + NS_STR_LITERAL("--") + r);
    printf("3:%s\n", a_buf.get());

    a_buf.Insert(10, "((xyz))");
    printf("4:%s\n", a_buf.get());

/*
    nsStrRef &a_ref = a_buf;
    a_ref.Assign(NS_STR_LITERAL("one day on the road"));
*/

    a_buf += aa + NS_STR_LITERAL("[[ ") + a_buf + NS_STR_LITERAL(" ]]") + c;
    printf("5:%s\n", a_buf.get());

    a_buf.Replace(0, 5, a);
    printf("6:%s\n", a_buf.get());
    return 0;
}
