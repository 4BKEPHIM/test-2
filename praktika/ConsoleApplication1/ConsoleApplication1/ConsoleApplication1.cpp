#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// ==================== СТРУКТУРЫ ДАННЫХ ====================

struct Date {
    int day;
    int month;
    int year;
};

struct Book {
    int id;              // код книги
    char author[50];     // фамилия автора
    char title[100];     // название
    int year;            // год издания
    char language[20];   // язык издания
};

struct Reader {
    int id;              // код читателя
    char fio[100];       // ФИО
    char address[150];   // домашний адрес
    char phone[20];      // телефон
};

struct Loan {
    int reader_id;       // код читателя
    int book_id;         // код книги
    Date issue_date;     // дата выдачи
    Date return_date;    // дата сдачи (0.0.0 - книга не сдана)
    bool is_returned;
};

struct BookNode { Book data;   BookNode* next; };
struct ReaderNode { Reader data; ReaderNode* next; };
struct LoanNode { Loan data;   LoanNode* next; };

// указатели на начало списков
BookNode* m_books = nullptr;
ReaderNode* m_readers = nullptr;
LoanNode* m_loans = nullptr;
bool dataLoaded = false;

const char* BOOKS_FILE = "books.dat";
const char* READERS_FILE = "readers.dat";
const char* LOANS_FILE = "loans.dat";
const char* DEBTORS_FILE = "debtors.txt";
const int MAX_DAYS = 10;   // срок, после которого читатель считается должником

// ==================== ВВОД ДАННЫХ ====================

void pause() {
    cout << "\nНажмите Enter для продолжения...";
    string s;
    getline(cin, s);
}

void copyStr(char* dst, const string& src, int size) {
    strncpy(dst, src.c_str(), size - 1);
    dst[size - 1] = '\0';
}

bool isNumber(const string& s) {
    if (s.empty() || s.size() > 9) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

int inputInt(const string& prompt, int minV, int maxV) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        if (!isNumber(line)) {
            cout << "Ошибка: введите целое неотрицательное число!\n";
            continue;
        }
        int v = stoi(line);
        if (v < minV || v > maxV) {
            cout << "Ошибка: допустимы значения от " << minV << " до " << maxV << "!\n";
            continue;
        }
        return v;
    }
}

// ввод строки; если allowEmpty = false, пустая строка не принимается
string inputStr(const string& prompt, bool allowEmpty) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        if (!line.empty() || allowEmpty) return line;
        cout << "Ошибка: поле не может быть пустым!\n";
    }
}

bool askYesNo(const string& prompt) {
    return inputInt(prompt + " (1 - да, 0 - нет): ", 0, 1) == 1;
}

// ==================== РАБОТА С ДАТАМИ ====================

bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int daysInMonth(int m, int y) {
    const int d[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (m == 2 && isLeapYear(y)) return 29;
    return d[m - 1];
}

bool isValidDate(const Date& d) {
    if (d.year < 1900 || d.year > 2100) return false;
    if (d.month < 1 || d.month > 12) return false;
    return d.day >= 1 && d.day <= daysInMonth(d.month, d.year);
}

// порядковый номер дня, считая от 01.01.1900
long dateToDays(const Date& d) {
    long days = 0;
    for (int y = 1900; y < d.year; y++) days += isLeapYear(y) ? 366 : 365;
    for (int m = 1; m < d.month; m++) days += daysInMonth(m, d.year);
    return days + d.day;
}

string dateToString(const Date& d) {
    if (d.year == 0) return "-";
    char buf[16];
    sprintf(buf, "%02d.%02d.%04d", d.day, d.month, d.year);
    return buf;
}

// разбор строки вида ДД.ММ.ГГГГ
bool parseDate(const string& s, Date& d) {
    if (sscanf(s.c_str(), "%d.%d.%d", &d.day, &d.month, &d.year) != 3) return false;
    return isValidDate(d);
}

Date inputDate(const string& prompt) {
    Date d;
    while (true) {
        string line = inputStr(prompt + " (ДД.ММ.ГГГГ): ", false);
        if (parseDate(line, d)) return d;
        cout << "Ошибка: неверный формат или несуществующая дата!\n";
    }
}

Date today() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    Date d = { lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900 };
    return d;
}

