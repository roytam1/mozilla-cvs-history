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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIDocumentEncoder.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsISelection.h"
#include "nsCOMPtr.h"
#include "nsIContentSerializer.h"
#include "nsIUnicodeEncoder.h"
#include "nsIOutputStream.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMComment.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "nsICharsetConverterManager.h"
#include "nsHTMLAtoms.h"
#include "nsITextContent.h"
#include "nsIEnumerator.h"
#include "nsISelectionPrivate.h"
#include "nsIContentIterator.h"
#include "nsISupportsArray.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);

nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult);

enum nsRangeIterationDirection {
  kDirectionOut = -1,
  kDirectionIn = 1
};

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsDocumentEncoder declaration 
#pragma mark -
#endif

class nsDocumentEncoder : public nsIDocumentEncoder
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOCUMENT_ENCODER_IID; return iid; }

  nsDocumentEncoder();
  virtual ~nsDocumentEncoder();

  NS_IMETHOD Init(nsIDocument* aDocument, const nsAReadableString& aMimeType, PRUint32 aFlags);

  /* Interfaces for addref and release and queryinterface */
  NS_DECL_ISUPPORTS

  // Inherited methods from nsIDocumentEncoder
  NS_IMETHOD SetSelection(nsISelection* aSelection);
  NS_IMETHOD SetRange(nsIDOMRange* aRange);
  NS_IMETHOD SetWrapColumn(PRUint32 aWC);
  NS_IMETHOD SetCharset(const nsAReadableString& aCharset);

  NS_IMETHOD EncodeToStream(nsIOutputStream* aStream);
  NS_IMETHOD EncodeToString(nsAWritableString& aOutputString);
  NS_IMETHOD EncodeToStringWithContext(nsAWritableString& aEncodedString, 
                                       nsAWritableString& aContextString,
                                       nsAWritableString& aInfoString);
                                       
protected:
  nsresult SerializeNodeStart(nsIDOMNode* aNode, PRInt32 aStartOffset,
                              PRInt32 aEndOffset, nsAWritableString& aStr);
  nsresult SerializeToStringRecursive(nsIDOMNode* aNode,
                                      nsAWritableString& aStr);
  nsresult SerializeNodeEnd(nsIDOMNode* aNode, nsAWritableString& aStr);
  nsresult SerializeRangeToString(nsIDOMRange *aRange,
                                  nsAWritableString& aOutputString);
  nsresult SerializeRangeNodes(nsIDOMRange* aRange, 
                               nsIDOMNode* aCommonParent, 
                               nsAWritableString& aString);
  nsresult SerializeRangeContextStart(const nsVoidArray& aAncestorArray,
                                      nsAWritableString& aString);
  nsresult SerializeRangeContextEnd(const nsVoidArray& aAncestorArray,
                                    nsAWritableString& aString);

  nsresult FlushText(nsAWritableString& aString, PRBool aForce);

  static PRBool IncludeInContext_HTML(nsIDOMNode *aNode);

  nsCOMPtr<nsIDocument>          mDocument;
  nsCOMPtr<nsISelection>         mSelection;
  nsCOMPtr<nsIDOMRange>          mRange;
  nsCOMPtr<nsIOutputStream>      mStream;
  nsCOMPtr<nsIContentSerializer> mSerializer;
  nsCOMPtr<nsIUnicodeEncoder>    mUnicodeEncoder;

  nsString          mMimeType;
  nsString          mCharset;
  PRUint32          mFlags;
  PRUint32          mWrapColumn;
  PRUint32          mStartDepth;
  PRUint32          mEndDepth;
  PRBool            mHaltRangeHint;  
  nsVoidArray       mCommonAncestors;

  PRBool (* mIncludeInContextFP)(nsIDOMNode *aNode);
};

#ifdef XP_MAC
#pragma mark  nsDocumentEncoder implementation 
#pragma mark -
#endif

