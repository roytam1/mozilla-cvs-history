/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIMsgAccount.idl
 */

#ifndef __gen_nsIMsgAccount_h__
#define __gen_nsIMsgAccount_h__

#include "nsISupports.h" /* interface nsISupports */
#include "nsIMsgIncomingServer.h" /* interface nsIMsgIncomingServer */
#include "nsIMsgSignature.h" /* interface nsIMsgSignature */
#include "nsIMsgIdentity.h" /* interface nsIMsgIdentity */
#include "nsIMsgVCard.h" /* interface nsIMsgVCard */
#include "nsrootidl.h" /* interface nsrootidl */
class nsISupportsArray; /* forward decl */
#include "nsISupportsArray.h"


/* starting interface:    nsIMsgAccount */

/* {da368bd0-e624-11d2-b7fc-00805f05ffa5} */
#define NS_IMSGACCOUNT_IID_STR "da368bd0-e624-11d2-b7fc-00805f05ffa5"
#define NS_IMSGACCOUNT_IID \
  {0xda368bd0, 0xe624, 0x11d2, \
    { 0xb7, 0xfc, 0x00, 0x80, 0x5f, 0x05, 0xff, 0xa5 }}

class nsIMsgAccount : public nsISupports {
 public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IMSGACCOUNT_IID)

  /* attribute string key; */
  NS_IMETHOD GetKey(char * *aKey) = 0;
  NS_IMETHOD SetKey(char * aKey) = 0;

  /* attribute nsIMsgIncomingServer incomingServer; */
  NS_IMETHOD GetIncomingServer(nsIMsgIncomingServer * *aIncomingServer) = 0;
  NS_IMETHOD SetIncomingServer(nsIMsgIncomingServer * aIncomingServer) = 0;

  /* nsISupportsArray GetIdentities (); */
  NS_IMETHOD GetIdentities(nsISupportsArray **_retval) = 0;

  /* attribute nsIMsgIdentity defaultIdentity; */
  NS_IMETHOD GetDefaultIdentity(nsIMsgIdentity * *aDefaultIdentity) = 0;
  NS_IMETHOD SetDefaultIdentity(nsIMsgIdentity * aDefaultIdentity) = 0;

  /* void addIdentity (in nsIMsgIdentity identity); */
  NS_IMETHOD addIdentity(nsIMsgIdentity *identity) = 0;

  /* void removeIdentity (in nsIMsgIdentity identity); */
  NS_IMETHOD removeIdentity(nsIMsgIdentity *identity) = 0;
};

#endif /* __gen_nsIMsgAccount_h__ */
