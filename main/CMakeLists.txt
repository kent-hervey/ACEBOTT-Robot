idf_component_register(SRCS "acebott_main.c" "acebott_hw.c"
        INCLUDE_DIRS "."
        REQUIRES driver esp_adc esp_ringbuf esp_timer esp_hw_support)