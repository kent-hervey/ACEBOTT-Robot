
# This tells ESP-IDF to link the ADC driver so driver/adc.h can be found
idf_component_register(SRCS "acebott_main.c" "acebott_hw.c"
        INCLUDE_DIRS "."
        REQUIRES esp_adc driver)