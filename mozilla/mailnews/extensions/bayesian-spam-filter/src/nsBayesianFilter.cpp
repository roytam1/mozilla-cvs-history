/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Patrick C. Beard <beard@netscape.com>
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

#include "nsCRT.h"
#include "nsBayesianFilter.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"
#include "nsQuickSort.h"
#include "nsIProfileInternal.h"
#include "nsIStreamConverterService.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "prnetdb.h"

static const char* kBayesianFilterTokenDelimiters = " \t\n\r\f!\"#%&()*+,./:;<=>?@[\\]^_`{|}~";

struct Token : public PLDHashEntryHdr {
    const char* mWord;
    PRUint32 mLength;
    PRUint32 mCount;            // TODO:  put good/bad count values in same token object.
    double mProbability;        // TODO:  cache probabilities
};

TokenEnumeration::TokenEnumeration(PLDHashTable* table)
    :   mEntrySize(table->entrySize),
        mEntryCount(table->entryCount),
        mEntryOffset(0),
        mEntryAddr(table->entryStore)
{
    PRUint32 capacity = PL_DHASH_TABLE_SIZE(table);
    mEntryLimit = mEntryAddr + capacity * mEntrySize;
}
    
inline bool TokenEnumeration::hasMoreTokens()
{
    return (mEntryOffset < mEntryCount);
}

inline Token* TokenEnumeration::nextToken()
{
    Token* token = NULL;
    PRUint32 entrySize = mEntrySize;
    char *entryAddr = mEntryAddr, *entryLimit = mEntryLimit;
    while (entryAddr < entryLimit) {
        PLDHashEntryHdr* entry = (PLDHashEntryHdr*) entryAddr;
        entryAddr += entrySize;
        if (PL_DHASH_ENTRY_IS_LIVE(entry)) {
            token = NS_STATIC_CAST(Token*, entry);
            ++mEntryOffset;
            break;
        }
    }
    mEntryAddr = entryAddr;
    return token;
}

// PLDHashTable operation callbacks

static const void* PR_CALLBACK GetKey(PLDHashTable* table, PLDHashEntryHdr* entry)
{
    return NS_STATIC_CAST(Token*, entry)->mWord;
}

static PRBool PR_CALLBACK MatchEntry(PLDHashTable* table,
                                     const PLDHashEntryHdr* entry,
                                     const void* key)
{
    const Token* token = NS_STATIC_CAST(const Token*, entry);
    return (strcmp(token->mWord, NS_REINTERPRET_CAST(const char*, key)) == 0);
}

static void PR_CALLBACK MoveEntry(PLDHashTable* table,
                                  const PLDHashEntryHdr* from,
                                  PLDHashEntryHdr* to)
{
    const Token* fromToken = NS_STATIC_CAST(const Token*, from);
    Token* toToken = NS_STATIC_CAST(Token*, to);
    NS_ASSERTION(fromToken->mLength != 0, "zero length token in table!");
    *toToken = *fromToken;
}

static void PR_CALLBACK ClearEntry(PLDHashTable* table, PLDHashEntryHdr* entry)
{
    // We use the mWord field to further indicate liveness when using PL_DHASH_ADD.
    // So we simply clear it when an entry is removed.
    Token* token = NS_STATIC_CAST(Token*, entry);
    token->mWord = NULL;
}

struct VisitClosure {
    bool (*f) (Token*, void*);
    void* data;
};

static PLDHashOperator PR_CALLBACK VisitEntry(PLDHashTable* table, PLDHashEntryHdr* entry,
                                              PRUint32 number, void* arg)
{
    VisitClosure* closure = NS_REINTERPRET_CAST(VisitClosure*, arg);
    Token* token = NS_STATIC_CAST(Token*, entry);
    return (closure->f(token, closure->data) ? PL_DHASH_NEXT : PL_DHASH_STOP);
}

// member variables
static PLDHashTableOps gTokenTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GetKey,
    PL_DHashStringKey,      // Save the extra layer of function call to PL_DHashStringKey.
    MatchEntry,
    MoveEntry,
    ClearEntry,
    PL_DHashFinalizeStub
};

