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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nsDOMError.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsDOMPropEnums.h"
#include "nsString.h"
#include "nsIDOMSelection.h"
#include "nsIDOMNode.h"
#include "nsIDOMSelectionListener.h"
#include "nsIEnumerator.h"
#include "nsIDOMRange.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kISelectionIID, NS_IDOMSELECTION_IID);
static NS_DEFINE_IID(kINodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kISelectionListenerIID, NS_IDOMSELECTIONLISTENER_IID);
static NS_DEFINE_IID(kIEnumeratorIID, NS_IENUMERATOR_IID);
static NS_DEFINE_IID(kIRangeIID, NS_IDOMRANGE_IID);

//
// Selection property ids
//
enum Selection_slots {
  SELECTION_ANCHORNODE = -1,
  SELECTION_ANCHOROFFSET = -2,
  SELECTION_FOCUSNODE = -3,
  SELECTION_FOCUSOFFSET = -4,
  SELECTION_ISCOLLAPSED = -5,
  SELECTION_RANGECOUNT = -6
};

/***********************************************************************/
//
// Selection Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetSelectionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMSelection *a = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case SELECTION_ANCHORNODE:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ANCHORNODE, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetAnchorNode(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, obj, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case SELECTION_ANCHOROFFSET:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ANCHOROFFSET, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        PRInt32 prop;
        nsresult result = NS_OK;
        result = a->GetAnchorOffset(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case SELECTION_FOCUSNODE:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_FOCUSNODE, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetFocusNode(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, obj, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case SELECTION_FOCUSOFFSET:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_FOCUSOFFSET, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        PRInt32 prop;
        nsresult result = NS_OK;
        result = a->GetFocusOffset(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case SELECTION_ISCOLLAPSED:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ISCOLLAPSED, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        PRBool prop;
        nsresult result = NS_OK;
        result = a->GetIsCollapsed(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case SELECTION_RANGECOUNT:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_RANGECOUNT, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        PRInt32 prop;
        nsresult result = NS_OK;
        result = a->GetRangeCount(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, obj, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, obj, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// Selection Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetSelectionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMSelection *a = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case 0:
      default:
        return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, obj, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, obj, id, vp);
  }

  return PR_TRUE;
}


//
// Selection finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeSelection(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// Selection enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateSelection(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// Selection resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveSelection(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method GetRangeAt
//
PR_STATIC_CALLBACK(JSBool)
SelectionGetRangeAt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMRange* nativeRet;
  PRInt32 b0;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_GETRANGEAT, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (!JS_ValueToInt32(cx, argv[0], (int32 *)&b0)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_NUMBER_ERR);
    }

    result = nativeThis->GetRangeAt(b0, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, obj, rval);
  }

  return JS_TRUE;
}


//
// Native method ClearSelection
//
PR_STATIC_CALLBACK(JSBool)
SelectionClearSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_CLEARSELECTION, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->ClearSelection();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method Collapse
//
PR_STATIC_CALLBACK(JSBool)
SelectionCollapse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode> b0;
  PRInt32 b1;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_COLLAPSE, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }
    if (!JS_ValueToInt32(cx, argv[1], (int32 *)&b1)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_NUMBER_ERR);
    }

    result = nativeThis->Collapse(b0, b1);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method Extend
//
PR_STATIC_CALLBACK(JSBool)
SelectionExtend(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode> b0;
  PRInt32 b1;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_EXTEND, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }
    if (!JS_ValueToInt32(cx, argv[1], (int32 *)&b1)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_NUMBER_ERR);
    }

    result = nativeThis->Extend(b0, b1);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method CollapseToStart
//
PR_STATIC_CALLBACK(JSBool)
SelectionCollapseToStart(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_COLLAPSETOSTART, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->CollapseToStart();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method CollapseToEnd
//
PR_STATIC_CALLBACK(JSBool)
SelectionCollapseToEnd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_COLLAPSETOEND, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->CollapseToEnd();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method ContainsNode
//
PR_STATIC_CALLBACK(JSBool)
SelectionContainsNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  PRBool nativeRet;
  nsCOMPtr<nsIDOMNode> b0;
  PRBool b1;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_CONTAINSNODE, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }
    if (!nsJSUtils::nsConvertJSValToBool(&b1, cx, argv[1])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_BOOLEAN_ERR);
    }

    result = nativeThis->ContainsNode(b0, b1, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }

  return JS_TRUE;
}


