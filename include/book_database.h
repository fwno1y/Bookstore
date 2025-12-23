#ifndef BOOKSTORE_2025_DATABASE_H
#define BOOKSTORE_2025_DATABASE_H
#include <iostream>
#include "book.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
const int BLOCKSIZE = 100;
class BookDatabase {
private:
    std::fstream book_file;
    std::string file_name;
    std::string selected_ISBN;
    std::unordered_map<std::string, int> ISBN_map{};
    std::unordered_map<std::string, std::vector<int>> name_map{};
    std::unordered_map<std::string, std::vector<int>> author_map{};
    std::unordered_map<std::string, std::vector<int>> keyword_map{};

    struct BookBlock {
        Book books[BLOCKSIZE];
        int size;
        int next_block;

        BookBlock();

        void read(std::fstream& file);

        void write(std::fstream& file);
    };

    static void read_block(std::fstream& file, BookBlock& block, int pos);

    static void write_block(std::fstream& file, BookBlock& block, int pos);

    bool insertBook(const Book& book);

    bool eraseBook(const std::string& ISBN);

    Book* findBookByISBN(const std::string& ISBN);

    bool updateBook(const Book& book);

    std::vector<Book> findAllBooks();

    std::vector<Book> findBooksByBookname(const std::string& bookname);

    std::vector<Book> findBooksByAuthor(const std::string& author);

    std::vector<Book> findBooksByKeyword(const std::string& keyword);

    void addIndex(const Book& book, int block_pos);

    void eraseIndex(const Book& book, int block_pos);

    void updateIndex(const Book& book, int block_pos, bool type );

    void rebuildIndex();

public:
    explicit BookDatabase(const std::string& book_file);

    ~BookDatabase();

    void initialize();

    //加入图书
    bool Add(const Book& book);
    //删除图书
    bool Delete(const std::string& ISBN);
    //检索图书
    std::vector<Book> showAllBooks();

    Book *showBooksByISBN(const std::string &ISBN);
    std::vector<Book> showBooksByName(const std::string& name);
    std::vector<Book> showBooksByAuthor(const std::string& author);
    std::vector<Book> showBooksByKeyword(const std::string& keyword);
    //购买图书
    double Buy(const std::string& ISBN, int Quantity);
    //选择图书
    void Select(const std::string& ISBN);
    //修改图书信息
    void Modify(int type, const std::string& info);
    //图书进货
    bool Import(int Quantity, long long TotalCost);
    //判断是否合法
    bool isValidISBN(const std::string& ISBN);

    bool isValidBookName(const std::string& name);

    bool isValidAuthor(const std::string& author);

    bool isValidKeyword(const std::string& keyword);

    bool isValidQuantity(int quantity);

    bool isValidPrice(double price);

    // 获取选中图书
    std::string getSelectedISBN();

    // 检查图书是否存在
    bool bookExists(const std::string& ISBN);

};


#endif //BOOKSTORE_2025_DATABASE_H