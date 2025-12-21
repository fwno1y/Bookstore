#ifndef BOOKSTORE_2025_BOOK_H
#define BOOKSTORE_2025_BOOK_H
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
struct Book {
    char ISBN[21]{};
    char BookName[61]{};
    char Author[61]{};
    char Keyword[61]{};
    int Quantity{};
    double Price{};

    Book();

    explicit Book(const std::string& ISBN);

    ~Book();

    Book(const std::string& ISBN, const std::string& Bookname, const std::string& Author, const std::string& Keyword, int Quantity, double price);

    void read(std::fstream& file);

    void write(std::fstream& file);

    std::vector<std::string> getKeywords() const;

    bool includeKeyword(const std::string& keyword);

    std::string print();

    bool operator < (const Book& other) const {
        return strcmp(ISBN,other.ISBN) < 0;
    }

    bool operator == (const Book& other) const {
        return strcmp(ISBN,other.ISBN) == 0;
    }


};


#endif //BOOKSTORE_2025_BOOK_H