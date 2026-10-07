package dev.deviceperformance.device_performance_monitor

import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import org.mockito.Mockito
import kotlin.test.Test

internal class DevicePerformanceMonitorPluginTest {
    @Test
    fun onMethodCall_getPerformanceMetrics_returnsMapShape() {
        val plugin = DevicePerformanceMonitorPlugin()
        val call = MethodCall("getPerformanceMetrics", null)
        val result = Mockito.mock(MethodChannel.Result::class.java)

        // Without an attached engine, collection still succeeds with N/A values.
        // The handler posts to a background executor; wait briefly.
        plugin.onMethodCall(call, result)
        Thread.sleep(500)

        Mockito.verify(result, Mockito.timeout(2000)).success(
            Mockito.argThat { value ->
                value is Map<*, *> &&
                    value.containsKey("cpu") &&
                    value.containsKey("ram") &&
                    value.containsKey("temperature")
            },
        )
    }
}
