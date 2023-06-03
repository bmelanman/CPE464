#!/bin/bash

# Setup server and client calls --------------------
APP_SERVER=server
APP_CLIENT=rcopy
PIDOF=$(which pidof)

# Functions ----------------------------------------
function usage {
    echo "usage: $0 -p port [-t test_number | -f test_file] [-e error_rate] [-h host_name]"
    exit 1
}

function clean_up {
    echo "Ensuring Apps are closed"
    kill -9 "$($PIDOF $APP_CLIENT)" &>/dev/null || :
    kill -9 "$($PIDOF $APP_SERVER)" &>/dev/null || :

    rm -f "$FILEOUT" "$APP_SERVER" "$APP_CLIENT"

    exit
}

trap clean_up SIGHUP SIGINT SIGTERM SIGQUIT

# Start --------------------------------------------

# Runtime args check
if [ $# -lt 2 ]; then
    usage
fi

PORT=0
TEST_CASE=0
FILE=''
WIN=10
SIZE=1000
ERROR=0.0
HOSTNAME=localhost

while getopts ':p:f:e:h:t:h:w:b:' flag; do
    case "${flag}" in
    p) PORT=$OPTARG ;;
    f) FILE=$OPTARG ;;
    e) ERROR=$OPTARG ;;
    h) HOSTNAME=$OPTARG ;;
    w) WIN=$OPTARG ;;
    b) SIZE=$OPTARG ;;
    t) # If a test case is specified, no other inputs are needed
        TEST_CASE="$OPTARG"
        break
        ;;
    *)
        usage
        ;;
    esac
done

if [ "$PORT" -eq 0 ]; then
    echo "A port must be specified!"
    exit 1
fi

# Generate output file name
FILEOUT=$(basename "$FILE").tmp

# Make sure everything is up to date
if [ ! -e APP_CLIENT ] || [ ! -e APP_SERVER ]; then

    # Check for the custom library
    if [ ! -e ../libcpe464.2.21.a ]; then
        make libcpe -C ../ -f Makefile
    fi

    make all -C ../ -f Makefile

    mv ../$APP_CLIENT ./${APP_CLIENT}_"$PORT"
    mv ../$APP_SERVER ./${APP_SERVER}_"$PORT"
    make cleano -C ../ -f Makefile
fi

APP_CLIENT=${APP_CLIENT}_$PORT
APP_SERVER=${APP_SERVER}_$PORT

clear

# Display settings before running
echo "----------- ARGS -----------"

# Check for test case
if [ "$TEST_CASE" -ne 0 ]; then

    case ${TEST_CASE} in
    1) # Test 1: File Size = ~900 Bytes, Window Length = 10, Buffer Size = 1000, Error Rate = 0.20
        FILE="./test_files/small.txt"
        WIN=10
        SIZE=1000
        ERROR=0.20
        ;;

    2) # Test 2: File Size = ~50k Bytes, Window Length = 10, Buffer Size = 1000, Error Rate = 0.20
        FILE="./test_files/medium.txt"
        WIN=10
        SIZE=1000
        ERROR=0.20
        ;;

    3) # Test 3: File Size = ~420k Bytes, Window Length = 50, Buffer Size = 1000, Error Rate = 0.10
        FILE="./test_files/large.txt"
        WIN=50
        SIZE=1000
        ERROR=0.10
        ;;

    4) # Test 4: File Size = ~420k Bytes, Window Length = 5, Buffer Size = 1000, Error Rate = 0.15
        FILE="./test_files/large.txt"
        WIN=5
        SIZE=1000
        ERROR=0.15
        ;;

    *) # Default: Send error and quit
        echo "Invalid test case selected! Use 1-4 to select the respective test case on the grading sheet."
        exit 1
        ;;
    esac

    echo "Test Case: $TEST_CASE"

elif [ "$FILE" == "" ]; then
    echo "If a test case is not specified, a test file must be given!"
    exit 1
fi

echo "Port: $PORT"
echo "File: $FILE"
echo "Window Length: $WIN"
echo "Buffer Size: $SIZE"
echo "Error Rate: $ERROR"
echo "Hostname: $HOSTNAME"

# Run the server in the background
echo "---------- SERVER ----------"
./"$APP_SERVER" "$ERROR" "$PORT" &

sleep 2

echo "---------- CLIENT ----------"
./"$APP_CLIENT" "$FILE" "$FILEOUT" "$WIN" "$SIZE" "$ERROR" "$HOSTNAME" "$PORT"
APP_CLIENT_RES=$?

echo "---------- RESULTS ----------"

if [ $APP_CLIENT_RES -ne 0 ]; then
    echo "- Client Returned $APP_CLIENT_RES"
fi

# Check server status
SERV_PID=$("$PIDOF" "$APP_SERVER")

if [ "$(echo "$SERV_PID" | wc -w)" -eq 0 ]; then

    echo "- Server closed early"

elif [ "$(echo "$SERV_PID" | wc -w)" -gt 1 ]; then

    echo "- Waiting for Server children to close"

    # Check if the child processes close
    for i in {1..11}; do
        if [ "$(echo "$SERV_PID" | wc -w)" -gt 1 ]; then
            echo "."
            sleep 1
            SERV_PID=$("$PIDOF" "$APP_SERVER")
        else
            break
        fi
    done

    if [ "$(echo "$SERV_PID" | wc -w)" -gt 1 ]; then

        echo "-- Children didn't close"

        kill -9 "$(pgrep "$APP_SERVER")" &>/dev/null || :

    fi
fi

if [ "$(echo "$SERV_PID" | wc -w)" -gt 0 ]; then

    kill -9 "$SERV_PID" &>/dev/null || :
    echo "- Waiting for Server to close"

    # shellcheck disable=SC2034
    for i in {1..3}; do
        if [ "$(echo "$SERV_PID" | wc -w)" -gt 0 ]; then
            echo "."
            sleep 1
            SERV_PID=$($PIDOF "$APP_SERVER")
        else
            break
        fi
    done

    if [ "$(echo "$SERV_PID" | wc -w)" -gt 0 ]; then

        echo "- Server Didn't Successfully Close"
        kill -9 "$SERV_PID" &>/dev/null || :

    fi
fi

echo "---------- DIFF (IN | OUT) ----------"

diff -qs "$FILE" "$FILEOUT"
RETVAL=$?

clean_up

exit $RETVAL
