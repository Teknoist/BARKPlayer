#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct Book {
    int id;
    std::string filepath;
    int last_position;
    int duration;
    bool is_finished;
    int last_played_at;

    Book()
        : id(0), last_position(0), duration(0), is_finished(false), last_played_at(0) {}
};

struct Bookmark {
    int id;
    int book_id;
    int position;
    std::string name;
    int created_at;

    Bookmark()
        : id(0), book_id(0), position(0), created_at(0) {}
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool init(const std::string& db_path);

    bool setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& default_value = "");

    bool updateBookProgress(const std::string& filepath, int position, int duration, bool is_finished);
    bool getBookProgress(const std::string& filepath, Book& out_book);
    std::string getLastPlayedFile();

    bool addBookmark(const std::string& filepath, int position, const std::string& name);
    std::vector<Bookmark> getBookmarks(const std::string& filepath);
    bool deleteBookmark(int bookmark_id);

    std::vector<Book> getHistory();

    bool migrateFromLegacy(const std::string& legacy_path);

private:
    sqlite3* db;

    bool executeQuery(const std::string& query);
    int getBookId(const std::string& filepath);
};

#endif // DATABASE_MANAGER_H
