/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


#include "nsICmdLineService.h"
#include "nsCommandLineService.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "plstr.h"

nsCmdLineService::nsCmdLineService()
	:  mArgCount(0), mArgc(0), mArgv(0)
{
  NS_INIT_REFCNT();
}

/*
 * Implement the nsISupports methods...
 */
NS_IMPL_ISUPPORTS1(nsCmdLineService, nsICmdLineService);

NS_IMETHODIMP
nsCmdLineService::Initialize(int aArgc, char ** aArgv)
{


  PRInt32   i=0;
  nsresult  rv = nsnull;

  // Save aArgc and argv
  mArgc = aArgc;
  mArgv = aArgv;
  //Insert the program name 
  if (aArgv[0])
  {
    mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("-progname")));
    mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[0])));
    mArgCount++;
    i++;
  }

  for(i=1; i<aArgc; i++) {

    if ((aArgv[i][0] == '-') || (aArgv[i][0] == '/')) {
       /* An option that starts with -. May or many not
	    * have a value after it. 
	    */
	   mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	   //Increment the index to look ahead at the next option.
       i++;


     //Look ahead if this option has a value like -w 60

	   if (i == aArgc) {
	     /* All args have been parsed. Append a PR_TRUE for the
	      * previous option in the mArgValueList
	      */
	     mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("1")));
	     mArgCount++;
	     break;
	   }
     if ((aArgv[i][0] == '-') || (aArgv[i][0] == '/')) {
        /* An other option. The previous one didn't have a value.
         * So, store the previous one's value as PR_TRUE in the
	     * mArgValue array and retract the index so that this option 
	     * will get stored in the next iteration
	     */
        mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("1")));
   	    mArgCount++;
        i--;
        continue;
	    }
      else {
        /* The next argument does not start with '-'. This 
	     * could be value to the previous option 
	     */
	      if (i == (aArgc-1)) {
	       /* This is the last argument and a URL 
            * Append a PR_TRUE for the previous option in the value array
            */
           //mArgValueList.AppendElement((void *)PL_strdup("1"));
	         //mArgCount++;

 		       // Append the url to the arrays
           //mArgList.AppendElement((void *)PL_strdup("-url"));
		       mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	 	       mArgCount++;
           continue;
        }
	      else {
	         /* This is a value to the previous option.
	          * Store it in the mArgValue array 
	          */
             mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	         mArgCount++;
	      }
	   }
  }
  else {
       if (i == (aArgc-1)) {
	      /* This must be the  URL at the end 
	       * Append the url to the arrays
           */
           mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("-url")));
	         mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	         mArgCount++;
	     }
	     else {
	       /* A bunch of unrecognized arguments */
	       rv = NS_ERROR_INVALID_ARG;
	     }
  }
	
 }  // for

#if 0
  for (i=0; i<mArgCount; i++)
  {
       printf("Argument: %s, ****** Value: %s\n", (char *)mArgList.ElementAt(i), (char *) mArgValueList.ElementAt(i));      
  }
#endif /* 0 */

   return rv;
	
}

NS_IMETHODIMP
nsCmdLineService::GetURLToLoad(char ** aResult)
{

   return GetCmdLineValue("-url", aResult);
}

NS_IMETHODIMP
nsCmdLineService::GetProgramName(char ** aResult)
{
  nsresult  rv=nsnull;

  *aResult = (char *)mArgValueList.ElementAt(0);

  return rv;

}

PRBool nsCmdLineService::ArgsMatch(const char *lookingFor, const char *userGave)
{
    if (!lookingFor || !userGave) return PR_FALSE;

    if (!PL_strcasecmp(lookingFor,userGave)) return PR_TRUE;

#if defined(XP_UNIX) || defined(XP_BEOS)
    /* on unix and beos, we'll allow --mail for -mail */
    if ((PL_strlen(lookingFor) > 0) && (PL_strlen(userGave) > 1)) {
        if (!PL_strcasecmp(lookingFor+1,userGave+2) && (lookingFor[0] == '-') && (userGave[0] == '-') && (userGave[1] == '-')) return PR_TRUE;
    }
#endif
#ifdef XP_PC
    /* on windows /mail is the same as -mail */
    if ((PL_strlen(lookingFor) > 0) && (PL_strlen(userGave) > 0)) {
        if (!PL_strcasecmp(lookingFor+1,userGave+1) && (lookingFor[0] == '-') && (userGave[0] == '/')) return PR_TRUE;
    }
#endif 
    return PR_FALSE;
}

NS_IMETHODIMP
nsCmdLineService::GetCmdLineValue(const char * aArg, char ** aResult)
{
   nsresult  rv = NS_OK;
   
   if (nsnull == aArg || nsnull == aResult ) {
	    return NS_ERROR_NULL_POINTER;
   }

   for (int i = 0; i<mArgCount; i++)
   {
     if (ArgsMatch(aArg,(char *) mArgList.ElementAt(i))) {
       *aResult = (char *)mArgValueList.ElementAt(i);
        return NS_OK;
     }
   }

   *aResult = nsnull;
   return rv;
	
}

NS_IMETHODIMP
nsCmdLineService::GetArgc(PRInt32 * aResult)
{

    if (nsnull == aResult)
        return NS_ERROR_NULL_POINTER;

    // if we are null, we were never initialized.
    if (mArgc == 0)
      return NS_ERROR_FAILURE;

    *aResult =  mArgc;
    return NS_OK;
}

NS_IMETHODIMP
nsCmdLineService::GetArgv(char *** aResult)
{
    if (nsnull == aResult)
      return NS_ERROR_NULL_POINTER;

    // if we are 0, we were never set.
    if (!mArgv)
      return NS_ERROR_FAILURE;

    *aResult = mArgv;

    return NS_OK;
}

nsCmdLineService::~nsCmdLineService()
{
  PRInt32 curr = mArgList.Count();
  while ( curr ) {
    char* str = NS_REINTERPRET_CAST(char*, mArgList[curr-1]);
    if ( str )
      nsAllocator::Free(str);
    --curr;
  }
  
  curr = mArgValueList.Count();
  while ( curr ) {
    char* str = NS_REINTERPRET_CAST(char*, mArgValueList[curr-1]);
    if ( str )
      nsAllocator::Free(str);
    --curr;
  }
}



#if 0
NS_IMETHODIMP
nsCmdLineService::PrintCmdArgs()
{

   if (mArgCount == 0) {
     printf("No command line options provided\n");
     return;
   }
   
   for (int i=0; i<mArgCount; i++)
   {
       printf("Argument: %s, ****** Value: %s\n", mArgList.ElementAt(i), mArgValueList.ElementAt(i));      

   }

  return NS_OK;

}
#endif

NS_EXPORT nsresult NS_NewCmdLineService(nsICmdLineService** aResult)
{
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = new nsCmdLineService();
  if (nsnull == *aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult);
  return NS_OK;
}

