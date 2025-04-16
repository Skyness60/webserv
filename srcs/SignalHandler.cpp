#include "SignalHandler.hpp"

bool SignalHandler::stopServer = false;

void SignalHandler::setupSignalHandlers() {
    signal(SIGINT, [](int) { stopServer = true; });
    signal(SIGTERM, [](int) { stopServer = true; });
}
