#! /bin/sh
#
# This is just a quick script so we can still run our testcases.
# Longer term we need a scriptable test environment..
#
. ../common/init.sh
CURDIR=`pwd`
echo "PATH=$PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
PORT=${PORT-8443}

# Test case files
SSLCOV=${CURDIR}/sslcov.txt
SSLAUTH=${CURDIR}/sslauth.txt
SSLSTRESS=${CURDIR}/sslstress.txt
REQUEST_FILE=${CURDIR}/sslreq.txt

#temparary files
TMP=${TMP-/tmp}
PWFILE=${TMP}/tests.pw.$$
CERTSCRIPT=${TMP}/tests_certs.$$
NOISE_FILE=${TMP}/tests_noise.$$
SERVEROUTFILE=${TMP}/tests_server.$$
SERVERPID=${TMP}/tests_pid.$$
CERTUTILOUT=${TMP}/certutilout.$$

TEMPFILES="${PWFILE} ${CERTSCRIPT} ${SERVEROUTFILE} ${NOISE_FILE} ${SERVERPID} ${CERTUTILOUT}"

none=1
coverage=0
auth=0
stress=0
certs=1
fileout=0

Exit()
{
	echo $1
	rm -f ${TEMPFILES}
	if [ -f "${SERVERPID}" ]
	then
		${KILL} `cat ${SERVERPID}`
	fi
	exit 1
}

is_selfserv_alive()
{
	#echo "Testing if server is alive..."
	if [ ! -f "${SERVERPID}" ]
	then
		echo "Error - selfserver pid file ${SERVERPID} does not exist"
		sleep 5
		if [ ! -f "${SERVERPID}" ]
		then
			Exit "Fatal Error - selfserver pid file ${SERVERPID} still does not exist - exiting"
		fi
	fi
	PID=`cat ${SERVERPID}`
	SERVER_OK="FALSE"
	if [ `uname -s` = "SunOS" ]
	then
		/usr/5bin/ps -e | grep $PID >/dev/null && SERVER_OK="TRUE"
	else
		ps -e | grep $PID >/dev/null && SERVER_OK="TRUE"
	fi
	if [ "$SERVER_OK" = "FALSE" ]
	then
		Exit "Fatal Error - selfserver process not dedectable"
	fi
}

for i in $*
do
	case $i in
		[aA][lL]*)
			none=0; coverage=1; auth=1; stress=1;;
		[aA][uU]*)
			none=0; auth=1;;
		[Nn][Oo][aA][uU]*)
			auth=0;;
		[Cc][Oo]*)
			none=0; coverage=1;;
		[Nn][Oo][Cc][Oo]*)
			coverage=0;;
		[Cc][Ee]*)
			none=0; certs=1;;
		[Nn][Oo][Cc][Ee]*)
			certs=0;;
		[Ss]*)
			none=0; stress=1;;
		[Nn][Oo][Ss]*)
			stress=0;;
		[Vv][Ee][Rr][Bb]*)
			verbose=-v;;
		f)
			fileout=1;
	esac
done

if [ $none -eq 1 ]; then
	coverage=1
	auth=1
	stress=1
fi

#
# should also try to kill any running server
#
trap "rm -f ${TEMPFILES}; Exit Signal_caught" 2 3

CADIR=${HOSTDIR}/CA
SERVERDIR=${HOSTDIR}/server
CLIENTDIR=${HOSTDIR}/client

if [ $certs -eq 1 ]; then
# Generate noise for our CA cert.
#
# NOTE: these keys are only suitable for testing, as this whole thing bypasses
# the entropy gathering. Don't use this method to generate keys and certs for
# product use or deployment.
#
	ps -efl > ${NOISE_FILE} 2>&1
	ps aux >> ${NOISE_FILE} 2>&1
	netstat >> ${NOISE_FILE} 2>&1
	date >> ${NOISE_FILE} 2>&1

