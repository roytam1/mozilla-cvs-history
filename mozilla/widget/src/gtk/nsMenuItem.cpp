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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include <gtk/gtk.h>

#include "nsMenuItem.h"
#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIWidget.h"

#include "nsGtkEventHandler.h"

#include "nsIPopUpMenu.h"

#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDOMElement.h"
#include "nsIDocumentViewer.h"
#include "nsIPresContext.h"
#include "nsIWebShell.h"
#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsresult nsMenuItem::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtr = NULL;

  if (aIID.Equals(nsIMenuItem::GetIID())) {
    *aInstancePtr = (void*)(nsIMenuItem*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)(nsIMenuItem*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIMenuListener::GetIID())) {
    *aInstancePtr = (void*)(nsIMenuListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsMenuItem)
NS_IMPL_RELEASE(nsMenuItem)


//-------------------------------------------------------------------------
//
// nsMenuItem constructor
//
//-------------------------------------------------------------------------
nsMenuItem::nsMenuItem() : nsIMenuItem()
{
  NS_INIT_REFCNT();
  mMenuItem    = nsnull;
  mMenuParent  = nsnull;
  mPopUpParent = nsnull;
  mTarget      = nsnull;
  mXULCommandListener = nsnull;
  mIsSeparator = PR_FALSE;
  mWebShell    = nsnull;
  mDOMElement  = nsnull;
  mIsSubMenu   = PR_FALSE;
}

//-------------------------------------------------------------------------
//
// nsMenuItem destructor
//
//-------------------------------------------------------------------------
nsMenuItem::~nsMenuItem()
{
  //g_print("nsMenuItem::~nsMenuItem called\n");
  //NS_IF_RELEASE(mTarget);
  gtk_widget_destroy(mMenuItem);
  mMenuItem = nsnull;
  //g_print("end nsMenuItem::~nsMenuItem\n");
}

//-------------------------------------------------------------------------
GtkWidget *nsMenuItem::GetNativeParent()
{
  void * voidData;
  if (nsnull != mMenuParent) {
    mMenuParent->GetNativeData(&voidData);
  } else if (nsnull != mPopUpParent) {
    mPopUpParent->GetNativeData(voidData);
  } else {
    return nsnull;
  }
  return GTK_WIDGET(voidData);
}


//-------------------------------------------------------------------------
nsIWidget * nsMenuItem::GetMenuBarParent(nsISupports * aParent)
{
  nsIWidget    * widget  = nsnull; // MenuBar's Parent
  nsIMenu      * menu    = nsnull;
  nsIMenuBar   * menuBar = nsnull;
  nsIPopUpMenu * popup   = nsnull;
  nsISupports  * parent  = aParent;
  
  while(1) {
    if (NS_OK == parent->QueryInterface(nsIMenu::GetIID(),(void**)&menu)) {
      NS_RELEASE(parent);
      if (NS_OK != menu->GetParent(parent)) {
        NS_RELEASE(menu);
        return nsnull;
      }
      NS_RELEASE(menu);

    } else if (NS_OK == parent->QueryInterface(nsIPopUpMenu::GetIID(),(void**)&popup)) {
      if (NS_OK != popup->GetParent(widget)) {
        widget =  nsnull;
      } 
      NS_RELEASE(popup);
      NS_RELEASE(parent);
      return widget;

    } else if (NS_OK == parent->QueryInterface(nsIMenuBar::GetIID(),(void**)&menuBar)) {
      if (NS_OK != menuBar->GetParent(widget)) {
        widget =  nsnull;
      } 
      NS_RELEASE(menuBar);
      NS_RELEASE(parent);
      return widget;
    } else {
      NS_RELEASE(parent);
      return nsnull;
    }
  }
  return nsnull;
}

GtkWidget*
nsMenuItem::CreateLocalized(const nsString& aLabel)
{
  nsresult result;
  static nsIUnicodeEncoder* converter = nsnull;
  static int isLatin1 = 0;
  static int initialized = 0;
  if (!initialized) {
    initialized = 1;
    result = NS_ERROR_FAILURE;
    NS_WITH_SERVICE(nsIPlatformCharset, platform, NS_PLATFORMCHARSET_PROGID,
      &result);
    if (platform && NS_SUCCEEDED(result)) {
      nsAutoString charset("");
      result = platform->GetCharset(kPlatformCharsetSel_Menu, charset);
      if (NS_SUCCEEDED(result) && (charset.Length() > 0)) {
        if (!charset.Compare("iso-8859-1", PR_TRUE)) {
	  isLatin1 = 1;
	}
	NS_WITH_SERVICE(nsICharsetConverterManager, manager,
	  NS_CHARSETCONVERTERMANAGER_PROGID, &result);
	if (manager && NS_SUCCEEDED(result)) {
	  result = manager->GetUnicodeEncoder(&charset, &converter);
	  if (NS_FAILED(result) && converter) {
	    NS_RELEASE(converter);
	    converter = nsnull;
	  }
	  else if (converter) {
	    result = converter->SetOutputErrorBehavior(
	      nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');
	  }
	}
      }
    }
  }

  GtkWidget* menuItem = nsnull;

  if (converter) {
    char labelStr[128];
    labelStr[0] = 0;
    PRInt32 srcLen = aLabel.Length() + 1;
    PRInt32 destLen = sizeof(labelStr);
    result = converter->Convert(aLabel.GetUnicode(), &srcLen, labelStr,
      &destLen);
    if (labelStr[0] && NS_SUCCEEDED(result)) {
      menuItem = gtk_menu_item_new_with_label(labelStr);
      if (menuItem && (!isLatin1)) {
        GtkWidget* label = GTK_BIN(menuItem)->child;
        gtk_widget_ensure_style(label);
        GtkStyle* style = gtk_style_copy(label->style);
        gdk_font_unref(style->font);
        style->font = gdk_fontset_load("*");
        gtk_widget_set_style(label, style);
        gtk_style_unref(style);
      }
    }
  }
  else {
    char labelStr[128];
    aLabel.ToCString(labelStr, sizeof(labelStr));
    menuItem = gtk_menu_item_new_with_label(labelStr);
  }

  return menuItem;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::Create(nsISupports *aParent, 
                             const nsString &aLabel, 
                             PRBool aIsSeparator)
                            
{
  if (nsnull == aParent) {
    return NS_ERROR_FAILURE;
  }

  if(aParent) {
    nsIMenu * menu;
    aParent->QueryInterface(nsIMenu::GetIID(), (void**) &menu);
    mMenuParent = menu;
    NS_RELEASE(menu);
  }

  nsIWidget   *widget = nsnull; // MenuBar's Parent
  nsISupports *sups;
  if (NS_OK == aParent->QueryInterface(kISupportsIID,(void**)&sups)) {
    widget = GetMenuBarParent(sups);
    // GetMenuBarParent will call release for us
    // NS_RELEASE(sups);
    mTarget = widget;
  }

  mIsSeparator = aIsSeparator;
  mLabel = aLabel;

  // create the native menu item

  if(mIsSeparator) {
    mMenuItem = gtk_menu_item_new();
  } else {
    mMenuItem = CreateLocalized(aLabel);
  }
  
  gtk_widget_show(mMenuItem);
 
  gtk_signal_connect(GTK_OBJECT(mMenuItem), "activate",
                     GTK_SIGNAL_FUNC(menu_item_activate_handler),
                     this);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetLabel(nsString &aText)
{
  aText = mLabel;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetLabel(nsString &aText)
{
  mLabel = aText;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetEnabled(PRBool aIsEnabled)
{
  gtk_widget_set_sensitive(GTK_WIDGET(mMenuItem), aIsEnabled);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetEnabled(PRBool *aIsEnabled)
{
  *aIsEnabled = GTK_WIDGET_IS_SENSITIVE(mMenuItem);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetChecked(PRBool aIsEnabled)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetChecked(PRBool *aIsEnabled)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetCheckboxType(PRBool aIsCheckbox)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetCheckboxType(PRBool *aIsCheckbox)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetCommand(PRUint32 & aCommand)
{
  aCommand = mCommand;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetTarget(nsIWidget *& aTarget)
{
  aTarget = mTarget;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetNativeData(void *& aData)
{
  aData = (void *)mMenuItem;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::AddMenuListener(nsIMenuListener * aMenuListener)
{
 
  NS_IF_RELEASE(mXULCommandListener);
  NS_IF_ADDREF(aMenuListener);
  mXULCommandListener = aMenuListener;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::RemoveMenuListener(nsIMenuListener * aMenuListener)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::IsSeparator(PRBool & aIsSep)
{
  aIsSep = mIsSeparator;
  return NS_OK;
}

//-------------------------------------------------------------------------
// nsIMenuListener interface
//-------------------------------------------------------------------------
nsEventStatus nsMenuItem::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  if(!mIsSeparator) {
    //g_print("nsMenuItem::MenuItemSelected\n");
    DoCommand();
  }else{
    //g_print("nsMenuItem::MenuItemSelected is separator\n");
  }
  return nsEventStatus_eIgnore;
}
//-------------------------------------------------------------------------
nsEventStatus nsMenuItem::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  if(mXULCommandListener)
    return mXULCommandListener->MenuSelected(aMenuEvent);

  //g_print("nsMenuItem::MenuSelected\n");
  return nsEventStatus_eIgnore;
}
//-------------------------------------------------------------------------
nsEventStatus nsMenuItem::MenuDeselected(const nsMenuEvent &aMenuEvent)
{
  //g_print("nsMenuItem::MenuDeselected\n");
  return nsEventStatus_eIgnore;
}
//-------------------------------------------------------------------------
nsEventStatus nsMenuItem::MenuConstruct(const nsMenuEvent &aMenuEvent,
                                        nsIWidget *aParentWindow, 
                                        void *menuNode,
                                        void *aWebShell)
{
  //g_print("nsMenuItem::MenuConstruct\n");
  return nsEventStatus_eIgnore;
}
//-------------------------------------------------------------------------
nsEventStatus nsMenuItem::MenuDestruct(const nsMenuEvent &aMenuEvent)
{
  //g_print("nsMenuItem::MenuDestruct\n");
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
/**
 * Sets the JavaScript Command to be invoked when a "gui" event
 * occurs on a source widget
 * @param aStrCmd the JS command to be cached for later execution
 * @return NS_OK 
 */
NS_METHOD nsMenuItem::SetCommand(const nsString &aStrCmd)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
/**
 * Executes the "cached" JavaScript Command 
 * @return NS_OK if the command was executed properly, otherwise an error code
 */
NS_METHOD nsMenuItem::DoCommand()
{
  nsresult rv = NS_ERROR_FAILURE;
 
  if(!mWebShell || !mDOMElement)
    return rv;
    
  nsCOMPtr<nsIContentViewerContainer> contentViewerContainer;
  contentViewerContainer = do_QueryInterface(mWebShell);
  if (!contentViewerContainer) {
    NS_ERROR("Webshell doesn't support the content viewer container interface");
    //g_print("Webshell doesn't support the content viewer container interface");
    return rv;
  }

  nsCOMPtr<nsIContentViewer> contentViewer;
  if (NS_FAILED(rv = contentViewerContainer->GetContentViewer(getter_AddRefs(contentViewer)))) {
    NS_ERROR("Unable to retrieve content viewer.");
    //g_print("Unable to retrieve content viewer.");
    return rv;
  }

  nsCOMPtr<nsIDocumentViewer> docViewer;
  docViewer = do_QueryInterface(contentViewer);
  if (!docViewer) {
    NS_ERROR("Document viewer interface not supported by the content viewer.");
    //g_print("Document viewer interface not supported by the content viewer.");
    return rv;
  }

  nsCOMPtr<nsIPresContext> presContext;
  if (NS_FAILED(rv = docViewer->GetPresContext(*getter_AddRefs(presContext)))) {
    NS_ERROR("Unable to retrieve the doc viewer's presentation context.");
    //g_print("Unable to retrieve the doc viewer's presentation context.");
    return rv;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_MOUSE_EVENT;
  event.message = NS_MENU_ACTION;

  nsCOMPtr<nsIContent> contentNode;
  contentNode = do_QueryInterface(mDOMElement);
  if (!contentNode) {
    NS_ERROR("DOM Node doesn't support the nsIContent interface required to handle DOM events.");
    //g_print("DOM Node doesn't support the nsIContent interface required to handle DOM events.");
    return rv;
  }

  rv = contentNode->HandleDOMEvent(*presContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
  //g_print("HandleDOMEvent called");
  return rv;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetDOMNode(nsIDOMNode * aDOMNode)
{
  return NS_OK;
}
    
//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetDOMNode(nsIDOMNode ** aDOMNode)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetDOMElement(nsIDOMElement * aDOMElement)
{
  mDOMElement = aDOMElement;
  return NS_OK;
}
    
//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetDOMElement(nsIDOMElement ** aDOMElement)
{
  return NS_OK;
}
    
//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::SetWebShell(nsIWebShell * aWebShell)
{
  mWebShell = aWebShell;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::SetShortcutChar(const nsString &aText)
{
  mKeyEquivalent = aText;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::GetShortcutChar(nsString &aText)
{
  aText = mKeyEquivalent;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::SetModifiers(PRUint8 aModifiers)
{
  mModifiers = aModifiers;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::GetModifiers(PRUint8 * aModifiers)
{
  *aModifiers = mModifiers; 
  return NS_OK;
}
