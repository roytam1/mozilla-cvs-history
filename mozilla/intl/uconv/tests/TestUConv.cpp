/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsUCSupport.h"
#include "nsString.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

#define ARRAY_SIZE(_array)  (sizeof(_array) / sizeof(_array[0]))
#define SMALL_BUFFER_SIZE   512
#define MED_BUFFER_SIZE     1024
#define BIG_BUFFER_SIZE     2048

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

//----------------------------------------------------------------------------
// Class nsTestLog [declaration]

/**
 * A Logging class for test programs.
 *
 * This simple test program will not trigger a component registration. So 
 * Mozilla has to be run once before running this, so that the necessary 
 * components will be registered. Also, please observe that the ContractID's are 
 * case sensitive now!
 *
 * @created         28/Mar/2000
 * @author  Catalin Rotaru [CATA]
 */
class nsTestLog
{
private:

  static const char * kTraceDelimiter;

  nsCAutoString mTrace;

public:

  void AddTrace(char * aTrace);
  void DelTrace(char * aTrace);
  void PrintError(char * aCall, int aError);
  void PrintError(char * aCall, char * aMessage);
};
  
//----------------------------------------------------------------------------
// Class nsTestUConv [declaration]

/**
 * The main class of the program.
 *
 * XXX Create a very general set of "bug and regression" test cases and the 
 * one in TestTempBug()
 * XXX Apply the new argument style (pointers) to the converters interfaces
 *
 * @created         28/Mar/2000
 * @author  Catalin Rotaru [CATA]
 */
class nsTestUConv
{
private:

  nsTestLog mLog;

  /**
   * Run the built-in set of self tests for encoders.
   */
  nsresult TestEncoders();

  /**
   * Run the built-in set of self tests for decoders.
   */
  nsresult TestDecoders();

  /**
   * Run the built-in set of self tests for the CharsetManager.
   */
  nsresult TestCharsetManager();

  /**
   * Display charset detectors and their attributes.
   */
  nsresult DisplayDetectors();

  /**
   * Display charsets and their attributes.
   */
  nsresult DisplayCharsets();

  /**
   * Run a temporary debug test. This method is ment as a placeholder when some
   * quick debugging is needed.
   */
  nsresult TestTempBug();

  nsresult Encode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, char ** aDest, 
    char * aDestEnd, nsString * aCharset);

  /**
   * Bridge methods between the new argument style (poiters) and the old one 
   * (lengths). To be removed when the converter interfaces will switch to the
   * new style.
   *
   * This wraps an encoder Convert() call.
   */
  nsresult ConvertEncode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, char ** aDest, 
    char * aDestEnd, nsIUnicodeEncoder * aEncoder);

  /**
   * This wraps an encoder Finish() call.
   */
  nsresult FinishEncode(char ** aDest, char * aDestEnd, 
    nsIUnicodeEncoder * aEncoder);

  void PrintSpaces(int aCount);

  nsresult AddArray(nsISupportsArray * aDest, nsISupportsArray * aSrc);

public:

  /**
   * Main method of the program.
   */
  nsresult Main(int aArgC, char ** aArgV);
};
  
//----------------------------------------------------------------------------
// Global functions and data [implementation]

int main(int argc, char ** argv)
{
  nsTestUConv testObj;
  nsresult res;
 
  res = testObj.Main(argc, argv);
  return (NS_FAILED(res));
}

//----------------------------------------------------------------------------
// Class nsTestLog [implementation]

const char * nsTestLog::kTraceDelimiter = ".";

void nsTestLog::AddTrace(char * aTrace)
{
  mTrace.Append(aTrace);
  mTrace.Append(kTraceDelimiter);
}

void nsTestLog::DelTrace(char * aTrace)
{
  mTrace.Truncate(mTrace.Length() - strlen(aTrace) - strlen(kTraceDelimiter));
}

void nsTestLog::PrintError(char * aCall, int aError)
{
  const char * trace = mTrace.get();
  printf("ERROR at %s%s code=0x%x.\n", trace, aCall, aError);
}

