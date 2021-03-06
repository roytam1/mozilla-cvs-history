/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.jss.pkix.cmc;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import org.mozilla.jss.asn1.*;
import org.mozilla.jss.pkix.primitive.*;
import org.mozilla.jss.util.Assert;

/**
 * A ResponseBody for CMC full enrollment request.
 *  ResponseBody ::= SEQUENCE { 
 *        controlSequence    SEQUENCE SIZE(0..MAX) OF TaggedAttribute, 
 *        cmsSequence        SEQUENCE SIZE(0..MAX) OF TaggedContentInfo, 
 *        otherMsgSequence   SEQUENCE SIZE(0..MAX) OF OtherMsg 
 *  } 
 */
public class ResponseBody implements ASN1Value {

    ///////////////////////////////////////////////////////////////////////
    // Members
    ///////////////////////////////////////////////////////////////////////
    private SEQUENCE sequence;
    private SEQUENCE controlSequence;
	private SEQUENCE cmsSequence;
	private SEQUENCE otherMsgSequence;

    ///////////////////////////////////////////////////////////////////////
    // Construction
    ///////////////////////////////////////////////////////////////////////

    // no default constructor
    private ResponseBody() { }

    /** 
     * Constructs a ResponseBody from its components.
     *
     * @param controlSequence Sequence of TagggedAttribute.
     * @param cmsSequence Sequence of TagggedContentInfo.
     * @param otherMsgSequence Sequence of OtherMsg.
     */
    public ResponseBody(SEQUENCE controlSequence, SEQUENCE
			cmsSequence, SEQUENCE otherMsgSequence) {
        sequence = new SEQUENCE();
        this.controlSequence = controlSequence;
        sequence.addElement(controlSequence);
        this.cmsSequence = cmsSequence;
        sequence.addElement(cmsSequence);
        this.otherMsgSequence = otherMsgSequence;
        sequence.addElement(otherMsgSequence);
    }


    ///////////////////////////////////////////////////////////////////////
    // accessors
    ///////////////////////////////////////////////////////////////////////

    public SEQUENCE getControlSequence() {
        return controlSequence;
    }

    public SEQUENCE getCmsSequence() {
        return cmsSequence;
    }

    public SEQUENCE getOtherMsgSequence() {
        return otherMsgSequence;
    }


    ///////////////////////////////////////////////////////////////////////
    // DER encoding/decoding
    ///////////////////////////////////////////////////////////////////////
    static Tag TAG = SEQUENCE.TAG;
    public Tag getTag() {
        return TAG;
    }

    public void encode(OutputStream ostream) throws IOException {
        encode(TAG, ostream);
    }

    public void encode(Tag implicitTag, OutputStream ostream)
        throws IOException
    {
        sequence.encode(implicitTag, ostream);
    }

    private static final Template templateInstance = new Template();
    public static Template getTemplate() {
        return templateInstance;
    }

    /**
     * A template for decoding an ResponseBody from its BER encoding.
     */
    public static class Template implements ASN1Template {
        private SEQUENCE.Template seqt;

        public Template() {
            seqt = new SEQUENCE.Template();
            seqt.addElement(new SEQUENCE.OF_Template(TaggedAttribute.getTemplate()) );
            seqt.addElement(new SEQUENCE.OF_Template(ANY.getTemplate()) );
            seqt.addElement(new SEQUENCE.OF_Template(ANY.getTemplate()) );
        }

        public boolean tagMatch(Tag tag) {
            return TAG.equals(tag);
        }

        public ASN1Value decode(InputStream istream)
            throws InvalidBERException, IOException
        {
            return decode(TAG, istream);
        }

        public ASN1Value decode(Tag implicitTag, InputStream istream)
            throws InvalidBERException, IOException
        {
            SEQUENCE seq = (SEQUENCE) seqt.decode(implicitTag, istream);

            Assert._assert(seq.size() == 3);

            return new ResponseBody(
                            (SEQUENCE)      seq.elementAt(0),
                            (SEQUENCE)      seq.elementAt(1),
                            (SEQUENCE)      seq.elementAt(2));
        }
    }
}

