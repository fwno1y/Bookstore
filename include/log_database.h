#ifndef BOOKSTORE_2025_LOG_DATABASE_H
#define BOOKSTORE_2025_LOG_DATABASE_H
#include <iostream>
#include <fstream>
#include <utility>
class LogDatabase {
private:
    std::fstream log_data;
    std::string file_name;
public:
    LogDatabase();
    ~LogDatabase();
};

class DealDatabase {
private:
    std::fstream deal_data;
    std::string file_name;
public:
    DealDatabase() = default;

    explicit DealDatabase(std::string  file_name) : file_name(std::move(file_name)) {}

    ~DealDatabase();

    void initialize();


};

#endif //BOOKSTORE_2025_LOG_DATABASE_H