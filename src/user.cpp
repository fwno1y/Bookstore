#include "user.h"
#include <algorithm>
#include "user_database.h"
#include <iostream>
#include <fstream>
#include <cstring>
const int USER_SIZE = 31 * 3 + sizeof(int);//结构体大小
int user_block_size = USER_SIZE * BLOCKSIZE + sizeof(int) * 2 + 31;//块状链表中块的大小

User::User() = default;

User::User(const std::string& UserID, const std::string& Password, int Privilege, const std::string& Username) {
    strcpy(this->UserID,UserID.c_str());
    strcpy(this->Password,Password.c_str());
    this->Privilege = Privilege;
    strcpy(this->Username,Username.c_str());
}

void User::read(std::fstream &file) {
    file.read(UserID, 31);
    UserID[30] = '\0';
    file.read(Password, 31);
    Password[30] = '\0';
    file.read(Username, 31);
    Username[30] = '\0';
    file.read(reinterpret_cast<char*>(&Privilege), sizeof(int));
}

void User::write(std::fstream &file) {
    UserID[30] = '\0';
    Password[30] = '\0';
    Username[30] = '\0';
    file.write(UserID, 31);
    file.write(Password,31);
    file.write(Username,31);
    file.write(reinterpret_cast<char*>(&Privilege),sizeof(int));
}

CurrentUser::CurrentUser(const User &User, const std::string &Book) {
    user = User;
    selected_book = Book;
}

UserDatabase::BlockIndex::BlockIndex() : block_pos(-1) {
    max_UserID[0] = '\0';
}

UserDatabase::BlockNode::BlockNode() : size(0),next_block(-1) {
    max_UserID[0] = '\0';
}

void UserDatabase::BlockNode::updateMaxUserID() {
    if (size > 0) {
        strncpy(max_UserID, users[size - 1].UserID, 30);
        max_UserID[30] = '\0';
    } else {
        max_UserID[0] = '\0';
    }
}

void UserDatabase::BlockNode::read(std::fstream& file) {
    file.read(reinterpret_cast<char *> (&size), sizeof(int));
    file.read(reinterpret_cast<char *> (&next_block),sizeof(int));
    file.read(max_UserID, 31);
    //读入所有用户，与write对应
    for (int i = 0; i < BLOCKSIZE; ++i) {
        users[i].read(file);
    }
}

void UserDatabase::BlockNode::write(std::fstream& file) {
    updateMaxUserID();
    file.write(reinterpret_cast<char *> (&size),sizeof(int));
    file.write(reinterpret_cast<char *> (&next_block),sizeof(int));
    file.write(max_UserID, 31);
    // 写入所有用户，包括未使用的，保证块的大小固定
    for (int i = 0; i < BLOCKSIZE; ++i) {
        users[i].write(file);
    }
}

void UserDatabase::read_block(std::fstream& file, BlockNode &node, int pos) {
    file.seekg(pos * user_block_size);
    node.read(file);
}

void UserDatabase::write_block(std::fstream& file, BlockNode &node, int pos) {
    file.seekp(pos * user_block_size);
    node.write(file);
}

// 重建内存中的索引，把每个块的最大user和块位置写入数组
void UserDatabase::rebuildBlockIndex() {
    block_index.clear();
    if (!user_file.is_open()) {
        return;
    }
    BlockNode block;
    int pos = 0;
    while (true) {
        read_block(user_file, block, pos);
        BlockIndex idx;
        idx.block_pos = pos;
        strncpy(idx.max_UserID, block.max_UserID, 30);
        idx.max_UserID[30] = '\0';
        block_index.push_back(idx);
        if (block.next_block == -1) {
            break;
        }
        pos = block.next_block;
    }
}

// 更新某个块的索引
void UserDatabase::updateBlockIndex(int block_pos, const char* max_UserID, int after_pos) {
    for (int i = 0; i < block_index.size(); ++i) {
        if (block_index[i].block_pos == block_pos) {
            strncpy(block_index[i].max_UserID, max_UserID, 30);
            block_index[i].max_UserID[30] = '\0';
            return;
        }
    }
    // 如果不存在，添加新索引
    BlockIndex new_idx;
    new_idx.block_pos = block_pos;
    strncpy(new_idx.max_UserID, max_UserID, 30);
    new_idx.max_UserID[30] = '\0';

    if (after_pos >= 0) {
        // 在指定块之后插入
        for (int i = 0; i < block_index.size(); ++i) {
            if (block_index[i].block_pos == after_pos) {
                block_index.insert(block_index.begin() + i + 1, new_idx);
                return;
            }
        }
    }
    // 否则添加到末尾
    block_index.push_back(new_idx);
}

