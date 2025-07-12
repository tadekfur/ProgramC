cmake_minimum_required(VERSION 3.16)

project(EtykietyManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_compile_options(/permissive-)
    add_compile_options(/FS)  # Fix for PDB access issues
endif()

# Włącz obsługę kopiowania plików
include(CMakeParseArguments)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Zmień poniższą ścieżkę na swoją lokalizację Qt6, np. C:/Qt/6.5.0/mingw_64/lib/cmake
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64/lib/cmake")

find_package(Qt6 COMPONENTS Core Gui Widgets Sql Network PrintSupport Pdf PdfWidgets REQUIRED)

# Opcjonalnie znajdź WebEngineWidgets jeśli dostępne
find_package(Qt6 COMPONENTS WebEngineWidgets QUIET)
if(Qt6WebEngineWidgets_FOUND)
    message(STATUS "Qt6WebEngineWidgets znalezione - włączam obsługę podglądu PDF")
    add_compile_definitions(QT_WEBENGINEWIDGETS_LIB)
    set(WEBENGINE_LIBS Qt6::WebEngineWidgets)
else()
    message(STATUS "Qt6WebEngineWidgets nie znalezione - podgląd PDF będzie ograniczony")
    set(WEBENGINE_LIBS "")
endif()

# Na Windows dodaj obsługę ActiveX dla Adobe Reader
if(WIN32)
    find_package(Qt6 COMPONENTS AxContainer QUIET)
    if(Qt6AxContainer_FOUND)
        message(STATUS "Qt6AxContainer znalezione - włączam obsługę Adobe Reader")
        add_compile_definitions(QT_AXCONTAINER_LIB)
        set(AXCONTAINER_LIBS Qt6::AxContainer)
    else()
        message(STATUS "Qt6AxContainer nie znalezione - Adobe Reader nie będzie dostępny")
        set(AXCONTAINER_LIBS "")
    endif()
else()
    set(AXCONTAINER_LIBS "")
endif()

file(GLOB_RECURSE SOURCES
    main.cpp
    mainwindow.cpp
    models/*.cpp
    views/*.cpp
    views/materials_order_dialog.cpp
    db/*.cpp
    network/*.cpp
    utils/*.cpp
)

file(GLOB_RECURSE HEADERS
    mainwindow.h
    models/*.h
    views/*.h
    db/*.h
    network/*.h
    utils/*.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Network Qt6::PrintSupport Qt6::Pdf Qt6::PdfWidgets ${WEBENGINE_LIBS} ${AXCONTAINER_LIBS})

# Dodaj zasoby, jeśli masz plik .qrc  
qt_add_resources(${PROJECT_NAME} resources/app.qrc)

# Kopiuj skrypty Python do katalogu wyjściowego
file(GLOB PYTHON_SCRIPTS
    utils/*.py
)

# Utwórz katalog utils w katalogu wyjściowym, jeśli nie istnieje
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/utils
    COMMENT "Tworzenie katalogu utils w katalogu wyjściowym"
)

foreach(SCRIPT ${PYTHON_SCRIPTS})
    get_filename_component(SCRIPT_FILENAME ${SCRIPT} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
                ${CMAKE_CURRENT_SOURCE_DIR}/utils/${SCRIPT_FILENAME}
                ${CMAKE_CURRENT_BINARY_DIR}/utils/${SCRIPT_FILENAME}
        COMMENT "Kopiowanie skryptu ${SCRIPT_FILENAME} do katalogu wyjściowego"
    )
endforeach()

# Kopiuj czcionki DejaVu do katalogu wyjściowego
file(GLOB DEJAVU_FONTS
    resources/DejaVu*.ttf
)
foreach(FONT ${DEJAVU_FONTS})
    get_filename_component(FONT_FILENAME ${FONT} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
                ${CMAKE_CURRENT_SOURCE_DIR}/resources/${FONT_FILENAME}
                ${CMAKE_CURRENT_BINARY_DIR}/utils/${FONT_FILENAME}
        COMMENT "Kopiowanie czcionki ${FONT_FILENAME} do katalogu utils"
    )
endforeach()

# Kopiuj skrypty Python i czcionki do katalogu wynikowego
set(PYTHON_SCRIPTS
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/order_confirmation.py
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/production_ticket.py
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_data.json
)

set(FONT_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/DejaVuSans.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/DejaVuSans-Bold.ttf
)

# Utwórz katalog utils w katalogu wynikowym
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/utils)

# Kopiuj pliki przy każdym budowaniu
foreach(PYTHON_SCRIPT ${PYTHON_SCRIPTS})
    get_filename_component(SCRIPT_NAME ${PYTHON_SCRIPT} NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PYTHON_SCRIPT}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/utils/${SCRIPT_NAME}
        COMMENT "Kopiowanie ${SCRIPT_NAME} do katalogu wynikowego"
    )
endforeach()

# Kopiuj czcionki
foreach(FONT_FILE ${FONT_FILES})
    get_filename_component(FONT_NAME ${FONT_FILE} NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FONT_FILE}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${FONT_NAME}
        COMMENT "Kopiowanie ${FONT_NAME} do katalogu wynikowego"
    )
endforeach()
