set(src_dirs
        LED
        MYIIC
        MYWIFI
        MYSPI
        CHINESE16_3K
        SPILCD
        XL9555
)
set(include_dirs
        LED
        MYIIC
        MYWIFI
        MYSPI
        SPILCD
        CHINESE16_3K
        XL9555
)

set(requires
        driver
        esp_lcd
        esp_wifi
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
