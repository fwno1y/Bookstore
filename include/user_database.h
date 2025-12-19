#ifndef BOOKSTORE_2025_USER_DATABASE_H
#define BOOKSTORE_2025_USER_DATABASE_H
#include <iostream>
#include "user.h"
#include <fstream>
#include <vector>
const int BLOCKSIZE = 100;
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

    struct BlockNode {
        User users[BLOCKSIZE];
        int size{};
        int next_block{};

        BlockNode();

        void read(std::fstream& file);

        void write(std::fstream& file);
    };

    void read_block(std::fstream& file, BlockNode& node, int pos);

    void write_block(std::fstream& file, BlockNode& node, int pos);

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

    User* gerCurrentUser();

    int getCurrentPrivilege();

    void set_selected_book(std::string& book);

    std::string get_selected_book();
};

#endif //BOOKSTORE_2025_USER_DATABASE_H