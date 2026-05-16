idf_component_register(
    SRC_DIRS "src" "src/threads" "src/external"
    INCLUDE_DIRS "src"
    REQUIRES arduino
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format -Wno-error=char-subscripts)
