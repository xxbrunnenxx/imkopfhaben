idf_component_register(
    SRCS "local_ai_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client esp_http_server esp_timer json nvs_flash recording_service task_config
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