void nsTestLog::PrintError(char * aCall, char * aMessage)
{
  const char * trace = mTrace.get();
  printf("ERROR at %s%s reason: %s.\n", trace, aCall, aMessage);
}

//----------------------------------------------------------------------------
// Class nsTestUConv [implementation]

nsresult nsTestUConv::TestEncoders()
{
  char * trace = "TestEncoders";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager2> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsISupportsArray> encoders;
  res = ccMan->GetEncoderList(getter_AddRefs(encoders));
  if (NS_FAILED(res)) return res;

  PRUint32 encoderCount;
  encoders->Count(&encoderCount);
  printf("There are %d encoders\n", encoderCount);
  
  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::TestDecoders()
{
  char * trace = "TestDecoders";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  // XXX write me

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::TestCharsetManager()
{
  char * trace = "TestCharsetManager";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;
  nsAutoString name;
  nsCOMPtr<nsIAtom> csAtom;

  nsCOMPtr<nsICharsetConverterManager2> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  // test alias resolving capability
  nsAutoString csAlias(NS_LITERAL_STRING("iso-10646-ucs-basic"));
  nsAutoString csName(NS_LITERAL_STRING("UTF-16BE"));
  res = ccMan->GetCharsetAtom(csAlias.get(), getter_AddRefs(csAtom));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetCharsetAtom()", res);
    return res;
  }
  res = csAtom->ToString(name);
  if (NS_FAILED(res)) {
    mLog.PrintError("get()", res);
    return res;
  }
  if (!csName.Equals(name)) {
    mLog.PrintError("Equals()", "unexpected charset name");
    return NS_ERROR_UNEXPECTED;
  }

  // test self returning if alias was not found
  nsAutoString csAlias2(NS_LITERAL_STRING("Totally_dummy_charset_name"));
  res = ccMan->GetCharsetAtom(csAlias2.get(), getter_AddRefs(csAtom));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetCharsetAtom()", res);
    return res;
  }
  res = csAtom->ToString(name);
  if (NS_FAILED(res)) {
    mLog.PrintError("get()", res);
    return res;
  }
  if (!csAlias2.Equals(name)) {
    mLog.PrintError("Equals()", "unexpected charset name");
    return NS_ERROR_UNEXPECTED;
  }

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::DisplayDetectors()
{
  char * trace = "DisplayDetectors";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager2> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  // charset detectors
  nsCOMPtr<nsISupportsArray> array;

  res = ccMan->GetCharsetDetectorList(getter_AddRefs(array));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetCharsetDetectorList()", res);
    return res;
  }

  PRUint32 count;
  res = array->Count(&count);
  if (NS_FAILED(res)) {
    mLog.PrintError("Count()", res);
    return res;
  }

  printf("***** Character Set Detectors [%d] *****\n", count);

  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> cs;
    nsAutoString str;

    res = array->GetElementAt(i, getter_AddRefs(cs));
    if (NS_FAILED(res)) {
      mLog.PrintError("GetElementAt()", res);
      return res;
    }

    nsAutoString name;
    res = cs->ToString(name);
    if (NS_FAILED(res)) {
      mLog.PrintError("get()", res);
      return res;
    }

    str.Assign(name);
    NS_LossyConvertUCS2toASCII buff(str);
    printf("%s", buff.get());
    PrintSpaces(36 - buff.Length());  // align to hard coded column number

    res = ccMan->GetCharsetTitle2(cs, &str);
    if (NS_FAILED(res)) str.SetLength(0);
    printf("\"%s\"\n", NS_LossyConvertUCS2toASCII(str).get());
  }
  
  mLog.DelTrace(trace);
  return NS_OK;
}

