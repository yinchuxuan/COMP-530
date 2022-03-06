#ifndef _SEMANTIC_CHECKING_C
#define _SEMANTIC_CHECKING_C

#include "semantic_checking.h"
#include "ParserTypes.h"
#include "MyDB_Catalog.h"
#include <algorithm>
#include <unordered_map>

int check_SQLStatement(struct SQLStatement* statement, MyDB_CatalogPtr catalog) {
    if (statement->isCreateTable()) {
        return check_CreateTable(statement->getCreateTable());
    } else {
        return check_SFWQuery(statement->getQuery(), catalog);
    }
}

int check_CreateTable(struct CreateTable* create_table) {
    // There is nothing to check in CreateTable?
    return 0;
}

int check_SFWQuery(struct SFWQuery* query, MyDB_CatalogPtr catalog) {
    vector<pair<string, string>> tables = query->getTables();
    unordered_map<string, MyDB_TablePtr> alias_map;

    if (check_tables(tables, catalog)) {
        return 1;
    }

    for (auto table : tables) {
        if (alias_map.count(table.second)) {
            cout << "Parse Error: duplicate alias " + table.second + "!" << endl;
            return 1;
        }

		MyDB_TablePtr temp = make_shared <MyDB_Table> ();
		temp->fromCatalog (table.first, catalog);	
        alias_map[table.second] = temp; 
    }

    if (check_selectedValues(query->getSelectedValues(), alias_map)) {
        return 1;
    }

    if (check_disjunctions(query->getDisjunctions(), alias_map)) {
        return 1;
    }

    // Check groupings


    return 0;
}

int check_tables(vector<pair<string, string>>& tables, MyDB_CatalogPtr catalog) {
    static map <string, MyDB_TablePtr> allTables = MyDB_Table :: getAllTables (catalog);

    for (auto table: tables) {
        if (!allTables.count(table.first)) {
            cout << "Parse Error: Table " + table.first + " doesn't exist!" << endl;
            return 1;
        }
    }

    return 0;
}

int check_selectedValues(vector<ExprTreePtr>& selectedValues, unordered_map<string, MyDB_TablePtr>& alias_map) {
    // Should contain at least one identifier?
    for (auto expression : selectedValues) {
        expression->check(alias_map);

        if (expression->getType() == BOOL) {
            cout << "Parse Error: wrong type for selected value of " + expression->toString() << endl;
            return 1;
        }
    }

    return 0;
}

int check_disjunctions(vector<ExprTreePtr>& expressions, unordered_map<string, MyDB_TablePtr>& alias_map) {
    for (auto expression : expressions) {
        expression->check(alias_map);

        if (expression->getType() != BOOL) {
            cout << "Parse Error: wrong type for disjunction term " + expression->toString() + "!" << endl;
            return 1;
        }
    }

    return 0;
}

#endif