PRBool
nsDocumentEncoder::IncludeInContext_HTML(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

  if (!content)
    return PR_FALSE;

  nsCOMPtr<nsIAtom> tag;

  content->GetTag(*getter_AddRefs(tag));

  if (tag.get() == nsHTMLAtoms::b ||
      tag.get() == nsHTMLAtoms::i ||
      tag.get() == nsHTMLAtoms::u ||
      tag.get() == nsHTMLAtoms::pre ||
      tag.get() == nsHTMLAtoms::h1 ||
      tag.get() == nsHTMLAtoms::h2 ||
      tag.get() == nsHTMLAtoms::h3 ||
      tag.get() == nsHTMLAtoms::h4 ||
      tag.get() == nsHTMLAtoms::h5 ||
      tag.get() == nsHTMLAtoms::h6) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

NS_IMPL_ADDREF(nsDocumentEncoder)
NS_IMPL_RELEASE(nsDocumentEncoder)

NS_INTERFACE_MAP_BEGIN(nsDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsIDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsDocumentEncoder::nsDocumentEncoder()
{
  NS_INIT_REFCNT();

  mMimeType.AssignWithConversion("text/plain");

  mFlags = 0;
  mWrapColumn = 72;
  mStartDepth = 0;
  mEndDepth = 0;
  mHaltRangeHint = PR_FALSE;
}

nsDocumentEncoder::~nsDocumentEncoder()
{
}

NS_IMETHODIMP
nsDocumentEncoder::Init(nsIDocument* aDocument,
                        const nsAReadableString& aMimeType,
                        PRUint32 aFlags)
{
  if (!aDocument)
    return NS_ERROR_INVALID_ARG;

  mDocument = aDocument;

  mMimeType = aMimeType;

  if (mMimeType.Equals(NS_LITERAL_STRING("text/html"))) {
    mIncludeInContextFP = IncludeInContext_HTML;
  } else {
    mIncludeInContextFP = nsnull;
  }

  mFlags = aFlags;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetWrapColumn(PRUint32 aWC)
{
  mWrapColumn = aWC;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetSelection(nsISelection* aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetRange(nsIDOMRange* aRange)
{
  mRange = aRange;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetCharset(const nsAReadableString& aCharset)
{
  mCharset = aCharset;
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeNodeStart(nsIDOMNode* aNode, PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      nsAWritableString& aStr)
{
  PRUint16 type;

  aNode->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
      mSerializer->AppendElementStart(element, aStr);
      break;
    }
    case nsIDOMNode::TEXT_NODE:
    {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(aNode);
      mSerializer->AppendText(text, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::CDATA_SECTION_NODE:
    {
      nsCOMPtr<nsIDOMCDATASection> cdata = do_QueryInterface(aNode);
      mSerializer->AppendCDATASection(cdata, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    {
      nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(aNode);
      mSerializer->AppendProcessingInstruction(pi, aStartOffset, aEndOffset,
                                               aStr);
      break;
    }
    case nsIDOMNode::COMMENT_NODE:
    {
      nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(aNode);
      mSerializer->AppendComment(comment, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      nsCOMPtr<nsIDOMDocumentType> doctype = do_QueryInterface(aNode);
      mSerializer->AppendDoctype(doctype, aStr);
      break;
    }
  }
  
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeNodeEnd(nsIDOMNode* aNode,
                                    nsAWritableString& aStr)
{
  PRUint16 type;

  aNode->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
      mSerializer->AppendElementEnd(element, aStr);
      break;
    }
  }

  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeToStringRecursive(nsIDOMNode* aNode,
                                              nsAWritableString& aStr)
{
  nsresult rv = SerializeNodeStart(aNode, 0, -1, aStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasChildren = PR_FALSE;

  aNode->HasChildNodes(&hasChildren);

  if (hasChildren) {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    rv = aNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_TRUE(childNodes, NS_SUCCEEDED(rv) ? NS_ERROR_FAILURE : rv);

    PRInt32 index, count;

    childNodes->GetLength((PRUint32*)&count);
    for (index = 0; index < count; index++) {
      nsCOMPtr<nsIDOMNode> child;

      rv = childNodes->Item(index, getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = SerializeToStringRecursive(child, aStr);
      NS_ENSURE_SUCCESS(rv, rv);     
    }
  }

  rv = SerializeNodeEnd(aNode, aStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return FlushText(aStr, PR_FALSE);
}

static nsresult
ConvertAndWrite(nsAReadableString& aString,
                nsIOutputStream* aStream,
                nsIUnicodeEncoder* aEncoder)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aEncoder);
  nsresult rv;
  PRInt32 charLength;
  const PRUnichar* unicodeBuf = (const PRUnichar*)nsPromiseFlatString(aString);
  PRInt32 unicodeLength = aString.Length();

  rv = aEncoder->GetMaxLength(unicodeBuf, unicodeLength, &charLength);
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString charXferString;
    charXferString.SetCapacity(charLength);
    char* charXferBuf = (char*)charXferString.GetBuffer();

    rv = aEncoder->Convert(unicodeBuf, &unicodeLength, charXferBuf, &charLength);
    if (NS_SUCCEEDED(rv)) {
      PRUint32 written;
      rv = aStream->Write(charXferBuf, charLength, &written);
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::FlushText(nsAWritableString& aString, PRBool aForce)
{
  if (!mStream)
    return NS_OK;

  nsresult rv = NS_OK;

  if (aString.Length() > 1024 || aForce) {
    rv = ConvertAndWrite(aString, mStream, mUnicodeEncoder);

    aString.Truncate();
  }

  return rv;
}

static nsresult ChildAt(nsIDOMNode* aNode, PRInt32 aIndex, nsIDOMNode*& aChild)
{
  nsCOMPtr<nsIContent> node(do_QueryInterface(aNode));
  nsCOMPtr<nsIContent> child;

  aChild = nsnull;

  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  node->ChildAt(aIndex, *getter_AddRefs(child));

  if (child)
    child->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)&aChild);

  return NS_OK;
}

static PRInt32 IndexOf(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIContent> parent(do_QueryInterface(aParent));
  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));

  if (!parent || !child)
    return -1;

  PRInt32 indx = 0;

  parent->IndexOf(child, indx);

  return indx;
}

static inline PRInt32 GetIndex(nsVoidArray& aIndexArray)
{
  PRInt32 count = aIndexArray.Count();

  if (count) {
    return (PRInt32)aIndexArray.ElementAt(count - 1);
  }

  return 0;
}

static nsresult GetNextNode(nsIDOMNode* aNode, nsVoidArray& aIndexArray,
                            nsIDOMNode*& aNextNode,
                            nsRangeIterationDirection& aDirection)
{
  PRBool hasChildren;

  aNextNode = nsnull;

  aNode->HasChildNodes(&hasChildren);

  if (hasChildren && aDirection == kDirectionIn) {
    ChildAt(aNode, 0, aNextNode);
    NS_ENSURE_TRUE(aNextNode, NS_ERROR_FAILURE);

    aIndexArray.AppendElement((void *)0);

    aDirection = kDirectionIn;
  } else if (aDirection == kDirectionIn) {
    aNextNode = aNode;

    NS_ADDREF(aNextNode);

    aDirection = kDirectionOut;
  } else {
    nsCOMPtr<nsIDOMNode> parent;

    aNode->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    PRInt32 count = aIndexArray.Count();

    if (count) {
      PRInt32 indx = (PRInt32)aIndexArray.ElementAt(count - 1);

      ChildAt(parent, indx + 1, aNextNode);

      if (aNextNode)
        aIndexArray.ReplaceElementAt((void *)(indx + 1), count - 1);
      else
        aIndexArray.RemoveElementAt(count - 1);
    } else {
      PRInt32 indx = IndexOf(parent, aNode);

      if (indx >= 0) {
        ChildAt(parent, indx + 1, aNextNode);

        if (aNextNode)
          aIndexArray.AppendElement((void *)(indx + 1));
      }
    }

    if (aNextNode) {
      aDirection = kDirectionIn;
    } else {
      aDirection = kDirectionOut;

      aNextNode = parent;

      NS_ADDREF(aNextNode);
    }
  }

  return NS_OK;
}

static PRBool IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode) return PR_FALSE;
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::TEXT_NODE)
    return PR_TRUE;
  return PR_FALSE;
}

static nsresult GetLengthOfDOMNode(nsIDOMNode *aNode, PRUint32 &aCount) 
{
  aCount = 0;
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  nsresult result=NS_OK;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nodeAsChar = do_QueryInterface(aNode);
  if (nodeAsChar) {
    nodeAsChar->GetLength(&aCount);
  }
  else
  {
    PRBool hasChildNodes;
    aNode->HasChildNodes(&hasChildNodes);
    if (PR_TRUE==hasChildNodes)
    {
      nsCOMPtr<nsIDOMNodeList>nodeList;
      result = aNode->GetChildNodes(getter_AddRefs(nodeList));
      if (NS_SUCCEEDED(result) && nodeList) {
        nodeList->GetLength(&aCount);
      }
    }
  }
  return result;
}

nsresult
nsDocumentEncoder::SerializeRangeNodes(nsIDOMRange* aRange, 
                                       nsIDOMNode* aNode, 
                                       nsAWritableString& aString)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
  
  if (IsNodeIntersectsRange(content, aRange))
  {
    PRBool nodeBefore, nodeAfter;
    nsresult rv = CompareNodeToRange(content, aRange, &nodeBefore, &nodeAfter);
    if (!nodeBefore && !nodeAfter) // node completely contained
    {
      rv = SerializeToStringRecursive(aNode, aString);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else // node intersects range, but is not contained.  recurse if needed.
    {
      if (IsTextNode(aNode))
      {
        if (nodeBefore)
        {
          PRInt32 startOffset;
          aRange->GetStartOffset(&startOffset);
          rv = SerializeNodeStart(aNode, startOffset, -1, aString);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else
        {
          PRInt32 endOffset;
          aRange->GetEndOffset(&endOffset);
          rv = SerializeNodeStart(aNode, 0, endOffset, aString);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else
      {
        if (IncludeInContext_HTML(aNode))
        {
          // halt the incrementing of mStartDepth/mEndDepth.  This is
          // so paste client will include this node in paste.
          mHaltRangeHint = PR_TRUE;
        }
        if (nodeBefore && !mHaltRangeHint) mStartDepth++;
        if (nodeAfter && !mHaltRangeHint) mEndDepth++;
        
        rv = SerializeNodeStart(aNode, 0, -1, aString);
        NS_ENSURE_SUCCESS(rv, rv);
      
        nsCOMPtr<nsIDOMNode> child, tmp;
        aNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          rv = SerializeRangeNodes(aRange, child, aString);
          NS_ENSURE_SUCCESS(rv, rv);
          child->GetNextSibling(getter_AddRefs(tmp));
          child = tmp;
        }

        rv = SerializeNodeEnd(aNode, aString);
        NS_ENSURE_SUCCESS(rv, rv); 
      }     
    }
  }
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeRangeContextStart(const nsVoidArray& aAncestorArray,
                                              nsAWritableString& aString)
{
  if (!mIncludeInContextFP)
    return NS_OK;

  PRInt32 i = 0;
  nsresult rv = NS_OK;

  while (1) {
    nsIDOMNode *node = (nsIDOMNode *)aAncestorArray.ElementAt(i++);

    if (!node)
      break;

    if (mIncludeInContextFP(node)) {
      rv = SerializeNodeStart(node, 0, -1, aString);

      if (NS_FAILED(rv))
        break;
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::SerializeRangeContextEnd(const nsVoidArray& aAncestorArray,
                                            nsAWritableString& aString)
{
  if (!mIncludeInContextFP)
    return NS_OK;

  PRInt32 i = aAncestorArray.Count();
  nsresult rv = NS_OK;

  while (i) {
    nsIDOMNode *node = (nsIDOMNode *)aAncestorArray.ElementAt(--i);

    if (!node)
      break;

    if (mIncludeInContextFP(node)) {
      rv = SerializeNodeEnd(node, aString);

      if (NS_FAILED(rv))
        break;
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::SerializeRangeToString(nsIDOMRange *aRange,
                                          nsAWritableString& aOutputString)
{
  if (!aRange)
    return NS_OK;

  PRBool collapsed;

  aRange->GetCollapsed(&collapsed);

  if (collapsed)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> commonParent, startParent, endParent;
  PRInt32 startOffset, endOffset;
  
  aRange->GetCommonAncestorContainer(getter_AddRefs(commonParent));

  if (!commonParent)
    return NS_OK;

  aRange->GetStartContainer(getter_AddRefs(startParent));
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);
  aRange->GetStartOffset(&startOffset);

  aRange->GetEndContainer(getter_AddRefs(endParent));
  NS_ENSURE_TRUE(endParent, NS_ERROR_FAILURE);
  aRange->GetEndOffset(&endOffset);

  mCommonAncestors.Clear();

  nsRange::FillArrayWithAncestors(&mCommonAncestors, commonParent);

  nsresult rv = NS_OK;

  rv = SerializeRangeContextStart(mCommonAncestors, aOutputString);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((startParent == endParent) && IsTextNode(startParent))
  {
    rv = SerializeNodeStart(startParent, startOffset, endOffset, aOutputString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    nsCOMPtr<nsIDOMNode> child, tmp;
    commonParent->GetFirstChild(getter_AddRefs(child));
    while (child)
    {
      rv = SerializeRangeNodes(aRange, child, aOutputString);
      NS_ENSURE_SUCCESS(rv, rv);
      child->GetNextSibling(getter_AddRefs(tmp));
      child = tmp;
    }
  }
  
  rv = SerializeRangeContextEnd(mCommonAncestors, aOutputString);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToString(nsAWritableString& aOutputString)
{
  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  aOutputString.Truncate();

  // xxx Also make sure mString is a mime type "text/html" or "text/plain"

  nsCAutoString progId(NS_CONTENTSERIALIZER_CONTRACTID_PREFIX);
  progId.AppendWithConversion(mMimeType);

  mSerializer = do_CreateInstance(NS_STATIC_CAST(const char *, progId));
  NS_ENSURE_TRUE(mSerializer, NS_ERROR_NOT_IMPLEMENTED);

  mSerializer->Init(mFlags, mWrapColumn);

  nsresult rv = NS_OK;

  if (mSelection) {
    nsCOMPtr<nsIDOMRange> range;
    PRInt32 i, count = 0;

    rv = mSelection->GetRangeCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    for (i = 0; i < count; i++) {
      mSelection->GetRangeAt(i, getter_AddRefs(range));

      rv = SerializeRangeToString(range, aOutputString);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mSelection = nsnull;
  } else if (mRange) {
      rv = SerializeRangeToString(mRange, aOutputString);

      mRange = nsnull;
  } else {
    nsCOMPtr<nsIDOMNode> doc(do_QueryInterface(mDocument));

    rv = SerializeToStringRecursive(doc, aOutputString);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSerializer->Flush(aOutputString);

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToStream(nsIOutputStream* aStream)
{
  nsresult rv = NS_OK;

  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  NS_WITH_SERVICE(nsICharsetConverterManager,
                  charsetConv, 
                  kCharsetConverterManagerCID,
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString charsetStr;
  charsetStr.Assign(mCharset);
  rv = charsetConv->GetUnicodeEncoder(&charsetStr,
                                      getter_AddRefs(mUnicodeEncoder));
  NS_ENSURE_SUCCESS(rv, rv);

  // xxx Also make sure mString is a mime type "text/html" or "text/plain"

  mStream = aStream;

  nsAutoString buf;

  rv = EncodeToString(buf);

  // Force a flush of the last chunk of data.
  FlushText(buf, PR_TRUE);

  mStream = nsnull;
  mUnicodeEncoder = nsnull;

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToStringWithContext(nsAWritableString& aEncodedString, 
                                             nsAWritableString& aContextString,
                                             nsAWritableString& aInfoString)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult NS_NewTextEncoder(nsIDocumentEncoder** aResult); // make mac compiler happy

nsresult
NS_NewTextEncoder(nsIDocumentEncoder** aResult)
{
  *aResult = new nsDocumentEncoder;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
 NS_ADDREF(*aResult);
 return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  nsDocumentEncoder declaration 
#pragma mark -
#endif

class nsHTMLCopyEncoder : public nsDocumentEncoder
{
public:

  nsHTMLCopyEncoder();
  virtual ~nsHTMLCopyEncoder();

  NS_IMETHOD Init(nsIDocument* aDocument, const nsAReadableString& aMimeType, PRUint32 aFlags);

  // overridden methods from nsDocumentEncoder
  NS_IMETHOD SetSelection(nsISelection* aSelection);
  NS_IMETHOD EncodeToStringWithContext(nsAWritableString& aEncodedString, 
                                       nsAWritableString& aContextString,
                                       nsAWritableString& aInfoString);

protected:

  enum Endpoint
  {
    kStart,
    kEnd
  };
  
  nsresult PromoteRange(nsIDOMRange *inRange);
  nsresult GetPromotedPoint(Endpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                                  nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset);
  nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset);
  PRBool IsBody(nsIDOMNode* aNode);
  PRBool IsMozBR(nsIDOMNode* aNode);
  nsresult GetNodeLocation(nsIDOMNode *inChild, nsCOMPtr<nsIDOMNode> *outParent, PRInt32 *outOffset);
  PRBool IsFirstNode(nsIDOMNode *aNode);
  PRBool IsLastNode(nsIDOMNode *aNode);
  PRBool IsEmptyTextContent(nsIDOMNode* aNode);
};

#ifdef XP_MAC
#pragma mark  nsDocumentEncoder implementation 
#pragma mark -
#endif

nsHTMLCopyEncoder::nsHTMLCopyEncoder()
{
}

nsHTMLCopyEncoder::~nsHTMLCopyEncoder()
{
}

NS_IMETHODIMP
nsHTMLCopyEncoder::Init(nsIDocument* aDocument,
                        const nsAReadableString& aIgnored,
                        PRUint32 aFlags)
{
  if (!aDocument)
    return NS_ERROR_INVALID_ARG;

  mDocument = aDocument;

  mMimeType = NS_LITERAL_STRING("text/html");

  mIncludeInContextFP = IncludeInContext_HTML;

  mFlags = aFlags;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCopyEncoder::SetSelection(nsISelection* aSelection)
{
  // normalize selection
  
  // there's no Clone() for selection! fix...
  //nsresult rv = aSelection->Clone(getter_AddRefs(mSelection);
  //NS_ENSURE_SUCCESS(rv, rv);
  mSelection = aSelection;
  
  nsCOMPtr<nsISelectionPrivate> privSelection = do_QueryInterface(mSelection);
  NS_ENSURE_TRUE(privSelection, NS_ERROR_FAILURE);
  
  // get selection range enumerator
  nsCOMPtr<nsIEnumerator> enumerator;
  nsresult rv = privSelection->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  // loop thru the ranges in the selection
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
  {
    rv = enumerator->CurrentItem(getter_AddRefs(currentItem));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);
    
    nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

    // adjust range to include any ancestors who's children are entirely selected
    rv = PromoteRange(range);
    NS_ENSURE_SUCCESS(rv, rv);
    
    enumerator->Next();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCopyEncoder::EncodeToStringWithContext(nsAWritableString& aEncodedString, 
                                             nsAWritableString& aContextString,
                                             nsAWritableString& aInfoString)
{
  nsresult rv = EncodeToString(aEncodedString);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // now encode common ancestors into aContextString.  Note that the common ancestors
  // will be for the last range in the selection in the case of multirange selections.
  // encoding ancestors every range in a multirange selection in a way that could be 
  // understood by the paste code would be a lot more work to do.  As a practical matter,
  // selections are single range, and the ones that aren't are table cell selections
  // where all the cells are in the same table.
  PRInt32 i = mCommonAncestors.Count();
  nsCOMPtr<nsIDOMNode> node;

  node = NS_STATIC_CAST(nsIDOMNode *, mCommonAncestors.ElementAt(--i));

  while (node) 
  {
    SerializeNodeStart(node, 0, -1, aContextString);
    node = NS_STATIC_CAST(nsIDOMNode *, mCommonAncestors.ElementAt(--i));
  }

  i = 0;
  node = NS_STATIC_CAST(nsIDOMNode *, mCommonAncestors.ElementAt(i));

  while (node) 
  {
    SerializeNodeEnd(node, aContextString);
    node = NS_STATIC_CAST(nsIDOMNode *, mCommonAncestors.ElementAt(++i));
  }

  // encode range info : the start and end depth of the selection, where the depth is 
  // distance down in the parent heirarchy.  Later we will need to add leading/trainlig
  // whitespace info to this.
  nsAutoString infoString;
  infoString.AppendInt(mStartDepth);
  infoString.AppendWithConversion(',');
  infoString.AppendInt(mEndDepth);
  aInfoString = infoString;
  
  return NS_OK;
}


nsresult 
nsHTMLCopyEncoder::PromoteRange(nsIDOMRange *inRange)
{
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult rv;
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset, endOffset;
  
  rv = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMNode> opStartNode;
  nsCOMPtr<nsIDOMNode> opEndNode;
  PRInt32 opStartOffset, opEndOffset;
  nsCOMPtr<nsIDOMRange> opRange;
  
  rv = GetPromotedPoint( kStart, startNode, startOffset, &opStartNode, &opStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GetPromotedPoint( kEnd, endNode, endOffset, &opEndNode, &opEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->SetStart(opStartNode, opStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->SetEnd(opEndNode, opEndOffset);
  return rv;
} 

nsresult
nsHTMLCopyEncoder::GetPromotedPoint(Endpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                                  nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMNode> node = aNode;
  nsCOMPtr<nsIDOMNode> parent = aNode;
  PRInt32 offset = aOffset;
  
  // defualt values
  *outNode = node;
  *outOffset = offset;

  if (aWhere == kStart)
  {
    // some special casing for text nodes
    if (IsTextNode(aNode))  
    {
      // if not at beginning of text node, we are done
      if (offset >  0) 
      {
        // unless everything before us in just whitespace.  NOTE: we need a more
        // general solution that truly detects all cases of non-significant
        // whitesace with no false alarms.
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
        nsAutoString text;
        nodeAsText->SubstringData(0, offset, text);
        text.CompressWhitespace();
        if (text.Length())
          return NS_OK;
      }
      // else
      rv = GetNodeLocation(aNode, &parent, &offset);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
      node = GetChildAt(parent,offset);
    }
    if (!node) node = parent;

    // finding the real start for this point.  look up the tree for as long as we are the 
    // first node in the container, and as long as we haven't hit the body node.
    if (!IsBody(node))
    {
      rv = GetNodeLocation(node, &parent, &offset);
      NS_ENSURE_SUCCESS(rv, rv);
      if (offset == -1) return NS_OK; // we hit generated content; STOP
      while ((IsFirstNode(node)) && (!IsBody(parent)))
      {
        node = parent;
        rv = GetNodeLocation(node, &parent, &offset);
        NS_ENSURE_SUCCESS(rv, rv);
        if (offset == -1)  // we hit generated content; STOP
        {
          // back up a bit
          parent = node;
          offset = 0;
          break;
        }
      } 
      *outNode = parent;
      *outOffset = offset;
      return rv;
    }
  }
  
  if (aWhere == kEnd)
  {
    // some special casing for text nodes
    if (IsTextNode(aNode))  
    {
      // if not at end of text node, we are done
      PRUint32 len;
      GetLengthOfDOMNode(aNode, len);
      if (offset < (PRInt32)len)
      {
        // unless everything after us in just whitespace.  NOTE: we need a more
        // general solution that truly detects all cases of non-significant
        // whitesace with no false alarms.
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
        nsAutoString text;
        nodeAsText->SubstringData(offset, len-offset, text);
        text.CompressWhitespace();
        if (text.Length())
          return NS_OK;
      }
      rv = GetNodeLocation(aNode, &parent, &offset);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
      if (offset) offset--; // we want node _before_ offset
      node = GetChildAt(parent,offset);
    }
    if (!node) node = parent;
    
    // finding the real end for this point.  look up the tree for as long as we are the 
    // last node in the container, and as long as we haven't hit the body node.
    if (!IsBody(node))
    {
      rv = GetNodeLocation(node, &parent, &offset);
      NS_ENSURE_SUCCESS(rv, rv);
      if (offset == -1) return NS_OK; // we hit generated content; STOP
      while ((IsLastNode(node)) && (!IsBody(parent)))
      {
        node = parent;
        rv = GetNodeLocation(node, &parent, &offset);
        NS_ENSURE_SUCCESS(rv, rv);
        if (offset == -1)  // we hit generated content; STOP
        {
          // back up a bit
          parent = node;
          offset = 0;
          break;
        }
      } 
      *outNode = parent;
      offset++;  // add one since this in an endpoint - want to be AFTER node.
      *outOffset = offset;
      return rv;
    }
  }
  
  return rv;
}

nsCOMPtr<nsIDOMNode> 
nsHTMLCopyEncoder::GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  if (!aParent) 
    return resultNode;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aParent);
  nsCOMPtr<nsIContent> cChild;
  NS_PRECONDITION(content, "null content in nsHTMLCopyEncoder::GetChildAt");
  
  if (NS_FAILED(content->ChildAt(aOffset, *getter_AddRefs(cChild))))
    return resultNode;
  
  resultNode = do_QueryInterface(cChild);
  return resultNode;
}

PRBool 
nsHTMLCopyEncoder::IsBody(nsIDOMNode* aNode)
{
  if (aNode)
  {
    nsAutoString tag;
    nsCOMPtr<nsIAtom> atom;
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content)
      content->GetTag(*getter_AddRefs(atom));
    if (atom)
    {
      atom->ToString(tag);
      tag.ToLowerCase();
      if (tag.EqualsWithConversion("body"))
      {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

PRBool 
nsHTMLCopyEncoder::IsMozBR(nsIDOMNode* aNode)
{
  if (aNode)
  {
    nsAutoString tag;
    nsCOMPtr<nsIAtom> atom;
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content)
      content->GetTag(*getter_AddRefs(atom));
    if (atom)
    {
      atom->ToString(tag);
      tag.ToLowerCase();
      if (tag.EqualsWithConversion("br"))
      {
        nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
        if (elem)
        {
          nsAutoString typeAttrName; typeAttrName.AssignWithConversion("type");
          nsAutoString typeAttrVal;
          nsresult rv = elem->GetAttribute(typeAttrName, typeAttrVal);
          typeAttrVal.ToLowerCase();
          if (NS_SUCCEEDED(rv) && (typeAttrVal.EqualsWithConversion("_moz")))
            return PR_TRUE;
        }
        return PR_FALSE;
      }
    }
  }
  return PR_FALSE;
}

nsresult 
nsHTMLCopyEncoder::GetNodeLocation(nsIDOMNode *inChild, nsCOMPtr<nsIDOMNode> *outParent, PRInt32 *outOffset)
{
  NS_ASSERTION((inChild && outParent && outOffset), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (inChild && outParent && outOffset)
  {
    result = inChild->GetParentNode(getter_AddRefs(*outParent));
    if ((NS_SUCCEEDED(result)) && (*outParent))
    {
      nsCOMPtr<nsIContent> content = do_QueryInterface(*outParent);
      nsCOMPtr<nsIContent> cChild = do_QueryInterface(inChild);
      if (!cChild || !content) return NS_ERROR_NULL_POINTER;
      result = content->IndexOf(cChild, *outOffset);
    }
  }
  return result;
}

PRBool
nsHTMLCopyEncoder::IsFirstNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset, j=0;
  nsresult rv = GetNodeLocation(aNode, &parent, &offset);
  if (NS_FAILED(rv)) 
  {
    NS_NOTREACHED("failure in IsFirstNode");
    return PR_FALSE;
  }
  if (offset == 0)  // easy case, we are first dom child
    return PR_TRUE;
  if (!parent)  
    return PR_TRUE;
  
  // need to check if any nodes before us are really visible.
  // Mike wrote something for me along these lines in nsSelectionController,
  // but I don't think it's ready for use yet - revisit.
  // HACK: for now, simply consider all whitespace text nodes to be 
  // invisible formatting nodes.
  nsCOMPtr<nsIDOMNodeList> childList;
  nsCOMPtr<nsIDOMNode> child;

  rv = parent->GetChildNodes(getter_AddRefs(childList));
  if (NS_FAILED(rv) || !childList) 
  {
    NS_NOTREACHED("failure in IsFirstNode");
    return PR_TRUE;
  }
  while (j < offset)
  {
    childList->Item(j, getter_AddRefs(child));
    if (!IsEmptyTextContent(child)) 
      return PR_FALSE;
    j++;
  }
  return PR_TRUE;
}


PRBool
nsHTMLCopyEncoder::IsLastNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset,j;
  PRUint32 numChildren;
  nsresult rv = GetNodeLocation(aNode, &parent, &offset);
  if (NS_FAILED(rv)) 
  {
    NS_NOTREACHED("failure in IsLastNode");
    return PR_FALSE;
  }
  GetLengthOfDOMNode(parent, numChildren); 
  if (offset+1 == (PRInt32)numChildren) // easy case, we are last dom child
    return PR_TRUE;
  if (!parent)
    return PR_TRUE;
  // need to check if any nodes after us are really visible.
  // Mike wrote something for me along these lines in nsSelectionController,
  // but I don't think it's ready for use yet - revisit.
  // HACK: for now, simply consider all whitespace text nodes to be 
  // invisible formatting nodes.
  j = (PRInt32)numChildren-1;
  nsCOMPtr<nsIDOMNodeList>childList;
  nsCOMPtr<nsIDOMNode> child;
  rv = parent->GetChildNodes(getter_AddRefs(childList));
  if (NS_FAILED(rv) || !childList) 
  {
    NS_NOTREACHED("failure in IsLastNode");
    return PR_TRUE;
  }
  while (j > offset)
  {
    childList->Item(j, getter_AddRefs(child));
    j--;
    if (IsMozBR(child))  // we ignore trailing moz BRs.  
      continue;
    if (!IsEmptyTextContent(child)) 
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsHTMLCopyEncoder::IsEmptyTextContent(nsIDOMNode* aNode)
{
  PRBool result = PR_FALSE;
  nsCOMPtr<nsITextContent> tc(do_QueryInterface(aNode));
  if (tc) {
    tc->IsOnlyWhitespace(&result);
  }
  return result;
}

nsresult NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult); // make mac compiler happy

nsresult
NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult)
{
  *aResult = new nsHTMLCopyEncoder;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
 NS_ADDREF(*aResult);
 return NS_OK;
}
