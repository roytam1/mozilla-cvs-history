#include <iostream.h>
#include "nsStringIO.h"

//#define TEST_STD_STRING


#include "nsString.h"
#include "nsSharedString.h"
#include "nsFragmentedString.h"
#include "nsReadableUtils.h"

#ifdef TEST_STD_STRING
#include "nsStdStringWrapper.h"
#else
  typedef nsString nsStdString;
  typedef nsCString nsStdCString;
#endif

template <class CharT>
basic_nsLiteralString<CharT>
literal_hello( CharT* )
  {
  }

NS_SPECIALIZE_TEMPLATE
basic_nsLiteralString<char>
literal_hello( char* )
  {
    return basic_nsLiteralString<char>("Hello");
  }

NS_SPECIALIZE_TEMPLATE
basic_nsLiteralString<PRUnichar>
literal_hello( PRUnichar* )
  {
#ifdef HAVE_CPP_2BYTE_WCHAR_T
    return basic_nsLiteralString<PRUnichar>(L"Hello");
#else
    static PRUnichar constant_unicode[] = { 'H', 'e', 'l', 'l', 'o', PRUnichar() };
    return basic_nsLiteralString<PRUnichar>(constant_unicode);
#endif
  }

template <class T>
struct string_class_traits
  {
  };

NS_SPECIALIZE_TEMPLATE
struct string_class_traits<PRUnichar>
  {
    typedef PRUnichar* pointer;
    typedef nsString implementation_t;
    
    static basic_nsLiteralString<PRUnichar> literal_hello() { return ::literal_hello(pointer()); }
  };

NS_SPECIALIZE_TEMPLATE
struct string_class_traits<char>
  {
    typedef char* pointer;
    typedef nsCString implementation_t;
    
    static basic_nsLiteralString<char> literal_hello() { return ::literal_hello(pointer()); }
  };


static
void
CallCMid( nsAWritableCString& aResult, const nsAReadableCString& aSource, PRUint32 aStartPos, PRUint32 aLengthToCopy )
  {
    aSource.Mid(aResult, aStartPos, aLengthToCopy);
  }



template <class CharT>
int
test_multifragment_iterators( const basic_nsAReadableString<CharT>& aString )
    /*
      ...this tests a problem that was present in |nsPromiseConcatenation| where,
      because it originally stored some iteration state in the object itself, rather than
      in the fragment, the iterators could get confused if used out of sequence.

      This test should be run on any multi-fragment implementation to verify that it
      does not have the same bug.  Make sure the first fragment is only one character long.
    */
  {
    typedef typename basic_nsAReadableString<CharT>::const_iterator ConstIterator;

    int tests_failed = 0;

    ConstIterator iter1 = aString.BeginReading();
    ConstIterator iter2 = aString.BeginReading();
    ++iter2; ++iter2;

    ConstIterator iter3 = aString.EndReading();
    --iter3;
    ++iter1; ++iter1;
    if ( iter1 != iter2 )
      {
        cout << "FAILED in |test_multifragment_iterators|" << endl;
        ++tests_failed;
      }

    return tests_failed;
  }

template <class CharT>
int
test_Vidur_functions( const basic_nsAReadableString<CharT>& aString )
  {
    char* char_copy = ToNewCString(aString);
    PRUnichar* PRUnichar_copy = ToNewUnicode(aString);

    nsMemory::Free(PRUnichar_copy);
    nsMemory::Free(char_copy);

    return 0;
  }

