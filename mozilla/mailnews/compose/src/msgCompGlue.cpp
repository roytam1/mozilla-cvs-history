//
// All the definition here after are temporary and must be removed in the future when 
// be defined somewhere else...
//

#define NS_IMPL_IDS
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIPref.h"
#include "nsIMimeConverter.h"
#include "msgCore.h"
#include "rosetta_mailnews.h"
#include "nsMsgCompose.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kCMimeConverterCID, NS_MIME_CONVERTER_CID);

class MSG_Pane;

void				FE_DestroyMailCompositionContext(MWContext*) {return;}
const char  *FE_UsersSignature() {return NULL;}
void				FE_UpdateCompToolbar(MSG_Pane*) {return;}
void				FE_SetWindowLoading(MWContext *, URL_Struct *,Net_GetUrlExitFunc **) {return;}
XP_Bool			NET_AreThereActiveConnectionsForWindow(MWContext *) {return PR_FALSE;}
int					NET_SilentInterruptWindow(MWContext * window_id) {return 0;}
void				NET_FreeURLStruct (URL_Struct *) {return;}
URL_Struct  *NET_CreateURLStruct (const char *, NET_ReloadMethod) {return NULL;}
char *			NET_ParseURL (const char *, int ) {return NULL;}
int					NET_URL_Type (const char *) {return nsnull;}
XP_Bool			NET_IsLocalFileURL(char *address) {return PR_TRUE;}
int					NET_InterruptWindow(MWContext * window_id) {return 0;}
XP_Bool			NET_IsOffline() {return PR_FALSE;}

XP_FILE_URL_PATH	XP_PlatformFileToURL (const XP_FILE_NATIVE_PATH ) {return NULL;}
MWContext   *XP_FindContextOfType(MWContext *, MWContextType) {return NULL;}

char *			XP_StripLine (char * string)
{
	//ducarroz: we should use nsString::Trim
	//
    char * ptr;

	/* remove leading blanks */
    while(*string=='\t' || *string==' ' || *string=='\r' || *string=='\n')
		string++;    

    for(ptr=string; *ptr; ptr++)
		;   /* NULL BODY; Find end of string */

	/* remove trailing blanks */
    for(ptr--; ptr >= string; ptr--) 
	  {
        if(*ptr=='\t' || *ptr==' ' || *ptr=='\r' || *ptr=='\n') 
			*ptr = '\0'; 
        else 
			break;
	  }

    return string;

}

int					XP_Stat(const char * name, XP_StatStruct * outStat, XP_FileType type) {return 0;}
int					XP_FileTruncate(const char* name, XP_FileType type, int32 length) {return 0;}
Bool				XP_IsContextBusy(MWContext * context) {return PR_FALSE;}

const char *		MSG_GetSpecialFolderName(int ) {return NULL;}
const char *		MSG_GetQueueFolderName() {return NULL;}
MSG_Pane *			MSG_FindPane(MWContext* , MSG_PaneType ) {return NULL;}
int					MSG_ExplodeHeaderField(MSG_HEADER_SET,const char * ,MSG_HeaderEntry **) {return nsnull;}
char *				MSG_MakeFullAddress (const char* , const char* ) {return NULL;}
void				MSG_MailCompositionAllConnectionsComplete (MSG_Pane* /*pane*/) {return;}


void				INTL_DestroyCharCodeConverter(CCCDataObject) {return;}
unsigned char *		INTL_CallCharCodeConverter(CCCDataObject,const unsigned char *,int32) {return NULL;}
int					INTL_GetCharCodeConverter(int16 ,int16 ,CCCDataObject) {return nsnull;}
CCCDataObject		INTL_CreateCharCodeConverter() {return NULL;}
int16				INTL_GetCSIWinCSID(INTL_CharSetInfo) {return 2;}
INTL_CharSetInfo	LO_GetDocumentCharacterSetInfo(MWContext *) {return NULL;}
int16				INTL_GetCSIDocCSID(INTL_CharSetInfo obj) {return 2;}
int16				INTL_DefaultWinCharSetID(MWContext *) {return 2;}
int16				INTL_DefaultMailCharSetID(int16 csid) {return 2;}
int16				INTL_DefaultNewsCharSetID(int16 csid) {return 2;}
void				INTL_MessageSendToNews(XP_Bool toNews) {return;}

