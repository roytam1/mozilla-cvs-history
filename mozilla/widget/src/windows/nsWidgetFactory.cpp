/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsdefs.h"
#include "nsWidgetsCID.h"

#include "nsButton.h"
#include "nsCheckButton.h"
#include "nsComboBox.h"
#include "nsFileWidget.h"
#include "nsListBox.h"
#include "nsLookAndFeel.h"
#include "nsRadioButton.h"
#include "nsRadioGroup.h"
#include "nsScrollbar.h"
#include "nsTextAreaWidget.h"
#include "nsTextHelper.h"
#include "nsTextWidget.h"
#include "nsToolkit.h"
#include "nsTabWidget.h"
#include "nsTooltipWidget.h"
#include "nsWindow.h"
#include "nsDialog.h"
#include "nsLabel.h"
#include "nsAppShell.h"

static NS_DEFINE_IID(kCWindow,        NS_WINDOW_CID);
static NS_DEFINE_IID(kCChild,         NS_CHILD_CID);
static NS_DEFINE_IID(kCButton,        NS_BUTTON_CID);
static NS_DEFINE_IID(kCCheckButton,   NS_CHECKBUTTON_CID);
static NS_DEFINE_IID(kCCombobox,      NS_COMBOBOX_CID);
static NS_DEFINE_IID(kCFileOpen,      NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kCListbox,       NS_LISTBOX_CID);
static NS_DEFINE_IID(kCRadioButton,   NS_RADIOBUTTON_CID);
static NS_DEFINE_IID(kCRadioGroup,    NS_RADIOGROUP_CID);
static NS_DEFINE_IID(kCHorzScrollbar, NS_HORZSCROLLBAR_CID);
static NS_DEFINE_IID(kCVertScrollbar, NS_VERTSCROLLBAR_CID);
static NS_DEFINE_IID(kCTextArea,      NS_TEXTAREA_CID);
static NS_DEFINE_IID(kCTextField,     NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kCTabWidget,     NS_TABWIDGET_CID);
static NS_DEFINE_IID(kCTooltipWidget, NS_TOOLTIPWIDGET_CID);
static NS_DEFINE_IID(kCAppShell,      NS_APPSHELL_CID);
static NS_DEFINE_IID(kCLookAndFeel,   NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kCDialog,        NS_DIALOG_CID);
static NS_DEFINE_IID(kCLabel,         NS_LABEL_CID);

static NS_DEFINE_IID(kISupportsIID,   NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID,    NS_IFACTORY_IID);

class nsWidgetFactory : public nsIFactory
{   
public:   
    // nsISupports methods   
    NS_IMETHOD QueryInterface(const nsIID &aIID,    
                                       void **aResult);   
    NS_IMETHOD_(nsrefcnt) AddRef(void);   
    NS_IMETHOD_(nsrefcnt) Release(void);   

    // nsIFactory methods   
    NS_IMETHOD CreateInstance(nsISupports *aOuter,   
                                       const nsIID &aIID,   
                                       void **aResult);   

    NS_IMETHOD LockFactory(PRBool aLock);   

    nsWidgetFactory(const nsCID &aClass);   
    ~nsWidgetFactory();   

private:   
    nsrefcnt mRefCnt;   
    nsCID mClassID;
}; 

NS_IMPL_ADDREF(nsWidgetFactory)
NS_IMPL_RELEASE(nsWidgetFactory)
  



nsWidgetFactory::nsWidgetFactory(const nsCID &aClass)   
{   
  NS_INIT_REFCNT();
  mClassID = aClass;
}   

nsWidgetFactory::~nsWidgetFactory()   
{   
    NS_ASSERTION(mRefCnt == 0, "Reference count not zero in destructor");   
}   