template <class CharT>
int
test_readable_hello( const basic_nsAReadableString<CharT>& aReadable )
  {
    int tests_failed = 0;

    if ( aReadable.Length() != 5 )
      {
        cout << "FAILED |test_readable_hello|: |Length()| --> " << aReadable.Length() << endl;
        ++tests_failed;
      }

    if ( aReadable.First() != CharT('H') )
      {
        cout << "FAILED |test_readable_hello|: |First()| --> '" << aReadable.First() << "'" << endl;
        ++tests_failed;
      }

    if ( aReadable.Last() != CharT('o') )
      {
        cout << "FAILED |test_readable_hello|: |Last()| --> '" << aReadable.Last() << "'" << endl;
        ++tests_failed;
      }

    if ( aReadable[3] != CharT('l') )
      {
        cout << "FAILED |test_readable_hello|: |operator[]| --> '" << aReadable[3] << "'" << endl;
        ++tests_failed;
      }

    if ( aReadable.CountChar( CharT('l') ) != 2 )
      {
        cout << "FAILED |test_readable_hello|: |CountChar('l')| --> " << aReadable.CountChar(CharT('l')) << endl;
        ++tests_failed;
      }

    basic_nsAReadableString<CharT>::const_iterator iter = aReadable.BeginReading();
    if ( *iter != CharT('H') )
      {
        cout << "FAILED |test_readable_hello|: didn't start out pointing to the right thing, or else couldn't be dereferenced. --> '" << *iter << "'" << endl;
        ++tests_failed;
      }

    ++iter;

    if ( *iter != CharT('e') )
      {
        cout << "FAILED |test_readable_hello|: iterator couldn't be incremented, or else couldn't be dereferenced. --> '" << *iter << "'" << endl;
        ++tests_failed;
      }

    iter = aReadable.EndReading();
    --iter;
    if ( *iter != CharT('o') )
      {
        cout << "FAILED |test_readable_hello|: iterator couldn't be set to |EndReading()|, or else couldn't be decremented, or else couldn't be dereferenced. --> '" << *iter << "'" << endl;
        ++tests_failed;
      }

    basic_nsAReadableString<CharT>::const_iterator iter1 = aReadable.BeginReading()+=3;
    if ( *iter1 != CharT('l') )
      {
        cout << "FAILED |test_readable_hello|: iterator couldn't be set to |BeginReading()+=n|, or else couldn't be dereferenced. --> '" << *iter1 << "'" << endl;
        ++tests_failed;
      }

    basic_nsAReadableString<CharT>::const_iterator iter2 = aReadable.EndReading()-=2;
    if ( *iter2 != CharT('l') )
      {
        cout << "FAILED |test_readable_hello|: iterator couldn't be set to |EndReading()-=n|, or else couldn't be dereferenced. --> '" << *iter2 << "'" << endl;
        ++tests_failed;
      }

    if ( iter1 != iter2 )
      {
        cout << "FAILED |test_readable_hello|: iterator comparison with !=." << endl;
        ++tests_failed;
      }

    if ( !(iter1 == iter2) )
      {
        cout << "FAILED |test_readable_hello|: iterator comparison with ==." << endl;
        ++tests_failed;
      }

    typedef CharT* CharT_ptr;
    if ( aReadable != literal_hello(CharT_ptr()) )
      {
        cout << "FAILED |test_readable_hello|: comparison with \"Hello\"" << endl;
        ++tests_failed;
      }

    tests_failed += test_multifragment_iterators(aReadable);
    // tests_failed += test_deprecated_GetBufferGetUnicode(aReadable);

    test_Vidur_functions(aReadable);

    return tests_failed;
  }


template <class CharT>
int
test_SetLength( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;

    string_class_traits<CharT>::implementation_t oldValue(aWritable);


    size_t oldLength = aWritable.Length();

    if ( oldValue != Substring(aWritable, 0, oldLength) )
      {
        cout << "FAILED growing a string in |test_SetLength|, saving the value didn't work." << endl;
        ++tests_failed;
      }

    size_t newLength = 2*(oldLength+1);

    aWritable.SetLength(newLength);
    if ( aWritable.Length() != newLength )
      {
        cout << "FAILED growing a string in |test_SetLength|, length is wrong." << endl;
        ++tests_failed;
      }

    if ( oldValue != Substring(aWritable, 0, oldLength) )
      {
        cout << "FAILED growing a string in |test_SetLength|, contents damaged after growing." << endl;
        ++tests_failed;
      }

    aWritable.SetLength(oldLength);
    if ( aWritable.Length() != oldLength )
      {
        cout << "FAILED shrinking a string in |test_SetLength|." << endl;
        ++tests_failed;
      }

    if ( oldValue != Substring(aWritable, 0, oldLength) )
      {
        cout << "FAILED growing a string in |test_SetLength|, contents damaged after shrinking." << endl;
        ++tests_failed;
      }

    return tests_failed;
  }


