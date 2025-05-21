#pragma once
#include <iostream>
#define LOG_INFO(msg)  std::cerr << FGRN("[INFO] ")  << msg << std::endl
#define LOG_DEBUG(msg) std::cerr << FRED("[DEBUG] ") << msg << std::endl