nsresult nsTestUConv::DisplayCharsets()
{
  char * trace = "DisplayCharsets";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager2> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  nsCOMPtr<nsISupportsArray> array;
  nsCOMPtr<nsISupportsArray> encArray;

  res = ccMan->GetDecoderList(getter_AddRefs(array));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetDecoderList()", res);
    return res;
  }

  res = ccMan->GetEncoderList(getter_AddRefs(encArray));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetEncoderList()", res);
    return res;
  }

  res = AddArray(array, encArray);
  if (NS_FAILED(res)) return res;

  PRUint32 count;
  res = array->Count(&count);
  if (NS_FAILED(res)) {
    mLog.PrintError("Count()", res);
    return res;
  }

  printf("***** Character Sets [%d] *****\n", count);

  PRUint32 encCount = 0, decCount = 0;
  PRUint32 basicEncCount = 0, basicDecCount = 0;
  
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> cs;
    nsAutoString str;
    nsAutoString prop;

    res = array->GetElementAt(i, getter_AddRefs(cs));
    if (NS_FAILED(res)) {
      mLog.PrintError("GetElementAt()", res);
      return res;
    }

    nsAutoString name;
    res = cs->ToString(name);
    if (NS_FAILED(res)) {
      mLog.PrintError("get()", res);
      return res;
    }

    str.Assign(name);
    NS_LossyConvertUCS2toASCII buff(str);
    printf("%s", buff.get());
    PrintSpaces(24 - buff.Length());  // align to hard coded column number


    nsCOMPtr<nsIUnicodeDecoder> dec = NULL;
    res = ccMan->GetUnicodeDecoder(cs, getter_AddRefs(dec));
    if (NS_FAILED(res)) printf (" "); 
    else {
      printf("D");
      decCount++;
    }
#ifdef NS_DEBUG
    // show the "basic" decoder classes
    if (dec) {
      nsCOMPtr<nsIBasicDecoder> isBasic = do_QueryInterface(dec);
      if (isBasic) {
        basicDecCount++;
        printf("b");
      }
      else printf(" ");
    }
    else printf(" ");
#endif

    nsCOMPtr<nsIUnicodeEncoder> enc = NULL;
    res = ccMan->GetUnicodeEncoder(cs, getter_AddRefs(enc));
    if (NS_FAILED(res)) printf (" "); 
    else {
      printf("E");
      encCount++;
    }

#ifdef NS_DEBUG
    if (enc) {
      nsCOMPtr<nsIBasicEncoder> isBasic = do_QueryInterface(enc);
      if (isBasic) {
        basicEncCount++;
        printf("b");
      }
      else printf(" ");
    }
    else printf(" ");
#endif
    
    printf(" ");

    prop.Assign(NS_LITERAL_STRING(".notForBrowser"));
    res = ccMan->GetCharsetData2(cs, prop.get(), &str);
    if ((dec != NULL) && (NS_FAILED(res))) printf ("B"); 
    else printf("X");

    prop.Assign(NS_LITERAL_STRING(".notForComposer"));
    res = ccMan->GetCharsetData2(cs, prop.get(), &str);
    if ((enc != NULL) && (NS_FAILED(res))) printf ("C"); 
    else printf("X");

    prop.Assign(NS_LITERAL_STRING(".notForMailView"));
    res = ccMan->GetCharsetData2(cs, prop.get(), &str);
    if ((dec != NULL) && (NS_FAILED(res))) printf ("V"); 
    else printf("X");

    prop.Assign(NS_LITERAL_STRING(".notForMailEdit"));
    res = ccMan->GetCharsetData2(cs, prop.get(), &str);
    if ((enc != NULL) && (NS_FAILED(res))) printf ("E"); 
    else printf("X");

    printf("(%3d, %3d) ", encCount, decCount);
    res = ccMan->GetCharsetTitle2(cs, &str);
    if (NS_FAILED(res)) str.SetLength(0);
    NS_LossyConvertUCS2toASCII buff2(str);
    printf(" \"%s\"\n", buff2.get());
  }

  printf("%u of %u decoders are basic (%d%%)\n",
         basicDecCount, decCount, (basicDecCount * 100) / decCount);

  printf("%u of %u encoders are basic (%d%%)\n",
         basicEncCount, encCount, (basicEncCount * 100) / encCount);
  mLog.DelTrace(trace);
  return NS_OK;
}

