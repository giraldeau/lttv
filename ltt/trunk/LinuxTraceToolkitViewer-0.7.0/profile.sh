#! /bin/sh

# Script to build three versions of ltt/lttv (optimized, optimized and profiled
# and not optimized) and compare their performance for processing a trace.
# The profiled version produces detailed per function timings.
#
# The script expects 2 arguments: the temporary directory where to install
# the three versions and the directory containing the trace to process.

v1=$1/ltt1
v2=$1/ltt2
v3=$1/ltt3
tracedir=$2

if [ -z "$1" -o -z "$tracedir" ]; then
  echo "Usage: $0 tmpdir trace"
  exit 1
fi

BuildTest () {
  (make clean; ./autogen.sh --prefix=$1 --enable-lttvstatic CFLAGS="$2" LDFLAGS="$3"; make; make install) >& build.`basename $1`
}

RunTest () {
  echo RunTest $1 $2
  rm gmon.out
  for version in $v1 $v2 $v3; do
    /usr/bin/time $version/bin/lttv -m batchtest -t $tracedir $1 >& test.`basename $version`.$2
  done
  gprof $v2/bin/lttv >& test.profile.$2
}

BuildTest $v1 "-O2 -g" "-g"
BuildTest $v2 "-pg -g -O2" "-pg -g"
BuildTest $v3 "-g" "-g"

RunTest --test1 countevents
RunTest --test2 computestate
RunTest --test4 computestats
RunTest --test6 savestate
RunTest "--test3 --test6 --test7 --seek-number 200" seekevents
