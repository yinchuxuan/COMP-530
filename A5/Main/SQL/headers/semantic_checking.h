#ifndef _SEMANTIC_CHECKING_H
#define _SEMANTIC_CHECKING_H

#include "ParserTypes.h"
#include "MyDB_Catalog.h"

int check_SQLStatement(struct SQLStatement* statement, MyDB_CatalogPtr catalog); 

#endif