nsresult nsTestUConv::TestTempBug()
{
  char * trace = "TestTempBug";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsAutoString charset(NS_LITERAL_STRING("ISO-2022-JP"));
  PRUnichar src[] = {0x0043, 0x004e, 0x0045, 0x0054, 0x0020, 0x004A, 0x0061, 
    0x0070, 0x0061, 0x006E, 0x0020, 0x7DE8, 0x96C6, 0x5C40};
  PRUnichar * srcEnd = src + ARRAY_SIZE(src);
  char dest[BIG_BUFFER_SIZE];
  char * destEnd = dest + BIG_BUFFER_SIZE;

  PRUnichar * p = src;
  char * q = dest;
  res = Encode(&p, srcEnd, &q, destEnd, &charset);

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::Encode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, 
                             char ** aDest, char * aDestEnd, 
                             nsString * aCharset)
{
  char * trace = "Encode";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  nsCOMPtr<nsIUnicodeEncoder> enc;
  res = ccMan->GetUnicodeEncoder(aCharset, getter_AddRefs(enc));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetUnicodeEncoder()", res);
    return res;
  }

  res = ConvertEncode(aSrc, aSrcEnd, aDest, aDestEnd, enc);
  if (NS_FAILED(res)) {
    mLog.PrintError("Convert()", res);
    return res;
  }

  res = FinishEncode(aDest, aDestEnd, enc);
  if (NS_FAILED(res)) {
    mLog.PrintError("Finish()", res);
    return res;
  }

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::ConvertEncode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, 
                                    char ** aDest, char * aDestEnd, 
                                    nsIUnicodeEncoder * aEncoder)
{
  PRUnichar * src = (*aSrc);
  char * dest = (*aDest);
  PRInt32 srcLen = aSrcEnd - src;
  PRInt32 destLen = aDestEnd - dest;

  nsresult res = aEncoder->Convert(src, &srcLen, dest, &destLen);

  (*aSrc) = src + srcLen;
  (*aDest) = dest + destLen;
  return res;
}

nsresult nsTestUConv::FinishEncode(char ** aDest, char * aDestEnd, 
                                   nsIUnicodeEncoder * aEncoder)
{
  char * dest = (*aDest);
  PRInt32 destLen = aDestEnd - dest;

  nsresult res = aEncoder->Finish(dest, &destLen);

  (*aDest) = dest + destLen;
  return res;
}

void nsTestUConv::PrintSpaces(int aCount)
{
  for (int i = 0; i < aCount; i++) printf(" ");
}

nsresult nsTestUConv::AddArray(nsISupportsArray * aDest, 
                               nsISupportsArray * aSrc)
{
  nsresult res = NS_OK;
  PRUint32 count, i;

  res = aSrc->Count(&count);
  if (NS_FAILED(res)) return res;

  for (i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> elem;
    res = aSrc->GetElementAt(i, getter_AddRefs(elem));
    if (NS_FAILED(res)) return res;

    PRInt32 index;
    res = aDest->GetIndexOf(elem, &index);
    if (NS_FAILED(res)) return res;

    if (index < 0) res = aDest->AppendElement(elem);
  }

  return NS_OK;
}

nsresult nsTestUConv::Main(int aArgC, char ** aArgV)
{
  char * trace = "Main";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  if (aArgC < 2) {
    // no arguments were passed to the program, so we just run the self tests
    res = TestCharsetManager();
    if (NS_SUCCEEDED(res)) res = TestEncoders();
    if (NS_SUCCEEDED(res)) res = TestDecoders();
  } else if (!strcmp(aArgV[1], "-tempbug")) {
    // we are testing a temporary bug
    res = TestTempBug();
  } else if (!strcmp(aArgV[1], "-display")) {
    // display all the available data
    res = DisplayDetectors();
    if (NS_SUCCEEDED(res)) res = DisplayCharsets();
  }

  mLog.DelTrace(trace);
  return res;
}
