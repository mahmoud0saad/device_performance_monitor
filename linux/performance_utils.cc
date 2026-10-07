#include "performance_utils.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace device_performance_monitor {
namespace {

struct CpuSample {
  unsigned long long idle = 0;
  unsigned long long total = 0;
  bool valid = false;
};

std::mutex g_cpu_mutex;
CpuSample g_prev_cpu;

CpuSample ReadCpuSample() {
  CpuSample sample;
  try {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
      return sample;
    }
    std::string label;
    unsigned long long user = 0, nice = 0, system = 0, idle = 0;
    unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
    file >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >>
        steal;
    if (label != "cpu") {
      return sample;
    }
    sample.idle = idle + iowait;
    sample.total = user + nice + system + idle + iowait + irq + softirq + steal;
    sample.valid = sample.total > 0;
  } catch (...) {
    sample.valid = false;
  }
  return sample;
}

std::string FormatCpuPercent(double percent) {
  try {
    const double clamped = std::min(100.0, std::max(0.0, percent));
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
      usleep(200 * 1000);
      CpuSample second = ReadCpuSample();
      if (!second.valid) {
        g_prev_cpu = current;
        return "N/A";
      }
      const unsigned long long total_delta = second.total - current.total;
      const unsigned long long idle_delta = second.idle - current.idle;
      g_prev_cpu = second;
      if (total_delta == 0) {
        return "0%";
      }
      const double percent =
          ((static_cast<double>(total_delta - idle_delta) * 100.0) /
           static_cast<double>(total_delta));
      return FormatCpuPercent(percent);
    }

    const unsigned long long total_delta = current.total - g_prev_cpu.total;
    const unsigned long long idle_delta = current.idle - g_prev_cpu.idle;
    g_prev_cpu = current;
    if (total_delta == 0) {
      return "0%";
    }
    const double percent =
        ((static_cast<double>(total_delta - idle_delta) * 100.0) /
         static_cast<double>(total_delta));
    return FormatCpuPercent(percent);
  } catch (...) {
    return "N/A";
  }
}

unsigned long long ParseMemKb(const std::string& line) {
  try {
    std::istringstream stream(line);
    std::string key;
    unsigned long long value = 0;
    std::string unit;
    stream >> key >> value >> unit;
    return value;
  } catch (...) {
    return 0;
  }
}

std::string ReadRamUsage() {
  try {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
      return "N/A";
    }
    unsigned long long mem_total = 0;
    unsigned long long mem_available = 0;
    std::string line;
    while (std::getline(file, line)) {
      if (line.rfind("MemTotal:", 0) == 0) {
        mem_total = ParseMemKb(line);
      } else if (line.rfind("MemAvailable:", 0) == 0) {
        mem_available = ParseMemKb(line);
      }
      if (mem_total > 0 && mem_available > 0) {
        break;
      }
    }
    if (mem_total == 0) {
      return "N/A";
    }
    if (mem_available > mem_total) {
      mem_available = mem_total;
    }
    const unsigned long long used = mem_total - mem_available;
    const double percent =
        (static_cast<double>(used) * 100.0) / static_cast<double>(mem_total);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f%%", percent);
    return std::string(buffer);
  } catch (...) {
    return "N/A";
  }
}

double ReadSysfsTempCelsius(const std::string& path) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      return -1000.0;
    }
    double value = 0;
    file >> value;
    if (!file) {
      return -1000.0;
    }
    const double celsius = value > 1000.0 ? value / 1000.0 : value;
    if (celsius < -50.0 || celsius > 150.0) {
      return -1000.0;
    }
    return celsius;
  } catch (...) {
    return -1000.0;
  }
}

std::string ReadTemperature() {
  try {
    const std::vector<std::string> preferred = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/devices/virtual/thermal/thermal_zone0/temp",
    };
    for (const auto& path : preferred) {
      const double celsius = ReadSysfsTempCelsius(path);
      if (celsius > -999.0) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.1f°C", celsius);
        return std::string(buffer);
      }
    }

    DIR* dir = opendir("/sys/class/thermal");
    if (dir != nullptr) {
      while (dirent* entry = readdir(dir)) {
        if (entry->d_name[0] == '.') {
          continue;
        }
        std::string name(entry->d_name);
        if (name.rfind("thermal_zone", 0) != 0) {
          continue;
        }
        const std::string path =
            std::string("/sys/class/thermal/") + name + "/temp";
        const double celsius = ReadSysfsTempCelsius(path);
        if (celsius > -999.0) {
          closedir(dir);
          char buffer[32];
          std::snprintf(buffer, sizeof(buffer), "%.1f°C", celsius);
          return std::string(buffer);
        }
      }
      closedir(dir);
    }
    return "N/A";
  } catch (...) {
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
