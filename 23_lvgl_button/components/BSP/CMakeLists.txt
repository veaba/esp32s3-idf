set(src_dirs
        IIC
        XL9555
)
set(include_dirs
        IIC
        XL9555
)
set(requires
        driver
)
idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})