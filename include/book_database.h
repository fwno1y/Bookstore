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

    // 内存索引：存储每个块的最大ISBN和块位置，用于快速定位
    struct BlockIndex {
        char max_ISBN[21];
        int block_pos;
        BlockIndex() : block_pos(-1) { max_ISBN[0] = '\0'; }
    };
    std::vector<BlockIndex> block_index;  // 按顺序存储所有块的索引

    struct BookBlock {
        Book books[BLOCKSIZE];
        int size;
        int next_block;
        char max_ISBN[21];  // 记录块内最大ISBN，用于快速跳过

        BookBlock();

        void read(std::fstream& file);

        void write(std::fstream& file);

        void updateMaxISBN();  // 更新最大ISBN
    };

    static void read_block(std::fstream& file, BookBlock& block, int pos);

    static void write_block(std::fstream& file, BookBlock& block, int pos);

    void rebuildBlockIndex();  // 重建内存中的块索引

    void updateBlockIndex(int block_pos, const char* max_ISBN, int after_pos = -1);  // 更新某个块的索引

    int findTargetBlock(const std::string& ISBN);  // 根据ISBN找到目标块位置

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

    void rebuildIndex();

    void rebuildIndexInternal();

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
    bool Select(const std::string& ISBN);
    //修改图书信息
    bool Modify(int type, const std::string& info);
    //图书进货
    bool Import(int Quantity, double TotalCost);
    //判断是否合法
    bool isValidISBN(const std::string& ISBN);

    bool isValidBookName(const std::string& name);

    bool isValidAuthor(const std::string& author);

    bool isValidKeyword(const std::string& keyword);

    bool isValidQuantity(int quantity);

    bool isValidPrice(double price);

    // 获取选中图书
    std::string getSelectedISBN();

    // 设置选中图书
    void setSelectedISBN(const std::string& ISBN);

    // 检查图书是否存在
    bool bookExists(const std::string& ISBN);

};


#endif //BOOKSTORE_2025_DATABASE_H