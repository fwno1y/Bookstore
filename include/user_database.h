#ifndef BOOKSTORE_2025_USER_DATABASE_H
#define BOOKSTORE_2025_USER_DATABASE_H
#include <iostream>
#include "user.h"
#include <fstream>
#include <vector>
#include "book_database.h"

//当前用户状态
struct CurrentUser {
    User user;
    std::string selected_book;//当前选择图书的ISBN

    CurrentUser(const User& User, const std::string& Book);
};

class UserDatabase {
private:
    std::fstream user_file;
    std::string file_name;
    std::vector<CurrentUser> Login_stack;

    // 内存中的索引：存储每个块的最大UserID和块位置，用于快速定位
    struct BlockIndex {
        char max_UserID[31]{};
        int block_pos;

        BlockIndex();
    };
    std::vector<BlockIndex> block_index;  // 按顺序存储所有块的索引

    //块状链表存储user结构体数组、大小和下一块位置、块内最大UserID
    struct BlockNode {
        User users[BLOCKSIZE];
        int size{};
        int next_block{};
        char max_UserID[31]{};

        BlockNode();

        void read(std::fstream& file);

        void write(std::fstream& file);

        void updateMaxUserID();
    };

    static void read_block(std::fstream& file, BlockNode& node, int pos);

    static void write_block(std::fstream& file, BlockNode& node, int pos);

    void rebuildBlockIndex();  // 重建内存中的块索引

    void updateBlockIndex(int block_pos, const char* max_UserID, int after_pos = -1);  // 更新某个块的索引

    int findTargetBlock(const std::string& UserID);  // 根据UserID找到目标块位置

    bool insert(const User& user);

    bool erase(const std::string& UserID);

    User* find(const std::string& UserID);

    bool update(const User& user);
public:
    explicit UserDatabase(const std::string& user_file);

    ~UserDatabase();

    void initialize();
    //登录账户
    bool Login(const std::string& UserID, const std::string& Password);
    //注销账户
    bool Logout();
    //注册账户
    bool Register(const std::string& UserID, const std::string& Password, const std::string& Username);
    //修改密码
    bool ChangePassword(const std::string& UserID, const std::string& CurrentPassword, const std::string& NewPassword);
    //创建账户
    bool UserAdd(const std::string& UserID, const std::string& Password, int Privilege, const std::string& Username);
    //删除账户
    bool Delete(const std::string& UserID);

    User* getCurrentUser();

    int getCurrentPrivilege();

    void set_selected_book(const std::string& book);

    std::string get_selected_book();

    void flush();

    void update_selected_book(const std::string& oldISBN, const std::string& newISBN);//要修改登录栈中其他用户选中
};

#endif //BOOKSTORE_2025_USER_DATABASE_H