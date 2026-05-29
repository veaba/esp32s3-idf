set(src_dirs
        LED
        MYIIC
        MYWIFI
        MYSPI
        CHINESE16_3K
        SPILCD
        XL9555
        KEY
)
set(include_dirs
        LED
        MYIIC
        MYWIFI
        MYSPI
        CHINESE16_3K
        SPILCD
        XL9555
        KEY
)

set(requires
        driver
        esp_lcd
        esp_wifi
        lwip
        esp_netif
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-O2 -Wno-error=format=-Wno-format)
