#include <algorithm>
#include <book.h>
#include <book_database.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <set>

// Book::write 实际写入 20+60+60+60+4+8=212 字节，而不是 sizeof(Book)=216 字节
const int BOOK_WRITE_SIZE = 20 + 60 + 60 + 60 + sizeof(int) + sizeof(double);  // 212 bytes
int book_block_size = BOOK_WRITE_SIZE * BLOCKSIZE + sizeof(int) * 2 + 21;  // +21 for max_ISBN

Book::Book() = default;

Book::Book(const std::string &ISBN) : Price(0.0), Quantity(0) {
    std::strncpy(this->ISBN, ISBN.c_str(), 20);
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
    file.read(BookName, 60);
    file.read(Author, 60);
    file.read(Keyword, 60);
    file.read(reinterpret_cast<char*>(&Quantity), sizeof(int));
    file.read(reinterpret_cast<char*>(&Price), sizeof(double));
}

void Book::write(std::fstream &file) {
    file.write(ISBN, 20);
    file.write(BookName, 60);
    file.write(Author, 60);
    file.write(Keyword, 60);
    file.write(reinterpret_cast<char*>(&Quantity), sizeof(int));
    file.write(reinterpret_cast<char*>(&Price), sizeof(double));
}

std::vector<std::string> Book::getKeywords() const {
    std::vector<std::string> keywords;
    std::string keyword;
    for (int i = 0; i < 60 && Keyword[i] != '\0'; ++i) {
        if (Keyword[i] != '|') {
            keyword += Keyword[i];
        }
        else {
            if (!keyword.empty()) {
                keywords.push_back(keyword);
            }
            keyword.clear();
        }
    }
    if (!keyword.empty()) {
        keywords.push_back(keyword);
    }
    return keywords;
}

