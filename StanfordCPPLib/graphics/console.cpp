/*
 * File: console.cpp
 * -----------------
 * This file implements the console .h interface.
 *
 * @author Marty Stepp
 * @version 2018/10/01
 * - bug fix for graphical console popping up even if not included
 * @version 2018/08/23
 * - renamed to console .cpp/h to replace Java version
 * - separated out gconsolewindow.h/cpp
 * @version 2018/07/15
 * - initial version, based on io/console .cpp
 */

#define INTERNAL_INCLUDE 1
#include "consoletext.h"
#include <cstdio>
#define INTERNAL_INCLUDE 1
#include "error.h"
#define INTERNAL_INCLUDE 1
#include "exceptions.h"
#define INTERNAL_INCLUDE 1
#include "filelib.h"
#define INTERNAL_INCLUDE 1
#include "gconsolewindow.h"
#define INTERNAL_INCLUDE 1
#include "gthread.h"
#define INTERNAL_INCLUDE 1
#include "private/static.h"
#define INTERNAL_INCLUDE 1
#include "private/version.h"
#undef INTERNAL_INCLUDE

void clearConsole() {
    GConsoleWindow::instance()->clearConsole();
}

bool getConsoleClearEnabled() {
    return GConsoleWindow::instance()->isClearEnabled();
}

/* GWindow::CloseOperation */ int getConsoleCloseOperation() {
    return GConsoleWindow::instance()->getCloseOperation();
}

bool getConsoleEcho() {
    return GConsoleWindow::instance()->isEcho();
}

bool getConsoleEnabled() {
#ifdef __DONT_ENABLE_QT_GRAPHICAL_CONSOLE
    return false;
#else
    return GConsoleWindow::consoleEnabled();
#endif
}

bool getConsoleEventOnClose() {
    return true;
}

bool getConsoleExitProgramOnClose() {
    return GConsoleWindow::instance()->getCloseOperation() == GWindow::CLOSE_EXIT;
}

std::string getConsoleFont() {
#ifdef __DONT_ENABLE_QT_GRAPHICAL_CONSOLE
    return GConsoleWindow::getDefaultFont();
#else
    return GConsoleWindow::instance()->getFont();
#endif
}

double getConsoleHeight() {
    return GConsoleWindow::instance()->getHeight();
}

GPoint getConsoleLocation() {
    return GConsoleWindow::instance()->getLocation();
}

bool getConsoleLocationSaved() {
    return GConsoleWindow::instance()->isLocationSaved();
}

bool getConsolePrintExceptions() {
    return exceptions::getTopLevelExceptionHandlerEnabled();
}

bool getConsoleSettingsLocked() {
    return GConsoleWindow::isInitialized()
            && GConsoleWindow::instance()->isLocked();
}

GDimension getConsoleSize() {
    return GConsoleWindow::instance()->getSize();
}

double getConsoleWidth() {
    return GConsoleWindow::instance()->getWidth();
}

GConsoleWindow* getConsoleWindow() {
    return GConsoleWindow::instance();
}

std::string getConsoleWindowTitle() {
    return GConsoleWindow::instance()->getTitle();
}

void setConsoleClearEnabled(bool value) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setClearEnabled(value);
}

void setConsoleCloseOperation(/*GWindow::CloseOperation*/ int op) {
    GWindow::CloseOperation gwcop = static_cast<GWindow::CloseOperation>(op);
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setCloseOperation(gwcop);
}

void setConsoleEcho(bool echo) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setEcho(echo);
}

void setConsoleErrorColor(const std::string& color) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setErrorColor(color);
}

void setConsoleEventOnClose(bool /*eventOnClose*/) {
    // empty
}

void setConsoleExitProgramOnClose(bool exitOnClose) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setExitOnClose(exitOnClose);
}

void setConsoleFont(const std::string& font) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setFont(font);
}

void setConsoleLocation(double x, double y) {
    if (getConsoleSettingsLocked()) { return; }
    if (floatingPointEqual(x, -1) && floatingPointEqual(y, -1)) {
        GConsoleWindow::instance()->center();
    } else {
        GConsoleWindow::instance()->setLocation(x, y);
    }
}

void setConsoleLocationSaved(bool value) {
    GConsoleWindow::instance()->setLocationSaved(value);
}

void setConsoleOutputColor(const std::string& color) {
    GConsoleWindow::instance()->setOutputColor(color);
}

void setConsolePrintExceptions(bool printExceptions, bool force) {
    if (getConsoleSettingsLocked()) { return; }
    exceptions::setTopLevelExceptionHandlerEnabled(printExceptions, force);
}

void setConsoleSettingsLocked(bool value) {
    GConsoleWindow::instance()->setLocked(value);
}

void setConsoleSize(double width, double height) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setConsoleSize(width, height);
}

void setConsoleWindowTitle(const std::string& title) {
    if (getConsoleSettingsLocked()) { return; }
    GConsoleWindow::instance()->setTitle(title);
}

void shutdownConsole() {
    if (getConsoleEnabled() && !GConsoleWindow::instance()->isLocked()) {
        GConsoleWindow::instance()->shutdown();
    }
}

/*
 * Sets up console settings like window size, location, exit-on-close, etc.
 * based on compiler options set in the .pro file.
 */
void setConsolePropertiesQt() {
#if defined(SPL_CONSOLE_FONTSIZE)
    std::string fontStr = std::string("Monospaced-Bold-") + to_string(SPL_CONSOLE_FONTSIZE);
    setConsoleFont(fontStr);
#endif

#if defined(SPL_CONSOLE_WIDTH) && defined(SPL_CONSOLE_HEIGHT)
    setConsoleSize(SPL_CONSOLE_WIDTH, SPL_CONSOLE_HEIGHT);
#endif

#if defined(SPL_CONSOLE_X) && defined(SPL_CONSOLE_Y)
    setConsoleLocation(SPL_CONSOLE_X, SPL_CONSOLE_Y);
#endif

#if defined(SPL_CONSOLE_ECHO)
    setConsoleEcho(true);
#endif

#if defined(SPL_CONSOLE_EXIT_ON_CLOSE)
    setConsoleExitProgramOnClose(true);
#endif

#if defined(SPL_CONSOLE_LOCATION_SAVED)
    setConsoleLocationSaved(true);
#endif

#if defined(SPL_CONSOLE_PRINT_EXCEPTIONS)
    setConsolePrintExceptions(true);
#endif

#if defined(SPL_VERIFY_PROJECT_VERSION)
    version::ensureProjectVersion();
#endif
}

void initializeQtGraphicalConsole() {
#ifndef __DONT_ENABLE_QT_GRAPHICAL_CONSOLE
    // ensure that console is initialized only once
    static bool _initialized = false;
    if (_initialized) {
        return;
    }
    _initialized = true;

    // declaring this object ensures that std::cin, cout, cerr are initialized
    // properly before our lib tries to mess with them / redirect them
    static std::ios_base::Init ios_base_init;

    if (GConsoleWindow::consoleEnabled()) {
        GConsoleWindow::instance();   // ensure that console window is ready
        setConsolePropertiesQt();
    }


#endif // __DONT_ENABLE_QT_GRAPHICAL_CONSOLE
}

// This one is at the bottom because it's not meant to be called by students.
void setConsoleEnabled(bool enabled) {
    GConsoleWindow::setConsoleEnabled(enabled);
}
