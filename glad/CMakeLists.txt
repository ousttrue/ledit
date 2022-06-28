set(TARGET_NAME glad)
add_library(${TARGET_NAME} glad.c)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
