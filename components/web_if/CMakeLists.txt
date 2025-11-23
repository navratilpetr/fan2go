idf_component_register(
    SRCS "web_if.c"
    INCLUDE_DIRS "." "../../main"
    REQUIRES esp_http_server fan
)

