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
 *     Jean-Francois Ducarroz <ducarroz@netscape.com>
 *     Ben Bucksch <mozilla@bucksch.org>
 *     H�kan Waara <hwaara@chello.se>
 *     Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nsMsgCompose.h"

#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsMsgI18N.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsMsgCompCID.h"
#include "nsMsgQuote.h"
#include "nsIPref.h"
#include "nsIDocumentEncoder.h"    // for editor output flags
#include "nsXPIDLString.h"
#include "nsIMsgHeaderParser.h"
#include "nsMsgCompUtils.h"
#include "nsIMsgStringService.h"
#include "nsMsgComposeStringBundle.h"
#include "nsSpecialSystemDirectory.h"
#include "nsMsgSend.h"
#include "nsMsgCreate.h"
#include "nsMailHeaders.h"
#include "nsMsgPrompts.h"
#include "nsMimeTypes.h"
#include "nsICharsetConverterManager.h"
#include "nsTextFormatter.h"
#include "nsIEditorShell.h"
#include "nsIPlaintextEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIEditorMailSupport.h"
#include "nsEscape.h"
#include "plstr.h"
#include "nsIDocShell.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsAbBaseCID.h"
#include "nsIAddrDatabase.h"
#include "nsIAddrBookSession.h"
#include "nsIAddressBook.h"
#include "nsCExternalHandlerService.h"
#include "nsIMIMEService.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWindowMediator.h"
#include "nsISupportsArray.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "nsIPrompt.h"
#include "nsMsgMimeCID.h"
#include "nsCOMPtr.h"
#include "nsDateTimeFormatCID.h"
#include "nsIDateTimeFormat.h"
#include "nsILocaleService.h"
#include "nsILocale.h"
#include "nsMsgComposeService.h"
#include "nsIMsgComposeProgressParams.h"
#include "nsMsgUtils.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapCore.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsNetUtil.h"
#include "nsMsgSimulateError.h"
#include "nsIAddrDatabase.h"
#include "nsILocalFile.h"

// Defines....
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kDateTimeFormatCID, NS_DATETIMEFORMAT_CID);

static PRInt32 GetReplyOnTop()
{
  PRInt32 reply_on_top = 1;
  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs)
    prefs->GetIntPref("mailnews.reply_on_top", &reply_on_top);
  return reply_on_top;
}

static nsresult GetReplyHeaderInfo(PRInt32* reply_header_type, 
                                   PRUnichar** reply_header_locale,
                                   PRUnichar** reply_header_authorwrote,
                                   PRUnichar** reply_header_ondate,
                                   PRUnichar** reply_header_separator,
                                   PRUnichar** reply_header_colon,
                                   PRUnichar** reply_header_originalmessage)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs) {
    rv = prefs->GetIntPref("mailnews.reply_header_type", reply_header_type);
    if (NS_FAILED(rv))
      *reply_header_type = 1;

    rv = prefs->CopyUnicharPref("mailnews.reply_header_locale", reply_header_locale);
    if (NS_FAILED(rv) || !*reply_header_locale)
      *reply_header_locale = nsCRT::strdup(NS_LITERAL_STRING("").get());

    rv = prefs->GetLocalizedUnicharPref("mailnews.reply_header_authorwrote", reply_header_authorwrote);
    if (NS_FAILED(rv) || !*reply_header_authorwrote)
      *reply_header_authorwrote = nsCRT::strdup(NS_LITERAL_STRING("%s wrote").get());

    rv = prefs->CopyUnicharPref("mailnews.reply_header_ondate", reply_header_ondate);
    if (NS_FAILED(rv) || !*reply_header_ondate)
      *reply_header_ondate = nsCRT::strdup(NS_LITERAL_STRING("On %s").get());

    rv = prefs->CopyUnicharPref("mailnews.reply_header_separator", reply_header_separator);
    if (NS_FAILED(rv) || !*reply_header_separator)
      *reply_header_separator = nsCRT::strdup(NS_LITERAL_STRING(", ").get());

    rv = prefs->CopyUnicharPref("mailnews.reply_header_colon", reply_header_colon);
    if (NS_FAILED(rv) || !*reply_header_colon)
      *reply_header_colon = nsCRT::strdup(NS_LITERAL_STRING(":").get());

    rv = prefs->GetLocalizedUnicharPref("mailnews.reply_header_originalmessage", reply_header_originalmessage);
    if (NS_FAILED(rv) || !*reply_header_originalmessage)
      *reply_header_originalmessage = nsCRT::strdup(NS_LITERAL_STRING("--- Original Message ---").get());
  }
  return rv;
}

static nsresult RemoveDuplicateAddresses(const char * addresses, const char * anothersAddresses, PRBool removeAliasesToMe, char** newAddress)
{
  nsresult rv;

  nsCOMPtr<nsIMsgHeaderParser> parser (do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID));
  if (parser)
    rv= parser->RemoveDuplicateAddresses("UTF-8", addresses, anothersAddresses, removeAliasesToMe, newAddress);
  else
    rv = NS_ERROR_FAILURE;

  return rv;
}

static void TranslateLineEnding(nsString& data)
{
  PRUnichar* rPtr;   //Read pointer
  PRUnichar* wPtr;   //Write pointer
  PRUnichar* sPtr;   //Start data pointer
  PRUnichar* ePtr;   //End data pointer

  rPtr = wPtr = sPtr = NS_CONST_CAST(PRUnichar*, data.get());
  ePtr = rPtr + data.Length();

  while (rPtr < ePtr)
  {
    if (*rPtr == 0x0D)
      if (rPtr + 1 < ePtr && *(rPtr + 1) == 0x0A)
      {
        *wPtr = 0x0A;
        rPtr ++;
      }
      else
        *wPtr = 0x0A;
    else
      *wPtr = *rPtr;

    rPtr ++;
    wPtr ++;
  }

  data.SetLength(wPtr - sPtr);
}

static void GetTopmostMsgWindowCharacterSet(nsXPIDLString& charset, PRBool* charsetOverride)
{
  // HACK: if we are replying to a message and that message used a charset over ride
  // (as specified in the top most window (assuming the reply originated from that window)
  // then use that over ride charset instead of the charset specified in the message
  nsCOMPtr <nsIMsgMailSession> mailSession (do_GetService(NS_MSGMAILSESSION_CONTRACTID));          
  if (mailSession)
  {
    nsCOMPtr<nsIMsgWindow>    msgWindow;
    mailSession->GetTopmostMsgWindow(getter_AddRefs(msgWindow));
    if (msgWindow)
    {
      nsXPIDLString mailCharset;
      msgWindow->GetMailCharacterSet(getter_Copies(charset));
      msgWindow->GetCharsetOverride(charsetOverride);
    }
  }
}

nsMsgCompose::nsMsgCompose()
{
#if defined(DEBUG_ducarroz)
  printf("CREATE nsMsgCompose: %x\n", this);
#endif

  NS_INIT_ISUPPORTS();

  mQuotingToFollow = PR_FALSE;
  mWhatHolder = 1;
  mDocumentListener = nsnull;
  m_window = nsnull;
  m_editorShell = nsnull;
  m_editor = nsnull;
  mQuoteStreamListener=nsnull;
  mCharsetOverride = PR_FALSE;
  m_compFields = nsnull;    //m_compFields will be set during nsMsgCompose::Initialize
  mType = nsIMsgCompType::New;

  // For TagConvertible
  // Read and cache pref
  mConvertStructs = PR_FALSE;
  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs)
    prefs->GetBoolPref("converter.html2txt.structs", &mConvertStructs);

  m_composeHTML = PR_FALSE;
  mRecycledWindow = PR_TRUE;
}


nsMsgCompose::~nsMsgCompose()
{
#if defined(DEBUG_ducarroz)
  printf("DISPOSE nsMsgCompose: %x\n", this);
#endif

  if (mDocumentListener)
  {
    mDocumentListener->SetComposeObj(nsnull);      
    NS_RELEASE(mDocumentListener);
  }
  NS_IF_RELEASE(m_compFields);
  NS_IF_RELEASE(mQuoteStreamListener);
}

/* the following macro actually implement addref, release and query interface for our component. */
NS_IMPL_ISUPPORTS2(nsMsgCompose, nsIMsgCompose, nsISupportsWeakReference)

//
// Once we are here, convert the data which we know to be UTF-8 to UTF-16
// for insertion into the editor
//
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

nsresult 
GetChildOffset(nsIDOMNode *aChild, nsIDOMNode *aParent, PRInt32 &aOffset)
{
  NS_ASSERTION((aChild && aParent), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (aChild && aParent)
  {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    result = aParent->GetChildNodes(getter_AddRefs(childNodes));
    if ((NS_SUCCEEDED(result)) && (childNodes))
    {
      PRInt32 i=0;
      for ( ; NS_SUCCEEDED(result); i++)
      {
        nsCOMPtr<nsIDOMNode> childNode;
        result = childNodes->Item(i, getter_AddRefs(childNode));
        if ((NS_SUCCEEDED(result)) && (childNode))
        {
          if (childNode.get()==aChild)
          {
            aOffset = i;
            break;
          }
        }
        else if (!childNode)
          result = NS_ERROR_NULL_POINTER;
      }
    }
    else if (!childNodes)
      result = NS_ERROR_NULL_POINTER;
  }
  return result;
}

nsresult 
GetNodeLocation(nsIDOMNode *inChild, nsCOMPtr<nsIDOMNode> *outParent, PRInt32 *outOffset)
{
  NS_ASSERTION((outParent && outOffset), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (inChild && outParent && outOffset)
  {
    result = inChild->GetParentNode(getter_AddRefs(*outParent));
    if ( (NS_SUCCEEDED(result)) && (*outParent) )
    {
      result = GetChildOffset(inChild, *outParent, *outOffset);
    }
  }

  return result;
}

PRBool nsMsgCompose::IsEmbeddedObjectSafe(const char * originalScheme,
                                          const char * originalHost,
                                          const char * originalPath,
                                          nsIDOMNode * object)
{
  nsresult rv;

  nsCOMPtr<nsIDOMHTMLImageElement> image;
  nsCOMPtr<nsIDOMHTMLLinkElement> link;
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor;
  nsAutoString objURL;

  if (!object || !originalScheme || !originalPath) //having a null host is ok...
    return PR_FALSE;

  if ((image = do_QueryInterface(object)))
  {
    if (NS_FAILED(image->GetSrc(objURL)))
      return PR_FALSE;
  }
  else if ((link = do_QueryInterface(object)))
  {
    if (NS_FAILED(link->GetHref(objURL)))
      return PR_FALSE;
  }
  else if ((anchor = do_QueryInterface(object)))
  {
    if (NS_FAILED(anchor->GetHref(objURL)))
      return PR_FALSE;
  }
  else
    return PR_FALSE;

  if (!objURL.IsEmpty())
  {
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), objURL);
    if (NS_SUCCEEDED(rv) && uri)
    {
      nsCAutoString scheme;
      rv = uri->GetScheme(scheme);
      if (NS_SUCCEEDED(rv) && (nsCRT::strcasecmp(scheme.get(), originalScheme) == 0))
      {
        nsCAutoString host;
        rv = uri->GetAsciiHost(host);
        // mailbox url don't have a host therefore don't be too strict.
        if (NS_SUCCEEDED(rv) && (host.IsEmpty() || originalHost || (nsCRT::strcasecmp(host.get(), originalHost) == 0)))
        {
          nsCAutoString path;
          rv = uri->GetPath(path);
          if (NS_SUCCEEDED(rv))
          {
            const char * query = strrchr(path.get(), '?');
            if (query && nsCRT::strncasecmp(path.get(), originalPath, query - path.get()) == 0)
              return PR_TRUE; //This object is a part of the original message, we can send it safely.
          }
        }
      }
    }
  }
  
  return PR_FALSE;
}

/* The purpose of this function is to mark any embedded object that wasn't a RFC822 part
   of the original message as moz-do-not-send.
   That will prevent us to attach data not specified by the user or not present in the
   original message.
*/
nsresult nsMsgCompose::TagEmbeddedObjects(nsIEditorMailSupport *aEditor)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupportsArray> aNodeList;
  PRUint32 count;
  PRUint32 i;

  if (!aEditor)
    return NS_ERROR_FAILURE;

  rv = aEditor->GetEmbeddedObjects(getter_AddRefs(aNodeList));
  if ((NS_FAILED(rv) || (!aNodeList)))
    return NS_ERROR_FAILURE;

  if (NS_FAILED(aNodeList->Count(&count)))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> isupp;
  nsCOMPtr<nsIDOMNode> node;

  nsCOMPtr<nsIURI> originalUrl;
  nsXPIDLCString originalScheme;
  nsXPIDLCString originalHost;
  nsXPIDLCString originalPath;

  // first, convert the rdf original msg uri into a url that represents the message...
  nsCOMPtr <nsIMsgMessageService> msgService;
  rv = GetMessageServiceFromURI(mQuoteURI.get(), getter_AddRefs(msgService));
  if (NS_SUCCEEDED(rv))
  {
    rv = msgService->GetUrlForUri(mQuoteURI.get(), getter_AddRefs(originalUrl), nsnull);
    if (NS_SUCCEEDED(rv) && originalUrl)
    {
      originalUrl->GetScheme(originalScheme);
      originalUrl->GetAsciiHost(originalHost);
      originalUrl->GetPath(originalPath);
    }
  }

  // Then compare the url of each embedded objects with the original message.
  // If they a not coming from the original message, they should not be sent
  // with the message.
  nsCOMPtr<nsIDOMElement> domElement;
  for (i = 0; i < count; i ++)
  {
    isupp = getter_AddRefs(aNodeList->ElementAt(i));
    if (!isupp)
      continue;

    node = do_QueryInterface(isupp);
    if (IsEmbeddedObjectSafe(originalScheme.get(), originalHost.get(),
                             originalPath.get(), node))
      continue; //Don't need to tag this object, it safe to send it.
    
    //The source of this object should not be sent with the message 
    domElement = do_QueryInterface(isupp);
    if (domElement)
      domElement->SetAttribute(NS_LITERAL_STRING("moz-do-not-send"), NS_LITERAL_STRING("true"));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMsgCompose::SetEditorFromEditorShell()
{
  NS_ASSERTION(m_editorShell, "SetEditorFromEditorShell() but no editor shell yet!");
  if (!m_editorShell)
    return NS_ERROR_UNEXPECTED;
  return m_editorShell->GetEditor(getter_AddRefs(m_editor));
}

NS_IMETHODIMP
nsMsgCompose::ConvertAndLoadComposeWindow(nsString& aPrefix,
                                          nsString& aBuf,
                                          nsString& aSignature,
                                          PRBool aQuoted,
                                          PRBool aHTMLEditor)
{
  NS_ASSERTION(m_editor, "ConvertAndLoadComposeWindow but no editor\n");
  if (!m_editor)
    return NS_ERROR_FAILURE;

  // First, get the nsIEditor interface for future use
  nsCOMPtr<nsIDOMNode> nodeInserted;

  TranslateLineEnding(aPrefix);
  TranslateLineEnding(aBuf);
  TranslateLineEnding(aSignature);

  // We're going to be inserting stuff, and MsgComposeCommands
  // may have set the editor to readonly in the recycled case.
  // So set it back to writable.
  // Note!  enableEditableFields in gComposeRecyclingListener::onReopen
  // will redundantly set this flag to writable, but it gets there
  // too late.  This will be revisited when cmanske lands the
  // next stage of editorshell removal.
  PRUint32 flags = 0;
  m_editor->GetFlags(&flags);
  flags &= ~nsIPlaintextEditor::eEditorReadonlyMask;
  m_editor->SetFlags(flags);

  m_editor->EnableUndo(PR_FALSE);

  // Ok - now we need to figure out the charset of the aBuf we are going to send
  // into the editor shell. There are I18N calls to sniff the data and then we need
  // to call the new routine in the editor that will allow us to send in the charset
  //

  // Now, insert it into the editor...
  nsCOMPtr<nsIHTMLEditor> htmlEditor (do_QueryInterface(m_editor));
  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(m_editor));
  nsCOMPtr<nsIEditorMailSupport> mailEditor (do_QueryInterface(m_editor));
  m_editor->BeginTransaction();
  if ( (aQuoted) )
  {
    if (!aPrefix.IsEmpty())
    {
      if (aHTMLEditor && htmlEditor)
        htmlEditor->InsertHTML(aPrefix);
      else if (textEditor)
        textEditor->InsertText(aPrefix);
      m_editor->EndOfDocument();
    }

    if (!aBuf.IsEmpty() && mailEditor)
    {
      if (aHTMLEditor && !mCiteReference.IsEmpty())
        mailEditor->InsertAsCitedQuotation(aBuf,
                                           mCiteReference,
                                           PR_TRUE,
                                           NS_LITERAL_STRING("UTF-8"),
                                           getter_AddRefs(nodeInserted));
      else
        mailEditor->InsertAsQuotation(aBuf,
                                      getter_AddRefs(nodeInserted));
      m_editor->EndOfDocument();
    }

    (void)TagEmbeddedObjects(mailEditor);

    if (!aSignature.IsEmpty())
    {
      if (aHTMLEditor && htmlEditor)
        htmlEditor->InsertHTML(aSignature);
      else if (textEditor)
        textEditor->InsertText(aSignature);
    }
  }
  else
  {
    if (aHTMLEditor && htmlEditor)
    {
      if (!aBuf.IsEmpty())
      {
        PRInt32 start;
        PRInt32 end;
        nsAutoString bodyAttributes;
        nsAutoString headContent;

        /* Before we can insert the data into editor, we need to remove the <head> tag
           content and then insert it back using ReplaceHeadContentsWithHTML */
        start = aBuf.Find("<head>", PR_TRUE);
        if (start != kNotFound)
        {
          start += 6; // move pass the <head> tag
          end = aBuf.Find("</head>", PR_TRUE, start);
          if (end != kNotFound)
          {
            const PRUnichar* data = aBuf.get();
            headContent.Adopt(nsCRT::strndup(&data[start], end - start));
            aBuf.Cut(start, end - start);
          }
        }

        /* If we have attribute for the body tag, we need to save them in order
           to add them back later as InsertSource will ignore them. */
        start = aBuf.Find("<body", PR_TRUE);
        if (start != kNotFound && aBuf[start + 5] == ' ')
        {
          start += 6;
          end = aBuf.FindChar('>', start);
          if (end != kNotFound)
          {
            const PRUnichar* data = aBuf.get();
            bodyAttributes.Adopt(nsCRT::strndup(&data[start], end - start));
          }
        }

        if (!headContent.IsEmpty())
          htmlEditor->ReplaceHeadContentsWithHTML(headContent);
        htmlEditor->InsertHTML(aBuf);

        m_editor->EndOfDocument();
        SetBodyAttributes(bodyAttributes);
      }
      if (!aSignature.IsEmpty())
        htmlEditor->InsertHTML(aSignature);
    }
    else if (textEditor)
    {
      if (!aBuf.IsEmpty())
      {
        if (mailEditor)
          mailEditor->InsertTextWithQuotations(aBuf);
        else
          textEditor->InsertText(aBuf);
        m_editor->EndOfDocument();
      }

      if (!aSignature.IsEmpty())
        textEditor->InsertText(aSignature);
    }
  }
  m_editor->EndTransaction();
  
  if (m_editor)
  {
    if (aBuf.IsEmpty())
      m_editor->BeginningOfDocument();
    else
      switch (GetReplyOnTop())
      {
        // This should set the cursor after the body but before the sig
        case 0  :
        {
          if (!textEditor)
          {
            m_editor->BeginningOfDocument();
            break;
          }

          nsCOMPtr<nsISelection> selection = nsnull; 
          nsCOMPtr<nsIDOMNode>      parent = nsnull; 
          PRInt32                   offset;
          nsresult                  rv;

          // get parent and offset of mailcite
          rv = GetNodeLocation(nodeInserted, address_of(parent), &offset);
          if (NS_FAILED(rv) || (!parent))
          {
            m_editor->BeginningOfDocument();
            break;
          }

          // get selection
          m_editor->GetSelection(getter_AddRefs(selection));
          if (!selection)
          {
            m_editor->BeginningOfDocument();
            break;
          }

          // place selection after mailcite
          selection->Collapse(parent, offset+1);

          // insert a break at current selection
          textEditor->InsertLineBreak();

          // i'm not sure if you need to move the selection back to before the
          // break. expirement.
          selection->Collapse(parent, offset+1);
 
          break;
        }
      
      case 2  : 
      {
        m_editor->SelectAll();
        break;
      }
      
      // This should set the cursor to the top!
      default : m_editor->BeginningOfDocument();    break;
    }

    nsCOMPtr<nsISelectionController> selCon;
    m_editor->GetSelectionController(getter_AddRefs(selCon));

    if (selCon)
      selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_ANCHOR_REGION, PR_TRUE);
  }

  if (m_editor)
    m_editor->EnableUndo(PR_TRUE);
  SetBodyModified(PR_FALSE);

