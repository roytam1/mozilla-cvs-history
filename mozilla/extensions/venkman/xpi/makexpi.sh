#!/bin/bash

# Set up paths for finding files.
if [ -z "$FEDIR" ]; then FEDIR=$PWD/../resources; fi
if [ -z "$CONFIGDIR" ]; then CONFIGDIR=$FEDIR/../../config; fi
if [ -z "$XPIFILES" ]; then XPIFILES=$PWD/resources; fi
if [ -z "$XPIROOT" ]; then XPIROOT=$PWD/xpi-tree; fi
if [ -z "$JARROOT" ]; then JARROOT=$PWD/jar-tree; fi
if [ -z "$PERL" ]; then PERL=perl; fi
if [ -z "$DEBUG" ]; then DEBUG=0; fi


function showParams()
{
  I=0
  for P in "$@"; do
    I=$((I+1))
    echo PARAM $I: "$P"
  done
}

## Call this with lots of parameters to run a command, log errors, and abort
## if it fails. Supports redirection if '>' and '<' are passed as arguments,
## e.g.:
##   safeCommand cmd arg1 arg2 '<' input.file '>' output-file
##
## Note: only a single input and single output redirection is supported.
##
function safeCommand()
{
  local -a CMD
  CMD_COUNT=$((0))
  INF=""
  OUTF=""
  LASTP=""
  for P in "$@"; do
    if [ "$LASTP" = "<" ]; then
      if [ -n "$INF" ]; then
        echo "ERROR: Multiple input files passed to safeCommand()." >&2
        exit 2
      fi
      INF="$P"
    elif [ "$LASTP" = ">" ]; then
      if [ -n "$OUTF" ]; then
        echo "ERROR: Multiple output files passed to safeCommand()." >&2
        exit 2
      fi
      OUTF="$P"
    elif [ "$P" = ">" -o "$P" = "<" ]; then
      echo >/dev/null
    else
      CMD[$CMD_COUNT]="$P"
      CMD_COUNT=$((CMD_COUNT+1))
    fi
    LASTP="$P"
  done
  
  if [ $DEBUG -gt 0 ]; then
    echo
    showParams "${CMD[@]}"
    echo 'INPUT  :' "$INF"
    echo 'OUTPUT :' "$OUTF"
  fi
  
  touch log.stdout log.stderr
  if [ -z "$INF" -a -z "$OUTF" ]; then
    "${CMD[@]}" 1>log.stdout 2>log.stderr
  elif [ -z "$INF" ]; then
    "${CMD[@]}" 1> "$OUTF" 2>log.stderr
  elif [ -z "$OUTF" ]; then
    "${CMD[@]}" < "$INF" 1>log.stdout 2>log.stderr
  else
    "${CMD[@]}" < "$INF" 1> "$OUTF" 2>log.stderr
  fi
  
  EC=$?
  if [ $DEBUG -gt 0 ]; then
    echo 'RESULT :' $EC
  fi
  if [ "$EC" != "0" ]; then
    echo "ERROR ($EC)"
    cat log.stdout
    cat log.stderr
    rm -f log.stdout log.stderr
    exit 1
  fi
  rm -f log.stdout log.stderr
  return $EC
}


## Begin real program ##


if [ "$1" = "clean" ]; then
  echo -n "Cleaning up files"
  echo -n .
  rm -rf "$XPIROOT"
  echo -n .
  rm -rf "$JARROOT"
  echo   ". done."
  
  exit
fi


# Check setup.
if ! [ -d "$FEDIR" ]; then
  echo "ERROR: Base JavaScript Debugger directory (FEDIR) not found."
  exit 1
fi
if ! [ -d "$CONFIGDIR" ]; then
  echo "ERROR: mozilla/config directory (CONFIGDIR) not found."
  exit 1
fi


# Extract version number.
VERSION=`cat $FEDIR/../version.txt`

if [ -z "$VERSION" ]; then
  echo "ERROR: Unable to get version number."
  exit 1
fi

echo Beginning build of JavaScript Debugger $VERSION...


# Check for existing.
if [ -r "venkman-$VERSION.xpi" ]; then
  echo "  WARNING: output XPI will be overwritten."
fi


# Check for required directory layouts.
echo -n "  Checking XPI structure"
echo -n .
if ! [ -d xpi-tree ]; then mkdir xpi-tree; fi
echo -n .
if ! [ -d xpi-tree/chrome ]; then mkdir xpi-tree/chrome; fi
echo -n .
if ! [ -d xpi-tree/components ]; then mkdir xpi-tree/components; fi
echo   ".               done"

echo -n "  Checking JAR structure"
echo -n .
if ! [ -d jar-tree ]; then mkdir jar-tree; fi
echo   ".                 done"


# Make Toolkit updates.
echo -n "  Updating Toolkit Extension files"
echo -n .
safeCommand $PERL $CONFIGDIR/preprocessor.pl -DVENKMAN_VERSION=$VERSION "$XPIFILES/install.rdf" '>' "$XPIROOT/install.rdf"
echo   ".       done"


# Make Mozilla Suite / SeaMonkey 1.x updates.
echo -n "  Updating XPFE Extension files"
echo -n .
safeCommand sed "s|@REVISION@|$VERSION|g" '<' "$XPIFILES/install.js" '>' "$XPIROOT/install.js"
echo -n .
safeCommand mv "$FEDIR/content/contents.rdf" "$FEDIR/content/contents.rdf.in"
echo -n .
safeCommand sed "s|@MOZILLA_VERSION@|vnk-$VERSION|g;s|\(chrome:displayName=\)\"[^\"]\{1,\}\"|\1\"JavaScript Debugger $VERSION\"|g" '<' "$FEDIR/content/contents.rdf.in" '>' "$FEDIR/content/contents.rdf"
echo -n .
safeCommand rm "$FEDIR/content/contents.rdf.in"
echo -n .
safeCommand mv "$FEDIR/locale/en-US/contents.rdf" "$FEDIR/locale/en-US/contents.rdf.in"
echo -n .
safeCommand sed "s|@MOZILLA_VERSION@|vnk-$VERSION|g" '<' "$FEDIR/locale/en-US/contents.rdf.in" '>' "$FEDIR/locale/en-US/contents.rdf"
echo -n .
safeCommand rm "$FEDIR/locale/en-US/contents.rdf.in"
echo   ".    done"


# Create JAR.
echo -n "  Constructing JAR package"
echo -n .
OLDPWD=`pwd`
cd "$CONFIGDIR"
echo -n .
safeCommand $PERL make-jars.pl -v -z zip -p preprocessor.pl -s "$FEDIR" -d "$JARROOT"  -- -DVENKMAN_VERSION=$VERSION '<' "$FEDIR/jar.mn"
echo -n .
cd "$OLDPWD"
echo   ".           done"


# Make XPI.
echo -n "  Constructing XPI package"
echo -n .
safeCommand cp -v "$JARROOT/venkman.jar" "$XPIROOT/chrome/"
echo -n .
safeCommand cp -v "$FEDIR/../js/venkman-service.js" "$XPIROOT/components/"
echo -n .
safeCommand chmod 664 "$XPIROOT/chrome/venkman.jar"
echo -n .
safeCommand chmod 664 "$XPIROOT/components/venkman-service.js"
echo -n .
OLDPWD=`pwd`
cd "$XPIROOT"
safeCommand zip -vr ../venkman-$VERSION.xpi . -i "*" -x "log*"
cd "$OLDPWD"
echo   ".           done"


echo "Build of JavaScript Debugger $VERSION... ALL DONE"
