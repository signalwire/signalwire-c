cmake_minimum_required(VERSION 3.7.2)

# Right now we can setup the example
add_executable(test main.c)
target_link_libraries(test signalwire_client2)
