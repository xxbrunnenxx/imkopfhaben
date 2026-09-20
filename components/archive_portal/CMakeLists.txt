idf_component_register(
    SRCS "archive_portal.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server json recording_archive_service summary_service
    PRIV_REQUIRES joint_tracker_service
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
