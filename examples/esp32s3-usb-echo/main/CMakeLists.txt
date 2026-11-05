idf_component_register(SRCS "usb_echo_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_usb_serial_jtag)