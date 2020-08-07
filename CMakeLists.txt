cmake_minimum_required(VERSION 3.18)
project(socket_example VERSION 1.0.0)

if(MSVC)
    add_compile_options(/W3 /WX)
else()
    add_compile_options(-W -Wall -Werror -pedantic)
endif()

set(CMAKE_CXX_STANDARD 20)

add_executable(client
    src/client.cpp
    src/utility.cpp)

add_executable(server
    src/server.cpp
    src/utility.cpp)

find_package(Threads REQUIRED)
target_link_libraries(server  ${CMAKE_THREAD_LIBS_INIT})

# target_link_libraries(hello_world PRIVATE say_hello)

# install(TARGETS "${PROJECT_NAME}" DESTINATION bin)    # ${CMAKE_INSTALL_PREFIX}/bin
# install(FILES "main.cpp" DESTINATION src)             # ${CMAKE_INSTALL_PREFIX}/src
