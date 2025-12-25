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
std::vector<std::string> splitCommand(const std::string& command) {
    std::vector<std::string> tokens;
    std::string token;
    bool flag = false;
    for (size_t i = 0; i < command.length(); ++i) {
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

// 记录日志
void logOperation(const std::string& operation) {
    User* currentUser = MyUserDatabase.getCurrentUser();
    std::string operatorID = currentUser ? std::string(currentUser->UserID) : "guest";
    int privilege = MyUserDatabase.getCurrentPrivilege();
    MyLogDatabase.addLog(operatorID, operation, privilege);
}

//解析show和modify的参数
bool parse(const std::vector<std::string>& tokens,
               std::vector<std::pair<std::string, std::string>>& info) {
    std::cerr << "entering parse" << std::endl;
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

        std::cerr << key << "/" << value << std::endl;

        // 去除value中的双引号
        if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }

        info.emplace_back(key, value);
    }
    std::cerr << "parse finished" << std::endl;
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
        std::cerr << "still alive" << std::endl;
        std::vector<std::string> tokens = splitCommand(line);
        if (tokens.empty()) {
            continue;
        }
        for (auto tok : tokens) {
            std::cerr << tok << std::endl;
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
                    std::cerr << "bad priv" << std::endl;
                    std::cout << "Invalid\n";
                    continue;
                }
                if (tokens.size() < 2 || tokens.size() > 3) {
                    std::cerr << "bad size" << std::endl;
                    std::cout << "Invalid\n";
                    continue;
                }
                std::string userID = tokens[1];
                std::string password = tokens.size() == 3 ? tokens[2] : "";

                std::cerr << userID << " " << password << std::endl;

                if (MyUserDatabase.Login(userID, password)) {
                    //logOperation("su " + userID);
                } else {
                    std::cerr << "bad login" << std::endl;
                    std::cout << "Invalid\n";
                }
            }

            else if (command == "logout") {
                if (!checkPrivilege(1)) {
                    std::cout << "Invalid\n";
                    continue;
                }

                if (MyUserDatabase.Logout()) {
                    //logOperation("logout");
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
                    //logOperation("register " + userID);
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
                int privilege = std::stoi(tokens[3]);
                std::string username = tokens[4];

                if (privilege != 1 && privilege != 3 && privilege != 7) {
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
                    } else if (tokens.size() == 3) {
                        int count = std::stoi(tokens[2]);
                        MyDealDatabase.showDeal(count);
                        //logOperation("show finance " + tokens[2]);
                    } else {
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
                        std::cerr << info.size() << std::endl;
                        auto& it = info[0];
                        if (it.first == "-ISBN") {
                            std::cerr << it.second << std::endl;
                            Book* book = MyBookDatabase.showBooksByISBN(it.second);
                            std::cerr << "sa" << std::endl;
                            if (book) {
                                books.push_back(*book);
                                delete book;
                            }
                        } else if (it.first == "-name") {
                            books = MyBookDatabase.showBooksByName(it.second);
                        } else if (it.first == "-author") {
                            books = MyBookDatabase.showBooksByAuthor(it.second);
                        } else if (it.first == "-keyword") {
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
                    std::cerr << books.size() << std::endl;
                    if (books.empty()) {
                        std::cout << '\n';
                    }
                    else {
                        for (const auto& book : books) {
                            book.print();
                            std::cout << "\n";
                        }
                    }
                    std::cerr << "printed" << std::endl;
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
                int quantity = std::stoi(tokens[2]);

                if (quantity <= 0) {
                    std::cout << "Invalid\n";
                    continue;
                }

                double expense = MyBookDatabase.Buy(ISBN, quantity);
                if (expense > 0) {
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

                std::cerr << "sa1" << std::endl;
                std::string ISBN = tokens[1];
                std::cerr << "sa2" << std::endl;
                MyBookDatabase.Select(ISBN);
                std::cerr << "sa3" << std::endl;
                std::string selectedISBN = MyBookDatabase.getSelectedISBN();
                std::cerr << "sa4" << std::endl;
                MyUserDatabase.set_selected_book(selectedISBN);
                std::cerr << "sa5" << std::endl;
                // std::cout << selectedISBN << std::endl;
                // //logOperation("select " + ISBN);
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

                // 检查是否选中图书
                std::string selectedISBN = MyBookDatabase.getSelectedISBN();
                if (selectedISBN.empty()) {
                    std::cout << "Invalid \n";
                    continue;
                }

                // 逐一修改
                for (const auto& it : info) {
                    int type = 0;
                    if (it.first == "-ISBN") type = 1;
                    else if (it.first == "-name") type = 2;
                    else if (it.first == "-author") type = 3;
                    else if (it.first == "-keyword") type = 4;
                    else if (it.first == "-price") type = 5;
                    else {
                        std::cout << "Invalid\n";
                        continue;
                    }

                    if (!MyBookDatabase.Modify(type, it.second)) {
                        std::cout << "Invalid\n";
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

                int quantity = std::stoi(tokens[1]);
                double totalCost = std::stod(tokens[2]);

                if (quantity <= 0 || totalCost <= 0) {
                    std::cout << "Invalid\n";
                    continue;
                }

                if (MyBookDatabase.Import(quantity, totalCost)) {
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
    // std::ofstream("book.txt", std::ios::trunc).close();
    // std::ofstream("user.txt", std::ios::trunc).close();
    // std::ofstream("log.txt", std::ios::trunc).close();
    // std::ofstream("deal.txt", std::ios::trunc).close();
    return 0;
}