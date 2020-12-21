#!/bin/bash
#
# Script for following up on search by chasing late escapes.
#
# It expects log of the search run via stdin
# As argument it expect invocation of search, without --late-escape argument
#
# It will generate output logs and late escape files in the current directory.
# It will also overwrite any existing files.
#
# Example invocation:
# echo search-log.txt | ${SCRIPT_PATH}ProcessLateEscapes.sh ${BIN_PATH}/BusyBeaverFinder [ARGS]

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

SEARCH_CMD=$*
echo Search command=${SEARCH_CMD}

RUN=1
grep "Late escape" | sed -f ${SCRIPTPATH}/ProcessLateEscapes.sed > late-escapes-${RUN}.txt

while true; do
  NUM_LATE_ESCAPES=`wc -l late-escapes-${RUN}.txt | sed 's_ __g' | sed 's_[^0-9].*__'`

  if [[ "${NUM_LATE_ESCAPES}" == "0" ]]
  then
    echo No more late escapes
    exit 0
  fi

  echo Run ${RUN}: late escapes=${NUM_LATE_ESCAPES}

  ${SEARCH_CMD} --late-escapes late-escapes-${RUN}.txt > late-escapes-${RUN}-log.txt
  cat late-escapes-${RUN}-log.txt | grep "Best=" | tail -n1

  NXTRUN=$((RUN+1))

  cat late-escapes-${RUN}-log.txt | grep "Late escape" | sed -f ${SCRIPTPATH}/ProcessLateEscapes.sed > late-escapes-${NXTRUN}.txt

  RUN=${NXTRUN}
done