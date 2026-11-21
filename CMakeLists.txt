cmake_minimum_required(VERSION 3.21)
project(Sudoku VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(sudoku
    src/main.cpp
    src/sudokuboard.h
    src/sudokuboard.cpp
    src/sudokucell.h
    src/sudokucell.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    resources.qrc
)

target_include_directories(sudoku PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(sudoku PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(sudoku PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ---------- 单元测试 ----------
enable_testing()
find_package(Qt6 QUIET COMPONENTS Test)
if(Qt6Test_FOUND)
    qt_add_executable(sudoku_tests
        tests/test_sudokuboard.cpp
        src/sudokuboard.h
        src/sudokuboard.cpp
    )
    target_include_directories(sudoku_tests PRIVATE src/)
    target_link_libraries(sudoku_tests PRIVATE Qt6::Test)
    add_test(NAME sudoku_tests COMMAND sudoku_tests)
else()
    message(WARNING "Qt6Test not found, tests disabled")
endif()
