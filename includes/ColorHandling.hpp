#pragma once

#include <iostream>
#include <sstream>
#include <string>

/* ************************************************************************** */
/*                                 FOREGROUND                                 */
/* ************************************************************************** */

// color handling
#define BOLD_BLACK   "\033[1;30m"
#define BOLD_RED     "\033[1;31m"
#define BOLD_GREEN   "\033[1;32m"
#define BOLD_YELLOW  "\033[1;33m"
#define BOLD_BLUE    "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN    "\033[1;36m"
#define BOLD_WHITE   "\033[1;37m"

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define FRED(x) (std::string(RED) + toString(x) + std::string(RESET))
#define FGRN(x) (std::string(GREEN) + toString(x) + std::string(RESET))
#define FYEL(x) (std::string(YELLOW) + toString(x) + std::string(RESET))
#define FBLU(x) (std::string(BLUE) + toString(x) + std::string(RESET))
#define FMAG(x) (std::string(MAGENTA) + toString(x) + std::string(RESET))
#define FCYN(x) (std::string(CYAN) + toString(x) + std::string(RESET))
#define FWHT(x) (std::string(WHITE) + toString(x) + std::string(RESET))

/* ************************************************************************** */
/*                                   BACKGROUND                               */
/* ************************************************************************** */

#define KBLACK "\033[40m"
#define KRED "\033[41m"
#define KGREEN "\033[42m"
#define KYELLOW "\033[43m"
#define KBLUE "\033[44m"
#define KMAGENTA "\033[45m"
#define KCYAN "\033[46m"
#define KWHITE "\033[47m"

#define BRED(x) (std::string(KRED) + toString(x) + std::string(RESET))
#define BGRN(x) (std::string(KGREEN) + toString(x) + std::string(RESET))
#define BYEL(x) (std::string(KYELLOW) + toString(x) + std::string(RESET))
#define BBLU(x) (std::string(KBLUE) + toString(x) + std::string(RESET))
#define BMAG(x) (std::string(KMAGENTA) + toString(x) + std::string(RESET))
#define BCYN(x) (std::string(KCYAN) + toString(x) + std::string(RESET))
#define BWHT(x) (std::string(KWHITE) + toString(x) + std::string(RESET))


/* ************************************************************************** */
/*                                   STYLES                                   */
/* ************************************************************************** */

#define BOLD(x) (std::string("\033[1m") + toString(x) + std::string(RESET))
#define ITALIC(x) (std::string("\033[3m") + toString(x) + std::string(RESET))
#define UNDERLINE(x) (std::string("\033[4m") + toString(x) + std::string(RESET))
#define BLINK(x) (std::string("\033[5m") + toString(x) + std::string(RESET))

/* ************************************************************************** */
/*                                     UTILS                                  */
/* ************************************************************************** */

template <typename T>
std::string toString(const T &value) {

    std::stringstream ss;
    ss << value;
    return ss.str();
}