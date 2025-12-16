#ifndef BOOKSTORE_2025_DATABASE_H
#define BOOKSTORE_2025_DATABASE_H
#include <iostream>
#include "book.h"
#include <fstream>
class BookDatabase {
private:
    std::fstream file;
    std::string file_name;
    long head_block = -1;
    long tail_block = -1;
    long free_head_block = -1;

    void read_block(long pos, Book& Book);

    void write_block(long pos,Book& Book);

    long allocate_block();

    void free_block(long pos);

    void split_block(long pos, Book& Book);

    void try_merge_blocks(long pos1, long pos2);

    long find_block(std::string ISBN);


public:
    BookDatabase();

    ~BookDatabase();

    void initialize() {

    }

    //检索图书
    void Show(int type, const std::string& info);
    //购买图书
    double Buy(const std::string& ISBN, int Quantity);
    //选择图书
    void Select(const std::string& ISBN);
    //修改图书信息
    void Modify(int type, const std::string& info);
    //图书进货
    bool Import(int Quantity, long long TotalCost);
};


#endif //BOOKSTORE_2025_DATABASE_H