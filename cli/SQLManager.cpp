#include "SQLManager.hpp"

static std::unordered_map<std::string, std::string> sql_datacallback;

static int sqlManagerCallback(void* NotUsed, int argc, char** argv, char** azColName)
{
    for (int i = 0; i < argc; i++) {
        sql_datacallback.try_emplace(azColName[i], argv[i]);
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}