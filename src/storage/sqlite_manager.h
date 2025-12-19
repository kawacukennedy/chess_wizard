#ifndef SQLITE_MANAGER_H
#define SQLITE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>

class SQLiteManager {
public:
    SQLiteManager(const std::string& db_path);
    ~SQLiteManager();

    void saveGame(const std::string& fen_sequence, const std::string& result);
    std::vector<std::string> loadGames();
    void saveSetting(const std::string& key, const std::string& value);
    std::string loadSetting(const std::string& key);

private:
    sqlite3* db;
};

#endif