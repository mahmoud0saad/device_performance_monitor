package dev.deviceperformance.device_performance_monitor

import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import android.os.Handler
import android.os.Looper
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result
import java.io.BufferedReader
import java.io.File
import java.io.FileReader
import java.util.Locale
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicLong
import kotlin.math.max
import kotlin.math.min

/** Fail-safe CPU / RAM / temperature collector for Android. Never errors the channel. */
class DevicePerformanceMonitorPlugin :
    FlutterPlugin,
    MethodCallHandler {
    private lateinit var channel: MethodChannel
    private var appContext: Context? = null
    private val executor = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())

    private val lastAppCpuTime = AtomicLong(-1)
    private val lastAppWallTimeNs = AtomicLong(-1)

    override fun onAttachedToEngine(flutterPluginBinding: FlutterPlugin.FlutterPluginBinding) {
        try {
            appContext = flutterPluginBinding.applicationContext
            channel = MethodChannel(
                flutterPluginBinding.binaryMessenger,
                "device_performance_monitor",
            )
            channel.setMethodCallHandler(this)
        } catch (_: Exception) {
            // Attachment failure must not crash the host process.
        }
    }

    override fun onMethodCall(call: MethodCall, result: Result) {
        try {
            if (call.method == "getPerformanceMetrics") {
                executor.execute {
                    val map = collectMetricsSafe()
                    mainHandler.post {
                        try {
                            result.success(map)
                        } catch (_: Exception) {
                            // Result already completed or engine torn down.
                        }
                    }
                }
            } else {
                // Unknown methods still return the stable map so Dart never sees errors.
                result.success(naMap())
            }
        } catch (_: Exception) {
            try {
                result.success(naMap())
            } catch (_: Exception) {
                // Ignore.
            }
        }
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        try {
            channel.setMethodCallHandler(null)
        } catch (_: Exception) {
            // Ignore.
        }
        appContext = null
    }

    private fun collectMetricsSafe(): Map<String, String> {
        return try {
            mapOf(
                "cpu" to readCpuUsage(),
                "ram" to readRamUsage(),
                "temperature" to readTemperature(),
            )
        } catch (_: Exception) {
            naMap()
        }
    }

    private fun naMap(): Map<String, String> =
        mapOf(
            "cpu" to "N/A",
            "ram" to "N/A",
            "temperature" to "N/A",
        )

    private fun readRamUsage(): String {
        return try {
            val context = appContext ?: return "N/A"
            val am = context.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
                ?: return "N/A"
            val info = ActivityManager.MemoryInfo()
            am.getMemoryInfo(info)
            val total = info.totalMem
            if (total <= 0L) {
                return "N/A"
            }
            val used = total - info.availMem
            val percent = (used.toDouble() / total.toDouble()) * 100.0
            String.format(Locale.US, "%.2f%%", percent)
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun readTemperature(): String {
        return try {
            val paths =
                listOf(
                    "/sys/class/thermal/thermal_zone0/temp",
                    "/sys/class/thermal/thermal_zone1/temp",
                    "/sys/devices/virtual/thermal/thermal_zone0/temp",
                )
            for (path in paths) {
                val celsius = readSysfsTempCelsius(path)
                if (celsius != null) {
                    return String.format(Locale.US, "%.1f°C", celsius)
                }
            }
            readBatteryTemperature()
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun readSysfsTempCelsius(path: String): Double? {
        return try {
            val file = File(path)
            if (!file.exists() || !file.canRead()) {
                return null
            }
            val raw = file.readText().trim()
            val value = raw.toDoubleOrNull() ?: return null
            val celsius = if (value > 1000.0) value / 1000.0 else value
            if (celsius < -50.0 || celsius > 150.0) {
                return null
            }
            celsius
        } catch (_: Exception) {
            null
        }
    }

    private fun readBatteryTemperature(): String {
        return try {
            val context = appContext ?: return "N/A"
            val intent =
                context.registerReceiver(
                    null,
                    IntentFilter(Intent.ACTION_BATTERY_CHANGED),
                ) ?: return "N/A"
            val tenths = intent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Int.MIN_VALUE)
            if (tenths == Int.MIN_VALUE) {
                return "N/A"
            }
            val celsius = tenths / 10.0
            String.format(Locale.US, "%.1f°C", celsius)
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun readCpuUsage(): String {
        return try {
            val fromTop = readCpuFromTop()
            if (fromTop != null) {
                return fromTop
            }
            readAppCpuUsage()
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun readCpuFromTop(): String? {
        return try {
            val process =
                ProcessBuilder("top", "-n", "1", "-b")
                    .redirectErrorStream(true)
                    .start()
            val output =
                process.inputStream.bufferedReader().use { it.readText() }
            process.waitFor()
            if (output.isBlank()) {
                return null
            }

            // toybox: "400%cpu  12%user   3%nice  45%sys ..."
            // procps: "Cpu(s): 12.5%us,  3.2%sy, ..."
            val toybox =
                Regex(
                    """(\d+(?:\.\d+)?)%user.*?(\d+(?:\.\d+)?)%sys""",
                    RegexOption.IGNORE_CASE,
                ).find(output)
            val procps =
                Regex(
                    """(\d+(?:\.\d+)?)%?\s*us[, ].*?(\d+(?:\.\d+)?)%?\s*sy""",
                    RegexOption.IGNORE_CASE,
                ).find(output)

            val match = toybox ?: procps ?: return null
            val user = match.groupValues[1].toDoubleOrNull() ?: return null
            val sys = match.groupValues[2].toDoubleOrNull() ?: return null
            val total = user + sys
            if (total <= 0.0) {
                return null
            }
            val cores = max(1, Runtime.getRuntime().availableProcessors())
            val normalized = min(100.0, max(0.0, total / cores))
            String.format(Locale.US, "%.0f%%", normalized)
        } catch (_: Exception) {
            null
        }
    }

    private fun readAppCpuUsage(): String {
        return try {
            val cpuTime = readProcSelfCpuTime() ?: return "N/A"
            val wallNs = System.nanoTime()
            val prevCpu = lastAppCpuTime.get()
            val prevWall = lastAppWallTimeNs.get()

            lastAppCpuTime.set(cpuTime)
            lastAppWallTimeNs.set(wallNs)

            if (prevCpu < 0L || prevWall < 0L) {
                // Establish baseline with a short second sample.
                try {
                    Thread.sleep(200L)
                } catch (_: InterruptedException) {
                    // Ignore.
                }
                val cpu2 = readProcSelfCpuTime() ?: return "0.1% (App)"
                val wall2 = System.nanoTime()
                lastAppCpuTime.set(cpu2)
                lastAppWallTimeNs.set(wall2)
                return formatAppCpu(prevCpu = cpuTime, prevWallNs = wallNs, cpu = cpu2, wallNs = wall2)
            }

            formatAppCpu(prevCpu = prevCpu, prevWallNs = prevWall, cpu = cpuTime, wallNs = wallNs)
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun formatAppCpu(
        prevCpu: Long,
        prevWallNs: Long,
        cpu: Long,
        wallNs: Long,
    ): String {
        return try {
            val deltaCpu = cpu - prevCpu
            val deltaWallNs = wallNs - prevWallNs
            if (deltaCpu < 0L || deltaWallNs <= 0L) {
                return "0.1% (App)"
            }
            // /proc ticks are typically 100 Hz (jiffies).
            val clockTicks = 100.0
            val deltaCpuNs = (deltaCpu / clockTicks) * 1_000_000_000.0
            val cores = max(1, Runtime.getRuntime().availableProcessors())
            val percent = (deltaCpuNs / deltaWallNs) * 100.0 / cores
            val clamped = min(100.0, max(0.0, percent))
            if (clamped < 0.1) {
                "0.1% (App)"
            } else {
                String.format(Locale.US, "%.1f%% (App)", clamped)
            }
        } catch (_: Exception) {
            "N/A"
        }
    }

    private fun readProcSelfCpuTime(): Long? {
        return try {
            BufferedReader(FileReader("/proc/self/stat")).use { reader ->
                val line = reader.readLine() ?: return null
                val closeParen = line.lastIndexOf(')')
                if (closeParen < 0 || closeParen + 2 >= line.length) {
                    return null
                }
                val parts = line.substring(closeParen + 2).trim().split(Regex("\\s+"))
                // After ")": state(0), ppid(1), ... utime(11), stime(12)
                if (parts.size < 13) {
                    return null
                }
                val utime = parts[11].toLongOrNull() ?: return null
                val stime = parts[12].toLongOrNull() ?: return null
                utime + stime
            }
        } catch (_: Exception) {
            null
        }
    }
}