bool Book::includeKeyword(const std::string &keyword) {
    if (keyword.empty()) {
        return true;
    }
    std::vector<std::string> keywords = getKeywords();
    if (std::find(keywords.begin(), keywords.end(), keyword) != keywords.end()) {
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
    if (keywords.size()) for (int i = 0; i < keywords.size() - 1; ++i) {
        std::cout << keywords[i] << '|';
    }
    if (keywords.size()) {
        std::cout << keywords.back();
    }
    std::cout << "\t" << std::fixed << std::setprecision(2) << Price << "\t" << Quantity;
}

BookDatabase::BookBlock::BookBlock() : size(0), next_block(-1) {
    for (int i = 0; i < BLOCKSIZE; ++i) {
        books[i] = Book();
    }
    max_ISBN[0] = '\0';
}

void BookDatabase::BookBlock::updateMaxISBN() {
    if (size > 0) {
        // 块内按ISBN排序，最后一个就是最大的
        strncpy(max_ISBN, books[size - 1].ISBN, 20);
        max_ISBN[20] = '\0';
    } else {
        max_ISBN[0] = '\0';
    }
}

void BookDatabase::BookBlock::read(std::fstream &file) {
    file.read(reinterpret_cast<char *>(&size), sizeof(int));
    file.read(reinterpret_cast<char *>(&next_block), sizeof(int));
    file.read(max_ISBN, 21);
    // 读取所有书籍槽位（包括未使用的），保证与write对应
    for (int i = 0; i < BLOCKSIZE; ++i) {
        books[i].read(file);
    }
}

void BookDatabase::BookBlock::write(std::fstream &file) {
    updateMaxISBN();
    file.write(reinterpret_cast<char *>(&size), sizeof(int));
    file.write(reinterpret_cast<char *>(&next_block), sizeof(int));
    file.write(max_ISBN, 21);
    // 写入所有书籍槽位（包括未使用的），保证块大小固定
    for (int i = 0; i < BLOCKSIZE; ++i) {
        books[i].write(file);
    }
}

void BookDatabase::read_block(std::fstream &file, BookBlock &block, int pos) {
    file.seekg(pos * book_block_size);
    block.read(file);
}

void BookDatabase::write_block(std::fstream &file, BookBlock &block, int pos) {
    file.seekp(pos * book_block_size);
    block.write(file);
}

// 重建内存中的块索引
void BookDatabase::rebuildBlockIndex() {
    block_index.clear();
    if (!book_file.is_open()) return;

    BookBlock block;
    int pos = 0;
    while (true) {
        read_block(book_file, block, pos);
        BlockIndex idx;
        idx.block_pos = pos;
        strncpy(idx.max_ISBN, block.max_ISBN, 20);
        idx.max_ISBN[20] = '\0';
        block_index.push_back(idx);

        if (block.next_block == -1) break;
        pos = block.next_block;
    }
}

// 更新某个块的索引，如果after_pos >= 0，表示新块应该插入到指定块之后
void BookDatabase::updateBlockIndex(int block_pos, const char* max_ISBN, int after_pos) {
    for (size_t i = 0; i < block_index.size(); ++i) {
        if (block_index[i].block_pos == block_pos) {
            strncpy(block_index[i].max_ISBN, max_ISBN, 20);
            block_index[i].max_ISBN[20] = '\0';
            return;
        }
    }
    // 如果不存在，需要添加新索引
    BlockIndex new_idx;
    new_idx.block_pos = block_pos;
    strncpy(new_idx.max_ISBN, max_ISBN, 20);
    new_idx.max_ISBN[20] = '\0';

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

// 根据ISBN找到目标块位置（只比较内存中的max值，实现根号n复杂度）
int BookDatabase::findTargetBlock(const std::string& ISBN) {
    if (block_index.empty()) return 0;

    // block_index 按照链表顺序存储
    // 遍历索引，找到第一个max_ISBN >= ISBN的块
    for (size_t i = 0; i < block_index.size(); ++i) {
        // 如果当前块的max为空，跳过
        if (block_index[i].max_ISBN[0] == '\0') continue;

        // 如果ISBN <= 当前块的max，或者是最后一个块，就返回这个块
        if (strcmp(ISBN.c_str(), block_index[i].max_ISBN) <= 0 ||
            i == block_index.size() - 1) {
            return block_index[i].block_pos;
        }
    }
    return block_index.back().block_pos;
}

bool BookDatabase::insertBook(const Book &book) {
    // 先检查是否已存在
    if (bookExists(book.ISBN)) {
        return false;
    }

    // 文件已在初始化时打开
    if (!book_file.is_open()) {
        return false;
    }

    // 使用内存索引找到目标块位置（只比较内存，不读磁盘）
    int pos = findTargetBlock(book.ISBN);

    BookBlock cur_block;
    read_block(book_file, cur_block, pos);

    // 找到插入位置（按ISBN字典序），使用二分查找
    int l = 0, r = cur_block.size;
    while (l < r) {
        int mid = l + (r - l) / 2;
        if (strcmp(cur_block.books[mid].ISBN, book.ISBN) < 0) {
            l = mid + 1;
        } else {
            r = mid;
        }
    }
    int insert_pos = l;

    if (cur_block.size < BLOCKSIZE) {
        // 当前块有空间，直接插入
        for (int i = cur_block.size; i > insert_pos; --i) {
            cur_block.books[i] = cur_block.books[i - 1];
        }
        cur_block.books[insert_pos] = book;
        cur_block.size++;
        write_block(book_file, cur_block, pos);
        addIndex(book, pos);
        updateBlockIndex(pos, cur_block.max_ISBN);
    }
    else {
        // 当前块已满，需要分块
        BookBlock new_block;
        int mid = cur_block.size / 2;
        int old_next_block = cur_block.next_block;

        // 计算新块位置
        book_file.seekp(0, std::ios::end);
        int new_pos = book_file.tellp() / book_block_size;

        // 将后半部分移到新块
        for (int i = mid; i < cur_block.size; ++i) {
            new_block.books[new_block.size] = cur_block.books[i];
            new_block.size++;
        }
        // 更新索引
        for (int i = mid; i < cur_block.size; ++i) {
            eraseIndex(cur_block.books[i], pos);
            addIndex(cur_block.books[i], new_pos);
        }
        cur_block.size = mid;

        // 设置链接
        new_block.next_block = old_next_block;
        cur_block.next_block = new_pos;

        // 写回两个块
        write_block(book_file, cur_block, pos);
        write_block(book_file, new_block, new_pos);

        // 更新内存中的块索引
        updateBlockIndex(pos, cur_block.max_ISBN);
        updateBlockIndex(new_pos, new_block.max_ISBN, pos);

        // 确定在哪个块插入
        if (insert_pos < mid) {
            // 插入到当前块
            for (int i = cur_block.size; i > insert_pos; --i) {
                cur_block.books[i] = cur_block.books[i - 1];
            }
            cur_block.books[insert_pos] = book;
            cur_block.size++;
            write_block(book_file, cur_block, pos);
            addIndex(book, pos);
            updateBlockIndex(pos, cur_block.max_ISBN);
        }
        else {
            // 插入到新块
            int new_insert_pos = insert_pos - mid;
            for (int i = new_block.size; i > new_insert_pos; --i) {
                new_block.books[i] = new_block.books[i - 1];
            }
            new_block.books[new_insert_pos] = book;
            new_block.size++;
            write_block(book_file, new_block, new_pos);
            addIndex(book, new_pos);
            updateBlockIndex(new_pos, new_block.max_ISBN);
        }
    }

    book_file.flush();
    return true;
}

bool BookDatabase::eraseBook(const std::string &ISBN) {
    // 先检查是否存在
    if (!bookExists(ISBN)) {
        return false;
    }
    // 文件已在初始化时打开
    if (!book_file.is_open()) {
        return false;
    }

    // 使用内存索引找到目标块位置
    int pos = findTargetBlock(ISBN);

    BookBlock cur_block, next_block;
    read_block(book_file, cur_block, pos);

    // 在该块中使用二分查找
    int l = 0, r = cur_block.size - 1;
    int erase_pos = -1;
    Book found_book;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = strcmp(cur_block.books[mid].ISBN, ISBN.c_str());
        if (cmp > 0) {
            r = mid - 1;
        } else if (cmp < 0) {
            l = mid + 1;
        } else {
            erase_pos = mid;
            found_book = cur_block.books[mid];
            break;
        }
    }

    if (erase_pos == -1) {
        return false;
    }

    // 删除索引
    eraseIndex(found_book, pos);
    // 删除图书
    for (int i = erase_pos; i < cur_block.size - 1; i++) {
        cur_block.books[i] = cur_block.books[i + 1];
    }
    cur_block.size--;

    // 检查是否需要合并块
    if (cur_block.size < BLOCKSIZE / 4 && cur_block.next_block != -1) {
        read_block(book_file, next_block, cur_block.next_block);
        if (cur_block.size + next_block.size < BLOCKSIZE) {
            // 合并到当前块
            int merged_pos = cur_block.next_block;
            for (int i = 0; i < next_block.size; i++) {
                cur_block.books[cur_block.size++] = next_block.books[i];
                // 更新索引位置
                eraseIndex(next_block.books[i], cur_block.next_block);
                addIndex(next_block.books[i], pos);
            }
            cur_block.next_block = next_block.next_block;
            write_block(book_file, cur_block, pos);
            updateBlockIndex(pos, cur_block.max_ISBN);
            // 移除被合并块的索引
            for (auto it = block_index.begin(); it != block_index.end(); ++it) {
                if (it->block_pos == merged_pos) {
                    block_index.erase(it);
                    break;
                }
            }
            book_file.flush();
            return true;
        }
    }

    // 写回当前块
    write_block(book_file, cur_block, pos);
    updateBlockIndex(pos, cur_block.max_ISBN);
    book_file.flush();
    return true;
}
Book *BookDatabase::findBookByISBN(const std::string &ISBN) {
    // 使用ISBN_map快速定位
    auto it = ISBN_map.find(ISBN);
    if (it == ISBN_map.end()) {
        return nullptr;
    }
    int block_pos = it->second;

    if (!book_file.is_open()) {
        return nullptr;
    }

    BookBlock cur_block;
    read_block(book_file, cur_block, block_pos);

    // 在该块中二分查找
    int l = 0, r = cur_block.size - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        int cmp = strcmp(cur_block.books[mid].ISBN, ISBN.c_str());
        if (cmp < 0) {
            l = mid + 1;
        }
        else if (cmp > 0) {
            r = mid - 1;
        }
        else {
            // 找到图书
            Book* found = new Book();
            *found = cur_block.books[mid];
            return found;
        }
    }
    return nullptr;
}

