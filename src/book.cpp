#include <algorithm>
#include <book.h>
#include <book_database.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <set>

int book_block_size = sizeof(Book) * BLOCKSIZE + sizeof(int) * 2;

Book::Book() = default;

Book::Book(const std::string &ISBN) : Price(0.0),Quantity(0) {
    std::strncpy(this->ISBN,ISBN.c_str(),20);
    this->ISBN[20] = '\0';
    this->BookName[0] = '\0';
    this->Author[0] = '\0';
    this->Keyword[0] = '\0';
}

Book::~Book() = default;

Book::Book(const std::string &ISBN, const std::string &Bookname, const std::string &Author, const std::string &Keyword, int Quantity, double price) : Price(price), Quantity(Quantity) {
    strncpy(this->ISBN, ISBN.c_str(), 20);
    this->ISBN[20] = '\0';
    strncpy(this->BookName, Bookname.c_str(), 60);
    this->BookName[60] = '\0';
    strncpy(this->Author, Author.c_str(), 60);
    this->Author[60] = '\0';
    strncpy(this->Keyword, Keyword.c_str(), 60);
    this->Keyword[60] = '\0';
}


void Book::read(std::fstream &file) {
    file.read(ISBN, 20);
    ISBN[20] = '\0';
    file.read(BookName, 60);
    BookName[60] = '\0';
    file.read(Author, 60);
    Author[60] = '\0';
    file.read(Keyword, 60);
    Keyword[60] = '\0';
    file.read(reinterpret_cast<char*>(&Quantity),sizeof(int));
    file.read(reinterpret_cast<char*>(&Price),sizeof(double));
}

void Book::write(std::fstream &file) {
    file.write(ISBN,20);
    file.write(BookName,60);
    file.write(Author,60);
    file.write(Keyword,60);
    file.write(reinterpret_cast<char*>(&Quantity),sizeof(int));
    file.write(reinterpret_cast<char*>(&Price),sizeof(double));
}

std::vector<std::string> Book::getKeywords() const {
    std::vector<std::string> keywords;
    std::string keyword;
    for (int i = 0; i < 60 && Keyword[i] != '\0'; ++i) {
        if (Keyword[i] != '|') {
            keyword += Keyword[i];
        }
        else {
            keywords.push_back(keyword);
            keyword.clear();
        }
    }
    keywords.push_back(keyword);
    return keywords;
}

bool Book::includeKeyword(const std::string &keyword) {
    if (keyword.empty()) {
        return true;
    }
    std::vector<std::string> keywords = getKeywords();
    if (std::find(keywords.begin(),keywords.end(),keyword) != keywords.end()) {
        return true;
    }
    return false;
}

void Book::print() const {
    if (strlen(ISBN) == 0) {
        return;
    }
    std::cout << ISBN << "\t" << BookName << "\t" << Author << "\t";
    std::vector<std::string> keywords = getKeywords();
    for (int i = 0; i < keywords.size() - 1; ++i) {
        std::cout << keywords[i] << '|';
    }
    std::cout << keywords.back();
    std::cout << "\t" << std::fixed << std::setprecision(2) << Price<< "\t" << Quantity;
}

BookDatabase::BookBlock::BookBlock() : size(0),next_block(-1) {
    for (int i = 0; i < BLOCKSIZE; ++i) {
        books[i] = Book();
    }
}

void BookDatabase::BookBlock::read(std::fstream &file) {
    file.read(reinterpret_cast<char *>(&size),sizeof(int));
    file.read(reinterpret_cast<char *>(&next_block),sizeof(int));
    for (int i = 0; i < size; ++i) {
        books[i].read(file);
    }
}

void BookDatabase::BookBlock::write(std::fstream &file) {
    file.write(reinterpret_cast<char *>(&size),sizeof(int));
    file.write(reinterpret_cast<char *>(&next_block),sizeof(int));
    for (int i = 0; i < size; ++i) {
        books[i].write(file);
    }
}

