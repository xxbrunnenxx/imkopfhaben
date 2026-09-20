idf_component_register(
    SRCS "waveshare_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES audio_hal audio_settings_service axp2101 esp_driver_gpio esp_driver_i2c esp_driver_spi
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
