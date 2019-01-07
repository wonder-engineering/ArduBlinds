#!/bin/bash

echo "run_lint.bash: LINTING START=========================================================="

cpplint --extensions=ino ArduBlinds.ino

MESSAGE=`cpplint --recursive --extensions=ino ArduBlinds.ino 2>&1 | grep "Total errors found"`

if [ "${MESSAGE}" = "" ]; then
    echo "run_lint.bash: ======================================================================="
    echo "run_lint.bash: linting success!";
    echo "run_lint.bash: ======================================================================="
    exit 0;
else
    echo "run_lint.bash: ======================================================================="
    echo "run_lint.bash: linting failure :      ${MESSAGE}";
    echo "run_lint.bash: ======================================================================="
    exit 1;
fi
