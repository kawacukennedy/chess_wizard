#include "sqlite_manager.h"
#include <iostream>

SQLiteManager::SQLiteManager(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database\n";
    }
    // Create tables
    const char* create_games = "CREATE TABLE IF NOT EXISTS games (id INTEGER PRIMARY KEY, fen_sequence TEXT, result TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    const char* create_settings = "CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT);";
    sqlite3_exec(db, create_games, nullptr, nullptr, nullptr);
    sqlite3_exec(db, create_settings, nullptr, nullptr, nullptr);
}

SQLiteManager::~SQLiteManager() {
    sqlite3_close(db);
}

void SQLiteManager::saveGame(const std::string& fen_sequence, const std::string& result) {
    std::string sql = "INSERT INTO games (fen_sequence, result) VALUES ('" + fen_sequence + "', '" + result + "');";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

std::vector<std::string> SQLiteManager::loadGames() {
    std::vector<std::string> games;
    // Placeholder
    return games;
}

void SQLiteManager::saveSetting(const std::string& key, const std::string& value) {
    std::string sql = "INSERT OR REPLACE INTO settings (key, value) VALUES ('" + key + "', '" + value + "');";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

std::string SQLiteManager::loadSetting(const std::string& key) {
    // Placeholder
    return "";
}