void BookDatabase::read_block(std::fstream &file, BookBlock &block, int pos) {
    file.seekg(pos * book_block_size);
    file.read(reinterpret_cast<char *>(&block.size),sizeof(int));
    file.read(reinterpret_cast<char *> (&block.next_block),sizeof(int));
    for (int i = 0; i < block.size; ++i) {
        block.books[i].read(file);
    }
}

void BookDatabase::write_block(std::fstream &file, BookBlock &block, int pos) {
    file.seekp(pos * book_block_size);
    file.write(reinterpret_cast<char *>(&block.size),sizeof(int));
    file.write(reinterpret_cast<char *> (&block.next_block),sizeof(int));
    for (int i = 0; i < block.size; ++i) {
        block.books[i].write(file);
    }
}

bool BookDatabase::insertBook(const Book &book) {
    Book* exist = findBookByISBN(book.ISBN);
    if (exist != nullptr) {
        delete exist;
        book_file.close();
        return false;
    }
    delete exist;
    book_file.open(file_name,std::ios::in | std::ios::out | std::ios::binary);
    BookBlock cur_block;
    int pos = 0;
    bool flag = false;
    while (!flag) {
        read_block(book_file,cur_block,pos);
        int insert_pos = 0;
        while (insert_pos < cur_block.size && strcmp(cur_block.books[insert_pos].ISBN,book.ISBN) < 0) {
            insert_pos++;
        }
        if (cur_block.size < BLOCKSIZE) {
            for (int i = cur_block.size; i > insert_pos; --i) {
                cur_block.books[i] = cur_block.books[i - 1];
            }
            cur_block.books[insert_pos] = book;
            cur_block.size++;
            write_block(book_file,cur_block,pos);
            addIndex(book, pos);
            flag = true;
        }
        else {
            if (cur_block.next_block == -1) {
                BookBlock new_block = BookBlock();
                int mid = cur_block.size / 2;
                for (int i = mid; i < cur_block.size; ++i) {
                    new_block.books[new_block.size] = cur_block.books[i];
                    new_block.size++;
                }
                cur_block.size = mid;
                new_block.next_block = cur_block.next_block;
                cur_block.next_block = book_file.tellp() / book_block_size;
                write_block(book_file,cur_block,pos);
                  write_block(book_file,new_block,cur_block.next_block);
                addIndex(book, cur_block.next_block);
                if (insert_pos >= mid) {
                    pos = cur_block.next_block;
                }
                else {
                    for (int i = cur_block.size; i > insert_pos; --i) {
                        cur_block.books[i] = cur_block.books[i - 1];
                    }
                    cur_block.books[insert_pos] = book;
                    cur_block.size++;
                    write_block(book_file,cur_block,pos);
                    flag = true;
                }
            }
            else {
                pos = cur_block.next_block;
            }
        }
    }
    book_file.close();
    return true;
}

bool BookDatabase::eraseBook(const std::string &ISBN) {
    book_file.open(file_name,std::ios::in | std::ios::out | std::ios::binary);
    Book* book = findBookByISBN(ISBN);
    if (book == nullptr) {
        book_file.close();
        return false;
    }
    Book found = *book;
    delete book;
    BookBlock cur_block,next_block;
    int pos = 0;
    while (pos != -1) {
        read_block(book_file, cur_block, pos);
        // 在当前块中查找图书
        int erase_pos = -1;
        for (int i = 0; i < cur_block.size; i++) {
            if (strcmp(cur_block.books[i].ISBN, ISBN.c_str()) == 0) {
                eraseIndex(found,pos);
                erase_pos = i;
                break;
            }
        }
        if (erase_pos != -1) {
            for (int i = erase_pos; i < cur_block.size - 1; i++) {
                cur_block.books[i] = cur_block.books[i + 1];
            }
            cur_block.size--;
            if (cur_block.size < BLOCKSIZE / 4 && cur_block.next_block != -1) {
                read_block(book_file, next_block, cur_block.next_block);
                if (cur_block.size + next_block.size < BLOCKSIZE) {
                    for (int i = 0; i < next_block.size; i++) {
                        cur_block.books[cur_block.size++] = next_block.books[i];
                    }
                    cur_block.next_block = next_block.next_block;
                    // 写回当前块
                    write_block(book_file, cur_block, pos);
                    book_file.close();
                    return true;
                }
            }
            // 写回当前块
            write_block(book_file, cur_block, pos);
            book_file.close();
            return true;
        }
        pos = cur_block.next_block;
    }
    book_file.close();
    return false;
}

