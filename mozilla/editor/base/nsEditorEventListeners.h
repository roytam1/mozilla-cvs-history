/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef editorInterfaces_h__
#define editorInterfaces_h__

#include "nsCOMPtr.h"


#include "nsIDOMEvent.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMFocusListener.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"

/** The nsTextEditorKeyListener public nsIDOMKeyListener
 *  This class will delegate events to its editor according to the translation
 *  it is responsible for.  i.e. 'c' becomes a keydown, but 'ESC' becomes nothing.
 */
class nsTextEditorKeyListener : public nsIDOMKeyListener {
public:
  /** the default constructor
   */
  nsTextEditorKeyListener();
  /** the default destructor. virtual due to the possibility of derivation.
   */
  virtual ~nsTextEditorKeyListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN interfaces in to the keylister base interface. must be supplied to handle pure virtual interfaces
  see the nsIDOMKeyListener interface implementation for details
  */
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent);
/*END interfaces from nsIDOMKeyListener*/

protected:
  virtual nsresult ProcessShortCutKeys(nsIDOMEvent* aKeyEvent, PRBool& aProcessed);
  virtual nsresult ScrollSelectionIntoView();

protected:
  nsIEditor*     mEditor;		// weak reference
};


/** editor Implementation of the TextListener interface
 */
class nsTextEditorTextListener : public nsIDOMTextListener
{
public:
  /** default constructor
   */
  nsTextEditorTextListener();
  /** default destructor
   */
  virtual ~nsTextEditorTextListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of textevent handler interface*/
    virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
    virtual nsresult HandleText(nsIDOMEvent* aTextEvent);
/*END implementations of textevent handler interface*/

protected:
  nsIEditor*      mEditor;		// weak reference
	PRBool					mCommitText;
	PRBool					mInTransaction;
};


class nsIEditorIMESupport;

class nsTextEditorCompositionListener : public nsIDOMCompositionListener
{
public:
  /** default constructor
   */
  nsTextEditorCompositionListener();
  /** default destructor
   */
  virtual ~nsTextEditorCompositionListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor);

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of textevent handler interface*/
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  virtual nsresult HandleEndComposition(nsIDOMEvent* aCompositionEvent);
  virtual nsresult HandleQueryComposition(nsIDOMEvent* aCompositionEvent);
/*END implementations of textevent handler interface*/

protected:
  nsIEditorIMESupport*     mEditor;		// weak reference
};


/** editor Implementation of the MouseListener interface
 */
class nsTextEditorMouseListener : public nsIDOMMouseListener 
{
public:
  /** default constructor
   */
  nsTextEditorMouseListener();
  /** default destructor
   */
  virtual ~nsTextEditorMouseListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of mouseevent handler interface*/
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent);
/*END implementations of mouseevent handler interface*/

protected:
  nsIEditor*     mEditor;		// weak reference

};


/** editor Implementation of the DragListener interface
 */
class nsTextEditorDragListener : public nsIDOMDragListener 
{
public:
  /** default constructor
   */
  nsTextEditorDragListener();
  /** default destructor
   */
  virtual ~nsTextEditorDragListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of dragevent handler interface*/
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult DragEnter(nsIDOMEvent* aDragEvent);
  virtual nsresult DragOver(nsIDOMEvent* aDragEvent);
  virtual nsresult DragExit(nsIDOMEvent* aDragEvent);
  virtual nsresult DragDrop(nsIDOMEvent* aDragEvent);
  virtual nsresult DragGesture(nsIDOMEvent* aDragEvent);
/*END implementations of dragevent handler interface*/

protected:
  nsIEditor*    mEditor;

};

/** editor Implementation of the FocusListener interface
 */
class nsTextEditorFocusListener : public nsIDOMFocusListener 
{
public:
  /** default constructor
   */
  nsTextEditorFocusListener();
  /** default destructor
   */
  virtual ~nsTextEditorFocusListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of focus event handler interface*/
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult Focus(nsIDOMEvent* aEvent);
  virtual nsresult Blur(nsIDOMEvent* aEvent);
/*END implementations of focus event handler interface*/

protected:
  nsIEditor*     mEditor;		// weak reference
};



/** factory for the editor key listener
 */
extern nsresult NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, nsIEditor *aEditor);

/** factory for the editor mouse listener
 */
extern nsresult NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, nsIEditor *aEditor);

/** factory for the editor text listener
 */
extern nsresult NS_NewEditorTextListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor *aEditor);

/** factory for the editor drag listener
 */
extern nsresult NS_NewEditorDragListener(nsIDOMEventListener ** aInstancePtrResult, nsIEditor *aEditor);

/** factory for the editor composition listener 
 */
extern nsresult NS_NewEditorCompositionListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor *aEditor);

/** factory for the editor composition listener 
 */
extern nsresult NS_NewEditorFocusListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor *aEditor);

#endif //editorInterfaces_h__