#ifdef MSGCOMP_TRACE_PERFORMANCE
  nsCOMPtr<nsIMsgComposeService> composeService (do_GetService(NS_MSGCOMPOSESERVICE_CONTRACTID));
  composeService->TimeStamp("Finished inserting data into the editor. The window is finally ready!", PR_FALSE);
#endif
  return NS_OK;
}

nsresult 
nsMsgCompose::SetQuotingToFollow(PRBool aVal)
{
  mQuotingToFollow = aVal;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgCompose::GetQuotingToFollow(PRBool* quotingToFollow)
{
  NS_ENSURE_ARG(quotingToFollow);
  *quotingToFollow = mQuotingToFollow;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgCompose::Initialize(nsIDOMWindowInternal *aWindow, nsIMsgComposeParams *params)
{
  NS_ENSURE_ARG_POINTER(params);
  nsresult rv;

  params->GetIdentity(getter_AddRefs(m_identity));

  if (aWindow)
  {
    m_window = aWindow;
    nsCOMPtr<nsIDocShell> docshell;
    nsCOMPtr<nsIScriptGlobalObject> globalObj(do_QueryInterface(aWindow));
    if (!globalObj)
      return NS_ERROR_FAILURE;
    
    globalObj->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDocShellTreeItem>  treeItem(do_QueryInterface(docshell));
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    rv = treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
    if (NS_FAILED(rv)) return rv;

    m_baseWindow = do_QueryInterface(treeOwner);
  }
  
  MSG_ComposeFormat format;
  params->GetFormat(&format);
  
  MSG_ComposeType type;
  params->GetType(&type);

  nsXPIDLCString originalMsgURI;
  params->GetOriginalMsgURI(getter_Copies(originalMsgURI));
  
  nsCOMPtr<nsIMsgCompFields> composeFields;
  params->GetComposeFields(getter_AddRefs(composeFields));

  nsCOMPtr<nsIMsgComposeService> composeService = do_GetService(NS_MSGCOMPOSESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
 
  rv = composeService->DetermineComposeHTML(m_identity, format, &m_composeHTML);
  NS_ENSURE_SUCCESS(rv,rv);

  params->GetSendListener(getter_AddRefs(mExternalSendListener));
  nsXPIDLCString smtpPassword;
  params->GetSmtpPassword(getter_Copies(smtpPassword));
  mSmtpPassword = (const char *)smtpPassword;

  return CreateMessage(originalMsgURI, type, composeFields);
}

nsresult nsMsgCompose::SetDocumentCharset(const char *charset) 
{
  // Set charset, this will be used for the MIME charset labeling.
  m_compFields->SetCharacterSet(charset);
  
  // notify the change to editor
  m_editor->SetDocumentCharacterSet(NS_ConvertASCIItoUCS2(charset));

  return NS_OK;
}

nsresult nsMsgCompose::RegisterStateListener(nsIMsgComposeStateListener *stateListener)
{
  nsresult rv = NS_OK;
  
  if (!stateListener)
    return NS_ERROR_NULL_POINTER;
  
  if (!mStateListeners)
  {
    rv = NS_NewISupportsArray(getter_AddRefs(mStateListeners));
    if (NS_FAILED(rv)) return rv;
  }
  nsCOMPtr<nsISupports> iSupports = do_QueryInterface(stateListener, &rv);
  if (NS_FAILED(rv)) return rv;

  // note that this return value is really a PRBool, so be sure to use
  // NS_SUCCEEDED or NS_FAILED to check it.
  return mStateListeners->AppendElement(iSupports);
}

nsresult nsMsgCompose::UnregisterStateListener(nsIMsgComposeStateListener *stateListener)
{
  if (!stateListener)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  
  // otherwise, see if it exists in our list
  if (!mStateListeners)
    return (nsresult)PR_FALSE;      // yeah, this sucks, but I'm emulating the behaviour of
                                    // nsISupportsArray::RemoveElement()

  nsCOMPtr<nsISupports> iSupports = do_QueryInterface(stateListener, &rv);
  if (NS_FAILED(rv)) return rv;

  // note that this return value is really a PRBool, so be sure to use
  // NS_SUCCEEDED or NS_FAILED to check it.
  return mStateListeners->RemoveElement(iSupports);
}

nsresult nsMsgCompose::_SendMsg(MSG_DeliverMode deliverMode, nsIMsgIdentity *identity, PRBool entityConversionDone)
{
  nsresult rv = NS_OK;
    
  if (m_compFields && identity) 
  {
    // Pref values are supposed to be stored as UTF-8, so no conversion
    nsXPIDLCString email;
    nsXPIDLString fullName;
    nsXPIDLString organization;
    
    identity->GetEmail(getter_Copies(email));
    identity->GetFullName(getter_Copies(fullName));
    identity->GetOrganization(getter_Copies(organization));
    
    char * sender = nsnull;
    nsCOMPtr<nsIMsgHeaderParser> parser (do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID));
    if (parser) {
      // convert to UTF8 before passing to MakeFullAddress
      parser->MakeFullAddress(nsnull, NS_ConvertUCS2toUTF8(fullName).get(), email, &sender);
    }
  
  if (!sender)
    m_compFields->SetFrom(email);
  else
    m_compFields->SetFrom(sender);
  PR_FREEIF(sender);

    m_compFields->SetOrganization(organization);
    
#if defined(DEBUG_ducarroz) || defined(DEBUG_seth_)
  {
    printf("----------------------------\n");
    printf("--  Sending Mail Message  --\n");
    printf("----------------------------\n");
    printf("from: %s\n", m_compFields->GetFrom());
    printf("To: %s  Cc: %s  Bcc: %s\n", m_compFields->GetTo(), m_compFields->GetCc(), m_compFields->GetBcc());
    printf("Newsgroups: %s\n", m_compFields->GetNewsgroups());
    printf("Subject: %s  \nMsg: %s\n", m_compFields->GetSubject(), m_compFields->GetBody());
    nsCOMPtr<nsISupportsArray> attachmentsArray;
    m_compFields->GetAttachmentsArray(getter_AddRefs(attachmentsArray));
    if (attachmentsArray)
    {
      PRUint32 i;
      PRUint32 attachmentCount = 0;
      attachmentsArray->Count(&attachmentCount);

      nsCOMPtr<nsIMsgAttachment> element;
      for (i = 0; i < attachmentCount; i ++)
      {
        attachmentsArray->QueryElementAt(i, NS_GET_IID(nsIMsgAttachment), getter_AddRefs(element));
        if (element)
        {
          nsXPIDLString name;
          nsXPIDLCString url;
          nsXPIDLCString nameCStr;
          element->GetName(getter_Copies(name));
          nameCStr.Adopt(ToNewCString(name));
          element->GetUrl(getter_Copies(url));
          printf("Attachment %d: %s - %s\n",i + 1, nameCStr.get(), url.get());
        }
      }
    }
    printf("----------------------------\n");
  }
#endif //DEBUG

    mMsgSend = do_CreateInstance(NS_MSGSEND_CONTRACTID);
    if (mMsgSend)
    {
      PRBool      newBody = PR_FALSE;
      char        *bodyString = (char *)m_compFields->GetBody();
      PRInt32     bodyLength;
      const char  attachment1_type[] = TEXT_HTML;  // we better be "text/html" at this point

      if (!entityConversionDone)
      {
        // Convert body to mail charset
        char      *outCString;
      
        if (  bodyString && *bodyString )
        {
          // Apply entity conversion then convert to a mail charset. 
          PRBool isAsciiOnly;
          rv = nsMsgI18NSaveAsCharset(attachment1_type, m_compFields->GetCharacterSet(), 
                                      NS_ConvertASCIItoUCS2(bodyString).get(), &outCString,
                                      nsnull, &isAsciiOnly);
          if (NS_SUCCEEDED(rv)) 
          {
            m_compFields->SetBodyIsAsciiOnly(isAsciiOnly);
            bodyString = outCString;
            newBody = PR_TRUE;
          }
        }
      }
      
      bodyLength = PL_strlen(bodyString);
      
      // Create the listener for the send operation...
      nsCOMPtr<nsIMsgComposeSendListener> composeSendListener = do_CreateInstance(NS_MSGCOMPOSESENDLISTENER_CONTRACTID);
      if (!composeSendListener)
        return NS_ERROR_OUT_OF_MEMORY;
      
      composeSendListener->SetMsgCompose(this);
      composeSendListener->SetDeliverMode(deliverMode);

      if (mProgress)
      {
        nsCOMPtr<nsIWebProgressListener> progressListener = do_QueryInterface(composeSendListener);
        mProgress->RegisterListener(progressListener);
      }
            
      // If we are composing HTML, then this should be sent as
      // multipart/related which means we pass the editor into the
      // backend...if not, just pass nsnull
      //
      nsCOMPtr<nsIMsgSendListener> sendListener = do_QueryInterface(composeSendListener);
      rv = mMsgSend->CreateAndSendMessage(
                    m_composeHTML ? m_editor.get() : nsnull,
                    identity,
                    m_compFields, 
                    PR_FALSE,                           // PRBool                            digest_p,
                    PR_FALSE,                           // PRBool                            dont_deliver_p,
                    (nsMsgDeliverMode)deliverMode,      // nsMsgDeliverMode                  mode,
                    nsnull,                             // nsIMsgDBHdr *msgToReplace                        *msgToReplace, 
                    m_composeHTML?TEXT_HTML:TEXT_PLAIN, // const char                        *attachment1_type,
                    bodyString,                         // const char                        *attachment1_body,
                    bodyLength,                         // PRUint32                          attachment1_body_length,
                    nsnull,                             // const struct nsMsgAttachmentData  *attachments,
                    nsnull,                             // const struct nsMsgAttachedFile    *preloaded_attachments,
                    nsnull,                             // nsMsgSendPart                     *relatedPart,
                    m_window,                           // nsIDOMWindowInternal              *parentWindow;
                    mProgress,                          // nsIMsgProgress                    *progress,
                    sendListener,                       // listener
                    mSmtpPassword.get());

      // Cleanup converted body...
      if (newBody)
        PR_FREEIF(bodyString);
    }
    else
        rv = NS_ERROR_FAILURE;
  }
  else
    rv = NS_ERROR_NOT_INITIALIZED;
  
  if (NS_FAILED(rv))
    NotifyStateListeners(eComposeProcessDone,rv);
  
  return rv;
}

nsresult nsMsgCompose::SendMsg(MSG_DeliverMode deliverMode,  nsIMsgIdentity *identity, nsIMsgProgress *progress)
{
  nsresult rv = NS_OK;
  PRBool entityConversionDone = PR_FALSE;
  nsCOMPtr<nsIPrompt> prompt;

  // i'm assuming the compose window is still up at this point...
  if (!prompt && m_window)
     m_window->GetPrompter(getter_AddRefs(prompt));

  if (m_editor && m_compFields && !m_composeHTML)
  {
    // Reset message body previously stored in the compose fields
    // There is 2 nsIMsgCompFields::SetBody() functions using a pointer as argument,
    // therefore a casting is required.
    m_compFields->SetBody((const char *)nsnull);

    // The plain text compose window was used
    const char contentType[] = "text/plain";
    nsAutoString msgBody;
    nsAutoString format; format.AssignWithConversion(contentType);
    PRUint32 flags = nsIDocumentEncoder::OutputFormatted;

    const char *charset = m_compFields->GetCharacterSet();
    if(UseFormatFlowed(charset))
        flags |= nsIDocumentEncoder::OutputFormatFlowed;
    
    rv = m_editor->OutputToString(format, flags, msgBody);
    
    if (NS_SUCCEEDED(rv) && !msgBody.IsEmpty())
    {
      // Convert body to mail charset not to utf-8 (because we don't manipulate body text)
      char *outCString = nsnull;
      nsXPIDLCString fallbackCharset;
      PRBool isAsciiOnly;
      rv = nsMsgI18NSaveAsCharset(contentType, m_compFields->GetCharacterSet(), 
                                  msgBody.get(), &outCString, 
                                  getter_Copies(fallbackCharset), &isAsciiOnly);
      SET_SIMULATED_ERROR(SIMULATED_SEND_ERROR_14, rv, NS_ERROR_UENC_NOMAPPING);
      if (NS_SUCCEEDED(rv) && nsnull != outCString) 
      {
        // body contains multilingual data, confirm send to the user
        if (NS_ERROR_UENC_NOMAPPING == rv) {
          PRBool proceedTheSend;
          rv = nsMsgAskBooleanQuestionByID(prompt, NS_ERROR_MSG_MULTILINGUAL_SEND, &proceedTheSend);
          if (!proceedTheSend) {
            PR_FREEIF(outCString);
            return NS_ERROR_MSG_MULTILINGUAL_SEND;
          }
        }
        // re-label to the fallback charset
        else if (fallbackCharset)
          m_compFields->SetCharacterSet(fallbackCharset.get());
        m_compFields->SetBodyIsAsciiOnly(isAsciiOnly);
        m_compFields->SetBody(outCString);
        entityConversionDone = PR_TRUE;
        PR_Free(outCString);
      }
      else
      {
        nsCAutoString msgbodyC;
        msgbodyC.AssignWithConversion(msgBody);
        m_compFields->SetBody(msgbodyC.get());
      }
    }
  }

  // Let's open the progress dialog
  if (progress)
  {
    mProgress = progress;
    nsXPIDLString msgSubject;
    m_compFields->GetSubject(getter_Copies(msgSubject));

    PRBool showProgress = PR_FALSE;
    nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
    if (prefs)
    {
      prefs->GetBoolPref("mailnews.show_send_progress", &showProgress);
      if (showProgress)
      {
        nsCOMPtr<nsIMsgComposeProgressParams> params = do_CreateInstance(NS_MSGCOMPOSEPROGRESSPARAMS_CONTRACTID, &rv);
        if (NS_FAILED(rv) || !params)
          return NS_ERROR_FAILURE;

        params->SetSubject((const PRUnichar*) msgSubject);
        params->SetDeliveryMode(deliverMode);
        
        mProgress->OpenProgressDialog(m_window, "chrome://messenger/content/messengercompose/sendProgress.xul", params);
        mProgress->GetPrompter(getter_AddRefs(prompt));
      }
    }

    mProgress->OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_START, NS_OK);
  }

  rv = _SendMsg(deliverMode, identity, entityConversionDone);
  if (NS_FAILED(rv))
  {
    nsCOMPtr<nsIMsgSendReport> sendReport;
    if (mMsgSend)
      mMsgSend->GetSendReport(getter_AddRefs(sendReport));
    if (sendReport)
    {
      nsresult theError;
      sendReport->DisplayReport(prompt, PR_TRUE, PR_TRUE, &theError);
    }
    else
    {
      /* If we come here it's because we got an error before we could intialize a
         send report! Let's try our best...
      */
      switch (deliverMode)
      {
        case nsIMsgCompDeliverMode::Later:
          nsMsgDisplayMessageByID(prompt, NS_MSG_UNABLE_TO_SEND_LATER);
          break;
        case nsIMsgCompDeliverMode::SaveAsDraft:
          nsMsgDisplayMessageByID(prompt, NS_MSG_UNABLE_TO_SAVE_DRAFT);
          break;
        case nsIMsgCompDeliverMode::SaveAsTemplate:
          nsMsgDisplayMessageByID(prompt, NS_MSG_UNABLE_TO_SAVE_TEMPLATE);
          break;

        default:
          nsMsgDisplayMessageByID(prompt, NS_ERROR_SEND_FAILED);
          break;
      }
    }

    if (progress)
      progress->CloseProgressDialog(PR_TRUE);
  }
  
  return rv;
}


// XXX when do we break this ref to the listener?
NS_IMETHODIMP nsMsgCompose::SetRecyclingListener(nsIMsgComposeRecyclingListener *aRecyclingListener)
{
  mRecyclingListener = aRecyclingListener;
  return NS_OK;
}
 
NS_IMETHODIMP nsMsgCompose::GetRecyclingListener(nsIMsgComposeRecyclingListener **aRecyclingListener)
{
  NS_ENSURE_ARG_POINTER(aRecyclingListener);
  *aRecyclingListener = mRecyclingListener;
  NS_IF_ADDREF(*aRecyclingListener);
  return NS_OK;
}
 
/* attribute boolean recycledWindow; */
NS_IMETHODIMP nsMsgCompose::GetRecycledWindow(PRBool *aRecycledWindow)
{
  NS_ENSURE_ARG_POINTER(aRecycledWindow);
  *aRecycledWindow = mRecycledWindow;
  return NS_OK;
}
NS_IMETHODIMP nsMsgCompose::SetRecycledWindow(PRBool aRecycledWindow)
{
  mRecycledWindow = aRecycledWindow;
  return NS_OK;
}

#if !defined(XP_MAC)
PRBool nsMsgCompose::IsLastWindow()
{
  nsresult rv;
  PRBool more;
  nsCOMPtr<nsIWindowMediator> windowMediator =
              do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
    rv = windowMediator->GetEnumerator(nsnull,
               getter_AddRefs(windowEnumerator));
    if (NS_SUCCEEDED(rv))
    {
      nsCOMPtr<nsISupports> isupports;

      if (NS_SUCCEEDED(windowEnumerator->GetNext(getter_AddRefs(isupports))))
        if (NS_SUCCEEDED(windowEnumerator->HasMoreElements(&more)))
          return !more;
    }
  }
  return PR_TRUE;
}
#endif /* XP_MAC */

NS_IMETHODIMP nsMsgCompose::CloseWindow(PRBool recycleIt)
{
  nsresult rv;

  nsCOMPtr<nsIMsgComposeService> composeService = do_GetService(NS_MSGCOMPOSESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
#if !defined(XP_MAC)
  recycleIt = recycleIt && !IsLastWindow();
#endif /* XP_MAC */
  if (recycleIt)
  {
    rv = composeService->CacheWindow(m_window, m_composeHTML, mRecyclingListener);
    if (NS_SUCCEEDED(rv))
    {
      NS_ASSERTION(m_editor, "no editor");
      if (m_editor)
      {
        m_editor->RemoveDocumentStateListener(mDocumentListener);

        // XXX clear undo txn manager?

        rv = m_editor->EnableUndo(PR_FALSE);
        NS_ENSURE_SUCCESS(rv,rv);

        rv = m_editor->BeginTransaction();
        NS_ENSURE_SUCCESS(rv,rv);

        rv = m_editor->SelectAll();
        NS_ENSURE_SUCCESS(rv,rv);

        rv = m_editor->DeleteSelection(0);
        NS_ENSURE_SUCCESS(rv,rv);

        rv = m_editor->EndTransaction();
        NS_ENSURE_SUCCESS(rv,rv);

        rv = m_editor->EnableUndo(PR_TRUE);
        NS_ENSURE_SUCCESS(rv,rv);

        SetBodyModified(PR_FALSE);
      }
      if (mRecyclingListener)
      {
        mRecyclingListener->OnClose();
        
        /**
         * In order to really free the memory, we need to call the JS garbage collector for our window.
         * If we don't call GC, the nsIMsgCompose object hold by JS will not be released despite we set
         * the JS global that hold it to null. Each time we reopen a recycled window, we allocate a new
         * nsIMsgCompose that we really need to be releazed when we recycle the window. In fact despite
         * we call GC here atfer the release wont occurs right away. But if we don't call it, the release
         * will apppend only when we phisically close the window which will append only on quit.
         */
        nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(m_window));
        if (sgo)
        {
          nsCOMPtr<nsIScriptContext> scriptContext;
          sgo->GetContext(getter_AddRefs(scriptContext));
          if (scriptContext)
            scriptContext->GC();
        }
      }
      return NS_OK;
    }
  }
  
  //We are going away for real, we need to do some clean up first
  if (m_baseWindow)
  {
    if (m_editor)
    {
      m_editor->RemoveDocumentStateListener(mDocumentListener);
      m_editorShell = nsnull;
      m_editor = nsnull;
        /* The editor will be destroyed during yje close window.
         * Set it to null to be sure we wont uses it anymore
         */
    }
    nsIBaseWindow * aWindow = m_baseWindow;
    m_baseWindow = nsnull;
    rv = aWindow->Destroy();
  }
  
  return rv;
}

