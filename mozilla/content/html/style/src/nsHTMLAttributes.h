/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsIHTMLAttributes_h___
#define nsIHTMLAttributes_h___

#include "nsISupports.h"
#include "nsHTMLValue.h"
#include "nsIHTMLContent.h"
#include "nsINodeInfo.h"
class nsIAtom;
class nsISupportsArray;
class nsIHTMLStyleSheet;
class nsRuleWalker;
struct HTMLAttribute;
class nsHTMLMappedAttributes;

// IID for the nsIHTMLMappedAttributes interface {0fdd27a0-2e7b-11d3-8060-006008159b5a}
#define NS_IHTML_MAPPED_ATTRIBUTES_IID  \
{0x0fdd27a0, 0x2e7b, 0x11d3,            \
    {0x80, 0x60, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsIHTMLMappedAttributes : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IHTML_MAPPED_ATTRIBUTES_IID);

  NS_IMETHOD GetAttribute(nsIAtom* aAttrName, nsHTMLValue& aValue) const = 0;
  NS_IMETHOD GetAttribute(nsIAtom* aAttrName, const nsHTMLValue** aValue) const = 0;
  NS_IMETHOD GetAttributeCount(PRInt32& aCount) const = 0;

  NS_IMETHOD Equals(const nsIHTMLMappedAttributes* aAttributes, PRBool& aResult) const = 0;
  NS_IMETHOD HashValue(PRUint32& aValue) const = 0;

  // Sheet accessors for unique table management
  NS_IMETHOD SetUniqued(PRBool aUniqued) = 0;
  NS_IMETHOD GetUniqued(PRBool& aUniqued) = 0;
  NS_IMETHOD DropStyleSheetReference(void) = 0;

#ifdef DEBUG
  virtual void SizeOf(nsISizeOfHandler* aSizer, PRUint32 &aResult) = 0;
#endif
};


const PRInt32 kHTMLAttrNameBufferSize = 4;

struct nsHTMLClassList {
  nsHTMLClassList(nsIAtom* aAtom)
    : mAtom(aAtom), // take ref
      mNext(nsnull)
  {
    MOZ_COUNT_CTOR(nsHTMLClassList);
  }

  nsHTMLClassList(const nsHTMLClassList& aCopy)
    : mAtom(aCopy.mAtom),
      mNext(nsnull)
  {
    MOZ_COUNT_CTOR(nsHTMLClassList);
    NS_IF_ADDREF(mAtom);
    if (aCopy.mNext) {
      mNext = new nsHTMLClassList(*(aCopy.mNext));
    }
  }

  ~nsHTMLClassList(void)
  {
    MOZ_COUNT_DTOR(nsHTMLClassList);
    Reset();
  }

  void Reset(void)
  {
    NS_IF_RELEASE(mAtom);
    if (mNext) {
      delete mNext;
      mNext = nsnull;
    }
  }

  nsIAtom*          mAtom;
  nsHTMLClassList*  mNext;
};

/*
 * union that represents an atom or a nsINodeInfo. It is used to save cycles
 * for the common case when an attribute is not in a namespace, and therefore
 * can't have a prefix. In that case we only store the attribute name and
 * don't spend cycles looking up the right nsINodeInfo in the nodeinfo-hash.
 * The union handles NO refcounting automatically, the client has to call
 * Addref and Release manually.
 */
#define NS_ATTRNAME_NODEINFO_BIT 0x01
union nsHTMLAttrName {
    nsHTMLAttrName()
    {
    }

    nsHTMLAttrName(nsIAtom* aAtom) : mAtom(aAtom)
    {
        NS_ASSERTION(aAtom, "null atom-name in nsHTMLAttrName");
    }

    nsHTMLAttrName(nsINodeInfo* aNodeInfo) : mNodeInfo(aNodeInfo)
    {
        NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsHTMLAttrName");
        mBits |= NS_ATTRNAME_NODEINFO_BIT;
    }

    PRBool IsAtom() const
    {
        return !(mBits & NS_ATTRNAME_NODEINFO_BIT);
    }

    nsINodeInfo* GetNodeInfo() const
    {
        NS_ASSERTION(!IsAtom(), "getting NodeInfo-value of atom-name");
        return (nsINodeInfo*)(mBits & ~NS_ATTRNAME_NODEINFO_BIT);
    }
    
    void SetNodeInfo(nsINodeInfo* aNodeInfo)
    {
        NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsHTMLAttrName");
        mNodeInfo = aNodeInfo;
        mBits |= NS_ATTRNAME_NODEINFO_BIT;
    }

    void Addref()
    {
        // Since both nsINodeInfo and nsIAtom inherits nsISupports as it's first
        // interface we can safly assume that it's first in the vtable
        nsISupports* name = NS_REINTERPRET_CAST(nsISupports *,
            mBits & ~NS_ATTRNAME_NODEINFO_BIT);

        NS_IF_ADDREF(name);
    }
    
