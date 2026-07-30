idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES port_bsp app_bsp esp_wifi_bsp esp_wifi esp_netif mbedtls esp_driver_gpio)
