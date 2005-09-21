/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * test_certstore.c
 *
 * Test CertStore Type
 *
 */

#include "testutil.h"
#include "testutil_nss.h"

void *plContext = NULL;

PKIX_Error *testCRLCallback(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        PKIX_List **pCrls,  /* list of PKIX_PL_Crl */
        void *plContext)
{
        return (0);
}

PKIX_Error *testCertCallback(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_List **pCerts,  /* list of PKIX_PL_Cert */
        void *plContext)
{
        return (0);
}

void testCertStore(void)
{
        PKIX_PL_String *dirString = NULL;
        PKIX_CertStore *certStore = NULL;
        PKIX_PL_Object *getCertStoreContext = NULL;
        PKIX_CertStore_CertCallback certCallback = NULL;
        PKIX_CertStore_CRLCallback crlCallback = NULL;
        char *crlDir = "../top/rev_data/crlchecker";

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_String_Create
                                    (PKIX_ESCASCII,
                                    crlDir,
                                    0,
                                    &dirString,
                                    plContext));

        subTest("PKIX_CertStore_Create");
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_Create
                                    (testCertCallback,
                                    testCRLCallback,
                                    (PKIX_PL_Object *) dirString,
                                    PKIX_TRUE,
                                    NULL,
                                    &certStore,
                                    plContext));

        subTest("PKIX_CertStore_GetCertCallback");
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCertCallback
                                    (certStore, &certCallback, plContext));

        if (certCallback != testCertCallback) {
                testError("PKIX_CertStore_GetCertCallback unexpected mismatch");
        }

        subTest("PKIX_CertStore_GetCRLCallback");
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCRLCallback
                                    (certStore, &crlCallback, plContext));

        if (crlCallback != testCRLCallback) {
                testError("PKIX_CertStore_GetCRLCallback unexpected mismatch");
        }

        subTest("PKIX_CertStore_GetCertStoreContext");
        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CertStore_GetCertStoreContext
                (certStore, &getCertStoreContext, plContext));

        if ((PKIX_PL_Object *)dirString != getCertStoreContext) {
            testError("PKIX_CertStore_GetCertStoreContext unexpected mismatch");
        }

cleanup:

        PKIX_TEST_DECREF_AC(dirString);
        PKIX_TEST_DECREF_AC(certStore);
        PKIX_TEST_DECREF_AC(getCertStoreContext);

        PKIX_TEST_RETURN();
}

/* Functional tests for CertStore public functions */

int main(int argc, char *argv[]) {

        PKIX_UInt32 actualMinorVersion;
        PKIX_UInt32 j = 0;

        PKIX_TEST_STD_VARS();

        startTests("CertStore");

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_Initialize
                                    (PKIX_MAJOR_VERSION,
                                    PKIX_MINOR_VERSION,
                                    PKIX_MINOR_VERSION,
                                    &actualMinorVersion,
                                    plContext));

        PKIX_TEST_NSSCONTEXT_SETUP(0x10, argv[1], NULL, &plContext);

        testCertStore();


cleanup:

        PKIX_Shutdown(plContext);

        PKIX_TEST_RETURN();

        endTests("CertStore");

        return (0);
}
