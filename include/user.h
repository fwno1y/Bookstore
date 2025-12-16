#ifndef BOOKSTORE_2025_USER_H
#define BOOKSTORE_2025_USER_H
#include <iostream>
const int BLOCK_MAX_USER = 100;
struct User {
    int size{};
    char UserID[BLOCK_MAX_USER][31]{};
    char Password[BLOCK_MAX_USER][31]{};
    int Privilege[BLOCK_MAX_USER]{};
    char Username[BLOCK_MAX_USER][31]{};
    long pre_block = -1;
    long next_block = -1;

    User();

    bool too_big();

    bool too_small();

    int find_index(const std::string& UserID);

    void insert(int index, const std::string& UserID, const std::string& Password, int Privilege, const std::string& Username);

    void erase(int index);

};


#endif //BOOKSTORE_2025_USER_H