// ==================== ОПЕРАЦИИ СО СПИСКАМИ ====================

void appendBook(const Book& b) {
    BookNode* node = new BookNode{ b, nullptr };
    if (!m_books) { m_books = node; return; }
    BookNode* p = m_books;
    while (p->next) p = p->next;
    p->next = node;
}

void appendReader(const Reader& r) {
    ReaderNode* node = new ReaderNode{ r, nullptr };
    if (!m_readers) { m_readers = node; return; }
    ReaderNode* p = m_readers;
    while (p->next) p = p->next;
    p->next = node;
}

void appendLoan(const Loan& l) {
    LoanNode* node = new LoanNode{ l, nullptr };
    if (!m_loans) { m_loans = node; return; }
    LoanNode* p = m_loans;
    while (p->next) p = p->next;
    p->next = node;
}

BookNode* findBook(int id) {
    for (BookNode* p = m_books; p; p = p->next)
        if (p->data.id == id) return p;
    return nullptr;
}

ReaderNode* findReader(int id) {
    for (ReaderNode* p = m_readers; p; p = p->next)
        if (p->data.id == id) return p;
    return nullptr;
}

// удаление из списка выдач всех записей, удовлетворяющих условию
int removeLoansBy(int readerId, int bookId) {
    int removed = 0;
    LoanNode* cur = m_loans;
    LoanNode* prev = nullptr;
    while (cur) {
        bool match = (readerId != -1 && cur->data.reader_id == readerId) ||
            (bookId != -1 && cur->data.book_id == bookId);
        if (match) {
            LoanNode* del = cur;
            if (prev) prev->next = cur->next; else m_loans = cur->next;
            cur = cur->next;
            delete del;
            removed++;
        }
        else {
            prev = cur;
            cur = cur->next;
        }
    }
    return removed;
}

int countLoans(int readerId, int bookId) {
    int n = 0;
    for (LoanNode* p = m_loans; p; p = p->next)
        if ((readerId != -1 && p->data.reader_id == readerId) ||
            (bookId != -1 && p->data.book_id == bookId)) n++;
    return n;
}

// количество прочитанных (сданных) читателем книг
int countReadBooks(int readerId) {
    int n = 0;
    for (LoanNode* p = m_loans; p; p = p->next)
        if (p->data.reader_id == readerId && p->data.is_returned) n++;
    return n;
}

void clearAllLists() {
    while (m_books) { BookNode* t = m_books;     m_books = m_books->next;     delete t; }
    while (m_readers) { ReaderNode* t = m_readers; m_readers = m_readers->next; delete t; }
    while (m_loans) { LoanNode* t = m_loans;     m_loans = m_loans->next;     delete t; }
}

// ==================== ВЫВОД ====================

void printBookHeader() {
    cout << left << setw(6) << "Код" << setw(20) << "Автор" << setw(32) << "Название"
        << setw(6) << "Год" << "Язык\n" << string(76, '-') << "\n";
}

void printBook(const Book& b) {
    cout << left << setw(6) << b.id << setw(20) << b.author << setw(32) << b.title
        << setw(6) << b.year << b.language << "\n";
}

void printReader(const Reader& r) {
    cout << left << setw(6) << r.id << setw(32) << r.fio << setw(36) << r.address << r.phone << "\n";
}

void printLoan(int num, const Loan& l) {
    cout << left << setw(4) << num << setw(10) << l.reader_id << setw(10) << l.book_id
        << setw(14) << dateToString(l.issue_date) << setw(14) << dateToString(l.return_date)
        << (l.is_returned ? "сдана" : "на руках") << "\n";
}

void showBooks() {
    cout << "\n=== СПИСОК КНИГ ===\n";
    if (!m_books) { cout << "Список пуст.\n"; return; }
    printBookHeader();
    for (BookNode* p = m_books; p; p = p->next) printBook(p->data);
}