Book *BookDatabase::findBookByISBN(const std::string &ISBN) {
    auto it = ISBN_map.find(ISBN);
    if (it != ISBN_map.end()) {
        book_file.open(file_name, std::ios::in | std::ios::binary);
        int block_pos = it->second;
        BookBlock block;
        read_block(book_file, block, block_pos);
        for (int i = 0; i < block.size; ++i) {
            if (strcmp(block.books[i].ISBN, ISBN.c_str()) == 0) {
                Book* result = new Book();
                *result = block.books[i];
                book_file.close();
                return result;
            }
        }
        book_file.close();
    }

    return nullptr;
    // book_file.open(file_name,std::ios::in | std::ios::binary);
    // if (!book_file) {
    //     return nullptr;
    // }
    // BookBlock cur_block;
    // int pos = 0;
    // while (pos != -1) {
    //     read_block(book_file, cur_block, pos);
    //     // 在当前块中二分查找
    //     int l = 0, r = cur_block.size - 1;
    //     while (l <= r) {
    //         int mid = l + (r - l) / 2;
    //         int cmp = strcmp(cur_block.books[mid].ISBN, ISBN.c_str());
    //         if (cmp < 0) {
    //             l = mid + 1;
    //         }
    //         else if (cmp > 0) {
    //             r = mid - 1;
    //         }
    //         else {
    //             // 找到图书
    //             book_file.close();
    //             Book* found = new Book();
    //             *found = cur_block.books[mid];
    //             return found;
    //         }
    //     }
    //     pos = cur_block.next_block;
    // }
    // book_file.close();
    // return nullptr;
}

bool BookDatabase::updateBook(const Book &book) {
    Book* old_book = findBookByISBN(book.ISBN);
    if (!old_book) {
        delete old_book;
        return false;
    }
    auto it = ISBN_map.find(book.ISBN);
    if (it == ISBN_map.end()) {
        delete old_book;
        return false;
    }
    int block_pos = it->second;
    eraseIndex(*old_book, block_pos);
    delete old_book;
    book_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    BookBlock cur_block;
    read_block(book_file, cur_block, block_pos);
    for (int i = 0; i < cur_block.size; i++) {
        if (strcmp(cur_block.books[i].ISBN, book.ISBN) == 0) {
            cur_block.books[i] = book;
            write_block(book_file, cur_block, block_pos);
            addIndex(book,block_pos);
            book_file.close();
            return true;
        }
    }
    book_file.close();
    return false;
}

std::vector<Book> BookDatabase::findAllBooks() {
    std::vector<Book> all;
    book_file.open(file_name,std::ios::in | std::ios::binary);
    if (!book_file) {
        return all;
    }
    BookBlock cur_block;
    int pos = 0;
    while (pos != -1) {
        read_block(book_file,cur_block,pos);
        for (int i = 0; i < cur_block.size; ++i) {
            if (strlen(cur_block.books[i].ISBN) == 0) {
                continue;
            }
            all.push_back(cur_block.books[i]);
        }
        pos = cur_block.next_block;
    }
    book_file.close();
    std::sort(all.begin(),all.end());
    return all;
}

std::vector<Book> BookDatabase::findBooksByBookname(const std::string &bookname) {
    std::vector<Book> res;
    auto it = name_map.find(bookname);
    if (it != name_map.end()) {
        book_file.open(file_name,std::ios::in);
        for (int pos : name_map[bookname]) {
            BookBlock block;
            read_block(book_file,block,pos);
            for (int i = 0; i < block.size; ++i) {
                if (strcmp(block.books[i].BookName, bookname.c_str()) == 0 && strlen(block.books[i].ISBN) > 0) {
                    res.push_back(block.books[i]);
                }
            }
        }
        book_file.close();
    }
    std::sort(res.begin(), res.end());
    return res;
}

