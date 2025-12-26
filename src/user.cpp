#include "user.h"
#include <algorithm>
#include "user_database.h"
#include <iostream>
#include <fstream>
#include <cstring>
// User::write 实际写入 31+31+31+4=97 字节，而不是 sizeof(User)=100 字节
const int USER_WRITE_SIZE = 31 + 31 + 31 + sizeof(int);  // 97 bytes
int user_block_size = USER_WRITE_SIZE * BLOCKSIZE + sizeof(int) * 2 + 31;  // +31 for max_UserID

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
    // file.read(UserID, 30);
    // file.read(Password,30);
    // file.read(Username,30);
    // file.read(reinterpret_cast<char*>(&Privilege),sizeof(int));
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
    // 读取所有用户槽位（包括未使用的），保证与write对应
    for (int i = 0; i < BLOCKSIZE; ++i) {
        users[i].read(file);
    }
}

void UserDatabase::BlockNode::write(std::fstream& file) {
    updateMaxUserID();
    file.write(reinterpret_cast<char *> (&size),sizeof(int));
    file.write(reinterpret_cast<char *> (&next_block),sizeof(int));
    file.write(max_UserID, 31);
    // 写入所有用户槽位（包括未使用的），保证块大小固定
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

// 重建内存中的块索引
void UserDatabase::rebuildBlockIndex() {
    block_index.clear();
    if (!user_file.is_open()) return;

    BlockNode block;
    int pos = 0;
    while (true) {
        read_block(user_file, block, pos);
        BlockIndex idx;
        idx.block_pos = pos;
        strncpy(idx.max_UserID, block.max_UserID, 30);
        idx.max_UserID[30] = '\0';
        block_index.push_back(idx);

        if (block.next_block == -1) break;
        pos = block.next_block;
    }
}

// 更新某个块的索引，如果after_pos >= 0，表示新块应该插入到指定块之后
void UserDatabase::updateBlockIndex(int block_pos, const char* max_UserID, int after_pos) {
    for (size_t i = 0; i < block_index.size(); ++i) {
        if (block_index[i].block_pos == block_pos) {
            strncpy(block_index[i].max_UserID, max_UserID, 30);
            block_index[i].max_UserID[30] = '\0';
            return;
        }
    }
    // 如果不存在，需要添加新索引
    BlockIndex new_idx;
    new_idx.block_pos = block_pos;
    strncpy(new_idx.max_UserID, max_UserID, 30);
    new_idx.max_UserID[30] = '\0';

    if (after_pos >= 0) {
        // 在指定块之后插入
        for (size_t i = 0; i < block_index.size(); ++i) {
            if (block_index[i].block_pos == after_pos) {
                block_index.insert(block_index.begin() + i + 1, new_idx);
                return;
            }
        }
    }
    // 否则添加到末尾
    block_index.push_back(new_idx);
}

// 根据UserID找到目标块位置（只比较内存中的max值，实现根号n复杂度）
int UserDatabase::findTargetBlock(const std::string& UserID) {
    if (block_index.empty()) return 0;

    // block_index 按照链表顺序存储
    // 遍历索引，找到第一个max_UserID >= UserID的块
    for (size_t i = 0; i < block_index.size(); ++i) {
        // 如果当前块的max为空，跳过
        if (block_index[i].max_UserID[0] == '\0') continue;

        // 如果UserID <= 当前块的max，或者是最后一个块，就返回这个块
        if (strcmp(UserID.c_str(), block_index[i].max_UserID) <= 0 ||
            i == block_index.size() - 1) {
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

    // 使用内存索引找到目标块位置（只比较内存，不读磁盘）
    int pos = findTargetBlock(user.UserID);

    BlockNode cur_block;
    read_block(user_file, cur_block, pos);

    // 二分查找插入位置
    int l = 0, r = cur_block.size;
    while (l < r) {
        int mid = l + (r - l) / 2;
        if (strcmp(cur_block.users[mid].UserID, user.UserID) < 0) {
            l = mid + 1;
        } else {
            r = mid;
        }
    }
    int insert_pos = l;

    if (cur_block.size < BLOCKSIZE) {
        // 当前块有空间，直接插入
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
        int old_next_block = cur_block.next_block;

        // 将后半部分移到新块
        for (int i = mid; i < cur_block.size; ++i) {
            new_block.users[new_block.size] = cur_block.users[i];
            new_block.size++;
        }
        cur_block.size = mid;

        // 计算新块位置
        user_file.seekp(0, std::ios::end);
        int new_pos = user_file.tellp() / user_block_size;

        // 设置链接
        new_block.next_block = old_next_block;
        cur_block.next_block = new_pos;

        // 写回两个块
        write_block(user_file, cur_block, pos);
        write_block(user_file, new_block, new_pos);

        // 更新内存中的块索引：先更新当前块，再添加新块
        updateBlockIndex(pos, cur_block.max_UserID);
        updateBlockIndex(new_pos, new_block.max_UserID, pos);

        // 确定在哪个块插入
        if (insert_pos < mid) {
            // 插入到当前块（cur_block）
            for (int i = cur_block.size; i > insert_pos; --i) {
                cur_block.users[i] = cur_block.users[i - 1];
            }
            cur_block.users[insert_pos] = user;
            cur_block.size++;
            write_block(user_file, cur_block, pos);
            updateBlockIndex(pos, cur_block.max_UserID);
        }
        else {
            // 插入到新块（new_block）
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

    // 使用内存索引找到目标块位置
    int pos = findTargetBlock(UserID);

    BlockNode cur_block, next_block;
    read_block(user_file, cur_block, pos);

    // 在该块中使用二分查找
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

    // 尝试合并
    if (cur_block.size < BLOCKSIZE / 4 && cur_block.next_block != -1) {
        read_block(user_file, next_block, cur_block.next_block);
        if (cur_block.size + next_block.size < BLOCKSIZE) {
            // 需要从block_index中移除被合并的块
            int merged_pos = cur_block.next_block;
            for (int i = 0; i < next_block.size; ++i) {
                cur_block.users[cur_block.size++] = next_block.users[i];
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

    // 使用内存索引找到目标块位置（只比较内存，不读磁盘）
    int pos = findTargetBlock(UserID);

    BlockNode cur_block;
    read_block(user_file, cur_block, pos);

    // 在该块中使用二分查找
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

    // 使用内存索引找到目标块位置
    int pos = findTargetBlock(user.UserID);

    BlockNode cur_block;
    read_block(user_file, cur_block, pos);

    // 在该块中使用二分查找
    int l = 0, r = cur_block.size - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = strcmp(cur_block.users[mid].UserID, user.UserID);
        if (cmp > 0) {
            r = mid - 1;
        } else if (cmp < 0) {
            l = mid + 1;
        } else {
            // 找到用户，更新信息
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

// void UserDatabase::initialize() {
//     user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
//     if (!user_file) {
//         // 文件不存在，创建并初始化
//         user_file.open(file_name, std::ios::out | std::ios::binary);
//         user_file.close();
//         user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
//
//         //写入第一个块
//         BlockNode headblock;
//         headblock.size = 1;
//         headblock.next_block = -1;
//         //创建店主root账户
//         User root = User("root","sjtu",7,"shopkeeper");
//         headblock.users[0] = root;
//         write_block(user_file,headblock,0);
//     } else {
//         // 文件已存在，检查是否为空
//         user_file.seekg(0, std::ios::end);
//         if (user_file.tellg() == 0) {
//             // 文件为空，需要初始化
//             BlockNode headblock;
//             headblock.size = 1;
//             headblock.next_block = -1;
//             User root = User("root","sjtu",7,"shopkeeper");
//             headblock.users[0] = root;
//             write_block(user_file,headblock,0);
//         }
//     }
//     user_file.close();
// }
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
    // 保持文件打开
}

bool UserDatabase::Login(const std::string &UserID, const std::string &Password) {
    User* user = find(UserID);
    if (user == nullptr) {
        return false;
    }
    CurrentUser cur_user = CurrentUser(*user,"");
    int cur_Privilege = getCurrentPrivilege();
    bool canLogin = false;
    if (cur_Privilege > user->Privilege) {
        canLogin = true;  // 特权更高，无需密码
    } else if (strcmp(Password.c_str(), user->Password) == 0) {
        canLogin = true;  // 密码正确
    }
    if (canLogin) {
        Login_stack.push_back(cur_user);
        delete user;
        return true;
    }
    // if (cur_Privilege > user->Privilege || strcmp(Password.c_str(),user->Password) == 0) {
    //     Login_stack.push_back(cur_user);
    //     delete user;
    //     return true;
    // }
    //判断已经登录但是被覆盖
    // for (int i = 0; i < Login_stack.size(); ++i) {
    //     if (strcmp(Login_stack[i].user.UserID, UserID.c_str()) == 0) {
    //         Login_stack.push_back(cur_user);
    //         delete user;
    //         return true;
    //     }
    // }
    delete user;
    return false;
}

bool UserDatabase::Logout() {
    if (Login_stack.empty()) {
        return false;
    }
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
        if (c < 32 || c > 126) {
            return false;
        }
    }

    User* exist = find(UserID);
    if (exist != nullptr) {
        delete exist;
        return false;
    }

    User new_user = User(UserID,Password,1,Username);
    return insert(new_user);
}

bool UserDatabase::ChangePassword(const std::string &UserID, const std::string &CurrentPassword, const std::string &NewPassword) {
    User* user = find(UserID);
    if (user == nullptr) {
        return false;
    }
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
    User* exist = find(UserID);
    if (exist != nullptr) {
        delete exist;
        return false;
    }
    User new_user = User(UserID,Password,Privilege,Username);
    return insert(new_user);
}

bool UserDatabase::Delete(const std::string &UserID) {
    int cur_Privilege = getCurrentPrivilege();

    if (cur_Privilege != 7) {
        return false;
    }
    User* exist = find(UserID);
    if (exist == nullptr) {
        delete exist;
        return false;
    }
    bool flag = false;
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

User * UserDatabase::getCurrentUser() {
    if (Login_stack.empty()) {
        return nullptr;
    }
    User* user = new User();
    *user = Login_stack.back().user;
    return user;
}

int UserDatabase::getCurrentPrivilege() {
    if (Login_stack.empty()) {
        return 0;
    }
    return Login_stack.back().user.Privilege;
}

void UserDatabase::set_selected_book(const std::string& ISBN) {
    if (Login_stack.empty()) {
        return;
    }
    Login_stack.back().selected_book = ISBN;
}

std::string UserDatabase::get_selected_book() {
    if (Login_stack.empty()) {
        return "";
    }
    return Login_stack.back().selected_book;
}

void UserDatabase::flush() {
    if (user_file.is_open()) {
        user_file.flush();
    }
}