char        *MimeGuessURLContentName(MWContext *context, const char *url) {return NULL;}
void				MIME_GetMessageCryptoState(MWContext *,PRBool *,PRBool *,PRBool *,PRBool *) {return;}

XP_FILE_NATIVE_PATH WH_FileName (const char *, XP_FileType ) {return NULL;}
XP_Bool			isMacFile(char* filename) {return PR_FALSE;}

HJ10196
History_entry *		SHIST_GetCurrent(History *) {return NULL;}
int					MISC_ValidateReturnAddress (MWContext *,const char *) {return nsnull;}
char        *msg_MagicFolderName(MSG_Prefs* prefs, uint32 flag, int *pStatus) {return NULL;}

//time_t 			GetTimeMac()	{return 0;}

extern "C" {
  void FE_MsgShowHeaders(MSG_Pane *, MSG_HEADER_SET) {return;}
  HJ98703
}


//
// Begin international functions.
//

// Convert an unicode string to a C string with a given charset.
nsresult ConvertFromUnicode(const nsString& aCharset, 
                            const nsString& inString,
                            char** outCString)
{
  nsresult res;

  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res); 

  if(NS_SUCCEEDED(res) && (nsnull != ccm)) {
    nsIUnicodeEncoder* encoder = nsnull;
    nsString convCharset;

    // map to converter charset
    if (aCharset.EqualsIgnoreCase("us-ascii")) {
      convCharset.SetString("iso-8859-1");
    }
    else {
      convCharset = aCharset; 
    }

    // get an unicode converter
    res = ccm->GetUnicodeEncoder(&convCharset, &encoder);
    if(NS_SUCCEEDED(res) && (nsnull != encoder)) {
      const PRUnichar *unichars = inString.GetUnicode();
      PRInt32 unicharLength = inString.Length();
      PRInt32 dstLength;
      res = encoder->GetMaxLength(unichars, unicharLength, &dstLength);
      // allocale an output buffer
      *outCString = (char *) PR_Malloc(dstLength + 1);
      if (*outCString != nsnull) {
        PRInt32 originalLength = unicharLength;
        // convert from unicode
        res = encoder->Convert(unichars, &unicharLength, *outCString, &dstLength);
        // estimation of GetMaxLength was incorrect
        if (unicharLength < originalLength) {
          PR_Free(*outCString);
          res = NS_ERROR_FAILURE;
        }
        else {
          (*outCString)[dstLength] = '\0';
        }
      }
      else {
        res = NS_ERROR_OUT_OF_MEMORY;
      }
      NS_IF_RELEASE(encoder);
    }    
  }
  return res;
}

// Convert a C string to an unicode string.
nsresult ConvertToUnicode(const nsString& aCharset, 
                          const char* inCString, 
                          nsString& outString)
{
  nsresult res;
  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res); 

  if(NS_SUCCEEDED(res) && (nsnull != ccm)) {
    nsIUnicodeDecoder* decoder = nsnull;
    PRUnichar *unichars;
    PRInt32 unicharLength;
    nsString convCharset;

    // map to converter charset
    if (aCharset.EqualsIgnoreCase("us-ascii")) {
      convCharset.SetString("iso-8859-1");
    }
    else {
      convCharset = aCharset; 
    }
    // get an unicode converter
    res = ccm->GetUnicodeDecoder(&convCharset, &decoder);
    if(NS_SUCCEEDED(res) && (nsnull != decoder)) {
      PRInt32 srcLen = PL_strlen(inCString);
      res = decoder->Length(inCString, 0, srcLen, &unicharLength);
      // allocale an output buffer
      unichars = (PRUnichar *) PR_Malloc(unicharLength * sizeof(PRUnichar));
      if (unichars != nsnull) {
        // convert to unicode
        res = decoder->Convert(unichars, 0, &unicharLength, inCString, 0, &srcLen);
        outString.SetString(unichars, unicharLength);
        PR_Free(unichars);
      }
      else {
        res = NS_ERROR_OUT_OF_MEMORY;
      }
      NS_IF_RELEASE(decoder);
    }    
  }  
  return res;
}