std::vector<Book> BookDatabase::findBooksByAuthor(const std::string &author) {
    std::vector<Book> res;
    auto it = author_map.find(author);
    if (it != author_map.end()) {
        book_file.open(file_name,std::ios::in);
        for (int pos : author_map[author]) {
            BookBlock block;
            read_block(book_file,block,pos);
            for (int i = 0; i < block.size; ++i) {
                if (strcmp(block.books[i].Author, author.c_str() ) == 0 && strlen(block.books[i].ISBN) > 0) {
                    res.push_back(block.books[i]);
                }
            }
        }
        book_file.close();
    }
    std::sort(res.begin(), res.end());
    return res;
}

std::vector<Book> BookDatabase::findBooksByKeyword(const std::string &keyword) {
    std::vector<Book> res;
    auto it = keyword_map.find(keyword);
    if (it != keyword_map.end()) {
        book_file.open(file_name,std::ios::in);
        for (int pos : keyword_map[keyword]) {
            BookBlock block;
            read_block(book_file,block,pos);
            for (int i = 0; i < block.size; ++i) {
                if (block.books[i].includeKeyword(keyword) && strlen(block.books[i].ISBN) > 0) {
                    res.push_back(block.books[i]);
                }
            }
        }
        book_file.close();
    }
    std::sort(res.begin(), res.end());
    return res;
}

void BookDatabase::addIndex(const Book &book, int block_pos) {
    ISBN_map[book.ISBN] = block_pos;
    if (book.BookName[0] != '\0') {
        name_map[book.BookName].push_back(block_pos);
    }
    if (book.Author[0] != '\0') {
        author_map[book.Author].push_back(block_pos);
    }
    std::vector<std::string> keywords = book.getKeywords();
    for (const auto& keyword : keywords) {
        keyword_map[keyword].push_back(block_pos);
    }
}

void BookDatabase::eraseIndex(const Book &book, int block_pos) {
    ISBN_map.erase(book.ISBN);
    if (book.BookName[0] != '\0') {
        auto name_it = name_map.find(book.BookName);
        if (name_it != name_map.end()) {
            auto& vec = name_it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), block_pos), vec.end());
            if (vec.empty()) {
                name_map.erase(name_it);
            }
        }
    }
    if (book.Author[0] != '\0') {
        auto author_it = author_map.find(book.Author);
        if (author_it != author_map.end()) {
            auto& vec = author_it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), block_pos), vec.end());
            if (vec.empty()) {
                author_map.erase(author_it);
            }
        }
    }

    std::vector<std::string> keywords = book.getKeywords();
    for (const auto& keyword : keywords) {
        if (!keyword.empty()) {
            auto kw_it = keyword_map.find(keyword);
            if (kw_it != keyword_map.end()) {
                auto& vec = kw_it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), block_pos), vec.end());
                if (vec.empty()) {
                    keyword_map.erase(kw_it);
                }
            }
        }
    }
}

void BookDatabase::rebuildIndex() {
    ISBN_map.clear();
    name_map.clear();
    author_map.clear();
    keyword_map.clear();
    // 重新构建索引
    book_file.open(file_name, std::ios::in | std::ios::binary);
    if (!book_file) {
        return;
    }
    int pos = 0;
    BookBlock block;
    while (true) {
        read_block(book_file, block, pos);
        for (int i = 0; i < block.size; ++i) {
            // 添加索引
            ISBN_map[block.books[i].ISBN] = pos;
            if (block.books[i].BookName[0] != '\0') {
                name_map[block.books[i].BookName].push_back(pos);
            }
            if (block.books[i].Author[0] != '\0') {
                author_map[block.books[i].Author].push_back(pos);
            }
            std::vector<std::string> keywords = block.books[i].getKeywords();
            for (const auto& keyword : keywords) {
                if (!keyword.empty()) {
                    keyword_map[keyword].push_back(pos);
                }
            }
        }
        if (block.next_block == -1) {
            break;
        }
        pos = block.next_block;
    }
    book_file.close();
}

