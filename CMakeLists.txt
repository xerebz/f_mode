project(f_mode LANGUAGES CXX)
add_executable(f_mode f_mode.cpp)
target_link_libraries(f_mode rte_eal rte_ethdev rte_mbuf)
