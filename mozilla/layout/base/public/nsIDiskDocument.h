/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL")=0; you may not use this file except in
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

#ifndef nsIDocumentOutput_h__
#define nsIDocumentOutput_h__


#include "nsIDOMDocument.h"


#define NS_IDISKDOCUMENT_IID												\
{/* {daf96f80-0183-11d3-9ce4-a865f869f0bc}		*/			\
0xdaf96f80, 0x0183, 0x11d3,														\
{ 0x9c, 0xe4, 0xa8, 0x65, 0xf8, 0x69, 0xf0, 0xbc } }


/**
 * This interface is used to save documents to disk (and other
 * output devices, like network streams one day).
 * 
 * It should be implemented by classes that need to save
 * documents, like the editor and browser windows.
 * 
 */

class nsIDOMDocument;
class nsFileSpec;


class nsIDiskDocument	: public nsISupports
{
public:

  static const nsIID& GetIID() { static nsIID iid = NS_IDISKDOCUMENT_IID; return iid; }

	typedef enum {eSaveFileText = 0, eSaveFileHTML = 1 } ESaveFileType;
	//typedef enum {eFileDiskFile = 0, eFileRemote = 1 } ESaveFileLocation;

  /** Initialize the document output. This may be called on document
    * creation, or lazily before the first save. For a document read
    * in from disk, it should be called on document instantiation.
    *
    * @param aDoc						The document being observed.
    * @param aFileSpec			Filespec for the file, if a disk version
    *												of the file exists already. Otherwise nsnull.
    */
  NS_IMETHOD InitDiskDocument(nsFileSpec *aFileSpec)=0;

  /** Save the file to disk. This will be called after the caller has
    * displayed a put file dialog, which the user confirmed. The internal
    * fileSpec of the document is only updated with the given fileSpec if inSaveCopy == PR_FALSE.
    *    
    * @param inFileSpec					File to which to stream the document.
    * @param inReplaceExisting	true if replacing an existing file, otherwise false.
    *    												If false and aFileSpec exists, SaveFile returns an error.
    * @param inSaveCopy					True to save a copy of the file, without changing the file
    *														referenced internally.
    * @param inSaveFileType			File type to save (text or HTML)
    * @param inSaveCharset			Charset to save the document in. If this is an empty
    *    												string, or "UCS2", then the doc will be saved as Unicode.
    */
  NS_IMETHOD SaveFile(			nsFileSpec*			inFileSpec,
  													PRBool 					inReplaceExisting,
  													PRBool					inSaveCopy,
  													ESaveFileType 	inSaveFileType,
  													const nsString&	inSaveCharset)=0;
  
  /** Return a file spec for the file. If the file has not been saved yet,
    * and thus has no fileSpec, this will return NS_ERROR_NOT_INITIALIZED.
    */

  NS_IMETHOD GetFileSpec(nsFileSpec& aFileSpec)=0;

  /** Get the modification count for the document. A +ve
    * mod count indicates that the document is dirty,
    * and needs saving.
    */
  NS_IMETHOD GetModCount(PRInt32 *outModCount)=0;
 
  /** Reset the modification count for the document.
  	* this marks the documents as 'clean' and not
  	* in need of saving.
    */
  NS_IMETHOD ResetModCount()=0;

  /** Increment the modification count for the document.
  	* this marks the documents as 'dirty' and
  	* in need of saving.
    */
  NS_IMETHOD IncrementModCount()=0;
	
};


#endif // nsIDocumentOutput_h__