BookDatabase::BookDatabase(const std::string &book_file) {
    if (!book_file.empty()) {
        file_name = book_file;
    }
    initialize();
}

BookDatabase::~BookDatabase() {
    if (book_file.is_open()) {
        book_file.close();
    }
}

void BookDatabase::initialize() {
    book_file.open(file_name,std::ios::in | std::ios::out);
    if (!book_file) {
        book_file.open(file_name,std::ios::out );
        book_file.close();
        book_file.open(file_name,std::ios::in | std::ios::out);
    }
    //写入第一个块
    BookBlock headblock;
    headblock.size = 1;
    headblock.next_block = -1;
    write_block(book_file,headblock,0);
    rebuildIndex();
    book_file.close();
}

bool BookDatabase::Add(const Book &book) {
    if (bookExists(book.ISBN)) {
        return false;
    }
    if (!isValidAuthor(book.Author) || !isValidBookName(book.BookName) || !isValidISBN(book.ISBN) || !isValidKeyword(book.Keyword) || !isValidPrice(book.Price) || !isValidQuantity(book.Quantity)) {
        return false;
    }
    if (!insertBook(book)) {
        return false;
    }
    return true;
}

bool BookDatabase::Delete(const std::string &ISBN) {
    if (!bookExists(ISBN)) {
        return false;
    }
    if (!eraseBook(ISBN)) {
        return false;
    }
    return true;
}

std::vector<Book> BookDatabase::showAllBooks() {
    return findAllBooks();
}

Book *BookDatabase::showBooksByISBN(const std::string &ISBN) {
    return findBookByISBN(ISBN);
}

std::vector<Book> BookDatabase::showBooksByName(const std::string &name) {
    return findBooksByBookname(name);
}

std::vector<Book> BookDatabase::showBooksByAuthor(const std::string &author) {
    return findBooksByAuthor(author);
}

std::vector<Book> BookDatabase::showBooksByKeyword(const std::string &keyword) {
    return findBooksByKeyword(keyword);
}

double BookDatabase::Buy(const std::string &ISBN, int Quantity) {
    Book* book = findBookByISBN(ISBN);
    if (book == nullptr || book->Quantity < Quantity) {
        delete book;
        return 0;
    }
    book->Quantity -= Quantity;
    updateBook(*book);
    double expense = book->Price * Quantity;
    delete book;
    return expense;
}

void BookDatabase::Select(const std::string &ISBN) {
    if (!isValidISBN(ISBN) || ISBN.empty()) {
        return;
    }
    if (!bookExists(ISBN)) {
        Book new_book(ISBN);
        new_book.Quantity = 0;
        new_book.Price = 0.0;
        new_book.BookName[0] = '\0';
        new_book.Author[0] = '\0';
        new_book.Keyword[0] = '\0';
        insertBook(new_book);
    }
    selected_ISBN = ISBN;
}

