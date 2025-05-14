#!/bin/bash

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
RESET="\033[0m"


TEST_DURATION=60 
SERVER_PORT=2222
SERVER_HOST="localhost"
CONCURRENT_USERS=50
REQUEST_PATH="/"
CONFIG_FILE="./config/webserv.conf"


if [ "$1" != "" ]; then
    SERVER_PORT=$1
fi

check_server() {
    if ! netstat -tuln | grep ":$SERVER_PORT " > /dev/null; then
        echo -e "${YELLOW}Server not running on port $SERVER_PORT. Starting server...${RESET}"
        cd ..
        ./webserv $CONFIG_FILE > ./server_log.txt 2>&1 &
        SERVER_PID=$!
        echo -e "${GREEN}Server started with PID: $SERVER_PID${RESET}"
        sleep 2 
        cd tests
    else
        echo -e "${GREEN}Server already running on port $SERVER_PORT${RESET}"
    fi
}

run_siege_test() {
    echo -e "\n${YELLOW}Running Siege test with $CONCURRENT_USERS concurrent users for $TEST_DURATION seconds...${RESET}"
    siege -b -t${TEST_DURATION}s -c${CONCURRENT_USERS} http://$SERVER_HOST:$SERVER_PORT$REQUEST_PATH
}

monitor_memory() {
    echo -e "\n${YELLOW}Monitoring memory usage of the webserv process...${RESET}"
    
    WEBSERV_PID=$(pgrep webserv)
    
    if [ -z "$WEBSERV_PID" ]; then
        echo -e "${RED}Cannot find webserv process!${RESET}"
        return
    fi
    
    echo "Initial memory usage:"
    ps -p $WEBSERV_PID -o pid,vsz=MEMORY,etime=ELAPSED,pcpu=CPU
    
    sleep $TEST_DURATION
    
    echo "Final memory usage:"
    ps -p $WEBSERV_PID -o pid,vsz=MEMORY,etime=ELAPSED,pcpu=CPU
}

analyze_results() {
    echo -e "\n${YELLOW}Analyzing test results...${RESET}"
    
    if pgrep webserv > /dev/null; then
        echo -e "${GREEN}✓ Server is still running after the test${RESET}"
    else
        echo -e "${RED}✗ Server crashed during the test${RESET}"
    fi
    
    if command -v netstat &> /dev/null; then
        OPEN_CONNECTIONS=$(netstat -tn | grep ":$SERVER_PORT " | wc -l)
        
        echo -e "${YELLOW}Waiting 5 seconds for connections to close naturally...${RESET}"
        sleep 5
        
        OPEN_CONNECTIONS_AFTER_WAIT=$(netstat -tn | grep ":$SERVER_PORT " | wc -l)
        
        if [ "$OPEN_CONNECTIONS_AFTER_WAIT" -gt 10 ]; then
            echo -e "${RED}✗ Potential hanging connections detected: $OPEN_CONNECTIONS_AFTER_WAIT connections are still open${RESET}"
            echo -e "${YELLOW}First 10 hanging connections:${RESET}"
            netstat -tn | grep ":$SERVER_PORT " | head -10
        else
            echo -e "${GREEN}✓ No hanging connections detected ($OPEN_CONNECTIONS_AFTER_WAIT open connections)${RESET}"
        fi
    else
        echo -e "${YELLOW}⚠️ netstat command not found. Cannot check for hanging connections.${RESET}"
    fi
    
    echo -e "\n${YELLOW}Test completed. Availability should be above 99.5%.${RESET}"
}

echo -e "${GREEN}=== Webserv Stress Test ===${RESET}"
echo -e "Testing server on port: $SERVER_PORT"


check_server
monitor_memory &
MONITOR_PID=$!
run_siege_test
kill $MONITOR_PID 2>/dev/null
analyze_results

echo -e "\n${GREEN}=== Test Completed ===${RESET}"

