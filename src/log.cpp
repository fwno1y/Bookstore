#include <cstring>
#include <user_database.h>
#include "log_database.h"
#include <fstream>
#include <iomanip>
#include <iostream>

Log::Log() = default;

Log::Log(const std::string &operator_ID, const std::string &operation, int Privilege) {
    strcpy(this->operator_ID,operator_ID.c_str());
    strcpy(this->operation,operation.c_str());
    this->Privilege = Privilege;
}

LogDatabase::LogDatabase(const std::string &filename) {
    this->file_name = filename;
    log_data.open(file_name, std::ios::in|std::ios::out);
    if (!log_data.is_open()) {
        log_data.open(file_name, std::ios::out);
        log_data.close();
        log_data.open(file_name, std::ios::in | std::ios::out);
    }
}

LogDatabase::~LogDatabase() {
    if (log_data.is_open()) {
        log_data.close();
    }
}

void LogDatabase::addLog(const std::string &operator_ID, const std::string &operation, int Privilege) {
    Log cur;
    std::strcpy(cur.operator_ID,operator_ID.c_str());
    std::strcpy(cur.operation,operation.c_str());
    cur.Privilege = Privilege;
    log_data.seekp(0,std::ios::end);
    log_data.write(reinterpret_cast<char*>(&cur),sizeof(Log));
    log_data.flush();
}

void LogDatabase::generateEmployeeReport() {
    log_data.seekp(0,std::ios::end);
    int end = log_data.tellp();
    int size = static_cast<int>(end / sizeof(Log));
    Log* EmployeeReport = new Log[size];
    log_data.seekp(0);
    log_data.read(reinterpret_cast<char *>(EmployeeReport), size * sizeof(Log));
    for (int i = 0; i < size; ++i) {
        if (EmployeeReport[i].Privilege == 3) {
            std::cout << EmployeeReport[i].operator_ID << '\t' << EmployeeReport[i].operation << '\n';
        }
    }
    delete[] EmployeeReport;
}

void LogDatabase::generateLogReport() {
    log_data.seekp(0,std::ios::end);
    int end = log_data.tellp();
    int size = static_cast<int> (end / sizeof(Log));
    Log* LogReport = new Log[size];
    log_data.seekp(0);
    log_data.read(reinterpret_cast<char *>(LogReport),size * sizeof(Log));
    for (int i = 0; i < size; ++i) {
        std::cout << LogReport[i].operator_ID << '\t' << LogReport[i].operation << '\n';
    }
    delete[] LogReport;
}

Deal::Deal() = default;

Deal::Deal(double income,double expense) {
    this->income = income;
    this->expense = expense;
}

DealDatabase::DealDatabase(const std::string &filename) {
    this->file_name = filename;
    deal_data.open(file_name, std::ios::in|std::ios::out);
    if (!deal_data.is_open()) {
        deal_data.open(file_name, std::ios::out);
        deal_data.close();
        deal_data.open(file_name, std::ios::in | std::ios::out);
    }
}

DealDatabase::~DealDatabase() {
    if (deal_data.is_open()) {
        deal_data.close();
    }

}

void DealDatabase::addDeal(double income, double expense) {
    Deal deal = Deal(income,expense);
    deal_data.seekp(0,std::ios::end);
    deal_data.write(reinterpret_cast<char*>(&deal),sizeof(Deal));
    deal_data.flush();
}

void DealDatabase::showDeal(int count) {
    if (count == 0) {
        std::cout << '\n';
        return;
    }
    deal_data.seekp(0,std::ios::end);
    int end = deal_data.tellp();
    int total_deals = end / sizeof(Deal);
    if (count > 0 && count > total_deals) {
        std::cout << "Invalid" << '\n';
        return;
    }
    if (count != -1) {
        end -= static_cast<int>(count * sizeof(Deal));
        deal_data.seekp(end);
    }
    else {
        count = total_deals;
        deal_data.seekp(0);
    }
    double total_income = 0,total_expense = 0;
    if (count == 0) {
        std::cout << "+ " << std::fixed << std::setprecision(2) << total_income << " - " << std::fixed << std::setprecision(2) << total_expense << '\n';
        return;
    }
    if (count > 0) {
        Deal* deal = new Deal[count + 3];
        deal_data.read(reinterpret_cast<char *>(deal),count * sizeof(Deal));
        for (int i = 0; i < count; ++i) {
            total_income += deal[i].income;
            total_expense += deal[i].expense;
        }
        std::cout << "+ " << std::fixed << std::setprecision(2) << total_income << " - " << std::fixed << std::setprecision(2) << total_expense << '\n';
        delete[] deal;
    }
}

void DealDatabase::generateDealReport() {
    showDeal(-1);
}
