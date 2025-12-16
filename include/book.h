#ifndef BOOKSTORE_2025_BOOK_H
#define BOOKSTORE_2025_BOOK_H
#include <iostream>
const int BLOCK_MAX_BOOK = 100;

struct Book {
    int size;
    char ISBN[BLOCK_MAX_BOOK][21]{};
    char BookName[BLOCK_MAX_BOOK][61]{};
    char Author[BLOCK_MAX_BOOK][61]{};
    char Keyword[BLOCK_MAX_BOOK][61]{};
    int Quantity[BLOCK_MAX_BOOK]{};
    double Price[BLOCK_MAX_BOOK]{};
    long pre_block = -1;
    long next_block = -1;

    Book();

    bool too_big();

    bool too_small();

    int find_index(const std::string& ISBN);

    void insert(int index, const std::string& ISBN, const std::string& BookName, const std::string& Author, const std::string& Keyword, int Quantity, int price);

    void erase(int index);
};


#endif //BOOKSTORE_2025_BOOK_H