/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the web scripts access security code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Harish Dhurvasula <harishd@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsWebScriptsAccess.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsComponentManagerUtils.h"
#include "nsIServiceManagerUtils.h"
#include "nsICodebasePrincipal.h"
#include "nsIURL.h"
#include "nsReadableUtils.h"
#include "nsIHttpChannel.h"
#include "nsNetUtil.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#define WSA_GRANT_ACCESS_TO_ALL   (1 << 0)
#define WSA_FILE_NOT_FOUND        (1 << 1)
#define WSA_FILE_DELEGATED        (1 << 2)

#define SECURITY_PROPERTIES "chrome://communicator/locale/webservices/security.properties"

NS_NAMED_LITERAL_STRING(kNamespace2002, "http://www.mozilla.org/2002/soap/security");

// Element set
NS_NAMED_LITERAL_STRING(kWebScriptAccessTag, "webScriptAccess");
NS_NAMED_LITERAL_STRING(kDelegateTag, "delegate");
NS_NAMED_LITERAL_STRING(kAllowTag, "allow");

// Attribute set
NS_NAMED_LITERAL_STRING(kTypeAttr, "type");
NS_NAMED_LITERAL_STRING(kFromAttr, "from");

// Default attribute value
NS_NAMED_LITERAL_STRING(kAny, "any");

/**
 *  This method compares two strings where the lhs string value may contain 
 *  asterisk. Therefore, an lhs with a value of "he*o" should be equal to a rhs
 *  value of "hello" or "hero" etc.. These strings are compared as follows:
 *  1) Characters before the first asterisk are compared from left to right.
 *     Thus if the lhs string did not contain an asterisk then we just do
 *     a simple string comparison.
 *  2) Match a pattern, found between asterisk. That is, if lhs and rhs were 
 *     "h*ll*" and "hello" respectively, then compare the pattern "ll".
 *  3) Characters after the last asterisk are compared from right to left.
 *     Thus, "*lo" == "hello" and != "blow"
 */
static PRBool 
IsEqual(const nsAString& aLhs, const nsAString& aRhs) 
{
  nsAString::const_iterator lhs_begin, lhs_end;
  nsAString::const_iterator rhs_begin, rhs_end;
 
  aLhs.BeginReading(lhs_begin);
  aLhs.EndReading(lhs_end);
  aRhs.BeginReading(rhs_begin);
  aRhs.EndReading(rhs_end);

  PRBool pattern_before_asterisk = PR_TRUE; 
  nsAString::const_iterator curr_posn = lhs_begin;
  while (curr_posn != lhs_end) {
    if (*lhs_begin == '*') {
      pattern_before_asterisk = PR_FALSE;
      ++lhs_begin; // Do this to not include '*' when pattern matching.
    }
    else if (pattern_before_asterisk) {
      // Match character by character to see if lhs and rhs are identical
      if (*curr_posn != *rhs_begin) {
        return PR_FALSE;
      }
      ++lhs_begin;
      ++curr_posn;
      ++rhs_begin;
      if (rhs_begin == rhs_end &&
          curr_posn == lhs_end) {
        return PR_TRUE; // lhs and rhs matched perfectly
      }
    }
    else if (++curr_posn == lhs_end) {
      if (curr_posn != lhs_begin) {
        // Here we're matching the last few characters to make sure
        // that lhs is actually equal to rhs. Ex. "a*c" != "abcd"
        // and "*xabcd" != "abcd".
        PRBool done = PR_FALSE;
        for (;;) {
          if (--curr_posn == lhs_begin)
            done = PR_TRUE;
          if (rhs_end == rhs_begin)
            return PR_FALSE;
          if (*(--rhs_end) != *curr_posn)
            return PR_FALSE;
          if (done)
            return PR_TRUE;
        }
      }
      // No discrepency between lhs and rhs
      return PR_TRUE;
    }
    else if (*curr_posn == '*') {
      // Matching pattern between asterisks. That is, in "h*ll*" we
      // check to see if "ll" exists in the rhs string.
      const nsAString& pattern = Substring(lhs_begin, curr_posn);

      nsAString::const_iterator tmp_end = rhs_end;
      if (!FindInReadable(pattern, rhs_begin, rhs_end)) {
         return PR_FALSE;
      }
      rhs_begin = rhs_end;
      rhs_end   = tmp_end;
      lhs_begin = curr_posn;
    }
  }

  return PR_FALSE;
}

