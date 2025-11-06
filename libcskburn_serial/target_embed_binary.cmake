find_package(Python3)

function(target_embed_binary TARGET VAR_NAME FILE)
    set(gen_c ${CMAKE_CURRENT_BINARY_DIR}/${VAR_NAME}_img.c)

    target_sources(${TARGET} PRIVATE ${gen_c})

    add_custom_command(
        OUTPUT  ${gen_c}
        COMMAND Python3::Interpreter
           ARGS ${CMAKE_SOURCE_DIR}/bin2c.py
                ${FILE}
                ${gen_c}
                ${VAR_NAME}
        DEPENDS ${FILE}
        COMMENT "Embedding binary file ${FILE} as ${VAR_NAME}"
    )
endfunction()
