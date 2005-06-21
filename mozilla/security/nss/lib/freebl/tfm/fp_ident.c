#include "tfm.h"

const char *fp_ident(void)
{
   static char buf[1024];

   memset(buf, 0, sizeof(buf));
   snprintf(buf, sizeof(buf)-1,
"TomsFastMath (%s)\n"
"\n"
"Sizeofs\n"
"\tfp_digit = %u\n"
"\tfp_word  = %u\n"
"\n"
"FP_MAX_SIZE = %u\n"
"\n"
"Defines: \n"
#ifdef __i386__
" __i386__ "
#endif
#ifdef __x86_64__
" __x86_64__ "
#endif
#ifdef TFM_X86
" TFM_X86 "
#endif
#ifdef TFM_X86_64
" TFM_X86_64 "
#endif
#ifdef TFM_SSE2
" TFM_SSE2 "
#endif
#ifdef TFM_ARM
" TFM_ARM "
#endif
#ifdef TFM_NO_ASM
" TFM_NO_ASM "
#endif
#ifdef FP_64BIT
" FP_64BIT "
#endif
#ifdef TFM_LARGE
" TFM_LARGE "
#endif
#ifdef TFM_HUGE
" TFM_HUGE "
#endif
"\n", 
__DATE__, (unsigned)sizeof(fp_digit), (unsigned)sizeof(fp_word), FP_MAX_SIZE);

   if (sizeof(fp_digit) == sizeof(fp_word)) {
      strncat(buf, "WARNING: sizeof(fp_digit) == sizeof(fp_word).\n"
		   "This build is likely to not work properly.\n", 
              sizeof(buf)-1);
   }
   return buf;
}

#ifdef STANDALONE

int main(void)
{
   printf("%s\n", fp_ident());
   return 0;
}

#endif

