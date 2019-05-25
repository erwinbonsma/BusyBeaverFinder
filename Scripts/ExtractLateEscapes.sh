SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

grep "Late escape" | sed -f ${SCRIPTPATH}/ProcessLateEscapes.sed