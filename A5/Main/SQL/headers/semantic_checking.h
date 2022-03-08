#ifndef _SEMANTIC_CHECKING_H
#define _SEMANTIC_CHECKING_H

#include "ParserTypes.h"
#include "MyDB_Catalog.h"
#include <vector>
#include <string>
#include <unordered_map>

int check_SQLStatement(struct SQLStatement* statement, MyDB_CatalogPtr catalog); 

int check_CreateTable(struct CreateTable* create_table);

int check_SFWQuery(struct SFWQuery* query, MyDB_CatalogPtr catalog);

int check_tables(vector<pair<string, string>>& tables, MyDB_CatalogPtr catalog);

int check_selectedValues(vector<ExprTreePtr>& selectedValues, unordered_map<string, MyDB_TablePtr>& alias_map);

int check_disjunctions(vector<ExprTreePtr>& expressions, unordered_map<string, MyDB_TablePtr>& alias_map);

int check_groupValues(vector<ExprTreePtr>& selectedValues, vector<ExprTreePtr>& groupings, unordered_map<string, MyDB_TablePtr>& alias_map);

#endif