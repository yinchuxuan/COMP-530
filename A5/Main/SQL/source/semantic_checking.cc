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
    if (check_groupValues(query->getDisjunctions(), alias_map)) {
        return 1;
    }

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

int check_groupValues(vector<ExprTreePtr>& selectedValues, vector<ExprTreePtr>& groupings, unordered_map<string, MyDB_TablePtr>& alias_map){
    if (groupings.size() == 0) {
        return 0;
    }
    for (auto expression: selectedValues) {
        // if expression is an aggregation function, skip
        bool found = false;
        if (typeid(expression) == typeId(SumOp) || typeid(expression) == typeid(AvgOp)) {
            continue;
        }
        expression.toString();
        for (auto group: groupings) {
            // check if this attribute is in groupings
            // if group.attribute = expression.attribute, found = true
            if (expression.toString().compare(group.toString())) {
                found = true;
                break;
            }
        }
        if (!found) {
            return 1;
        } else {
            found = false;
        }
    }
    return 0;
}

#endif