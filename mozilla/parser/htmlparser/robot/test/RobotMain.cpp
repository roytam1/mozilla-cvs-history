#include <stdio.h>
#include "nsVoidArray.h"
#include "nsIWebShell.h"
#include "nsString.h"

extern "C" NS_EXPORT int DebugRobot(nsVoidArray * workList, nsIWebShell * ww);

int main(int argc, char **argv)
{
  nsVoidArray * gWorkList = new nsVoidArray();
  int i;
  for (i = 1; i < argc; i++) {
    gWorkList->AppendElement(new nsString(argv[i]));
  }
  return DebugRobot(gWorkList, nsnull);
}

