/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsFontRetrieverService.h"
#include "nsIWidget.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
 
#include "nsFont.h"
#include "nsVoidArray.h"
#include "nsFontSizeIterator.h"
 
NS_IMPL_ADDREF(nsFontRetrieverService)
NS_IMPL_RELEASE(nsFontRetrieverService)


//----------------------------------------------------------
nsFontRetrieverService::nsFontRetrieverService()
{
  NS_INIT_REFCNT();

  mFontList     = nsnull;
  mSizeIter     = nsnull;
  mNameIterInx  = 0;

}

//----------------------------------------------------------
nsFontRetrieverService::~nsFontRetrieverService()
{
  if (nsnull != mFontList) {
    for (PRInt32 i=0;i<mFontList->Count();i++) {
      FontInfo * font = (FontInfo *)mFontList->ElementAt(i);
      if (font->mSizes) {
        delete font->mSizes;
      }
      delete font;
    }
    delete mFontList;
  }
  NS_IF_RELEASE(mSizeIter);
}

/**
 * @param aIID The name of the class implementing the method
 * @param _classiiddef The name of the #define symbol that defines the IID
 * for the class (e.g. NS_ISUPPORTS_IID)
 * 
*/ 
nsresult nsFontRetrieverService::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{

  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_NOINTERFACE;

  if (aIID.Equals(nsIFontRetrieverService::GetIID())) {
    *aInstancePtr = (void*) ((nsIFontRetrieverService*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(nsIFontNameIterator::GetIID())) {
    *aInstancePtr = (void*) ((nsIFontNameIterator*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return rv;
}


//----------------------------------------------------------
//-- nsIFontRetrieverService
//----------------------------------------------------------
NS_IMETHODIMP nsFontRetrieverService::CreateFontNameIterator( nsIFontNameIterator** aIterator )
{
  if (nsnull == aIterator) {
    return NS_ERROR_FAILURE;
  }

  if (nsnull == mFontList) {
    LoadFontList();
  }
  *aIterator = this;
  NS_ADDREF_THIS();
  return NS_OK;
}

//----------------------------------------------------------
NS_IMETHODIMP nsFontRetrieverService::CreateFontSizeIterator( const nsString * aFontName, 
                                                              nsIFontSizeIterator** aIterator )
{
  PRBool found = PR_FALSE;
  Reset();
  do {
    nsAutoString name;
    Get(&name);
    if (name.Equals(*aFontName)) {
      found = PR_TRUE;
      break;
    }
  } while (Advance() == NS_OK);

  if (found) {
    if (nsnull == mSizeIter) {
      mSizeIter = new nsFontSizeIterator();
    }
    NS_ASSERTION( nsnull != mSizeIter, "nsFontSizeIterator instance pointer is null");

    *aIterator = (nsIFontSizeIterator *)mSizeIter;
    NS_ADDREF(mSizeIter);

    FontInfo * fontInfo = (FontInfo *)mFontList->ElementAt(mNameIterInx);
    mSizeIter->SetFontInfo(fontInfo);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------
//-- nsIFontNameIterator
//----------------------------------------------------------
NS_IMETHODIMP nsFontRetrieverService::Reset()
{
  mNameIterInx = 0;
  return NS_OK;
}

//----------------------------------------------------------
NS_IMETHODIMP nsFontRetrieverService::Get( nsString* aFontName )
{
  if (mNameIterInx < mFontList->Count()) {
    FontInfo * fontInfo = (FontInfo *)mFontList->ElementAt(mNameIterInx);
    *aFontName = fontInfo->mName;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------
NS_IMETHODIMP nsFontRetrieverService::Advance()
{
  if (mNameIterInx < mFontList->Count()-1) {
    mNameIterInx++;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

//------------------------------
FontInfo * GetFontInfo(nsVoidArray * aFontList, char * aName)
{
  nsAutoString name(aName);
  PRInt32 i;
  PRInt32 cnt = aFontList->Count();
  for (i=0;i<cnt;i++) {
    FontInfo * fontInfo = (FontInfo *)aFontList->ElementAt(i);
    if (fontInfo->mName.Equals(name)) {
      return fontInfo;
    }
  }

  FontInfo * fontInfo   = new FontInfo();
  fontInfo->mName       = aName;
  printf("Adding [%s]\n", aName);fflush(stdout); 
  fontInfo->mIsScalable = PR_FALSE; // X fonts aren't scalable right??
  fontInfo->mSizes      = nsnull;
  aFontList->AppendElement(fontInfo);
  return fontInfo;
}

//------------------------------
void AddSizeToFontInfo(FontInfo * aFontInfo, PRInt32 aSize)
{
  nsVoidArray * sizes;
  if (nsnull == aFontInfo->mSizes) {
    sizes = new nsVoidArray();
    aFontInfo->mSizes = sizes;
  } else {
    sizes = aFontInfo->mSizes;
  }
  PRInt32 i;
  PRInt32 cnt = sizes->Count();
  for (i=0;i<cnt;i++) {
    PRInt32 size = (int)sizes->ElementAt(i);
    if (size == aSize) {
      return;
    }
  }
  sizes->AppendElement((void *)aSize);
}

//------------------------------
// XXX - Hack - Parts of this will need to be reworked
//------------------------------
NS_IMETHODIMP nsFontRetrieverService::LoadFontList()
{
  int       font_cnt;
  char    * pattern = "*";
  int nnames = 1024;

  int available = nnames+1;
  int i;
  char **fonts;
  XFontStruct *info;

  if (nsnull == mFontList) { 
    mFontList = new nsVoidArray(); 
    if (nsnull == mFontList) { 
      return NS_ERROR_FAILURE; 
    } 
  } 

  /* Get list of fonts matching pattern */
  for (;;) {

    fonts = XListFontsWithInfo(GDK_DISPLAY(), pattern, nnames, &available, &info
);

    if (fonts == NULL || available < nnames)
      break;

    XFreeFontInfo(fonts, info, available);

    nnames = available * 2;
  }

  if (fonts == NULL) {
    fprintf(stderr, "pattern \"%s\" unmatched\n", pattern);
    return NS_ERROR_FAILURE;
  }

  printf("-----------------------------\n");
  for (i=0; i<available; i++) {
    printf("[%s]i\n", fonts[i]);
  }
  printf("-----------------------------\n");
  printf("-----------------------------\n");
  char buffer[1024];
  char currentName[1024];
  // this code assumes all like fonts are grouped together
  // currentName is the current name of the font we are gathering
  // sizes for, when the name changes we create a new FontInfo object
  currentName[0]   = 0;
  FontInfo * font = nsnull;
  for (i=0; i<available; i++) {

    // This is kind of lame, but it will have to do for now
    strcpy(buffer, fonts[i]);
    printf("Font[%s]\n", buffer);fflush(stdout);

    // Start by checking to see if the name begins with a dash
    char * ptr  = buffer;
    if (buffer[0] == '-') {
      printf("Step #1 ptr[%s]\n", ptr);fflush(stdout); 
      PRInt32 cnt = 0;
      // skip first two '-'
      do {
        if (*ptr == '-') cnt++;
        ptr++;
      } while (cnt < 2);
      printf("ptr[%s]\n", ptr);fflush(stdout); 
      // find the dash at the end of the name
      char * end = strchr(ptr, '-');
      if (end) {
        *end = 0;
        printf("ptr/end[%s]\n", ptr);fflush(stdout); 
        // Check to see if we need to create a new FontInfo obj
        // and set the currentName var to this guys font name
        if (strcmp(currentName, ptr) || NULL == font) {
          font = GetFontInfo(mFontList, ptr);
          strcpy(currentName, ptr);
        }
        if (nsnull == font->mSizes) {
          font->mSizes = new nsVoidArray();
        }
        ptr = end+1; // skip past the dash that was set to zero
        printf("end+1/ptr[%s]\n", ptr);fflush(stdout); 
        cnt = 0;
        // now skip ahead 4 dashes
        do {
          if (*ptr == '-') cnt++;
          ptr++;
        } while (cnt < 4);
        printf("ptr4[%s]\n", ptr);fflush(stdout); 
        // find the dash after the size
        end = strchr(ptr, '-');
        printf("end[%s]\n", end);fflush(stdout); 
        if (end) {
	  *end = 0;
          PRInt32 size;
          printf("sccanf[%s]\n", ptr);fflush(stdout); 
          sscanf(ptr, "%d", &size);
          printf("size[%d]\n", size);fflush(stdout); 
	  AddSizeToFontInfo(font, size);
        }
      }
    } else {
      printf("Step #2\n");fflush(stdout); 
      // no leading dash means the start of the 
      // buffer is the start of the name
      // this checks for a dash at the end of the font name
      // which means there is a size at the end
      char * end = strchr(buffer, '-');
      if (end) {
        *end = 0;
        printf("buffer2[%s]\n", buffer);fflush(stdout); 
        // Check to see if we need to create a new FontInfo obj
        // and set the currentName var to this guys font name
        if (strcmp(currentName, buffer) || NULL == font) {
          font = GetFontInfo(mFontList, buffer);
          strcpy(currentName, buffer);
        }
        end++; // advance past the dash
	// check to see if we have a number
	ptr = end;
	if (isalpha(*ptr)) {
	  // skip until next dash
	  end = strchr(ptr, '-');
	  if (end) {
            *end = 0;
	    ptr = end+1;
	  }
	}
        PRInt32 size;
        // yes, it has a dash at the end so it must have the size
        // check to see if the size is terminated by a dash
        // it shouldn't be
        char * end2 = strchr(ptr, '-');
        if (end2) *end2 = 0; // put terminator at the dash
        sscanf(end, "%d", &size);
	AddSizeToFontInfo(font, size);
      } else {
        // The font has an implicit size, 
        // so there is nothing to parse for size
        // so we can't really do much here
        // Check to see if we need to create a new FontInfo obj
        // and set the currentName var to this guys font name
        if (strcmp(currentName, buffer) || NULL == font) {
          font = GetFontInfo(mFontList, buffer);
          strcpy(currentName, buffer);
        }
      } 
    }

  }
  printf("-----------------------------\n");
  printf("-----------------------------\n");
  printf("-----------------------------\n");

  //XFreeFontInfo(fonts, info, available);

  return NS_OK;
}