bool BookDatabase::updateBook(const Book &book) {
    // 文件已在初始化时打开
    if (!book_file.is_open()) {
        return false;
    }

    // 使用内存索引找到目标块位置
    int pos = findTargetBlock(book.ISBN);

    BookBlock cur_block;
    read_block(book_file, cur_block, pos);

    // 在该块中使用二分查找
    int left = 0, right = cur_block.size - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(cur_block.books[mid].ISBN, book.ISBN);
        if (cmp < 0) {
            left = mid + 1;
        } else if (cmp > 0) {
            right = mid - 1;
        } else {
            // 找到图书，先删除旧索引
            eraseIndex(cur_block.books[mid], pos);
            // 更新图书
            cur_block.books[mid] = book;
            write_block(book_file, cur_block, pos);
            // 添加新索引
            addIndex(book, pos);
            book_file.flush();
            return true;
        }
    }
    return false;
}

std::vector<Book> BookDatabase::findAllBooks() {
    std::vector<Book> all;
    if (!book_file.is_open()) {
        return all;
    }
    BookBlock cur_block;
    int pos = 0;
    while (pos != -1) {
        read_block(book_file, cur_block, pos);
        for (int i = 0; i < cur_block.size; ++i) {
            if (strlen(cur_block.books[i].ISBN) == 0) {
                continue;
            }
            all.push_back(cur_block.books[i]);
        }
        pos = cur_block.next_block;
    }
    std::sort(all.begin(), all.end());
    return all;
}

