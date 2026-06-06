set(src_dirs
        LED
        BLUETOOTH
        MYWIFI
)
set(include_dirs
        LED
        BLUETOOTH
        MYWIFI
)

set(requires
        nvs_flash
        driver
        esp_lcd
        esp_wifi
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
