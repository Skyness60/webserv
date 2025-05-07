#!/bin/bash

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
RESET="\033[0m"

# Configuration
CONFIG_FILE="./config/webserv.conf"
SERVER_HOST="localhost"
BASE_PORT=2222
SECOND_PORT=2282

make_scripts_executable() {
    chmod +x ./complete_stress_test.sh
}

test_availability() {
    echo -e "\n${BLUE}=== Testing Basic Availability (should be >99.5%) ===${RESET}"
    ./complete_stress_test.sh $BASE_PORT
}

test_concurrent_connections() {
    echo -e "\n${BLUE}=== Testing with 100 Concurrent Connections ===${RESET}"
    siege -b -t30s -c100 http://$SERVER_HOST:$BASE_PORT/
}

test_multiple_ports() {
    echo -e "\n${BLUE}=== Testing Multiple Server Ports ===${RESET}"
    echo -e "${YELLOW}Testing Port $BASE_PORT${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$BASE_PORT/
    
    echo -e "\n${YELLOW}Testing Port $SECOND_PORT${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$SECOND_PORT/
}

test_http_methods() {
    echo -e "\n${BLUE}=== Testing Different HTTP Methods ===${RESET}"
    
    echo -e "${YELLOW}Testing GET Method${RESET}"
    siege -g http://$SERVER_HOST:$BASE_PORT/
    
    echo -e "\n${YELLOW}Testing POST Method${RESET}"
    curl -X POST -d "content=test_content" http://$SERVER_HOST:$BASE_PORT/createFile.html
    
    echo -e "\n${YELLOW}Testing DELETE Method${RESET}"
    curl -X DELETE http://$SERVER_HOST:$BASE_PORT/Playlist/
}

test_file_types() {
    echo -e "\n${BLUE}=== Testing Different File Types ===${RESET}"
    
    echo -e "${YELLOW}Testing HTML Files${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$BASE_PORT/index.html
    
    echo -e "\n${YELLOW}Testing CSS Files${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$BASE_PORT/style.css
    
    echo -e "\n${YELLOW}Testing Image Files${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$BASE_PORT/images/photo.jpg
}

test_error_handling() {
    echo -e "\n${BLUE}=== Testing Error Handling ===${RESET}"
    
    # 404 Error
    echo -e "${YELLOW}Testing 404 Error${RESET}"
    siege -b -t10s -c10 http://$SERVER_HOST:$BASE_PORT/nonexistent.html
    
    # 405 Error (Method Not Allowed)
    echo -e "\n${YELLOW}Testing 405 Error${RESET}"
    curl -X PUT http://$SERVER_HOST:$BASE_PORT/
}

test_long_running() {
    echo -e "\n${BLUE}=== Starting Long-Running Test (2 minutes) ===${RESET}"
    echo -e "${YELLOW}This test will run for 2 minutes with varied load to check stability${RESET}"
    
    WEBSERV_PID=$(pgrep webserv)
    echo "Initial memory usage:"
    ps -p $WEBSERV_PID -o pid,vsz=MEMORY,etime=ELAPSED,pcpu=CPU
    
    siege -b -t120s -c$(( RANDOM % 50 + 10 )) http://$SERVER_HOST:$BASE_PORT/
    
    echo "Final memory usage:"
    ps -p $WEBSERV_PID -o pid,vsz=MEMORY,etime=ELAPSED,pcpu=CPU
    
    echo -e "${YELLOW}Checking for hanging connections...${RESET}"
    OPEN_CONNECTIONS=$(netstat -tn | grep ":$BASE_PORT " | wc -l)
    if [ "$OPEN_CONNECTIONS" -gt 100 ]; then
        echo -e "${RED}✗ Potential hanging connections detected: $OPEN_CONNECTIONS connections are still open${RESET}"
    else
        echo -e "${GREEN}✓ No hanging connections detected ($OPEN_CONNECTIONS open connections)${RESET}"
    fi
}

echo -e "${GREEN}=== Webserv Comprehensive Stress Test ===${RESET}"

make_scripts_executable

cd ..
./webserv $CONFIG_FILE &
SERVER_PID=$!
echo -e "${GREEN}Server started with PID: $SERVER_PID${RESET}"
sleep 2  
cd tests

test_availability
test_concurrent_connections
test_multiple_ports
test_http_methods
test_file_types
test_error_handling
test_long_running

echo -e "\n${GREEN}=== All Tests Completed ===${RESET}"
echo -e "${YELLOW}You can analyze the results to see if the server meets the correction requirements:${RESET}"
echo -e "${YELLOW}1. Availability should be above 99.5%${RESET}"
echo -e "${YELLOW}2. No memory leaks (memory usage shouldn't increase indefinitely)${RESET}"
echo -e "${YELLOW}3. No hanging connections${RESET}"
echo -e "${YELLOW}4. Server should be stable during continuous operation${RESET}"

read -p "Do you want to terminate the webserv process? (y/n): " kill_server
if [ "$kill_server" = "y" ]; then
    echo -e "${YELLOW}Terminating webserv process...${RESET}"
    pkill webserv
    echo -e "${GREEN}Webserv process terminated.${RESET}"
fi