std::vector<Book> BookDatabase::findBooksByBookname(const std::string &bookname) {
    std::vector<Book> res;
    if (!book_file.is_open()) {
        return res;
    }
    BookBlock cur_block;
    int pos = 0;
    // 遍历所有块
    while (pos != -1) {
        read_block(book_file, cur_block, pos);
        for (int i = 0; i < cur_block.size; ++i) {
            if (strcmp(cur_block.books[i].BookName, bookname.c_str()) == 0 && strlen(cur_block.books[i].ISBN) > 0) {
                res.push_back(cur_block.books[i]);
            }
        }
        pos = cur_block.next_block;
    }
    std::sort(res.begin(), res.end());
    return res;
}

std::vector<Book> BookDatabase::findBooksByAuthor(const std::string &author) {
    std::vector<Book> res;
    if (!book_file.is_open()) {
        return res;
    }
    BookBlock cur_block;
    int pos = 0;
    // 遍历所有块
    while (pos != -1) {
        read_block(book_file, cur_block, pos);
        for (int i = 0; i < cur_block.size; ++i) {
            if (strcmp(cur_block.books[i].Author, author.c_str()) == 0 && strlen(cur_block.books[i].ISBN) > 0) {
                res.push_back(cur_block.books[i]);
            }
        }
        pos = cur_block.next_block;
    }
    std::sort(res.begin(), res.end());
    return res;
}

std::vector<Book> BookDatabase::findBooksByKeyword(const std::string &keyword) {
    std::vector<Book> res;
    if (!book_file.is_open()) {
        return res;
    }
    BookBlock cur_block;
    int pos = 0;
    // 遍历所有块
    while (pos != -1) {
        read_block(book_file, cur_block, pos);
        for (int i = 0; i < cur_block.size; ++i) {
            if (cur_block.books[i].includeKeyword(keyword) && strlen(cur_block.books[i].ISBN) > 0) {
                res.push_back(cur_block.books[i]);
            }
        }
        pos = cur_block.next_block;
    }
    std::sort(res.begin(), res.end());
    return res;
}

