/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsCOMPtr.h"
#include "nsIHTMLFragmentContentSink.h"
#include "nsIParser.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLContentContainer.h"
#include "nsHTMLTokens.h"  
#include "nsHTMLEntities.h" 
#include "nsHTMLParts.h"
#include "nsIDOMComment.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsVoidArray.h"
#include "nsITextContent.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "prmem.h"

//
// XXX THIS IS TEMPORARY CODE
// There's a considerable amount of copied code from the
// regular nsHTMLContentSink. All of it will be factored
// at some pointe really soon!
//

static NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
static NS_DEFINE_IID(kIHTMLContentSinkIID, NS_IHTML_CONTENT_SINK_IID);
static NS_DEFINE_IID(kIHTMLFragmentContentSinkIID, NS_IHTML_FRAGMENT_CONTENT_SINK_IID);
static NS_DEFINE_IID(kIDOMCommentIID, NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIDOMDocumentFragmentIID, NS_IDOMDOCUMENTFRAGMENT_IID);

class nsHTMLFragmentContentSink : public nsIHTMLFragmentContentSink {
public:
  nsHTMLFragmentContentSink();
  virtual ~nsHTMLFragmentContentSink();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIContentSink
  NS_IMETHOD WillBuildModel(void);
  NS_IMETHOD DidBuildModel(PRInt32 aQualityLevel);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);  
  
  // nsIHTMLContentSink
  NS_IMETHOD BeginContext(PRInt32 aID);
  NS_IMETHOD EndContext(PRInt32 aID);
  NS_IMETHOD SetTitle(const nsString& aValue);
  NS_IMETHOD OpenHTML(const nsIParserNode& aNode);
  NS_IMETHOD CloseHTML(const nsIParserNode& aNode);
  NS_IMETHOD OpenHead(const nsIParserNode& aNode);
  NS_IMETHOD CloseHead(const nsIParserNode& aNode);
  NS_IMETHOD OpenBody(const nsIParserNode& aNode);
  NS_IMETHOD CloseBody(const nsIParserNode& aNode);
  NS_IMETHOD OpenForm(const nsIParserNode& aNode);
  NS_IMETHOD CloseForm(const nsIParserNode& aNode);
  NS_IMETHOD OpenFrameset(const nsIParserNode& aNode);
  NS_IMETHOD CloseFrameset(const nsIParserNode& aNode);
  NS_IMETHOD OpenMap(const nsIParserNode& aNode);
  NS_IMETHOD CloseMap(const nsIParserNode& aNode);
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsIParserNode& aNode);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD NotifyError(const nsParserError* aError);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);
  NS_IMETHOD DoFragment(PRBool aFlag);

  // nsIHTMLFragmentContentSink
  NS_IMETHOD GetFragment(nsIDOMDocumentFragment** aFragment);

  nsIContent* GetCurrentContent();
  PRInt32 PushContent(nsIContent *aContent);
  nsIContent* PopContent();

  nsresult AddAttributes(const nsIParserNode& aNode,
                         nsIContent* aContent);

  nsresult AddText(const nsString& aString);
  nsresult FlushText();

  PRBool mHitSentinel;

  nsIContent* mRoot;
  nsIParser* mParser;
  nsIDOMHTMLFormElement* mCurrentForm;
  nsIHTMLContent* mCurrentMap;

  nsVoidArray* mContentStack;

  PRUnichar* mText;
  PRInt32 mTextLength;
  PRInt32 mTextSize;
};


nsresult
NS_NewHTMLFragmentContentSink(nsIHTMLFragmentContentSink** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsHTMLFragmentContentSink* it;
  NS_NEWXPCOM(it, nsHTMLFragmentContentSink);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLFragmentContentSinkIID, (void **)aResult);
}

nsHTMLFragmentContentSink::nsHTMLFragmentContentSink()
{
  NS_INIT_REFCNT();
  mHitSentinel = PR_FALSE;
  mRoot = nsnull;
  mParser = nsnull;
  mCurrentForm = nsnull;
  mCurrentMap = nsnull;
  mContentStack = nsnull;
  mText = nsnull;
  mTextLength = 0;
  mTextSize = 0;
}