void showReaders() {
    cout << "\n=== СПИСОК ЧИТАТЕЛЕЙ ===\n";
    if (!m_readers) { cout << "Список пуст.\n"; return; }
    cout << left << setw(6) << "Код" << setw(32) << "ФИО" << setw(36) << "Адрес" << "Телефон\n"
        << string(90, '-') << "\n";
    for (ReaderNode* p = m_readers; p; p = p->next) printReader(p->data);
}

void showLoans() {
    cout << "\n=== СПИСОК ВЫДАЧ ===\n";
    if (!m_loans) { cout << "Список пуст.\n"; return; }
    cout << left << setw(4) << "№" << setw(10) << "Читатель" << setw(10) << "Книга"
        << setw(14) << "Выдана" << setw(14) << "Сдана" << "Состояние\n" << string(62, '-') << "\n";
    int i = 1;
    for (LoanNode* p = m_loans; p; p = p->next) printLoan(i++, p->data);
}

// подменю выбора списка, возвращает 0 при отмене
int chooseList(const string& action) {
    cout << "\n--- " << action << " ---\n";
    cout << "1. Список книг\n2. Список читателей\n3. Список выдач\n0. Назад\n";
    return inputInt("Выберите список: ", 0, 3);
}

// ==================== ПУНКТ 1. ЧТЕНИЕ ИЗ ФАЙЛОВ ====================

void createTestData() {
    Book books[] = {
        { 1, "Толстой", "Война и мир", 2015, "русский" },
        { 2, "Достоевский", "Преступление и наказание", 2018, "русский" },
        { 3, "Короткевич", "Дикая охота короля Стаха", 2012, "белорусский" },
        { 4, "Orwell", "1984", 2008, "английский" },
        { 5, "Булгаков", "Мастер и Маргарита", 2020, "русский" },
        { 6, "Толстой", "Анна Каренина", 2016, "русский" },
        { 7, "Быков", "Сотников", 2019, "белорусский" }
    };
    Reader readers[] = {
        { 1, "Иванов Петр Сергеевич", "г. Минск, ул. Гикало, 9-12", "+375291112233" },
        { 2, "Ковалева Анна Игоревна", "г. Минск, пр. Независимости, 45-7", "+375335556677" },
        { 3, "Сидоренко Олег Викторович", "г. Минск, ул. Платонова, 3-41", "+375447778899" },
        { 4, "Петрова Мария Андреевна", "г. Минск, ул. Кальварийская, 21-5", "+375259990011" }
    };
    Loan loans[] = {
        { 1, 1, { 1, 9, 2026 },  { 8, 9, 2026 },  true },
        { 1, 4, { 10, 9, 2026 }, { 18, 9, 2026 }, true },
        { 1, 5, { 20, 9, 2026 }, { 0, 0, 0 },     false },
        { 2, 2, { 5, 9, 2026 },  { 12, 9, 2026 }, true },
        { 2, 3, { 3, 10, 2026 }, { 0, 0, 0 },     false },
        { 3, 6, { 12, 10, 2026 },{ 0, 0, 0 },     false },
        { 4, 7, { 15, 9, 2026 }, { 22, 9, 2026 }, true },
        { 4, 1, { 25, 9, 2026 }, { 1, 10, 2026 }, true },
        { 4, 2, { 2, 10, 2026 }, { 9, 10, 2026 }, true }
    };
    for (const Book& b : books) appendBook(b);
    for (const Reader& r : readers) appendReader(r);
    for (const Loan& l : loans) appendLoan(l);
}

bool saveToFiles();

bool loadFromFiles() {
    ifstream fb(BOOKS_FILE, ios::binary);
    ifstream fr(READERS_FILE, ios::binary);
    ifstream fl(LOANS_FILE, ios::binary);
    if (!fb.is_open() || !fr.is_open() || !fl.is_open()) return false;

    clearAllLists();
    Book b;
    while (fb.read((char*)&b, sizeof(Book))) appendBook(b);
    Reader r;
    while (fr.read((char*)&r, sizeof(Reader))) appendReader(r);
    Loan l;
    while (fl.read((char*)&l, sizeof(Loan))) appendLoan(l);
    return true;
}

