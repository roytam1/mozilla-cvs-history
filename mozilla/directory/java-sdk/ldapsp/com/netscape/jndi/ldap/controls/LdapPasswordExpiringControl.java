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
package com.netscape.jndi.ldap.controls;

import javax.naming.ldap.Control;
import netscape.ldap.controls.*;

/**
 * Represents an LDAP v3 server control that may be returned if a
 * password is about to expire, and password policy is enabled on the server.
 * The OID for this control is 2.16.840.1.113730.3.4.5.
 * <P>
 */
public class LdapPasswordExpiringControl extends LDAPPasswordExpiringControl implements Control {

    //number of seconds before password expiration
    int m_secondsToExpire = -1;
    
    /**
     * This constractor is used by the NetscapeControlFactory
     */
    LdapPasswordExpiringControl(boolean critical, byte[] value) throws Exception{
        m_value = value;
        m_critical = critical;
        String msg = new String(m_value, "UTF8");
        if (msg != null) {
            m_secondsToExpire = Integer.parseInt(msg);
        }    
    }
    
    /**
     * Return parsed number of seconds before password expires
     */
    public int getSecondsToExipre() {
        return m_secondsToExpire;
    }    

    /**
     * Implements Control interface
     */
    public byte[] getEncodedValue() {
        return getValue();
    }
}
