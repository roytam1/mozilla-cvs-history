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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*
 * nsINodeInfo is an interface to node info, such as name, prefix, namespace
 * ID and possibly other data that is shared shared between nodes (elements
 * and attributes) that have the same name, prefix and namespace ID within
 * the same document.
 *
 * nsINodeInfoManager is an interface to an object that manages a list of
 * nsINodeInfo's, every document object should hold a strong reference to
 * a nsINodeInfoManager and every nsINodeInfo also holds a string reference
 * to their owning manager. When a nsINodeInfo is no longer used it will
 * automatically remove itself from its owner manager, and when all
 * nsINodeInfo's have been removed from a nsINodeInfoManager and all external
 * references are released the nsINodeInfoManager deletes itself.
 *
 * -- jst@netscape.com
 */

#ifndef nsINodeInfo_h___
#define nsINodeInfo_h___

#include "nsISupports.h"

// Forward declarations
class nsIAtom;
class nsINodeInfoManager;
class nsINameSpaceManager;
class nsString;


// IID for the nsINodeInfo interface
#define NS_INODEINFO_IID       \
{ 0x93dbfd8c, 0x2fb3, 0x4ef5, \
  {0xa2, 0xa0, 0xcf, 0xf2, 0x69, 0x6f, 0x07, 0x88} }

// IID for the nsINodeInfoManager interface
#define NS_INODEINFOMANAGER_IID \
{ 0xb622469b, 0x4dcf, 0x45c4, \
  {0xb0, 0xb9, 0xa7, 0x32, 0xbc, 0xee, 0xa5, 0xcc} }

#define NS_NODEINFOMANAGER_PROGID "component://netscape/layout/nodeinfomanager"


