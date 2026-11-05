#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"

/* Binary-safe test endpoint: every received byte is returned unchanged. */
void app_main(void)
{
    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = 4096,
        .tx_buffer_size = 4096,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));

    uint8_t data[256];
    while (1) {
        const int count = usb_serial_jtag_read_bytes(data, sizeof(data),
                                                     pdMS_TO_TICKS(50));
        if (count <= 0) {
            continue;
        }
        int sent = 0;
        while (sent < count) {
            const int n = usb_serial_jtag_write_bytes(data + sent,
                                                       (size_t)(count - sent),
                                                       pdMS_TO_TICKS(1000));
            if (n > 0) {
                sent += n;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
    }
}