#ifdef DEBUG

struct TestStruct {
  const char* lhs; // string that contains the wild char(s).
  const char* rhs; // string that is compared against lhs.
  PRBool equal;    // set to true if lhs and rhs are expected 
                   // to be equal else set false;
};

static 
const TestStruct kStrings[] = {
  { "f*o*bar", "foobar", PR_TRUE },
  { "foo*bar", "foofbar", PR_TRUE },
  { "*foo*bar", "ffoofoobbarbarbar", PR_TRUE },
  { "*foo*bar*barbar", "ffoofoobbarbarbar", PR_TRUE },
  { "http://*.*.*/*", "http://www.mozilla.org/", PR_TRUE},
  { "http://*/*", "http://www.mozilla.org/", PR_TRUE},
  { "http://*.mozilla.org/*/*", "http://www.mozilla.org/Projects/", PR_TRUE},
  { "http://www.m*zi*la.org/*", "http://www.mozilla.org/", PR_TRUE },
  { "http://www.mozilla.org/*.html", "http://www.mozilla.org/owners.html", PR_TRUE },
  { "http://www.mozilla.org/*.htm*", "http://www.mozilla.org/owners.html", PR_TRUE },
  { "http://www.mozilla.org/*rs.htm*", "http://www.mozilla.org/ownres.html", PR_FALSE },
  { "http://www.mozilla.org/a*c.html", "http://www.mozilla.org/abcd.html", PR_FALSE },
  { "https://www.mozilla.org/*", "http://www.mozilla.org/abcd.html", PR_FALSE },
};

static 
void VerifyIsEqual()
{
  static PRUint32 size = sizeof(kStrings)/sizeof(kStrings[0]);
  PRUint32 i;
  for (i = 0; i < size; ++i) {
    if (IsEqual(NS_ConvertUTF8toUCS2(kStrings[i].lhs), 
                NS_ConvertUTF8toUCS2(kStrings[i].rhs)) 
                != kStrings[i].equal) {
      const char* equal = 
        kStrings[i].equal ? "equivalent" : 
                            "not equivalent";
      printf("\nTest Failed: %s is %s to %s.\n", 
             kStrings[i].lhs, equal, kStrings[i].rhs);
    }
  }
}

#endif

static PRBool PR_CALLBACK 
FreeEntries(nsHashKey *aKey, void *aData, void* aClosure)
{
  AccessInfoEntry* entry = NS_REINTERPRET_CAST(AccessInfoEntry*, aData);
  delete entry;
  return PR_TRUE;
}

NS_IMPL_ISUPPORTS1(nsWebScriptsAccess, 
                   nsIWebScriptsAccessService)

nsWebScriptsAccess::nsWebScriptsAccess()
{
  NS_INIT_ISUPPORTS();
}

nsWebScriptsAccess::~nsWebScriptsAccess()
{
  mAccessInfoTable.Enumerate(FreeEntries, this);
}

