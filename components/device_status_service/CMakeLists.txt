idf_component_register(SRCS "device_status_service.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_tsens esp_http_client json local_ai_service wifi_service)
