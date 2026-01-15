cmake_minimum_required(VERSION 3.10)
project(calculator_rpc C)

set(CMAKE_C_STANDARD 11)

# Добавляем флаги для macOS
if(APPLE)
    add_compile_options(-D_DARWIN_C_SOURCE)
endif()

# Добавляем исполняемые файлы
add_executable(calculator_server
    calculator_server.c
    calculator_svc.c
    calculator_xdr.c
)

add_executable(calculator_client
    calculator_client.c
    calculator_clnt.c
    calculator_xdr.c
)

# Базовые библиотеки (системные на macOS)
target_link_libraries(calculator_server pthread)

# На macOS не нужно явно линковать librpcsvc, 
# так как она входит в системную библиотеку libc
# Для Linux может потребоваться -ltirpc

configure_file(calculator.h ${CMAKE_CURRENT_BINARY_DIR}/calculator.h COPYONLY)