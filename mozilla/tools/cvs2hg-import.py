#!/usr/bin/python

"""A script which will import from Mozilla CVS into a Mercurial
repository. It pulls NSPR and NSS from the appropriate release tags
currently in use on the Mozilla trunk."""

from sys import exit
from os import environ, makedirs, listdir, rename, unlink
from os.path import isdir
from shutil import rmtree
from tempfile import mkdtemp
from datetime import datetime

try:
    from subprocess import check_call
except ImportError:
    # Python 2.4 doesn't have check_call, so we reimplement it
    from subprocess import call
    def check_call(*popenargs, **kwargs):
        retcode = call(*popenargs, **kwargs)
        if retcode:
            cmd = kwargs.get("args")
            if cmd is None:
                cmd = popenargs[0]
            raise Exception("Command '%s' returned non-zero exit status %d" % (cmd, retcode))

## The HG_REPO_INITIAL_IMPORT tag is based on the MOZILLA_1_9a3_RELEASE tag,
## but only applied to the directories that were imported into Hg (not
## the entire tree, like the 1.9a3 tag. The tag date was pulled from the logs
## when that tag was applied. The _DATE is for record keeping purposes, and
## for the initial import checkin message ; the import pulls the tag.
CVS_REPO_IMPORT_TAG = "HG_REPO_INITIAL_IMPORT"
CVS_REPO_IMPORT_DATE = "22 Mar 2007 10:30 PDT"

mozilla_files = (
    "Makefile.in",
    "LICENSE",
    "LEGAL",
    "README.txt",
    "accessible",
    "aclocal.m4",
    "allmakefiles.sh",
    "browser",
    "build",
    "caps",
    "chrome",
    "config",
    "configure.in",
    "client.mk",
    "content",
    "db",
    "docshell",
    "dom",
    "editor",
    "embedding",
    "extensions/Makefile.in",
    "extensions/access-builtin",
    "extensions/auth",
    "extensions/canvas3d",
    "extensions/cookie",
    "extensions/gnomevfs",
    "extensions/inspector",
    "extensions/java",
    "extensions/jssh",
    "extensions/layout-debug",
    "extensions/metrics",
    "extensions/negotiateauth",
    "extensions/permissions",
    "extensions/pref",
    "extensions/python",
    "extensions/reporter",
    "extensions/spellcheck",
    "extensions/universalchardet",
    "gfx",
    "intl",
    "ipc",
    "jpeg",
    "js",
    "layout",
    "modules",
    "netwerk",
    "other-licenses/7zstub",
    "other-licenses/atk-1.0",
    "other-licenses/branding/firefox",
    "other-licenses/bsdiff",
    "other-licenses/ia2",
    "parser",
    "plugin",
    "profile",
    "rdf",
    "security/manager",
    "storage",
    "sun-java",
    "testing",
    "toolkit",
    "tools/build",
    "tools/codesighs",
    "tools/cross-commit",
    "tools/elf-dynstr-gc",
    "tools/footprint",
    "tools/httptester",
    "tools/jprof",
    "tools/l10n",
    "tools/leaky",
    "tools/memory",
    "tools/page-loader",
    "tools/patcher",
    "tools/performance",
    "tools/rb",
    "tools/release",
    "tools/relic",
    "tools/reorder",
    "tools/test-harness",
    "tools/tests",
    "tools/testserver",
    "tools/testy",
    "tools/trace-malloc",
    "tools/update-packaging",
    "tools/uuiddeps",
    "uriloader",
    "view",
    "webshell",
    "widget",
    "xpcom",
    "xpfe",
    "xpinstall",
    "xulrunner"
    )

nspr_files = ("nsprpub",)
NSPR_CVS_TRACKING_TAG = "NSPRPUB_PRE_4_2_CLIENT_BRANCH"

nss_files = (
    "dbm",
    "security/nss",
    "security/coreconf",
    "security/dbm"
    )
NSS_CVS_TRACKING_TAG = "NSS_3_11_5_RTM"

def ensurevalue(val, envvar, default = None):
    if val:
        return val

    if envvar in environ:
        return environ[envvar]

    if default:
        return default

    raise ValueError("No %s found." % envvar)

def rmfileortree(path):
    print "Removing %s" % path
    sys.stdout.flush()
    if isdir(path):
        rmtree(path)
    else:
        unlink(path)

def CheckoutDirs(directory, cvsroot, dirlist, date=None, branch=None):
    arglist = ['cvs', '-d', cvsroot, 'co', '-P', '-N']
    if date is not None:
        arglist.extend(['-D', date])
    if branch is not None:
        arglist.extend(['-r', branch])

    arglist.extend(["mozilla/%s" % dir for dir in dirlist])
    check_call(arglist, cwd=directory)

