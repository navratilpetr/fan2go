idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES fan proto wifi_if mqtt_if web_if storage
)