// Charset to be used for the internatl processing.
const char *msgCompHeaderInternalCharset()
{
  // UTF-8 is a super set of us-ascii. 
  // We can use the same string manipulation methods as us-ascii without breaking non us-ascii characters. 
  return "UTF-8";
}

// MIME encoder, output string should be freed by PR_FREE
char * INTL_EncodeMimePartIIStr(const char *header, const char *charset, PRBool bUseMime) 
{
  // No MIME, just duplicate the string.
  if (PR_FALSE == bUseMime) {
    return PL_strdup(header);
  }

  char *encodedString = nsnull;
  nsIMimeConverter *converter;
  nsresult res = nsComponentManager::CreateInstance(kCMimeConverterCID, nsnull, 
                                           nsIMimeConverter::GetIID(), (void **)&converter);
  if (NS_SUCCEEDED(res) && nsnull != converter) {
    res = converter->EncodeMimePartIIStr_UTF8(header, charset, kMIME_ENCODED_WORD_SIZE, &encodedString);
    NS_RELEASE(converter);
  }
  return NS_SUCCEEDED(res) ? encodedString : nsnull;
}

// MIME decoder
nsresult INTL_DecodeMimePartIIStr(const nsString& header, nsString& charset, nsString& decodedString)
{
  nsIMimeConverter *converter;
  nsresult res = nsComponentManager::CreateInstance(kCMimeConverterCID, nsnull, 
                                                    nsIMimeConverter::GetIID(), (void **)&converter);
  if (NS_SUCCEEDED(res) && nsnull != converter) {
    res = converter->DecodeMimePartIIStr(header, charset, decodedString);
    NS_RELEASE(converter);
  }
  return res;
}

// Get a default mail character set.
char * INTL_GetDefaultMailCharset()
{
  nsresult res = NS_OK;
  char * retVal = nsnull;
  NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &res); 
  if (nsnull != prefs && NS_SUCCEEDED(res))
  {
	  char prefValue[kMAX_CSNAME+1];
	  PRInt32 prefLength = kMAX_CSNAME;
	  res = prefs->GetCharPref("intl.character_set_name", prefValue, &prefLength);
	  
	  if (NS_SUCCEEDED(res) && prefLength > 0) 
	  {
		//TODO: map to mail charset (e.g. Shift_JIS -> ISO-2022-JP) bug#3941.
		retVal = PL_strdup(prefValue);
	  }
	  else 
		retVal = PL_strdup("us-ascii");
  }

  return (nsnull != retVal) ? retVal : PL_strdup("us-ascii");
}

// Return True if a charset is stateful (e.g. JIS).
PRBool INTL_stateful_charset(const char *charset)
{
  //TODO: use charset manager's service
  return (PL_strcasecmp(charset, "iso-2022-jp") == 0);
}

// Check 7bit in a given buffer.
// This is expensive (both memory and performance).
// The check would be very simple if applied to an unicode text (e.g. nsString or utf-8).
// Possible optimazaion is to search ESC(0x1B) in case of iso-2022-jp and iso-2022-kr.
// Or convert and check line by line.
PRBool INTL_7bit_data_part(const char *charset, const char *inString, const PRUint32 size)
{
  char *aCString;
  nsString aCharset(charset);
  nsString outString;
  nsresult res;
  
  aCString = (char *) PR_Malloc(size + 1);
  if (nsnull != aCString) {
    PL_strncpy(aCString, inString, size); // make a C string
    res = ConvertToUnicode(aCharset, aCString, outString);
    PR_Free(aCString);
    if (NS_SUCCEEDED(res)) {
      for (PRInt32 i = 0; i < outString.Length(); i++) {
        if (outString.CharAt(i) > 127) {
          return PR_FALSE;
        }
      }
    }
  }
  return PR_TRUE;  // all 7 bit
}

// Obsolescent
int					INTL_IsLeadByte(int charSetID,unsigned char ch) {return 0;}
// Obsolescent
CCCDataObject		INTL_CreateDocToMailConverter(iDocumentContext context, XP_Bool isHTML, unsigned char *buffer,uint32 buffer_size) {return NULL;}
// Obsolescent
char *				INTL_GetAcceptLanguage() {return "en";}


//
// End international functions.
//
