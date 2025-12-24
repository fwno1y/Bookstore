#ifndef BOOKSTORE_2025_USER_H
#define BOOKSTORE_2025_USER_H
#include <iostream>
struct User {
    char UserID[31]{};
    char Password[31]{};
    int Privilege{};
    char Username[31]{};

    User();

    User(const std::string& UserID, const std::string& Password, int Privilege, const std::string& Username);

    void read(std::fstream &file);

    void write(std::fstream &file);

};


#endif //BOOKSTORE_2025_USER_H