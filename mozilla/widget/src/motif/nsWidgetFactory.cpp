/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *   Dan Rosen <dr@netscape.com>
 */

#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIButton.h"
#include "nsITextWidget.h"
#include "nsWidgetsCID.h"

#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsButton.h"
#include "nsScrollbar.h"
#include "nsCheckButton.h"
#include "nsRadioButton.h"
#include "nsTextWidget.h"
#include "nsTextAreaWidget.h"
#include "nsFileWidget.h"
#include "nsListBox.h"
#include "nsComboBox.h"
#include "nsLookAndFeel.h"
#include "nsLabel.h"
// #include "nsFontRetrieverService.h"

// Drag & Drop, Clipboard
// #include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"

// #include "nsDragService.h"
// #include "nsDragSource.h"
// #include "nsDragTarget.h"
// #include "nsDraggedObject.h"

#ifdef IBMBIDI
#include "nsBidiKeyboard.h"
#endif

static NS_DEFINE_IID(kCWindow,        NS_WINDOW_CID);
static NS_DEFINE_IID(kCChild,         NS_CHILD_CID);
static NS_DEFINE_IID(kCButton,        NS_BUTTON_CID);
static NS_DEFINE_IID(kCCheckButton,   NS_CHECKBUTTON_CID);
static NS_DEFINE_IID(kCCombobox,      NS_COMBOBOX_CID);
static NS_DEFINE_IID(kCFileOpen,      NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kCListbox,       NS_LISTBOX_CID);
static NS_DEFINE_IID(kCRadioButton,   NS_RADIOBUTTON_CID);
static NS_DEFINE_IID(kCHorzScrollbar, NS_HORZSCROLLBAR_CID);
static NS_DEFINE_IID(kCVertScrollbar, NS_VERTSCROLLBAR_CID);
static NS_DEFINE_IID(kCTextArea,      NS_TEXTAREA_CID);
static NS_DEFINE_IID(kCTextField,     NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kCAppShell,      NS_APPSHELL_CID);
static NS_DEFINE_IID(kCToolkit,       NS_TOOLKIT_CID);
static NS_DEFINE_IID(kCLookAndFeel,   NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kCLabel,         NS_LABEL_CID);
static NS_DEFINE_IID(kCFontRetrieverService,    NS_FONTRETRIEVERSERVICE_CID);

// Drag & Drop, Clipboard
static NS_DEFINE_IID(kCDataObj,       NS_DATAOBJ_CID);
static NS_DEFINE_IID(kCClipboard,     NS_CLIPBOARD_CID);
static NS_DEFINE_IID(kCClipboardHelper,  NS_CLIPBOARDHELPER_CID);
static NS_DEFINE_IID(kCTransferable,  NS_TRANSFERABLE_CID);
static NS_DEFINE_IID(kCDataFlavor,    NS_DATAFLAVOR_CID);
static NS_DEFINE_IID(kCHTMLFormatConverter,  NS_HTMLFORMATCONVERTER_CID);
// static NS_DEFINE_IID(kCDragService,   NS_DRAGSERVICE_CID);
// static NS_DEFINE_IID(kCDragSource,    NS_DRAGSOURCE_CID);
// static NS_DEFINE_IID(kCDragTarget,    NS_DRAGTARGET_CID);
// static NS_DEFINE_IID(kCDraggedObject, NS_DRAGGEDOBJECT_CID);

#ifdef IBMBIDI
static NS_DEFINE_IID(kCBidiKeyboard,   NS_BIDIKEYBOARD_CID);
#endif

static NS_DEFINE_IID(kISupportsIID,   NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID,    NS_IFACTORY_IID);


class nsWidgetFactory : public nsIFactory
{   
public:   

    NS_DECL_ISUPPORTS

    // nsIFactory methods   
    NS_IMETHOD CreateInstance(nsISupports *aOuter,   
                              const nsIID &aIID,   
                              void **aResult);   

    NS_IMETHOD LockFactory(PRBool aLock);   

    nsWidgetFactory(const nsCID &aClass);   
    virtual ~nsWidgetFactory();   
private:
  nsCID mClassID;

};   



nsWidgetFactory::nsWidgetFactory(const nsCID &aClass)   
{   
 NS_INIT_REFCNT();
 mClassID = aClass;
}   

nsWidgetFactory::~nsWidgetFactory()   
{   
}   

