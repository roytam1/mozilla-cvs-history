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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
package com.netscape.jndi.ldap.controls;

import javax.naming.ldap.Control;
import netscape.ldap.controls.*;

/**
 * Represents an LDAP v3 server control that specifies that you want
 * the server to use the specified DN's identity for this operation.
 * (The OID for this control is 2.16.840.1.113730.3.4.12.)
 * <P>
 *
 * You can include the control in any request by constructing
 * an <CODE>LDAPSearchConstraints</CODE> object and calling the
 * <CODE>setServerControls</CODE> method.  You can then pass this
 * <CODE>LDAPSearchConstraints</CODE> object to the <CODE>search</CODE>
 * or other request method of an <CODE>LDAPConnection</CODE> object.
 * <P>
 *
 * For example:
 * <PRE>
 * ...
 * LDAPConnection ld = new LDAPConnection();
 * try {
 *     // Connect to server.
 *     ld.connect( 3, hostname, portnumber, "", "" );
 *
 *     // Create a "critical" proxied auth server control using
 *     // the DN "uid=charlie,ou=people,o=acme.com".
 *     LDAPProxiedAuthControl ctrl =
 *         new LDAPProxiedAuthControl( "uid=charlie,ou=people,o=acme.com",
 *                                     true );
 *
 *     // Create search constraints to use that control.
 *     LDAPSearchConstraints cons = new LDAPSearchConstraints();
 *     cons.setServerControls( sortCtrl );
 *
 *     // Send the search request.
 *     LDAPSearchResults res = ld.search( "o=Airius.com",
 *        LDAPv3.SCOPE_SUB, "(cn=Barbara*)", null, false, cons );
 *
 *     ...
 *
 * </PRE>
 *
 * <P>
 * @see com.netscape.jndi.ldap.LdapSearchConstraints
 * @see com.netscape.jndi.ldap.LdapSearchConstraints#setServerControls(LDAPControl)
 */
public class LdapProxiedAuthControl extends LDAPProxiedAuthControl implements Control {

    /**
     * Constructs an <CODE>LdapProxiedAuthControl</CODE> object with a
     * DN to use as identity.
     * @param dn DN to use as identity for execution of a request.
     * @param critical <CODE>true</CODE> if the LDAP operation should be
     * discarded when the server does not support this control (in other
     * words, this control is critical to the LDAP operation).
     */
    public LdapProxiedAuthControl( String dn,
                                   boolean critical) {
        super( dn, critical);
    }

    /**
     * Implements Control interface
     */
    public byte[] getEncodedValue() {
        return getValue();
    }    
    
}

