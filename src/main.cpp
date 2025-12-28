#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>
#include "book_database.h"
#include "log_database.h"
#include "user_database.h"
BookDatabase MyBookDatabase("book.txt");
UserDatabase MyUserDatabase("user.txt");
LogDatabase MyLogDatabase("log.txt");
DealDatabase MyDealDatabase("deal.txt");
//分词
std::vector<std::string> splitCommand(const std::string& command) {
    std::vector<std::string> tokens;
    std::string token;
    bool flag = false;
    for (int i = 0; i < command.length(); ++i) {
        char c = command[i];
        if (c == '"') {
            flag = !flag;
            token += c;
        }
        else if (c == ' ' && !flag) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        }
        else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

// 检查权限
bool checkPrivilege(int required) {
    int cur = MyUserDatabase.getCurrentPrivilege();
    return cur >= required;
}

// 检查是否为纯数字字符串
bool isDigitString(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

// 检查是否为正整数
bool isPositiveInt(const std::string& s, int& result) {
    if (!isDigitString(s)) {
        return false;
    }
    if (s.length() > 10) {
        return false;
    }
    try {
        long long val = std::stoll(s);
        if (val <= 0 || val > 2147483647) return false;
        result = static_cast<int>(val);
        return true;
    } catch (...) {
        return false;
    }
}


bool isNotNegativeInt(const std::string& s, int& result) {
    if (!isDigitString(s)) {
        return false;
    }
    if (s.length() > 10) {
        return false;
    }
    try {
        long long val = std::stoll(s);
        if (val < 0 || val > 2147483647) return false;
        result = static_cast<int>(val);
        return true;
    } catch (...) {
        return false;
    }
}

// 检查小数点
bool checkDot(const std::string& s, double& result) {
    if (s.empty() || s.length() > 13) return false;
    bool flag = false;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (c == '.') {
            if (flag) {
                return false;
            }
            flag = true;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    if (s[0] == '.' || s.back() == '.' || s == ".") return false;
    try {
        result = std::stod(s);
        if (result <= 0) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// 记录日志
void logOperation(const std::string& operation) {
    User* currentUser = MyUserDatabase.getCurrentUser();
    std::string operatorID = currentUser ? std::string(currentUser->UserID) : "guest";
    int privilege = MyUserDatabase.getCurrentPrivilege();
    MyLogDatabase.addLog(operatorID, operation, privilege);
}

//解析show和modify的参数
bool parse(const std::vector<std::string>& tokens,std::vector<std::pair<std::string, std::string>>& info) {
    for (int i = 1; i < tokens.size(); ++i) {
        std::string token = tokens[i];
        if (token.empty() || token[0] != '-') {
            return false;
        }
        int pos = token.find('=');
        if (pos == std::string::npos) {
            return false;
        }
        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);
        // 检查是否需要引号
        if (key == "-name" || key == "-author" || key == "-keyword") {
            if (value.length() < 2 || value[0] != '"' || value.back() != '"') {
                return false;
            }
            value = value.substr(1, value.length() - 2);
            if (value.empty()) {
                return false;
            }
        }
        else if (key == "-ISBN" || key == "-price") {
            if (value.find('"') != std::string::npos) {
                return false;
            }
            if (value.empty()) {
                return false;
            }
            //检查小数点
            if (key == "-price") {
                bool flag = false;
                for (char c : value) {
                    if (c == '.') {
                        if (flag) {
                            return false;
                        }
                        flag = true;
                    } else if (!isdigit(c)) {
                        return false;
                    }
                }
                if (flag) {
                    int pos = value.find('.');
                    if (value.length() - pos - 1 > 2) {
                        return false;
                    }
                }
            }
        }
        else {
            return false;
        }
        info.emplace_back(key, value);
    }
    return true;
}



int main() {
    std::string line;
    while (getline(std::cin,line)) {
        if (line.empty()) {
            continue;
        }
        if (line.back() == '\r') {
            line.pop_back();
        }
        if (line.find('\t') != std::string::npos && line.find('\t') != line.size() - 1) {
            std::cout << "Invalid\n";
            continue;
        }
        std::vector<std::string> tokens = splitCommand(line);
        if (tokens.empty()) {
            continue;
        }
        const std::string& command = tokens[0];
        try {
            if (command == "quit" || command == "exit") {
                if (!checkPrivilege(0)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                MyUserDatabase.flush();
                break;
            }
            //账户系统指令
            else if (command == "su") {
                if (!checkPrivilege(0)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() < 2 || tokens.size() > 3) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                std::string password = tokens.size() == 3 ? tokens[2] : "";
                // 先保存当前用户的选中图书到登录栈
                std::string currentSelectedBook = MyBookDatabase.getSelectedISBN();
                MyUserDatabase.set_selected_book(currentSelectedBook);
                if (MyUserDatabase.Login(userID, password)) {
                    MyBookDatabase.setSelectedISBN("");
                    // logOperation("su " + userID);
                } else {
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "logout") {
                if (!checkPrivilege(1)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (MyUserDatabase.Logout()) {
                    // 更新选中的图书为当前登录用户的选中图书
                    std::string selectedBook = MyUserDatabase.get_selected_book();
                    MyBookDatabase.setSelectedISBN(selectedBook);
                    // logOperation("logout");
                } else {
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "register") {
                if (!checkPrivilege(0)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 4) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                std::string password = tokens[2];
                std::string username = tokens[3];
                if (MyUserDatabase.Register(userID, password, username)) {
                    // logOperation("register " + userID);
                } else {
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "passwd") {
                if (!checkPrivilege(1)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 3 && tokens.size() != 4) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                std::string currentPassword, newPassword;
                if (tokens.size() == 3) {
                    // 省略当前密码，只有root可以
                    if (!checkPrivilege(7)) {
                        std::cout << "Invalid\n";
                        continue;
                    }
                    newPassword = tokens[2];
                    if (!MyUserDatabase.ChangePassword(userID, "", newPassword)) {
                        std::cout << "Invalid\n";
                    } else {
                        //logOperation("passwd " + userID);
                    }
                } else {
                    currentPassword = tokens[2];
                    newPassword = tokens[3];
                    if (!MyUserDatabase.ChangePassword(userID, currentPassword, newPassword)) {
                        std::cout << "Invalid\n";
                    } else {
                        //logOperation("passwd " + userID);
                    }
                }
            }

            else if (command == "useradd") {
                if (!checkPrivilege(3)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 5) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                std::string password = tokens[2];
                // 验证 privilege 是否为合法值
                if (tokens[3].length() != 1 || !isdigit(tokens[3][0])) {
                    std::cout << "Invalid\n";
                    continue;
                }
                int privilege = tokens[3][0] - '0';
                std::string username = tokens[4];
                if (privilege != 1 && privilege != 3) {//不能创建7的
                    std::cout << "Invalid\n";
                    continue;
                }
                if (MyUserDatabase.UserAdd(userID, password, privilege, username)) {
                    //logOperation("useradd " + userID);
                } else {
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "delete") {
                if (!checkPrivilege(7)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 2) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                if (MyUserDatabase.Delete(userID)) {
                    //logOperation("delete " + userID);
                } else {
                    std::cout << "Invalid\n";
                }
            }
            //图书系统指令
            else if (command == "show") {
                if (!checkPrivilege(1)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // show finance指令
                if (tokens.size() >= 2 && tokens[1] == "finance") {
                    if (!checkPrivilege(7)) {
                        std::cout << "Invalid\n";
                        continue;
                    }
                    if (tokens.size() == 2) {
                        MyDealDatabase.showDeal(-1);
                        //logOperation("show finance");
                    }
                    else if (tokens.size() == 3) {
                        int count;
                        if (!isNotNegativeInt(tokens[2], count)) {
                            std::cout << "Invalid\n";
                            continue;
                        }
                        MyDealDatabase.showDeal(count);
                        //logOperation("show finance " + tokens[2]);
                    }
                    else {
                        std::cout << "Invalid\n";
                    }
                }
                // show图书指令
                else {
                    std::vector<std::pair<std::string, std::string>> info;
                    if (!parse(tokens, info)) {
                        std::cout << "Invalid\n";
                        continue;
                    }
                    std::vector<Book> books;
                    if (info.empty()) {
                        // 显示所有图书
                        books = MyBookDatabase.showAllBooks();
                    }
                    else if (info.size() == 1) {
                        auto& it = info[0];
                        // 检查参数内容是否为空
                        if (it.second.empty()) {
                            std::cout << "Invalid\n";
                            continue;
                        }
                        if (it.first == "-ISBN") {
                            Book* book = MyBookDatabase.showBooksByISBN(it.second);
                            if (book) {
                                books.push_back(*book);
                                delete book;
                            }
                        }
                        else if (it.first == "-name") {
                            books = MyBookDatabase.showBooksByName(it.second);
                        }
                        else if (it.first == "-author") {
                            books = MyBookDatabase.showBooksByAuthor(it.second);
                        }
                        else if (it.first == "-keyword") {
                            if (it.second.find('|') != std::string::npos) {
                                std::cout << "Invalid\n";
                                continue;
                            }
                            books = MyBookDatabase.showBooksByKeyword(it.second);
                        } else {
                            std::cout << "Invalid\n";
                            continue;
                        }
                    } else {
                        std::cout << "Invalid\n";
                        continue;
                    }
                    // 输出图书信息
                    if (books.empty()) {
                        std::cout << '\n';
                    }
                    else {
                        for (const auto& book : books) {
                            book.print();
                            std::cout << "\n";
                        }
                    }
                    //logOperation("show books");
                }
            }

            else if (command == "buy") {
                if (!checkPrivilege(1)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 3) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string ISBN = tokens[1];
                int Quantity;
                if (!isPositiveInt(tokens[2], Quantity)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                double expense = MyBookDatabase.Buy(ISBN, Quantity);
                if (expense >= 0) {
                    std::cout << std::fixed << std::setprecision(2) << expense << "\n";
                    MyDealDatabase.addDeal(expense, 0);
                    //logOperation("buy " + ISBN + " " + std::to_string(quantity));
                } else {
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "select") {
                if (!checkPrivilege(3)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 2) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string ISBN = tokens[1];
                if (!MyBookDatabase.Select(ISBN)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string selectedISBN = MyBookDatabase.getSelectedISBN();
                MyUserDatabase.set_selected_book(selectedISBN);
            }

            else if (command == "modify") {
                if (!checkPrivilege(3)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() < 2) {
                    std::cout << "Invalid\n";
                    continue;
                }
                std::vector<std::pair<std::string, std::string>> info;
                if (!parse(tokens, info)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // 检查是否有重复参数
                std::vector<std::string> parameters;
                for (const auto& it : info) {
                    parameters.push_back(it.first);
                }
                std::set<std::string> tmp(parameters.begin(), parameters.end());
                if (tmp.size() < parameters.size()) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // 检查是否有空参数值
                bool hasEmptyValue = false;
                for (const auto& it : info) {
                    if (it.second.empty()) {
                        hasEmptyValue = true;
                        break;
                    }
                }
                if (hasEmptyValue || info.empty()) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // 检查是否选中图书
                std::string selectedISBN = MyBookDatabase.getSelectedISBN();
                if (selectedISBN.empty()) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // 验证所有参数的合法性
                bool isInvalid = true;
                for (const auto& it : info) {
                    int type = 0;
                    if (it.first == "-ISBN") type = 1;
                    else if (it.first == "-name") type = 2;
                    else if (it.first == "-author") type = 3;
                    else if (it.first == "-keyword") type = 4;
                    else if (it.first == "-price") type = 5;
                    else {
                        isInvalid = false;
                        break;
                    }
                }
                if (!isInvalid) {
                    std::cout << "Invalid\n";
                    continue;
                }

                // 逐一修改
                bool flag = false;
                for (const auto& it : info) {
                    int type = 0;
                    if (it.first == "-ISBN") type = 1;
                    else if (it.first == "-name") type = 2;
                    else if (it.first == "-author") type = 3;
                    else if (it.first == "-keyword") type = 4;
                    else if (it.first == "-price") type = 5;
                    if (!MyBookDatabase.Modify(type, it.second)) {
                        std::cout << "Invalid\n";
                        flag = true;
                        break;
                    }
                    else if (type == 1) {
                        MyUserDatabase.set_selected_book(it.second);
                    }
                }
                //logOperation("modify book");
            }

            else if (command == "import") {
                if (!checkPrivilege(3)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 3) {
                    std::cout << "Invalid\n";
                    continue;
                }
                // 验证 quantity 是否为合法正整数
                int Quantity;
                if (!isPositiveInt(tokens[1], Quantity)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                double totalCost;
                if (!checkDot(tokens[2], totalCost)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (MyBookDatabase.Import(Quantity, totalCost)) {
                    MyDealDatabase.addDeal(0, totalCost);
                    //logOperation("import " + std::to_string(quantity));
                } else {
                    std::cout << "Invalid\n";
                }
            }
            //日志系统指令
            else if (command == "log") {
                if (!checkPrivilege(7)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                MyLogDatabase.generateLogReport();
                //logOperation("log");
            }

            else if (command == "report") {
                if (!checkPrivilege(7)) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() != 2) {
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens[1] == "finance") {
                    MyDealDatabase.generateDealReport();
                    //logOperation("report finance");
                } else if (tokens[1] == "employee") {
                    MyLogDatabase.generateEmployeeReport();
                    //logOperation("report employee");
                } else {
                    std::cout << "Invalid\n";
                }
            }
            else {
                std::cout << "Invalid\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Invalid\n";
        }
    }
    return 0;
}