void readData() {
    if (dataLoaded && !askYesNo("Несохраненные изменения будут потеряны. Продолжить?")) return;

    if (!loadFromFiles()) {
        cout << "Файлы данных не найдены.\n";
        if (!askYesNo("Создать файлы с тестовыми данными?")) { pause(); return; }
        clearAllLists();
        createTestData();
        if (!saveToFiles()) { clearAllLists(); pause(); return; }
        cout << "Тестовые файлы созданы.\n";
    }
    dataLoaded = true;
    int nb = 0, nr = 0, nl = 0;
    for (BookNode* p = m_books; p; p = p->next) nb++;
    for (ReaderNode* p = m_readers; p; p = p->next) nr++;
    for (LoanNode* p = m_loans; p; p = p->next) nl++;
    cout << "Данные загружены. Книг: " << nb << ", читателей: " << nr << ", выдач: " << nl << "\n";
    pause();
}

// ==================== ПУНКТ 2. ПРОСМОТР ====================

void viewData() {
    int ch = chooseList("Просмотр списка");
    if (ch == 1) showBooks();
    else if (ch == 2) showReaders();
    else if (ch == 3) showLoans();
    if (ch != 0) pause();
}

// ==================== ПУНКТ 3. СОРТИРОВКА ====================

void sortReadersByBooks() {
    if (!m_readers) { cout << "Список читателей пуст!\n"; pause(); return; }

    // сортировка вставками: узлы переносятся в новый список по убыванию
    ReaderNode* sorted = nullptr;
    ReaderNode* cur = m_readers;
    while (cur) {
        ReaderNode* nextNode = cur->next;
        int n = countReadBooks(cur->data.id);
        if (!sorted || countReadBooks(sorted->data.id) < n) {
            cur->next = sorted;
            sorted = cur;
        }
        else {
            ReaderNode* p = sorted;
            while (p->next && countReadBooks(p->next->data.id) >= n) p = p->next;
            cur->next = p->next;
            p->next = cur;
        }
        cur = nextNode;
    }
    m_readers = sorted;

    cout << "\n=== ЧИТАТЕЛИ ПО КОЛИЧЕСТВУ ПРОЧИТАННЫХ КНИГ ===\n";
    cout << left << setw(8) << "Книг" << setw(6) << "Код" << setw(32) << "ФИО" << "Телефон\n"
        << string(60, '-') << "\n";
    for (ReaderNode* p = m_readers; p; p = p->next)
        cout << left << setw(8) << countReadBooks(p->data.id) << setw(6) << p->data.id
        << setw(32) << p->data.fio << p->data.phone << "\n";
    pause();
}

// ==================== ПУНКТ 4. ПОИСК ====================

// перевод в нижний регистр (латиница и кириллица в кодировке 1251)
string toLower(const string& s) {
    string r = s;
    for (char& c : r) {
        unsigned char u = (unsigned char)c;
        if (u >= 'A' && u <= 'Z') c = (char)(u + 32);
        else if (u >= 0xC0 && u <= 0xDF) c = (char)(u + 32);
        else if (u == 0xA8) c = (char)0xB8;
    }
    return r;
}

bool contains(const char* text, const string& part) {
    return toLower(text).find(toLower(part)) != string::npos;
}

void searchBooks() {
    if (!m_books) { cout << "Список книг пуст!\n"; pause(); return; }

    cout << "\n=== ПОИСК КНИГ ===\n";
    cout << "1. По фамилии автора\n2. По названию\n3. По автору и названию\n0. Назад\n";
    int mode = inputInt("Выберите: ", 0, 3);
    if (mode == 0) return;

    string author, title;
    if (mode == 1 || mode == 3) author = inputStr("Автор (можно часть фамилии): ", false);
    if (mode == 2 || mode == 3) title = inputStr("Название (можно часть): ", false);

    int found = 0;
    for (BookNode* p = m_books; p; p = p->next) {
        bool ok = true;
        if (!author.empty() && !contains(p->data.author, author)) ok = false;
        if (!title.empty() && !contains(p->data.title, title)) ok = false;
        if (ok) {
            if (found == 0) { cout << "\n"; printBookHeader(); }
            printBook(p->data);
            found++;
        }
    }
    if (found == 0) cout << "Книги не найдены.\n";
    else cout << "Найдено книг: " << found << "\n";
    pause();
}

