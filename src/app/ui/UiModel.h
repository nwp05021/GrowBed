#pragma once
#include <Arduino.h>
#include "../controllers/MotionController.h"

enum class UiScreen : uint8_t {
    Main = 0,
    MenuRoot,
    MenuMotion,
    MenuParams,
    MenuDiag,
    MenuSystem,
    MenuLed,
    MenuTest,
    TestRunning,
    TestResult,
    Toast,
    AlertPopup,
    EditValue,
    Engineering,
};

struct UiViewModel {
    bool envValid = false;
    float tempC = 0;
    float humPct = 0;

    MotionStatus st;
    MotionConfig cfg;

    UiScreen screen = UiScreen::Main;
    uint8_t cursor = 0;
    uint8_t page = 0;
    bool invert = false;
    bool blink = false;

    uint32_t uptimeMs = 0;

    // edit
    const char* editLabel = nullptr;
    int32_t editValue = 0;
    int32_t editMin = 0;
    int32_t editMax = 0;
    const char* editUnit = nullptr;
    bool editAsTime = false;

    // fault
    bool isFault = false;
    uint8_t faultCode = 0;
    uint8_t retryCount = 0;
    const char* faultTitle = nullptr;
    const char* faultDetail = nullptr;

    // diagnostic
    uint32_t faultTotal = 0;
    uint32_t lastFaultUptimeMs = 0;
    uint8_t  recoverAttempts = 0;
    uint8_t  lastFaultCode = 0;
    bool     permanentFault = false;

    uint32_t resetCount = 0;

    // alert popup
    bool showAlertPopup = false;
    uint8_t popupFaultCode = 0;
    uint32_t popupAlertSeq = 0;

    // toast
    bool showToast = false;
    const char* toastTitle = nullptr;
    const char* toastLine1 = nullptr;
    const char* toastLine2 = nullptr;

    // factory / test
    bool factoryRunning = false;
    bool factoryDone = false;
    bool factoryPass = false;
    uint8_t factoryStep = 0;
    const char* factoryStepName = nullptr;
    uint8_t factoryFailCode = 0;
    uint8_t factoryFailStep = 0;
    uint32_t factoryStepElapsedMs = 0;

    bool testRunning = false;
    uint8_t testStep = 0;
    uint32_t testElapsedMs = 0;
    uint32_t testNextInMs = 0;

    bool autoHallEnabled = false;

    bool factoryAutoRunning = false;
    uint16_t factoryAutoProgress = 0;
    uint16_t factoryAutoTarget = 0;
};