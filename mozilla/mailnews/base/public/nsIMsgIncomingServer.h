/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIMsgIncomingServer.idl
 */

#ifndef __gen_nsIMsgIncomingServer_h__
#define __gen_nsIMsgIncomingServer_h__

#include "nsISupports.h" /* interface nsISupports */
#include "nsIPref.h" /* interface nsIPref */
#include "nsID.h" /* interface nsID */

#ifdef XPIDL_JS_STUBS
#include "jsapi.h"
#endif

/* starting interface:    nsIMsgIncomingServer */

/* {60dcb100-e3f2-11d2-b7fc-00805f05ffa5} */
#define NS_IMSGINCOMINGSERVER_IID_STR "60dcb100-e3f2-11d2-b7fc-00805f05ffa5"
#define NS_IMSGINCOMINGSERVER_IID \
  {0x60dcb100, 0xe3f2, 0x11d2, \
    { 0xb7, 0xfc, 0x00, 0x80, 0x5f, 0x05, 0xff, 0xa5 }}

class nsIMsgIncomingServer : public nsISupports {
 public: 
  static const nsIID& GetIID() {
    static nsIID iid = NS_IMSGINCOMINGSERVER_IID;
    return iid;
  }

  /* attribute string prettyName; */
  NS_IMETHOD GetPrettyName(char * *aPrettyName) = 0;
  NS_IMETHOD SetPrettyName(char * aPrettyName) = 0;

  /* attribute string hostName; */
  NS_IMETHOD GetHostName(char * *aHostName) = 0;
  NS_IMETHOD SetHostName(char * aHostName) = 0;

  /* attribute string userName; */
  NS_IMETHOD GetUserName(char * *aUserName) = 0;
  NS_IMETHOD SetUserName(char * aUserName) = 0;

  /* attribute string password; */
  NS_IMETHOD GetPassword(char * *aPassword) = 0;
  NS_IMETHOD SetPassword(char * aPassword) = 0;

  /* attribute boolean doBiff; */
  NS_IMETHOD GetDoBiff(PRBool *aDoBiff) = 0;
  NS_IMETHOD SetDoBiff(PRBool aDoBiff) = 0;

  /* attribute long biffMinutes; */
  NS_IMETHOD GetBiffMinutes(PRInt32 *aBiffMinutes) = 0;
  NS_IMETHOD SetBiffMinutes(PRInt32 aBiffMinutes) = 0;

  /* void LoadPreferences (in nsIPref prefs, in string identityKey); */
  NS_IMETHOD LoadPreferences(nsIPref *prefs, const char *identityKey) = 0;

#ifdef XPIDL_JS_STUBS
  static NS_EXPORT_(JSObject *) InitJSClass(JSContext *cx);
  static NS_EXPORT_(JSObject *) GetJSObject(JSContext *cx, nsIMsgIncomingServer *priv);
#endif
};

#endif /* __gen_nsIMsgIncomingServer_h__ */
