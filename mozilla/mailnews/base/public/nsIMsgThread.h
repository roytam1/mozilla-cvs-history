/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIMsgThread.idl
 */

#ifndef __gen_nsIMsgThread_h__
#define __gen_nsIMsgThread_h__

#include "nsISupports.h" /* interface nsISupports */
#include "MailNewsTypes.h" /* interface MailNewsTypes */
#include "nsrootidl.h" /* interface nsrootidl */
#include "nsIEnumerator.h" /* interface nsIEnumerator */

#ifdef XPIDL_JS_STUBS
#include "jsapi.h"
#endif
class nsIMsgDBHdr; /* forward decl */
#include "nsIMsgHdr.h"


/* starting interface:    nsIMsgThread */

/* {df64af90-e2da-11d2-8d6c-00805f8a6617} */
#define NS_IMSGTHREAD_IID_STR "df64af90-e2da-11d2-8d6c-00805f8a6617"
#define NS_IMSGTHREAD_IID \
  {0xdf64af90, 0xe2da, 0x11d2, \
    { 0x8d, 0x6c, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0x17 }}

class nsIMsgThread : public nsISupports {
 public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IMSGTHREAD_IID)

  /* attribute nsMsgKey threadKey; */
  NS_IMETHOD GetThreadKey(nsMsgKey *aThreadKey) = 0;
  NS_IMETHOD SetThreadKey(nsMsgKey aThreadKey) = 0;

  /* attribute unsigned long flags; */
  NS_IMETHOD GetFlags(PRUint32 *aFlags) = 0;
  NS_IMETHOD SetFlags(PRUint32 aFlags) = 0;

  /* readonly attribute unsigned long numChildren; */
  NS_IMETHOD GetNumChildren(PRUint32 *aNumChildren) = 0;

  /* readonly attribute unsigned long numUnreadChildren; */
  NS_IMETHOD GetNumUnreadChildren(PRUint32 *aNumUnreadChildren) = 0;

  /* void AddChild (in nsIMsgDBHdr child, in boolean threadInThread); */
  NS_IMETHOD AddChild(nsIMsgDBHdr *child, PRBool threadInThread) = 0;

  /* nsIMsgDBHdr GetChildAt (in long index); */
  NS_IMETHOD GetChildAt(PRInt32 index, nsIMsgDBHdr **_retval) = 0;

  /* nsIMsgDBHdr GetChild (in nsMsgKey msgKey); */
  NS_IMETHOD GetChild(nsMsgKey msgKey, nsIMsgDBHdr **_retval) = 0;

  /* nsIMsgDBHdr GetChildHdrAt (in long index); */
  NS_IMETHOD GetChildHdrAt(PRInt32 index, nsIMsgDBHdr **_retval) = 0;

  /* void RemoveChildAt (in long index); */
  NS_IMETHOD RemoveChildAt(PRInt32 index) = 0;

  /* void RemoveChild (in nsMsgKey msgKey); */
  NS_IMETHOD RemoveChild(nsMsgKey msgKey) = 0;

  /* void MarkChildRead (in boolean bRead); */
  NS_IMETHOD MarkChildRead(PRBool bRead) = 0;

  /* nsIEnumerator EnumerateMessages (in nsMsgKey parent); */
  NS_IMETHOD EnumerateMessages(nsMsgKey parent, nsIEnumerator **_retval) = 0;

#ifdef XPIDL_JS_STUBS
  static NS_EXPORT_(JSObject *) InitJSClass(JSContext *cx);
  static NS_EXPORT_(JSObject *) GetJSObject(JSContext *cx, nsIMsgThread *priv);
#endif
};

#endif /* __gen_nsIMsgThread_h__ */
