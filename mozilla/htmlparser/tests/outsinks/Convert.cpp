/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *  
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *  
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): Akkana Peck.
 */

#include <ctype.h>      // for isdigit()

#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsHTMLContentSinkStream.h"
#include "nsHTMLToTXTSinkStream.h"
#include "nsIComponentManager.h"
#include "CNavDTD.h"
#include "nsXIFDTD.h"

extern "C" void NS_SetupRegistry();

#ifdef XP_PC
#define PARSER_DLL "gkparser.dll"
#endif
#ifdef XP_MAC
#endif
#if defined(XP_UNIX) || defined(XP_BEOS)
#define PARSER_DLL "libraptorhtmlpars"MOZ_DLL_SUFFIX
#endif

// Class IID's
static NS_DEFINE_IID(kParserCID, NS_PARSER_IID);

// Interface IID's
static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);

int
Compare(nsString& str, nsString& aFileName)
{
  // Open the file in a Unix-centric way,
  // until I find out how to use nsFileSpec:
  char* filename = aFileName.ToNewCString();
  FILE* file = fopen(filename, "r");
  if (!file)
  {
    fprintf(stderr, "Can't open file %s", filename);
    perror(" ");
    delete[] filename;
    return 2;
  }
  delete[] filename;

  // Inefficiently read from the file:
  nsString inString;
  char c;
  int index = 0;
  int different = 0;
  while ((c = getc(file)) != EOF)
  {
    inString += c;
    // CVS isn't doing newline comparisons on these files for some reason.
    // So compensate for possible newline problems in the CVS file:
    if (c == '\n' && str[index] == '\r')
      ++index;
    if (c != str[index++])
    {
      //printf("Comparison failed at char %d: generated was %d, file had %d\n",
      //       index, (int)str[index-1], (int)c);
      different = index;
      break;
    }
  }
  if (file != stdin)
    fclose(file);

  if (!different)
    return 0;
  else
  {
    char* cstr = str.ToNewUTF8String();
    printf("Comparison failed at char %d:\n-----\n%s\n-----\n",
           different, cstr);
    Recycle(cstr);
    return 1;
  }
}

//----------------------------------------------------------------------
// Convert html on stdin to either plaintext or (if toHTML) html
//----------------------------------------------------------------------
nsresult
HTML2text(nsString& inString, nsString& inType, nsString& outType,
          int flags, int wrapCol, nsString& compareAgainst)
{
  nsresult rv = NS_OK;

  nsString outString;

  // Create a parser
  nsIParser* parser;
   rv = nsComponentManager::CreateInstance(kParserCID, nsnull,
                                           kIParserIID,(void**)&parser);
  if (NS_FAILED(rv))
  {
    printf("Unable to create a parser : 0x%x\n", rv);
    return NS_ERROR_FAILURE;
  }

  nsIHTMLContentSink* sink = nsnull;

  // Create the appropriate output sink
  if (outType == "text/html")
    rv = NS_New_HTML_ContentSinkStream(&sink, &outString, flags);

  else  // default to plaintext
    rv = NS_New_HTMLToTXT_SinkStream(&sink, &outString, wrapCol, flags);

  if (NS_FAILED(rv))
  {
    printf("Couldn't create new content sink: 0x%x\n", rv);
    return rv;
  }

  parser->SetContentSink(sink);
  nsIDTD* dtd = nsnull;
  if (inType == "text/xif")
    rv = NS_NewXIFDTD(&dtd);
  else
    rv = NS_NewNavHTMLDTD(&dtd);
  if (NS_FAILED(rv))
  {
    printf("Couldn't create new HTML DTD: 0x%x\n", rv);
    return rv;
  }

  parser->RegisterDTD(dtd);

  char* inTypeStr = inType.ToNewCString();
  rv = parser->Parse(inString, 0, inTypeStr, PR_FALSE, PR_TRUE);
  delete[] inTypeStr;
  if (NS_FAILED(rv))
  {
    printf("Parse() failed! 0x%x\n", rv);
    return rv;
  }

  NS_IF_RELEASE(dtd);
  NS_IF_RELEASE(sink);
  NS_RELEASE(parser);

  if (compareAgainst.Length() > 0)
    return Compare(outString, compareAgainst);

  char* charstar = outString.ToNewUTF8String();
  printf("Output string is:\n--------------------\n%s--------------------\n",
         charstar);
  delete[] charstar;

  return NS_OK;
}

//----------------------------------------------------------------------

int main(int argc, char** argv)
{
  nsString inType ("text/html");
  nsString outType ("text/plain");
  int wrapCol = 72;
  int flags = 0;
  nsString compareAgainst;


  // Skip over progname arg:
  const char* progname = argv[0];
  --argc; ++argv;

  // Process flags
  while (argc > 0 && argv[0][0] == '-')
  {
    switch (argv[0][1])
    {
      case 'h':
        printf("\
Usage: %s [-i intype] [-o outtype] [-f flags] [-w wrapcol] [-c comparison_file] infile\n\
\tIn/out types are mime types (e.g. text/html)\n\
\tcomparison_file is a file against which to compare the output\n\
\n\
\tDefaults are -i text/html -o text/plain -f 0 -w 72 [stdin]\n",
               progname);
        exit(0);

        case 'i':
        if (argv[0][2] != '\0')
          inType = argv[0]+2;
        else {
          inType = argv[1];
          --argc;
          ++argv;
        }
        break;

      case 'o':
        if (argv[0][2] != '\0')
          outType = argv[0]+2;
        else {
          outType = argv[1];
          --argc;
          ++argv;
        }
        break;

      case 'w':
        if (isdigit(argv[0][2]))
          wrapCol = atoi(argv[0]+2);
        else {
          wrapCol = atoi(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'f':
        if (isdigit(argv[0][2]))
          flags = atoi(argv[0]+2);
        else {
          flags = atoi(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'c':
        if (argv[0][2] != '\0')
          compareAgainst = argv[0]+2;
        else {
          compareAgainst = argv[1];
          --argc;
          ++argv;
        }
        break;
    }
    ++argv;
    --argc;
  }

  FILE* file = 0;
  if (argc > 0)         // read from a file
  {
    // Open the file in a Unix-centric way,
    // until I find out how to use nsFileSpec:
    file = fopen(argv[0], "r");
    if (!file)
    {
      fprintf(stderr, "Can't open file %s", argv[0]);
      perror(" ");
      exit(1);
    }
  }
  else file = stdin;

  nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, 0);
  NS_SetupRegistry();

  // Read in the string: very inefficient, but who cares?
  nsString inString;
  char c;
  while ((c = getc(file)) != EOF)
    inString += c;

  if (file != stdin)
    fclose(file);

  return HTML2text(inString, inType, outType, flags, wrapCol, compareAgainst);
}
