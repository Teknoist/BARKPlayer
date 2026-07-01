#include "database_manager.h"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

#ifndef SQLITE_TRANSIENT
#define SQLITE_TRANSIENT ((sqlite3_destructor_type)-1)
#endif

DatabaseManager::DatabaseManager() : db(nullptr) {}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool DatabaseManager::init(const std::string& db_path) {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }

    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::printf("Can't open database: %s\n", db ? sqlite3_errmsg(db) : "unknown error");
        return false;
    }

    const char* sql_settings =
        "CREATE TABLE IF NOT EXISTS settings ("
        "key TEXT PRIMARY KEY, "
        "value TEXT);";

    const char* sql_books =
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "filepath TEXT UNIQUE NOT NULL, "
        "last_position INTEGER DEFAULT 0, "
        "duration INTEGER DEFAULT 0, "
        "is_finished INTEGER DEFAULT 0, "
        "last_played_at INTEGER);";

    const char* sql_bookmarks =
        "CREATE TABLE IF NOT EXISTS bookmarks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "book_id INTEGER, "
        "position INTEGER NOT NULL, "
        "name TEXT, "
        "created_at INTEGER, "
        "FOREIGN KEY(book_id) REFERENCES books(id) ON DELETE CASCADE);";

    if (!executeQuery("PRAGMA foreign_keys = ON;")) return false;
    if (!executeQuery(sql_settings)) return false;
    if (!executeQuery(sql_books)) return false;
    if (!executeQuery(sql_bookmarks)) return false;
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_books_last_played ON books(last_played_at DESC);")) return false;
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_bookmarks_book_position ON bookmarks(book_id, position ASC);")) return false;

    return true;
}

bool DatabaseManager::executeQuery(const std::string& query) {
    char* error_message = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        std::printf("SQL error: %s\n", error_message ? error_message : "unknown error");
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

bool DatabaseManager::setSetting(const std::string& key, const std::string& value) {
    const char* sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::string DatabaseManager::getSetting(const std::string& key, const std::string& default_value) {
    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return default_value;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    std::string result = default_value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value) result = std::string(reinterpret_cast<const char*>(value));
    }

    sqlite3_finalize(stmt);
    return result;
}

int DatabaseManager::getBookId(const std::string& filepath) {
    const char* sql = "SELECT id FROM books WHERE filepath = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, filepath.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return id;
}

bool DatabaseManager::updateBookProgress(const std::string& filepath, int position, int duration, bool is_finished) {
    if (filepath.empty()) return false;

    int id = getBookId(filepath);
    int current_time = static_cast<int>(std::time(nullptr));

    if (id > 0) {
        const char* sql = "UPDATE books SET last_position = ?, duration = ?, is_finished = ?, last_played_at = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, position);
        sqlite3_bind_int(stmt, 2, duration);
        sqlite3_bind_int(stmt, 3, is_finished ? 1 : 0);
        sqlite3_bind_int(stmt, 4, current_time);
        sqlite3_bind_int(stmt, 5, id);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    const char* sql = "INSERT INTO books (filepath, last_position, duration, is_finished, last_played_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, filepath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, position);
    sqlite3_bind_int(stmt, 3, duration);
    sqlite3_bind_int(stmt, 4, is_finished ? 1 : 0);
    sqlite3_bind_int(stmt, 5, current_time);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::getBookProgress(const std::string& filepath, Book& out_book) {
    const char* sql = "SELECT id, last_position, duration, is_finished, last_played_at FROM books WHERE filepath = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, filepath.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_book.id = sqlite3_column_int(stmt, 0);
        out_book.filepath = filepath;
        out_book.last_position = sqlite3_column_int(stmt, 1);
        out_book.duration = sqlite3_column_int(stmt, 2);
        out_book.is_finished = sqlite3_column_int(stmt, 3) != 0;
        out_book.last_played_at = sqlite3_column_int(stmt, 4);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

std::string DatabaseManager::getLastPlayedFile() {
    return getSetting("last_opened_file", "");
}

bool DatabaseManager::addBookmark(const std::string& filepath, int position, const std::string& name) {
    if (filepath.empty()) return false;

    int book_id = getBookId(filepath);
    if (book_id == 0) {
        if (!updateBookProgress(filepath, 0, 0, false)) return false;
        book_id = getBookId(filepath);
        if (book_id == 0) return false;
    }

    const char* sql = "INSERT INTO bookmarks (book_id, position, name, created_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, book_id);
    sqlite3_bind_int(stmt, 2, position);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(std::time(nullptr)));

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<Bookmark> DatabaseManager::getBookmarks(const std::string& filepath) {
    std::vector<Bookmark> bookmarks;
    int book_id = getBookId(filepath);
    if (book_id == 0) return bookmarks;

    const char* sql = "SELECT id, position, name, created_at FROM bookmarks WHERE book_id = ? ORDER BY position ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return bookmarks;

    sqlite3_bind_int(stmt, 1, book_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Bookmark bookmark;
        bookmark.id = sqlite3_column_int(stmt, 0);
        bookmark.book_id = book_id;
        bookmark.position = sqlite3_column_int(stmt, 1);
        const unsigned char* name = sqlite3_column_text(stmt, 2);
        bookmark.name = name ? std::string(reinterpret_cast<const char*>(name)) : "";
        bookmark.created_at = sqlite3_column_int(stmt, 3);
        bookmarks.push_back(bookmark);
    }

    sqlite3_finalize(stmt);
    return bookmarks;
}

bool DatabaseManager::deleteBookmark(int bookmark_id) {
    const char* sql = "DELETE FROM bookmarks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, bookmark_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<Book> DatabaseManager::getHistory() {
    std::vector<Book> history;
    const char* sql = "SELECT id, filepath, last_position, duration, is_finished, last_played_at FROM books ORDER BY last_played_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return history;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Book book;
        book.id = sqlite3_column_int(stmt, 0);
        const unsigned char* filepath = sqlite3_column_text(stmt, 1);
        book.filepath = filepath ? std::string(reinterpret_cast<const char*>(filepath)) : "";
        book.last_position = sqlite3_column_int(stmt, 2);
        book.duration = sqlite3_column_int(stmt, 3);
        book.is_finished = sqlite3_column_int(stmt, 4) != 0;
        book.last_played_at = sqlite3_column_int(stmt, 5);
        history.push_back(book);
    }

    sqlite3_finalize(stmt);
    return history;
}

bool DatabaseManager::migrateFromLegacy(const std::string& legacy_path) {
    std::ifstream in(legacy_path.c_str());
    if (!in.is_open()) return false;

    std::string line;
    if (std::getline(in, line)) {
        if (line != "NONE" && !line.empty()) {
            setSetting("last_opened_file", line);
        }
    }

    while (std::getline(in, line)) {
        std::size_t delimiter = line.find('|');
        if (delimiter == std::string::npos) continue;

        std::string filepath = line.substr(0, delimiter);
        std::string pos_text = line.substr(delimiter + 1);
        std::istringstream iss(pos_text);
        int position = 0;
        if (iss >> position) {
            updateBookProgress(filepath, position, 0, false);
        }
    }

    in.close();

    std::string backup_path = legacy_path + ".bak";
    std::rename(legacy_path.c_str(), backup_path.c_str());
    return true;
}