#
# build the TEMP CA used for testing purposes
#
	echo "<TABLE BORDER=1><TR><TH COLSPAN=3>Certutil Tests</TH></TR>" >> ${RESULTS}
	echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}
	echo "********************** Creating a CA Certificate **********************"
	if [ ! -d ${CADIR} ]; then
		mkdir -p ${CADIR}
	fi
	cd ${CADIR}
	echo nss > ${PWFILE}
	echo "certutil -N -d . -f ${PWFILE} 2>&1"
	certutil -N -d . -f ${PWFILE} 2>&1

	echo initialized
	echo 5 > ${CERTSCRIPT}
	echo 9 >> ${CERTSCRIPT}
	echo n >> ${CERTSCRIPT}
	echo y >> ${CERTSCRIPT}
	echo 3 >> ${CERTSCRIPT}
	echo n >> ${CERTSCRIPT}
	echo 5 >> ${CERTSCRIPT}
	echo 6 >> ${CERTSCRIPT}
	echo 7 >> ${CERTSCRIPT}
	echo 9 >> ${CERTSCRIPT}
	echo n >> ${CERTSCRIPT}
	echo "certutil -S -n \"TestCA\" -s \"CN=NSS Test CA, O=BOGUS NSS, L=Mountain View, ST=California, C=US\" -t \"CTu,CTu,CTu\" -v 60 -x -d . -1 -2 -5 -f ${PWFILE} -z ${NOISE_FILE} 2>&1"
	certutil -S -n "TestCA" -s "CN=NSS Test CA, O=BOGUS NSS, L=Mountain View, ST=California, C=US" -t "CTu,CTu,CTu" -v 60 -x -d . -1 -2 -5 -f ${PWFILE} -z ${NOISE_FILE} < ${CERTSCRIPT} 2>&1

	if [ $? -ne 0 ]; then
		echo "<TR><TD>Creating CA Cert</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
	else
		echo "<TR><TD>Creating CA Cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
	fi

	echo "**************** Creating Client CA Issued Certificate ****************"
	netstat >> ${NOISE_FILE} 2>&1
	date >> ${NOISE_FILE} 2>&1
	if [ ! -d ${CLIENTDIR} ]; then
		mkdir -p ${CLIENTDIR}
	fi
	cd ${CLIENTDIR}
	echo "certutil -N -d . -f ${PWFILE} 2>&1"
	certutil -N -d . -f ${PWFILE} 2>&1
	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Init DB"}
	fi
	echo "Import the root CA"
	echo "certutil -L -n \"TestCA\" -r -d ../CA > root.cert 2>>$CERTUTILOUT"
	certutil -L -n "TestCA" -r -d ../CA > root.cert 2>>$CERTUTILOUT

	cat $CERTUTILOUT

	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Export Root"}
	fi
	echo "certutil -A -n \"TestCA\" -t \"TC,TC,TC\" -f ${PWFILE} -d . -i root.cert 2>&1"
	certutil -A -n "TestCA" -t "TC,TC,TC" -f ${PWFILE} -d . -i root.cert 2>&1
	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Import Root"}
	fi
	echo "Generate a Certificate request"
	echo " certutil -R -s \"CN=Test User, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d . -f ${PWFILE} -z ${NOISE_FILE} -o req 2>&1"
	certutil -R -s "CN=Test User, O=BOGUS NSS, L=Mountain View, ST=California, C=US" -d . -f ${PWFILE} -z ${NOISE_FILE} -o req 2>&1
	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Generate Request"}
	fi
	echo "Sign the Certificate request"
	echo "certutil -C -c "TestCA" -m 3 -v 60 -d ../CA -f ${PWFILE} -i req -o user.cert 2>&1"
	certutil -C -c "TestCA" -m 3 -v 60 -d ../CA -i req -o user.cert -f ${PWFILE} 2>&1
	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Sign User Cert"}
	fi
	echo "Import the new Cert"
	echo "certutil -A -n \"TestUser\" -t \"u,u,u\" -d . -f ${PWFILE} -i user.cert 2>&1"
	certutil -A -n "TestUser" -t "u,u,u" -d . -f ${PWFILE} -i user.cert 2>&1
	if [ $? -ne 0 ]; then
		CERTFAILED=${CERTFAILED-"Import User"}
	fi
	if [ -n "${CERTFAILED}" ]; then
		echo "<TR><TD>Creating User Cert</TD><TD bgcolor=red>Failed ($CERTFAILED)</TD><TR>" >> ${RESULTS}
	else
		echo "<TR><TD>Creating User Cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
	fi

	echo "***** Creating Server CA Issued Certificate for ${HOST}.${DOMSUF} *****"
	netstat >> ${NOISE_FILE} 2>&1
	date >> ${NOISE_FILE} 2>&1
	if [ ! -d ${SERVERDIR} ]; then
		mkdir -p ${SERVERDIR}
	fi
	cd ${SERVERDIR}
	cp ../CA/*.db .
	echo "certutil -S -n \"${HOST}.${DOMSUF}\" -s \"CN=${HOST}.${DOMSUF}, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -t \"Pu,Pu,Pu\" -c "TestCA" -v 60 -d . -f ${PWFILE} -z ${NOISE_FILE} 2>&1"
	certutil -S -n "${HOST}.${DOMSUF}" -s "CN=${HOST}.${DOMSUF}, O=BOGUS Netscape, L=Mountain View, ST=California, C=US" -t "Pu,Pu,Pu" -c "TestCA" -m 1 -v 60 -d . -f ${PWFILE} -z ${NOISE_FILE} 2>&1
	if [ $? -ne 0 ]; then
		echo "<TR><TD>Creating Server Cert</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
	else
		echo "<TR><TD>Creating Server Cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
	fi
	echo "</TABLE><BR>" >> ${RESULTS}

	rm -f ${TEMPFILES}
fi


# OK now lets run the tests....
if [ $coverage -eq 1 ]; then
	echo "********************* SSL Cipher Coverage ****************************"
	echo "<TABLE BORDER=1><TR><TH COLSPAN=3>SSL Cipher Coverage</TH></TR>" >> ${RESULTS}
	echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}
	cd ${CLIENTDIR}

	# Launch the server
	echo "selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -i ${SERVERPID} -w nss -c ABCDEFabcdefghijklm & "
	if [ ${fileout} -eq 1 ]; then
		selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -i ${SERVERPID} -w nss -c ABCDEFabcdefghijklm > ${SERVEROUTFILE} 2>&1 &
	else
		selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss -i ${SERVERPID} -c ABCDEFabcdefghijklm &
	fi
	# wait until it's alive
	echo "tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}"
	tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}
	if [ $? -ne 0 ]; then
		echo "<TR><TD> Wait for Server </TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
	else
		echo "<TR><TD> Wait for Server </TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
	fi

	cat ${SSLCOV} | while read tls param testname
	do
		if [ $tls != "#" ]; then
			echo "********************* $testname ****************************"
			TLS_FLAG=-T
			if [ $tls = "TLS" ]; then
				TLS_FLAG=""
			fi
			sparam=""
			if [ ${param} = "i" ]; then
				sparam='-c i'
			fi

			is_selfserv_alive
			echo "tstclnt -p ${PORT} -h ${HOST} -c ${param} ${TLS_FLAG} -f -d . redir from ${REQUEST_FILE}"
			tstclnt -p ${PORT} -h ${HOST} -c ${param} ${TLS_FLAG} -f -d . < ${REQUEST_FILE}
			if [ $? -ne 0 ]; then
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
			else
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
			fi
		fi
	done
	# now kill the server
	${KILL} `cat ${SERVERPID}`
	wait `cat ${SERVERPID}`
	if [ ${fileout} -eq 1 ]; then
		cat ${SERVEROUTFILE}
	fi
	${SLEEP}

	echo "</TABLE><BR>" >> ${RESULTS}
fi

if [ $auth -eq 1 ]; then
	echo "********************* SSL Client Auth ****************************"
	cd ${CLIENTDIR}
	echo "<TABLE BORDER=1><TR><TH COLSPAN=3>SSL Client Authentication</TH></TR>" >> ${RESULTS}
	echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}

	cat ${SSLAUTH} | while read value sparam cparam testname
	do
		if [ $value != "#" ]; then
			echo "***** $testname ****"
			sparam=`echo $sparam | sed -e 's;_; ;g'`
			cparam=`echo $cparam | sed -e 's;_; ;g'`
			echo "selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} &"
			if [ ${fileout} -eq 1 ]; then
				selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} > ${SERVEROUTFILE} 2>&1 &
			else
				selfserv -v -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} &
			fi
			echo "tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} redir from ${REQUEST_FILE}"
			tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}
			if [ $? -ne 0 ]; then
				echo "<TR><TD> Wait for Server </TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
				echo "tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} redir from ${REQUEST_FILE}"
				tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}
			fi
			pwd
			is_selfserv_alive
			echo "tstclnt -p ${PORT} -h ${HOST} -f -d ${CLIENTDIR} ${cparam} redir from ${REQUEST_FILE}"
			tstclnt -p ${PORT} -h ${HOST} -f -d ${CLIENTDIR} ${cparam} < ${REQUEST_FILE}
			ret=$?

#
# for some reason the NT client does not return the same error code as Unix
# (sigh).
#
			if [ ${OS_ARCH} = "WINNT" ]; then
				if [ $value -ne 0 ]; then
					if [ $ret -ne 0 ]; then
						value=$ret
					fi
				fi
			fi

			if [ $ret -ne $value ]; then
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
				echo "FAILURE: test $testname produced a returncode of $ret, expected is $value O_CRON = $O_CRON"
			else
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
				echo "test $testname produced a returncode of $ret as expected "
			fi
			${KILL} `cat ${SERVERPID}`
			wait `cat ${SERVERPID}`
			if [ ${fileout} -eq 1 ]; then
				cat ${SERVEROUTFILE}
			fi
			${SLEEP}
		fi
	done

	echo "</TABLE><BR>" >> ${RESULTS}
fi


if [ $stress -eq 1 ]; then
	echo "********************* Stress Test ****************************"
	cd ${CLIENTDIR}
	echo "<TABLE BORDER=1><TR><TH COLSPAN=3>SSL Stress Test</TH></TR>" >> ${RESULTS}
	echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}

	cat ${SSLSTRESS} | while read value sparam cparam testname
	do
		if [ $value != "#" ]; then
			echo "********************* $testname ****************************"
			sparam=`echo $sparam | sed -e 's;_; ;g'`
			cparam=`echo $cparam | sed -e 's;_; ;g'`
			echo "selfserv -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} $verbose & started at `date`"
			if [ ${fileout} -eq 1 ]; then
				selfserv -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} $verbose > ${SERVEROUTFILE} 2>&1 &
			else
				selfserv -p ${PORT} -d ${SERVERDIR} -n ${HOST}.${DOMSUF} -w nss ${sparam} -i ${SERVERPID} $verbose &
			fi
			echo "tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE} started at `date`"
			tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}
			if [ $? -ne 0 ]; then
				echo "<TR><TD> Wait for Server </TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
				echo "tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} redir from ${REQUEST_FILE}"
				tstclnt -p ${PORT} -h ${HOST} -q -d ${CLIENTDIR} < ${REQUEST_FILE}
			fi

			is_selfserv_alive
			echo "strsclnt -p ${PORT} -d . -w nss $cparam $verbose ${HOST}.${DOMSUF} started at `date`"
			strsclnt -p ${PORT} -d . -w nss $cparam $verbose ${HOST}.${DOMSUF}
			echo "strsclnt completed at `date`"

			if [ $? -ne $value ]; then
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
			else
				echo "<TR><TD>"${testname}"</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
			fi
			${KILL} `cat ${SERVERPID}`
			wait `cat ${SERVERPID}`
			if [ ${fileout} -eq 1 ]; then
				cat ${SERVEROUTFILE}
			fi
			${SLEEP}
		fi
	done

	echo "</TABLE><BR>" >> ${RESULTS}
fi

rm -f ${TEMPFILES}