nsresult nsMsgCompose::Abort()
{
  if (mMsgSend)
    mMsgSend->Abort();

  if (mProgress)
    mProgress->CloseProgressDialog(PR_TRUE);

  return NS_OK;
}

nsresult nsMsgCompose::GetEditorShell(nsIEditorShell * *aEditorShell)
{ 
  NS_IF_ADDREF(*aEditorShell = m_editorShell);
  return NS_OK;
} 

nsresult nsMsgCompose::GetEditor(nsIEditor * *aEditor)
{ 
  NS_IF_ADDREF(*aEditor = m_editor);
  return NS_OK;
} 

nsresult nsMsgCompose::ClearEditor()
{
  m_editor = nsnull;
  m_editorShell = nsnull;
  return NS_OK;
}

nsresult nsMsgCompose::SetEditorShell(nsIEditorShell * aEditorShell)
{
    // First, store the editor shell but do not addref it (see sfraser@netscape.com for explanation).
    m_editorShell = aEditorShell;
    m_editor = nsnull;

    if (nsnull == m_editorShell)
      return NS_OK;
    
    //
    // Now this routine will create a listener for state changes
    // in the editor and set us as the compose object of interest.
    //
    mDocumentListener = new nsMsgDocumentStateListener();
    if (!mDocumentListener)
        return NS_ERROR_OUT_OF_MEMORY;

    mDocumentListener->SetComposeObj(this);      
    NS_ADDREF(mDocumentListener);

    // Make sure we setup to listen for editor state changes...
    m_editorShell->RegisterDocumentStateListener(mDocumentListener);

    // Set the charset
    nsAutoString msgCharSet;
    msgCharSet.AssignWithConversion(m_compFields->GetCharacterSet());
    m_editorShell->SetDocumentCharacterSet(msgCharSet.get());

    if (mRecycledWindow)
    {
      // Editor document is already created therefore, we need to call the document
      // listener ourself in order to make quoting works
      mDocumentListener->NotifyDocumentCreated();
    }
    else
    {
      // Now, lets init the editor here!
      // Just get a blank editor started...
      m_editorShell->LoadUrl(NS_LITERAL_STRING("about:blank").get());
    }
    return NS_OK;
} 

nsresult nsMsgCompose::GetBodyModified(PRBool * modified)
{
  nsresult rv;

  if (! modified)
    return NS_ERROR_NULL_POINTER;

  *modified = PR_TRUE;
      
  if (m_editor)
  {
    rv = m_editor->GetDocumentModified(modified);
    if (NS_FAILED(rv))
      *modified = PR_TRUE;
  }

  return NS_OK;   
}

nsresult nsMsgCompose::SetBodyModified(PRBool modified)
{
  nsresult  rv = NS_OK;

  if (m_editor)
  {
    if (modified)
    {
      PRInt32  modCount = 0;
      m_editor->GetModificationCount(&modCount);
      if (modCount == 0)
        m_editor->IncrementModificationCount(1);
    }
    else
      m_editor->ResetModificationCount();
  }

  return rv;  
}

nsresult nsMsgCompose::GetDomWindow(nsIDOMWindowInternal * *aDomWindow)
{
  *aDomWindow = m_window;
  NS_IF_ADDREF(*aDomWindow);
  return NS_OK;
}


nsresult nsMsgCompose::GetCompFields(nsIMsgCompFields * *aCompFields)
{
  *aCompFields = (nsIMsgCompFields*)m_compFields;
  NS_IF_ADDREF(*aCompFields);
  return NS_OK;
}


NS_IMETHODIMP nsMsgCompose::GetComposeHTML(PRBool *aComposeHTML)
{
  *aComposeHTML = m_composeHTML;
  return NS_OK;
}


nsresult nsMsgCompose::GetWrapLength(PRInt32 *aWrapLength)
{
  nsresult rv;
  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  
  return prefs->GetIntPref("mailnews.wraplength", aWrapLength);
}

nsresult nsMsgCompose::CreateMessage(const char * originalMsgURI,
                                     MSG_ComposeType type,
                                     nsIMsgCompFields * compFields)
{
  nsresult rv = NS_OK;

  mType = type;

  if (compFields)
  {
    NS_IF_RELEASE(m_compFields);
    m_compFields = NS_REINTERPRET_CAST(nsMsgCompFields*, compFields);
    NS_ADDREF(m_compFields);
  }
  else
  {
    NS_NEWXPCOM(m_compFields, nsMsgCompFields);
    if (m_compFields)
      NS_ADDREF(m_compFields);
    else
      return NS_ERROR_OUT_OF_MEMORY;
  }
  
  if (m_identity)
  {
      /* Setup reply-to field */
      nsXPIDLCString replyTo;
      m_identity->GetReplyTo(getter_Copies(replyTo));
      if (replyTo && *(const char *)replyTo)
      {
        nsXPIDLCString replyToStr;
        replyToStr.Assign(m_compFields->GetReplyTo());

        if (replyToStr.Length() > 0)
          replyToStr.Append(',');
        replyToStr.Append(replyTo);
        m_compFields->SetReplyTo(replyToStr.get());
      }

      /* Setup bcc field */
      PRBool  bccSelf, bccOthers;
      m_identity->GetBccSelf(&bccSelf);
      m_identity->GetBccOthers(&bccOthers);
      if (bccSelf || bccOthers) 
      {
        nsXPIDLCString bccStr;
        bccStr.Assign(m_compFields->GetBcc());

        if (bccSelf)
        {
          nsXPIDLCString email;
          m_identity->GetEmail(getter_Copies(email));
          if (bccStr.Length() > 0)
            bccStr.Append(',');
          bccStr.Append(email);
        }

        if (bccOthers)
        {
          nsXPIDLCString bccList;
          m_identity->GetBccList(getter_Copies(bccList));
          if (bccStr.Length() > 0)
            bccStr.Append(',');
          bccStr.Append(bccList);
        }
        m_compFields->SetBcc(bccStr.get());
      }
  }

  // If we don't have an original message URI, nothing else to do...
  if (!originalMsgURI || *originalMsgURI == 0)
    return rv;

  // store the original message URI so we can extract it after we send the message to properly
  // mark any disposition flags like replied or forwarded on the message.
  mOriginalMsgURI = originalMsgURI;

  // If we are forwarding inline, mime did already setup the compose fields therefore we should stop now
  if (type == nsIMsgCompType::ForwardInline )
    return rv;
  
  char *uriList = PL_strdup(originalMsgURI);
  if (!uriList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIMimeConverter> mimeConverter = do_GetService(NS_MIME_CONVERTER_CONTRACTID);

  nsXPIDLCString charset;
  // use a charset of the original message
  nsXPIDLString mailCharset;
  PRBool charsetOverride = PR_FALSE;
  GetTopmostMsgWindowCharacterSet(mailCharset, &mCharsetOverride);
  if (!mailCharset.IsEmpty())
  {
    charset.Adopt(ToNewUTF8String(nsDependentString(mailCharset)));
    charsetOverride = PR_TRUE;
  }

  PRBool isFirstPass = PR_TRUE;
  char *uri = uriList;
  char *nextUri;
  do 
  {
    nextUri = strstr(uri, "://");
    if (nextUri)
    {
      // look for next ://, and then back up to previous ','
      nextUri = strstr(nextUri + 1, "://");
      if (nextUri)
      {
        *nextUri = '\0';
        char *saveNextUri = nextUri;
        nextUri = strrchr(uri, ',');
        if (nextUri)
          *nextUri = '\0';
        *saveNextUri = ':';
      }
    }
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = GetMsgDBHdrFromURI(uri, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv,rv);

    if (msgHdr)
    {
      nsXPIDLCString subject;
      nsCString subjectStr;
      nsXPIDLString decodedString;
      nsXPIDLCString decodedCString;

      if (!charsetOverride)
      {
        rv = msgHdr->GetCharset(getter_Copies(charset));
        if (NS_FAILED(rv)) return rv;
      }

      // get an original charset, used for a label, UTF-8 is used for the internal processing
      if (isFirstPass && !charset.IsEmpty())
        m_compFields->SetCharacterSet(charset);

      rv = msgHdr->GetSubject(getter_Copies(subject));
      if (NS_FAILED(rv)) return rv;

      subjectStr.SetLength(0);

      switch (type)
      {
        default: break;
        case nsIMsgCompType::Reply :
        case nsIMsgCompType::ReplyAll:
        case nsIMsgCompType::ReplyToGroup:
        case nsIMsgCompType::ReplyToSender:
        case nsIMsgCompType::ReplyToSenderAndGroup:
          {
            if (!isFirstPass)       // safeguard, just in case...
            {
              PR_Free(uriList);
              return rv;
            }
            mQuotingToFollow = PR_TRUE;

            subjectStr.Append("Re: ");
            subjectStr.Append(subject);
            rv = mimeConverter->DecodeMimeHeader(subjectStr.get(),
                getter_Copies(decodedString),
                charset, charsetOverride);
            if (NS_SUCCEEDED(rv))
              m_compFields->SetSubject(decodedString);
            else
              m_compFields->SetSubject(subjectStr.get());

            nsXPIDLCString author;
            rv = msgHdr->GetAuthor(getter_Copies(author));
            if (NS_FAILED(rv))
              return rv;

            rv = mimeConverter->DecodeMimeHeader(author,
                getter_Copies(decodedCString),
                charset, charsetOverride);
            if (NS_SUCCEEDED(rv) && decodedCString)
              m_compFields->SetTo(decodedCString);
            else
              m_compFields->SetTo(author);

            // Setup quoting callbacks for later...
            mWhatHolder = 1;
            mQuoteURI = originalMsgURI;

            break;
          }
        case nsIMsgCompType::ForwardAsAttachment:
          {
            nsAutoString decodedSubject;
            PRUint32 flags;

            msgHdr->GetFlags(&flags);
            if (flags & MSG_FLAG_HAS_RE)
              subjectStr.Append("Re: ");
            subjectStr.Append(subject);

            rv = mimeConverter->DecodeMimeHeader(subjectStr.get(), 
                getter_Copies(decodedString),
                charset, charsetOverride);
            if (NS_SUCCEEDED(rv))
              decodedSubject.Assign(decodedString);
            else
              decodedSubject.AssignWithConversion(subjectStr.get());

            // Setup quoting callbacks for later...
            mQuotingToFollow = PR_FALSE;  //We don't need to quote the original message.
            nsCOMPtr<nsIMsgAttachment> attachment = do_CreateInstance(NS_MSGATTACHMENT_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv) && attachment)
            {
              attachment->SetName(decodedSubject.get());
              attachment->SetUrl(uri);
              m_compFields->AddAttachment(attachment);
            }

            if (isFirstPass)
            {
              decodedSubject.Insert(NS_LITERAL_STRING("[Fwd: ").get(), 0);
              decodedSubject.Append(NS_LITERAL_STRING("]").get());
              m_compFields->SetSubject(decodedSubject.get()); 
            }
            break;
          }
      }
    }
    isFirstPass = PR_FALSE;
    uri = nextUri + 1;
  }
  while (nextUri);
  PR_Free(uriList);
  return rv;
}