nsresult nsWidgetFactory::QueryInterface(const nsIID &aIID,   
                                         void **aResult)   
{   
    if (aResult == NULL) {   
        return NS_ERROR_NULL_POINTER;   
    }   

    // Always NULL result, in case of failure   
    *aResult = NULL;   

    if (aIID.Equals(kISupportsIID)) {   
        *aResult = (void *)(nsISupports*)this;   
    } else if (aIID.Equals(kIFactoryIID)) {   
        *aResult = (void *)(nsIFactory*)this;   
    }   

    if (*aResult == NULL) {   
        return NS_NOINTERFACE;   
    }   

    AddRef(); // Increase reference count for caller   
    return NS_OK;   
}   


nsresult nsWidgetFactory::CreateInstance( nsISupports* aOuter,
                                          const nsIID &aIID,  
                                          void **aResult)  
{  
    if (aResult == NULL) {  
        return NS_ERROR_NULL_POINTER;  
    }  

    *aResult = NULL;  
  
    if (nsnull != aOuter && !aIID.Equals(kISupportsIID)) {
        // aggregation with IID != nsISupports
        return NS_ERROR_ILLEGAL_VALUE;
    }

    nsISupports *inst = nsnull;
    if (mClassID.Equals(kCWindow)) {
        inst = (nsISupports*)(nsObject*)new nsWindow();
    }
    else if (mClassID.Equals(kCChild)) {
        inst = (nsISupports*)(nsObject*)new ChildWindow();
    }
    else if (mClassID.Equals(kCButton)) {
        inst = (nsISupports*)(nsObject*)new nsButton();
    }
    else if (mClassID.Equals(kCCheckButton)) {
        inst = (nsISupports*)(nsObject*)new nsCheckButton();
    }
    else if (mClassID.Equals(kCCombobox)) {
        inst = (nsISupports*)(nsObject*)new nsComboBox();
    }
    else if (mClassID.Equals(kCRadioButton)) {
        inst = (nsISupports*)(nsObject*)new nsRadioButton();
    }
    else if (mClassID.Equals(kCRadioGroup)) {
        inst = (nsISupports*)(nsObject*)new nsRadioGroup();
    }
    else if (mClassID.Equals(kCFileOpen)) {
        inst = (nsISupports*)(nsObject*)new nsFileWidget();
    }
    else if (mClassID.Equals(kCListbox)) {
        inst = (nsISupports*)(nsObject*)new nsListBox();
    }
    else if (mClassID.Equals(kCHorzScrollbar)) {
        inst = (nsISupports*)(nsObject*)new nsScrollbar(PR_FALSE);
    }
    else if (mClassID.Equals(kCVertScrollbar)) {
        inst = (nsISupports*)(nsObject*)new nsScrollbar(PR_TRUE);
    }
    else if (mClassID.Equals(kCTextArea)) {
        inst = (nsISupports*)(nsObject*)new nsTextAreaWidget();
    }
    else if (mClassID.Equals(kCTextField)) {
        inst = (nsISupports*)(nsObject*)new nsTextWidget();
    }
    else if (mClassID.Equals(kCTabWidget)) {
        inst = (nsISupports*)(nsObject*)new nsTabWidget();
    }
    else if (mClassID.Equals(kCTooltipWidget)) {
        inst = (nsISupports*)(nsObject*)new nsTooltipWidget();
    }
    else if (mClassID.Equals(kCAppShell)) {
        inst = (nsISupports*)(nsObject*)(nsAppShell*)new nsAppShell();
    }
    else if (mClassID.Equals(kCLookAndFeel)) {
        inst = (nsISupports*)(nsObject*)new nsLookAndFeel();
    }
    else if (mClassID.Equals(kCDialog)) {
        inst = (nsISupports*)(nsObject*)new nsDialog();
    }
    else if (mClassID.Equals(kCLabel)) {
        inst = (nsISupports*)(nsObject*)new nsLabel();
    }/* */
  
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
extern "C" NS_WIDGET nsresult NSGetFactory(const nsCID &aClass, nsIFactory **aFactory)
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


