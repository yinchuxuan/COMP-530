
#ifndef INDEX_UTIL_H
#define INDEX_UTIL_H

#include <string>
#include "MyDB_Table.h"

using namespace std;

typedef string MyDB_PageKey;

// Generate page key by table name and page index
// This page key is used for hashing
inline MyDB_PageKey generatePageKey(string tableName, long index) {
    return tableName + to_string(index);
}

#endif