Tokenizer::Tokenizer()
{
    PRBool ok = PL_DHashTableInit(&mTokenTable, &gTokenTableOps, nsnull, sizeof(Token), 256);
    NS_ASSERTION(ok, "mTokenTable failed to initialize");
    PL_INIT_ARENA_POOL(&mWordPool, "Words Arena", 16384);
}

Tokenizer::~Tokenizer()
{
    if (mTokenTable.entryStore)
        PL_DHashTableFinish(&mTokenTable);
    PL_FinishArenaPool(&mWordPool);
}

char* Tokenizer::copyWord(const char* word, PRUint32 len)
{
    void* result;
    PRUint32 size = 1 + len;
    PL_ARENA_ALLOCATE(result, &mWordPool, size);
    if (result)
        memcpy(result, word, size);
    return NS_REINTERPRET_CAST(char*, result);
}

inline Token* Tokenizer::get(const char* word)
{
    PLDHashEntryHdr* entry = PL_DHashTableOperate(&mTokenTable, word, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(entry))
        return NS_STATIC_CAST(Token*, entry);
    return NULL;
}

Token* Tokenizer::add(const char* word, PRUint32 count)
{
    PLDHashEntryHdr* entry = PL_DHashTableOperate(&mTokenTable, word, PL_DHASH_ADD);
    Token* token = NS_STATIC_CAST(Token*, entry);
    if (token) {
        if (token->mWord == NULL) {
            PRUint32 len = strlen(word);
            NS_ASSERTION(len != 0, "adding zero length word to tokenizer");
            token->mWord = copyWord(word, len);
            NS_ASSERTION(token->mWord, "copyWord failed");
            if (!token->mWord) {
                PL_DHashTableRawRemove(&mTokenTable, entry);
                return NULL;
            }
            token->mLength = len;
            token->mCount = count;
            token->mProbability = 0;
        } else {
            token->mCount += count;
        }
    }
    return token;
}

void Tokenizer::remove(const char* word, PRUint32 count)
{
    Token* token = get(word);
    if (token) {
        NS_ASSERTION(token->mCount >= count, "token count underflow");
        if (token->mCount >= count) {
            token->mCount -= count;
            if (token->mCount == 0)
                PL_DHashTableRawRemove(&mTokenTable, token);
        }
    }
}

static bool isDecimalNumber(const char* word)
{
    const char* p = word;
    if (*p == '-') ++p;
    char c;
    while ((c = *p++)) {
        if (!isdigit(c))
            return false;
    }
    return true;
}

inline bool isUpperCase(char c) { return ('A' <= c) && (c <= 'Z'); }

static char* toLowerCase(char* str)
{
    char c, *p = str;
    while ((c = *p++)) {
        if (isUpperCase(c))
            p[-1] = c + ('a' - 'A');
    }
    return str;
}

void Tokenizer::tokenize(char* text)
{
    char* word;
    char* next = text;
    while ((word = nsCRT::strtok(next, kBayesianFilterTokenDelimiters, &next)) != NULL) {
        if (word[0] == '\0') continue;
        if (isDecimalNumber(word)) continue;
        add(toLowerCase(word));
    }
}

void Tokenizer::tokenize(const char* str)
{
    char* text = nsCRT::strdup(str);
    if (text) {
        tokenize(text);
        nsCRT::free(text);
    }
}

void Tokenizer::visit(bool (*f) (Token*, void*), void* data)
{
    VisitClosure closure = { f, data };
    PRUint32 visitCount = PL_DHashTableEnumerate(&mTokenTable, VisitEntry, &closure);
    NS_ASSERTION(visitCount == mTokenTable.entryCount, "visitCount != entryCount!");
}

inline PRUint32 Tokenizer::countTokens()
{
    return mTokenTable.entryCount;
}

Token* Tokenizer::copyTokens()
{
    PRUint32 count = countTokens();
    if (count > 0) {
        Token* tokens = new Token[count];
        if (tokens) {
            Token* tp = tokens;
            TokenEnumeration e(&mTokenTable);
            while (e.hasMoreTokens())
                *tp++ = *e.nextToken();
        }
        return tokens;
    }
    return NULL;
}

inline TokenEnumeration Tokenizer::getTokens()
{
    return TokenEnumeration(&mTokenTable);
}

class TokenAnalyzer {
public:
    virtual ~TokenAnalyzer() {}
    