NS_IMETHODIMP 
nsWebScriptsAccess::CanAccess(nsIURI* aTransportURI,
                              const nsAString& aRequestType,
                              PRBool* aAccessGranted)
{
  *aAccessGranted = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aTransportURI);

  nsresult rv;
  if (!mSecurityManager) {
    mSecurityManager = do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
   
  rv =
    mSecurityManager->IsCapabilityEnabled("UniversalBrowserRead", 
                                          aAccessGranted);
  if (NS_FAILED(rv) || *aAccessGranted)
    return rv;
  
  rv = mSecurityManager->CheckSameOrigin(0, aTransportURI);
  if (NS_SUCCEEDED(rv)) {
    // script security manager has granted access
    *aAccessGranted = PR_TRUE;
    return rv;
  }
  else {
    // Script security manager has denied access and has set an
    // exception. Clear the exception and fall back on the new
    // security model's decision.
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID()));
    if (xpc) {
      nsCOMPtr<nsIXPCNativeCallContext> cc;
      xpc->GetCurrentNativeCallContext(getter_AddRefs(cc));
      if (cc) {
        JSContext* cx;
        rv = cc->GetJSContext(&cx);
        NS_ENSURE_SUCCESS(rv, rv);

        JS_ClearPendingException(cx);
        cc->SetExceptionWasThrown(PR_FALSE);
      }
    }
  }

  mServiceURI = aTransportURI;

  nsXPIDLCString path;
  aTransportURI->GetPrePath(path);
  path += '/';

  AccessInfoEntry* entry = 0;
  rv = GetAccessInfoEntry(path, PR_FALSE, &entry);
  NS_ENSURE_SUCCESS(rv, rv);

  return CheckAccess(entry, aRequestType, aAccessGranted);
}

NS_IMETHODIMP 
nsWebScriptsAccess::InvalidateCache(const char* aTransportURI)
{
  if (aTransportURI) {
    nsCStringKey key(aTransportURI);
    if (mAccessInfoTable.Exists(&key)) {
      AccessInfoEntry* entry = 
        NS_REINTERPRET_CAST(AccessInfoEntry*, mAccessInfoTable.Remove(&key));
      delete entry;
    }
  }
  else {
    // If a URI is not specified then we clear the entire cache.
    mAccessInfoTable.Enumerate(FreeEntries, this);
  }
  return NS_OK;
}

nsresult 
nsWebScriptsAccess::GetAccessInfoEntry(const char* aKey,
                                       const PRBool aIsDelegated,
                                       AccessInfoEntry** aEntry)
{
  nsCStringKey key(aKey);

  *aEntry = NS_REINTERPRET_CAST(AccessInfoEntry*, mAccessInfoTable.Get(&key));

  if (!*aEntry) {
    // There's no entry for this server. Load the declaration file (
    // web-scripts-access.xml ) and extract access information from
    // it. Record the extracted info. for this session
    nsCOMPtr<nsIDOMDocument> document;
    nsresult rv = 
      GetDocument(PromiseFlatCString(nsDependentCString(aKey) + 
                  NS_LITERAL_CSTRING("web-scripts-access.xml")).get(),
                  getter_AddRefs(document));
    NS_ENSURE_SUCCESS(rv, rv);
    if (document) {
      // Extract access information from the document.
      rv = GetInfoFromDocument(document, aIsDelegated, aEntry);
      NS_ENSURE_SUCCESS(rv, rv);

      // If the document is invalid then an entry will not be created.
      if (!*aEntry)
        return NS_OK;
    }
    else {
      rv = CreateAccessInfoEntry(WSA_FILE_NOT_FOUND, aEntry);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    mAccessInfoTable.Put(&key, *aEntry);
  }
  NS_ASSERTION(*aEntry, "unexpected: access info entry is null!");
  if (*aEntry  && ((*aEntry)->mFlags & WSA_FILE_DELEGATED))
    return GetDelegatedInfo(aEntry);
  return NS_OK;
}

nsresult 
nsWebScriptsAccess::GetInfoFromDocument(nsIDOMDocument* aDocument,
                                        const PRBool aIsDelegated,
                                        AccessInfoEntry** aEntry)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  PRBool valid;
  nsresult rv = ValidateDocument(aDocument, &valid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!valid) {
    return NS_OK;
  }

  if (!aIsDelegated) {
    nsCOMPtr<nsIDOMNodeList> delegateList; 
    rv = aDocument->GetElementsByTagNameNS(kNamespace2002, kDelegateTag, 
                                          getter_AddRefs(delegateList));
    NS_ENSURE_TRUE(delegateList, rv);
    nsCOMPtr<nsIDOMNode> node;
    delegateList->Item(0, getter_AddRefs(node));
    if (node) {
      rv = CreateAccessInfoEntry(WSA_FILE_DELEGATED, aEntry);
      return rv;
    }
  }

  nsCOMPtr<nsIDOMNodeList> allowList;
  rv = aDocument->GetElementsByTagNameNS(kNamespace2002, kAllowTag, 
                                        getter_AddRefs(allowList));
  NS_ENSURE_TRUE(allowList, rv);

  PRUint32 count;
  allowList->GetLength(&count);
  if (count) {
    rv = CreateAccessInfoEntry(allowList, aEntry);
  }
  else {
    // Since there are no ALLOW elements present grant access to all.
    rv = CreateAccessInfoEntry(WSA_GRANT_ACCESS_TO_ALL, aEntry);
  }

  return NS_OK;
}