// ==================== ПУНКТ 5. ДОБАВЛЕНИЕ ====================

void addBook() {
    Book b;
    b.id = inputInt("Код книги: ", 1, 999999);
    if (findBook(b.id)) { cout << "Ошибка: книга с таким кодом уже есть!\n"; return; }
    copyStr(b.author, inputStr("Фамилия автора: ", false), 50);
    copyStr(b.title, inputStr("Название: ", false), 100);
    b.year = inputInt("Год издания: ", 1500, today().year);
    copyStr(b.language, inputStr("Язык издания: ", false), 20);
    appendBook(b);
    cout << "Книга добавлена.\n";
}

void addReader() {
    Reader r;
    r.id = inputInt("Код читателя: ", 1, 999999);
    if (findReader(r.id)) { cout << "Ошибка: читатель с таким кодом уже есть!\n"; return; }
    copyStr(r.fio, inputStr("ФИО: ", false), 100);
    copyStr(r.address, inputStr("Домашний адрес: ", false), 150);
    copyStr(r.phone, inputStr("Телефон: ", false), 20);
    appendReader(r);
    cout << "Читатель добавлен.\n";
}

void addLoan() {
    Loan l;
    l.reader_id = inputInt("Код читателя: ", 1, 999999);
    if (!findReader(l.reader_id)) { cout << "Ошибка: читатель не найден!\n"; return; }
    l.book_id = inputInt("Код книги: ", 1, 999999);
    if (!findBook(l.book_id)) { cout << "Ошибка: книга не найдена!\n"; return; }
    for (LoanNode* p = m_loans; p; p = p->next)
        if (p->data.book_id == l.book_id && !p->data.is_returned) {
            cout << "Ошибка: эта книга сейчас на руках у читателя " << p->data.reader_id << "!\n";
            return;
        }
    l.issue_date = inputDate("Дата выдачи");
    l.return_date = { 0, 0, 0 };
    l.is_returned = false;
    appendLoan(l);
    cout << "Выдача добавлена.\n";
}

void addData() {
    int ch = chooseList("Добавление");
    if (ch == 1) addBook();
    else if (ch == 2) addReader();
    else if (ch == 3) addLoan();
    if (ch != 0) pause();
}

// ==================== ПУНКТ 6. УДАЛЕНИЕ ====================

void deleteBook() {
    int id = inputInt("Код удаляемой книги: ", 1, 999999);
    BookNode* cur = m_books;
    BookNode* prev = nullptr;
    while (cur && cur->data.id != id) { prev = cur; cur = cur->next; }
    if (!cur) { cout << "Книга не найдена!\n"; return; }

    int n = countLoans(-1, id);
    if (n > 0 && !askYesNo("По книге есть записи о выдаче (" + to_string(n) + "). Удалить вместе с ними?"))
        return;
    removeLoansBy(-1, id);
    if (prev) prev->next = cur->next; else m_books = cur->next;
    delete cur;
    cout << "Книга удалена.\n";
}

void deleteReader() {
    int id = inputInt("Код удаляемого читателя: ", 1, 999999);
    ReaderNode* cur = m_readers;
    ReaderNode* prev = nullptr;
    while (cur && cur->data.id != id) { prev = cur; cur = cur->next; }
    if (!cur) { cout << "Читатель не найден!\n"; return; }

    int n = countLoans(id, -1);
    if (n > 0 && !askYesNo("У читателя есть записи о выдаче (" + to_string(n) + "). Удалить вместе с ними?"))
        return;
    removeLoansBy(id, -1);
    if (prev) prev->next = cur->next; else m_readers = cur->next;
    delete cur;
    cout << "Читатель удален.\n";
}

void deleteLoan() {
    if (!m_loans) { cout << "Список выдач пуст!\n"; return; }
    showLoans();
    int num = inputInt("Номер удаляемой записи: ", 1, 999999);
    LoanNode* cur = m_loans;
    LoanNode* prev = nullptr;
    for (int i = 1; cur && i < num; i++) { prev = cur; cur = cur->next; }
    if (!cur) { cout << "Записи с таким номером нет!\n"; return; }
    if (prev) prev->next = cur->next; else m_loans = cur->next;
    delete cur;
    cout << "Запись удалена.\n";
}

