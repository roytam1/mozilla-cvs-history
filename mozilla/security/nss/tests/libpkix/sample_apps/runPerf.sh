#! /bin/ksh
# 
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Netscape security libraries.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1994-2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
#
# runPerf.sh
#

### when the script is exiting, handle it in the Cleanup routine...the result
### value will get set to 0 if all the tests completed successfully, so we can
### use that value in the handler
trap 'Cleanup' EXIT
result=1
checkmem=0
arenas=0
typeset -i combinedErrors=0
typeset -i totalErrors=0
typeset -i errors=0
prematureTermination=0

### setup some defaults
WD=`pwd`
prog=`basename $0`
testOut=${WD}/${prog}.$$
testOutMem=${WD}/${prog}_mem.$$

### setup NIST files need to link in
linkNistFiles="ValidCertificatePathTest1EE.crt"

####################
# cleanup from tests
####################
function Cleanup
{
    if [[ ${testOut} != "" ]]; then
        rm -f ${testOut}
    fi

    if [[ ${testOutMem} != "" ]]; then
        rm -f ${testOutMem}
    fi

    for i in ${linkNistFiles}; do
        if [[ -f ./$i ]]; then
            rm ./$i
        fi
    done
}

function Display # string
{
    if [[ ${quiet} -eq 0 ]]; then
        echo "$1"
    fi
}

if [ -z "${INIT_SOURCED}" ] ; then
    curdir=`pwd`
    cd ../../common
    . ./init.sh > /dev/null
    cd ${curdir}
fi

Display ""
Display "*******************************************************************************"
Display "START OF PKIX PERFORMANCE"
Display "*******************************************************************************"

DIST_BIN=${DIST}/${OBJDIR}/bin
echo "\nRunning executibles at ${DIST_BIN}"
echo "Using libraries at ${LD_LIBRARY_PATH}"

### ParseArgs
function ParseArgs # args
{
    while [[ $# -gt 0 ]]; do
        if [[ $1 = "-checkmem" ]]; then
            checkmem=1
        elif [[ $1 = "-quiet" ]]; then
            quiet=1
        elif [[ $1 = "-arenas" ]]; then
            echo "option -arenas is not supported with performance tests"
            arenas=1
        fi
        shift
    done

    memText=""
    if [[ ${checkmem} -eq 1 ]]; then
            memText="   (Memory Checking Enabled)"
    fi

}


#
# Any test that use NIST files should have a tag of NIST-Test-Files-Used
# at the command option so if there is no NIST files installed in the system,
# the test can be skipped
#
if [ -z "${NIST_FILES_DIR}" ] ; then
    Display "\n*******************************************************************************"
    Display "NIST_FILES_DIR is not set but we need NIST file to run the"
    Display "performance for this nss db."
    Display "Set NIST_FILES_DIR to where NIST Certificates and CRLs located"
    Display "to enable tests at this directory"
    Display "Now exiting..."
    exit
else

    NIST=${NIST_FILES_DIR}

    for i in ${linkNistFiles}; do
        if [[ -f ./$i ]]; then
            rm ./$i
        fi
        ln -s ${NIST_FILES_DIR}/$i ./$i
    done
fi


# Check the performance data ...
function perfTest
{
    Display ""
    Display "*******************************************************************************"
    Display "START OF PKIX PERFORMANCE SCENAROS ${memText}"
Display "*******************************************************************************"
    Display ""

    while read -r perfPgm args; do
        Display "Running ${perfPgm} ${args}"
        if [[ ${checkmem} -eq 1 ]]; then
            dbx -C -c "runargs $args; check -all ;run;exit" ${DIST_BIN}/${perfPgm} > ${testOut} 2>&1
        else
            ${DIST_BIN}/${perfPgm} ${args} > ${testOut} 2>&1
        fi

        # Examine output file to see if test failed and keep track of number
        # of failures and names of failed tests. This assumes that the test
        # uses our utility library for displaying information

        cat ${testOut} | grep "per second"

        outputCount=`cat ${testOut} | grep "per second"`

        if [[ $? -ne 0 || ${outputCount} == "" ]]; then
            errors=`expr ${errors} + 1`
            failedpgms="${failedpgms}${perfPgm} ${args}\n"
            cat ${testOut}
        fi

        if [[ ${checkmem} -eq 1 ]]; then
            grep "(actual leaks:" ${testOut} > ${testOutMem} 2>&1
            if [[ $? -ne 0 ]]; then
                prematureErrors=`expr ${prematureErrors} + 1`
                failedprematurepgms="${failedprematurepgms}${perfPgm} "
                Display "...program terminated prematurely (unable to check for memory leak errors) ..."
            else
                grep  "(actual leaks:         1  total size:       4 bytes)" ${testOut} > /dev/null 2>&1
                if [[ $? -ne 0 ]]; then
                    memErrors=`expr ${memErrors} + 1`
                    failedmempgms="${failedmempgms}${perfPgm} "
                    cat ${testOutMem}
                fi
            fi
        fi
    done <<EOF
libpkix_buildthreads 5 1 ValidCertificatePathTest1EE
libpkix_buildthreads 5 8 ValidCertificatePathTest1EE
nss_threads 5 1 ValidCertificatePathTest1EE
nss_threads 5 8 ValidCertificatePathTest1EE
EOF
    return ${errors}

}


# If there is race condition bug, may this test catch it...
function loopTest
{
    typeset -i totalLoop=10

    Display ""
    Display "*******************************************************************************"
    Display "START OF TESTS FOR PKIX PERFORMANCE SANITY LOOP (${totalLoop} times)"
Display "*******************************************************************************"
    Display ""

    typeset -i iLoop=0
    perfPgm="${DIST_BIN}/libpkix_buildthreads 5 8 ValidCertificatePathTest1EE"

    while [[ $iLoop -lt $totalLoop ]]
    do
        iLoop=iLoop+1

        Display "Running ${perfPgm}"
        ${perfPgm} > ${testOut} 2>&1
        cat ${testOut} | grep "per second"

        outputCount=`cat ${testOut} | grep "per second"`

        if [[ $? -ne 0 || ${outputCount} == "" ]]; then
            errors=`expr ${errors} + 1`
            failedpgms="${failedpgms} ${perfPgm}\n"
            cat ${testOut}
        fi
    done

    return ${errors}

}

#main

ParseArgs $*
perfTest
totalErrors=$?
loopTest
totalErrors=${totalError}+$?

if [[ ${totalErrors} -eq 0 ]]; then
    Display "\n************************************************************"
    Display "END OF TESTS FOR PKIX PERFORMANCE: ALL TESTS COMPLETED SUCCESSFULLY"
    Display "************************************************************"
    return 0
fi

    Display "\n*******************************************************************************"
    Display "END OF TESTS FOR PKIX TOP: ${errors} UNIT TEST${plural} FAILED:\n${failedpgms}"
    if [[ ${checkmem} -eq 1 ]]; then
        if [[ ${memErrors} -eq 1 ]]; then
            memPlural=""
        else
            memPlural="S"
        fi
        Display "                          ${memErrors} MEMORY LEAK TEST${memPlural} FAILED: ${failedmempgms}"
        
        if [[ ${prematureErrors} -ne 0 ]]; then
            if [[ ${prematureErrors} -eq 1 ]]; then
                prematurePlural=""
            else
                prematurePlural="S"
            fi
            Display "                          ${prematureErrors} MEMORY LEAK TEST${prematurePlural} INDETERMINATE: ${failedprematurepgms}"
        fi

    fi
    Display "*******************************************************************************"

return ${totalErrors}