void BookDatabase::addIndex(const Book &book, int block_pos) {
    // 更新ISBN索引
    if (book.ISBN[0] != '\0') {
        ISBN_map[book.ISBN] = block_pos;
    }
    // 更新书名索引
    if (book.BookName[0] != '\0') {
        name_map[book.BookName].push_back(block_pos);
    }
    // 更新作者索引
    if (book.Author[0] != '\0') {
        author_map[book.Author].push_back(block_pos);
    }
    // 更新关键词索引
    std::vector<std::string> keywords = book.getKeywords();
    for (const auto& keyword : keywords) {
        if (!keyword.empty()) {
            keyword_map[keyword].push_back(block_pos);
        }
    }
}

void BookDatabase::eraseIndex(const Book &book, int block_pos) {
    // 删除ISBN索引
    if (book.ISBN[0] != '\0') {
        ISBN_map.erase(book.ISBN);
    }
    // 删除书名索引
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
    // 删除作者索引
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
    // 删除关键词索引
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

void BookDatabase::rebuildIndexInternal() {
    // 清空所有索引
    ISBN_map.clear();
    name_map.clear();
    author_map.clear();
    keyword_map.clear();
    // 文件已打开，直接读取
    if (!book_file.is_open()) {
        return;
    }
    BookBlock block;
    int pos = 0;
    while (true) {
        read_block(book_file, block, pos);
        for (int i = 0; i < block.size; ++i) {
            // 添加ISBN索引
            if (block.books[i].ISBN[0] != '\0') {
                ISBN_map[block.books[i].ISBN] = pos;
            }
            // 添加索引
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
}

void BookDatabase::rebuildIndex() {
    // 公开版本：如果文件未打开则打开
    bool needClose = false;
    if (!book_file.is_open()) {
        book_file.open(file_name, std::ios::in | std::ios::binary);
        needClose = true;
    }
    rebuildIndexInternal();
    if (needClose) {
        book_file.close();
    }
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
    book_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    if (!book_file) {
        // 文件不存在，创建并初始化
        book_file.open(file_name, std::ios::out | std::ios::binary);
        book_file.close();
        book_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        // 写入第一个块
        BookBlock headblock;
        headblock.size = 0;
        headblock.next_block = -1;
        write_block(book_file, headblock, 0);
    }
    else {
        // 文件已存在，检查是否为空
        book_file.seekg(0, std::ios::end);
        if (book_file.tellg() == 0) {
            // 文件为空，需要初始化
            BookBlock headblock;
            headblock.size = 0;
            headblock.next_block = -1;
            write_block(book_file, headblock, 0);
        }
    }
    // 重建内存中的块索引
    rebuildBlockIndex();
    // 重建哈希索引（文件保持打开）
    rebuildIndexInternal();
}

bool BookDatabase::Add(const Book &book) {
    // 检查ISBN是否已存在
    if (bookExists(book.ISBN)) {
        return false;
    }
    // 验证所有字段的合法性
    if (!isValidISBN(book.ISBN) || !isValidBookName(book.BookName) ||
        !isValidAuthor(book.Author) || !isValidKeyword(book.Keyword) ||
        !isValidQuantity(book.Quantity) || !isValidPrice(book.Price)) {
        return false;
    }
    // 插入图书
    return insertBook(book);
}

bool BookDatabase::Delete(const std::string &ISBN) {
    // 检查ISBN是否存在
    if (!bookExists(ISBN)) {
        return false;
    }
    // 删除图书
    return eraseBook(ISBN);
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
    // 检查购买数量是否合法
    if (Quantity <= 0) {
        return -1.0;
    }
    // 查找图书
    Book* book = findBookByISBN(ISBN);
    if (book == nullptr || book->Quantity < Quantity) {
        delete book;
        return -1.0;
    }
    // 更新库存
    book->Quantity -= Quantity;
    if (!updateBook(*book)) {
        delete book;
        return -1.0;
    }
    // 计算总价
    double expense = book->Price * Quantity;
    delete book;
    return expense;
}

bool BookDatabase::Select(const std::string &ISBN) {
    // 验证ISBN
    if (!isValidISBN(ISBN) || ISBN.empty()) {
        return false;
    }
    // 如果图书不存在，创建新图书
    if (!bookExists(ISBN)) {
        Book new_book(ISBN);
        new_book.Quantity = 0;
        new_book.Price = 0.0;
        new_book.BookName[0] = '\0';
        new_book.Author[0] = '\0';
        new_book.Keyword[0] = '\0';
        if (!insertBook(new_book)) {
            return false;
        }
    }
    // 设置选中图书
    selected_ISBN = ISBN;
    return true;
}

bool BookDatabase::Modify(int type, const std::string &info) {
    // 检查是否有选中图书
    if (selected_ISBN.empty()) {
        return false;
    }
    // 查找图书
    Book* book = findBookByISBN(selected_ISBN);
    if (book == nullptr) {
        return false;
    }
    Book old_book = *book;
    bool success = false;
    switch (type) {
        case 1: { // 修改ISBN
            if (!isValidISBN(info)) {
                delete book;
                return false;
            }
            // 检查新ISBN是否与旧ISBN相同
            if (info == old_book.ISBN) {
                delete book;
                return false;
            }
            // 检查新ISBN是否已存在
            if (bookExists(info)) {
                delete book;
                return false;
            }
            // 创建新书（使用新ISBN和旧书的其他信息）
            Book new_book = old_book;
            strncpy(new_book.ISBN, info.c_str(), 20);
            new_book.ISBN[20] = '\0';
            // 从文件中删除旧书
            if (!eraseBook(old_book.ISBN)) {
                delete book;
                return false;
            }
            // 插入新书
            if (insertBook(new_book)) {
                selected_ISBN = info;
                success = true;
            } else {
                // 插入失败，尝试恢复旧书
                insertBook(old_book);
                success = false;
            }
            break;
        }
        case 2: // 修改书名
            if (!isValidBookName(info)) {
                delete book;
                return false;
            }
            strncpy(book->BookName, info.c_str(), 60);
            book->BookName[60] = '\0';
            success = updateBook(*book);
            break;
        case 3: // 修改作者
            if (!isValidAuthor(info)) {
                delete book;
                return false;
            }
            strncpy(book->Author, info.c_str(), 60);
            book->Author[60] = '\0';
            success = updateBook(*book);
            break;
        case 4: // 修改关键词
            if (!isValidKeyword(info)) {
                delete book;
                return false;
            }
            strncpy(book->Keyword, info.c_str(), 60);
            book->Keyword[60] = '\0';
            success = updateBook(*book);
            break;
        case 5: { // 修改价格
            double price = std::stod(info);
            if (!isValidPrice(price)) {
                delete book;
                return false;
            }
            book->Price = price;
            success = updateBook(*book);
            break;
        }
        default:
            success = false;
            break;
    }

    delete book;
    return success;
}

bool BookDatabase::Import(int Quantity, double TotalCost) {
    // 验证参数
    if (!isValidQuantity(Quantity) || TotalCost <= 0) {
        return false;
    }
    // 检查是否有选中图书
    if (selected_ISBN.empty()) {
        return false;
    }
    // 查找图书
    Book* book = findBookByISBN(selected_ISBN);
    if (book == nullptr) {
        return false;
    }
    // 更新库存
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
    // 检查字符合法性
    for (char c : keyword) {
        if (c <= 32 || c > 126 || c == '"') {
            return false;
        }
    }
    // 检查关键词段是否重复
    std::vector<std::string> keywords;
    std::string cur;
    for (size_t i = 0; i < keyword.length(); ++i) {
        if (keyword[i] != '|') {
            cur += keyword[i];
        }
        else {
            if (cur.empty()) {
                return false;
            }
            keywords.push_back(cur);
            cur.clear();
        }
    }
    if (cur.empty()) {
        return false;
    }
    keywords.push_back(cur);
    // 检查重复
    std::set<std::string> tmp(keywords.begin(), keywords.end());
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

void BookDatabase::setSelectedISBN(const std::string& ISBN) {
    selected_ISBN = ISBN;
}

bool BookDatabase::bookExists(const std::string &ISBN) {
    if (!isValidISBN(ISBN)) {
        return false;
    }
    // 使用ISBN_map快速检查，O(1)复杂度
    return ISBN_map.find(ISBN) != ISBN_map.end();
}