    virtual void analyzeTokens(const char* source, Tokenizer& tokenizer) = 0;
};

/**
 * This class downloads the raw content of an email message, buffering until
 * complete segments are seen, that is until a linefeed is seen, although
 * any of the valid token separators would do. This could be a further
 * refinement.
 */
class TokenStreamListener : public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    
    TokenStreamListener(const char* tokenSource, TokenAnalyzer* analyzer);
    virtual ~TokenStreamListener();
    
protected:
    nsCString mTokenSource;
    TokenAnalyzer* mAnalyzer;
    char* mBuffer;
    PRUint32 mBufferSize;
    PRUint32 mLeftOverCount;
    Tokenizer mTokenizer;
};

const PRUint32 kBufferSize = 16384;

TokenStreamListener::TokenStreamListener(const char* tokenSource, TokenAnalyzer* analyzer)
    :   mTokenSource(tokenSource), mAnalyzer(analyzer),
        mBuffer(NULL), mBufferSize(kBufferSize), mLeftOverCount(0)
{
    NS_INIT_ISUPPORTS();
}

TokenStreamListener::~TokenStreamListener()
{
    delete[] mBuffer;
    delete mAnalyzer;
}

NS_IMPL_ISUPPORTS2(TokenStreamListener, nsIRequestObserver, nsIStreamListener)

/* void onStartRequest (in nsIRequest aRequest, in nsISupports aContext); */
NS_IMETHODIMP TokenStreamListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
    if (!mTokenizer)
        return NS_ERROR_OUT_OF_MEMORY;
    mBuffer = new char[mBufferSize];
    if (!mBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

/* void onDataAvailable (in nsIRequest aRequest, in nsISupports aContext, in nsIInputStream aInputStream, in unsigned long aOffset, in unsigned long aCount); */
NS_IMETHODIMP TokenStreamListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *aContext, nsIInputStream *aInputStream, PRUint32 aOffset, PRUint32 aCount)
{
    nsresult rv = NS_OK;

    while (aCount > 0) {
        PRUint32 readCount, totalCount = (aCount + mLeftOverCount);
        if (totalCount >= mBufferSize) {
            readCount = mBufferSize - mLeftOverCount - 1;
        } else {
            readCount = aCount;
        }
        
        char* buffer = mBuffer;
        rv = aInputStream->Read(buffer + mLeftOverCount, readCount, &readCount);
        if (NS_FAILED(rv))
            break;

        if (readCount == 0) {
            rv = NS_ERROR_UNEXPECTED;
            NS_WARNING("failed to tokenize");
            break;
        }
            
        aCount -= readCount;
        
        /* consume the tokens up to the last legal token delimiter in the buffer. */
        totalCount = (readCount + mLeftOverCount);
        buffer[totalCount] = '\0';
        char* lastDelimiter = NULL;
        char* scan = buffer + totalCount;
        while (scan > buffer) {
            if (strchr(kBayesianFilterTokenDelimiters, *--scan)) {
                lastDelimiter = scan;
                break;
            }
        }
        
        if (lastDelimiter) {
            *lastDelimiter = '\0';
            mTokenizer.tokenize(buffer);

            PRUint32 consumedCount = 1 + (lastDelimiter - buffer);
            mLeftOverCount = totalCount - consumedCount;
            if (mLeftOverCount)
                memmove(buffer, buffer + consumedCount, mLeftOverCount);
        } else {
            /* didn't find a delimiter, keep the whole buffer around. */
            mLeftOverCount = totalCount;
            if (totalCount >= (mBufferSize / 2)) {
                PRUint32 newBufferSize = mBufferSize * 2;
                char* newBuffer = new char[newBufferSize];
                if (!newBuffer) return NS_ERROR_OUT_OF_MEMORY;
                memcpy(newBuffer, mBuffer, mLeftOverCount);
                delete[] mBuffer;
                mBuffer = newBuffer;
                mBufferSize = newBufferSize;
            }
        }
    }
    
    return rv;
}

/* void onStopRequest (in nsIRequest aRequest, in nsISupports aContext, in nsresult aStatusCode); */
NS_IMETHODIMP TokenStreamListener::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatusCode)
{
    if (mLeftOverCount) {
        /* assume final buffer is complete. */
        char* buffer = mBuffer;
        buffer[mLeftOverCount] = '\0';
        mTokenizer.tokenize(buffer);
    }
    
    /* finally, analyze the tokenized message. */
    if (mAnalyzer)
        mAnalyzer->analyzeTokens(mTokenSource.get(), mTokenizer);
    
    return NS_OK;
}

