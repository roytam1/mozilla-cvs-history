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
 * Original Author: Aaron Leventhal
 *
 * Contributor(s): 
 */

/* For documentation of the accessibility architecture, 
 * see http://lxr.mozilla.org/seamonkey/source/accessible/accessible-docs.html
 */

#ifndef _SimpleDOMNode_H_
#define _SimpleDOMNode_H_

#include "nsCOMPtr.h"
#include "nsIAccessible.h"
#include "Accessible.h"
#include "nsIAccessibleEventListener.h"
#include "ISimpleDOMNode.h"
#include "ISimpleDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"

#include "nsString.h"

class SimpleDOMNode : public ISimpleDOMNode
{
  public: // construction, destruction
    SimpleDOMNode(nsIAccessible *, HWND);
    SimpleDOMNode(nsIDOMNode *, HWND);
    virtual ~SimpleDOMNode();

  public: // IUnknown methods - see iunknown.h for documentation
    STDMETHODIMP_(ULONG) AddRef        ();
    STDMETHODIMP      QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) Release       ();

    nsIDOMNode* GetRealDOMNode();

  private:
    void GetAccessibleFor(nsIDOMNode *node, nsIAccessible **newAcc);
    ISimpleDOMNode* SimpleDOMNode::MakeSimpleDOMNode(nsIDOMNode *node);

  public:

    virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_nodeInfo( 
        /* [out] */ BSTR __RPC_FAR *tagName,
        /* [out] */ short __RPC_FAR *nameSpaceID,
        /* [out] */ unsigned short __RPC_FAR *nodeType,
        /* [out] */ BSTR __RPC_FAR *nodeValue,
        /* [out] */ unsigned int __RPC_FAR *numChildren);
  
    virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [in] */ unsigned short maxAttribs,
        /* [out] */ unsigned short __RPC_FAR *numAttribs,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribNames,
        /* [length_is][size_is][out] */ short __RPC_FAR *nameSpaceID,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribValues);
  
    virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_computedStyle( 
        /* [in] */ unsigned short maxStyleProperties,
        /* [out] */ unsigned short __RPC_FAR *numStyleProperties,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleProperties,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleValues);
    
    virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_computedStyleForMedia( 
        /* [in] */ BSTR __RPC_FAR *mediaType,  // W3C media type such as aural, braille, embossed, handheld, print, projection, screen, tty
        /* [in] */ unsigned short maxStyleProperties,
        /* [out] */ unsigned short __RPC_FAR *numStyleProperties,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleProperties,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleValues);
  
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_parentNode(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_firstChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_lastChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);

  protected:
    nsCOMPtr<nsIDOMNode> mDOMNode;
    ULONG        m_cRef;              // the reference count
    HWND mWnd;

    void GetElementAndContentFor(nsCOMPtr<nsIDOMElement>& aElement, nsCOMPtr<nsIContent>& aContent);
};

#endif