void BookDatabase::Modify(int type, const std::string &info) {
    if (selected_ISBN.empty()) {
        return;
    }
    Book* book = findBookByISBN(selected_ISBN);
    if (book == nullptr) {
        delete book;
        return;
    }
    Book old_book = *book;
    auto it = ISBN_map.find(selected_ISBN);
    if (it == ISBN_map.end()) {
        delete book;
        return;
    }
    int block_pos = it->second;
    switch (type) {
        case 1: // 修改ISBN
        {
            if (!isValidISBN(info)) {
                delete book;
                return;
            }
            // 检查新ISBN是否与旧ISBN相同
            if (info == old_book.ISBN) {
                delete book;
                return;
            }
            // 检查新ISBN是否已存在
            if (bookExists(info)) {
                delete book;
                return;
            }
            std::string old_ISBN = selected_ISBN;
            strncpy(book->ISBN, info.c_str(), 20);
            book->ISBN[20] = '\0';
            book_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
            BookBlock cur_block;
            read_block(book_file, cur_block, block_pos);
            int erase_pos = -1;
            for (int i = 0; i < cur_block.size; i++) {
                if (strcmp(cur_block.books[i].ISBN, old_ISBN.c_str()) == 0) {
                    erase_pos = i;
                    break;
                }
            }
            if (erase_pos != -1) {
                eraseIndex(old_book, block_pos);
                for (int i = erase_pos; i < cur_block.size - 1; i++) {
                    cur_block.books[i] = cur_block.books[i + 1];
                }
                cur_block.size--;
                write_block(book_file, cur_block, block_pos);
            }
            book_file.close();
            insertBook(*book);
            selected_ISBN = info;
            delete book;
            return;
        }
        case 2: // 修改书名
            if (!isValidBookName(info)) {
                delete book;
                return;
            }
            strncpy(book->BookName, info.c_str(), 60);
            book->BookName[60] = '\0';
            break;
        case 3: // 修改作者
            if (!isValidAuthor(info)) {
                delete book;
                return;
            }
            strncpy(book->Author, info.c_str(), 60);
            book->Author[60] = '\0';
            break;
        case 4: // 修改关键词
            if (!isValidKeyword(info)) {
                delete book;
                return;
            }
            strncpy(book->Keyword, info.c_str(), 60);
            book->Keyword[60] = '\0';
            break;
        case 5: // 修改价格
            try {
                double price = std::stod(info);
                if (!isValidPrice(price)) {
                    delete book;
                    return;
                }
                book->Price = price;
            } catch (...) {
                delete book;
                return;
            }
            break;
        default:
            delete book;
            return;
    }
    updateBook(*book);
    delete book;
}

bool BookDatabase::Import(int Quantity, long long TotalCost) {
    if (!isValidQuantity(Quantity) || TotalCost <= 0) {
        return false;
    }
    if (selected_ISBN.empty()) {
        return false;
    }
    Book* book = findBookByISBN(selected_ISBN);
    if (book == nullptr) {
        return false;
    }
    book->Quantity += Quantity;
    bool success = updateBook(*book);
    delete book;
    return success;
}

bool BookDatabase::isValidISBN(const std::string &ISBN) {
    if (ISBN.empty() || ISBN.length() > 20) {
        return false;
    }
    for (char c : ISBN) {
        if (c <= 32 || c > 126) {
            return false;
        }
    }
    return true;
}

bool BookDatabase::isValidBookName(const std::string &name) {
    if (name.empty() || name.length() > 60) {
        return false;
    }
    for (char c : name) {
        if (c <= 32 || c > 126 || c == '"') {
            return false;
        }
    }
    return true;
}

bool BookDatabase::isValidAuthor(const std::string &author) {
    if (author.empty() || author.length() > 60) {
        return false;
    }
    for (char c : author) {
        if (c <= 32 || c > 126 || c == '"') {
            return false;
        }
    }
    return true;
}

bool BookDatabase::isValidKeyword(const std::string &keyword) {
    if (keyword.empty() || keyword.length() > 60) {
        return false;
    }
    for (char c: keyword) {
        if (c <= 32 || c > 126 || c == '"') {
            return false;
        }
    }

    // 检查关键词段是否重复
    std::vector<std::string> keywords;
    std::string cur;
    for (int i = 0; i < 60 && keyword[i] != '\0'; ++i) {
        if (keyword[i] != '|') {
            cur += keyword[i];
        }
        else {
            keywords.push_back(cur);
            cur.clear();
        }
    }
    if (cur.empty()) {
        return false;
    }
    keywords.push_back(cur);
    std::set <std::string> tmp(keywords.begin(),keywords.end());
    if (tmp.size() < keywords.size()) {
        return false;
    }
    return true;
}

bool BookDatabase::isValidQuantity(int quantity) {
    return quantity >= 0 && quantity <= 2147483647;
}

bool BookDatabase::isValidPrice(double price) {
    return price >= 0 && price <= 99999999999.99;
}

std::string BookDatabase::getSelectedISBN() {
    return selected_ISBN;
}

bool BookDatabase::bookExists(const std::string &ISBN) {
    if (!isValidISBN(ISBN)) {
        return false;
    }
    if (ISBN_map.find(ISBN) != ISBN_map.end()) {
        return true;
    }
    return false;
}