// 根据UserID找到目标块位置,只比较内存中的max值
int UserDatabase::findTargetBlock(const std::string& UserID) {
    if (block_index.empty()) return 0;
    // 遍历索引，找到第一个max_UserID >= UserID的块
    for (size_t i = 0; i < block_index.size(); ++i) {
        if (block_index[i].max_UserID[0] == '\0') {
            continue;
        }
        //找到或最后一块
        if (strcmp(UserID.c_str(), block_index[i].max_UserID) <= 0 || i == block_index.size() - 1) {
            return block_index[i].block_pos;
        }
    }
    return block_index.back().block_pos;
}

bool UserDatabase::insert(const User& user) {
    User* exist = find(user.UserID);
    if (exist != nullptr) {
        delete exist;
        return false;
    }
    delete exist;
    if (!user_file.is_open()) {
        return false;
    }
    // 使用内存索引找到目标块位置
    int pos = findTargetBlock(user.UserID);
    BlockNode cur_block;
    read_block(user_file, cur_block, pos);
    // 二分查找插入
    int l = 0, r = cur_block.size - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        if (strcmp(cur_block.users[mid].UserID, user.UserID) < 0) {
            l = mid + 1;
        }
        else {
            r = mid - 1;
        }
    }
    int insert_pos = l;
    if (cur_block.size < BLOCKSIZE) {
        for (int i = cur_block.size; i > insert_pos; --i) {
            cur_block.users[i] = cur_block.users[i - 1];
        }
        cur_block.users[insert_pos] = user;
        cur_block.size++;
        write_block(user_file, cur_block, pos);
        updateBlockIndex(pos, cur_block.max_UserID);
    }
    else {
        // 当前块已满，需要分块
        BlockNode new_block = BlockNode();
        int mid = cur_block.size / 2;
        // 把后半部分移到新块
        for (int i = mid; i < cur_block.size; ++i) {
            new_block.users[new_block.size] = cur_block.users[i];
            new_block.size++;
        }
        cur_block.size = mid;
        // 新块位置最后
        user_file.seekp(0, std::ios::end);
        int new_pos = user_file.tellp() / user_block_size;
        new_block.next_block = cur_block.next_block;
        cur_block.next_block = new_pos;
        // 写回两个块
        write_block(user_file, cur_block, pos);
        write_block(user_file, new_block, new_pos);
        // 更新内存中的块索引
        updateBlockIndex(pos, cur_block.max_UserID);
        updateBlockIndex(new_pos, new_block.max_UserID, pos);
        // 确定在哪个块插入
        if (insert_pos < mid) {
            for (int i = cur_block.size; i > insert_pos; --i) {
                cur_block.users[i] = cur_block.users[i - 1];
            }
            cur_block.users[insert_pos] = user;
            cur_block.size++;
            write_block(user_file, cur_block, pos);
            updateBlockIndex(pos, cur_block.max_UserID);
        }
        else {
            int new_insert_pos = insert_pos - mid;
            for (int i = new_block.size; i > new_insert_pos; --i) {
                new_block.users[i] = new_block.users[i - 1];
            }
            new_block.users[new_insert_pos] = user;
            new_block.size++;
            write_block(user_file, new_block, new_pos);
            updateBlockIndex(new_pos, new_block.max_UserID);
        }
    }
    user_file.flush();
    return true;
}

