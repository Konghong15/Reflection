# Demo/CMakeLists.txt

set(DEMO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Demo.h"
)

foreach(HEADER ${DEMO_HEADERS})
    get_filename_component(FILENAME ${HEADER} NAME_WE)
    get_filename_component(DIR ${HEADER} DIRECTORY)
    set(GENERATED "${DIR}/${FILENAME}.generate.h")

    add_custom_command(
        OUTPUT ${GENERATED}
        COMMAND ReflectionCodeGenerator ${HEADER} ${GENERATED}
        DEPENDS ${HEADER}
        COMMENT "Generating reflection header for ${HEADER}"
    )

    list(APPEND GENERATED_FILES ${GENERATED})
endforeach()

add_executable(Demo
    Demo.cpp
    ${DEMO_HEADERS}
    ${GENERATED_FILES}
 "DemoClass.cpp")

set_property(TARGET Demo PROPERTY CXX_STANDARD 20)
target_include_directories(Demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
