#include "user.h"

#include <algorithm>

#include "user_database.h"
#include <iostream>
#include <fstream>
#include <cstring>
int user_block_size = sizeof(User) * BLOCKSIZE + sizeof(int) * 2;

User::User() = default;

User::User(const std::string& UserID, const std::string& Password, int Privilege, const std::string& Username) {
    strcpy(this->UserID,UserID.c_str());
    strcpy(this->Password,Password.c_str());
    this->Privilege = Privilege;
    strcpy(this->Username,Username.c_str());
}

void User::read(std::fstream &file) {
    file.read(UserID, 30);
    file.read(Password,30);
    file.read(Username,30);
    file.read(reinterpret_cast<char*>(&Privilege),sizeof(int));
}

void User::write(std::fstream &file) {
    file.write(UserID, 30);
    file.write(Password,30);
    file.write(Username,30);
    file.write(reinterpret_cast<char*>(&Privilege),sizeof(int));
}

CurrentUser::CurrentUser(const User &User, const std::string &Book) {
    user = User;
    selected_book = Book;
}

UserDatabase::BlockNode::BlockNode() : size(0),next_block(-1) {};

void UserDatabase::BlockNode::read(std::fstream& file) {
    file.read(reinterpret_cast<char *> (&size), sizeof(int));
    file.read(reinterpret_cast<char *> (&next_block),sizeof(int));
    for (int i = 0; i < size; ++i) {
        users[i].read(file);
    }
}

void UserDatabase::BlockNode::write(std::fstream& file) {
    file.write(reinterpret_cast<char *> (&size),sizeof(int));
    file.write(reinterpret_cast<char *> (&next_block),sizeof(int));
    for (int i = 0; i < size; ++i) {
        users[i].write(file);
    }
}

void UserDatabase::read_block(std::fstream& file, BlockNode &node, int pos) {
    file.seekg(pos * user_block_size);
    file.read(reinterpret_cast<char *> (&node.size),sizeof(int));
    file.read(reinterpret_cast<char *> (&node.next_block),sizeof(int));
    for (int i = 0; i < node.size; ++i) {
        node.users[i].read(file);
    }
}

void UserDatabase::write_block(std::fstream& file, BlockNode &node, int pos) {
    file.seekp(pos * user_block_size);
    file.write(reinterpret_cast<char *> (&node.size),sizeof(int));
    file.write(reinterpret_cast<char *> (&node.next_block),sizeof(int));
    for (int i = 0; i < node.size; ++i) {
        node.users[i].write(file);
    }

}

bool UserDatabase::insert(const User& user) {
    User* exist = find(user.UserID);
    if (exist != nullptr) {
        delete exist;
        user_file.close();
        return false;
    }
    delete exist;
    user_file.open(file_name,std::ios::in | std::ios::out);
    BlockNode cur_block;
    int pos = 0;
    bool flag = false;
    while (!flag) {
        read_block(user_file,cur_block,pos);
        int insert_pos = 0;
        while (insert_pos < cur_block.size && strcmp(cur_block.users[insert_pos].UserID,user.UserID) < 0) {
            insert_pos++;
        }
        if (cur_block.size < BLOCKSIZE) {
            for (int i = cur_block.size; i > insert_pos; --i) {
                cur_block.users[i] = cur_block.users[i - 1];
            }
            cur_block.users[insert_pos] = user;
            cur_block.size++;
            write_block(user_file,cur_block,pos);
            flag = true;
        }
        else {
            //尝试分块
            if (cur_block.next_block == -1) {
                BlockNode new_block = BlockNode();
                int mid = cur_block.size / 2;
                for (int i = mid; i < cur_block.size; ++i) {
                    new_block.users[new_block.size] = cur_block.users[i];
                    new_block.size++;
                }
                cur_block.size = mid;
                new_block.next_block = cur_block.next_block;
                cur_block.next_block = user_file.tellp() / user_block_size;
                write_block(user_file,cur_block,pos);
                write_block(user_file,new_block,cur_block.next_block);
                if (insert_pos >= mid) {
                    pos = cur_block.next_block;
                }
                else {
                    for (int i = cur_block.size; i > insert_pos; --i) {
                        cur_block.users[i] = cur_block.users[i - 1];
                    }
                    cur_block.users[insert_pos] = user;
                    cur_block.size++;
                    write_block(user_file,cur_block,pos);
                    flag = true;
                }
            }
            else {
                pos = cur_block.next_block;
            }
        }
    }
    user_file.close();
    return true;
}

