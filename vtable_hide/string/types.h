/* vim:set ts=2 sw=2 et cindent: */

#ifndef str_types_h__
#define str_types_h__

/* -------------------------------------------------------------------------- */
#include <stdio.h>
typedef int PRInt32;
typedef unsigned int PRUint32;
typedef unsigned int PRBool;
#define PR_TRUE 1
#define PR_FALSE 0
typedef char NS_CHAR;
#define NS_CHAR_SIZE sizeof(NS_CHAR)
#define NS_COM
#define NS_ASSERTION(x, s) if (!(x)) printf("### ASSERTION [%s]: %s\n", # x, s) 
#define NS_ERROR(s) printf("### ERROR: %s\n", s)
#define PR_MIN(x,y) ((x) < (y) ? (x) : (y))
#define NS_CONST_CAST(x,y) const_cast<x>(y)
#define NS_STATIC_CAST(x,y) static_cast<x>(y)
#define nsnull 0
#define NS_SPECIALIZE_TEMPLATE  template <>
#define PR_AtomicIncrement(x) (++(*(x)))
#define PR_AtomicDecrement(x) (--(*(x)))
#define PR_UINT32_MAX PRUint32(-1)
/* -------------------------------------------------------------------------- */

#endif