//
// Native method DeleteFromDocument
//
PR_STATIC_CALLBACK(JSBool)
SelectionDeleteFromDocument(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_DELETEFROMDOCUMENT, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->DeleteFromDocument();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method AddRange
//
PR_STATIC_CALLBACK(JSBool)
SelectionAddRange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMRange> b0;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ADDRANGE, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kIRangeIID,
                                           "Range",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->AddRange(b0);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method StartBatchChanges
//
PR_STATIC_CALLBACK(JSBool)
SelectionStartBatchChanges(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_STARTBATCHCHANGES, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->StartBatchChanges();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method EndBatchChanges
//
PR_STATIC_CALLBACK(JSBool)
SelectionEndBatchChanges(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ENDBATCHCHANGES, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->EndBatchChanges();
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method AddSelectionListener
//
PR_STATIC_CALLBACK(JSBool)
SelectionAddSelectionListener(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMSelectionListener> b0;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_ADDSELECTIONLISTENER, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kISelectionListenerIID,
                                           "SelectionListener",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->AddSelectionListener(b0);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method RemoveSelectionListener
//
PR_STATIC_CALLBACK(JSBool)
SelectionRemoveSelectionListener(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMSelectionListener> b0;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_REMOVESELECTIONLISTENER, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }

    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)(void**)getter_AddRefs(b0),
                                           kISelectionListenerIID,
                                           "SelectionListener",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->RemoveSelectionListener(b0);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method ToString
//
PR_STATIC_CALLBACK(JSBool)
SelectionToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMSelection *nativeThis = (nsIDOMSelection*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsAutoString nativeRet;
  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

  *rval = JSVAL_NULL;

  {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_SELECTION_TOSTRING, PR_FALSE);
    }
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, rv);
    }
  }


    result = nativeThis->ToString(nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, obj, result);
    }

    nsJSUtils::nsConvertStringToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for Selection
//
JSClass SelectionClass = {
  "Selection", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetSelectionProperty,
  SetSelectionProperty,
  EnumerateSelection,
  ResolveSelection,
  JS_ConvertStub,
  FinalizeSelection,
  nsnull,
  nsJSUtils::nsCheckAccess
};


//
// Selection class properties
//
static JSPropertySpec SelectionProperties[] =
{
  {"anchorNode",    SELECTION_ANCHORNODE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"anchorOffset",    SELECTION_ANCHOROFFSET,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"focusNode",    SELECTION_FOCUSNODE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"focusOffset",    SELECTION_FOCUSOFFSET,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"isCollapsed",    SELECTION_ISCOLLAPSED,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"rangeCount",    SELECTION_RANGECOUNT,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// Selection class methods
//
static JSFunctionSpec SelectionMethods[] = 
{
  {"getRangeAt",          SelectionGetRangeAt,     1},
  {"clearSelection",          SelectionClearSelection,     0},
  {"collapse",          SelectionCollapse,     2},
  {"extend",          SelectionExtend,     2},
  {"collapseToStart",          SelectionCollapseToStart,     0},
  {"collapseToEnd",          SelectionCollapseToEnd,     0},
  {"containsNode",          SelectionContainsNode,     2},
  {"deleteFromDocument",          SelectionDeleteFromDocument,     0},
  {"addRange",          SelectionAddRange,     1},
  {"startBatchChanges",          SelectionStartBatchChanges,     0},
  {"endBatchChanges",          SelectionEndBatchChanges,     0},
  {"addSelectionListener",          SelectionAddSelectionListener,     1},
  {"removeSelectionListener",          SelectionRemoveSelectionListener,     1},
  {"toString",          SelectionToString,     0},
  {0}
};


//
// Selection constructor
//
PR_STATIC_CALLBACK(JSBool)
Selection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// Selection class initialization
//
extern "C" NS_DOM nsresult NS_InitSelectionClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "Selection", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &SelectionClass,      // JSClass
                         Selection,            // JSNative ctor
                         0,             // ctor args
                         SelectionProperties,  // proto props
                         SelectionMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

  }
  else if ((nsnull != constructor) && JSVAL_IS_OBJECT(vp)) {
    proto = JSVAL_TO_OBJECT(vp);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (aPrototype) {
    *aPrototype = proto;
  }
  return NS_OK;
}


//
// Method for creating a new Selection JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptSelection(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptSelection");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMSelection *aSelection;

  if (nsnull == aParent) {
    parent = nsnull;
  }
  else if (NS_OK == aParent->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
    if (NS_OK != owner->GetScriptObject(aContext, (void **)&parent)) {
      NS_RELEASE(owner);
      return NS_ERROR_FAILURE;
    }
    NS_RELEASE(owner);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (NS_OK != NS_InitSelectionClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kISelectionIID, (void **)&aSelection);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &SelectionClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aSelection);
  }
  else {
    NS_RELEASE(aSelection);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
