cmake_minimum_required(VERSION 3.16)
project(LotusNotebook VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Disable AUTOUIC - UI is built programmatically in C++ code
set(CMAKE_AUTOUIC OFF)

# Find Qt components
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Include directories
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/pythonexecutor.cpp
    src/cellwidget.cpp
    src/notebookmanager.cpp
    src/backupmanager.cpp
    src/syntaxhighlighter.cpp
    src/commandpalette.cpp
    src/thememanager.cpp
    src/settingsdialog.cpp
    src/variableinspector.cpp
    src/codecompleter.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/pythonexecutor.h
    src/cellwidget.h
    src/notebookmanager.h
    src/backupmanager.h
    src/syntaxhighlighter.h
    src/commandpalette.h
    src/thememanager.h
    src/settingsdialog.h
    src/variableinspector.h
    src/codecompleter.h
)

# Create executable
add_executable(lotus-notebook ${SOURCES} ${HEADERS})

# Link Qt and Python libraries
target_link_libraries(lotus-notebook PRIVATE
    Qt5::Widgets
    ${Python3_LIBRARIES}
)

# Compiler definitions
target_compile_definitions(lotus-notebook PRIVATE
    QT_DEPRECATED_WARNINGS
)

# Installation directories
install(TARGETS lotus-notebook RUNTIME DESTINATION bin)
install(FILES assets/lotus.desktop DESTINATION share/applications)
install(FILES assets/icon.png DESTINATION share/lotus-notebook)

# Create backup directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/backups)