    void Release()
    {
        // Since both nsINodeInfo and nsIAtom inherits nsISupports as it's first
        // interface we can safly assume that it's first in the vtable
        nsISupports* name = NS_REINTERPRET_CAST(nsISupports *,
            mBits & ~NS_ATTRNAME_NODEINFO_BIT);

        NS_IF_RELEASE(name);
        mBits = 0;
    }
    nsIAtom* mAtom;  // Used if !(mBits & NS_ATTRNAME_NODEINFO_BIT)
    nsINodeInfo* mNodeInfo; // Used if (mBits & NS_ATTRNAME_NODEINFO_BIT)
    PRWord mBits;
};

class nsHTMLAttributes {
public:
  nsHTMLAttributes(void);
  nsHTMLAttributes(const nsHTMLAttributes& aCopy);
  ~nsHTMLAttributes(void);

  NS_METHOD SetAttributeFor(nsIAtom* aAttrName, const nsHTMLValue& aValue,
                            PRBool aMappedToStyle, 
                            nsIHTMLContent* aContent,
                            nsIHTMLStyleSheet* aSheet,
                            PRInt32& aAttrCount);
  NS_METHOD SetAttributeFor(nsIAtom* aAttrName, const nsAString& aValue,
                            PRBool aMappedToStyle,
                            nsIHTMLContent* aContent,
                            nsIHTMLStyleSheet* aSheet);
  NS_METHOD UnsetAttributeFor(nsIAtom* aAttrName, 
                              PRInt32 aNamespaceID,
                              nsIHTMLContent* aContent,
                              nsIHTMLStyleSheet* aSheet,
                              PRInt32& aAttrCount);

  NS_METHOD GetAttribute(nsIAtom* aAttrName,
                         nsHTMLValue& aValue) const;
  NS_METHOD GetAttribute(nsIAtom* aAttribute,
                         const nsHTMLValue** aValue) const;

  NS_METHOD_(PRBool) HasAttribute(nsIAtom* aAttrName,
                                  PRInt32 aNamespaceID) const;

  // Namespace-able methods. The namespace must NOT be null
  NS_METHOD SetAttributeFor(nsINodeInfo* aAttrName,
                            const nsAString& aValue);
  NS_METHOD GetAttribute(nsIAtom* aAttrName, PRInt32 aNamespaceID,
                         nsIAtom*& aPrefix,
                         const nsHTMLValue** aValue) const;


  NS_METHOD GetAttributeNameAt(PRInt32 aIndex,
                               PRInt32& aNamespaceID,
                               nsIAtom*& aName,
                               nsIAtom*& aPrefix) const;

  NS_METHOD GetAttributeCount(PRInt32& aCount) const;

  NS_METHOD GetID(nsIAtom*& aResult) const;
  NS_METHOD GetClasses(nsVoidArray& aArray) const;
  NS_METHOD HasClass(nsIAtom* aClass, PRBool aCaseSensitive) const;

  NS_METHOD Clone(nsHTMLAttributes** aInstancePtrResult) const;

  NS_METHOD SetStyleSheet(nsIHTMLStyleSheet* aSheet);

  NS_METHOD WalkMappedAttributeStyleRules(nsRuleWalker* aRuleWalker) const;

#ifdef UNIQUE_ATTR_SUPPORT
  NS_METHOD AddContentRef(void);
  NS_METHOD ReleaseContentRef(void);
  NS_METHOD GetContentRefCount(PRInt32& aCount) const;
#endif
  NS_METHOD Reset(void);

#ifdef DEBUG
  NS_METHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  void SizeOf(nsISizeOfHandler* aSizer, PRUint32 &aResult);
#endif

protected:
  nsresult SetAttributeName(nsHTMLAttrName aAttrName, PRBool& aFound);
  nsresult UnsetAttributeName(nsIAtom* aAttrName, PRBool& aFound);
  nsresult UnsetAttributeName(nsIAtom* aAttrName, PRInt32 aNamespaceID,
                              PRBool& aFound);
  nsresult EnsureSingleMappedFor(nsIHTMLContent* aContent, 
                                 nsIHTMLStyleSheet* aSheet,
                                 PRBool aCreate);
  nsresult UniqueMapped(nsIHTMLStyleSheet* aSheet);

private:
  nsHTMLAttributes& operator=(const nsHTMLAttributes& aCopy);
  PRBool operator==(const nsHTMLAttributes& aCopy) const;

protected:

  nsHTMLAttrName*         mAttrNames;
  PRInt32                 mAttrCount;
  PRInt32                 mAttrSize;
  HTMLAttribute*          mFirstUnmapped;
  nsHTMLMappedAttributes* mMapped;
  nsIAtom*                mID;
  nsHTMLClassList         mFirstClass;

  nsHTMLAttrName          mNameBuffer[kHTMLAttrNameBufferSize];
};

nsresult
NS_NewHTMLAttributes(nsHTMLAttributes** aInstancePtrResult);

#endif /* nsIHTMLAttributes_h___ */
