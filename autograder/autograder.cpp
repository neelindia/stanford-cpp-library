/*
 * File: autograder.cpp
 * --------------------
 * This file contains implementations of utility code used to help implement
 * autograder programs for grading student assignments.
 * See autograder.h for documentation of each member.
 * 
 * @author Marty Stepp
 * @version 2014/11/04
 * - made showLateDays dialog be 'plain' (no icon)
 * @version 2014/10/31
 * - added graphical autograder support (setGraphicalUI)
 * @since 2014/10/14
 */

#include "autograder.h"
#include <cstdio>
#include "console.h"
#include "date.h"
#include "filelib.h"
#include "gevents.h"
#include "ginteractors.h"
#include "goptionpane.h"
#include "gtest-marty.h"
#include "gwindow.h"
#include "inputpanel.h"
#include "ioutils.h"
#include "map.h"
#include "platform.h"
#include "simpio.h"
#include "stringutils.h"
#include "stylecheck.h"

namespace autograder {
static const std::string DEFAULT_ABOUT_TEXT = "CS 106 B/X Autograder Framework\nDeveloped by Marty Stepp (stepp@cs.stanford.edu)";
static Platform* pp = getPlatform();

struct CallbackButtonInfo {
    void (* func)();
    std::string text;
    std::string icon;
};

struct AutograderFlags {
public:
    std::string assignmentName;
    std::string studentProgramFileName;
    std::string currentCategoryName;
    std::string currentTestCaseName;
    std::string inputPanelFilename;
    std::string startMessage;
    std::string aboutText;
    int failsToPrintPerTest;
    int testNameWidth;
    bool showInputPanel;
    bool showLateDays;
    bool graphicalUI;
    Vector<std::string> styleCheckFiles;
    Map<std::string, std::string> styleCheckFileMap;
    void(* callbackStart)();
    void(* callbackEnd)();
    Vector<CallbackButtonInfo> callbackButtons;
    
