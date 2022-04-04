cmake_minimum_required(VERSION 3.21) # C++ 20

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Boost 1.78 REQUIRED COMPONENTS log)
set(SPDLOG_WCHAR_TO_UTF8_SUPPORT ON)
find_package(spdlog CONFIG REQUIRED)

set(THIS_EXEC "_imterm_exmpl_")
file(GLOB HEADER_COLLECTION "src/**.h" "src/**.hpp")

add_executable(${THIS_EXEC}
    # Headers
        ${HEADER_COLLECTION}
    # Source
        #"src/"
        "src/main.cpp"
        "src/terminal_commands.cpp"
     )
target_include_directories(${THIS_EXEC}
    PUBLIC
        ${TARGET_SHARED}
        "${CMAKE_SOURCE_DIR}/include"
        ${Boost_INCLUDE_DIR}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/src"
    )
target_link_libraries(${THIS_EXEC}
    PUBLIC
        ${TARGET_SHARED}
        glfw
        imgui::imgui
        glm::glm
        ${Boost_LIBRARIES}
        spdlog::spdlog spdlog::spdlog_header_only
    )