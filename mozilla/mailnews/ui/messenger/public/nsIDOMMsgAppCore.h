/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIDOMMsgAppCore.idl
 */

#ifndef __gen_nsIDOMMsgAppCore_h__
#define __gen_nsIDOMMsgAppCore_h__

#include "nsISupports.h" /* interface nsISupports */
#include "nsIDOMWindow.h" /* interface nsIDOMWindow */
#include "nsIDOMBaseAppCore.h" /* interface nsIDOMBaseAppCore */


/* starting interface nsIDOMMsgAppCore */

/* {4f7966d0-c14f-11d2-b7f2-00805f05ffa5} */
#define NS_IDOMMSGAPPCORE_IID_STR "4f7966d0-c14f-11d2-b7f2-00805f05ffa5"
#define NS_IDOMMSGAPPCORE_IID \
  {0x4f7966d0, 0xc14f, 0x11d2, \
    { 0xb7, 0xf2, 0x00, 0x80, 0x5f, 0x05, 0xff, 0xa5 }}

class nsIDOMMsgAppCore : public nsIDOMBaseAppCore {
 public: 
  static const nsIID& IID() {
    static nsIID iid = NS_IDOMMSGAPPCORE_IID;
    return iid;
  }

  /* void GetNewMail (); */
  NS_IMETHOD GetNewMail() = 0;

  /* void Open3PaneWindow (); */
  NS_IMETHOD Open3PaneWindow() = 0;

  /* void SetWindow (in nsIDOMWindow ptr); */
  NS_IMETHOD SetWindow(nsIDOMWindow *ptr) = 0;

  /* void OpenURL (in string str); */
  NS_IMETHOD OpenURL(nsAutoString& str) = 0;
};


extern "C" NS_DOM
nsresult
NS_InitMsgAppCoreClass(nsIScriptContext *aContext, void **aPrototype);

#endif /* __gen_nsIDOMMsgAppCore_h__ */