template <class CharT>
int
test_insert( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;

    string_class_traits<CharT>::implementation_t oldValue(aWritable);

    if ( oldValue != aWritable )
      {
        cout << "FAILED saving the old string value in |test_insert|." << endl;
        ++tests_failed;
      }

    string_class_traits<CharT>::implementation_t insertable( string_class_traits<CharT>::literal_hello() );

    insertable.SetLength(1);
    aWritable.Insert(insertable, 0);

    if ( aWritable != (insertable + oldValue) )
      {
        cout << "FAILED in |test_insert|." << endl;
        ++tests_failed;
      }

    aWritable = oldValue;

    return tests_failed;
  }


template <class CharT>
int
test_cut( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;

    aWritable.Cut(0, aWritable.Length()+5);

    return tests_failed;
  }

template <class CharT>
int
test_self_assign( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;
    string_class_traits<CharT>::implementation_t oldValue(aWritable);

    aWritable = aWritable;
    if ( aWritable != oldValue )
      {
        cout << "FAILED self assignment." << endl;
        ++tests_failed;
      }

    aWritable = oldValue;
    return tests_failed;
  }

template <class CharT>
int
test_self_append( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;
    string_class_traits<CharT>::implementation_t oldValue(aWritable);

    aWritable += aWritable;
    if ( aWritable != oldValue + oldValue )
      {
        cout << "FAILED self append." << endl;
        ++tests_failed;
      }

    aWritable = oldValue;
    return tests_failed;
  }

template <class CharT>
int
test_self_insert( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;
    string_class_traits<CharT>::implementation_t oldValue(aWritable);

    aWritable.Insert(aWritable, 0);
    if ( aWritable != oldValue + oldValue )
      {
        cout << "FAILED self insert." << endl;
        ++tests_failed;
      }

    aWritable = oldValue;
    return tests_failed;
  }

template <class CharT>
int
test_self_replace( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;
    string_class_traits<CharT>::implementation_t oldValue(aWritable);

    aWritable.Replace(0, 0, aWritable);
    if ( aWritable != oldValue + oldValue )
      {
        cout << "FAILED self insert." << endl;
        ++tests_failed;
      }

    aWritable = oldValue;
    return tests_failed;
  }



template <class CharT>
int
test_writable( basic_nsAWritableString<CharT>& aWritable )
  {
    int tests_failed = 0;
    // ...


    {
      typedef CharT* CharT_ptr;
      aWritable = literal_hello(CharT_ptr());

      if ( aWritable != literal_hello(CharT_ptr()) )
        {
          cout << "FAILED assignment and/or comparison in |test_writable|." << endl;
          ++tests_failed;
        }

      tests_failed += test_readable_hello(aWritable);
    }

    tests_failed += test_SetLength(aWritable);
    tests_failed += test_insert(aWritable);
    tests_failed += test_cut(aWritable);
    tests_failed += test_self_assign(aWritable);
    tests_failed += test_self_append(aWritable);
    tests_failed += test_self_insert(aWritable);
    tests_failed += test_self_replace(aWritable);

    return tests_failed;
  }



typedef void* void_ptr;

