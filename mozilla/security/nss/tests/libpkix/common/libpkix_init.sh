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
# libpkix_init.sh
#

### when the script is exiting, handle it in the Cleanup routine...the result
### value will get set to 0 if all the tests completed successfully, so we can
### use that value in the handler

trap 'Cleanup' EXIT

result=1
checkMem=0
arenas=0

doNist=1
doTop=0
doModule=0
doPki=0

typeset -i combinedErrors=0
typeset -i totalErrors=0
prematureTermination=0
errors=0

if [ -z "${INIT_SOURCED}" ] ; then
    libpkixCommondir=`pwd`
    cd ../../common
    . ./init.sh > /dev/null
    cd ${libpkixCommondir}
fi

DIST_BIN=${DIST}/${OBJDIR}/bin

### setup some defaults
WD=`pwd`
prog=`basename $0`
testOut=${HOSTDIR}/${prog}.$$
testOutMem=${HOSTDIR}/${prog}_mem.$$

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

    if [[ -d ../../nist_pkits/certs ]]; then
        rm -f ../../nist_pkits/certs
    fi

    if [[ ${doTop} -eq 1 ]]; then
        for i in ${linkMStoreNistFiles}; do
            if [[ -L ./rev_data/multiple_certstores/$i ]]; then
                rm -f ./rev_data/multiple_certstores/$i
            fi
        done
        if [[ -d ./rev_data/multiple_certstores ]]; then
            rm -fr rev_data/multiple_certstores
        fi
    fi

    if [[ ${doModule} -eq 1 ]]; then
        for i in ${linkModuleNistFiles}; do
            if [[ -L ./rev_data/local/$i ]]; then
                rm -f ./rev_data/local/$i
            fi
        done
    fi

    if [[ ${doPki} -eq 1 ]]; then
        for i in ${linkPkiNistFiles}; do
            if [[ -L ./rev_data/local/$i ]]; then
                rm -f ./rev_data/local/$i
            fi
        done
    fi

    return ${result}
}

### ParseArgs
function ParseArgs # args
{
    while [[ $# -gt 0 ]]; do
        if [[ $1 = "-checkmem" ]]; then
            checkmem=1
        elif [[ $1 = "-quiet" ]]; then
            quiet=1
        elif [[ $1 = "-arenas" ]]; then
            arenas=1
        fi
        shift
    done
}

function Display # string
{
    if [[ ${quiet} -eq 0 ]]; then
        echo "$1"
    fi
}

###########
# RunTests
###########
function RunTests
{
    typeset -i errors=0
    typeset -i memErrors=0
    typeset -i prematureErrors=0

    failedpgms=""
    failedmempgms=""
    failedprematurepgms=""
    memText=""
    arenaCmd=""

    if [[ ${checkmem} -eq 1 ]]; then
            memText="   (Memory Checking Enabled)"
    fi

    if [[ ${arenas} -eq 1 ]]; then
            arenaCmd="-arenas"
    fi

    #
    # Announce start of tests
    #
    Display "*******************************************************************************"
    Display "START OF TESTS FOR PKIX ${testunit} ${memText}"
Display "*******************************************************************************"
    Display ""

    # run each test specified by the input redirection below

    while read -r testPgm args; do

        if [[ ${doTop} -eq 1 || ${doModule} -eq 1 ]]; then
            testPurpose=`echo $args | awk '{print $1 " " $2 " "}'`
        else
            testPurPose=${args}
        fi

        if [[ ${doNIST} -eq 0 ]]; then
            hasNIST=`echo ${args} | grep NIST-Test`
            if [ ! -z "${hasNIST}" ]; then
                Display "SKIPPING ${testPgm} ${testPurpose}"
	        continue
	    fi
        fi

        Display "RUNNING ${testPgm} ${arenaCmd} ${testPurpose}"

        if [[ ${checkmem} -eq 1 ]]; then
            dbx -C -c "runargs ${arenaCmd} ${args};check -all;run;exit" ${DIST_BIN}/${testPgm} > ${testOut} 2>&1
        else
            ${DIST_BIN}/${testPgm} ${arenaCmd} ${args}> ${testOut} 2>&1
        fi

        # Examine output file to see if test failed and keep track of number
        # of failures and names of failed tests. This assumes that the test
        # uses our utility library for displaying information

        grep "END OF TESTS FOR" ${testOut} | tail -1 | grep "COMPLETED SUCCESSFULLY" >/dev/null 2>&1
        
        if [[ $? -ne 0 ]]; then
            errors=`expr ${errors} + 1`
            failedpgms="${failedpgms}${testPgm} ${testPurpose} "
            cat ${testOut}
        fi

        if [[ ${checkmem} -eq 1 ]]; then
            grep "(actual leaks:" ${testOut} > ${testOutMem} 2>&1
            if [[ $? -ne 0 ]]; then
                prematureErrors=`expr ${prematureErrors} + 1`
                failedprematurepgms="${failedprematurepgms}${testPgm} "
                Display "...program terminated prematurely (unable to check for memory leak errors) ..."
            else
                #grep "(actual leaks:         0" ${testOut} > /dev/null 2>&1
                # special consideration for memory leak in NSS_NoDB_Init
                grep  "(actual leaks:         1  total size:       4 bytes)" ${testOut} > /dev/null 2>&1
                if [[ $? -ne 0 ]]; then
                    memErrors=`expr ${memErrors} + 1`
                    failedmempgms="${failedmempgms}${testPgm} "
                    cat ${testOutMem}
                fi
            fi
        fi

    done

    if [[ ${errors} -eq 0 ]]; then
        if [[ ${memErrors} -eq 0 ]]; then
            Display "\n************************************************************"
            Display "END OF TESTS FOR PKIX ${testunit}: ALL TESTS COMPLETED SUCCESSFULLY"
            Display "************************************************************"
            return 0
        fi
    fi

    if [[ ${errors} -eq 1 ]]; then
        plural=""
    else
        plural="S"
    fi

    Display "\n*******************************************************************************"
    Display "END OF TESTS FOR PKIX ${testunit}: ${errors} UNIT TEST${plural} FAILED: ${failedpgms}"
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
    combinedErrors=${errors}+${memErrors}+${prematureErrors}
    return ${combinedErrors}
}
