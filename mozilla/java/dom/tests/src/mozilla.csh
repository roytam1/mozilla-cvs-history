setenv TEST_PATH `pwd`/..
setenv MOZILLA_FIVE_HOME `pwd`/../../../../dist/bin
setenv JAVADOM_HOME `pwd`/../../
setenv JAVA_HOME /usr/local/java/jdk1.2/solaris 
setenv PREFIX /workspace

setenv LD_LIBRARY_PATH ${JAVA_HOME}/jre/lib/sparc
setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${JAVA_HOME}/jre/lib/sparc/classic
setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${JAVA_HOME}/jre/lib/sparc/native_threads
setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${PREFIX}/lib:${PREFIX}/mozilla/dist/lib
setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${MOZILLA_FIVE_HOME}

setenv PATH ${JAVA_HOME}/bin:${PATH}
setenv PATH ${PATH}:${MOZILLA_FIVE_HOME}:${PREFIX}/bin

setenv CLASSPATH ${JAVADOM_HOME}/classes:${MOZILLA_FIVE_HOME}/../classes:${PREFIX}/xml/xml.jar:.
setenv CLASSPATH ${TEST_PATH}/classes:${CLASSPATH}
