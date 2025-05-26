cmake_minimum_required(VERSION 3.30)
project(MyCompressionProject)

set(CMAKE_CXX_STANDARD 20)

add_executable(MyCompressionProject main.cpp)

# Линкуем с comdlg32 (нужно для GetOpenFileNameA и диалогов выбора файла)
target_link_libraries(MyCompressionProject comdlg32)
