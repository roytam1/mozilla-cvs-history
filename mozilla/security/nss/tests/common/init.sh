#! /bin/sh
#
#  Initialize a bunch of variables that may tests would be interested in
#
#
mozilla_root=`(cd ../../../..; pwd)`
common=`(cd ../common; pwd)`
MOZILLA_ROOT=${MOZILLA_ROOT-$mozilla_root}
COMMON=${TEST_COMMON-$common}
DIST=${DIST-${MOZILLA_ROOT}/dist}
SECURITY_ROOT=${SECURITY_ROOT-${MOZILLA_ROOT}/security/nss}
TESTDIR=${TESTDIR-${MOZILLA_ROOT}/tests_results/security}
. ../common/arch.sh
OBJDIR=${OBJDIR_NAME}
PATH=${DIST}/${OBJDIR}/bin:${DIST}/${OBJDIR}/lib:$PATH
export PATH
LD_LIBRARY_PATH=${DIST}/${OBJDIR}/lib
export LD_LIBRARY_PATH
echo "Creating ${TESTDIR}"
if [ ! -d ${TESTDIR} ]; then
   mkdir -p ${TESTDIR}
fi

if [ ! -s "${HOSTDIR}" ]; then
    version=1
    if [ -f ${TESTDIR}/${HOST} ]; then
	version=`cat ${TESTDIR}/${HOST}`
    fi
    expr $version + 1 > ${TESTDIR}/${HOST}

    HOSTDIR=${TESTDIR}/${HOST}'.'$version
fi

if [ ! -d ${HOSTDIR} ]; then
   mkdir -p ${HOSTDIR}
fi

RESULTS=${HOSTDIR}/results.html
if [ ! -f "${RESULTS}" ]; then

	cp ${COMMON}/results_header.html ${RESULTS}
	echo "<H4>Platform: ${OBJDIR}<BR>" >> ${RESULTS}
	echo "Test Run: ${HOST}.$version</H4>" >> ${RESULTS}
	echo "<HR><BR>" >> ${RESULTS}

	echo "********************************************"
	echo "   Platform: ${OBJDIR}"
	echo "   Results: ${HOST}.$version"
	echo "********************************************"
fi


KILL="kill"
if  [ ${OS_ARCH} = "Linux" ]; then
        KILL="kill -kill"
fi

export  KILL


