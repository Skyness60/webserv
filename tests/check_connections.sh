#!/bin/bash
# Script to check for hanging connections in webserv

PORT=2222 
SERVER_PID=$(pgrep webserv)

echo "=== Checking for hanging connections on port $PORT ==="
echo

echo "Active connections to webserv by port:"

ss -tan "sport = :$PORT or dport = :$PORT" | grep -v LISTEN

echo
echo "Connection counts by state:"
ss -tan "sport = :$PORT or dport = :$PORT" | awk 'NR>1 {print $1}' | sort | uniq -c

echo
echo "Total socket connections for webserv process (PID: $SERVER_PID):"
if [ ! -z "$SERVER_PID" ]; then
    ls -l /proc/$SERVER_PID/fd 2>/dev/null | grep socket | wc -l
else
    echo "webserv process not found"
fi

echo
echo "=== Running stress test ==="

siege -c50 -t10S -q http://localhost:$PORT/

echo
echo "=== Checking for hanging connections after test ==="
sleep 2

echo "Active connections to webserv after test by port:"
ss -tan "sport = :$PORT or dport = :$PORT" | grep -v LISTEN

echo
echo "Connection counts by state after test:"
ss -tan "sport = :$PORT or dport = :$PORT" | awk 'NR>1 {print $1}' | sort | uniq -c

echo
echo "Socket connections for webserv process after test:"
if [ ! -z "$SERVER_PID" ]; then
    ls -l /proc/$SERVER_PID/fd 2>/dev/null | grep socket | wc -l
else
    echo "webserv process not found"
fi

echo
echo "=== ANALYSIS ==="
ESTABLISHED=$(ss -tan "sport = :$PORT or dport = :$PORT" | grep ESTAB | wc -l)
CLOSE_WAIT=$(ss -tan "sport = :$PORT or dport = :$PORT" | grep CLOSE-WAIT | wc -l)

echo "Established connections: $ESTABLISHED"
echo "Close-Wait connections: $CLOSE_WAIT"

if [ $ESTABLISHED -gt 0 ] || [ $CLOSE_WAIT -gt 0 ]; then
    echo "POSSIBLE HANGING CONNECTIONS DETECTED!"
    echo "There are still connections in ESTABLISHED or CLOSE-WAIT state after test."
    echo "This may indicate your server is not properly closing connections."
else
    echo "No hanging connections detected."
    echo "Your server appears to be handling connection cleanup properly."
fi