class nsINodeInfo : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_INODEINFO_IID)

  /*
   * Get the name from this node as a string, this does not include the prefix.
   *
   * For the HTML element "<body>" this will return "body" and for the XML
   * element "<html:body>" this will return "body".
   */
  NS_IMETHOD GetName(nsString& aName) = 0;

  /*
   * Get the name from this node as an atom, this does not include the prefix.
   * This function never returns a null atom.
   *
   * For the HTML element "<body>" this will return the "body" atom and for
   * the XML element "<html:body>" this will return the "body" atom.
   */
  NS_IMETHOD GetNameAtom(nsIAtom*& aAtom) = 0;

  /*
   * Get the qualified name from this node as a string, the qualified name
   * includes the prefix, if one exists.
   *
   * For the HTML element "<body>" this will return "body" and for the XML
   * element "<html:body>" this will return "html:body".
   */
  NS_IMETHOD GetQualifiedName(nsString& aQualifiedName) = 0;

  /*
   * Get the local name from this node as a string, GetLocalName() gets the
   * same string as GetName() but only if the node has a prefix and/or a
   * namespace URI. If the node has neither a prefix nor a namespace URI the
   * local name is a null string.
   *
   * For the HTML element "<body>" in a HTML document this will return a null
   * string and for the XML element "<html:body>" this will return "body".
   */
  NS_IMETHOD GetLocalName(nsString& aLocalName) = 0;

  /*
   * Get the prefix from this node as a string.
   *
   * For the HTML element "<body>" this will return a null string and for
   * the XML element "<html:body>" this will return the string "html".
   */
  NS_IMETHOD GetPrefix(nsString& aPrefix) = 0;  

  /*
   * Get the prefix from this node as an atom.
   *
   * For the HTML element "<body>" this will return a null atom and for
   * the XML element "<html:body>" this will return the "html" atom.
   */
  NS_IMETHOD GetPrefixAtom(nsIAtom*& aAtom) = 0;  

  /*
   * Get the namespace URI for a node, if the node has a namespace URI.
   *
   * For the HTML element "<body>" in a HTML document this will return a null
   * string and for the XML element "<html:body>" (assuming that this element,
   * or one of it's ancestors has an
   * xmlns:html='http://www.w3.org/1999/xhtml' attribute) this will return
   * the string "http://www.w3.org/1999/xhtml".
   */
  NS_IMETHOD GetNamespaceURI(nsString& aNameSpaceURI) = 0;

  /*
   * Get the namespace ID for a node if the node has a namespace, if not this
   * returns kNameSpaceID_None.
   *
   * For the HTML element "<body>" in a HTML document this will return
   * kNameSpaceID_None and for the XML element "<html:body>" (assuming that
   * this element, or one of it's ancestors has an
   * xmlns:html='http://www.w3.org/1999/xhtml' attribute) this will return
   * the namespace ID for "http://www.w3.org/1999/xhtml".
   */
  NS_IMETHOD GetNamespaceID(PRInt32& aResult) = 0;

  /*
   * Get the owning node info manager, this will never return null.
   */
  NS_IMETHOD GetNodeInfoManager(nsINodeInfoManager*& aNodeInfoManager) = 0;

  /*
   * Utility functions that can be used to check if a nodeinfo holds a spcific
   * name, name and prefix, name and prefix and namespace ID, or just
   * namespace ID.
   */
  NS_IMETHOD_(PRBool) Equals(nsIAtom *aNameAtom) = 0;
  NS_IMETHOD_(PRBool) Equals(const nsString& aName) = 0;
  NS_IMETHOD_(PRBool) Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom) = 0;
  NS_IMETHOD_(PRBool) Equals(const nsString& aName,
                             const nsString& aPrefix) = 0;
  NS_IMETHOD_(PRBool) Equals(nsIAtom *aNameAtom, PRInt32 aNamespaceID) = 0;
  NS_IMETHOD_(PRBool) Equals(const nsString& aName, PRInt32 aNamespaceID) = 0;
  NS_IMETHOD_(PRBool) Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
                             PRInt32 aNamespaceID) = 0;
  NS_IMETHOD_(PRBool) Equals(const nsString& aName, const nsString& aPrefix,
                             PRInt32 aNamespaceID) = 0;
  NS_IMETHOD_(PRBool) NamespaceEquals(PRInt32 aNamespaceID) = 0;
  NS_IMETHOD_(PRBool) NamespaceEquals(const nsString& aNamespaceURI) = 0;

  /*
   * This is a convinience method that creates a new nsINodeInfo that differs
   * only by name from the one this is called on.
   */
  NS_IMETHOD NameChanged(nsIAtom *aName, nsINodeInfo*& aResult) = 0;

  /*
   * This is a convinience method that creates a new nsINodeInfo that differs
   * only by prefix from the one this is called on.
   */
  NS_IMETHOD PrefixChanged(nsIAtom *aPrefix, nsINodeInfo*& aResult) = 0;
};


class nsINodeInfoManager : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_INODEINFOMANAGER_IID)

  /*
   * Initialize the nodeinfo manager with a namespace manager, this should
   * allways be done.
   */
  NS_IMETHOD Init(nsINameSpaceManager *aNameSpaceManager) = 0;

  /*
   * Methods for creating nodeinfo's from atoms and/or strings.
   */
  NS_IMETHOD GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                         PRInt32 aNamespaceID, nsINodeInfo*& aNodeInfo) = 0;
  NS_IMETHOD GetNodeInfo(const nsString& aName, nsIAtom *aPrefix,
                         PRInt32 aNamespaceID, nsINodeInfo*& aNodeInfo) = 0;
  NS_IMETHOD GetNodeInfo(const nsString& aName, const nsString& aPrefix,
                         PRInt32 aNamespaceID, nsINodeInfo*& aNodeInfo) = 0;
  NS_IMETHOD GetNodeInfo(const nsString& aName, const nsString& aPrefix,
                         const nsString& aNamespaceURI,
                         nsINodeInfo*& aNodeInfo) = 0;
  NS_IMETHOD GetNodeInfo(const nsString& aQualifiedName,
                         const nsString& aNamespaceURI,
                         nsINodeInfo*& aNodeInfo) = 0;

  /*
   * Getter for the namespace manager used by this nodeinfo manager.
   */
  NS_IMETHOD GetNamespaceManager(nsINameSpaceManager*& aNameSpaceManager) = 0;
};

extern nsresult NS_NewNodeInfoManager(nsINodeInfoManager** aResult);

#endif /* nsINodeInfo_h___ */