void deleteData() {
    int ch = chooseList("Удаление");
    if (ch == 1) deleteBook();
    else if (ch == 2) deleteReader();
    else if (ch == 3) deleteLoan();
    if (ch != 0) pause();
}

// ==================== ПУНКТ 7. РЕДАКТИРОВАНИЕ ====================

void editBook() {
    BookNode* p = findBook(inputInt("Код книги: ", 1, 999999));
    if (!p) { cout << "Книга не найдена!\n"; return; }
    cout << "Текущие данные: "; printBook(p->data);
    cout << "(Enter - оставить без изменений)\n";

    string s = inputStr("Автор: ", true);
    if (!s.empty()) copyStr(p->data.author, s, 50);
    s = inputStr("Название: ", true);
    if (!s.empty()) copyStr(p->data.title, s, 100);
    while (true) {
        s = inputStr("Год издания: ", true);
        if (s.empty()) break;
        if (isNumber(s) && stoi(s) >= 1500 && stoi(s) <= today().year) { p->data.year = stoi(s); break; }
        cout << "Ошибка: неверный год!\n";
    }
    s = inputStr("Язык: ", true);
    if (!s.empty()) copyStr(p->data.language, s, 20);
    cout << "Данные книги изменены.\n";
}

void editReader() {
    ReaderNode* p = findReader(inputInt("Код читателя: ", 1, 999999));
    if (!p) { cout << "Читатель не найден!\n"; return; }
    cout << "Текущие данные: "; printReader(p->data);
    cout << "(Enter - оставить без изменений)\n";

    string s = inputStr("ФИО: ", true);
    if (!s.empty()) copyStr(p->data.fio, s, 100);
    s = inputStr("Адрес: ", true);
    if (!s.empty()) copyStr(p->data.address, s, 150);
    s = inputStr("Телефон: ", true);
    if (!s.empty()) copyStr(p->data.phone, s, 20);
    cout << "Данные читателя изменены.\n";
}

void editLoan() {
    if (!m_loans) { cout << "Список выдач пуст!\n"; return; }
    showLoans();
    int num = inputInt("Номер записи: ", 1, 999999);
    LoanNode* p = m_loans;
    for (int i = 1; p && i < num; i++) p = p->next;
    if (!p) { cout << "Записи с таким номером нет!\n"; return; }

    cout << "(Enter - оставить без изменений)\n";
    while (true) {
        string s = inputStr("Дата выдачи (ДД.ММ.ГГГГ): ", true);
        Date d;
        if (s.empty()) break;
        if (parseDate(s, d)) { p->data.issue_date = d; break; }
        cout << "Ошибка: неверная дата!\n";
    }
    while (true) {
        string s = inputStr("Дата сдачи (ДД.ММ.ГГГГ, 0 - книга не сдана): ", true);
        Date d;
        if (s.empty()) break;
        if (s == "0") { p->data.return_date = { 0, 0, 0 }; p->data.is_returned = false; break; }
        if (parseDate(s, d)) {
            if (dateToDays(d) < dateToDays(p->data.issue_date)) {
                cout << "Ошибка: дата сдачи раньше даты выдачи!\n";
                continue;
            }
            p->data.return_date = d;
            p->data.is_returned = true;
            break;
        }
        cout << "Ошибка: неверная дата!\n";
    }
    cout << "Запись изменена.\n";
}

void editData() {
    int ch = chooseList("Редактирование");
    if (ch == 1) editBook();
    else if (ch == 2) editReader();
    else if (ch == 3) editLoan();
    if (ch != 0) pause();
}

// ==================== ПУНКТ 8. СПИСОК ДОЛЖНИКОВ ====================

