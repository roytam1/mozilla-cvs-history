/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
package netscape.ldap.util;

import java.util.*;
import netscape.ldap.client.*;
import java.io.*;

/**
 * An object of this class represents an LDIF record in an LDIF
 * file (or in LDIF data). A record can contain a list of attributes
 * (which describes an entry) or a list of modifications (which
 * decribes the changes that need to be made to an entry).
 * Each record also has a distinguished name.
 * <P>
 *
 * You can get an <CODE>LDIFRecord</CODE> object from LDIF data
 * by calling the <CODE>nextRecord</CODE> method of the
 * <CODE>LDIF</CODE> object holding the data.
 * <P>
 *
 * If you are constructing a new <CODE>LDIFRecord</CODE> object,
 * you can specify the content of the record in one of the
 * following ways:
 * <P>
 *
 * <UL>
 * <LI>To create a record that specifies an entry, use an object of
 * the <CODE>LDIFAttributeContent</CODE> class.
 * <LI>To create a record that specifies a modification to be made,
 * use an object of one of the following classes:
 * <UL>
 * <LI>Use <CODE>LDIFAddContent</CODE> to add a new entry.
 * <LI>Use <CODE>LDIFModifyContent</CODE> to modify an entry.
 * <LI>Use <CODE>LDIFDeleteContent</CODE> to delete an entry.
 * </UL>
 * </UL>
 * <P>
 *
 * @version 1.0
 * @see netscape.ldap.util.LDIF
 * @see netscape.ldap.util.LDIFAddContent
 * @see netscape.ldap.util.LDIFModifyContent
 * @see netscape.ldap.util.LDIFDeleteContent
 * @see netscape.ldap.util.LDIFAttributeContent
 */
public class LDIFRecord {

    /**
     * Internal variables
     */
    private String m_dn = null;
    private LDIFContent m_content = null;

    /**
     * Constructs a new <CODE>LDIFRecord</CODE> object with the
     * specified content.
     * @param dn Distinguished name of the entry associated with
     * the record.
     * @param content Content of the LDIF record.  You can specify
     * an object of the <CODE>LDIFAttributeContent</CODE>,
     * <CODE>LDIFAddContent</CODE>, <CODE>LDIFModifyContent</CODE>,
     * or <CODE>LDIFDeleteContent</CODE> classes.
     * @see netscape.ldap.util.LDIFAddContent
     * @see netscape.ldap.util.LDIFModifyContent
     * @see netscape.ldap.util.LDIFDeleteContent
     * @see netscape.ldap.util.LDIFAttributeContent
     */
    public LDIFRecord(String dn, LDIFContent content) {
        m_dn = dn;
        m_content = content;
    }

    /**
     * Retrieves the distinguished name of the LDIF record.
     * @return The distinguished name of the LDIF record.
     */
    public String getDN() {
        return m_dn;
    }

    /**
     * Retrieves the content of the LDIF record.  The content is
     * an object of the <CODE>LDIFAttributeContent</CODE>,
     * <CODE>LDIFAddContent</CODE>, <CODE>LDIFModifyContent</CODE>,
     * or <CODE>LDIFDeleteContent</CODE> classes.
     * <P>
     *
     * To determine the class of the object, use the <CODE>getType</CODE>
     * method of that object.  <CODE>getType</CODE> returns one of
     * the following values:
     * <UL>
     * <LI><CODE>LDIFContent.ATTRIBUTE_CONTENT</CODE> (the object is an
     * <CODE>LDIFAttributeContent</CODE> object)
     * <LI><CODE>LDIFContent.ADD_CONTENT</CODE> (the object is an
     * <CODE>LDIFAddContent</CODE> object)
     * <LI><CODE>LDIFContent.MODIFICATION_CONTENT</CODE> (the object is an
     * <CODE>LDIFModifyContent</CODE> object)
     * <LI><CODE>LDIFContent.DELETE_CONTENT</CODE> (the object is an
     * <CODE>LDIFDeleteContent</CODE> object)
     * </UL>
     * <P>
     *
     * For example:
     * <PRE>
     * ...
     * import netscape.ldap.*;
     * import netscape.ldap.util.*;
     * import java.io.*;
     * import java.util.*;
     * ...
     *     try {
     *         // Parse the LDIF file test.ldif.
     *         LDIF parser = new LDIF( "test.ldif" );
     *         // Iterate through each LDIF record in the file.
     *         LDIFRecord nextRec = parser.nextRecord();
     *         while ( nextRec != null ) {
     *             // Based on the type of content in the record,
     *             // get the content and cast it as the appropriate
     *             // type.
     *             switch( nextRec.getContent().getType() ) {
     *                 case LDIFContent.ATTRIBUTE_CONTENT:
     *                     LDIFAttributeContent attrContent =
     *                         (LDIFAttributeContent)nextRec.getContent();
     *                     break;
     *                 case LDIFContent.ADD_CONTENT:
     *                     LDIFAddContent addContent =
     *                         (LDIFAddContent)nextRec.getContent();
     *                     break;
     *                 case LDIFContent.MODIFICATION_CONTENT:
     *                     LDIFModifyContent modifyContent =
     *                         (LDIFModifyContent)nextRec.getContent();
     *                     break;
     *                 case LDIFContent.DELETE_CONTENT:
     *                     LDIFDeleteContent deleteContent =
     *                         (LDIFDeleteContent)nextRec.getContent();
     *                     break;
     *             }
     *             ...
     *             // Iterate through each record.
     *             nextRec = parser.nextRecord();
     *         }
     *     } catch ( IOException e ) {
     *         System.out.println( "Error: " + e.toString() );
     *         System.exit(1);
     *     }
     * ...
     * </PRE>
     *
     * @return The content of the LDIF record.
     * @see netscape.ldap.util.LDIFAddContent
     * @see netscape.ldap.util.LDIFModifyContent
     * @see netscape.ldap.util.LDIFDeleteContent
     * @see netscape.ldap.util.LDIFAttributeContent
     */
    public LDIFContent getContent() {
        return m_content;
    }

    /**
     * Gets the string representation of the <CODE>LDIFRecord</CODE>
     * object.
     * @return The string representation of the LDIF record.
     */
    public String toString() {
        return "LDIFRecord {dn=" + m_dn + ", content=" + m_content + "}";
    }
}
