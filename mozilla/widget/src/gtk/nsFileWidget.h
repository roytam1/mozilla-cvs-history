/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsFileWidget_h__
#define nsFileWidget_h__

#include "nsIWidget.h"
#include "nsIFileWidget.h"

#include <gtk/gtk.h>

class nsIToolkit;

/**
 * Native GTK FileSelector wrapper
 */

class nsFileWidget : public nsIFileWidget 
{
public:
  nsFileWidget(); 
  virtual ~nsFileWidget();

  NS_DECL_ISUPPORTS

  // nsIWidget interface
  
  NS_IMETHOD		Create(nsIWidget *aParent,
                       const nsString& aTitle,
                       nsFileDlgMode aMode,
                       nsIDeviceContext *aContext = nsnull,
                       nsIAppShell *aAppShell = nsnull,
                       nsIToolkit *aToolkit = nsnull,
                       void *aInitData = nsnull);

  // nsIFileWidget part
  virtual PRBool	Show();
  NS_IMETHOD GetFile(nsFileSpec& aFile);
  NS_IMETHOD SetDefaultString(const nsString& aFile);
  NS_IMETHOD SetFilterList(PRUint32 aNumberOfFilters,
                           const nsString aTitles[],
                           const nsString aFilters[]);

  NS_IMETHOD GetDisplayDirectory(nsFileSpec& aDirectory);
  NS_IMETHOD SetDisplayDirectory(const nsFileSpec& aDirectory);
  
  virtual nsFileDlgResults GetFile(nsIWidget *aParent,
                                   const nsString &promptString,
                                   nsFileSpec &theFileSpec);
  
  virtual nsFileDlgResults GetFolder(nsIWidget *aParent,
                                     const nsString &promptString,
                                     nsFileSpec &theFileSpec);

  virtual nsFileDlgResults PutFile(nsIWidget *aParent,
                                   const nsString &promptString,
                                   nsFileSpec &theFileSpec);


  NS_IMETHOD            GetSelectedType(PRInt16& theType);

protected:
  static gint DestroySignal(GtkWidget *  aGtkWidget,
                            nsFileWidget* aWidget);

  virtual void OnDestroySignal(GtkWidget* aGtkWidget);

  GtkWidget		*mWidget;
  nsString		mTitle;
  GtkWidget   *mOptionMenu;
  GtkWidget   *mFilterMenu;


  nsFileDlgMode   mMode;
  PRUint32		    mNumberOfFilters;  
  const nsString*	mTitles;
  const nsString*	mFilters;
  nsString		mDefault;
  nsFileSpec    mDisplayDirectory;
  PRInt16       mSelectedType;
};

#endif // nsFileWidget_h__