void debtorsList() {
    Date cur;
    while (true) {
        string s = inputStr("Текущая дата ДД.ММ.ГГГГ (Enter - системная дата): ", true);
        if (s.empty()) { cur = today(); break; }
        if (parseDate(s, cur)) break;
        cout << "Ошибка: неверная дата!\n";
    }

    ofstream out(DEBTORS_FILE);
    if (!out.is_open()) cout << "Не удалось создать файл " << DEBTORS_FILE << "!\n";

    string head = "СПИСОК ДОЛЖНИКОВ на " + dateToString(cur) +
        " (книга не возвращена в течение " + to_string(MAX_DAYS) + " дней)\n";
    cout << "\n" << head;
    out << head;

    int count = 0;
    for (LoanNode* p = m_loans; p; p = p->next) {
        if (p->data.is_returned) continue;
        long days = dateToDays(cur) - dateToDays(p->data.issue_date);
        if (days <= MAX_DAYS) continue;

        ReaderNode* r = findReader(p->data.reader_id);
        BookNode* b = findBook(p->data.book_id);
        count++;
        string line = to_string(count) + ". " + (r ? r->data.fio : "читатель " + to_string(p->data.reader_id)) +
            ", тел. " + (r ? r->data.phone : "-") + "\n   книга: " +
            (b ? string(b->data.author) + " \"" + b->data.title + "\"" : "код " + to_string(p->data.book_id)) +
            ", выдана " + dateToString(p->data.issue_date) +
            ", на руках " + to_string(days) + " дн.\n";
        cout << line;
        out << line;
    }
    string total = count ? "Всего должников: " + to_string(count) + "\n" : "Должников нет.\n";
    cout << total;
    out << total;
    if (out.is_open()) cout << "Результат записан в файл " << DEBTORS_FILE << "\n";
    pause();
}

// ==================== ПУНКТЫ 9, 10. ВЫХОД ====================

bool saveToFiles() {
    ofstream fb(BOOKS_FILE, ios::binary);
    ofstream fr(READERS_FILE, ios::binary);
    ofstream fl(LOANS_FILE, ios::binary);
    if (!fb.is_open() || !fr.is_open() || !fl.is_open()) {
        cout << "Ошибка: не удалось открыть файлы для записи!\n";
        return false;
    }
    for (BookNode* p = m_books; p; p = p->next) fb.write((char*)&p->data, sizeof(Book));
    for (ReaderNode* p = m_readers; p; p = p->next) fr.write((char*)&p->data, sizeof(Reader));
    for (LoanNode* p = m_loans; p; p = p->next) fl.write((char*)&p->data, sizeof(Loan));
    return true;
}

// ==================== ГЛАВНОЕ МЕНЮ ====================

int main() {
#ifdef _WIN32
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
#endif
    setlocale(LC_ALL, "Russian");

    while (true) {
        cout << "\n========= БИБЛИОТЕКА =========\n";
        cout << "1.  Чтение данных из файлов\n";
        cout << "2.  Просмотр списков\n";
        cout << "3.  Сортировка читателей по числу прочитанных книг\n";
        cout << "4.  Поиск книг по автору или названию\n";
        cout << "5.  Добавление записи\n";
        cout << "6.  Удаление записи\n";
        cout << "7.  Редактирование записи\n";
        cout << "8.  Список должников\n";
        cout << "9.  Выход без сохранения\n";
        cout << "10. Выход с сохранением\n";
        cout << "==============================\n";
        int choice = inputInt("Выберите пункт: ", 1, 10);

        if (choice >= 2 && choice <= 8 && !dataLoaded) {
            cout << "Сначала загрузите данные (пункт 1)!\n";
            pause();
            continue;
        }
        switch (choice) {
        case 1: readData(); break;
        case 2: viewData(); break;
        case 3: sortReadersByBooks(); break;
        case 4: searchBooks(); break;
        case 5: addData(); break;
        case 6: deleteData(); break;
        case 7: editData(); break;
        case 8: debtorsList(); break;
        case 9:
            clearAllLists();
            cout << "Работа завершена без сохранения.\n";
            return 0;
        case 10:
            if (dataLoaded) {
                if (!saveToFiles()) { pause(); break; }
                cout << "Данные сохранены в файлы.\n";
            }
            else {
                cout << "Данные не загружались, файлы не изменены.\n";
            }
            clearAllLists();
            cout << "Работа завершена.\n";
            return 0;
        }
    }
}
