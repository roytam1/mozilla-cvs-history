#! /bin/bash

#### LOCAL MACHINE SETTINGS ####
PORT_64_DBG=8543
PORT_64_OPT=8544
PORT_32_DBG=8545
PORT_32_OPT=8546
if [ "${NSS_TESTS}" = "memtest" ]; then
    PORT_64_DBG=8547
    PORT_64_OPT=8548
    PORT_32_DBG=8549
    PORT_32_OPT=8550
fi
JAVA_HOME_64=/usr/lib/jvm/java-1.6.0-openjdk.x86_64
JAVA_HOME_32=/usr/lib/jvm/java-1.6.0-openjdk

# example configuration
case ${HOST} in
host1) 
    JAVA_HOME_64=/opt/jdk/1.6.0_01/SunOS64
    JAVA_HOME_32=/opt/jdk/1.6.0_01/SunOS
    ;;
host2)
    run_bits="32"
    export NSS_TESTS=memleak
    NO_JSS=1
    ;;
esac