/* Implementation file */
NS_IMPL_ISUPPORTS2(nsBayesianFilter, nsIMsgFilterPlugin, nsIJunkMailPlugin)

nsBayesianFilter::nsBayesianFilter()
    :   mGoodCount(0), mBadCount(0),
        mBatchLevel(0), mTrainingDataDirty(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
    
    bool ok = (mGoodTokens && mBadTokens);
    NS_ASSERTION(ok, "error allocating tokenizers");
    if (ok)
        readTrainingData();
}

nsBayesianFilter::~nsBayesianFilter() {}

class MessageClassifier : public TokenAnalyzer {
public:
    MessageClassifier(nsBayesianFilter* filter, nsIJunkMailClassificationListener* listener)
        :   mFilter(filter), mSupports(filter), mListener(listener)
    {
    }
    
    virtual void analyzeTokens(const char* source, Tokenizer& tokenizer)
    {
        mFilter->classifyMessage(tokenizer, source, mListener);
    }

private:
    nsBayesianFilter* mFilter;
    nsCOMPtr<nsISupports> mSupports;
    nsCOMPtr<nsIJunkMailClassificationListener> mListener;
};

nsresult nsBayesianFilter::tokenizeMessage(const char* messageURI, TokenAnalyzer* analyzer)
{
    nsresult rv;
    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv); 
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLCString messageURL;
    rv = mailSession->ConvertMsgURIToMsgURL(messageURI, nsnull, getter_Copies(messageURL));
    // Tell mime we just want to scan the message data
    nsCAutoString aUrl(messageURL);
    aUrl.FindChar('?') == kNotFound ? aUrl += "?" : aUrl += "&";
    aUrl += "header=filter";

    nsCOMPtr<nsIChannel> channel;
    rv = ioService->NewChannel(aUrl, NULL, NULL, getter_AddRefs(channel));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIStreamListener> tokenListener = new TokenStreamListener(messageURI, analyzer);
    if (!tokenListener) return NS_ERROR_OUT_OF_MEMORY;

    static NS_DEFINE_CID(kIStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
    nsCOMPtr<nsIStreamConverterService> streamConverter = do_GetService(kIStreamConverterServiceCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStreamListener> conversionListener;
    rv = streamConverter->AsyncConvertData(NS_LITERAL_STRING("message/rfc822").get(),
                                           NS_LITERAL_STRING("*/*").get(),
                                           tokenListener, channel, getter_AddRefs(conversionListener));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = channel->AsyncOpen(conversionListener, NULL);
    return rv;
}

inline double abs(double x) { return (x >= 0 ? x : -x); }

static int compareTokens(const void* p1, const void* p2, void* /* data */)
{
    Token *t1 = (Token*) p1, *t2 = (Token*) p2;
    double delta = abs(t1->mProbability - 0.5) - abs(t2->mProbability - 0.5);
    return (delta == 0.0 ? 0 : (delta > 0.0 ? 1 : -1));
}

inline double max(double x, double y) { return (x > y ? x : y); }
inline double min(double x, double y) { return (x < y ? x : y); }

void nsBayesianFilter::classifyMessage(Tokenizer& tokenizer, const char* messageURI,
                                       nsIJunkMailClassificationListener* listener)
{
    Token* tokens = tokenizer.copyTokens();
    if (!tokens) return;
    
    /* run the kernel of the Graham filter algorithm here. */
    PRUint32 i, count = tokenizer.countTokens();
    double ngood = mGoodCount, nbad = mBadCount;
    for (i = 0; i < count; ++i) {
        Token& token = tokens[i];
        const char* word = token.mWord;
        // ((g (* 2 (or (gethash word good) 0)))
        Token* t = mGoodTokens.get(word);
        double g = 2.0 * ((t != NULL) ? t->mCount : 0);
        // (b (or (gethash word bad) 0)))
        t = mBadTokens.get(word);
        double b = ((t != NULL) ? t->mCount : 0);
        if ((g + b) > 5) {
            // (max .01
            //      (min .99 (float (/ (min 1 (/ b nbad))
            //                         (+ (min 1 (/ g ngood))
            //                            (min 1 (/ b nbad)))))))
            token.mProbability = max(.01,
                                     min(.99,
                                         (min(1.0, (b / nbad)) /
                                              (min(1.0, (g / ngood)) +
                                               min(1.0, (b / nbad))))));
        } else {
            token.mProbability = 0.4;
        }
    }
    
    // sort the array by the distance of the token probabilities from a 50-50 value of 0.5.
    PRUint32 first, last = count;
    if (count > 15) {
        first = count - 15;
        NS_QuickSort(tokens, count, sizeof(Token), compareTokens, NULL);
    } else {
        first = 0;
    }

    double prod1 = 1.0, prod2 = 1.0;
    for (i = first; i < last; ++i) {
        double value = tokens[i].mProbability;
        prod1 *= value;
        prod2 *= (1.0 - value);
    }
    double prob = (prod1 / (prod1 + prod2));
    bool isJunk = (prob >= 0.90);

    delete[] tokens;

    if (listener)
        listener->OnMessageClassified(messageURI, isJunk ? nsMsgJunkStatus(nsIJunkMailPlugin::JUNK) : nsMsgJunkStatus(nsIJunkMailPlugin::GOOD));
}

/* void shutdown (); */
NS_IMETHODIMP nsBayesianFilter::Shutdown()
{
    if (mTrainingDataDirty)
        writeTrainingData();
    return NS_OK;
}

/* readonly attribute boolean shouldDownloadAllHeaders; */
NS_IMETHODIMP nsBayesianFilter::GetShouldDownloadAllHeaders(PRBool *aShouldDownloadAllHeaders)
{
    // bayesian filters work on the whole msg body currently.
    *aShouldDownloadAllHeaders = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsBayesianFilter::StartBatch(void)
{
#ifdef DEBUG_dmose
    printf("StartBatch() entered with mBatchLevel=%d\n", mBatchLevel);
#endif
    ++mBatchLevel;
    return NS_OK;
}

NS_IMETHODIMP nsBayesianFilter::EndBatch(void)
{
#ifdef DEBUG_dmose
    printf("EndBatch() entered with mBatchLevel=%d\n", mBatchLevel);
#endif
    NS_ASSERTION(mBatchLevel > 0, "nsBayesianFilter::EndBatch() called with"
                 " mBatchLevel <= 0");
    --mBatchLevel;
    
    if (!mBatchLevel && mTrainingDataDirty)
        writeTrainingData();
    
    return NS_OK;
}

/* void classifyMessage (in string aMsgURL, in nsIJunkMailClassificationListener aListener); */
NS_IMETHODIMP nsBayesianFilter::ClassifyMessage(const char *aMessageURL, nsIJunkMailClassificationListener *aListener)
{
    TokenAnalyzer* analyzer = new MessageClassifier(this, aListener);
    if (!analyzer) return NS_ERROR_OUT_OF_MEMORY;
    return tokenizeMessage(aMessageURL, analyzer);
}

/* void classifyMessages (in unsigned long aCount, [array, size_is (aCount)] in string aMsgURLs, in nsIJunkMailClassificationListener aListener); */
NS_IMETHODIMP nsBayesianFilter::ClassifyMessages(PRUint32 aCount, const char **aMsgURLs, nsIJunkMailClassificationListener *aListener)
{
    nsresult rv = NS_OK;
    for (PRUint32 i = 0; i < aCount; ++i) {
        rv = ClassifyMessage(aMsgURLs[i], aListener);
        if (NS_FAILED(rv))
            break;
    }
    return rv;
}

class MessageObserver : public TokenAnalyzer {
public:
    MessageObserver(nsBayesianFilter* filter,
                    nsMsgJunkStatus oldClassification,
                    nsMsgJunkStatus newClassification,
                    nsIJunkMailClassificationListener* listener)
        :   mFilter(filter), mSupports(filter), mListener(listener),
            mOldClassification(oldClassification),
            mNewClassification(newClassification)
    {
    }
    
    virtual void analyzeTokens(const char* source, Tokenizer& tokenizer)
    {
        mFilter->observeMessage(tokenizer, source, mOldClassification,
                                mNewClassification, mListener);
    }

private:
    nsBayesianFilter* mFilter;
    nsCOMPtr<nsISupports> mSupports;
    nsCOMPtr<nsIJunkMailClassificationListener> mListener;
    nsMsgJunkStatus mOldClassification;
    nsMsgJunkStatus mNewClassification;
};

static void forgetTokens(Tokenizer& corpus, TokenEnumeration tokens)
{
    while (tokens.hasMoreTokens()) {
        Token* token = tokens.nextToken();
        corpus.remove(token->mWord, token->mCount);
    }
}

static void rememberTokens(Tokenizer& corpus, TokenEnumeration tokens)
{
    while (tokens.hasMoreTokens()) {
        Token* token = tokens.nextToken();
        corpus.add(token->mWord, token->mCount);
    }
}

void nsBayesianFilter::observeMessage(Tokenizer& tokenizer, const char* messageURL,
                                      nsMsgJunkStatus oldClassification, nsMsgJunkStatus newClassification,
                                      nsIJunkMailClassificationListener* listener)
{
    TokenEnumeration tokens = tokenizer.getTokens();
    switch (oldClassification) {
    case nsIJunkMailPlugin::JUNK:
        // remove tokens from junk corpus.
        if (mBadCount > 0) {
            --mBadCount;
            forgetTokens(mBadTokens, tokens);
            mTrainingDataDirty = PR_TRUE;
        }
        break;
    case nsIJunkMailPlugin::GOOD:
        // remove tokens from good corpus.
        if (mGoodCount > 0) {
            --mGoodCount;
            forgetTokens(mGoodTokens, tokens);
            mTrainingDataDirty = PR_TRUE;
        }
        break;
    }
    switch (newClassification) {
    case nsIJunkMailPlugin::JUNK:
        // put tokens into junk corpus.
        ++mBadCount;
        rememberTokens(mBadTokens, tokens);
        mTrainingDataDirty = PR_TRUE;
        break;
    case nsIJunkMailPlugin::GOOD:
        // put tokens into good corpus.
        ++mGoodCount;
        rememberTokens(mGoodTokens, tokens);
        mTrainingDataDirty = PR_TRUE;
        break;
    }
    
    if (listener)
        listener->OnMessageClassified(messageURL, newClassification);

    if (mTrainingDataDirty && !mBatchLevel)
        writeTrainingData();
}

static nsresult getTrainingFile(nsCOMPtr<nsILocalFile>& file)
{
    // should we cache the profile manager's directory?
    nsresult rv;
    nsCOMPtr<nsIProfileInternal> profileManager = do_GetService("@mozilla.org/profile/manager;1", &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsXPIDLString currentProfile;
    rv = profileManager->GetCurrentProfile(getter_Copies(currentProfile));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFile> profileDir;
    rv = profileManager->GetProfileDir(currentProfile.get(), getter_AddRefs(profileDir));
    if (NS_FAILED(rv)) return rv;
    
    rv = profileDir->Append(NS_LITERAL_STRING("training.dat"));
    if (NS_FAILED(rv)) return rv;
    
    file = do_QueryInterface(profileDir, &rv);
    return rv;
}

/*
    Format of the training file for version 1:
    [0xFEEDFACE]
    [number good messages][number bad messages]
    [number good tokens]
    [count][length of word]word
    ...
    [number bad tokens]
    [count][length of word]word
    ...
 */

inline int writeUInt32(FILE* stream, PRUint32 value)
{
    value = PR_htonl(value);
    return fwrite(&value, sizeof(PRUint32), 1, stream);
}

inline int readUInt32(FILE* stream, PRUint32* value)
{
    int n = fread(value, sizeof(PRUint32), 1, stream);
    if (n == 1) {
        *value = PR_ntohl(*value);
    }
    return n;
}

static bool writeTokens(FILE* stream, Tokenizer& tokenizer)
{
    PRUint32 tokenCount = tokenizer.countTokens();
    if (writeUInt32(stream, tokenCount) != 1)
        return false;

    if (tokenCount > 0) {
        TokenEnumeration tokens = tokenizer.getTokens();
        for (PRUint32 i = 0; i < tokenCount; ++i) {
            Token* token = tokens.nextToken();
            if (writeUInt32(stream, token->mCount) != 1)
                break;
            PRUint32 tokenLength = token->mLength;
            if (writeUInt32(stream, tokenLength) != 1)
                break;
            if (fwrite(token->mWord, tokenLength, 1, stream) != 1)
                break;
        }
    }
    
    return true;
}

static bool readTokens(FILE* stream, Tokenizer& tokenizer)
{
    PRUint32 tokenCount;
    if (readUInt32(stream, &tokenCount) != 1)
        return false;

    PRUint32 bufferSize = 4096;
    char* buffer = new char[bufferSize];
    if (!buffer) return false;

    for (PRUint32 i = 0; i < tokenCount; ++i) {
        PRUint32 count;
        if (readUInt32(stream, &count) != 1)
            break;
        PRUint32 size;
        if (readUInt32(stream, &size) != 1)
            break;
        if (size >= bufferSize) {
            delete[] buffer;
            PRUint32 newBufferSize = 2 * bufferSize;
            while (size >= newBufferSize)
                newBufferSize *= 2;
            buffer = new char[newBufferSize];
            if (!buffer) return false;
            bufferSize = newBufferSize;
        }
        if (fread(buffer, size, 1, stream) != 1)
            break;
        buffer[size] = '\0';
        tokenizer.add(buffer, count);
    }
    
    delete[] buffer;
    
    return true;
}

static const char kMagicCookie[] = { '\xFE', '\xED', '\xFA', '\xCE' };

void nsBayesianFilter::writeTrainingData()
{
#ifdef DEBUG_dmose
    printf("writeTrainingData() entered\n");
#endif
    nsCOMPtr<nsILocalFile> file;
    nsresult rv = getTrainingFile(file);
    if (NS_FAILED(rv)) return;
 
    // open the file, and write out training data using fprintf for now.
    FILE* stream;
    rv = file->OpenANSIFileDesc("wb", &stream);
    if (NS_FAILED(rv)) return;

    if (!((fwrite(kMagicCookie, sizeof(kMagicCookie), 1, stream) == 1) &&
          (writeUInt32(stream, mGoodCount) == 1) &&
          (writeUInt32(stream, mBadCount) == 1) &&
           writeTokens(stream, mGoodTokens) &&
           writeTokens(stream, mBadTokens))) {
        NS_WARNING("failed to write training data.");
        fclose(stream);
        // delete the training data file, since it is potentially corrupt.
        file->Remove(PR_FALSE);
    } else {
        fclose(stream);
        mTrainingDataDirty = PR_FALSE;
    }
}

void nsBayesianFilter::readTrainingData()
{
    nsCOMPtr<nsILocalFile> file;
    nsresult rv = getTrainingFile(file);
    if (NS_FAILED(rv)) return;
    
    PRBool exists;
    rv = file->Exists(&exists);
    if (NS_FAILED(rv) || !exists) return;

    // open the file, and write out training data using fprintf for now.
    FILE* stream;
    rv = file->OpenANSIFileDesc("rb", &stream);
    if (NS_FAILED(rv)) return;

    // FIXME:  should make sure that the tokenizers are empty.
    char cookie[4];
    if (!((fread(cookie, sizeof(cookie), 1, stream) == 1) &&
          (memcmp(cookie, kMagicCookie, sizeof(cookie)) == 0) &&
          (readUInt32(stream, &mGoodCount) == 1) &&
          (readUInt32(stream, &mBadCount) == 1) &&
           readTokens(stream, mGoodTokens) &&
           readTokens(stream, mBadTokens))) {
        NS_WARNING("failed to read training data.");
    }
    
    fclose(stream);
}

/* void setMessageClassification (in string aMsgURL, in long aOldClassification, in long aNewClassification); */
NS_IMETHODIMP nsBayesianFilter::SetMessageClassification(const char *aMsgURL,
                                                         nsMsgJunkStatus aOldClassification,
                                                         nsMsgJunkStatus aNewClassification,
                                                         nsIJunkMailClassificationListener *aListener)
{
    MessageObserver* analyzer = new MessageObserver(this, aOldClassification, aNewClassification, aListener);
    if (!analyzer) return NS_ERROR_OUT_OF_MEMORY;
    return tokenizeMessage(aMsgURL, analyzer);
}
