#ifndef BOOKSTORE_2025_USER_DATABASE_H
#define BOOKSTORE_2025_USER_DATABASE_H
#include <iostream>
#include "user.h"
#include <fstream>
class UserDatabase {
private:
    std::fstream file;
    std::string file_name;
    long head_block = -1;
    long tail_block = -1;
    long free_head_block = -1;

    void read_block(long pos, User& User);

    void write_block(long pos, User& User);

    long allocate_block();

    void free_block(long pos);

    void split_block(long pos, User& User);

    void try_merge_blocks(long pos1, long pos2);

    long find_block(std::string& UserID);
public:
    UserDatabase();

    ~UserDatabase();

    void initialize() {

    }
    //登录账户和注销账户由main.cpp的登录栈完成
    //注册账户
    void Register(const std::string & UserID, const std::string& Password, const std::string& Username);
    //修改密码
    void ChangePassword(const std::string& UserID, const std::string& CurrentPassword, const std::string& NewPassword);
    //创建账户
    void UserAdd(const std::string & UserID, const std::string& Password, int Privilege, const std::string& Username);
    //删除账户
    void Delete(const std::string& UserID);


};



#endif //BOOKSTORE_2025_USER_DATABASE_H