#pragma once

#include <csignal>

class SignalHandler {
public:
    static void setupSignalHandlers();
    static bool stopServer;
};