nsHTMLFragmentContentSink::~nsHTMLFragmentContentSink()
{
  // Should probably flush the text buffer here, just to make sure:
  //FlushText();

  NS_IF_RELEASE(mRoot);
  NS_IF_RELEASE(mParser);
  NS_IF_RELEASE(mCurrentForm);
  NS_IF_RELEASE(mCurrentMap);
  if (nsnull != mContentStack) {
    // there shouldn't be anything here except in an error condition
    PRInt32 indx = mContentStack->Count();
    while (0 < indx--) {
      nsIContent* content = (nsIContent*)mContentStack->ElementAt(indx);
      NS_RELEASE(content);
    }
    delete mContentStack;
  }
  if (nsnull != mText) {
    PR_FREEIF(mText);
  }
}

NS_IMPL_ADDREF(nsHTMLFragmentContentSink)
NS_IMPL_RELEASE(nsHTMLFragmentContentSink)

NS_IMETHODIMP 
nsHTMLFragmentContentSink::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
                                                                         
  *aInstancePtr = NULL;                                                  
                                                                         
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
  if (aIID.Equals(kIHTMLFragmentContentSinkIID)) {
    nsIHTMLFragmentContentSink* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  } 
  if (aIID.Equals(kIHTMLContentSinkIID)) {
    nsIHTMLContentSink* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  } 
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) ((nsISupports*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


NS_IMETHODIMP 
nsHTMLFragmentContentSink::WillBuildModel(void)
{
  nsresult result = NS_OK;

  if (nsnull == mRoot) {
    nsIDOMDocumentFragment* frag;

    result = NS_NewDocumentFragment(&frag, nsnull);
    if (NS_SUCCEEDED(result)) {
      result = frag->QueryInterface(kIContentIID, (void**)&mRoot);
      NS_RELEASE(frag);
    }
  }
  return result;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::DidBuildModel(PRInt32 aQualityLevel)
{
  FlushText();

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::WillInterrupt(void)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::WillResume(void)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::SetParser(nsIParser* aParser)
{
  NS_IF_RELEASE(mParser);
  mParser = aParser;
  NS_IF_ADDREF(mParser);
  
  return NS_OK;
}
  
NS_IMETHODIMP 
nsHTMLFragmentContentSink::BeginContext(PRInt32 aID)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::EndContext(PRInt32 aID)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::SetTitle(const nsString& aValue)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenHTML(const nsIParserNode& aNode)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseHTML(const nsIParserNode& aNode)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenHead(const nsIParserNode& aNode)
{
  // XXX Not likely to get a head in the fragment
  return OpenContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseHead(const nsIParserNode& aNode)
{
  return CloseContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenBody(const nsIParserNode& aNode)
{
  return OpenContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseBody(const nsIParserNode& aNode)
{
  return CloseContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenForm(const nsIParserNode& aNode)
{
  return OpenContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseForm(const nsIParserNode& aNode)
{
  return CloseContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenFrameset(const nsIParserNode& aNode)
{
  return OpenContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseFrameset(const nsIParserNode& aNode)
{
  return CloseContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenMap(const nsIParserNode& aNode)
{
  return OpenContainer(aNode);
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseMap(const nsIParserNode& aNode)
{
  return CloseContainer(aNode);
}

static char* kSentinelStr = "endnote";

NS_IMETHODIMP 
nsHTMLFragmentContentSink::OpenContainer(const nsIParserNode& aNode)
{
  nsAutoString tag;
  nsresult result = NS_OK;

  tag = aNode.GetText();
  if (tag.EqualsIgnoreCase(kSentinelStr)) {
    mHitSentinel = PR_TRUE;
  }
  else if (mHitSentinel) {
    FlushText();
    
    nsIHTMLContent *content = nsnull;
    result = NS_CreateHTMLElement(&content, tag);

    if (NS_OK == result) {
      result = AddAttributes(aNode, content);
      if (NS_OK == result) {
        nsIContent *parent = GetCurrentContent();
        
        if (nsnull == parent) {
          parent = mRoot;
        }
        
        parent->AppendChildTo(content, PR_FALSE);
        PushContent(content);
      }
    }
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::CloseContainer(const nsIParserNode& aNode)
{
  if (mHitSentinel && (nsnull != GetCurrentContent())) {
    nsIContent* content;
    FlushText();
    content = PopContent();
    NS_RELEASE(content);
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::AddLeaf(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;

  switch (aNode.GetTokenType()) {
    case eToken_start:
      {
        FlushText();
        
        // Create new leaf content object
        nsIHTMLContent* content;
        nsAutoString tag = aNode.GetText();
        result = NS_CreateHTMLElement(&content, tag);
        
        if (NS_OK == result) {
          result = AddAttributes(aNode, content);
          if (NS_OK == result) {
            nsIContent *parent = GetCurrentContent();
            
            if (nsnull == parent) {
              parent = mRoot;
            }
            
            parent->AppendChildTo(content, PR_FALSE);
          }
          NS_RELEASE(content);
        }
      }
      break;
    case eToken_text:
    case eToken_whitespace:
    case eToken_newline:
      result = AddText(aNode.GetText());
      break;
      
    case eToken_entity:
      {
        nsAutoString tmp;
        PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
        if (unicode < 0) {
          result = AddText(aNode.GetText());
        }
        else {
          result = AddText(tmp);
        }
      }
      break;
  }
  
  return result;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::NotifyError(const nsParserError* aError)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::AddComment(const nsIParserNode& aNode)
{
  nsIContent *comment;
  nsIDOMComment *domComment;
  nsresult result = NS_OK;

  FlushText();
  
  result = NS_NewCommentNode(&comment);
  if (NS_OK == result) {
    result = comment->QueryInterface(kIDOMCommentIID, 
                                     (void **)&domComment);
    if (NS_OK == result) {
      domComment->AppendData(aNode.GetText());
      NS_RELEASE(domComment);
      
      nsIContent *parent = GetCurrentContent();
      
      if (nsnull == parent) {
        parent = mRoot;
      }
      
      parent->AppendChildTo(comment, PR_FALSE);
    }
    NS_RELEASE(comment);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::DoFragment(PRBool aFlag)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFragmentContentSink::GetFragment(nsIDOMDocumentFragment** aFragment)
{
  if (nsnull != mRoot) {
    return mRoot->QueryInterface(kIDOMDocumentFragmentIID, (void**)aFragment);
  }
  else {
    *aFragment = nsnull;
    return NS_OK;
  }
}

nsIContent* 
nsHTMLFragmentContentSink::GetCurrentContent()
{
  if (nsnull != mContentStack) {
    PRInt32 indx = mContentStack->Count() - 1;
    return (nsIContent *)mContentStack->ElementAt(indx);
  }
  return nsnull;
}

PRInt32 
nsHTMLFragmentContentSink::PushContent(nsIContent *aContent)
{
  if (nsnull == mContentStack) {
    mContentStack = new nsVoidArray();
  }
  
  mContentStack->AppendElement((void *)aContent);
  return mContentStack->Count();
}
 
nsIContent*
nsHTMLFragmentContentSink::PopContent()
{
  nsIContent* content = nsnull;
  if (nsnull != mContentStack) {
    PRInt32 indx = mContentStack->Count() - 1;
    content = (nsIContent *)mContentStack->ElementAt(indx);
    mContentStack->RemoveElementAt(indx);
  }
  return content;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

nsresult
nsHTMLFragmentContentSink::AddText(const nsString& aString)
{
  PRInt32 addLen = aString.Length();
  if (0 == addLen) {
    return NS_OK;
  }

  // Create buffer when we first need it
  if (0 == mTextSize) {
    mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nsnull == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  // Copy data from string into our buffer; flush buffer when it fills up
  PRInt32 offset = 0;
  while (0 != addLen) {
    PRInt32 amount = mTextSize - mTextLength;
    if (amount > addLen) {
      amount = addLen;
    }
    if (0 == amount) {
      nsresult rv = FlushText();
      if (NS_OK != rv) {
        return rv;
      }
    }
    memcpy(&mText[mTextLength], aString.GetUnicode() + offset,
           sizeof(PRUnichar) * amount);
    mTextLength += amount;
    offset += amount;
    addLen -= amount;
  }

  return NS_OK;
}

nsresult
nsHTMLFragmentContentSink::FlushText()
{
  nsresult rv = NS_OK;
  if (0 != mTextLength) {
    nsIContent* content;
    rv = NS_NewTextNode(&content);
    if (NS_OK == rv) {
      
      // Set the text in the text node
      static NS_DEFINE_IID(kITextContentIID, NS_ITEXT_CONTENT_IID);
      nsITextContent* text = nsnull;
      content->QueryInterface(kITextContentIID, (void**) &text);
      text->SetText(mText, mTextLength, PR_FALSE);
      NS_RELEASE(text);

      // Add text to its parent
      nsIContent *parent = GetCurrentContent();
      
      if (nsnull == parent) {
        parent = mRoot;
      }
      
      parent->AppendChildTo(content, PR_FALSE);

      NS_RELEASE(content);
    }

    mTextLength = 0;
  }

  return rv;
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
static void
GetAttributeValueAt(const nsIParserNode& aNode,
                    PRInt32 aIndex,
                    nsString& aResult)
{
  // Copy value
  const nsString& value = aNode.GetValueAt(aIndex);
  aResult.Truncate();
  aResult.Append(value);

  // Strip quotes if present
  PRUnichar first = aResult.First();
  if ((first == '\"') || (first == '\'')) {
    if (aResult.Last() == first) {
      aResult.Cut(0, 1);
      PRInt32 pos = aResult.Length() - 1;
      if (pos >= 0) {
        aResult.Cut(pos, 1);
      }
    } else {
      // Mismatched quotes - leave them in
    }
  }

  // Reduce any entities
  // XXX Note: as coded today, this will only convert well formed
  // entities.  This may not be compatible enough.
  // XXX there is a table in navigator that translates some numeric entities
  // should we be doing that? If so then it needs to live in two places (bad)
  // so we should add a translate numeric entity method from the parser...
  char cbuf[100];
  PRInt32 indx = 0;
  while (indx < aResult.Length()) {
    // If we have the start of an entity (and it's not at the end of
    // our string) then translate the entity into it's unicode value.
    if ((aResult.CharAt(indx++) == '&') && (indx < aResult.Length())) {
      PRInt32 start = indx - 1;
      PRUnichar e = aResult.CharAt(indx);
      if (e == '#') {
        // Convert a numeric character reference
        indx++;
        char* cp = cbuf;
        char* limit = cp + sizeof(cbuf) - 1;
        PRBool ok = PR_FALSE;
        PRInt32 slen = aResult.Length();
        while ((indx < slen) && (cp < limit)) {
          e = aResult.CharAt(indx);
          if (e == ';') {
            indx++;
            ok = PR_TRUE;
            break;
          }
          if ((e >= '0') && (e <= '9')) {
            *cp++ = char(e);
            indx++;
            continue;
          }
          break;
        }
        if (!ok || (cp == cbuf)) {
          continue;
        }
        *cp = '\0';
        if (cp - cbuf > 5) {
          continue;
        }
        PRInt32 ch = PRInt32( ::atoi(cbuf) );
        if (ch > 65535) {
          continue;
        }

        // Remove entity from string and replace it with the integer
        // value.
        aResult.Cut(start, indx - start);
        aResult.Insert(PRUnichar(ch), start);
        indx = start + 1;
      }
      else if (((e >= 'A') && (e <= 'Z')) ||
               ((e >= 'a') && (e <= 'z'))) {
        // Convert a named entity
        indx++;
        char* cp = cbuf;
        char* limit = cp + sizeof(cbuf) - 1;
        *cp++ = char(e);
        PRBool ok = PR_FALSE;
        PRInt32 slen = aResult.Length();
        while ((indx < slen) && (cp < limit)) {
          e = aResult.CharAt(indx);
          if (e == ';') {
            indx++;
            ok = PR_TRUE;
            break;
          }
          if (((e >= '0') && (e <= '9')) ||
              ((e >= 'A') && (e <= 'Z')) ||
              ((e >= 'a') && (e <= 'z'))) {
            *cp++ = char(e);
            indx++;
            continue;
          }
          break;
        }
        if (!ok || (cp == cbuf)) {
          continue;
        }
        *cp = '\0';
        PRInt32 ch = NS_EntityToUnicode(cbuf);
        if (ch < 0) {
          continue;
        }

        // Remove entity from string and replace it with the integer
        // value.
        aResult.Cut(start, indx - start);
        aResult.Insert(PRUnichar(ch), start);
        indx = start + 1;
      }
      else if (e == '{') {
        // Convert a script entity
        // XXX write me!
      }
    }
  }
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
nsresult
nsHTMLFragmentContentSink::AddAttributes(const nsIParserNode& aNode,
                                         nsIContent* aContent)
{
  // Add tag attributes to the content attributes
  nsAutoString k, v;
  PRInt32 ac = aNode.GetAttributeCount();
  for (PRInt32 i = 0; i < ac; i++) {
    // Get upper-cased key
    const nsString& key = aNode.GetKeyAt(i);
    k.Truncate();
    k.Append(key);
    k.ToLowerCase();

    nsIAtom*  keyAtom = NS_NewAtom(k);
    
    if (NS_CONTENT_ATTR_NOT_THERE == 
        aContent->GetAttribute(kNameSpaceID_HTML, keyAtom, v)) {
      // Get value and remove mandatory quotes
      GetAttributeValueAt(aNode, i, v);

      // Add attribute to content
      aContent->SetAttribute(kNameSpaceID_HTML, keyAtom, v, PR_FALSE);
    }
    NS_RELEASE(keyAtom);
  }
  return NS_OK;
}
