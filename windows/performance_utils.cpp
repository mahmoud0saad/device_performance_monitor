#include "performance_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <comdef.h>
#include <Wbemidl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

namespace device_performance_monitor {
namespace {

struct CpuSample {
  ULONGLONG idle = 0;
  ULONGLONG kernel = 0;
  ULONGLONG user = 0;
  bool valid = false;
};

std::mutex g_cpu_mutex;
CpuSample g_prev_cpu;

ULONGLONG FileTimeToUll(const FILETIME& ft) {
  ULARGE_INTEGER value;
  value.LowPart = ft.dwLowDateTime;
  value.HighPart = ft.dwHighDateTime;
  return value.QuadPart;
}

CpuSample ReadCpuSample() {
  CpuSample sample;
  try {
    FILETIME idle{};
    FILETIME kernel{};
    FILETIME user{};
    if (!GetSystemTimes(&idle, &kernel, &user)) {
      return sample;
    }
    sample.idle = FileTimeToUll(idle);
    sample.kernel = FileTimeToUll(kernel);
    sample.user = FileTimeToUll(user);
    sample.valid = true;
  } catch (...) {
    sample.valid = false;
  }
  return sample;
}

std::string FormatCpuPercent(double percent) {
  try {
    const double clamped = (std::min)(100.0, (std::max)(0.0, percent));
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f%%", clamped);
    return std::string(buffer);
  } catch (...) {
    return "N/A";
  }
}

std::string ReadCpuUsage() {
  try {
    std::lock_guard<std::mutex> lock(g_cpu_mutex);
    CpuSample current = ReadCpuSample();
    if (!current.valid) {
      return "N/A";
    }

    if (!g_prev_cpu.valid) {
      Sleep(200);
      CpuSample second = ReadCpuSample();
      if (!second.valid) {
        g_prev_cpu = current;
        return "N/A";
      }
      const ULONGLONG idle_delta = second.idle - current.idle;
      // Kernel includes idle time on Windows.
      const ULONGLONG kernel_delta = second.kernel - current.kernel;
      const ULONGLONG user_delta = second.user - current.user;
      const ULONGLONG total_delta = kernel_delta + user_delta;
      g_prev_cpu = second;
      if (total_delta == 0) {
        return "0%";
      }
      const ULONGLONG busy = total_delta - idle_delta;
      const double percent =
          (static_cast<double>(busy) * 100.0) / static_cast<double>(total_delta);
      return FormatCpuPercent(percent);
    }

    const ULONGLONG idle_delta = current.idle - g_prev_cpu.idle;
    const ULONGLONG kernel_delta = current.kernel - g_prev_cpu.kernel;
    const ULONGLONG user_delta = current.user - g_prev_cpu.user;
    const ULONGLONG total_delta = kernel_delta + user_delta;
    g_prev_cpu = current;
    if (total_delta == 0) {
      return "0%";
    }
    const ULONGLONG busy = total_delta - idle_delta;
    const double percent =
        (static_cast<double>(busy) * 100.0) / static_cast<double>(total_delta);
    return FormatCpuPercent(percent);
  } catch (...) {
    return "N/A";
  }
}

std::string ReadRamUsage() {
  try {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
      return "N/A";
    }
    if (status.ullTotalPhys == 0) {
      return "N/A";
    }
    const ULONGLONG used = status.ullTotalPhys - status.ullAvailPhys;
    const double percent =
        (static_cast<double>(used) * 100.0) /
        static_cast<double>(status.ullTotalPhys);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f%%", percent);
    return std::string(buffer);
  } catch (...) {
    return "N/A";
  }
}

std::string ReadTemperature() {
  HRESULT hr = E_FAIL;
  bool com_initialized = false;
  IWbemLocator* locator = nullptr;
  IWbemServices* services = nullptr;
  IEnumWbemClassObject* enumerator = nullptr;

  try {
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
      com_initialized = true;
    } else if (hr == RPC_E_CHANGED_MODE) {
      // COM already initialized with a different model; continue.
    } else {
      return "N/A";
    }

    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, reinterpret_cast<LPVOID*>(&locator));
    if (FAILED(hr) || locator == nullptr) {
      if (com_initialized) {
        CoUninitialize();
      }
      return "N/A";
    }

    hr = locator->ConnectServer(_bstr_t(L"ROOT\\WMI"), nullptr, nullptr, nullptr,
                                0, nullptr, nullptr, &services);
    if (FAILED(hr) || services == nullptr) {
      locator->Release();
      if (com_initialized) {
        CoUninitialize();
      }
      return "N/A";
    }

    hr = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           nullptr, EOAC_NONE);
    if (FAILED(hr)) {
      services->Release();
      locator->Release();
      if (com_initialized) {
        CoUninitialize();
      }
      return "N/A";
    }

    hr = services->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
        &enumerator);
    if (FAILED(hr) || enumerator == nullptr) {
      services->Release();
      locator->Release();
      if (com_initialized) {
        CoUninitialize();
      }
      return "N/A";
    }

    IWbemClassObject* object = nullptr;
    ULONG returned = 0;
    std::string formatted = "N/A";

    if (enumerator->Next(WBEM_INFINITE, 1, &object, &returned) == S_OK &&
        returned > 0 && object != nullptr) {
      VARIANT value;
      VariantInit(&value);
      if (SUCCEEDED(object->Get(L"CurrentTemperature", 0, &value, nullptr,
                                nullptr))) {
        double tenths_kelvin = -1.0;
        switch (value.vt) {
          case VT_I4:
            tenths_kelvin = static_cast<double>(value.lVal);
            break;
          case VT_UI4:
            tenths_kelvin = static_cast<double>(value.ulVal);
            break;
          case VT_INT:
            tenths_kelvin = static_cast<double>(value.intVal);
            break;
          case VT_UINT:
            tenths_kelvin = static_cast<double>(value.uintVal);
            break;
          default:
            break;
        }
        if (tenths_kelvin >= 0.0) {
          // Tenths of Kelvin -> Celsius.
          const double celsius = (tenths_kelvin / 10.0) - 273.15;
          if (celsius > -50.0 && celsius < 150.0) {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.1f°C", celsius);
            formatted = buffer;
          }
        }
      }
      VariantClear(&value);
      object->Release();
    }

    enumerator->Release();
    services->Release();
    locator->Release();
    if (com_initialized) {
      CoUninitialize();
    }
    return formatted;
  } catch (...) {
    try {
      if (enumerator) {
        enumerator->Release();
      }
      if (services) {
        services->Release();
      }
      if (locator) {
        locator->Release();
      }
      if (com_initialized) {
        CoUninitialize();
      }
    } catch (...) {
      // Ignore cleanup failures.
    }
    return "N/A";
  }
}

}  // namespace

PerformanceMetrics CollectPerformanceMetrics() {
  PerformanceMetrics metrics;
  try {
    metrics.cpu = ReadCpuUsage();
  } catch (...) {
    metrics.cpu = "N/A";
  }
  try {
    metrics.ram = ReadRamUsage();
  } catch (...) {
    metrics.ram = "N/A";
  }
  try {
    metrics.temperature = ReadTemperature();
  } catch (...) {
    metrics.temperature = "N/A";
  }
  if (metrics.cpu.empty()) {
    metrics.cpu = "N/A";
  }
  if (metrics.ram.empty()) {
    metrics.ram = "N/A";
  }
  if (metrics.temperature.empty()) {
    metrics.temperature = "N/A";
  }
  return metrics;
}

}  // namespace device_performance_monitor
