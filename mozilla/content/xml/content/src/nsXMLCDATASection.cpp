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
 * The Original Code is Mozilla Communicator client code.
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

#include "nsIDOMCDATASection.h"
#include "nsGenericDOMDataNode.h"
#include "nsLayoutAtoms.h"

#include "nsContentUtils.h"


class nsXMLCDATASection : public nsGenericDOMDataNode,
                          public nsIDOMCDATASection
{
public:
  nsXMLCDATASection();
  virtual ~nsXMLCDATASection();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  // nsIDOMCharacterData
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  // nsIDOMText
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  // nsIDOMCDATASection
  // Empty interface

  // nsIContent
  NS_IMETHOD GetTag(nsIAtom*& aResult) const;
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
#endif

  // nsITextContent
  NS_IMETHOD CloneContent(PRBool aCloneText, nsITextContent** aClone);

protected:
};

nsresult
NS_NewXMLCDATASection(nsIContent** aInstancePtrResult)
{
  *aInstancePtrResult = new nsXMLCDATASection();
  NS_ENSURE_TRUE(*aInstancePtrResult, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsXMLCDATASection::nsXMLCDATASection()
{
}

nsXMLCDATASection::~nsXMLCDATASection()
{
}


// QueryInterface implementation for nsXMLCDATASection
NS_INTERFACE_MAP_BEGIN(nsXMLCDATASection)
  NS_INTERFACE_MAP_ENTRY(nsITextContent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCharacterData)
  NS_INTERFACE_MAP_ENTRY(nsIDOMText)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCDATASection)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CDATASection)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)

NS_IMPL_ADDREF_INHERITED(nsXMLCDATASection, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsXMLCDATASection, nsGenericDOMDataNode)


NS_IMETHODIMP 
nsXMLCDATASection::GetTag(nsIAtom*& aResult) const
{
  aResult = nsLayoutAtoms::textTagName;
  NS_ADDREF(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLCDATASection::GetNodeName(nsAString& aNodeName)
{
  aNodeName.Assign(NS_LITERAL_STRING("#cdata-section"));
  return NS_OK;
}

NS_IMETHODIMP
nsXMLCDATASection::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::CDATA_SECTION_NODE;
  return NS_OK;
}

NS_IMETHODIMP
nsXMLCDATASection::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsCOMPtr<nsITextContent> textContent;
  nsresult rv = CloneContent(PR_TRUE, getter_AddRefs(textContent));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(textContent, aReturn);
}

NS_IMETHODIMP 
nsXMLCDATASection::CloneContent(PRBool aCloneText, nsITextContent** aReturn)
{
  nsXMLCDATASection* it = new nsXMLCDATASection();
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIContent> kungFuDeathGrip(it);

  if (aCloneText) {
    it->mText = mText;
  }

  *aReturn = it;
  NS_ADDREF(*aReturn);

  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsXMLCDATASection::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "CDATASection refcount=%d<", mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUCS2toASCII(tmp).get(), out);

  fputs(">\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLCDATASection::DumpContent(FILE* out, PRInt32 aIndent,
                               PRBool aDumpAll) const {
  return NS_OK;
}
#endif