nsresult 
nsWebScriptsAccess::GetDocument(const char* aDeclFilePath,
                                nsIDOMDocument** aDocument)
{
  nsresult rv = NS_OK;
  
  if (!mRequest) {
    mRequest = do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
 
  rv = mRequest->OpenRequest("GET", aDeclFilePath, PR_FALSE, nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
    
  rv = mRequest->OverrideMimeType("text/xml");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mRequest->Send(0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  mRequest->GetChannel(getter_AddRefs(channel));
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel, &rv));
  NS_ENSURE_TRUE(httpChannel, rv);

  PRBool succeeded;
  httpChannel->GetRequestSucceeded(&succeeded);
 
  if (succeeded) {
    rv = mRequest->GetResponseXML(aDocument);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
nsWebScriptsAccess::GetCodebaseURI(nsIURI** aCodebase)
{
  nsresult rv = NS_OK;
 
  if (!mSecurityManager) {
    mSecurityManager = 
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = mSecurityManager->GetSubjectPrincipal(getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsICodebasePrincipal> codebase(do_QueryInterface(principal, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = codebase->GetURI(aCodebase);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsWebScriptsAccess::GetDelegatedInfo(AccessInfoEntry** aEntry)
{
  nsresult rv;
  nsCOMPtr<nsIURL> url(do_QueryInterface(mServiceURI, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsXPIDLCString path;
  url->GetPrePath(path);
  nsXPIDLCString directory;
  url->GetDirectory(directory);
  path += directory;

  return GetAccessInfoEntry(path, PR_TRUE, aEntry);
}

nsresult
nsWebScriptsAccess::CreateAccessInfoEntry(const PRInt32 aFlags, 
                                          AccessInfoEntry** aEntry)
{
  *aEntry = new AccessInfoEntry(aFlags);
  NS_ENSURE_TRUE(*aEntry, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
nsWebScriptsAccess::CreateAccessInfoEntry(nsIDOMNodeList* aAllowList,
                                          AccessInfoEntry** aEntry)
{
  NS_ENSURE_ARG_POINTER(aAllowList);
  
  nsAutoPtr<AccessInfoEntry> entry(new AccessInfoEntry());
  NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 count;
  aAllowList->GetLength(&count);

  PRUint32 index; 
  nsCOMPtr<nsIDOMNode> node;
  nsAutoString type, from;
  for (index = 0; index < count; index++) {
    aAllowList->Item(index, getter_AddRefs(node));
    NS_ENSURE_TRUE(node, NS_ERROR_UNEXPECTED);
     
    nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));
    element->GetAttribute(kTypeAttr, type);
    element->GetAttribute(kFromAttr, from);

    PRBool found_type = !type.IsEmpty();
    PRBool found_from = !from.IsEmpty();

    if (!found_type && !found_from) {
      // Minor optimization - If the "type" and "from"
      // attributes aren't present then no need to check
      // for attributes in other "allow" elements because
      // access will be granted to all regardless.
      entry->mFlags |= WSA_GRANT_ACCESS_TO_ALL;
      break;
    }

    nsAutoPtr<AccessInfo> access_info(new AccessInfo());
    NS_ENSURE_TRUE(access_info, NS_ERROR_OUT_OF_MEMORY);
    
    if (found_type) {
      access_info->mType = ToNewUnicode(type);
      NS_ENSURE_TRUE(access_info->mType, NS_ERROR_OUT_OF_MEMORY);
    }

    if (found_from) {
      access_info->mFrom = ToNewUnicode(from);
      NS_ENSURE_TRUE(access_info->mFrom, NS_ERROR_OUT_OF_MEMORY);
    }

    entry->mInfoArray.AppendElement(access_info.forget());
    
    type.Truncate();
    from.Truncate();
  }

  *aEntry = entry.forget();

  return NS_OK;
}

nsresult
nsWebScriptsAccess::CheckAccess(AccessInfoEntry* aEntry,
                                const nsAString& aRequestType, 
                                PRBool* aAccessGranted)
{
#ifdef DEBUG
  static PRBool verified = PR_FALSE;
  if (!verified) {
    verified = PR_TRUE;
    VerifyIsEqual();
  }
#endif

  *aAccessGranted = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aEntry);

  if (aEntry->mFlags & WSA_FILE_NOT_FOUND) {
    return NS_OK;
  }
    
  if (aEntry->mFlags & WSA_GRANT_ACCESS_TO_ALL) {
    *aAccessGranted = PR_TRUE;
    return NS_OK;
  }

  nsCOMPtr<nsIURI> codebase_uri;
  nsresult rv = GetCodebaseURI(getter_AddRefs(codebase_uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString tmp;
  codebase_uri->GetSpec(tmp);
  const nsAString& codebase = NS_ConvertUTF8toUCS2(tmp);

  PRUint32 count = aEntry->mInfoArray.Count();
  PRUint32 index;
  for (index = 0; index < count; index++) {
    AccessInfo* access_info = 
      NS_REINTERPRET_CAST(AccessInfo*, aEntry->mInfoArray.ElementAt(index));
    NS_ASSERTION(access_info, "Entry is missing attribute information");
    
    if (!access_info->mType || kAny.Equals(access_info->mType) || 
        aRequestType.Equals(access_info->mType)) {
      if (!access_info->mFrom) {
        // If "from" is not specified, then all scripts will be  allowed 
        *aAccessGranted = PR_TRUE;
        break;
      }
      else {
        if (IsEqual(nsDependentString(access_info->mFrom), codebase)) {
          *aAccessGranted = PR_TRUE;
          break;
        }
      }
    }
  }
  
  return NS_OK;
}

/** 
  * Validation is based on the following syntax:
  * 
  * <!ELEMENT webScriptAccess (delegate?|allow*)>
  * <!ELEMENT delegate EMPTY>
  * <!ELEMENT allow EMPTY>
  * <!ATTLIST allow type|from CDATA #IMPLIED>.
  *
  */
nsresult
nsWebScriptsAccess::ValidateDocument(nsIDOMDocument* aDocument,
                                     PRBool* aIsValid)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  *aIsValid = PR_FALSE;
  nsCOMPtr<nsIDOMElement> rootElement;
  aDocument->GetDocumentElement(getter_AddRefs(rootElement));
  
  nsAutoString ns;
  nsAutoString name;
  nsresult rv = rootElement->GetNamespaceURI(ns);
  if (NS_FAILED(rv))
    return rv;
  rootElement->GetLocalName(name);
  if (NS_FAILED(rv))
    return rv;
  
  if (!ns.Equals(kNamespace2002)) {
    const PRUnichar *inputs[1]  = { ns.get() };
    return ReportError(NS_LITERAL_STRING("UnsupportedNamespace").get(), 
                       inputs, 1);
  }
  if (!name.Equals(kWebScriptAccessTag)) {
    const PRUnichar *inputs[1]  = { name.get() };
    return ReportError(NS_LITERAL_STRING("UnknownRootElement").get(), 
                       inputs, 1);
  }

  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));
  NS_ENSURE_TRUE(rootNode, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNodeList> children;
  rootNode->GetChildNodes(getter_AddRefs(children));
  NS_ENSURE_TRUE(children, NS_ERROR_UNEXPECTED);

  PRUint32 length;
  children->GetLength(&length);

  PRBool hadDelegate = PR_FALSE;
  nsCOMPtr<nsIDOMNode> child, attr;
  nsCOMPtr<nsIDOMNamedNodeMap> attrs;
  PRUint32 i;
  for (i = 0; i < length; i++) {
    children->Item(i, getter_AddRefs(child));
    NS_ENSURE_TRUE(child, NS_ERROR_UNEXPECTED);
  
    PRUint16 type;
    child->GetNodeType(&type);

    if (nsIDOMNode::ELEMENT_NODE == type) {
      rv = child->GetNamespaceURI(ns);
      if (NS_FAILED(rv))
        return rv;
      rv = child->GetLocalName(name);
      if (NS_FAILED(rv))
        return rv;
 
      if (!ns.Equals(kNamespace2002))
        continue; // ignore elements with different ns.

      PRBool hasChildNodes = PR_FALSE;
      if (name.Equals(kDelegateTag)) {
        // There can me no more than one delegate element.
        if (hadDelegate) {
          const PRUnichar *inputs[1] = { name.get() };
          return ReportError(NS_LITERAL_STRING("TooManyElements").get(), 
                             inputs, 1);
        }
        // Make sure that the delegate element is EMPTY.
        child->HasChildNodes(&hasChildNodes);
        if (hasChildNodes) {
          const PRUnichar *inputs[1] = { name.get() };
          return ReportError(NS_LITERAL_STRING("ElementNotEmpty").get(), 
                             inputs, 1);
        }
        hadDelegate = PR_TRUE;
      }
      else if (name.Equals(kAllowTag)) {
        // Make sure that the allow element is EMPTY.
        child->HasChildNodes(&hasChildNodes);
        if (hasChildNodes) {
          const PRUnichar *inputs[1] = { name.get() };
          return ReportError(NS_LITERAL_STRING("ElementNotEmpty").get(), 
                             inputs, 1);
        }
        rv = child->GetAttributes(getter_AddRefs(attrs));
        if (NS_FAILED(rv))
          return rv;
        
        PRUint32 count, i;
        attrs->GetLength(&count);
        for (i = 0; i < count; i++) {
          attrs->Item(i, getter_AddRefs(attr));
          if (attr) {
            rv = attr->GetLocalName(name);
            if (NS_FAILED(rv))
              return rv;
            if (!name.Equals(kTypeAttr) && !name.Equals(kFromAttr)) {
              const PRUnichar *inputs[1] = { name.get() };
              return ReportError(NS_LITERAL_STRING("UnknownAttribute").get(), 
                                 inputs, 1);
            }
          }
        }
      }
      else {
        const PRUnichar *inputs[1] = { name.get() };
        return ReportError(NS_LITERAL_STRING("UnknownElement").get(), 
                           inputs, 1);
      }

    }
  }
  *aIsValid = PR_TRUE;
  return NS_OK;
}

nsresult
nsWebScriptsAccess::ReportError(const PRUnichar* aMessageID, 
                                const PRUnichar** aInputs, 
                                const PRInt32 aLength)
{
  nsCOMPtr<nsIStringBundleService> bundleService
    = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  NS_ENSURE_TRUE(bundleService, NS_OK); // intentionally returning NS_OK;

  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle(SECURITY_PROPERTIES, getter_AddRefs(bundle));
  NS_ENSURE_TRUE(bundle, NS_OK);

  nsXPIDLString message;
  bundle->FormatStringFromName(aMessageID, aInputs, aLength,
                               getter_Copies(message));

  nsCOMPtr<nsIConsoleService> consoleService = 
    do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  NS_ENSURE_TRUE(consoleService, NS_OK); // intentionally returning NS_OK;
  
  return consoleService->LogStringMessage(message.get());
}

