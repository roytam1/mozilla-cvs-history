/* vim:set ts=2 sw=2 et cindent: */
#ifndef nsCharTraits_h__
#define nsCharTraits_h__

#include "types.h" // XXX temporary
#include <string.h>

template <class T> struct nsCharTraits {};

NS_SPECIALIZE_TEMPLATE
struct nsCharTraits<char>
{
  public:
    typedef char char_type;

    typedef PRInt32 (* comparator_func)
      (const char_type *a, const char_type *b, int n);

    static const char_type *empty_string;

    static PRInt32 case_sensitive_comparator(const char_type *a,
                                             const char_type *b, int n)
      { return (PRInt32) memcmp(a, b, n); }

    static PRUint32 length_of(const char_type *data)
      { return (PRUint32) ::strlen(data); }

    static void copy(char_type *dest, const char_type *src, PRUint32 len)
      { ::memcpy(dest, src, len); }

    static void move(char_type *dest, const char_type *src, PRUint32 len)
      { ::memmove(dest, src, len); }

    static const char_type *find_char(const char_type *start,
                                      const char_type *end,
                                      char_type c)
      { return (const char_type *) ::memchr(start, c, end - start); }
};

#endif // nsCharTraits_h__
