#pragma once

#define GROWBED_FW_VERSION        "0.1.0"
#define GROWBED_DEVICE_ID_PREFIX  "GB-"

static constexpr uint32_t kWatchdogTimeoutMs = 5000U;

static constexpr uint32_t kSensorPollMs     = 2000U;
static constexpr uint32_t kControlTickMs    =  500U;
static constexpr uint32_t kSchedulerTickMs  = 10000U;
static constexpr uint32_t kUiTickMs         =  100U;
