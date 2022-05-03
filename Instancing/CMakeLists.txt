cmake_minimum_required(VERSION 3.21) # C++ 20

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Boost 1.78 REQUIRED COMPONENTS log)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(THIS_EXEC "_instancing_")
file(GLOB HEADER_COLLECTION "src/**.h" "src/**.hpp")

add_executable(${THIS_EXEC}
    # Headers
        ${HEADER_COLLECTION}
    # Source
        #"src/"
        "src/main.cppm"

        "src/Core/app_default.ixx"
        "src/Core/app.ixx"

        "src/core/helpers/fontatlas.ixx"
        "src/core/helpers/files.ixx"
        "src/core/helpers/glfw.ixx"
        "src/core/helpers/vk.ixx"

        "src/my_app.ixx"
        "src/process.cppm"
        "src/vulkan.cppm"
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
        fmt::fmt-header-only
        glfw
        imgui::imgui
        glm::glm
        ${Boost_LIBRARIES}
        spdlog::spdlog spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
    )