bool UserDatabase::erase(const std::string &UserID) {
    if (!user_file.is_open()) {
        return false;
    }
    int pos = findTargetBlock(UserID);
    BlockNode cur_block, next_block;
    read_block(user_file, cur_block, pos);
    int l = 0, r = cur_block.size - 1;
    int erase_pos = -1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = strcmp(cur_block.users[mid].UserID, UserID.c_str());
        if (cmp > 0) {
            r = mid - 1;
        } else if (cmp < 0) {
            l = mid + 1;
        } else {
            erase_pos = mid;
            break;
        }
    }
    if (erase_pos == -1) {
        return false;
    }
    for (int i = erase_pos; i < cur_block.size - 1; ++i) {
        cur_block.users[i] = cur_block.users[i + 1];
    }
    cur_block.size--;
    //尝试合并
    if (cur_block.size < BLOCKSIZE / 4 && cur_block.next_block != -1) {
        read_block(user_file, next_block, cur_block.next_block);
        if (cur_block.size + next_block.size < BLOCKSIZE) {
            int merged_pos = cur_block.next_block;
            for (int i = 0; i < next_block.size; ++i) {
                cur_block.users[cur_block.size] = next_block.users[i];
                cur_block.size++;
            }
            cur_block.next_block = next_block.next_block;
            write_block(user_file, cur_block, pos);
            updateBlockIndex(pos, cur_block.max_UserID);
            // 移除被合并块的索引
            for (auto it = block_index.begin(); it != block_index.end(); ++it) {
                if (it->block_pos == merged_pos) {
                    block_index.erase(it);
                    break;
                }
            }
            user_file.flush();
            return true;
        }
    }
    write_block(user_file, cur_block, pos);
    updateBlockIndex(pos, cur_block.max_UserID);
    user_file.flush();
    return true;
}

User *UserDatabase::find(const std::string &UserID) {
    if (!user_file.is_open()) {
        return nullptr;
    }
    int pos = findTargetBlock(UserID);
    BlockNode cur_block;
    read_block(user_file, cur_block, pos);
    int l = 0, r = cur_block.size - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = std::strcmp(cur_block.users[mid].UserID, UserID.c_str());
        if (cmp > 0) {
            r = mid - 1;
        }
        else if (cmp < 0) {
            l = mid + 1;
        }
        else {
            User* found = new User();
            *found = cur_block.users[mid];
            return found;
        }
    }
    return nullptr;
}

bool UserDatabase::update(const User &user) {
    if (!user_file.is_open()) {
        return false;
    }
    int pos = findTargetBlock(user.UserID);
    BlockNode cur_block;
    read_block(user_file, cur_block, pos);
    int l = 0, r = cur_block.size - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = strcmp(cur_block.users[mid].UserID, user.UserID);
        if (cmp > 0) {
            r = mid - 1;
        } else if (cmp < 0) {
            l = mid + 1;
        } else {
            cur_block.users[mid] = user;
            write_block(user_file, cur_block, pos);
            user_file.flush();
            return true;
        }
    }
    return false;
}

UserDatabase::UserDatabase(const std::string &user_file) {
    if (!user_file.empty()) {
        file_name = user_file;
    }
    initialize();
}

UserDatabase::~UserDatabase() {
    flush();
    Login_stack.clear();
    if (user_file.is_open()) {
        user_file.close();
    }
}

void UserDatabase::initialize() {
    user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    if (!user_file) {
        user_file.open(file_name, std::ios::out | std::ios::binary);
        user_file.close();
        user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    }
    user_file.seekg(0,std::ios::end);
    if (user_file.tellg() == 0) {
        //写入第一个块
        BlockNode headblock;
        headblock.size = 1;
        headblock.next_block = -1;
        //创建店主root账户
        User root = User("root","sjtu",7,"shopkeeper");
        headblock.users[0] = root;
        write_block(user_file,headblock,0);
    }
    // 重建内存中的块索引
    rebuildBlockIndex();
}

bool UserDatabase::Login(const std::string &UserID, const std::string &Password) {
    User* user = find(UserID);
    if (user == nullptr) {
        return false;
    }
    CurrentUser cur_user = CurrentUser(*user,"");
    int cur_Privilege = getCurrentPrivilege();
    bool flag = false;
    if (Password.empty() && cur_Privilege > user->Privilege) {
        flag = true;
    }
    // 提供了密码，必须正确
    else if (!Password.empty() && strcmp(Password.c_str(), user->Password) == 0) {
        flag = true;
    }
    if (flag) {
        Login_stack.push_back(cur_user);
        delete user;
        return true;
    }
    delete user;
    return false;
}

bool UserDatabase::Logout() {
    //无登录用户
    if (Login_stack.empty()) {
        return false;
    }
    //弹出登录栈最后一个
    Login_stack.pop_back();
    return true;
}

