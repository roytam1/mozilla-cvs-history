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

#ifndef nsIEditorMailSupport_h__
#define nsIEditorMailSupport_h__


#define NS_IEDITORMAILSUPPORT_IID                  \
{ /* {fdf23301-4a94-11d3-9ce4-9960496c41bc} */     \
0xfdf23301, 0x4a94, 0x11d3,                        \
{ 0x9c, 0xe4, 0x99, 0x60, 0x49, 0x6c, 0x41, 0xbc } }

class nsISupportsArray;
class nsIDOMNode;

class nsIEditorMailSupport : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IEDITORMAILSUPPORT_IID; return iid; }


  /** Paste the text in the OS clipboard at the cursor position,
    * as a quotation (whose representation is dependant on the editor type),
    * replacing the selected text (if any).
    * @param aSelectionType Text or html?
    */
  NS_IMETHOD PasteAsQuotation(PRInt32 aSelectionType)=0;

  /** Insert a string as quoted text
    * (whose representation is dependant on the editor type),
    * replacing the selected text (if any).
    * @param aQuotedText  The actual text to be quoted
    */
  NS_IMETHOD InsertAsQuotation(const nsAString& aQuotedText,
                               nsIDOMNode** aNodeInserted)=0;

  /** Paste a string as quoted text,
    * as a quotation (whose representation is dependant on the editor type),
    * replacing the selected text (if any)
    * @param aCitation    The "mid" URL of the source message
    * @param aSelectionType Text or html?
    */
  NS_IMETHOD PasteAsCitedQuotation(const nsAString& aCitation,
                                   PRInt32 aSelectionType)=0;

  /** Insert a string as quoted text
    * (whose representation is dependant on the editor type),
    * replacing the selected text (if any),
    * including, if possible, a "cite" attribute.
    * @param aQuotedText  The actual text to be quoted
    * @param aCitation    The "mid" URL of the source message
    * @param aInsertHTML  Insert as html?  (vs plaintext)
    * @param aCharset     The charset of the text to be inserted
    */
  NS_IMETHOD InsertAsCitedQuotation(const nsAString& aQuotedText,
                                    const nsAString& aCitation,
                                    PRBool aInsertHTML,
                                    const nsAString& aCharset,
                                    nsIDOMNode** aNodeInserted)=0;

  /**
   * Rewrap the selected part of the document, re-quoting if necessary.
   * @param aRespectNewlines  Try to maintain newlines in the original?
   */
  NS_IMETHOD Rewrap(PRBool aRespectNewlines)=0;

  /**
   * Strip any citations in the selected part of the document.
   */
  NS_IMETHOD StripCites()=0;


  /**
   * Get a list of IMG and OBJECT tags in the current document.
   */
  NS_IMETHOD GetEmbeddedObjects(nsISupportsArray** aNodeList)=0;
};


#endif // nsIEditorMailSupport_h__
