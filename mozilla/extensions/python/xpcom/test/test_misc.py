# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with the
# License. You may obtain a copy of the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
# the specific language governing rights and limitations under the License.
#
# The Original Code is the Python XPCOM language bindings.
#
# The Initial Developer of the Original Code is ActiveState Tool Corp.
# Portions created by ActiveState Tool Corp. are Copyright (C) 2000, 2001
# ActiveState Tool Corp.  All Rights Reserved.
#
# Contributor(s): Mark Hammond <MarkH@ActiveState.com> (original author)
#

import xpcom
import xpcom.client
import xpcom._xpcom
import xpcom.components
import string

import traceback, getopt, sys

verbose_level = 0

def DumpEveryInterfaceUnderTheSun():
    "Dump every interface under the sun!"
    import xpcom, xpcom.xpt, xpcom._xpcom
    iim = xpcom._xpcom.XPTI_GetInterfaceInfoManager()

    print "Dumping every interface I can find - please wait"
    if verbose_level == 0:
        print "(verbosity is turned off, so Im not actually going to print them)"
    enum = iim.EnumerateInterfaces()
    rc = enum.First()
    num = 0
    while rc==0:
        item = enum.CurrentItem(xpcom._xpcom.IID_nsIInterfaceInfo)
        try:
            iid = item.GetIID()
        except xpcom.Exception:
            if verbose_level:
                print "Can't dump", item
            continue # Dont bother dumping this.
        interface = xpcom.xpt.Interface(iid)
        num = num + 1
        text = interface.Describe()
        if verbose_level:
            print text

        rc = enum.Next()
    if num < 200:
        print "Only found", num, "interfaces - this seems unusually low!"
    print "Finished dumping all the interfaces."

def EnumContractIDs():
    """Enumerate all the ContractIDs registered"""
    cm = xpcom._xpcom.NS_GetGlobalComponentManager()
    enum = cm.EnumerateContractIDs()
    rc = enum.First()
    n = 0
    while rc == 0:
        n = n + 1
        if verbose_level:
            print "ContractID:", enum.CurrentItem()
        rc = enum.Next()
    if n < 200:
        print "Only found", n, "ContractIDs - this seems unusually low!"
    print "Enumerated all the ContractIDs"

def TestSampleComponent():
    """Test the standard Netscape 'sample' sample"""
    # contractid = "mozilla.jssample.1" # the JS version
    contractid = "@mozilla.org/sample;1" # The C++ version.
    c = xpcom.components.classes[contractid].createInstance() \
            .queryInterface(xpcom.components.interfaces.nsISample)
    assert c.value == "initial value"
    c.value = "new value"
    assert c.value == "new value"
    c.poke("poked value")
    assert c.value == "poked value"
    c.writeValue("Python just poked:")
    print "The netscape sample worked!"

def TestHash():
    "Test that hashing COM objects works"
    d = {}
    contractid = "@mozilla.org/sample;1" # The C++ version.
    c = xpcom.components.classes[contractid].createInstance() \
        .queryInterface(xpcom.components.interfaces.nsISample)
    d[c] = None
    if not d.has_key(c):
        raise RuntimeError, "Can't get the exact same object back!"
    if not d.has_key(c.queryInterface(xpcom.components.interfaces.nsISupports)):
        raise RuntimeError, "Can't get back as nsISupports"
    # And the same in reverse - stick an nsISupports in, and make sure an explicit interface comes back.
    d = {}
    contractid = "@mozilla.org/sample;1" # The C++ version.
    c = xpcom.components.classes[contractid].createInstance() \
        .queryInterface(xpcom.components.interfaces.nsISupports)
    d[c] = None
    if not d.has_key(c):
        raise RuntimeError, "Can't get the exact same object back!"
    if not d.has_key(c.queryInterface(xpcom.components.interfaces.nsISample)):
        raise RuntimeError, "Can't get back as nsISupports"
    print "xpcom object hashing tests seemed to work"

def TestIIDs():
    "Do some basic IID semantic tests."
    iid_str = "{7ee4bdc6-cb53-42c1-a9e4-616b8e012aba}"
    IID = xpcom._xpcom.IID
    assert IID(iid_str)==IID(iid_str), "IIDs with identical strings dont compare!"
    assert hash(IID(iid_str))==hash(IID(iid_str)), "IIDs with identical strings dont have identical hashes!"
    assert IID(iid_str)==IID(iid_str.upper()), "IIDs with case-different strings dont compare!"
    assert hash(IID(iid_str))==hash(IID(iid_str.upper())), "IIDs with case-different strings dont have identical hashes!"
    # If the above work, this shoud too, but WTF
    dict = {}
    dict[IID(iid_str)] = None
    assert dict.has_key(IID(iid_str))
    assert dict.has_key(IID(iid_str.upper()))
    print "The IID tests seemed to work"
    

def usage(tests):
    import os
    print "Usage: %s [-v] [Test ...]" % os.path.basename(sys.argv[0])
    print "  -v : Verbose - print more information"
    print "where Test is one of:"
    for t in tests:
        print t.__name__,":", t.__doc__
    print
    print "If not tests are specified, all tests are run"
    sys.exit(1)

def main():
    tests = []
    args = []
    for ob in globals().values():
        if type(ob)==type(main) and ob.__doc__:
            tests.append(ob)
    if __name__ == '__main__': # Only process args when not running under the test suite!
        opts, args = getopt.getopt(sys.argv[1:], "hv")
        for opt, val in opts:
            if opt=="-h":
                usage(tests)
            if opt=="-v":
                global verbose_level
                verbose_level = verbose_level + 1

    if len(args)==0:
        print "Running all tests - use '-h' to see command-line options..."
        dotests = tests
    else:
        dotests = []
        for arg in args:
            for t in tests:
                if t.__name__==arg:
                    dotests.append(t)
                    break
            else:
                print "Test '%s' unknown - skipping" % arg
    if not len(dotests):
        print "Nothing to do!"
        usage(tests)
    for test in dotests:
        try:
            test()
        except:
            print "Test %s failed" % test.__name__
            traceback.print_exc()

# regrtest doesnt like if __name__=='__main__' blocks - it fails when running as a test!


main()
if __name__=='__main__':
    # We can only afford to shutdown if we are truly running as the main script.
    # (xpcom can't handle shutdown/init pairs)
    xpcom._xpcom.NS_ShutdownXPCOM()
    ni = xpcom._xpcom._GetInterfaceCount()
    ng = xpcom._xpcom._GetGatewayCount()
    if ni or ng:
        print "********* WARNING - Leaving with %d/%d objects alive" % (ni,ng)
