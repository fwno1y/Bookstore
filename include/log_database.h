#ifndef BOOKSTORE_2025_LOG_DATABASE_H
#define BOOKSTORE_2025_LOG_DATABASE_H
#include <iostream>
#include <fstream>

struct Log {
    char operator_ID[61]{};
    char operation[20]{};
    int Privilege{};

    Log();

    Log(const std::string& operator_ID,const std::string& operation,int Privilege);
};

class LogDatabase {
private:
    std::fstream log_data;
    std::string file_name;

public:
    LogDatabase() = default;

    explicit LogDatabase(const std::string& filename);

    ~LogDatabase();

    void addLog(const std::string& operator_ID, const std::string& operation, int Privilege);

    void generateEmployeeReport();

    void generateLogReport();

};
struct Deal {
    double income{};
    double expense{};

    Deal();

    Deal(double income,double expense);
};


class DealDatabase {
private:
    std::fstream deal_data;
    std::string file_name;

public:
    DealDatabase() = default;

    explicit DealDatabase(const std::string& filename);

    ~DealDatabase();

    void addDeal(double income, double expense);

    void showDeal(int count = -1);

    void generateDealReport();

};

#endif //BOOKSTORE_2025_LOG_DATABASE_H