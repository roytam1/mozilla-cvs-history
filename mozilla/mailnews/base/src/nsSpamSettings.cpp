#include "nsSpamSettings.h"
#include "nsISupportsObsolete.h"
#include "nsILocalFile.h"
#include "plstr.h"
#include "prmem.h"
#include "nsIFileStreams.h"
#include "nsIMsgHdr.h"
#include "nsEscape.h"

#define MOVE_TARGET_MODE_JUNK_ON_ACCOUNT 0
#define MOVE_TARGET_MODE_FOLDER          1

nsSpamSettings::nsSpamSettings()
{
  NS_INIT_ISUPPORTS();

  mLevel = 0;
  mMoveOnSpam = PR_FALSE;
  mMoveTargetMode = MOVE_TARGET_MODE_JUNK_ON_ACCOUNT;
  mPurge = PR_FALSE;
  mPurgeInterval = 14; // 14 days
  mUseWhiteList = PR_FALSE;
  mLoggingEnabled = PR_FALSE;
}

nsSpamSettings::~nsSpamSettings()
{
}

NS_IMPL_ISUPPORTS1(nsSpamSettings, nsISpamSettings)

NS_IMETHODIMP 
nsSpamSettings::GetLevel(PRInt32 *aLevel)
{
  NS_ENSURE_ARG_POINTER(aLevel);
  *aLevel = mLevel;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetLevel(PRInt32 aLevel)
{
  NS_ASSERTION((aLevel >= 0 && aLevel <= 100), "bad level");
  mLevel = aLevel;
  return NS_OK;
}

NS_IMETHODIMP 
nsSpamSettings::GetMoveTargetMode(PRInt32 *aMoveTargetMode)
{
  NS_ENSURE_ARG_POINTER(aMoveTargetMode);
  *aMoveTargetMode = mMoveTargetMode;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetMoveTargetMode(PRInt32 aMoveTargetMode)
{
  NS_ASSERTION((aMoveTargetMode == 0 || aMoveTargetMode == 1), "bad mode");
  mMoveTargetMode = aMoveTargetMode;
  return NS_OK;
}

NS_IMPL_GETSET(nsSpamSettings, LoggingEnabled, PRBool, mLoggingEnabled);
NS_IMPL_GETSET(nsSpamSettings, MoveOnSpam, PRBool, mMoveOnSpam);
NS_IMPL_GETSET(nsSpamSettings, Purge, PRBool, mPurge);
NS_IMPL_GETSET(nsSpamSettings, UseWhiteList, PRBool, mUseWhiteList);


NS_IMETHODIMP nsSpamSettings::GetWhiteListAbURI(char * *aWhiteListAbURI)
{
  NS_ENSURE_ARG_POINTER(aWhiteListAbURI);
  *aWhiteListAbURI = ToNewCString(mWhiteListAbURI);
  return NS_OK;
}
NS_IMETHODIMP nsSpamSettings::SetWhiteListAbURI(const char * aWhiteListAbURI)
{
  mWhiteListAbURI = aWhiteListAbURI;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::GetActionTargetAccount(char * *aActionTargetAccount)
{
  NS_ENSURE_ARG_POINTER(aActionTargetAccount);
  *aActionTargetAccount = ToNewCString(mActionTargetAccount);
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetActionTargetAccount(const char * aActionTargetAccount)
{
  mActionTargetAccount = aActionTargetAccount;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::GetActionTargetFolder(char * *aActionTargetFolder)
{
  NS_ENSURE_ARG_POINTER(aActionTargetFolder);
  *aActionTargetFolder = ToNewCString(mActionTargetFolder);
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetActionTargetFolder(const char * aActionTargetFolder)
{
  mActionTargetFolder = aActionTargetFolder;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::GetPurgeInterval(PRInt32 *aPurgeInterval)
{
  NS_ENSURE_ARG_POINTER(aPurgeInterval);
  *aPurgeInterval = mPurgeInterval;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetPurgeInterval(PRInt32 aPurgeInterval)
{
  NS_ASSERTION(aPurgeInterval >= 0, "bad purge interval");
  mPurgeInterval = aPurgeInterval;
  return NS_OK;
}

NS_IMETHODIMP
nsSpamSettings::SetLogStream(nsIOutputStream *aLogStream)
{
  // if there is a log stream already, close it
  if (mLogStream) {
    // will flush
    nsresult rv = mLogStream->Close();
    NS_ENSURE_SUCCESS(rv,rv);
  }

  mLogStream = aLogStream;
  return NS_OK;
}

NS_IMETHODIMP
nsSpamSettings::GetLogStream(nsIOutputStream **aLogStream)
{
  NS_ENSURE_ARG_POINTER(aLogStream);

  nsresult rv;

  if (!mLogStream) {
    nsCOMPtr <nsIFileSpec> file;
    rv = GetLogFileSpec(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv,rv);

    nsXPIDLCString nativePath;
    rv = file->GetNativePath(getter_Copies(nativePath));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr <nsILocalFile> logFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = logFile->InitWithNativePath(nsDependentCString(nativePath));
    NS_ENSURE_SUCCESS(rv,rv);

    // append to the end of the log file
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(mLogStream),
                                   logFile,
                                   PR_CREATE_FILE | PR_WRONLY | PR_APPEND,
                                   0600);
    NS_ENSURE_SUCCESS(rv,rv);

    if (!mLogStream)
      return NS_ERROR_FAILURE;
  }
 
  NS_ADDREF(*aLogStream = mLogStream);
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::GetServer(nsIMsgIncomingServer **aServer)
{
  NS_ENSURE_ARG_POINTER(aServer);
  NS_IF_ADDREF(*aServer = mServer);
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::SetServer(nsIMsgIncomingServer *aServer)
{
  mServer = aServer;
  return NS_OK;
}

nsresult
nsSpamSettings::GetLogFileSpec(nsIFileSpec **aFileSpec)
{
  NS_ENSURE_ARG_POINTER(aFileSpec);

  nsCOMPtr <nsIMsgIncomingServer> server;
  nsresult rv = GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv,rv);

  NS_ASSERTION(server, "are you trying to use a cloned spam settings?");
  if (!server)
    return NS_ERROR_FAILURE;

  rv = server->GetLocalPath(aFileSpec);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = (*aFileSpec)->AppendRelativeUnixPath("junklog.html");
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::GetLogURL(char * *aLogURL)
{
  NS_ENSURE_ARG_POINTER(aLogURL);

  nsCOMPtr <nsIFileSpec> file;
  nsresult rv = GetLogFileSpec(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv,rv);
  
  rv = file->GetURLString(aLogURL);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

nsresult nsSpamSettings::TruncateLog()
{
  // this will flush and close the steam
  nsresult rv = SetLogStream(nsnull);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIFileSpec> file;
  rv = GetLogFileSpec(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = file->Truncate(0);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

NS_IMETHODIMP nsSpamSettings::ClearLog()
{
  PRBool loggingEnabled = mLoggingEnabled;
  
  // disable logging while clearing
  mLoggingEnabled = PR_FALSE;

  nsresult rv = TruncateLog();
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to truncate filter log");

  mLoggingEnabled = loggingEnabled;
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::EnsureLogFile()
{
  nsCOMPtr <nsIFileSpec> file;
  nsresult rv = GetLogFileSpec(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv,rv);

  PRBool exists;
  rv = file->Exists(&exists);
  if (NS_SUCCEEDED(rv) && !exists) {
    rv = file->Touch();
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

NS_IMETHODIMP nsSpamSettings::FlushLogIfNecessary()
{
  // only flush the log if we are logging
  PRBool loggingEnabled = PR_FALSE;
  nsresult rv = GetLoggingEnabled(&loggingEnabled);
  NS_ENSURE_SUCCESS(rv,rv);

  if (loggingEnabled) 
  {
    nsCOMPtr <nsIOutputStream> logStream;
    rv = GetLogStream(getter_AddRefs(logStream));    
    if (NS_SUCCEEDED(rv) && logStream) {
      rv = logStream->Flush();
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }
  return rv;
}

NS_IMETHODIMP nsSpamSettings::Clone(nsISpamSettings *aSpamSettings)
{
  NS_ENSURE_ARG_POINTER(aSpamSettings);

  nsresult rv = aSpamSettings->GetUseWhiteList(&mUseWhiteList); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aSpamSettings->GetMoveOnSpam(&mMoveOnSpam); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aSpamSettings->GetPurge(&mPurge); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aSpamSettings->GetPurgeInterval(&mPurgeInterval); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aSpamSettings->GetLevel(&mLevel); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = aSpamSettings->GetMoveTargetMode(&mMoveTargetMode); 
  NS_ENSURE_SUCCESS(rv,rv);

  nsXPIDLCString actionTargetAccount;
  rv = aSpamSettings->GetActionTargetAccount(getter_Copies(actionTargetAccount)); 
  NS_ENSURE_SUCCESS(rv,rv);
  mActionTargetAccount = actionTargetAccount;

  nsXPIDLCString actionTargetFolder;
  rv = aSpamSettings->GetActionTargetFolder(getter_Copies(actionTargetFolder)); 
  NS_ENSURE_SUCCESS(rv,rv);
  mActionTargetFolder = actionTargetFolder;

  nsXPIDLCString whiteListAbURI;
  rv = aSpamSettings->GetWhiteListAbURI(getter_Copies(whiteListAbURI)); 
  NS_ENSURE_SUCCESS(rv,rv);
  mWhiteListAbURI = whiteListAbURI;
  
  PRBool loggingEnabled;
  rv = aSpamSettings->GetLoggingEnabled(&loggingEnabled);
  NS_ENSURE_SUCCESS(rv,rv);
  mLoggingEnabled = loggingEnabled;
  return rv;
}

NS_IMETHODIMP nsSpamSettings::GetSpamFolderURI(char **aSpamFolderURI)
{
  NS_ENSURE_ARG_POINTER(aSpamFolderURI);
  nsresult rv;

  if (mMoveTargetMode == MOVE_TARGET_MODE_FOLDER)
    return GetActionTargetFolder(aSpamFolderURI);

  NS_ASSERTION(mMoveTargetMode == MOVE_TARGET_MODE_JUNK_ON_ACCOUNT, "bad mode");

  // if the mode is MOVE_TARGET_MODE_JUNK_ON_ACCOUNT
  // the spam folder URI = account uri + "/Junk"
  nsXPIDLCString folderURI;
  rv = GetActionTargetAccount(getter_Copies(folderURI));
  NS_ENSURE_SUCCESS(rv,rv);

  // see nsMsgFolder::SetPrettyName() for where the pretty name is set.
  folderURI.Append("/Junk");
  *aSpamFolderURI = ToNewCString(folderURI);
  if (!*aSpamFolderURI)
    return NS_ERROR_OUT_OF_MEMORY;
  else 
    return rv;
}


#define LOG_ENTRY_START_TAG "<p>\n"
#define LOG_ENTRY_START_TAG_LEN (strlen(LOG_ENTRY_START_TAG))
#define LOG_ENTRY_END_TAG "</p>\n"
#define LOG_ENTRY_END_TAG_LEN (strlen(LOG_ENTRY_END_TAG))

NS_IMETHODIMP nsSpamSettings::LogJunkHit(nsIMsgDBHdr *aMsgHdr, PRBool aMoveMessage)
{
  PRBool loggingEnabled;
  nsresult rv = GetLoggingEnabled(&loggingEnabled);
  NS_ENSURE_SUCCESS(rv,rv);

  if (!loggingEnabled)
    return NS_OK;

  nsCOMPtr <nsIOutputStream> logStream;
  rv = GetLogStream(getter_AddRefs(logStream));
  NS_ENSURE_SUCCESS(rv,rv);
  
  PRTime date;
  char dateStr[40];	/* 30 probably not enough */
  
  nsXPIDLCString author;
  nsXPIDLCString subject;
  
  rv = aMsgHdr->GetDate(&date);
  PRExplodedTime exploded;
  PR_ExplodeTime(date, PR_LocalTimeParameters, &exploded);
  PR_FormatTimeUSEnglish(dateStr, 100, "%m/%d/%Y %I:%M %p", &exploded);
  
  aMsgHdr->GetAuthor(getter_Copies(author));
  aMsgHdr->GetSubject(getter_Copies(subject));
  
  nsCString buffer;
  // this is big enough to hold a log entry.  
  // do this so we avoid growing and copying as we append to the log.
  buffer.SetCapacity(512);  
  
  buffer = "Detected junk message from ";
  buffer +=  (const char*)author;
  buffer +=  " - ";
  buffer +=  (const char *)subject;
  buffer +=  " at ";
  buffer +=  dateStr;
  buffer +=  "\n";
  
  if (aMoveMessage) {
    nsXPIDLCString msgId;
    aMsgHdr->GetMessageId(getter_Copies(msgId));
    
    nsXPIDLCString junkFolderURI;
    GetSpamFolderURI(getter_Copies(junkFolderURI));
    
    buffer += "Move message id = ";
    buffer += msgId.get();
    buffer += " to ";
    buffer += junkFolderURI.get();
    buffer += "\n";
  }
  
  PRUint32 writeCount;
  
  rv = logStream->Write(LOG_ENTRY_START_TAG, LOG_ENTRY_START_TAG_LEN, &writeCount);
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ASSERTION(writeCount == LOG_ENTRY_START_TAG_LEN, "failed to write out start log tag");
  
  // html escape the log for security reasons.
  // we don't want some to send us a message with a subject with
  // html tags, especially <script>
  char *escapedBuffer = nsEscapeHTML(buffer.get());
  if (!escapedBuffer)
    return NS_ERROR_OUT_OF_MEMORY;
  
  PRUint32 escapedBufferLen = strlen(escapedBuffer);
  rv = logStream->Write(escapedBuffer, escapedBufferLen, &writeCount);
  PR_FREEIF(escapedBuffer);
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ASSERTION(writeCount == escapedBufferLen, "failed to write out log hit");
  
  rv = logStream->Write(LOG_ENTRY_END_TAG, LOG_ENTRY_END_TAG_LEN, &writeCount);
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ASSERTION(writeCount == LOG_ENTRY_END_TAG_LEN, "failed to write out end log tag");
  return NS_OK;
}

