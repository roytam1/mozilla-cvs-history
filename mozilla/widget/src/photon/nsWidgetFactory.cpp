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
 * The Initial Developer of the Original Code is Christopher
 * Blizzard.  Portions created by Christopher Blizzard are
 * Copyright (C) 2000 Christopher Blizzard. All Rights Reserved.
 *
 * Contributor(s): 
 *   Christopher Blizzzard <blizzard@mozilla.org>
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include "nsWindow.h"
#include "nsButton.h"
//#include "nsCheckButton.h"
#include "nsFileWidget.h"
#include "nsTextWidget.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsLookAndFeel.h"
//#include "nsLabel.h"
#include "nsTransferable.h"
#include "nsClipboard.h"
#include "nsXIFFormatConverter.h"
#include "nsFontRetrieverService.h"
#include "nsDragService.h"
#include "nsFileSpecWithUIImpl.h"
#include "nsScrollbar.h"
#include "nsSound.h"

#ifdef IBMBIDI
#include "nsBidiKeyboard.h"
#endif

#include <prlog.h>
struct PRLogModuleInfo  *PhWidLog =  nsnull;
#include "nsPhWidgetLog.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsButton)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsCheckButton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileWidget)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTextWidget)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsLabel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXIFFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontRetrieverService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileSpecWithUIImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)

static nsresult nsHorizScrollbarConstructor (nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;
  nsISupports *inst = nsnull;

  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsWidgetFactory::nsHorizScrollbarConstructor\n"));
  
  if ( NULL == aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = NULL;
  if (NULL != aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }
  
  inst = (nsISupports *)(nsBaseWidget *)(nsWidget *)new nsScrollbar(PR_FALSE);
  if (inst == NULL)
  {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

static nsresult nsVertScrollbarConstructor (nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;
  nsISupports *inst = nsnull;

  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsWidgetFactory::nsVertScrollbarConstructor\n"));
  if ( NULL == aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = NULL;
  if (NULL != aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }
  
  inst = (nsISupports *)(nsBaseWidget *)(nsWidget *)new nsScrollbar(PR_TRUE);
  if (inst == NULL)
  {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

static nsModuleComponentInfo components[] =
{
  { "Ph nsWindow",
    NS_WINDOW_CID,
    "mozilla.widgets.window.ph.1",
    nsWindowConstructor },
  { "Ph Child nsWindow",
    NS_CHILD_CID,
    "mozilla.widgets.child_window.ph.1",
    ChildWindowConstructor },
  { "Ph Button",
    NS_BUTTON_CID,
    "mozilla.widgets.button.ph.1",
    nsButtonConstructor },
/*
  { "Ph Check Button",
    NS_CHECKBUTTON_CID,
    "mozilla.widgets.checkbutton.ph.1",
    nsCheckButtonConstructor },
*/
  { "Ph File Widget",
    NS_FILEWIDGET_CID,
    "mozilla.widgets.filewidget.ph.1",
    nsFileWidgetConstructor },
  { "Ph Horiz Scrollbar",
    NS_HORZSCROLLBAR_CID,
    "mozilla.widgets.horizscroll.ph.1",
    nsHorizScrollbarConstructor },
  { "Ph Vert Scrollbar",
    NS_VERTSCROLLBAR_CID,
    "mozilla.widgets.vertscroll.ph.1",
    nsVertScrollbarConstructor },
  { "Ph Text Widget",
    NS_TEXTFIELD_CID,
    "mozilla.widgets.textwidget.ph.1",
    nsTextWidgetConstructor },
  { "Ph AppShell",
    NS_APPSHELL_CID,
    "mozilla.widget.appshell.ph.1",
    nsAppShellConstructor },
  { "Ph Toolkit",
    NS_TOOLKIT_CID,
    "mozilla.widget.toolkit.ph.1",
    nsToolkitConstructor },
  { "Ph Look And Feel",
    NS_LOOKANDFEEL_CID,
    "mozilla.widget.lookandfeel.ph.1",
    nsLookAndFeelConstructor },
/*
  { "Ph Label",
    NS_LABEL_CID,
    "mozilla.widget.label.ph.1",
    nsLabelConstructor },
*/
  { "Ph Sound",
    NS_SOUND_CID,
    //    "mozilla.widget.sound.ph.1"
    "component://netscape/sound",
    nsSoundConstructor },
  { "Transferrable",
    NS_TRANSFERABLE_CID,
    //    "mozilla.widget.transferrable.ph.1",
    "component://netscape/widget/transferable",
    nsTransferableConstructor },
  { "Ph Clipboard",
    NS_CLIPBOARD_CID,
    //    "mozilla.widget.clipboard.ph.1",
    "component://netscape/widget/clipboard",
    nsClipboardConstructor },
  { "XIF Format Converter",
    NS_XIFFORMATCONVERTER_CID,
    "mozilla.widget.xifformatconverter.ph.1",
    nsXIFFormatConverterConstructor },
  { "Ph Font Retriever Service",
    NS_FONTRETRIEVERSERVICE_CID,
    "mozilla.widget.fontretrieverservice.ph.1",
    nsFontRetrieverServiceConstructor },
  { "Ph Drag Service",
    NS_DRAGSERVICE_CID,
    //    "mozilla.widget.dragservice.ph.1",
    "component://netscape/widget/dragservice",
    nsDragServiceConstructor },
#ifdef IBMBIDI
    { "Ph Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "component://netscape/widget/bidikeyboard",
    nsBidiKeyboardConstructor },
#endif // IBMBIDI
  { "File Spec with UI",
    NS_FILESPECWITHUI_CID,
    //    "mozilla.widget.filespecwithui.ph.1",
    "component://netscape/filespecwithui",
    nsFileSpecWithUIImplConstructor }
};


NS_IMPL_NSGETMODULE("nsWidgetPhModule", components)

