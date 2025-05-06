# Stress Testing Scripts for webserv

## Correction Requirements

The webserv project must meet the following criteria:

1. **Availability should be above 99.5%** for a simple GET on an empty page with a siege -b on that page.
2. **No memory leaks** (Monitor the process memory usage. It should not go up indefinitely).
3. **No hanging connections**.
4. **Ability to use siege indefinitely** without having to restart the server.

## Available Test Scripts

### 1. Simple Stress Test

This script runs a basic stress test on a specified port using Siege. It:
- Checks if the server is running and starts it if needed
- Monitors memory usage before and after the test
- Runs Siege with concurrent connections
- Checks for server stability and hanging connections

**Usage:**
```bash
cd tests
chmod +x simple_stress_test.sh
./simple_stress_test.sh [PORT]
```

If no port is specified, it defaults to 2222.

### 2. Complete Stress Test 

This script runs a series of tests to thoroughly evaluate the server's performance:
- Basic availability test
- Multiple concurrent connections test
- Multiple port testing
- Different HTTP methods (GET, POST, DELETE)
- Different file types (HTML, CSS, images)
- Error handling
- Long-running stability test

**Usage:**
```bash
cd tests
chmod +x complete_stress_test.sh
./complete_stress_test.sh
```

## Interpreting Results

### Availability

Siege will output availability as a percentage. Look for the "Availability" field in the Siege output. It should be above 99.5% to meet the requirements.

Example:
```
Availability:       100.00 %
```

### Memory Leaks

The test scripts monitor memory usage before and after testing. If the memory usage increases significantly after each test, it may indicate a memory leak.

Look for memory usage (VSZ column) in the output:
```
  PID MEMORY   ELAPSED  CPU
12345  54321   01:23:45 0.5
```

If the memory usage increases steadily over time without decreasing after request handling, it suggests a memory leak.

### Hanging Connections

The scripts check for hanging connections by counting open connections to the server port after the tests are complete. A high number of connections that remain open after the test has finished may indicate hanging connections.

### Server Stability

The scripts verify that the server is still running after the tests. If the server crashes during testing, it will be reported in the output.

## How to Fix Common Issues

### Low Availability
- Check error handling in the code
- Ensure proper socket cleanup
- Verify that the server correctly handles concurrent connections

### Memory Leaks
- Check for proper deallocation of resources
- Make sure all opened file descriptors are closed
- Review dynamic memory allocation (new/delete, malloc/free)

### Hanging Connections
- Ensure sockets are properly closed after use
- Check timeout handling in the server
- Verify that the server correctly handles client disconnections

### Server Crashes
- Implement proper signal handling
- Add robust error checking
- Make sure buffer sizes are adequate to prevent overflows

## Example Successful Test Output

A successful test should show:
- Availability above 99.5%
- Stable memory usage
- No hanging connections
- Server remaining operational throughout the tests

```
=== Webserv Stress Test ===
Testing server on port: 2222
Server already running on port 2222

Monitoring memory usage of the webserv process...
Initial memory usage:
  PID MEMORY   ELAPSED  CPU
12345  54321   00:10:25 0.5
Final memory usage:
  PID MEMORY   ELAPSED  CPU
12345  54876   00:11:25 1.2

Running Siege test with 50 concurrent users for 60 seconds...
** SIEGE 4.0.4
** Preparing 50 concurrent users for battle.
The server is now under siege...
Lifting the server siege...
Transactions:               12345 hits
Availability:               100.00 %
Elapsed time:               59.99 secs
Data transferred:           123.45 MB
Response time:              0.24 secs
Transaction rate:           205.79 trans/sec
Throughput:                 2.06 MB/sec
Concurrency:                49.87
Successful transactions:    12345
Failed transactions:        0
Longest transaction:        1.23
Shortest transaction:       0.01

Analyzing test results...
✓ Server is still running after the test
✓ No hanging connections detected (3 open connections)

Test completed. Check the Siege output for availability percentage.
Availability should be above 99.5% to pass the correction requirements.

=== Test Completed ===
```