    AutograderFlags() {
        assignmentName = "";
        studentProgramFileName = "";
        currentCategoryName = "";
        currentTestCaseName = "";
        startMessage = "";
        aboutText = DEFAULT_ABOUT_TEXT;
        failsToPrintPerTest = 1;
        testNameWidth = -1;
        showInputPanel = true;
        inputPanelFilename = INPUT_PANE_FILENAME;
        showLateDays = true;
        graphicalUI = false;
        callbackStart = NULL;
        callbackEnd = NULL;
    }
};
static AutograderFlags FLAGS;

void addCallbackButton(void (* func)(), const std::string& text, const std::string& icon) {
    CallbackButtonInfo buttonInfo;
    buttonInfo.func = func;
    buttonInfo.text = text;
    buttonInfo.icon = icon;
    FLAGS.callbackButtons.add(buttonInfo);
}

void displayDiffs(const std::string& expectedOutput, const std::string& studentOutput,
                  const std::string& diffs, const std::string& diffFile, int truncateHeight) {
    if (FLAGS.graphicalUI) {
        
    } else {
        std::cout << "FAIL!" << std::endl;
        int expectedHeight = stringutils::height(expectedOutput);
        int studentHeight  = stringutils::height(studentOutput);
        if (expectedHeight != studentHeight) {
            std::cout << "  Expected " << expectedHeight << " lines,"
                      << "found " << studentHeight << " lines." << std::endl;
        }
        std::cout << "      DIFFS:";
        if (!diffFile.empty()) {
            std::cout << " (see complete output in " << diffFile << " in build folder)";
        }
        std::cout << std::endl;
        std::string diffsToShow = diffs;
        if (truncateHeight > 0) {
            diffsToShow = stringutils::trimToHeight(diffsToShow, truncateHeight);
        }
        std::cout << stringutils::indent(diffsToShow, 8) << std::endl;
    }
}

std::string getCurrentCategoryName() {
    return FLAGS.currentCategoryName;
}

std::string getCurrentTestCaseName() {
    return FLAGS.currentTestCaseName;
}

bool isGraphicalUI() {
    return FLAGS.graphicalUI;
}

std::string runAndCapture(int (* mainFunc)(),
                          const std::string& cinInput,
                          const std::string& outputFileName) {
    // run the 'main' function, possibly feeding it cin, input, and capture its cout output
    if (!cinInput.empty()) {
        ioutils::redirectStdinBegin(cinInput);
    }
    
    ioutils::setConsoleOutputLimit(MAX_STUDENT_OUTPUT);   // prevent infinite output by student
    ioutils::captureStdoutBegin();
    
    mainFunc();
    
    std::string output = ioutils::captureStdoutEnd();
    if (!cinInput.empty()) {
        ioutils::redirectStdinEnd();
    }

    // return the output as a string (and also possibly write it to a file)
    if (!outputFileName.empty()) {
        writeEntireFile(outputFileName, output);
    }
    return output;
}


void setAboutMessage(const std::string& message) {
    FLAGS.aboutText = DEFAULT_ABOUT_TEXT
            + "\n==================================\n"
            + message;
}

void setAssignmentName(const std::string& name) {
    FLAGS.assignmentName = name;
}

void setCallbackEnd(void (*func)()) {
    FLAGS.callbackEnd = func;
}

void setCallbackStart(void (*func)()) {
    FLAGS.callbackStart = func;
}

void setCurrentCategoryName(const std::string& categoryName) {
    FLAGS.currentCategoryName = categoryName;
}

void setCurrentTestCaseName(const std::string& testName) {
    FLAGS.currentTestCaseName = testName;
}

void setFailDetails(const autograder::UnitTestDetails& deets) {
    if (FLAGS.graphicalUI) {
        MartyGraphicalTestResultPrinter::setFailDetails(deets);
    } else {
        MartyTestResultPrinter::setFailDetails(deets);
    }
}

void setFailsToPrintPerTest(int count) {
    FLAGS.failsToPrintPerTest = count;
}

void setGraphicalUI(bool value) {
    FLAGS.graphicalUI = value;
}

void setShowInputPanel(bool show, const std::string& filename) {
    FLAGS.showInputPanel = show;
    if (!filename.empty()) {
        FLAGS.inputPanelFilename = filename;
    }
}

/*
 * Toggles whether to show the late_days.txt info for each student.
 */
void setShowLateDays(bool show) {
    FLAGS.showLateDays = show;
}

void setShowTestCaseFailDetails(bool /*show*/) {
    // showTestCaseFailDetails = show;
}

void setStartMessage(const std::string& startMessage) {
    FLAGS.startMessage = startMessage;
}

void setStudentProgramFileName(const std::string& filename) {
    FLAGS.studentProgramFileName = filename;
}

void setTestNameWidth(int width) {
    FLAGS.testNameWidth = width;
}

static std::string formatDate(const std::string& dateStr) {
    // date = "13/Oct/2014 10:31:15"
    int day;
    char cmonth[16];
    int year;
    int hour;
    int minute;
    int second;
    if (sscanf(dateStr.c_str(), "%2d/%3s/%4d %2d:%2d:%2d", &day, cmonth, &year, &hour, &minute, &second) < 6) {
        return "INVALID DATE STRING";
    }
    std::string monthStr = cmonth;
    std::string amPm = (hour < 12) ? "AM" : "PM";
    if (hour > 12) {
        hour -= 12;
    }
    if (hour == 0) {
        hour = 12;
    }
    Date date(year, Date::parseMonth(monthStr), day);
    
    std::ostringstream out;
    out << date.getDayOfWeekName().substr(0, 3) << ", "
        << monthStr << " " << day << ", " << year << ", "
        << hour << ":" << minute << " " << amPm;
    return out.str();
}

/*
 * student_submission_time: 13/Oct/2014 10:31:15
 * assignment_due_time: 13/Oct/2014 23:59:00
 * calendar_days_late: 0
 */
void showLateDays(const std::string& filename) {
    Map<std::string, std::string> lineMap;
    std::string text;
    if (fileExists(filename)) {
        text = readEntireFile(filename);
    } else {
        text = std::string("student_submission_time: unknown\n")
                + "assignment_due_time: unknown\n"
                + "calendar_days_late: unknown\n"
                + "details: " + filename + " not found!";
    }
    
    if (FLAGS.graphicalUI) {
        for (std::string line : stringSplit(text, "\n")) {
            if (stringContains(line, ": ")) {
                std::string key = line.substr(0, line.find(": "));
                std::string value = line.substr(line.find(": ") + 2);
                lineMap.put(key, value);
            }
        }
        
        std::string message;
        std::string dueTime = lineMap["assignment_due_time"];
        std::string submitTime = lineMap["student_submission_time"];
        std::string daysLate = lineMap["calendar_days_late"];
        std::string details = lineMap["details"];
        if (dueTime != "unknown") {
            dueTime = formatDate(dueTime);
        }
        if (submitTime != "unknown") {
            submitTime = formatDate(submitTime);
        }
        message += "<html><table>";
        if (!details.empty()) {
            message += "<tr><td><b>NOTE:</b></td><td>" + details + "</td></tr>";
        }
        message += "<tr><td><b>due</b></td><td>" + dueTime + "</td></tr>";
        message += "<tr><td><b>submitted</b></td><td>" + submitTime + "</td></tr>";
        message += "<tr><td><b>cal.days late</b></td><td>" + daysLate + "</td></tr>";
        message += "</table></html>";
        GOptionPane::showMessageDialog(message, filename,
                                       GOptionPane::PLAIN);
    } else {
        std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
        std::cout << "Contents of " << filename << ":" << std::endl;
        std::cout << text;
        if (!endsWith(text, '\n')) {
            std::cout << std::endl;
        }
        std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
    }
}

void showOutput(const std::string& output, bool showIfGraphical, bool showIfConsole) {
    if (isGraphicalUI()) {
        if (showIfGraphical) {
            GOptionPane::showMessageDialog(output);
        }
    } else {
        // not graphical UI
        if (showIfConsole) {
            std::cout << output << std::endl;
        }
    }
}

void showOutput(std::ostringstream& output, bool showIfGraphical, bool showIfConsole) {
    showOutput(output.str(), showIfGraphical, showIfConsole);
    output.str("");   // clear output
}

void showStudentTextFile(const std::string& filename, int maxWidth, int maxHeight) {
    std::string myText = readEntireFile(filename);
    int height = stringutils::height(myText);
    if (maxWidth > 0) {
        myText = stringutils::trimToWidth(myText, maxWidth);
    }
    if (maxWidth > 0) {
        myText = stringutils::trimToHeight(myText, maxHeight);
    }
    if (!endsWith(myText, "\n")) {
        myText += "\n";
    }
    if (FLAGS.graphicalUI) {
        GOptionPane::showTextFileDialog(
                    myText,
                    ///* title */ "Contents of " + filename + " (" + integerToString(height) + " lines)",
                    /* title */ "Contents of " + filename,
                    maxHeight, maxWidth
                    );
    } else {
        // these begin/end markers are kept to the same width
        // as AUTOGRADER_OUTPUT_SEPARATOR from autograder.h
        std::cout << "Here are the contents of the student's " << filename << " file"
                  << " (" << height << " lines):" << std::endl
                  << "============================== BEGIN ===============================" << std::endl
                  << myText
                  << "=============================== END ================================" << std::endl;
    }
}

void styleCheckAddFile(const std::string& filename, const std::string& styleCheckXmlFileName) {
    std::string styleFile = styleCheckXmlFileName;
    if (styleCheckXmlFileName.empty()) {
        // "Foo.cpp" -> "stylecheck-foo-cpp.xml"
        styleFile = toLowerCase(filename);
        styleFile = stringReplace(styleFile, ".", "-");
        styleFile = "stylecheck-" + styleFile + ".xml";
    }
    FLAGS.styleCheckFiles.add(filename);
    FLAGS.styleCheckFileMap.put(filename, styleFile);
}

#ifdef SPL_AUTOGRADER_MODE
static bool autograderYesOrNo(std::string prompt, std::string reprompt = "", std::string defaultValue = "") {
    if (FLAGS.graphicalUI) {
        prompt = stringReplace(prompt, " (Y/n)", "");
        prompt = stringReplace(prompt, " (y/N)", "");
        prompt = stringReplace(prompt, " (y/n)", "");
        prompt = stringReplace(prompt, " (Y/N)", "");
        return GOptionPane::showConfirmDialog(prompt) == GOptionPane::ConfirmResult::YES;
    } else {
        return getYesOrNo(prompt, reprompt, defaultValue);
    }
}


static int mainRunAutograderTestCases(int argc, char** argv) {
    static bool gtestInitialized = false;
    
    // set up a few initial settings and lock-down the program
    ioutils::setConsoleEchoUserInput(true);
    setConsoleClearEnabled(false);
    autograder::gwindowSetPauseEnabled(false);
    autograder::gwindowSetExitGraphicsEnabled(false);
    setConsoleSettingsLocked(true);

    if (!gtestInitialized) {
        gtestInitialized = true;
        ::testing::InitGoogleTest(&argc, argv);

        // set up 'test result printer' to better format test results and errors
        ::testing::TestEventListeners& listeners =
            ::testing::UnitTest::GetInstance()->listeners();
        delete listeners.Release(listeners.default_result_printer());
        if (FLAGS.graphicalUI) {
            autograder::MartyGraphicalTestResultPrinter* graphicalPrinter = new autograder::MartyGraphicalTestResultPrinter();
            listeners.Append(graphicalPrinter);
        } else {
            autograder::MartyTestResultPrinter* printer = new autograder::MartyTestResultPrinter();
            if (FLAGS.testNameWidth > 0) {
                printer->setTestNameWidth(FLAGS.testNameWidth);
            }
            if (FLAGS.failsToPrintPerTest > 0) {
                printer->setFailsToPrintPerTest(FLAGS.failsToPrintPerTest);
            }
            listeners.Append(printer);
        }
    }
    
    int result = RUN_ALL_TESTS();   // run Google Test framework now

    // un-lock-down
    setConsoleSettingsLocked(false);
    ioutils::setConsoleEchoUserInput(false);
    setConsoleClearEnabled(true);
    autograder::gwindowSetPauseEnabled(true);
    autograder::gwindowSetExitGraphicsEnabled(true);

    if (!FLAGS.graphicalUI) {
        std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
        getLine("Press Enter to continue . . .");
    }
    
    return result;
}

static void mainRunStyleChecker() {
    pp->jbeconsole_toFront();
    int styleCheckCount = 0;
    for (std::string filename : FLAGS.styleCheckFiles) {
        stylecheck::styleCheck(
                    filename,
                    FLAGS.styleCheckFileMap[filename],
                     /* printWarning */ styleCheckCount == 0);
        if (!FLAGS.graphicalUI) {
            std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
        }
        styleCheckCount++;
    }
}

int autograderTextMain(int argc, char** argv) {
    std::cout << FLAGS.assignmentName << " Autograder" << std::endl
              << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
    
    // set up buttons to automatically enter user input
    if (FLAGS.showInputPanel) {
        inputpanel::load(FLAGS.inputPanelFilename);
    }
    
    std::string autogradeYesNoMessage = FLAGS.startMessage;
    if (!autogradeYesNoMessage.empty()) {
        autogradeYesNoMessage += "\n\n";
    }
    autogradeYesNoMessage += "Attempt auto-grading (Y/n)? ";
    
    int result = 0;
    bool autograde = autograderYesOrNo(autogradeYesNoMessage, "", "y");
    if (autograde) {
        if (FLAGS.callbackStart) {
            FLAGS.callbackStart();
        }
        
        result = mainRunAutograderTestCases(argc, argv);
        
        // a hook to allow per-assignment code to run at the start
        if (FLAGS.callbackEnd) {
            FLAGS.callbackEnd();
        }
    }
    
    // manual testing
    bool manualGrade = autograderYesOrNo("Run program for manual testing (y/N)? ", "", "n");
    if (manualGrade) {
        autograder::gwindowSetExitGraphicsEnabled(false);
        while (manualGrade) {
            studentMain();
            std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
            manualGrade = autograderYesOrNo("Run program again (y/N)? ", "", "n");
        }
        autograder::gwindowSetExitGraphicsEnabled(true);
    }
    std::cout << AUTOGRADER_OUTPUT_SEPARATOR << std::endl;
    
    // run any custom callbacks that the assignment's autograder added
    for (CallbackButtonInfo& buttonInfo : FLAGS.callbackButtons) {
        std::string buttonText = buttonInfo.text;
        buttonText = stringReplace(buttonText, "\n", " ");   // GUI often uses multi-line button text strings; strip
        if (autograderYesOrNo(buttonText + " (Y/n)? ", "", "y")) {
            buttonInfo.func();
        }
    }
    
    // run Style Checker to point out common possible style issues
    if (!FLAGS.styleCheckFiles.isEmpty()) {
        if (autograderYesOrNo("Run style checker (Y/n)? ", "", "y")) {
            mainRunStyleChecker();
        }
    }
    
    // display turnin time information to decide late days used
    if (FLAGS.showLateDays) {
        showLateDays();
    }
    
    std::cout << "COMPLETE: Autograder terminated successfully. Exiting." << std::endl;
    return result;
}

static std::string addAutograderButton(GWindow& gui, const std::string& text, const std::string& icon) {
    std::string html = "<html><center>" + stringReplace(text, "\n", "<br>") + "</center></html>";
    GButton* button = new GButton(html);
    if (!icon.empty()) {
        button->setIcon(icon);
        button->setTextPosition(SwingConstants::SWING_CENTER, SwingConstants::SWING_BOTTOM);
    }
    gui.addToRegion(button, "SOUTH");
    // memory leak
    return html;
}

int autograderGraphicalMain(int argc, char** argv) {
    GWindow gui;
    gui.setVisible(false);
    gui.setTitle(FLAGS.assignmentName + " Autograder");
    gui.setCanvasSize(0, 0);
    gui.setExitOnClose(true);
    
    GLabel startLabel("");
    if (!FLAGS.startMessage.empty()) {
        std::string startMessage = FLAGS.startMessage;
        if (!stringContains(startMessage, "<html>")) {
            startMessage = stringReplace(startMessage, "Note:", "<b>Note:</b>");
            startMessage = stringReplace(startMessage, "NOTE:", "<b>NOTE:</b>");
            startMessage = "<html><body style='width: 500px; max-width: 500px;'>"
                    + startMessage
                    + "</body></html>";
        }
        
        startLabel.setLabel(startMessage);
        gui.addToRegion(&startLabel, "NORTH");
    }
    
    std::string autogradeText = addAutograderButton(gui, "Automated\ntests", "check.gif");
    std::string manualText = addAutograderButton(gui, "Run\nmanually", "play.gif");
    std::string styleCheckText = addAutograderButton(gui, "Style\nchecker", "magnifier.gif");
    for (CallbackButtonInfo& buttonInfo : FLAGS.callbackButtons) {
        buttonInfo.text = addAutograderButton(gui, buttonInfo.text, buttonInfo.icon);
    }
    std::string lateDayText = addAutograderButton(gui, "Late days\ninfo", "calendar.gif");
    std::string aboutText = addAutograderButton(gui, "About\nGrader", "help.gif");
    std::string exitText = addAutograderButton(gui, "Exit\nGrader", "stop.gif");
    gui.pack();
    gui.setVisible(true);
    
    int result = 0;
    while (true) {
        GEvent event = waitForEvent(ACTION_EVENT | WINDOW_EVENT);
        if (event.getEventClass() == ACTION_EVENT) {
            GActionEvent actionEvent(event);
            std::string cmd = actionEvent.getActionCommand();
            if (cmd == autogradeText) {
                if (FLAGS.callbackStart) {
                    FLAGS.callbackStart();
                }
                
                pp->autograderunittest_clearTests();
                result = mainRunAutograderTestCases(argc, argv);
                
                if (FLAGS.callbackEnd) {
                    FLAGS.callbackEnd();
                }
            } else if (cmd == manualText) {
                // set up buttons to automatically enter user input
                if (FLAGS.showInputPanel) {
                    inputpanel::load(FLAGS.inputPanelFilename);
                }
                
                // actually run the student's program
                pp->jbeconsole_toFront();
                studentMain();
            } else if (cmd == styleCheckText) {
                mainRunStyleChecker();
            } else if (cmd == lateDayText) {
                showLateDays();
            } else if (cmd == aboutText) {
                GOptionPane::showMessageDialog(FLAGS.aboutText, "About Autograder",
                                               GOptionPane::MessageType::INFORMATION);
            } else if (cmd == exitText) {
                gui.close();
                break;
            } else {
                for (CallbackButtonInfo buttonInfo : FLAGS.callbackButtons) {
                    if (cmd == buttonInfo.text) {
                        buttonInfo.func();
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

int autograderMain(int argc, char** argv) {
    // initialize Stanford libraries and graphical console
    _mainFlags = GRAPHICS_FLAG + CONSOLE_FLAG;
    startupMainDontRunMain(argc, argv);
    setConsoleLocationSaved(true);
    if (FLAGS.graphicalUI) {
        return autograderGraphicalMain(argc, argv);
    } else {
        return autograderTextMain(argc, argv);
    }
}
#endif // SPL_AUTOGRADER_MODE
} // namespace autograder