bool UserDatabase::Register(const std::string &UserID, const std::string &Password, const std::string &Username) {
    if (UserID.length() > 30 || Password.length() > 30 || Username.length() > 30) {
        return false;
    }
    for (char c : UserID) {
        if (!(isdigit(c) || isalpha(c) || c == '_')) {
            return false;
        }
    }
    for (char c : Password) {
        if (!(isdigit(c) || isalpha(c) || c == '_')) {
            return false;
        }
    }
    for (char c : Username) {
        if (c <= 32 || c > 126) {
            return false;
        }
    }
    //看UserID是否已经被注册
    User* exist = find(UserID);
    if (exist != nullptr) {
        delete exist;
        return false;
    }
    User new_user = User(UserID,Password,1,Username);
    return insert(new_user);
}

bool UserDatabase::ChangePassword(const std::string &UserID, const std::string &CurrentPassword, const std::string &NewPassword) {
    //判断账户是否存在
    User* user = find(UserID);
    if (user == nullptr) {
        return false;
    }
    // 验证新密码格式
    if (NewPassword.length() > 30) {
        delete user;
        return false;
    }
    for (char c : NewPassword) {
        if (!(isdigit(c) || isalpha(c) || c == '_')) {
            delete user;
            return false;
        }
    }
    //权限够或者密码正确
    int cur_Privilege = getCurrentPrivilege();
    if (cur_Privilege == 7 || strcmp(user->Password,CurrentPassword.c_str()) == 0) {
        strcpy(user->Password,NewPassword.c_str());
        bool flag = update(*user);
        delete user;
        return flag;
    }
    delete user;
    return false;
}

bool UserDatabase::UserAdd(const std::string &UserID, const std::string &Password, int Privilege, const std::string &Username) {
    //权限是否足够
    int cur_Privilege = getCurrentPrivilege();
    if (cur_Privilege <= Privilege) {
        return false;
    }
    if (UserID.length() > 30 || Password.length() > 30 || Username.length() > 30) {
        return false;
    }
    for (char c : UserID) {
        if (!(isdigit(c) || isalpha(c) || c == '_')) {
            return false;
        }
    }
    for (char c : Password) {
        if (!(isdigit(c) || isalpha(c) || c == '_')) {
            return false;
        }
    }
    for (char c : Username) {
        if (c <= 32 || c > 126) {
            return false;
        }
    }
    //判断UserID是否已被注册
    User* exist = find(UserID);
    if (exist != nullptr) {
        delete exist;
        return false;
    }
    User new_user = User(UserID,Password,Privilege,Username);
    return insert(new_user);
}

bool UserDatabase::Delete(const std::string &UserID) {
    //权限要7
    int cur_Privilege = getCurrentPrivilege();
    if (cur_Privilege != 7) {
        return false;
    }
    //判断账户是否存在
    User* exist = find(UserID);
    if (exist == nullptr) {
        delete exist;
        return false;
    }
    bool flag = false;
    //判断是否在登录栈中
    for (int i = 0; i < Login_stack.size(); ++i) {
        if (strcmp(Login_stack[i].user.UserID, UserID.c_str()) == 0) {
            flag = true;
            break;
        }
    }
    if (flag) {
        return false;
    }
    return erase(UserID);
}
//获取当前用户
User * UserDatabase::getCurrentUser() {
    if (Login_stack.empty()) {
        return nullptr;
    }
    User* user = new User();
    *user = Login_stack.back().user;
    return user;
}
//获取当前权限
int UserDatabase::getCurrentPrivilege() {
    if (Login_stack.empty()) {
        return 0;
    }
    return Login_stack.back().user.Privilege;
}
//设置选中图书
void UserDatabase::set_selected_book(const std::string& ISBN) {
    if (Login_stack.empty()) {
        return;
    }
    Login_stack.back().selected_book = ISBN;
}
//获取选中图书
std::string UserDatabase::get_selected_book() {
    if (Login_stack.empty()) {
        return "";
    }
    return Login_stack.back().selected_book;
}
//将缓冲区内的数据写入文件
void UserDatabase::flush() {
    if (user_file.is_open()) {
        user_file.flush();
    }
}
//更新登录栈中其他用户的选中图书ISBN
void UserDatabase::update_selected_book(const std::string &oldISBN, const std::string &newISBN) {
    for (auto& userState : Login_stack) {
        if (userState.selected_book == oldISBN) {
            userState.selected_book = newISBN;
        }
    }
}
