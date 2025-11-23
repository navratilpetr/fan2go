idf_component_register(
    SRCS "wifi_if.c"
    INCLUDE_DIRS "." "../../main"
    REQUIRES esp_wifi esp_netif esp_event storage
)