def ImportMozillaCVS(directory, cvsroot=None, hg=None, tempdir=None, mode=None, importDate=None):
    cvsroot = ensurevalue(cvsroot, "CVSROOT", ":ext:cltbld@cvs.mozilla.org:/cvsroot")
    
    tempd = mkdtemp("cvsimport", dir=tempdir)

    if mode == 'init':
        nspr_tag = nss_tag = mozilla_tag = CVS_REPO_IMPORT_TAG
        nspr_date = nss_date = mozilla_date = None
    elif mode == 'import':
        if importDate is not None:
            nspr_date = nss_date = mozilla_date = importDate
        else:
            nspr_date = nss_date = mozilla_date = datetime.utcnow().strftime("%d %b %Y %T +0000")

        nspr_tag = NSPR_CVS_TRACKING_TAG
        nss_tag = NSS_CVS_TRACKING_TAG
        mozilla_tag = None
    else:
        raise Exception, "ImportMozillaCVS unknown/unsupported mode: %s" % mode

    importModules = {};
    importModules['mozilla'] = { 'files' : mozilla_files,
                                 'tag' : mozilla_tag,
                                 'date' : mozilla_date };

    importModules['nss'] = { 'files' : nss_files,
                             'tag' : nss_tag,
                             'date' : nss_date };

    importModules['nspr'] = { 'files' : nspr_files,
                              'tag' : nspr_tag,
                              'date' : nspr_date };

    try:
        try:
            for cvsModuleName in importModules.keys(): 
                cvsModule = importModules[cvsModuleName]

                CheckoutDirs(tempd, cvsroot, cvsModule['files'],
                 cvsModule['date'], cvsModule['tag'])

            # Remove everything in the hg repository except for the .hg
            # directory
            for f in listdir(directory):
                if f != ".hg" and f != ".hgignore":
                    rmfileortree("%s/%s" % (directory, f))

            # Move everything from the mozilla/ directory to the hg repo
            for f in listdir("%s/mozilla" % tempd):
                source = "%s/mozilla/%s" % (tempd, f)
                dest = "%s/%s" % (directory, f)
                print "Moving %s to %s" % (source, dest)
                sys.stdout.flush()
                rename(source, dest)

            check_call(['hg', 'add'], cwd=directory)
            check_call(['hg', 'remove', '--after'], cwd=directory)

            commitMesg = "Automatic merge from CVS: " 

            for cvsModuleName in importModules.keys(): 
                cvsModule = importModules[cvsModuleName]
                if cvsModule['tag'] is None:
                    cvsTagName = 'HEAD'
                else:
                    cvsTagName = cvsModule['tag']

                ## In the init case, munge the date (which is None, because
                ## we pull by tag for the initial import), so the checkin
                ## message has something useful.
                if mode == 'init':
                    cvsDate = CVS_REPO_IMPORT_DATE
                else:
                    cvsDate = cvsModule['date']

                commitMesg = (commitMesg + "Module %s: tag %s at %s, " % 
                 (cvsModuleName, cvsTagName, cvsDate))

            check_call(['hg', 'commit', '-m', commitMesg], cwd=directory)
    
        except Exception, e:
            print "ImportMozillaCVS: Exception hit: %s" % (str(e))
            sys.stdout.flush()
            raise

    finally:
        rmtree(tempd)

def InitRepo(directory, hg=None):
    hg = ensurevalue(hg, "HG", "hg")
    check_call([hg, 'init', directory])

    ignoref = open("%s/.hgignore" % directory, "wb")
    print >>ignoref, "CVS\n\\.cvsignore"
    ignoref.close()

    check_call([hg, 'add', '.hgignore'], cwd=directory)
    check_call([hg, 'commit', '-m', 'Set up .hgignore to ignore CVS files.'],
               cwd=directory)

if __name__ == '__main__':
    from optparse import OptionParser

    usage = "usage: %prog [options] directory"
    p = OptionParser()
    p.add_option("--cvsroot", dest="cvsroot",
                 help="Specify the CVSROOT for checkout.")
    p.add_option("--hg", dest="hg",
                 help="Path to the hg executable.")
    p.add_option("--initrepo", dest="initrepo", action="store_true",
                 help="Initialize a repository for import.")
    p.add_option("--tempdir", dest="tempdir",
                 help="Use a specific directory for temporary files.")
    p.add_option("--importdate", dest="importdate",
                 help="Use a specific date to import from CVS; " +
                  "must be parseable by CVS's -D option.")

    (options, args) = p.parse_args()

    if len(args) != 1:
        p.print_help()
        exit(1)

    if options.initrepo:
        print "Initializing hg repository '%s'." % args[0]
        sys.stdout.flush()
        InitRepo(args[0], options.hg)
        ImportMozillaCVS(args[0], options.hg, options.cvsroot, options.tempdir,
         'init')
        print "Initialization successful."
        sys.stdout.flush()
    else:
        print "Importing CVS to repository '%s'." % args[0]
        sys.stdout.flush()
        ImportMozillaCVS(args[0], options.hg, options.cvsroot, options.tempdir,
         'import', options.importdate)
        print "Import successful."
        sys.stdout.flush()
