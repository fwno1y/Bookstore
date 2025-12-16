cmake_minimum_required(VERSION 3.28)
project(Bookstore_2025)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(Bookstore_2025
        include/book.h
        include/book_database.h
        include/log_database.h
        include/user.h
        include/user_database.h
        src/book.cpp
        src/log.cpp
        src/main.cpp
        src/user.cpp)