NS_IMETHODIMP nsMsgCompose::GetProgress(nsIMsgProgress **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mProgress;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::GetMessageSend(nsIMsgSend **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mMsgSend;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::GetExternalSendListener(nsIMsgSendListener **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mExternalSendListener;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::SetCiteReference(nsString citeReference)
{
  mCiteReference = citeReference;
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::SetSavedFolderURI(const char *folderURI)
{
  m_folderName = folderURI;
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::GetSavedFolderURI(char ** folderURI)
{
  NS_ENSURE_ARG_POINTER(folderURI);
  *folderURI = ToNewCString(m_folderName);
  return (*folderURI) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsMsgCompose::GetOriginalMsgURI(char ** originalMsgURI)
{
  NS_ENSURE_ARG_POINTER(originalMsgURI);
  *originalMsgURI = ToNewCString(mOriginalMsgURI);
  return (*originalMsgURI) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


////////////////////////////////////////////////////////////////////////////////////
// THIS IS THE CLASS THAT IS THE STREAM CONSUMER OF THE HTML OUPUT
// FROM LIBMIME. THIS IS FOR QUOTING
////////////////////////////////////////////////////////////////////////////////////
QuotingOutputStreamListener::~QuotingOutputStreamListener() 
{
  if (mUnicodeConversionBuffer)
    nsMemory::Free(mUnicodeConversionBuffer);
}

QuotingOutputStreamListener::QuotingOutputStreamListener(const char * originalMsgURI,
                                                         PRBool quoteHeaders,
                                                         PRBool headersOnly,
                                                         nsIMsgIdentity *identity,
                                                         const char *charset,
                                                         PRBool charetOverride, 
                                                         PRBool quoteOriginal)
{ 
  nsresult rv;
  mQuoteHeaders = quoteHeaders;
  mHeadersOnly = headersOnly;
  mIdentity = identity;
  mUnicodeBufferCharacterLength = 0;
  mUnicodeConversionBuffer = nsnull;
  mQuoteOriginal = quoteOriginal;

  if (! mHeadersOnly)
  {
    nsXPIDLString replyHeaderOriginalmessage;
    // For the built message body...
    nsCOMPtr <nsIMsgDBHdr> originalMsgHdr;
    rv = GetMsgDBHdrFromURI(originalMsgURI, getter_AddRefs(originalMsgHdr));
    if (NS_SUCCEEDED(rv) && originalMsgHdr && !quoteHeaders)
    {
      // Setup the cite information....
      nsXPIDLCString myGetter;
      if (NS_SUCCEEDED(originalMsgHdr->GetMessageId(getter_Copies(myGetter))))
      {
        if (!myGetter.IsEmpty())
        {
          nsCAutoString buf;
          mCiteReference = NS_LITERAL_STRING("mid")
             + NS_ConvertASCIItoUCS2(NS_EscapeURL(myGetter, esc_FileBaseName | esc_Forced, buf));
        }
      }

      if (GetReplyOnTop() == 1) 
        mCitePrefix += NS_LITERAL_STRING("<br><br>");

      
      PRBool header, headerDate;
      PRInt32 replyHeaderType;
      nsXPIDLString replyHeaderLocale;
      nsXPIDLString replyHeaderAuthorwrote;
      nsXPIDLString replyHeaderOndate;
      nsXPIDLString replyHeaderSeparator;
      nsXPIDLString replyHeaderColon;

      // Get header type, locale and strings from pref.
      rv = GetReplyHeaderInfo(&replyHeaderType, 
                              getter_Copies(replyHeaderLocale),
                              getter_Copies(replyHeaderAuthorwrote),
                              getter_Copies(replyHeaderOndate),
                              getter_Copies(replyHeaderSeparator),
                              getter_Copies(replyHeaderColon),
                              getter_Copies(replyHeaderOriginalmessage));

      switch (replyHeaderType)
      {
        case 0: // No reply header at all
          header=PR_FALSE;
          headerDate=PR_FALSE;
          break;

        case 2: // Insert both the original author and date in the reply header (date followed by author)
        case 3: // Insert both the original author and date in the reply header (author followed by date)
          header=PR_TRUE;
          headerDate=PR_TRUE;
          break;

        case 4: // XXX implement user specified header
        case 1: // Default is to only view the author. We will reconsider this decision when bug 75377 is fixed.
        default:
          header=PR_TRUE;
          headerDate=PR_FALSE;
          break;
      }

      nsAutoString citePrefixDate;
      nsAutoString citePrefixAuthor;

      if (header)
      {
        if (headerDate)
        {
          nsCOMPtr<nsIDateTimeFormat> dateFormatter = do_CreateInstance(kDateTimeFormatCID, &rv);

          if (NS_SUCCEEDED(rv)) 
          {  
            PRTime originalMsgDate;
            rv = originalMsgHdr->GetDate(&originalMsgDate); 
                
            if (NS_SUCCEEDED(rv)) 
            {
              nsAutoString formattedDateString;
              nsCOMPtr<nsILocale> locale;
              nsCOMPtr<nsILocaleService> localeService(do_GetService(NS_LOCALESERVICE_CONTRACTID));

              // Format date using "mailnews.reply_header_locale", if empty then use application default locale.
              if (replyHeaderLocale && *replyHeaderLocale)
                rv = localeService->NewLocale(replyHeaderLocale.get(), getter_AddRefs(locale));
              else
                rv = localeService->GetApplicationLocale(getter_AddRefs(locale));
              
              if (NS_SUCCEEDED(rv))
              {
                rv = dateFormatter->FormatPRTime(locale,
                                                 kDateFormatShort,
                                                 kTimeFormatNoSeconds,
                                                 originalMsgDate,
                                                 formattedDateString);

                if (NS_SUCCEEDED(rv)) 
                {
                  // take care "On %s"
                  PRUnichar *formatedString = nsnull;
                  formatedString = nsTextFormatter::smprintf(replyHeaderOndate.get(), 
                                                             NS_ConvertUCS2toUTF8(formattedDateString.get()).get());
                  if (formatedString) 
                  {
                    citePrefixDate.Assign(formatedString);
                    nsTextFormatter::smprintf_free(formatedString);
                  }
                }
              }
            }
          }
        }


      nsXPIDLCString author;
      rv = originalMsgHdr->GetAuthor(getter_Copies(author));

      if (NS_SUCCEEDED(rv))
      {
        nsXPIDLCString decodedString;
        mMimeConverter = do_GetService(NS_MIME_CONVERTER_CONTRACTID);
        // Decode header, the result string is null if the input is non MIME encoded ASCII.
        if (mMimeConverter)
          mMimeConverter->DecodeMimeHeader(author.get(), getter_Copies(decodedString), charset, charetOverride);

        nsCOMPtr<nsIMsgHeaderParser> parser (do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID));

        if (parser)
        {
          nsXPIDLCString authorName;
          rv = parser->ExtractHeaderAddressName("UTF-8", decodedString ? decodedString.get() : author.get(),
                                                getter_Copies(authorName));
          // take care "%s wrote"
          PRUnichar *formatedString = nsnull;
          if (NS_SUCCEEDED(rv) && authorName)
            formatedString = nsTextFormatter::smprintf(replyHeaderAuthorwrote.get(), authorName.get());
          else
            formatedString = nsTextFormatter::smprintf(replyHeaderAuthorwrote.get(), author.get());
          if (formatedString) 
          {
            citePrefixAuthor.Assign(formatedString);
            nsTextFormatter::smprintf_free(formatedString);
          }
        }


        }
        if (replyHeaderType == 2)
        {
          mCitePrefix.Append(citePrefixDate);
          mCitePrefix.Append(replyHeaderSeparator);
          mCitePrefix.Append(citePrefixAuthor);
        }
        else if (replyHeaderType == 3) 
        {
          mCitePrefix.Append(citePrefixAuthor);
          mCitePrefix.Append(replyHeaderSeparator);
          mCitePrefix.Append(citePrefixDate);
        }
        else
          mCitePrefix.Append(citePrefixAuthor);
        mCitePrefix.Append(replyHeaderColon);
        mCitePrefix.Append(NS_LITERAL_STRING("<br><html>"));
      }
    }

    if (mCitePrefix.IsEmpty())
    {
      if (!replyHeaderOriginalmessage)
      {
        // This is not likely to happen but load the string if it's not done already.
        PRInt32 replyHeaderType;
        nsXPIDLString replyHeaderLocale;
        nsXPIDLString replyHeaderAuthorwrote;
        nsXPIDLString replyHeaderOndate;
        nsXPIDLString replyHeaderSeparator;
        nsXPIDLString replyHeaderColon;
        rv = GetReplyHeaderInfo(&replyHeaderType, 
                                getter_Copies(replyHeaderLocale),
                                getter_Copies(replyHeaderAuthorwrote),
                                getter_Copies(replyHeaderOndate),
                                getter_Copies(replyHeaderSeparator),
                                getter_Copies(replyHeaderColon),
                                getter_Copies(replyHeaderOriginalmessage));
      }
      mCitePrefix.Append(NS_LITERAL_STRING("<br><br>"));
      mCitePrefix.Append(replyHeaderOriginalmessage);
      mCitePrefix.Append(NS_LITERAL_STRING("<br><html>"));
    }
  }
  
  NS_INIT_ISUPPORTS(); 
}

/**
 * The formatflowed parameter directs if formatflowed should be used in the conversion.
 * format=flowed (RFC 2646) is a way to represent flow in a plain text mail, without
 * disturbing the plain text.
 */
nsresult
QuotingOutputStreamListener::ConvertToPlainText(PRBool formatflowed /* = PR_FALSE */)
{
  nsresult  rv = NS_OK;

  rv += ConvertBufToPlainText(mCitePrefix, formatflowed);
  rv += ConvertBufToPlainText(mMsgBody, formatflowed);
  rv += ConvertBufToPlainText(mSignature, formatflowed);
  return rv;
}

NS_IMETHODIMP QuotingOutputStreamListener::OnStartRequest(nsIRequest *request, nsISupports * /* ctxt */)
{
  return NS_OK;
}

NS_IMETHODIMP QuotingOutputStreamListener::OnStopRequest(nsIRequest *request, nsISupports * /* ctxt */, nsresult status)
{
  nsresult rv = NS_OK;
  nsAutoString aCharset;
  
  nsCOMPtr<nsIMsgCompose> compose = do_QueryReferent(mWeakComposeObj);
  if (compose) 
  {
    MSG_ComposeType type;
    compose->GetType(&type);
    
    // Assign cite information if available...
    if (!mCiteReference.IsEmpty())
      compose->SetCiteReference(mCiteReference);

    if (mHeaders && (type == nsIMsgCompType::Reply || type == nsIMsgCompType::ReplyAll || type == nsIMsgCompType::ReplyToSender ||
                     type == nsIMsgCompType::ReplyToGroup || type == nsIMsgCompType::ReplyToSenderAndGroup) && mQuoteOriginal)
    {
      nsCOMPtr<nsIMsgCompFields> compFields;
      compose->GetCompFields(getter_AddRefs(compFields));
      if (compFields)
      {
        aCharset.AssignWithConversion(msgCompHeaderInternalCharset());
        nsAutoString recipient;
        nsAutoString cc;
        nsAutoString replyTo;
        nsAutoString newgroups;
        nsAutoString followUpTo;
        nsAutoString messageId;
        nsAutoString references;
        nsXPIDLCString outCString;
        PRUnichar emptyUnichar = 0;
        PRBool needToRemoveDup = PR_FALSE;
        if (!mMimeConverter)
        {
          mMimeConverter = do_GetService(NS_MIME_CONVERTER_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        nsXPIDLCString charset;
        compFields->GetCharacterSet(getter_Copies(charset));
        
        if (type == nsIMsgCompType::ReplyAll)
        {
          mHeaders->ExtractHeader(HEADER_TO, PR_TRUE, getter_Copies(outCString));
          if (outCString)
          {
            mMimeConverter->DecodeMimeHeader(outCString, recipient, charset);
          }
              
          mHeaders->ExtractHeader(HEADER_CC, PR_TRUE, getter_Copies(outCString));
          if (outCString)
          {
            mMimeConverter->DecodeMimeHeader(outCString, cc, charset);
          }
              
          if (recipient.Length() > 0 && cc.Length() > 0)
            recipient.Append(NS_LITERAL_STRING(", "));
          recipient += cc;
          compFields->SetCc(recipient.get());

          needToRemoveDup = PR_TRUE;
        }
              
        mHeaders->ExtractHeader(HEADER_REPLY_TO, PR_FALSE, getter_Copies(outCString));
        if (outCString)
        {
          mMimeConverter->DecodeMimeHeader(outCString, replyTo, charset);
        }
        
        mHeaders->ExtractHeader(HEADER_NEWSGROUPS, PR_FALSE, getter_Copies(outCString));
        if (outCString)
        {
          mMimeConverter->DecodeMimeHeader(outCString, newgroups, charset);
        }
        
        mHeaders->ExtractHeader(HEADER_FOLLOWUP_TO, PR_FALSE, getter_Copies(outCString));
        if (outCString)
        {
          mMimeConverter->DecodeMimeHeader(outCString, followUpTo, charset);
        }
        
        mHeaders->ExtractHeader(HEADER_MESSAGE_ID, PR_FALSE, getter_Copies(outCString));
        if (outCString)
        {
          mMimeConverter->DecodeMimeHeader(outCString, messageId, charset);
        }
        
        mHeaders->ExtractHeader(HEADER_REFERENCES, PR_FALSE, getter_Copies(outCString));
        if (outCString)
        {
          mMimeConverter->DecodeMimeHeader(outCString, references, charset);
        }
        
        if (! replyTo.IsEmpty())
        {
          compFields->SetTo(replyTo.get());
          needToRemoveDup = PR_TRUE;
        }
        
        if (! newgroups.IsEmpty())
        {
          if ((type != nsIMsgCompType::Reply) && (type != nsIMsgCompType::ReplyToSender))
            compFields->SetNewsgroups(NS_LossyConvertUCS2toASCII(newgroups).get());
          if (type == nsIMsgCompType::ReplyToGroup)
            compFields->SetTo(&emptyUnichar);
        }
        
        if (! followUpTo.IsEmpty())
        {
          // Handle "followup-to: poster" magic keyword here
          if (followUpTo == NS_LITERAL_STRING("poster"))
          {
            nsCOMPtr<nsIDOMWindowInternal> composeWindow;
            nsCOMPtr<nsIPrompt> prompt;
            compose->GetDomWindow(getter_AddRefs(composeWindow));
            if (composeWindow)
              composeWindow->GetPrompter(getter_AddRefs(prompt));
            nsMsgDisplayMessageByID(prompt, NS_MSG_FOLLOWUPTO_ALERT);
            
            // If reply-to is empty, use the from header to fetch
            // the original sender's email
            if (!replyTo.IsEmpty())
              compFields->SetTo(replyTo.get());
            else
            {
              mHeaders->ExtractHeader(HEADER_FROM, PR_FALSE, getter_Copies(outCString));
              if (outCString)
              {
                nsAutoString from;
                mMimeConverter->DecodeMimeHeader(outCString, from, charset);
                compFields->SetTo(from.get());
              }
            }

            // Clear the newsgroup: header field, because followup-to: poster
            // only follows up to the original sender
            if (! newgroups.IsEmpty())
              compFields->SetNewsgroups(nsnull);
          }
          else // Process "followup-to: newsgroup-content" here
          {
            if (type != nsIMsgCompType::ReplyToSender)
              compFields->SetNewsgroups(NS_LossyConvertUCS2toASCII(followUpTo).get());
            if (type == nsIMsgCompType::Reply)
              compFields->SetTo(&emptyUnichar);
          }
        }
        
        if (! references.IsEmpty())
          references.Append(PRUnichar(' '));
        references += messageId;
        compFields->SetReferences(NS_LossyConvertUCS2toASCII(references).get());
        
        if (needToRemoveDup)
        {
          //Remove duplicate addresses between TO && CC
          char * resultStr;
          nsMsgCompFields* _compFields = (nsMsgCompFields*)compFields.get();  // XXX what is this?
          if (NS_SUCCEEDED(rv))
          {
            nsCString addressToBeRemoved(_compFields->GetTo());
            if (mIdentity)
            {
              nsXPIDLCString email;
              mIdentity->GetEmail(getter_Copies(email));
              addressToBeRemoved += ", ";
              addressToBeRemoved += NS_CONST_CAST(char*, (const char *)email);
            }

            rv= RemoveDuplicateAddresses(_compFields->GetCc(), addressToBeRemoved.get(), PR_TRUE, &resultStr);
            if (NS_SUCCEEDED(rv))
            {
              _compFields->SetCc(resultStr);
              PR_Free(resultStr);
            }
          }
        }    
      }
    }
    
#ifdef MSGCOMP_TRACE_PERFORMANCE
    nsCOMPtr<nsIMsgComposeService> composeService (do_GetService(NS_MSGCOMPOSESERVICE_CONTRACTID));
    composeService->TimeStamp("Done with MIME. Now we're updating the UI elements", PR_FALSE);
#endif

    if (mQuoteOriginal)
      compose->NotifyStateListeners(eComposeFieldsReady, NS_OK);

#ifdef MSGCOMP_TRACE_PERFORMANCE
    composeService->TimeStamp("Addressing widget, window title and focus are now set, time to insert the body", PR_FALSE);
#endif

    if (! mHeadersOnly)
      mMsgBody.Append(NS_LITERAL_STRING("</html>"));
    
    // Now we have an HTML representation of the quoted message.
    // If we are in plain text mode, we need to convert this to plain
    // text before we try to insert it into the editor. If we don't, we
    // just get lots of HTML text in the message...not good.
    //
    // XXX not m_composeHTML? /BenB
    PRBool composeHTML = PR_TRUE;
    compose->GetComposeHTML(&composeHTML);
    if (!composeHTML)
    {
      // Downsampling. The charset should only consist of ascii.
      char *target_charset = ToNewCString(aCharset);
      PRBool formatflowed = UseFormatFlowed(target_charset);
      ConvertToPlainText(formatflowed);
      Recycle(target_charset);
    }

    compose->ProcessSignature(mIdentity, &mSignature);

    nsCOMPtr<nsIEditor> editor;
    if (NS_SUCCEEDED(compose->GetEditor(getter_AddRefs(editor))) && editor)
    {
      if (mQuoteOriginal)
        compose->ConvertAndLoadComposeWindow(mCitePrefix,
                                             mMsgBody, mSignature,
                                             PR_TRUE, composeHTML);
      else
        InsertToCompose(editor, composeHTML);
    }
  }
  return rv;
}

NS_IMETHODIMP QuotingOutputStreamListener::OnDataAvailable(nsIRequest *request,
                              nsISupports *ctxt, nsIInputStream *inStr, 
                              PRUint32 sourceOffset, PRUint32 count)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG(inStr);

  if (mHeadersOnly)
    return rv;

  char *newBuf = (char *)PR_Malloc(count + 1);
  if (!newBuf)
    return NS_ERROR_FAILURE;

  PRUint32 numWritten = 0; 
  rv = inStr->Read(newBuf, count, &numWritten);
  if (rv == NS_BASE_STREAM_WOULD_BLOCK)
    rv = NS_OK;
  newBuf[numWritten] = '\0';
  if (NS_SUCCEEDED(rv) && numWritten > 0)
  {
    // Create unicode decoder.
    if (!mUnicodeDecoder)
    {
      nsCOMPtr<nsICharsetConverterManager2> ccm2 = 
               do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv); 
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr <nsIAtom> charsetAtom = getter_AddRefs(NS_NewAtom("UTF-8"));

        if (!charsetAtom)
        {
          PR_Free(newBuf);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        rv = ccm2->GetUnicodeDecoder(charsetAtom, getter_AddRefs(mUnicodeDecoder));
      }
    }

    if (NS_SUCCEEDED(rv))
    {
      PRInt32 unicharLength;
      PRInt32 inputLength = (PRInt32) numWritten;
      rv = mUnicodeDecoder->GetMaxLength(newBuf, numWritten, &unicharLength);
      if (NS_SUCCEEDED(rv))
      {
        // Use this local buffer if possible.
        const PRUint32 kLocalBufSize = 4096;
        PRUnichar localBuf[kLocalBufSize];
        PRUnichar *unichars = localBuf;
        
        if (unicharLength > kLocalBufSize)
        {
          // Otherwise, use the buffer of the class.
          if (!mUnicodeConversionBuffer ||
              unicharLength > mUnicodeBufferCharacterLength)
          {
            if (mUnicodeConversionBuffer)
              nsMemory::Free(mUnicodeConversionBuffer);
            mUnicodeConversionBuffer = (PRUnichar *) nsMemory::Alloc(unicharLength * sizeof(PRUnichar));
            if (!mUnicodeConversionBuffer)
            {
              mUnicodeBufferCharacterLength = 0;
              PR_Free(newBuf);
              return NS_ERROR_OUT_OF_MEMORY;
            }
            mUnicodeBufferCharacterLength = unicharLength;
          }
          unichars = mUnicodeConversionBuffer;
        }

        PRInt32 consumedInputLength = 0;
        PRInt32 originalInputLength = inputLength;
        char *inputBuffer = newBuf;
        PRInt32 convertedOutputLength = 0;
        PRInt32 outputBufferLength = unicharLength;
        PRUnichar *originalOutputBuffer = unichars;
        do 
        {
          rv = mUnicodeDecoder->Convert(inputBuffer, &inputLength, unichars, &unicharLength);
          if (NS_SUCCEEDED(rv)) 
          {
            convertedOutputLength += unicharLength;
            break;
          }

          // if we failed, we consume one byte, replace it with a question mark
          // and try the conversion again.
          unichars += unicharLength;
          *unichars = (PRUnichar)'?';
          unichars++;
          unicharLength++;

          mUnicodeDecoder->Reset();

          inputBuffer += ++inputLength;
          consumedInputLength += inputLength;
          inputLength = originalInputLength - consumedInputLength;  // update input length to convert
          convertedOutputLength += unicharLength;
          unicharLength = outputBufferLength - unicharLength;       // update output length

        } while (NS_FAILED(rv) &&
                 (originalInputLength > consumedInputLength) && 
                 (outputBufferLength > convertedOutputLength));

        if (convertedOutputLength > 0)
          mMsgBody.Append(originalOutputBuffer, convertedOutputLength);
      }
    }
  }

  PR_FREEIF(newBuf);
  return rv;
}

nsresult
QuotingOutputStreamListener::SetComposeObj(nsIMsgCompose *obj)
{
  mWeakComposeObj = getter_AddRefs(NS_GetWeakReference(obj));
  return NS_OK;
}

nsresult
QuotingOutputStreamListener::SetMimeHeaders(nsIMimeHeaders * headers)
{
  mHeaders = headers;
  return NS_OK;
}

NS_IMETHODIMP
QuotingOutputStreamListener::InsertToCompose(nsIEditor *aEditor,
                                             PRBool aHTMLEditor)
{
  // First, get the nsIEditor interface for future use
  nsCOMPtr<nsIDOMNode> nodeInserted;

  TranslateLineEnding(mMsgBody);

  // Now, insert it into the editor...
  if (aEditor)
    aEditor->EnableUndo(PR_TRUE);

  if (!mMsgBody.IsEmpty())
  {
    if (!mCitePrefix.IsEmpty())
    {
      nsCOMPtr<nsIHTMLEditor> htmlEditor (do_QueryInterface(aEditor));
      htmlEditor->InsertHTML(mCitePrefix);
    }

    nsCOMPtr<nsIEditorMailSupport> mailEditor (do_QueryInterface(aEditor));
    if (mailEditor)
    {
      if (aHTMLEditor)
        mailEditor->InsertAsCitedQuotation(mMsgBody,
                                           NS_LITERAL_STRING(""),
                                           PR_TRUE,
                                           NS_LITERAL_STRING("UTF-8"),
                                           getter_AddRefs(nodeInserted));
      else
        mailEditor->InsertAsQuotation(mMsgBody, getter_AddRefs(nodeInserted));
    }
      
  }

  if (aEditor)
  {
    nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(aEditor);
    if (textEditor)
    {
      nsCOMPtr<nsISelection> selection;
      nsCOMPtr<nsIDOMNode>   parent;
      PRInt32                offset;
      nsresult               rv;

      // get parent and offset of mailcite
      rv = GetNodeLocation(nodeInserted, address_of(parent), &offset);
      NS_ENSURE_SUCCESS(rv, rv);

      // get selection
      aEditor->GetSelection(getter_AddRefs(selection));
      if (selection)
      {
        // place selection after mailcite
        selection->Collapse(parent, offset+1);
        // insert a break at current selection
        textEditor->InsertLineBreak();
        selection->Collapse(parent, offset+1);
      }
      nsCOMPtr<nsISelectionController> selCon;
      aEditor->GetSelectionController(getter_AddRefs(selCon));

      if (selCon)
        selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_ANCHOR_REGION, PR_TRUE);
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(QuotingOutputStreamListener, nsIStreamListener)
////////////////////////////////////////////////////////////////////////////////////
// END OF QUOTING LISTENER
////////////////////////////////////////////////////////////////////////////////////

/* attribute MSG_ComposeType type; */
NS_IMETHODIMP nsMsgCompose::SetType(MSG_ComposeType aType)
{
 
  mType = aType;
  return NS_OK;
}

NS_IMETHODIMP nsMsgCompose::GetType(MSG_ComposeType *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgCompose::QuoteMessage(const char *msgURI)
{
  nsresult    rv;

  mQuotingToFollow = PR_FALSE;
  
  // Create a mime parser (nsIStreamConverter)!
  mQuote = do_CreateInstance(NS_MSGQUOTE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the consumer output stream.. this will receive all the HTML from libmime
  mQuoteStreamListener =
    new QuotingOutputStreamListener(msgURI, PR_FALSE, PR_FALSE, m_identity,
                                    m_compFields->GetCharacterSet(), mCharsetOverride, PR_FALSE);
  
  if (!mQuoteStreamListener)
  {
#ifdef NS_DEBUG
    printf("Failed to create mQuoteStreamListener\n");
#endif
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(mQuoteStreamListener);

  mQuoteStreamListener->SetComposeObj(this);

  rv = mQuote->QuoteMessage(msgURI, PR_FALSE, mQuoteStreamListener, 
                            mCharsetOverride ? m_compFields->GetCharacterSet() : "", PR_FALSE);
  return rv;
}

nsresult
nsMsgCompose::QuoteOriginalMessage(const char *originalMsgURI, PRInt32 what) // New template
{
  nsresult    rv;

  mQuotingToFollow = PR_FALSE;
  
  // Create a mime parser (nsIStreamConverter)!
  mQuote = do_CreateInstance(NS_MSGQUOTE_CONTRACTID, &rv);
  if (NS_FAILED(rv) || !mQuote)
    return NS_ERROR_FAILURE;

  PRBool bAutoQuote = PR_TRUE;
  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs)
    prefs->GetBoolPref("mail.auto_quote", &bAutoQuote);

  // Create the consumer output stream.. this will receive all the HTML from libmime
  mQuoteStreamListener =
    new QuotingOutputStreamListener(originalMsgURI, what != 1, !bAutoQuote, m_identity,
                                    m_compFields->GetCharacterSet(), mCharsetOverride, PR_TRUE);
  
  if (!mQuoteStreamListener)
  {
#ifdef NS_DEBUG
    printf("Failed to create mQuoteStreamListener\n");
#endif
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(mQuoteStreamListener);

  mQuoteStreamListener->SetComposeObj(this);

  rv = mQuote->QuoteMessage(originalMsgURI, what != 1, mQuoteStreamListener, 
                            mCharsetOverride ? m_compFields->GetCharacterSet() : "", !bAutoQuote);
  return rv;
}

//CleanUpRecipient will remove un-necesary "<>" when a recipient as an address without name
void nsMsgCompose::CleanUpRecipients(nsString& recipients)
{
  PRUint16 i;
  PRBool startANewRecipient = PR_TRUE;
  PRBool removeBracket = PR_FALSE;
  nsAutoString newRecipient;
  PRUnichar aChar;

  for (i = 0; i < recipients.Length(); i ++)
  {
    aChar = recipients[i];
    switch (aChar)
    {
      case '<'  :
        if (startANewRecipient)
          removeBracket = PR_TRUE;
        else
          newRecipient += aChar;
        startANewRecipient = PR_FALSE;
        break;

      case '>'  :
        if (removeBracket)
          removeBracket = PR_FALSE;
        else
          newRecipient += aChar;
        break;

      case ' '  :
        newRecipient += aChar;
        break;

      case ','  :
        newRecipient += aChar;
        startANewRecipient = PR_TRUE;
        removeBracket = PR_FALSE;
        break;

      default   :
        newRecipient += aChar;
        startANewRecipient = PR_FALSE;
        break;
    } 
  }
  recipients = newRecipient;
}

nsresult nsMsgCompose::ProcessReplyFlags()
{
  nsresult rv;
  // check to see if we were doing a reply or a forward, if we were, set the answered field flag on the message folder
  // for this URI.
  if (mType == nsIMsgCompType::Reply || 
      mType == nsIMsgCompType::ReplyAll ||
      mType == nsIMsgCompType::ReplyToGroup ||
      mType == nsIMsgCompType::ReplyToSender ||
      mType == nsIMsgCompType::ReplyToSenderAndGroup ||
      mType == nsIMsgCompType::ForwardAsAttachment ||              
      mType == nsIMsgCompType::ForwardInline)
  {
    if (!mOriginalMsgURI.IsEmpty())
    {
      char *uriList = PL_strdup(mOriginalMsgURI.get());
      if (!uriList)
        return NS_ERROR_OUT_OF_MEMORY;
      char *newStr = uriList;
      char *uri;
      while (nsnull != (uri = nsCRT::strtok(newStr, ",", &newStr)))
      {
        nsCOMPtr <nsIMsgDBHdr> msgHdr;
        rv = GetMsgDBHdrFromURI(uri, getter_AddRefs(msgHdr));
        NS_ENSURE_SUCCESS(rv,rv);
        if (msgHdr)
        {
          // get the folder for the message resource
          nsCOMPtr<nsIMsgFolder> msgFolder;
          msgHdr->GetFolder(getter_AddRefs(msgFolder));
          if (msgFolder)
          {
            nsMsgDispositionState dispositionSetting = nsIMsgFolder::nsMsgDispositionState_Replied;
            if (mType == nsIMsgCompType::ForwardAsAttachment ||              
                mType == nsIMsgCompType::ForwardInline)
              dispositionSetting = nsIMsgFolder::nsMsgDispositionState_Forwarded;

            msgFolder->AddMessageDispositionState(msgHdr, dispositionSetting);
            if (mType != nsIMsgCompType::ForwardAsAttachment)
              break;         // just safeguard
          }
        }
      }
      PR_Free(uriList);
    }
  }

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for both the send operation and the copy operation. 
// We have to create this class to listen for message send completion and deal with
// failures in both send and copy operations
////////////////////////////////////////////////////////////////////////////////////
NS_IMPL_ADDREF(nsMsgComposeSendListener)
NS_IMPL_RELEASE(nsMsgComposeSendListener)

/*
NS_IMPL_QUERY_INTERFACE4(nsMsgComposeSendListener,
                         nsIMsgComposeSendListener,
                         nsIMsgSendListener,
                         nsIMsgCopyServiceListener,
                         nsIWebProgressListener)
*/
NS_INTERFACE_MAP_BEGIN(nsMsgComposeSendListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMsgComposeSendListener)
  NS_INTERFACE_MAP_ENTRY(nsIMsgComposeSendListener)
  NS_INTERFACE_MAP_ENTRY(nsIMsgSendListener)
  NS_INTERFACE_MAP_ENTRY(nsIMsgCopyServiceListener)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END


nsMsgComposeSendListener::nsMsgComposeSendListener(void) 
{ 
#if defined(DEBUG_ducarroz)
  printf("CREATE nsMsgComposeSendListener: %x\n", this);
#endif
  mDeliverMode = 0;

  NS_INIT_ISUPPORTS(); 
}

nsMsgComposeSendListener::~nsMsgComposeSendListener(void) 
{
#if defined(DEBUG_ducarroz)
  printf("DISPOSE nsMsgComposeSendListener: %x\n", this);
#endif
}

NS_IMETHODIMP nsMsgComposeSendListener::SetMsgCompose(nsIMsgCompose *obj)
{
  mWeakComposeObj = getter_AddRefs(NS_GetWeakReference(obj));
  return NS_OK;
}

NS_IMETHODIMP nsMsgComposeSendListener::SetDeliverMode(MSG_DeliverMode deliverMode)
{
  mDeliverMode = deliverMode;
  return NS_OK;
}

nsresult
nsMsgComposeSendListener::OnStartSending(const char *aMsgID, PRUint32 aMsgSize)
{
#ifdef NS_DEBUG
  printf("nsMsgComposeSendListener::OnStartSending()\n");
#endif

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnStartSending(aMsgID, aMsgSize);
  }

  return NS_OK;
}
  
nsresult
nsMsgComposeSendListener::OnProgress(const char *aMsgID, PRUint32 aProgress, PRUint32 aProgressMax)
{
#ifdef NS_DEBUG
  printf("nsMsgComposeSendListener::OnProgress()\n");
#endif

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnProgress(aMsgID, aProgress, aProgressMax);
  }

  return NS_OK;
}

nsresult
nsMsgComposeSendListener::OnStatus(const char *aMsgID, const PRUnichar *aMsg)
{
#ifdef NS_DEBUG
  printf("nsMsgComposeSendListener::OnStatus()\n");
#endif

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnStatus(aMsgID, aMsg);
  }

  return NS_OK;
}
  
nsresult nsMsgComposeSendListener::OnSendNotPerformed(const char *aMsgID, nsresult aStatus)
{
 // since OnSendNotPerformed is called in the case where the user aborts the operation
 // by closing the compose window, we need not do the stuff required 
 // for closing the windows. However we would need to do the other operations as below.

  nsresult rv = NS_OK;

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    compose->NotifyStateListeners(eComposeProcessDone,aStatus);

    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnSendNotPerformed(aMsgID, aStatus) ;
  }

  return rv ;
}


nsresult nsMsgComposeSendListener::OnStopSending(const char *aMsgID, nsresult aStatus, const PRUnichar *aMsg, 
                                     nsIFileSpec *returnFileSpec)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    nsCOMPtr<nsIMsgProgress> progress;
    compose->GetProgress(getter_AddRefs(progress));
    
    //Unregister ourself from msg compose progress
    if (progress)
      progress->UnregisterListener(this);

    if (NS_SUCCEEDED(aStatus))
    {
#ifdef NS_DEBUG
      printf("nsMsgComposeSendListener: Success on the message send operation!\n");
#endif
      nsCOMPtr<nsIMsgCompFields> compFields;
      compose->GetCompFields(getter_AddRefs(compFields));

      // only process the reply flags if we successfully sent the message
      compose->ProcessReplyFlags();

      // Close the window ONLY if we are not going to do a save operation
      nsXPIDLString fieldsFCC;
      if (NS_SUCCEEDED(compFields->GetFcc(getter_Copies(fieldsFCC))))
      {
        if (!fieldsFCC.IsEmpty())
        {
          if (fieldsFCC.Equals(NS_LITERAL_STRING("nocopy://"),
                               nsCaseInsensitiveStringComparator()))
          {
            compose->NotifyStateListeners(eComposeProcessDone, NS_OK);
            if (progress)
              progress->CloseProgressDialog(PR_FALSE);
            compose->CloseWindow(PR_TRUE);
          }
        }
      }
      else
      {
        compose->NotifyStateListeners(eComposeProcessDone, NS_OK);
        if (progress)
          progress->CloseProgressDialog(PR_FALSE);
        compose->CloseWindow(PR_TRUE);  // if we fail on the simple GetFcc call, close the window to be safe and avoid
                                        // windows hanging around to prevent the app from exiting.
      }

      // Remove the current draft msg when sending draft is done.
      MSG_ComposeType compType = nsIMsgCompType::Draft;
      compose->GetType(&compType);
      if (compType == nsIMsgCompType::Draft)
        RemoveCurrentDraftMessage(compose, PR_FALSE);
    }
    else
    {
#ifdef NS_DEBUG
      printf("nsMsgComposeSendListener: the message send operation failed!\n");
#endif
      compose->NotifyStateListeners(eComposeProcessDone,aStatus);
      if (progress)
        progress->CloseProgressDialog(PR_TRUE);
    }

    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnStopSending(aMsgID, aStatus, aMsg, returnFileSpec);
  }

  return rv;
}

nsresult
nsMsgComposeSendListener::OnGetDraftFolderURI(const char *aFolderURI)
{
  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    compose->SetSavedFolderURI(aFolderURI);

    nsCOMPtr<nsIMsgSendListener> externalListener;
    compose->GetExternalSendListener(getter_AddRefs(externalListener));
    if (externalListener)
      externalListener->OnGetDraftFolderURI(aFolderURI);
  }

  return NS_OK;
}


nsresult
nsMsgComposeSendListener::OnStartCopy()
{
#ifdef NS_DEBUG
  printf("nsMsgComposeSendListener::OnStartCopy()\n");
#endif

  return NS_OK;
}

nsresult
nsMsgComposeSendListener::OnProgress(PRUint32 aProgress, PRUint32 aProgressMax)
{
#ifdef NS_DEBUG
  printf("nsMsgComposeSendListener::OnProgress() - COPY\n");
#endif
  return NS_OK;
}
  
nsresult
nsMsgComposeSendListener::OnStopCopy(nsresult aStatus)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    // Ok, if we are here, we are done with the send/copy operation so
    // we have to do something with the window....SHOW if failed, Close
    // if succeeded

    nsCOMPtr<nsIMsgProgress> progress;
    compose->GetProgress(getter_AddRefs(progress));
    if (progress)
      progress->CloseProgressDialog(NS_FAILED(aStatus));
    compose->NotifyStateListeners(eComposeProcessDone,aStatus);

    if (NS_SUCCEEDED(aStatus))
    {
#ifdef NS_DEBUG
      printf("nsMsgComposeSendListener: Success on the message copy operation!\n");
#endif
      // We should only close the window if we are done. Things like templates
      // and drafts aren't done so their windows should stay open
      if ( (mDeliverMode != nsIMsgSend::nsMsgSaveAsDraft) &&
           (mDeliverMode != nsIMsgSend::nsMsgSaveAsTemplate) )
        compose->CloseWindow(PR_TRUE);
      else
      {  
        compose->NotifyStateListeners(eSaveInFolderDone,aStatus);
        if (mDeliverMode == nsIMsgSend::nsMsgSaveAsDraft)
        { 
          // Remove the current draft msg when saving to draft is done. Also,
          // if it was a NEW comp type, it's now DRAFT comp type. Otherwise
          // if the msg is then sent we won't be able to remove the saved msg.
          compose->SetType(nsIMsgCompType::Draft);
          RemoveCurrentDraftMessage(compose, PR_TRUE);
        }
      }
    }
#ifdef NS_DEBUG
    else
      printf("nsMsgComposeSendListener: the message copy operation failed!\n");
#endif
  }

  return rv;
}

nsresult
nsMsgComposeSendListener::GetMsgFolder(nsIMsgCompose *compObj, nsIMsgFolder **msgFolder)
{
  nsresult    rv;
  nsCOMPtr<nsIMsgFolder> aMsgFolder;
  nsXPIDLCString folderUri;

  rv = compObj->GetSavedFolderURI(getter_Copies(folderUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRDFService> rdfService (do_GetService("@mozilla.org/rdf/rdf-service;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv); 

  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(folderUri, getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv, rv); 

  aMsgFolder = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  *msgFolder = aMsgFolder;
  NS_IF_ADDREF(*msgFolder);
  return rv;
}

nsresult
nsMsgComposeSendListener::RemoveCurrentDraftMessage(nsIMsgCompose *compObj, PRBool calledByCopy)
{
  nsresult    rv;
  nsCOMPtr <nsIMsgCompFields> compFields = nsnull;

    rv = compObj->GetCompFields(getter_AddRefs(compFields));
  NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't get compose fields");
  if (NS_FAILED(rv) || !compFields)
    return rv;

  nsXPIDLCString curDraftIdURL;
  nsMsgKey newUid = 0;
  nsXPIDLCString newDraftIdURL;
  nsCOMPtr<nsIMsgFolder> msgFolder;

  rv = compFields->GetDraftId(getter_Copies(curDraftIdURL));
  NS_ASSERTION((NS_SUCCEEDED(rv) && (curDraftIdURL)), "RemoveCurrentDraftMessage can't get draft id");

  // Skip if no draft id (probably a new draft msg).
  if (NS_SUCCEEDED(rv) && curDraftIdURL.get() && strlen(curDraftIdURL.get()))
  { 
    nsCOMPtr <nsIMsgDBHdr> msgDBHdr;
    rv = GetMsgDBHdrFromURI(curDraftIdURL, getter_AddRefs(msgDBHdr));
    NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't get msg header DB interface pointer.");
    if (NS_SUCCEEDED(rv) && msgDBHdr)
    { // get the folder for the message resource
    msgDBHdr->GetFolder(getter_AddRefs(msgFolder));
    NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't get msg folder interface pointer.");
    if (NS_SUCCEEDED(rv) && msgFolder)
    { // build the msg arrary
      nsCOMPtr<nsISupportsArray> messageArray;
      rv = NS_NewISupportsArray(getter_AddRefs(messageArray));
      NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't allocate support array.");

      //nsCOMPtr<nsISupports> msgSupport = do_QueryInterface(msgDBHdr, &rv);
      //NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't get msg header interface pointer.");
      if (NS_SUCCEEDED(rv) && messageArray)
      {   // ready to delete the msg
        rv = messageArray->AppendElement(msgDBHdr);
        NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't append msg header to array.");
        if (NS_SUCCEEDED(rv))
        rv = msgFolder->DeleteMessages(messageArray, nsnull, PR_TRUE, PR_FALSE, nsnull, PR_FALSE /*allowUndo*/);
        NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveCurrentDraftMessage can't delete message.");
      }
    }
    }
    else
    {
      // If we get here we have the case where the draft folder 
                  // is on the server and
      // it's not currently open (in thread pane), so draft 
                  // msgs are saved to the server
      // but they're not in our local DB. In this case, 
      // GetMsgDBHdrFromURI() will never
      // find the msg. If the draft folder is a local one 
      // then we'll not get here because
      // the draft msgs are saved to the local folder and 
      // are in local DB. Make sure the
      // msg folder is imap.  Even if we get here due to 
      // DB errors (worst case), we should
      // still try to delete msg on the server because 
      // that's where the master copy of the
      // msgs are stored, if draft folder is on the server.  
      // For local case, since DB is bad
      // we can't do anything with it anyway so it'll be 
      // noop in this case.
      rv = GetMsgFolder(compObj, getter_AddRefs(msgFolder));
      if (NS_SUCCEEDED(rv) && msgFolder)
      {
        nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(msgFolder);
        NS_ASSERTION(imapFolder, "The draft folder MUST be an imap folder in order to mark the msg delete!");
        if (NS_SUCCEEDED(rv) && imapFolder)
        {
          char * str = PL_strstr(curDraftIdURL.get(), "#");
          NS_ASSERTION(str, "Failed to get current draft id url");
          if (str)
          {
          nsMsgKeyArray messageID;
          nsCAutoString srcStr(str+1);
          PRInt32 num=0, err;
          num = srcStr.ToInteger(&err);
          if (num != nsMsgKey_None)
          {
                                                messageID.Add(num);
            rv = imapFolder->StoreImapFlags(kImapMsgDeletedFlag, PR_TRUE, messageID.GetArray(), messageID.GetSize());
          }
          }
        }
      }
    }
  
  }

  // Now get the new uid so that next save will remove the right msg
  // regardless whether or not the exiting msg can be deleted.
  if (calledByCopy)
  {
    nsCOMPtr<nsIMsgSend> msgSend;
    rv = compObj->GetMessageSend(getter_AddRefs(msgSend));
    NS_ASSERTION(msgSend, "RemoveCurrentDraftMessage msgSend is null.");
    if (NS_FAILED(rv) || !msgSend)
      return rv;

    rv = msgSend->GetMessageKey(&newUid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Make sure we have a folder interface pointer
    if (!msgFolder)
    {
      rv = GetMsgFolder(compObj, getter_AddRefs(msgFolder));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Reset draft (uid) url with the new uid.
    if (msgFolder && newUid)
    {
      rv = msgFolder->GenerateMessageURI(newUid, getter_Copies(newDraftIdURL));
      NS_ENSURE_SUCCESS(rv, rv);

      compFields->SetDraftId(newDraftIdURL.get());
    }
  }
  return rv;
}

nsresult
nsMsgComposeSendListener::SetMessageKey(PRUint32 aMessageKey)
{
  return NS_OK;
}

nsresult
nsMsgComposeSendListener::GetMessageId(nsCString* aMessageId)
{
  return NS_OK;
}

/* void onStateChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in unsigned long aStateFlags, in nsresult aStatus); */
NS_IMETHODIMP nsMsgComposeSendListener::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  if (aStateFlags == nsIWebProgressListener::STATE_STOP)
  {
    nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
    if (compose)
    {
      nsCOMPtr<nsIMsgProgress> progress;
      compose->GetProgress(getter_AddRefs(progress));
      
      //Time to stop any pending operation...
      if (progress)
      {
        //Unregister ourself from msg compose progress
        progress->UnregisterListener(this);
    
        PRBool  bCanceled = PR_FALSE;
        progress->GetProcessCanceledByUser(&bCanceled);
        if (bCanceled)
        {
          nsXPIDLString msg; 
          nsCOMPtr<nsIMsgStringService> strBundle = do_GetService(NS_MSG_COMPOSESTRINGSERVICE_CONTRACTID);
          strBundle->GetStringByID(NS_MSG_CANCELLING, getter_Copies(msg));
          progress->OnStatusChange(nsnull, nsnull, 0, msg);
        }
      }
      
      nsCOMPtr<nsIMsgSend> msgSend;
      compose->GetMessageSend(getter_AddRefs(msgSend));
      if (msgSend)
          msgSend->Abort();
    }
  }
  return NS_OK;
}

/* void onProgressChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aCurSelfProgress, in long aMaxSelfProgress, in long aCurTotalProgress, in long aMaxTotalProgress); */
NS_IMETHODIMP nsMsgComposeSendListener::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  /* Ignore this call */
  return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP nsMsgComposeSendListener::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
  /* Ignore this call */
  return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP nsMsgComposeSendListener::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
  /* Ignore this call */
  return NS_OK;
}

/* void onSecurityChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in unsigned long state); */
NS_IMETHODIMP nsMsgComposeSendListener::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
  /* Ignore this call */
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////////
// This is a class that will allow us to listen to state changes in the Ender 
// compose window. This is important since we must wait until we are told Ender
// is ready before we do various quoting operations
////////////////////////////////////////////////////////////////////////////////////

NS_IMPL_ISUPPORTS1(nsMsgDocumentStateListener, nsIDocumentStateListener)

nsMsgDocumentStateListener::nsMsgDocumentStateListener(void)
{
  NS_INIT_ISUPPORTS();
}

nsMsgDocumentStateListener::~nsMsgDocumentStateListener(void)
{
}

void        
nsMsgDocumentStateListener::SetComposeObj(nsIMsgCompose *obj)
{
  mWeakComposeObj = getter_AddRefs(NS_GetWeakReference(obj));
}

nsresult
nsMsgDocumentStateListener::NotifyDocumentCreated(void)
{
  // Ok, now the document has been loaded, so we are ready to setup  
  // the compose window and let the user run hog wild!

  // Now, do the appropriate startup operation...signature only
  // or quoted message and signature...

#ifdef MSGCOMP_TRACE_PERFORMANCE
  nsCOMPtr<nsIMsgComposeService> composeService (do_GetService(NS_MSGCOMPOSESERVICE_CONTRACTID));
  composeService->TimeStamp("Editor is done loading about:blank. This is New Compose window time.  Time for MIME.", PR_FALSE);
#endif

  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
  {
    compose->SetEditorFromEditorShell();

    PRBool quotingToFollow = PR_FALSE;
    compose->GetQuotingToFollow(&quotingToFollow);
    if (quotingToFollow)
      return compose->BuildQuotedMessageAndSignature();
    else
    {
      compose->NotifyStateListeners(eComposeFieldsReady, NS_OK);
      return compose->BuildBodyMessageAndSignature();
    }
  }
  return NS_OK;
}

nsresult
nsMsgDocumentStateListener::NotifyDocumentWillBeDestroyed(void)
{
  nsCOMPtr<nsIMsgCompose>compose = do_QueryReferent(mWeakComposeObj);
  if (compose)
    compose->SetEditorShell(nsnull);
      /* The editor will be destroyed. Set it to null to
       * be sure we wont use it anymore.
       */
  return NS_OK;
}

nsresult
nsMsgDocumentStateListener::NotifyDocumentStateChanged(PRBool nowDirty)
{
  return NS_OK;
}

nsresult
nsMsgCompose::ConvertHTMLToText(nsFileSpec& aSigFile, nsString &aSigData)
{
  nsresult    rv;
  nsAutoString    origBuf;

  rv = LoadDataFromFile(aSigFile, origBuf);
  if (NS_FAILED(rv))
    return rv;

  ConvertBufToPlainText(origBuf,PR_FALSE);
  aSigData = origBuf;
  return NS_OK;
}

nsresult
nsMsgCompose::ConvertTextToHTML(nsFileSpec& aSigFile, nsString &aSigData)
{
  nsresult    rv;
  nsAutoString    origBuf;

  rv = LoadDataFromFile(aSigFile, origBuf);
  if (NS_FAILED(rv))
    return rv;

  // Ok, once we are here, we need to escape the data to make sure that
  // we don't do HTML stuff with plain text sigs.
  //
  PRUnichar *escaped = nsEscapeHTML2(origBuf.get());
  if (escaped) 
  {
    aSigData.Append(escaped);
    nsCRT::free(escaped);
  }
  else
  {
    aSigData.Append(origBuf);
  }

  return NS_OK;
}

nsresult
nsMsgCompose::LoadDataFromFile(nsFileSpec& fSpec, nsString &sigData)
{
  PRInt32       readSize;
  PRInt32       nGot;
  char          *readBuf;
  char          *ptr;

  if (fSpec.IsDirectory()) {
    NS_ASSERTION(0,"file is a directory");
    return NS_MSG_ERROR_READING_FILE;  
  }

  nsInputFileStream tempFile(fSpec);
  if (!tempFile.is_open())
    return NS_MSG_ERROR_READING_FILE;  
  
  readSize = fSpec.GetFileSize();
  ptr = readBuf = (char *)PR_Malloc(readSize + 1);  if (!readBuf)
    return NS_ERROR_OUT_OF_MEMORY;
  memset(readBuf, 0, readSize + 1);

  while (readSize) {
    nGot = tempFile.read(ptr, readSize);
    if (nGot > 0) {
      readSize -= nGot;
      ptr += nGot;
    }
    else {
      readSize = 0;
    }
  }
  tempFile.close();

  nsCAutoString sigEncoding;
  sigEncoding.Assign(nsMsgI18NParseMetaCharset(&fSpec));
  PRBool removeSigCharset = !sigEncoding.IsEmpty() && m_composeHTML;

  //default to platform encoding for signature files w/o meta charset
  if (sigEncoding.IsEmpty())
    sigEncoding.Assign(nsMsgI18NFileSystemCharset());

  if (NS_FAILED(ConvertToUnicode(sigEncoding.get(), readBuf, sigData)))
    sigData.AssignWithConversion(readBuf);

  //remove sig meta charset to allow user charset override during composition
  if (removeSigCharset)
  {
    nsAutoString metaCharset;
    metaCharset.Assign(NS_LITERAL_STRING("charset="));
    metaCharset.AppendWithConversion(sigEncoding.get());
    nsAString::const_iterator realstart, start, end;
    sigData.BeginReading(start);
    sigData.EndReading(end);
    realstart = start;
    if (FindInReadable(metaCharset, start, end,
                       nsCaseInsensitiveStringComparator()))
      sigData.Cut(Distance(realstart, start), Distance(start, end));
  }

  PR_FREEIF(readBuf);
  return NS_OK;
}

nsresult
nsMsgCompose::BuildQuotedMessageAndSignature(void)
{
  // 
  // This should never happen...if it does, just bail out...
  //
  NS_ASSERTION(m_editor, "BuildQuotedMessageAndSignature but no editor!\n");
  if (!m_editor)
    return NS_ERROR_FAILURE;

  // We will fire off the quote operation and wait for it to
  // finish before we actually do anything with Ender...
  return QuoteOriginalMessage(mQuoteURI.get(), mWhatHolder);
}

//
// This will process the signature file for the user. This method
// will always append the results to the mMsgBody member variable.
//
nsresult
nsMsgCompose::ProcessSignature(nsIMsgIdentity *identity, nsString *aMsgBody)
{
  nsresult    rv;

  // Now, we can get sort of fancy. This is the time we need to check
  // for all sorts of user defined stuff, like signatures and editor
  // types and the like!
  //
  //    user_pref(".....signature_file", "y:\\sig.html");
  //    user_pref(".....use_signature_file", true);
  //
  // Note: We will have intelligent signature behavior in that we
  // look at the signature file first...if the extension is .htm or 
  // .html, we assume its HTML, otherwise, we assume it is plain text
  //
  // ...and that's not all! What we will also do now is look and see if
  // the file is an image file. If it is an image file, then we should 
  // insert the correct HTML into the composer to have it work, but if we
  // are doing plain text compose, we should insert some sort of message
  // saying "Image Signature Omitted" or something.
  //
  nsCAutoString sigNativePath;
  PRBool        useSigFile = PR_FALSE;
  PRBool        htmlSig = PR_FALSE;
  PRBool        imageSig = PR_FALSE;
  nsAutoString  sigData;
  nsAutoString sigOutput;

  if (identity)
  {
    rv = identity->GetAttachSignature(&useSigFile);
    if (NS_SUCCEEDED(rv) && useSigFile) 
    {
      useSigFile = PR_FALSE;  // by default, assume no signature file!

      nsCOMPtr<nsILocalFile> sigFile;
      rv = identity->GetSignature(getter_AddRefs(sigFile));
      if (NS_SUCCEEDED(rv) && sigFile) {
        rv = sigFile->GetNativePath(sigNativePath);
        if (NS_SUCCEEDED(rv) && !sigNativePath.IsEmpty())
          useSigFile = PR_TRUE; // ok, there's a signature file
      }
    }
  }
  
  // Now, if they didn't even want to use a signature, we should
  // just return nicely.
  //
  if ((!useSigFile) || NS_FAILED(rv))
    return NS_OK;

  nsFileSpec    testSpec(sigNativePath.get());
  
  // If this file doesn't really exist, just bail!
  if (!testSpec.Exists())
    return NS_OK;

  // Once we get here, we need to figure out if we have the correct file
  // type for the editor.
  //
  nsCOMPtr<nsIURL> fileUrl(do_CreateInstance(NS_STANDARDURL_CONTRACTID));
  if (fileUrl)
  {
    fileUrl->SetFilePath(sigNativePath);
    nsCAutoString fileExt;
    rv = fileUrl->GetFileExtension(fileExt);
    if (NS_SUCCEEDED(rv) && !fileExt.IsEmpty())
  {
    // Now, most importantly, we need to figure out what the content type is for
    // this signature...if we can't, we assume text
    rv = NS_OK;
      nsXPIDLCString sigContentType;
    nsCOMPtr<nsIMIMEService> mimeFinder (do_GetService(NS_MIMESERVICE_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv) && mimeFinder) 
        mimeFinder->GetTypeFromExtension(fileExt.get(), getter_Copies(sigContentType));
  
      if (!sigContentType.IsEmpty())
    {
        imageSig = (!PL_strncasecmp(sigContentType.get(), "image/", 6));
      if (!imageSig)
          htmlSig = (!PL_strcasecmp(sigContentType.get(), TEXT_HTML));
    }
    else
        htmlSig = ( (!PL_strcasecmp(fileExt.get(), "HTM")) || (!PL_strcasecmp(fileExt.get(), "HTML")) );
    }
  }

  static const char      htmlBreak[] = "<BR>";
  static const char      dashes[] = "-- ";
  static const char      htmlsigopen[] = "<div class=\"moz-signature\">";
  static const char      htmlsigclose[] = "</div>";    /* XXX: Due to a bug in
                             4.x' HTML editor, it will not be able to
                             break this HTML sig, if quoted (for the user to
                             interleave a comment). */
  static const char      _preopen[] = "<pre class=\"moz-signature\" cols=%d>";
  char*                  preopen;
  static const char      preclose[] = "</pre>";

  PRInt32 wrapLength = 72; // setup default value in case GetWrapLength failed
  GetWrapLength(&wrapLength);
  preopen = PR_smprintf(_preopen, wrapLength);
  if (!preopen)
    return NS_ERROR_OUT_OF_MEMORY;
  
  if (imageSig)
  {
    // We have an image signature. If we're using the in HTML composer, we
    // should put in the appropriate HTML for inclusion, otherwise, do nothing.
    if (m_composeHTML)
    {
      sigOutput.AppendWithConversion(htmlBreak);
      sigOutput.AppendWithConversion(htmlsigopen);
      sigOutput.AppendWithConversion(dashes);
      sigOutput.AppendWithConversion(htmlBreak);
      sigOutput.Append(NS_LITERAL_STRING("<img src=\"file:///"));
           /* XXX pp This gives me 4 slashes on Unix, that's at least one to
              much. Better construct the URL with some service. */
      sigOutput.AppendWithConversion(testSpec);
      sigOutput.Append(NS_LITERAL_STRING("\" border=0>"));
      sigOutput.AppendWithConversion(htmlsigclose);
    }
  }
  else
  {
    // is this a text sig with an HTML editor?
    if ( (m_composeHTML) && (!htmlSig) )
      ConvertTextToHTML(testSpec, sigData);
    // is this a HTML sig with a text window?
    else if ( (!m_composeHTML) && (htmlSig) )
      ConvertHTMLToText(testSpec, sigData);
    else // We have a match...
      LoadDataFromFile(testSpec, sigData);  // Get the data!
  }

  // Now that sigData holds data...if any, append it to the body in a nice
  // looking manner
  if (!sigData.IsEmpty())
  {
    if (m_composeHTML)
    {
      sigOutput.AppendWithConversion(htmlBreak);
      if (htmlSig)
        sigOutput.AppendWithConversion(htmlsigopen);
      else
        sigOutput.AppendWithConversion(preopen);
    }
    else
      sigOutput.AppendWithConversion(CRLF);

    nsDependentSubstring firstFourChars(sigData, 0, 4);
    
    if (!(firstFourChars.Equals(NS_LITERAL_STRING("-- \n")) ||
          firstFourChars.Equals(NS_LITERAL_STRING("-- \r"))))
    {
      sigOutput.AppendWithConversion(dashes);
    
      if (!m_composeHTML || !htmlSig)
        sigOutput.AppendWithConversion(CRLF);
      else if (m_composeHTML)
        sigOutput.AppendWithConversion(htmlBreak);
    }

    sigOutput.Append(sigData);
    
    if (m_composeHTML)
    {
      if (htmlSig)
        sigOutput.AppendWithConversion(htmlsigclose);
      else
        sigOutput.AppendWithConversion(preclose);
    }
  }

  aMsgBody->Append(sigOutput);
  PR_Free(preopen);
  return NS_OK;
}

nsresult
nsMsgCompose::BuildBodyMessageAndSignature()
{
  PRUnichar   *bod = nsnull;
  nsresult    rv = NS_OK;

  // 
  // This should never happen...if it does, just bail out...
  //
  if (!m_editor)
    return NS_ERROR_FAILURE;

  // 
  // Now, we have the body so we can just blast it into the
  // composition editor window.
  //
  m_compFields->GetBody(&bod);

  /* Some time we want to add a signature and sometime we wont. Let's figure that now...*/
  PRBool addSignature;
  switch (mType)
  {
    case nsIMsgCompType::New :
    case nsIMsgCompType::Reply :        /* should not append! but just in case */
    case nsIMsgCompType::ReplyAll :       /* should not append! but just in case */
    case nsIMsgCompType::ForwardAsAttachment :  /* should not append! but just in case */
    case nsIMsgCompType::ForwardInline :
    case nsIMsgCompType::NewsPost :
    case nsIMsgCompType::ReplyToGroup :
    case nsIMsgCompType::ReplyToSender : 
    case nsIMsgCompType::ReplyToSenderAndGroup :
      addSignature = PR_TRUE;
      break;

    case nsIMsgCompType::Draft :
    case nsIMsgCompType::Template :
      addSignature = PR_FALSE;
      break;
    
    case nsIMsgCompType::MailToUrl :
      addSignature = !(bod && *bod != 0);
      break;

    default :
      addSignature = PR_FALSE;
      break;
  }

  nsAutoString empty;
  nsAutoString bodStr(bod);
  nsAutoString tSignature;

  if (addSignature)
    ProcessSignature(m_identity, &tSignature);

  rv = ConvertAndLoadComposeWindow(empty, bodStr, tSignature,
                                   PR_FALSE, m_composeHTML);

  PR_FREEIF(bod);
  return rv;
}

nsresult nsMsgCompose::NotifyStateListeners(TStateListenerNotification aNotificationType, nsresult aResult)
{
  if (!mStateListeners)
    return NS_OK;    // maybe there just aren't any.
 
  PRUint32 numListeners;
  nsresult rv = mStateListeners->Count(&numListeners);
  if (NS_FAILED(rv)) return rv;

  PRUint32 i;
  for (i = 0; i < numListeners;i++)
  {
    nsCOMPtr<nsISupports> iSupports = getter_AddRefs(mStateListeners->ElementAt(i));
    nsCOMPtr<nsIMsgComposeStateListener> thisListener = do_QueryInterface(iSupports);
    if (thisListener)
    {
      switch (aNotificationType)
      {
        case eComposeFieldsReady:
          thisListener->NotifyComposeFieldsReady();
          break;
        
        case eComposeProcessDone:
          thisListener->ComposeProcessDone(aResult);
          break;
        
        case eSaveInFolderDone:
          thisListener->SaveInFolderDone(m_folderName.get());
          break;

        default:
          NS_NOTREACHED("Unknown notification");
          break;
      }
    }
  }

  return NS_OK;
}

nsresult nsMsgCompose::AttachmentPrettyName(const char* url, PRUnichar** _retval)
{
  nsCAutoString unescapeURL(url);
  nsUnescape(NS_CONST_CAST(char*, unescapeURL.get()));
  if (unescapeURL.IsEmpty())
  {
    nsAutoString unicodeUrl;
    unicodeUrl.AssignWithConversion(url);

    *_retval = ToNewUnicode(unicodeUrl);
    return NS_OK;
  }
  
  if (PL_strncasestr(unescapeURL.get(), "file:", 5))
  {
    nsFileURL fileUrl(url);
    nsFileSpec fileSpec(fileUrl);
    char * leafName = fileSpec.GetLeafName();
    if (leafName && *leafName)
    {
        nsAutoString tempStr;
        nsresult rv = ConvertToUnicode(nsMsgI18NFileSystemCharset(), leafName, tempStr);
        if (NS_FAILED(rv))
          tempStr.AssignWithConversion(leafName);
      *_retval = ToNewUnicode(tempStr);
      nsCRT::free(leafName);
      return NS_OK;
    }
  }

  if (PL_strncasestr(unescapeURL.get(), "http:", 5))
    unescapeURL.Cut(0, 7);

  *_retval = ToNewUnicode(unescapeURL);
  return NS_OK;
}

static nsresult OpenAddressBook(const char * dbUri, nsIAddrDatabase** aDatabase)
{
  NS_ENSURE_ARG_POINTER(aDatabase);

  nsresult rv;
  nsCOMPtr<nsIAddressBook> addressBook = do_GetService(NS_ADDRESSBOOK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = addressBook->GetAbDatabaseFromURI(dbUri, aDatabase);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsMsgCompose::GetABDirectories(const char * dirUri, nsISupportsArray* directoriesArray, PRBool searchSubDirectory)
{
  static PRBool collectedAddressbookFound;
  if (nsCRT::strcmp(dirUri, kMDBDirectoryRoot) == 0)
    collectedAddressbookFound = PR_FALSE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFService> rdfService (do_GetService("@mozilla.org/rdf/rdf-service;1", &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(dirUri, getter_AddRefs(resource));
  if (NS_FAILED(rv)) return rv;

  // query interface 
  nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(resource, &rv));
  if (NS_FAILED(rv)) return rv;

  if (!searchSubDirectory)
      return rv;
  
  nsCOMPtr<nsIEnumerator> subDirectories;
  if (NS_SUCCEEDED(directory->GetChildNodes(getter_AddRefs(subDirectories))) && subDirectories)
  {
    nsCOMPtr<nsISupports> item;
    if (NS_SUCCEEDED(subDirectories->First()))
    {
      do
      {
        if (NS_SUCCEEDED(subDirectories->CurrentItem(getter_AddRefs(item))))
        {
          directory = do_QueryInterface(item, &rv);
          if (NS_SUCCEEDED(rv))
          {
            PRBool bIsMailList;

            if (NS_SUCCEEDED(directory->GetIsMailList(&bIsMailList)) && bIsMailList)
              continue;

      nsCOMPtr<nsIRDFResource> source(do_QueryInterface(directory));
      
            nsXPIDLCString uri;
            // rv = directory->GetDirUri(getter_Copies(uri));
            rv = source->GetValue(getter_Copies(uri));
            NS_ENSURE_SUCCESS(rv, rv);

            PRInt32 pos;
            if (nsCRT::strcmp((const char *)uri, kPersonalAddressbookUri) == 0)
              pos = 0;
            else
            {
              PRUint32 count = 0;
              directoriesArray->Count(&count);

              if (PL_strcmp((const char *)uri, kCollectedAddressbookUri) == 0)
              {
                collectedAddressbookFound = PR_TRUE;
                pos = count;
              }
              else
              {
                if (collectedAddressbookFound && count > 1)
                  pos = count - 1;
                else
                  pos = count;
              }
            }

            directoriesArray->InsertElementAt(directory, pos);
            rv = GetABDirectories((const char *)uri, directoriesArray, PR_TRUE);
          }
        }
      } while (NS_SUCCEEDED(subDirectories->Next()));
    }
  }
  return rv;
}

nsresult nsMsgCompose::BuildMailListArray(nsIAddrDatabase* database, nsIAbDirectory* parentDir, nsISupportsArray* array)
{
  nsresult rv;

  nsCOMPtr<nsIAbDirectory> directory;
  nsCOMPtr<nsIEnumerator> subDirectories;

  if (NS_SUCCEEDED(parentDir->GetChildNodes(getter_AddRefs(subDirectories))) && subDirectories)
  {
    nsCOMPtr<nsISupports> item;
    if (NS_SUCCEEDED(subDirectories->First()))
    {
      do
      {
        if (NS_SUCCEEDED(subDirectories->CurrentItem(getter_AddRefs(item))))
        {
          directory = do_QueryInterface(item, &rv);
          if (NS_SUCCEEDED(rv))
          {
            PRBool bIsMailList;

            if (NS_SUCCEEDED(directory->GetIsMailList(&bIsMailList)) && bIsMailList)
            {
              nsXPIDLString listName;
              nsXPIDLString listDescription;

              directory->GetDirName(getter_Copies(listName));
              directory->GetDescription(getter_Copies(listDescription));

              nsMsgMailList* mailList = new nsMsgMailList(nsAutoString((const PRUnichar*)listName),
                    nsAutoString((const PRUnichar*)listDescription), directory);
              if (!mailList)
                return NS_ERROR_OUT_OF_MEMORY;
              NS_ADDREF(mailList);

              rv = array->AppendElement(mailList);
              if (NS_FAILED(rv))
                return rv;

              NS_RELEASE(mailList);
            }
          }
        }
      } while (NS_SUCCEEDED(subDirectories->Next()));
    }
  }
  return rv;
}


nsresult nsMsgCompose::GetMailListAddresses(nsString& name, nsISupportsArray* mailListArray, nsISupportsArray** addressesArray)
{
  nsresult rv;
  nsCOMPtr<nsIEnumerator> enumerator;

  rv = mailListArray->Enumerate(getter_AddRefs(enumerator));
  if (NS_SUCCEEDED(rv))
  {
    for (rv = enumerator->First(); NS_SUCCEEDED(rv); rv = enumerator->Next())
    {
      nsMsgMailList* mailList;
      rv = enumerator->CurrentItem((nsISupports**)&mailList);
      if (NS_SUCCEEDED(rv) && mailList)
      {
        if (name.Equals(mailList->mFullName, nsCaseInsensitiveStringComparator()))
        {
          if (!mailList->mDirectory)
            return NS_ERROR_FAILURE;

          mailList->mDirectory->GetAddressLists(addressesArray);
          NS_RELEASE(mailList);
          return NS_OK;
        }
        NS_RELEASE(mailList);
      }
    }
  }

  return NS_ERROR_FAILURE;
}


#define MAX_OF_RECIPIENT_ARRAY    3

NS_IMETHODIMP nsMsgCompose::CheckAndPopulateRecipients(PRBool populateMailList, PRBool returnNonHTMLRecipients, PRUnichar **nonHTMLRecipients, PRUint32 *_retval)
{
  if (returnNonHTMLRecipients && !nonHTMLRecipients || !_retval)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = NS_OK;
  PRInt32 i;
  PRInt32 j;
  PRInt32 k;

  if (nonHTMLRecipients)
    *nonHTMLRecipients = nsnull;
  if (_retval)
    *_retval = nsIAbPreferMailFormat::unknown;

  /* First, build an array with original recipients */
  nsCOMPtr<nsISupportsArray> recipientsList[MAX_OF_RECIPIENT_ARRAY];

  nsXPIDLString originalRecipients[MAX_OF_RECIPIENT_ARRAY];
  m_compFields->GetTo(getter_Copies(originalRecipients[0]));
  m_compFields->GetCc(getter_Copies(originalRecipients[1]));
  m_compFields->GetBcc(getter_Copies(originalRecipients[2]));

  nsCOMPtr<nsIMsgRecipientArray> addressArray;
  nsCOMPtr<nsIMsgRecipientArray> emailArray;
  for (i = 0; i < MAX_OF_RECIPIENT_ARRAY; i ++)
  {
    if (originalRecipients[i].IsEmpty())
      continue;
    rv = m_compFields->SplitRecipientsEx((const PRUnichar *)(originalRecipients[i]), getter_AddRefs(addressArray), getter_AddRefs(emailArray));
    if (NS_SUCCEEDED(rv))
    {
      PRInt32 nbrRecipients;
      nsXPIDLString emailAddr;
      nsXPIDLString addr;
      addressArray->GetCount(&nbrRecipients);

      recipientsList[i] = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv);
      if (NS_FAILED(rv))
        return rv;

      for (j = 0; j < nbrRecipients; j ++)
      {
        rv = addressArray->StringAt(j, getter_Copies(addr));
        if (NS_FAILED(rv))
          return rv;

        rv = emailArray->StringAt(j, getter_Copies(emailAddr));
        if (NS_FAILED(rv))
          return rv;

        nsMsgRecipient* recipient = new nsMsgRecipient(nsAutoString(addr), nsAutoString(emailAddr));
        if (!recipient)
           return  NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(recipient);

        rv = recipientsList[i]->AppendElement(recipient);
        NS_RELEASE(recipient);
        if (NS_FAILED(rv))
          return rv;
       }
    }
    else
      return rv;
  }

  /* Then look them up in the Addressbooks*/

  PRBool stillNeedToSearch = PR_TRUE;
  nsCOMPtr<nsIAddrDatabase> abDataBase;
  nsCOMPtr<nsIAbDirectory> abDirectory;   
  nsCOMPtr <nsIAbCard> existingCard;
  nsCOMPtr <nsISupportsArray> mailListAddresses;
  nsCOMPtr<nsIMsgHeaderParser> parser (do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID));
  nsCOMPtr<nsISupportsArray> mailListArray (do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISupportsArray> addrbookDirArray (do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv) && addrbookDirArray)
  {
    nsString dirPath;
    GetABDirectories(kAllDirectoryRoot, addrbookDirArray, PR_TRUE);
    PRUint32 nbrRecipients;

    PRUint32 nbrAddressbook;
    addrbookDirArray->Count(&nbrAddressbook);
    for (k = 0; k < (PRInt32)nbrAddressbook && stillNeedToSearch; k ++)
    {
      nsCOMPtr<nsISupports> item;
      addrbookDirArray->GetElementAt(k, getter_AddRefs(item));
      abDirectory = do_QueryInterface(item, &rv);
      if (NS_FAILED(rv))
        return rv;
      
      nsCOMPtr<nsIRDFResource> source(do_QueryInterface(abDirectory));

      nsXPIDLCString uri;
      // rv = abDirectory->GetDirUri(getter_Copies(uri));
      rv = source->GetValue(getter_Copies(uri));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool supportsMailingLists;
      rv = abDirectory->GetSupportsMailingLists(&supportsMailingLists);
      if (NS_FAILED(rv) || !supportsMailingLists)
        continue;

      rv = OpenAddressBook(uri.get(), getter_AddRefs(abDataBase));
      if (NS_FAILED(rv) || !abDataBase)
        continue;

      /* Collect all mailing list defined in this address book */
      rv = BuildMailListArray(abDataBase, abDirectory, mailListArray);
      if (NS_FAILED(rv))
        return rv;

      stillNeedToSearch = PR_FALSE;
      for (i = 0; i < MAX_OF_RECIPIENT_ARRAY; i ++)
      {
        if (!recipientsList[i])
          continue;
        recipientsList[i]->Count(&nbrRecipients);
        for (j = 0; j < (PRInt32)nbrRecipients; j ++, recipientsList[i]->Count(&nbrRecipients))
        {
          nsMsgRecipient* recipient = NS_STATIC_CAST(nsMsgRecipient*, recipientsList[i]->ElementAt(j));
          if (recipient && !recipient->mProcessed)
          {
            /* First check if it's a mailing list */
            if (NS_SUCCEEDED(GetMailListAddresses(recipient->mAddress, mailListArray, getter_AddRefs(mailListAddresses))))
            {
              if (populateMailList)
              {
                  PRUint32 nbrAddresses = 0;
                  for (mailListAddresses->Count(&nbrAddresses); nbrAddresses > 0; nbrAddresses --)
                  {
                    item = getter_AddRefs(mailListAddresses->ElementAt(nbrAddresses - 1));
                    existingCard = do_QueryInterface(item, &rv);
                    if (NS_FAILED(rv))
                      return rv;

                    nsXPIDLString pDisplayName;
                    nsXPIDLString pEmail;
                    nsAutoString fullNameStr;

                    PRBool bIsMailList;
                    rv = existingCard->GetIsMailList(&bIsMailList);
                    if (NS_FAILED(rv))
                      return rv;

                    rv = existingCard->GetDisplayName(getter_Copies(pDisplayName));
                    if (NS_FAILED(rv))
                      return rv;

                    if (bIsMailList)
                      rv = existingCard->GetNotes(getter_Copies(pEmail));
                    else
                      rv = existingCard->GetPrimaryEmail(getter_Copies(pEmail));
                    if (NS_FAILED(rv))
                      return rv;

                    if (parser)
                    {
                      nsXPIDLCString fullAddress;

                      parser->MakeFullAddress(nsnull, NS_ConvertUCS2toUTF8(pDisplayName).get(),
                                              NS_ConvertUCS2toUTF8(pEmail).get(), getter_Copies(fullAddress));
                      if (!fullAddress.IsEmpty())
                      {
                        /* We need to convert back the result from UTF-8 to Unicode */
                        (void)ConvertToUnicode(msgCompHeaderInternalCharset(), fullAddress.get(), fullNameStr);
                      }
                    }
                    if (fullNameStr.IsEmpty())
                    {
                      //oops, parser problem! I will try to do my best...
                      fullNameStr = pDisplayName;
                      fullNameStr.Append(NS_LITERAL_STRING(" <"));
                      if (bIsMailList)
                      {
                        if (pEmail && pEmail[0] != 0)
                          fullNameStr += pEmail;
                        else
                          fullNameStr += pDisplayName;
                      }
                      else
                        fullNameStr += pEmail;
                      fullNameStr.Append(PRUnichar('>'));
                    }

                    if (fullNameStr.IsEmpty())
                      continue;

                    /* Now we need to insert the new address into the list of recipient */
                    nsMsgRecipient* newRecipient = new nsMsgRecipient(fullNameStr, nsAutoString(pEmail));
                    if (!recipient)
                       return  NS_ERROR_OUT_OF_MEMORY;
                    NS_ADDREF(newRecipient);

                    if (bIsMailList)
                    {
                      //TODO: we must to something to avoid recursivity
                      stillNeedToSearch = PR_TRUE;
                    }
                    else
                    {
                      newRecipient->mPreferFormat = nsIAbPreferMailFormat::unknown;
                      rv = existingCard->GetPreferMailFormat(&newRecipient->mPreferFormat);
                      if (NS_SUCCEEDED(rv))
                        recipient->mProcessed = PR_TRUE;
                    }
                    rv = recipientsList[i]->InsertElementAt(newRecipient, j + 1);
                    NS_RELEASE(newRecipient);
                    if (NS_FAILED(rv))
                      return rv;
                  }
                  rv = recipientsList[i]->RemoveElementAt(j);
                 j --;
              }
              else
                recipient->mProcessed = PR_TRUE;

              NS_RELEASE(recipient);
              continue;
            }

            /* Then if we have a card for this email address */
            nsCAutoString emailStr; emailStr.AssignWithConversion(recipient->mEmail);
            // Please DO NOT change the 3rd param of GetCardFromAttribute() call to 
            // PR_TRUE (ie, case insensitive) without reading bugs #128535 and #121478.
            rv = abDataBase->GetCardFromAttribute(abDirectory, kPriEmailColumn, emailStr.get(), PR_FALSE /* retain case */, getter_AddRefs(existingCard));
            if (NS_SUCCEEDED(rv) && existingCard)
            {
              recipient->mPreferFormat = nsIAbPreferMailFormat::unknown;
              rv = existingCard->GetPreferMailFormat(&recipient->mPreferFormat);
              if (NS_SUCCEEDED(rv))
                recipient->mProcessed = PR_TRUE;
            }
            else
              stillNeedToSearch = PR_TRUE;
            NS_RELEASE(recipient);
          }
        }
      }

      if (abDataBase)
        abDataBase->Close(PR_FALSE);
    }
  }

  /* Finally return the list of non HTML recipient if requested and/or rebuilt the recipient field.
     Also, check for domain preference when preferFormat is unknown
  */
    nsAutoString recipientsStr;
    nsAutoString nonHtmlRecipientsStr;
    nsAutoString plaintextDomains;
    nsAutoString htmlDomains;
    nsAutoString domain;
    
    nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
    if (prefs)
    {
      nsXPIDLString str;
      prefs->CopyUnicharPref("mailnews.plaintext_domains", getter_Copies(str));
      plaintextDomains = str;
      prefs->CopyUnicharPref("mailnews.html_domains", getter_Copies(str));
      htmlDomains = str;
    }

    *_retval = -1;
    for (i = 0; i < MAX_OF_RECIPIENT_ARRAY; i ++)
    {
      if (!recipientsList[i])
        continue;
      recipientsStr.SetLength(0);
      PRUint32 nbrRecipients;

      recipientsList[i]->Count(&nbrRecipients);
      for (j = 0; j < (PRInt32)nbrRecipients; j ++)
      {
        nsMsgRecipient* recipient = NS_STATIC_CAST(nsMsgRecipient*, recipientsList[i]->ElementAt(j));
        if (recipient)
        {          
          /* if we don't have a prefer format for a recipient, check the domain in case we have a format defined for it */
          if (recipient->mPreferFormat == nsIAbPreferMailFormat::unknown &&
              (plaintextDomains.Length() || htmlDomains.Length()))
          {
            PRInt32 atPos = recipient->mEmail.FindChar('@');
            if (atPos >= 0)
            {
              recipient->mEmail.Right(domain, recipient->mEmail.Length() - atPos - 1);
              if (FindInReadable(domain, plaintextDomains, nsCaseInsensitiveStringComparator()))
                recipient->mPreferFormat = nsIAbPreferMailFormat::plaintext;
              else
                if (FindInReadable(domain, htmlDomains, nsCaseInsensitiveStringComparator()))
                  recipient->mPreferFormat = nsIAbPreferMailFormat::html;
            }
          }

          /* setup return value */
          switch (recipient->mPreferFormat)
          {
            case nsIAbPreferMailFormat::html :
              if (*_retval == -1)
                *_retval = nsIAbPreferMailFormat::html;
              break;

            case nsIAbPreferMailFormat::plaintext :
              if (*_retval == -1 || *_retval == nsIAbPreferMailFormat::html)
                *_retval = nsIAbPreferMailFormat::plaintext;
              break;

            default :
              *_retval = nsIAbPreferMailFormat::unknown;
              break;
          }
 
          if (populateMailList)
          {
            if (! recipientsStr.IsEmpty())
              recipientsStr.Append(PRUnichar(','));
            recipientsStr.Append(recipient->mAddress);
          }

          if (returnNonHTMLRecipients && recipient->mPreferFormat != nsIAbPreferMailFormat::html)
          {
            if (! nonHtmlRecipientsStr.IsEmpty())
              nonHtmlRecipientsStr.Append(PRUnichar(','));
            nonHtmlRecipientsStr.Append(recipient->mEmail);
          }

          NS_RELEASE(recipient);
        }
      }

      if (populateMailList)
      {
        switch (i)
        {
          case 0 : m_compFields->SetTo(recipientsStr.get());  break;
          case 1 : m_compFields->SetCc(recipientsStr.get());  break;
          case 2 : m_compFields->SetBcc(recipientsStr.get()); break;
        }
      }
  }

  if (returnNonHTMLRecipients)
    *nonHTMLRecipients = ToNewUnicode(nonHtmlRecipientsStr);

  return rv;
}

nsresult nsMsgCompose::GetNoHtmlNewsgroups(const char *newsgroups, char **_retval)
{
    //FIX ME: write me
    nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
    *_retval = nsnull;
    return rv;
}

/* Decides which tags trigger which convertible mode, i.e. here is the logic
   for BodyConvertible */
// Helper function. Parameters are not checked.
nsresult nsMsgCompose::TagConvertible(nsIDOMNode *node,  PRInt32 *_retval)
{
    nsresult rv;

    *_retval = nsIMsgCompConvertible::No;

    nsAutoString elementStr;
    rv = node->GetNodeName(elementStr);
    if (NS_FAILED(rv))
      return rv;
    nsCAutoString element; element.AssignWithConversion(elementStr);

    nsCOMPtr<nsIDOMNode> pItem;
    if      (
              element.EqualsIgnoreCase("#text") ||
              element.EqualsIgnoreCase("br") ||
              element.EqualsIgnoreCase("p") ||
              element.EqualsIgnoreCase("pre") ||
              element.EqualsIgnoreCase("tt") ||
              element.EqualsIgnoreCase("html") ||
              element.EqualsIgnoreCase("head") ||
              element.EqualsIgnoreCase("title")
            )
    {
      *_retval = nsIMsgCompConvertible::Plain;
    }
    else if (
              //element.EqualsIgnoreCase("blockquote") || // see below
              element.EqualsIgnoreCase("ul") ||
              element.EqualsIgnoreCase("ol") ||
              element.EqualsIgnoreCase("li") ||
              element.EqualsIgnoreCase("dl") ||
              element.EqualsIgnoreCase("dt") ||
              element.EqualsIgnoreCase("dd")
            )
    {
      *_retval = nsIMsgCompConvertible::Yes;
    }
    else if (
              //element.EqualsIgnoreCase("a") || // see below
              element.EqualsIgnoreCase("h1") ||
              element.EqualsIgnoreCase("h2") ||
              element.EqualsIgnoreCase("h3") ||
              element.EqualsIgnoreCase("h4") ||
              element.EqualsIgnoreCase("h5") ||
              element.EqualsIgnoreCase("h6") ||
              element.EqualsIgnoreCase("hr") ||
              (
                mConvertStructs
                &&
                (
                  element.EqualsIgnoreCase("em") ||
                  element.EqualsIgnoreCase("strong") ||
                  element.EqualsIgnoreCase("code") ||
                  element.EqualsIgnoreCase("b") ||
                  element.EqualsIgnoreCase("i") ||
                  element.EqualsIgnoreCase("u")
                )
              )
            )
    {
      *_retval = nsIMsgCompConvertible::Altering;
    }
    else if (element.EqualsIgnoreCase("body"))
    {
      *_retval = nsIMsgCompConvertible::Plain;

      nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(node);
      if (domElement)
      {
        PRBool hasAttribute;
        if (NS_SUCCEEDED(domElement->HasAttribute(NS_LITERAL_STRING("background"), &hasAttribute))
            && hasAttribute)  // There is a background image
          *_retval = nsIMsgCompConvertible::No; 
        else if (NS_SUCCEEDED(domElement->HasAttribute(NS_LITERAL_STRING("text"), &hasAttribute))
            && hasAttribute)  // There is a text color
          *_retval = nsIMsgCompConvertible::Altering;
        else if (NS_SUCCEEDED(domElement->HasAttribute(NS_LITERAL_STRING("bgcolor"), &hasAttribute))
            && hasAttribute)  // There is a background color
          *_retval = nsIMsgCompConvertible::Altering;

        //ignore special color setting for link, vlink and alink at this point.
      }

    }
    else if (element.EqualsIgnoreCase("blockquote"))
    {
      // Skip <blockquote type="cite">
      *_retval = nsIMsgCompConvertible::Yes;

      nsCOMPtr<nsIDOMNamedNodeMap> pAttributes;
      if (NS_SUCCEEDED(node->GetAttributes(getter_AddRefs(pAttributes)))
          && pAttributes)
      {
        nsAutoString typeName; typeName.Assign(NS_LITERAL_STRING("type"));
        if (NS_SUCCEEDED(pAttributes->GetNamedItem(typeName,
                                                   getter_AddRefs(pItem)))
            && pItem)
        {
          nsAutoString typeValue;
          if (NS_SUCCEEDED(pItem->GetNodeValue(typeValue)))
          {
            typeValue.StripChars("\"");
            if (typeValue.Equals(NS_LITERAL_STRING("cite"),
                                 nsCaseInsensitiveStringComparator()))
              *_retval = nsIMsgCompConvertible::Plain;
          }
        }
      }
    }
    else if (
              element.EqualsIgnoreCase("div") ||
              element.EqualsIgnoreCase("span") ||
              element.EqualsIgnoreCase("a")
            )
    {
      /* Do some special checks for these tags. They are inside this |else if|
         for performance reasons */
      nsCOMPtr<nsIDOMNamedNodeMap> pAttributes;

      /* First, test, if the <a>, <div> or <span> is inserted by our
         [TXT|HTML]->HTML converter */
      /* This is for an edge case: A Mozilla user replies to plaintext per HTML
         and the recipient of that HTML msg, also a Mozilla user, replies to
         that again. Then we'll have to recognize the stuff inserted by our
         TXT->HTML converter. */
      if (NS_SUCCEEDED(node->GetAttributes(getter_AddRefs(pAttributes)))
          && pAttributes)
      {
        nsAutoString className;
        className.Assign(NS_LITERAL_STRING("class"));
        if (NS_SUCCEEDED(pAttributes->GetNamedItem(className,
                                                   getter_AddRefs(pItem)))
            && pItem)
        {
          nsAutoString classValue;
          if (NS_SUCCEEDED(pItem->GetNodeValue(classValue))
              && (classValue.EqualsIgnoreCase("moz-txt", 7) ||
                  classValue.EqualsIgnoreCase("\"moz-txt", 8)))
          {
            *_retval = nsIMsgCompConvertible::Plain;
            return rv;  // Inconsistent :-(
          }
        }
      }

      // Maybe, it's an <a> element inserted by another recognizer (e.g. 4.x')
      if (element.EqualsIgnoreCase("a"))
      {
        /* Ignore anchor tag, if the URI is the same as the text
           (as inserted by recognizers) */
        *_retval = nsIMsgCompConvertible::Altering;

        if (NS_SUCCEEDED(node->GetAttributes(getter_AddRefs(pAttributes)))
            && pAttributes)
        {
          nsAutoString hrefName; hrefName.Assign(NS_LITERAL_STRING("href"));
          if (NS_SUCCEEDED(pAttributes->GetNamedItem(hrefName,
                                                     getter_AddRefs(pItem)))
              && pItem)
          {
            nsAutoString hrefValue;
            PRBool hasChild;
            if (NS_SUCCEEDED(pItem->GetNodeValue(hrefValue))
                && NS_SUCCEEDED(node->HasChildNodes(&hasChild)) && hasChild)
            {
              nsCOMPtr<nsIDOMNodeList> children;
              if (NS_SUCCEEDED(node->GetChildNodes(getter_AddRefs(children)))
                  && children
                  && NS_SUCCEEDED(children->Item(0, getter_AddRefs(pItem)))
                  && pItem)
              {
                nsAutoString textValue;
                if (NS_SUCCEEDED(pItem->GetNodeValue(textValue))
                    && textValue == hrefValue)
                  *_retval = nsIMsgCompConvertible::Plain;
              }
            }
          }
        }
      }

      // Lastly, test, if it is just a "simple" <div> or <span>
      else if (
                element.EqualsIgnoreCase("div") ||
                element.EqualsIgnoreCase("span")
              )
      {
        /* skip only if no style attribute */
        *_retval = nsIMsgCompConvertible::Plain;

        if (NS_SUCCEEDED(node->GetAttributes(getter_AddRefs(pAttributes)))
            && pAttributes)
        {
          nsAutoString styleName;
          styleName.Assign(NS_LITERAL_STRING("style"));
          if (NS_SUCCEEDED(pAttributes->GetNamedItem(styleName,
                                                     getter_AddRefs(pItem)))
              && pItem)
          {
            nsAutoString styleValue;
            if (NS_SUCCEEDED(pItem->GetNodeValue(styleValue))
                && !styleValue.IsEmpty())
              *_retval = nsIMsgCompConvertible::No;
          }
        }
      }
    }

    return rv;
}

nsresult nsMsgCompose::_BodyConvertible(nsIDOMNode *node, PRInt32 *_retval)
{
    NS_ENSURE_TRUE(node && _retval, NS_ERROR_NULL_POINTER);

    nsresult rv;
    PRInt32 result;
    
    // Check this node
    rv = TagConvertible(node, &result);
    if (NS_FAILED(rv))
        return rv;

    // Walk tree recursively to check the children
    PRBool hasChild;
    if (NS_SUCCEEDED(node->HasChildNodes(&hasChild)) && hasChild)
    {
      nsCOMPtr<nsIDOMNodeList> children;
      if (NS_SUCCEEDED(node->GetChildNodes(getter_AddRefs(children)))
          && children)
      {
        PRUint32 nbrOfElements;
        rv = children->GetLength(&nbrOfElements);
        for (PRUint32 i = 0; NS_SUCCEEDED(rv) && i < nbrOfElements; i++)
        {
          nsCOMPtr<nsIDOMNode> pItem;
          if (NS_SUCCEEDED(children->Item(i, getter_AddRefs(pItem)))
              && pItem)
          {
            PRInt32 curresult;
            rv = _BodyConvertible(pItem, &curresult);
            if (NS_SUCCEEDED(rv) && curresult > result)
              result = curresult;
          }
        }
      }
    }

    *_retval = result;
    return rv;
}

nsresult nsMsgCompose::BodyConvertible(PRInt32 *_retval)
{
    NS_ENSURE_TRUE(_retval, NS_ERROR_NULL_POINTER);

    nsresult rv;
    
    if (!m_editor)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMElement> rootElement;
    rv = m_editor->GetRootElement(getter_AddRefs(rootElement));
    if (NS_FAILED(rv) || nsnull == rootElement)
      return rv;
      
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(rootElement);
    if (nsnull == node)
      return NS_ERROR_FAILURE;
      
    return _BodyConvertible(node, _retval);
}

nsresult nsMsgCompose::SetBodyAttribute(nsIEditor* editor, nsIDOMElement* element, nsString& name, nsString& value)
{
  /* cleanup the attribute name and check if we care about it */
  name.Trim(" \t\n\r\"");
  if (name.CompareWithConversion("text", PR_TRUE) == 0 ||
      name.CompareWithConversion("bgcolor", PR_TRUE) == 0 ||
      name.CompareWithConversion("link", PR_TRUE) == 0 ||
      name.CompareWithConversion("vlink", PR_TRUE) == 0 ||
      name.CompareWithConversion("alink", PR_TRUE) == 0 ||
      name.CompareWithConversion("background", PR_TRUE) == 0)
  {
    /* cleanup the attribute value */
    value.Trim(" \t\n\r");
    value.Trim("\"");

    /* remove the attribute from the node first */
    (void)editor->RemoveAttribute(element, name);

    /* then add the new one */
    return editor->SetAttribute(element, name, value);
  }

  return NS_OK;
}

nsresult nsMsgCompose::SetBodyAttributes(nsString& attributes)
{
  nsresult rv = NS_OK;

  if (attributes.IsEmpty()) //Nothing to do...
    return NS_OK;

  if (!m_editor)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> rootElement;
  rv = m_editor->GetRootElement(getter_AddRefs(rootElement));
  if (NS_FAILED(rv) || nsnull == rootElement)
    return rv;
  
  /* The following code will parse a string and extract pairs of
     <attribute name>=<attribute value>. A pair consists of an
     attribute name and an attribute value. An attribute value can
     be between double-quote and could potentially contains an
     '=' character which should not be interpreted as a delimiter.

     Once we get a pair, we can call SetBodyAttribute to set the
     attribute in the DOM.
  */
  PRUnichar * data = (PRUnichar *)attributes.get();
  PRUnichar * start = data;
  PRUnichar * end = data + attributes.Length();

  /* Let's initialized the delimiter to a '=', it's the delimiter between
     attribute name and attribute value */
  PRUnichar delimiter = '=';

  nsAutoString attributeName;
  nsAutoString attributeValue;
  
  while (data < end)
  {
    if (*data == '\n' || *data == '\r' || *data == '\t')
    {
      /* skip over line feed, carriage return and tab.
         That will work as long we are between attributes */
      start = data + 1;
    }
    else if (*data == delimiter)
    {
      if (attributeName.IsEmpty())
      {
        /* we found the end of an attribute name */
        attributeName.Assign(start, data - start);
        start = data + 1;
        if (start < end && *start == '\"')
        {
          /* if the attribute value start with a double-quote, we need to find the other one... */
          delimiter = '\"';
          data ++;
        }
        else
        {
          /* ... else the separator with the next attribute is a space */
          delimiter = ' ';
        }
      }
      else
      {
        if (delimiter == '\"')
        {
          /* we found the closing double-quote of an attribute value,
             let's find now the real attribute delimiter */
          delimiter = ' ';
        }
        else
        {
          /* we found the end of an attribute value */
          attributeValue.Assign(start, data - start);
          rv = SetBodyAttribute(m_editor, rootElement, attributeName, attributeValue);
          NS_ENSURE_SUCCESS(rv, rv);

          /* restart the search for the next pair of attribute */
          start = data + 1;
          attributeName.Truncate();
          attributeValue.Truncate();
          delimiter = '=';
        }
      }
    }

    data ++;
  }

  /* In the case the buffer we are parsing doesn't finish with a space character,
     we will exit the loop above before we get a chance to get the value of the attribute.
     Be sure we already got the name of the attribute before accepting the value */
  if (!attributeName.IsEmpty() && attributeValue.IsEmpty() && delimiter == ' ')
  {
    attributeValue.Assign(start, data - start);
    rv = SetBodyAttribute(m_editor, rootElement, attributeName, attributeValue);
  }

  return rv;
}

nsresult nsMsgCompose::SetSignature(nsIMsgIdentity *identity)
{
  nsresult rv;
  
  if (! m_editor)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> rootElement;
  rv = m_editor->GetRootElement(getter_AddRefs(rootElement));
  if (NS_FAILED(rv) || nsnull == rootElement)
    return rv;

  //First look for the current signature, if we have one
  nsCOMPtr<nsIDOMNode> lastNode;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMNode> tempNode;
  nsAutoString tagLocalName;

  rv = rootElement->GetLastChild(getter_AddRefs(lastNode));
  if (NS_SUCCEEDED(rv) && nsnull != lastNode)
  {
    node = lastNode;
    if (m_composeHTML)
    {
      /* In html, the signature is inside an element with
         class="moz-signature", it's must be the last node */
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
      if (element)
      {
        nsAutoString attributeName;
        nsAutoString attributeValue;
        attributeName.Assign(NS_LITERAL_STRING("class"));

        rv = element->GetAttribute(attributeName, attributeValue);
        if (NS_SUCCEEDED(rv))
        {
          if (attributeValue.Find("moz-signature", PR_TRUE) != kNotFound)
          {
            //Now, I am sure I get the right node!
            m_editor->BeginTransaction();
            node->GetPreviousSibling(getter_AddRefs(tempNode));
            rv = m_editor->DeleteNode(node);
            if (NS_FAILED(rv))
            {
              m_editor->EndTransaction();
              return rv;
            }

            //Also, remove the <br> right before the signature.
            if (tempNode)
            {
              tempNode->GetLocalName(tagLocalName);
              if (tagLocalName.Equals(NS_LITERAL_STRING("BR")))
                m_editor->DeleteNode(tempNode);
            }
            m_editor->EndTransaction();
          }
        }
      }
    }
    else
    {
      //In plain text, we have to walk back the dom look for the pattern <br>-- <br>
      PRUint16 nodeType;
      PRInt32 searchState = 0; //0=nothing, 1=br 2='-- '+br, 3=br+'-- '+br

      do
      {
        node->GetNodeType(&nodeType);
        node->GetLocalName(tagLocalName);
        switch (searchState)
        {
          case 0: 
            if (nodeType == nsIDOMNode::ELEMENT_NODE && tagLocalName.Equals(NS_LITERAL_STRING("BR")))
              searchState = 1;
            break;

          case 1:
            searchState = 0;
            if (nodeType == nsIDOMNode::TEXT_NODE)
            {
              nsString nodeValue;
              node->GetNodeValue(nodeValue);
              if (nodeValue.Equals(NS_LITERAL_STRING("-- ")))
                searchState = 2;
            }
            else
              if (nodeType == nsIDOMNode::ELEMENT_NODE && tagLocalName.Equals(NS_LITERAL_STRING("BR")))
              {
                searchState = 1;
                break;
              }
            break;

          case 2:
            if (nodeType == nsIDOMNode::ELEMENT_NODE && tagLocalName.Equals(NS_LITERAL_STRING("BR")))
              searchState = 3;
            else
              searchState = 0;               
            break;
        }

        tempNode = node;
      } while (searchState != 3 && NS_SUCCEEDED(tempNode->GetPreviousSibling(getter_AddRefs(node))) && node);
      
      if (searchState == 3)
      {
        //Now, I am sure I get the right node!
        m_editor->BeginTransaction();

        tempNode = lastNode;
        lastNode = node;
        do
        {
          node = tempNode;
          node->GetPreviousSibling(getter_AddRefs(tempNode));
          rv = m_editor->DeleteNode(node);
          if (NS_FAILED(rv))
          {
            m_editor->EndTransaction();
            return rv;
          }

        } while (node != lastNode && tempNode);
        m_editor->EndTransaction();
      }
    }
  }

  //Then add the new one if needed
  nsAutoString aSignature;
  ProcessSignature(identity, &aSignature);

  if (!aSignature.IsEmpty())
  {
    TranslateLineEnding(aSignature);

    m_editor->BeginTransaction();
    m_editor->EndOfDocument();
    if (m_composeHTML)
    {
      nsCOMPtr<nsIHTMLEditor> htmlEditor (do_QueryInterface(m_editor));
      rv = htmlEditor->InsertHTML(aSignature);
    }
    else
    {
      nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(m_editor));
      rv = textEditor->InsertText(aSignature);
    }
    m_editor->EndTransaction();
  }

  return rv;
}

nsresult nsMsgCompose::ResetNodeEventHandlers(nsIDOMNode *node)
{
  // Because event handler attributes set into a node before this node is inserted 
  // into the DOM are not recognised (in fact not compiled), we need to parsed again
  // the whole node and reset event handlers.
    
    nsresult rv;
    nsAutoString aStr;
    PRUint32 i;
    PRUint32 nbrOfElements;
    nsCOMPtr<nsIDOMNode> pItem;

    if (nsnull == node)
        return NS_ERROR_NULL_POINTER;
    
    nsCOMPtr<nsIDOMNamedNodeMap> pAttributes;
    rv = node->GetAttributes(getter_AddRefs(pAttributes));
    if (NS_SUCCEEDED(rv) && pAttributes)
    {
        rv = pAttributes->GetLength(&nbrOfElements);
        if (NS_FAILED(rv))
            return rv;

        for (i = 0; i < nbrOfElements; i ++)
        {
            rv = pAttributes->Item(i, getter_AddRefs(pItem));
            if (NS_SUCCEEDED(rv) && pItem)
            {
                rv = pItem->GetNodeName(aStr);
                if (NS_SUCCEEDED(rv))
                {
                    if (aStr.Find("on", PR_FALSE, 0, 2) == 0)   //name start with "on"
                    {
                        rv = pItem->GetNodeValue(aStr);
                        if (NS_SUCCEEDED(rv))
                            rv = pItem->SetNodeValue(aStr);
                        //Do not abort if it failed, let do the next one...
                    }
                }
            }
        }
        
        PRBool hasChild;
        rv = node->HasChildNodes(&hasChild);
        if (NS_SUCCEEDED(rv) && hasChild)
        {
            nsCOMPtr<nsIDOMNodeList> children;
            rv = node->GetChildNodes(getter_AddRefs(children));
            if (NS_SUCCEEDED(rv) && children)
            {
                rv = children->GetLength(&nbrOfElements);
                if (NS_FAILED(rv))
                    return rv;
                
                for (i = 0; i < nbrOfElements; i ++)
                {
                    rv = children->Item(i, getter_AddRefs(pItem));
                    if (NS_SUCCEEDED(rv) && pItem)
                       ResetNodeEventHandlers(pItem);
                    //Do not abort if it failed, let do the next one...
                }
            }
        }
    }
    
    return rv;
}

NS_IMETHODIMP nsMsgCompose::CheckCharsetConversion(nsIMsgIdentity *identity, char **fallbackCharset, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(identity);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = m_compFields->CheckCharsetConversion(fallbackCharset, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*_retval) 
  {
    nsXPIDLString fullName;
    nsXPIDLString organization;
    nsAutoString identityStrings;

    rv = identity->GetFullName(getter_Copies(fullName));
    NS_ENSURE_SUCCESS(rv, rv);
    if (fullName)
      identityStrings.Append(fullName.get());

    rv = identity->GetOrganization(getter_Copies(organization));
    NS_ENSURE_SUCCESS(rv, rv);
    if (organization)
      identityStrings.Append(organization.get());

    if (!identityStrings.IsEmpty())
    {
      // use fallback charset if that's already set
      const char *charset = (fallbackCharset && *fallbackCharset) ? *fallbackCharset : m_compFields->GetCharacterSet();
      *_retval = nsMsgI18Ncheck_data_in_charset_range(charset, identityStrings.get(),
                                                      fallbackCharset);
    }
  }

  return NS_OK;
}

NS_IMPL_ADDREF(nsMsgRecipient)
NS_IMPL_RELEASE(nsMsgRecipient)

NS_INTERFACE_MAP_BEGIN(nsMsgRecipient)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISupports)
NS_INTERFACE_MAP_END

nsMsgRecipient::nsMsgRecipient() :
  mPreferFormat(nsIAbPreferMailFormat::unknown),
  mProcessed(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
}
 
nsMsgRecipient::nsMsgRecipient(nsString fullAddress, nsString email, PRUint32 preferFormat, PRBool processed) :
  mAddress(fullAddress),
  mEmail(email),
  mPreferFormat(preferFormat),
  mProcessed(processed)
{
    NS_INIT_ISUPPORTS();
}

nsMsgRecipient::~nsMsgRecipient()
{
}

NS_IMPL_ADDREF(nsMsgMailList)
NS_IMPL_RELEASE(nsMsgMailList)

NS_INTERFACE_MAP_BEGIN(nsMsgMailList)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISupports)
NS_INTERFACE_MAP_END


nsMsgMailList::nsMsgMailList()
{
    NS_INIT_ISUPPORTS();
}
 
nsMsgMailList::nsMsgMailList(nsString listName, nsString listDescription, nsIAbDirectory* directory) :
  mDirectory(directory)
{
  NS_INIT_ISUPPORTS();
 
  nsCOMPtr<nsIMsgHeaderParser> parser (do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID));

  if (parser)
  {
    nsXPIDLCString fullAddress;
    nsXPIDLCString utf8Email;
    if (listDescription.IsEmpty())
      utf8Email.Adopt(ToNewUTF8String(listName));
    else
      utf8Email.Adopt(ToNewUTF8String(listDescription));

    parser->MakeFullAddress(nsnull, NS_ConvertUCS2toUTF8(listName).get(), utf8Email, getter_Copies(fullAddress));
    if (!fullAddress.IsEmpty())
    {
      /* We need to convert back the result from UTF-8 to Unicode */
      (void)ConvertToUnicode(msgCompHeaderInternalCharset(), fullAddress, mFullName);
    }
  }

  if (mFullName.IsEmpty())
  {
      //oops, parser problem! I will try to do my best...
      mFullName = listName;
      mFullName.Append(NS_LITERAL_STRING(" <"));
      if (listDescription.IsEmpty())
        mFullName += listName;
      else
        mFullName += listDescription;
      mFullName.Append(PRUnichar('>'));
  }

  mDirectory = directory;
}

nsMsgMailList::~nsMsgMailList()
{
}
