/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*
 * capiredr.h
 * John Sun
 * 4/16/98 3:31:51 PM
 */
#ifndef __NSQQnsCapiCallbackReader_H_
#define __NSQQnsCapiCallbackReader_H_

#include "nscapiexport.h"
#include "jdefines.h"
#include <unistring.h>
#include "ptrarray.h"
#include "icalredr.h"
#include "prmon.h"
#include "jutility.h"

CLASS_EXPORT_CAPI nsCapiBufferStruct
{
public:
    char * m_pBuf;
    size_t m_pBufSize;
};

/**
 *  nsCapiCallbackReader is a subclass of ICalReader.  It implements
 *  the ICalReader interface to work with CAPI callback method
 *  to parse from stream.
 *  Uses multiple threads.
 */
CLASS_EXPORT_CAPI nsCapiCallbackReader
{
private:
    /*-----------------------------
    ** MEMBERS
    **---------------------------*/
    /* current buffer of iCal information,
     * when CAPI has information to return to
     * the buffer, it must append to this buffer
     * when no more full-lines can be made from the
     * buffer, block on the monitor.  Will be
     * notified when CAPI gets back more information
     */
    char * m_Buffer;
    t_int32 m_BufferSize;
    t_bool m_Init;
    t_int32 m_Mark;
    t_int32 m_ChunkMark;
    t_int32 m_Pos;
    t_int32 m_ChunkIndex;

    /** encoding of stream */
    JulianUtility::MimeEncoding m_Encoding;

    JulianPtrArray * m_Chunks;

    static const t_int32 m_MAXBUFFERSIZE;
    static const t_int32 m_NOMORECHUNKS;

    /* finished getting input from CAPI callback */
    t_bool m_bFinished;

    PRMonitor * m_Monitor;
    /*-----------------------------
    ** PRIVATE METHODS
    **---------------------------*/

    UnicodeString & createLine(t_int32 oldPos, t_int32 oldChunkIndex,
        t_int32 newPos, t_int32 newChunkIndex, UnicodeString & aLine);

    static void deleteUnicodeStringVector(JulianPtrArray * stringVector);

    nsCapiCallbackReader();
public:
    /*-----------------------------
    ** CONSTRUCTORS and DESTRUCTORS
    **---------------------------*/
    nsCapiCallbackReader(PRMonitor * monitor,
        JulianUtility::MimeEncoding encoding = JulianUtility::MimeEncoding_7bit);
    ~nsCapiCallbackReader();

    /*-----------------------------
    ** ACCESSORS (GET AND SET)
    **---------------------------*/

    virtual void * getMonitor() { return m_Monitor; }

    void setFinished() { m_bFinished = TRUE; }
    void setEncoding(JulianUtility::MimeEncoding encoding) { m_Encoding = encoding; }
    t_bool isFinished() const { return m_bFinished; }
    /**
     * Sets a the buffer to read from.
     * Appends the m_Buffer.
     */
    /*virtual void setBuffer(const char * capiChunk);*/


    /**
     * Don't delete u until this object is deleted.
     * @param           UnicodeString * u
     *
     * @return          void
     */
    void AddChunk(UnicodeString * u);

    /** buffer to contain current line, assumed to be less than 1024 bytes */
    char m_pBuffer[1024];
    void AddBuffer(nsCapiBufferStruct * cBuf);
    /*-----------------------------
    ** UTILITIES
    **---------------------------*/

    void mark() { m_Mark = m_Pos; m_ChunkMark = m_ChunkIndex;}

    void reset() { m_Pos = m_Mark; m_ChunkIndex = m_ChunkMark; m_Mark = -1; m_ChunkMark = -1; }

    /**
     * Read next character from file.
     *
     * @param           status, return 1 if no more characters
     * @return          next character of string
     */
    virtual t_int8 read(ErrorCode & status);



    /**
     * Read the next ICAL full line of the file.  The definition
     * of a full ICAL line can be found in the ICAL spec.
     * Basically, a line followed by a CRLF and a space character
     * signifies that the next line is a continuation of the previous line.
     * Uses the readLine, read methods.
     *
     * @param           aLine, returns next full line of string
     * @param           status, return 1 if no more lines
     *
     * @return          next full line of string
     */
    virtual UnicodeString & readFullLine(UnicodeString & aLine, ErrorCode & status, t_int32 i = 0);

    /**
     * Read next line of file.  A line is defined to be
     * characters terminated by either a '\n', '\r' or "\r\n".
     * @param           aLine, return next line of string
     * @param           status, return 1 if no more lines
     *
     * @return          next line of string
     */
    virtual UnicodeString & readLine(UnicodeString & aLine, ErrorCode & status);
public:

    /*virtual UnicodeString & readLineZero(UnicodeString & aLine, ErrorCode & status);*/



};

#endif /* __NSQQnsCapiCallbackReader_H_ */