nsresult nsWidgetFactory::QueryInterface(const nsIID &aIID,   
                                         void **aResult)   
{   
  if (NULL == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = NULL;

  if (aIID.Equals(kISupportsIID)) {
    *aResult = (void *)(nsISupports *)this;
  } else if (aIID.Equals(kIFactoryIID)) {
    *aResult = (void *)(nsIFactory *)this;
  }

  if (*aResult == NULL) {
    return NS_NOINTERFACE;
  }

  NS_ADDREF_THIS();
  return NS_OK;
}   

NS_IMPL_ADDREF(nsWidgetFactory)
NS_IMPL_RELEASE(nsWidgetFactory)

nsresult nsWidgetFactory::CreateInstance(nsISupports *aOuter,  
                                          const nsIID &aIID,  
                                          void **aResult)  
{  
    if (aResult == NULL) {  
        return NS_ERROR_NULL_POINTER;  
    }  

    *aResult = NULL;  

    if (nsnull != aOuter)
      return NS_ERROR_NO_AGGREGATION;
  
    nsISupports *inst = nsnull;
    if (mClassID.Equals(kCWindow)) {
      inst = (nsISupports *)new nsWindow();
    }
    else if (mClassID.Equals(kCChild)) {
      inst = (nsISupports *)new ChildWindow();
    }
    else if (mClassID.Equals(kCButton)) {
      inst = (nsISupports*)(nsIWidget *)new nsButton();
    }
    else if (mClassID.Equals(kCCheckButton)) {
        inst = (nsISupports*)(nsIWidget *)new nsCheckButton();
    }
    else if (mClassID.Equals(kCCombobox)) {
        inst = (nsISupports*)(nsIWidget *)new nsComboBox();
    }
    else if (mClassID.Equals(kCRadioButton)) {
        inst = (nsISupports*)(nsIWidget *)new nsRadioButton();
    }
    else if (mClassID.Equals(kCFileOpen)) {
        inst = (nsISupports*)(nsIWidget *)new nsFileWidget();
    }
    else if (mClassID.Equals(kCListbox)) {
        inst = (nsISupports*)(nsIWidget *)new nsListBox();
    }
    else if (mClassID.Equals(kCHorzScrollbar)) {
        inst = (nsISupports*)(nsIWidget *)new nsScrollbar(PR_FALSE);
    }
    else if (mClassID.Equals(kCVertScrollbar)) {
        inst = (nsISupports*)(nsIWidget *)new nsScrollbar(PR_TRUE);
    }
    else if (mClassID.Equals(kCTextArea)) {
        inst = (nsISupports*)(nsIWidget *)new nsTextAreaWidget();
    }
    else if (mClassID.Equals(kCTextField)) {
        inst = (nsISupports*)(nsIWidget *)new nsTextWidget();
    }
    else if (mClassID.Equals(kCAppShell)) {
        inst = (nsISupports*)new nsAppShell();
    }
    else if (mClassID.Equals(kCToolkit)) {
        inst = (nsISupports*)(nsIWidget *)new nsToolkit();
    }
    else if (mClassID.Equals(kCLookAndFeel)) {
        inst = (nsISupports*)(nsIWidget *)new nsLookAndFeel();
    }
    else if (mClassID.Equals(kCLabel)) {
        inst = (nsISupports*)(nsIWidget *)new nsLabel();
    }
    else if (mClassID.Equals(kCTransferable)) {
        inst = (nsISupports*)new nsTransferable();
    }
//     else if (mClassID.Equals(kCClipboard)) {
//         inst = (nsISupports*)new nsClipboard();
//     }
    else if (mClassID.Equals(kCClipboardHelper)) {
        inst = (nsISupports*)new nsClipboardHelper();
    }
    else if (mClassID.Equals(kCHTMLFormatConverter)) {
        inst = (nsISupports*)new nsHTMLFormatConverter();
    }
#ifdef IBMBIDI
    else if (mClassID.Equals(kCBidiKeyboard)) {
        inst = (nsISupports*)(nsIBidiKeyboard*) new nsBidiKeyboard();
    }
#endif // IBMBIDI
//     else if (mClassID.Equals(kCFontRetrieverService)) {
//         inst = (nsISupports*)(nsIFontRetrieverService *) new nsFontRetrieverService();
//     }
#if 0
    else if (mClassID.Equals(kCDragService)) {
        inst = (nsISupports*)new nsDragService();
    }
#endif
    else {
        printf("nsWidgetFactory::CreateInstance(), unhandled class.\n");
    }
  
    if (inst == NULL) {  
        return NS_ERROR_OUT_OF_MEMORY;  
    }  

    nsresult res = inst->QueryInterface(aIID, aResult);

    if (res != NS_OK) {  
        // We didn't get the right interface, so clean up  
        delete inst;  
    }

    return res;

}  

nsresult nsWidgetFactory::LockFactory(PRBool aLock)  
{  
    // Not implemented in simplest case.  
    return NS_OK;
}  

// return the proper factory to the caller
extern "C" NS_WIDGET nsresult 
NSGetFactory(nsISupports* serviceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aContractID,
             nsIFactory **aFactory)
{
    if (nsnull == aFactory) {
        return NS_ERROR_NULL_POINTER;
    }

    *aFactory = new nsWidgetFactory(aClass);

    if (nsnull == aFactory) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}