int
main()
  {	
    int tests_failed = 0;
  	cout << "String unit tests.  Compiled " __DATE__ " " __TIME__ << endl;

    {
      nsFragmentedCString fs0;
      fs0.Append("Hello");
      tests_failed += test_readable_hello(fs0);
      tests_failed += test_writable(fs0);
    }


    {
      nsLiteralCString s0("Patrick Beard made me write this: \"This is just a test\"\n");
      print_string(s0);

      const char* raw_string = "He also made me write this.\n";
      copy_string(raw_string, raw_string+nsCharTraits<char>::length(raw_string), nsFileCharSink<char>(stdout));


      nsLiteralCString s1("This ", 5), s2("is ", 3), s3("a ", 2), s4("test\n", 5);
      print_string(s1+s2+s3+s4);

      nsLiteralCString s5 = "This is " "a " "test";
      print_string(s5+NS_LITERAL_CSTRING("\n"));

      print_string(nsLiteralCString("The value of the string |x| is \"") + Substring(s0, 0, s0.Length()-1) + NS_LITERAL_CSTRING("\".  Hope you liked it."));
    }


    {
      tests_failed += test_readable_hello(NS_LITERAL_STRING("Hello"));

      nsLiteralCString s1("Hello");
      tests_failed += test_readable_hello(s1);
    }

    {

      nsString s3( NS_LITERAL_STRING("Hello") );

      tests_failed += test_readable_hello(s3);
      tests_failed += test_writable(s3);

      nsCString s4("Hello");
      tests_failed += test_readable_hello(s4);
      tests_failed += test_writable(s4);
    }

    {
      nsStdString s5( NS_LITERAL_STRING("Hello") );

      tests_failed += test_readable_hello(s5);
      tests_failed += test_writable(s5);

      nsStdCString s6("Hello");
      tests_failed += test_readable_hello(s6);
      tests_failed += test_writable(s6);
    }

    {
      nsLiteralString s7(NS_LITERAL_STRING("He"));
      nsString        s8(NS_LITERAL_STRING("l"));
      nsStdString     s9(NS_LITERAL_STRING("lo"));

      tests_failed += test_readable_hello(s7+s8+s9);

      nsString s13( s7 + s8 + s9 );
      tests_failed += test_readable_hello(s13);

      nsStdString s14( s7 + s8 + s9 );
      tests_failed += test_readable_hello(s14);

      nsSharedString* s15 = new_nsSharedString( s7 + s8 + s9 );
      tests_failed += test_readable_hello(*s15);
      // cout << "Here's a string: \""; print_string(*s15) << "\"" << endl;

      nsCString         s10("He");
      nsLiteralCString  s11("l");
      nsStdCString      s12("lo");
      
      tests_failed += test_readable_hello(s10+s11+s12);

      
    }

    {
      const char *foo = "this is a really long string";
      nsCString origString;
      nsCString string2;
      nsCString string3;

      origString = foo;

      string2 = Substring(origString, 0, 5);
      string3 = Substring(origString, 6, origString.Length() - 6);
    }

    {
      nsLiteralCString s13("He");
      nsCAutoString    s14("l");
      nsLiteralCString s15("lo");

      s14.Assign(s13 + s14 + s15);

      if ( int failures = test_readable_hello(s14) )
        {
          tests_failed += failures;
          cout << "FAILED to keep a promise." << endl;
        }
    }


    {
      nsLiteralCString str1("Hello");
      nsStdCString str2("Hello");

      nsLiteralCString str3("Hello there");

      if ( str1 != str2 )
        {
          cout << "string comparison using != failed" << endl;
          ++tests_failed;
        }

      if ( !(str3 > str2) )
        {
          cout << "string comparison using > failed" << endl;
          ++tests_failed;
        }

      if ( !(str2 < str3) )
        {
          cout << "string comparison using < failed" << endl;
          ++tests_failed;
        }

      if ( str1 != "Hello" )
        {
          cout << "string comparison using == failed" << endl;
          ++tests_failed;
        }
    }

#if 0
    nsStdCString extracted_middle("XXXXXXXXXX");
    CallCMid(extracted_middle, part1+part2a+part2b, 1, 3);
    
    cout << "The result of mid was \"" << extracted_middle << "\"" << endl;
    
    nsLiteralCString middle_answer("ell");
    if ( middle_answer != extracted_middle )
      {
        cout << "mid FAILED on nsConcatString" << endl;
        ++tests_failed;
      }



      //
      // |nsStdStringWrapper|, i.e., |nsStdCString|
      //

    {
      nsStdCString extracted_middle;
      CallCMid(extracted_middle, part1+part2a+part2b, 1, 3);

     cout << "The result of mid was \"" << extracted_middle << "\"" << endl;

      nsLiteralCString middle_answer("ell");
      if ( middle_answer != extracted_middle )
        {
          cout << "mid FAILED on nsStdCString" << endl;
          ++tests_failed;
        }
    }



    nsStdCString leftString;
    source.Left(leftString, 9);
    // cout << static_cast<nsAReadableCString>(leftString) << endl;



    tests_failed += test_multifragment_iterators(part1+part2a+part2b);
#endif

    
    
    cout << "End of string unit tests." << endl;
    if ( !tests_failed )
      cout << "OK, all tests passed." << endl;
    else
      cout << "FAILED one or more tests." << endl;

  	return 0;
  }