bool UserDatabase::erase(const std::string &UserID) {
    user_file.open(file_name,std::ios::in | std::ios::out);
    BlockNode cur_block,next_block;
    int pos = 0;
    while (pos != -1) {
        read_block(user_file,cur_block,pos);
        int erase_pos = -1;
        for (int i = 0; i < cur_block.size; i++) {
            if (strcmp(cur_block.users[i].UserID, UserID.c_str()) == 0) {
                erase_pos = i;
                break;
            }
        }
        if (erase_pos != -1) {
            for (int i = erase_pos; i < cur_block.size - 1; ++i) {
                cur_block.users[i] = cur_block.users[i + 1];
            }
            cur_block.size--;
            //尝试合并
            if (cur_block.size < BLOCKSIZE / 4 && cur_block.next_block != -1) {
                read_block(user_file,next_block,cur_block.next_block);
                if (cur_block.size + next_block.size < BLOCKSIZE) {
                    for (int i = 0; i < next_block.size; ++i) {
                        cur_block.users[cur_block.size++] = next_block.users[i];
                    }
                    cur_block.next_block = next_block.next_block;
                    write_block(user_file, cur_block,pos);
                    user_file.close();
                    return true;
                }
            }
            write_block(user_file,cur_block,pos);
            user_file.close();
            return true;
        }
        pos = cur_block.next_block;
    }
    user_file.close();
    return false;
}

User *UserDatabase::find(const std::string &UserID) {
    user_file.open(file_name,std::ios::in | std::ios::out);
    if (!user_file) {
        user_file.close();
        return nullptr;
    }
    BlockNode cur_block;
    int pos = 0;
    while (pos != -1) {
        read_block(user_file,cur_block,pos);
        int l = 0,r = cur_block.size - 1;
        while (l <= r) {
            int mid = l + (r - l) / 2;
            int cmp = std::strcmp(cur_block.users[mid].UserID,UserID.c_str());
            if (cmp > 0) {
                r = mid - 1;
            }
            else if (cmp < 0) {
                l = mid + 1;
            }
            else {
                user_file.close();
                User* found = new User();
                *found = cur_block.users[mid];
                return found;
            }
        }
        pos = cur_block.next_block;
    }
    user_file.close();
    return nullptr;
}

bool UserDatabase::update(const User &user) {
    user_file.open(file_name,std::ios::in | std::ios::out);
    BlockNode cur_block;
    int pos = 0;
    while (pos != -1) {
        read_block(user_file,cur_block,pos);
        for (int i = 0; i < cur_block.size; i++) {
            if (strcmp(cur_block.users[i].UserID, user.UserID) == 0) {
                // 找到用户，更新信息
                cur_block.users[i] = user;
                write_block(user_file, cur_block, pos);
                user_file.close();
                return true;
            }
        }
        pos = cur_block.next_block;
    }
    user_file.close();
    return false;
}

UserDatabase::UserDatabase(const std::string &user_file) {
    if (!user_file.empty()) {
        file_name = user_file;
    }
    initialize();
}

UserDatabase::~UserDatabase() {
    Login_stack.clear();
    user_file.close();
}

void UserDatabase::initialize() {
    user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    if (!user_file) {
        // 文件不存在，创建并初始化
        user_file.open(file_name, std::ios::out | std::ios::binary);
        user_file.close();
        user_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);

        //写入第一个块
        BlockNode headblock;
        headblock.size = 1;
        headblock.next_block = -1;
        //创建店主root账户
        User root = User("root","sjtu",7,"shopkeeper");
        headblock.users[0] = root;
        write_block(user_file,headblock,0);
    } else {
        // 文件已存在，检查是否为空
        user_file.seekg(0, std::ios::end);
        if (user_file.tellg() == 0) {
            // 文件为空，需要初始化
            BlockNode headblock;
            headblock.size = 1;
            headblock.next_block = -1;
            User root = User("root","sjtu",7,"shopkeeper");
            headblock.users[0] = root;
            write_block(user_file,headblock,0);
        }
    }
    user_file.close();
}
// void UserDatabase::initialize() {
    // user_file.open(file_name,std::ios::in | std::ios::out);
    // if (!user_file) {
    //     user_file.open(file_name,std::ios::out);
    //     user_file.close();
    //     user_file.open(file_name,std::ios::in | std::ios::out);
    // }
    // //写入第一个块
    // BlockNode headblock;
    // headblock.size = 1;
    // headblock.next_block = -1;
    // //创建店主root账户
    // User root = User("root","sjtu",7,"shopkeeper");
    // headblock.users[0] = root;
    // write_block(user_file,headblock,0);

    // else {
    //     user_file.seekg(0, std::ios::end);
    //     if (user_file.tellg() == 0) {
    //         // 文件为空，初始化
    //         BlockNode headblock;
    //         headblock.size = 1;
    //         headblock.next_block = -1;
    //         User root = User("root", "sjtu", 7, "shopkeeper");
    //         headblock.users[0] = root;
    //         write_block(user_file, headblock, 0);
    //     }
    // }
//     user_file.close();
// }

bool UserDatabase::Login(const std::string &UserID, const std::string &Password) {
    User* user = find(UserID);
    if (user == nullptr) {
        delete user;
        return false;
    }
    CurrentUser cur_user = CurrentUser(*user,"");
    int cur_Privilege = getCurrentPrivilege();
    if (cur_Privilege > user->Privilege || strcmp(Password.c_str(),user->Password) == 0) {
        Login_stack.push_back(cur_user);
        delete user;
        return true;
    }
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












