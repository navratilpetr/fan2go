idf_component_register(
    SRCS "mqtt_if.c"
    INCLUDE_DIRS "." "../../main"
    REQUIRES mqtt fan freertos storage
)

