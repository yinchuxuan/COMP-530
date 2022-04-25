
#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "ParserTypes.h"
#include <algorithm>

bool isTableInTableSet(string tableName, vector<pair<string, string>>& tableSet) {
	for (auto table : tableSet) {
		if (tableName == table.first) {
			return true;
		}
	}
	return false;
}

bool isDisjunctionOnlyRefTableSet(ExprTreePtr disjunction, vector<pair<string, string>>& tableSet, vector<pair<string, string>>& tablesToProcess) {
	bool result = true;
	bool isReferenced = false;
	for (int i = 0; i < tablesToProcess.size(); i++) {
		if (isTableInTableSet(tablesToProcess[i].first, tableSet)) {
			isReferenced = isReferenced || disjunction->referencesTable(tablesToProcess[i].second);
		} else {
			result = result && !disjunction->referencesTable(tablesToProcess[i].second);
		}
	}
	return result && isReferenced;
}

vector<ExprTreePtr> getCNFOfTableSet(vector<ExprTreePtr>& remaingDisjunctions, vector<pair<string, string>>& tableSet, vector<pair<string, string>>& tablesToProcess) {
	vector<ExprTreePtr> result;
	vector<ExprTreePtr> rest;
	for (auto disjunction : remaingDisjunctions) {
		if (isDisjunctionOnlyRefTableSet(disjunction, tableSet, tablesToProcess)) {
			result.push_back(disjunction);
		} else {
			rest.push_back(disjunction);
		}
	}
	remaingDisjunctions = rest;
	return result;
}

bool isAttInValuesToSelect(pair<string, MyDB_AttTypePtr> att, vector<ExprTreePtr>& valuesToSelect, string tableAlias) {
	for (auto value : valuesToSelect) {
		if (value->referencesAtt(tableAlias, att.first)) {
			return true;
		}
	}
	return false;
}

bool isExprInSet(ExprTreePtr expression, vector<ExprTreePtr>& set) {
	for (auto expr : set) {
		if (expression->toString() == expr->toString()) {
			return true;
		}
	}
	return false;
}

bool isAttNeededInCNF(pair<string, MyDB_AttTypePtr> att, vector<ExprTreePtr>& remainingDisjunctions, string tableAlias) {
	for (auto disjunction : remainingDisjunctions) {
		if (disjunction->referencesAtt(tableAlias, att.first)) {
			return true;
		}
	}
	return false;
}

void renameAtt(pair<string, MyDB_AttTypePtr>& att) {
	string attName = att.first;
	int i = 0;
	while (attName[i] != '_') {
		i++;
	}
	att.first = attName.substr(i + 1, attName.size() - i);
}

vector<string> getExprsToCompute(vector<ExprTreePtr>& valuesToSelect, vector<ExprTreePtr>& remainingDisjunctions, MyDB_TablePtr selfTable, string tableAlias) {
	vector<string> result;
	for (auto att : selfTable->getSchema()->getAtts()) {
		if (isAttInValuesToSelect(att, valuesToSelect, tableAlias) || isAttNeededInCNF(att, remainingDisjunctions, tableAlias)) {
			result.push_back("[" + att.first + "]");
		}
	}
	return result;
}

vector<ExprTreePtr> getJoinExprsToCompute(vector<ExprTreePtr>& valuesToSelect, vector<ExprTreePtr>& remainingDisjunctions, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema, vector<pair<string, string>> tableSet) {
	vector<ExprTreePtr> result;
	for (auto att : leftSchema->getAtts()) {
		renameAtt(att);
		for (auto table : tableSet) {		// try all tables in table set to find table alias that matches att 
			if (isAttInValuesToSelect(att, valuesToSelect, table.second) || isAttNeededInCNF(att, remainingDisjunctions, table.second)) {
				result.push_back(make_shared<Identifier>(table.second, att.first));
			}
		}
	}
	for (auto att : rightSchema->getAtts()) {
		renameAtt(att);
		for (auto table : tableSet) {
			if (isAttInValuesToSelect(att, valuesToSelect, table.second) || isAttNeededInCNF(att, remainingDisjunctions, table.second)) {
				result.push_back(make_shared<Identifier>(table.second, att.first));
			}
		}
	}
	return result;
}

MyDB_SchemaPtr mergeSchema(MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema) {
	MyDB_SchemaPtr result = make_shared<MyDB_Schema>();
	for (auto att : leftSchema->getAtts()) {
		result->getAtts().push_back(att);
	}
	for (auto att : rightSchema->getAtts()) {
		result->getAtts().push_back(att);
	}
	return result;
}

MyDB_SchemaPtr getSchemaFromExprs(vector<string>& exprsToCompute, MyDB_SchemaPtr inputSchema) {
	MyDB_SchemaPtr result = make_shared<MyDB_Schema>();
	MyDB_Record myRec(inputSchema);
	for (auto exprToCompute : exprsToCompute) {
		result->getAtts().push_back(make_pair(exprToCompute.substr(1, exprToCompute.size() - 2), myRec.getType(exprToCompute)));	
	}
	return result;
}

MyDB_SchemaPtr makeScanSchema(vector<string>& exprsToCompute, MyDB_SchemaPtr inputSchema, string tableAlias) {
	MyDB_SchemaPtr result = make_shared<MyDB_Schema>();
	MyDB_Record myRec(inputSchema);
	for (auto exprToCompute : exprsToCompute) {
		result->getAtts().push_back(make_pair(tableAlias + "_" + exprToCompute.substr(1, exprToCompute.size() - 2), myRec.getType(exprToCompute)));	
	}
	return result;
}

MyDB_SchemaPtr makeJoinSchema(vector<ExprTreePtr>& exprsToCompute, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema) {
	MyDB_SchemaPtr result = make_shared<MyDB_Schema>();
	MyDB_SchemaPtr inputSchema = mergeSchema(leftSchema, rightSchema);
	MyDB_Record myRec(inputSchema);
	for (auto exprToCompute : exprsToCompute) {
		result->getAtts().push_back(make_pair(exprToCompute->toString().substr(1, exprToCompute->toString().size() - 2), myRec.getType(exprToCompute->toString())));	
	}

	for (auto exprToCompute : exprsToCompute) {
		cout << exprToCompute->toString() << endl;
	}
	return result;
}

MyDB_SchemaPtr makeOutputSchema(MyDB_SchemaPtr opSchema, vector<ExprTreePtr>& valuesToSelect) {
	MyDB_SchemaPtr outputSchema = make_shared<MyDB_Schema>();
	MyDB_Record myRec(opSchema);

	int i = 0;
	for (auto a : valuesToSelect) {
		outputSchema->getAtts ().push_back (make_pair ("att_" + to_string (i++), myRec.getType (a->toString ())));
	}
	return outputSchema;
}

void copySchema(MyDB_SchemaPtr src, MyDB_SchemaPtr dest) {
	for (auto att : src->getAtts()) {
		dest->getAtts().push_back(att);
	}
}

bool SFWQuery :: areAggs() {
	bool result = false;
	for (auto a : valuesToSelect) {
		if (a->hasAgg ()) {
			result = true;
		}
	}
	return result;
}

void SFWQuery :: sortTablesBySize(map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters) {
	auto compareFunc = [&](const pair<string, string> table1, const pair<string, string> table2) {
		return allTableReaderWriters[table1.first]->getNumPages() < allTableReaderWriters[table2.first]->getNumPages();
	};	
	sort(tablesToProcess.begin(), tablesToProcess.end(), compareFunc);
}

LogicalOpPtr SFWQuery :: buildSingleScanOp(pair<string, string> tableToProcess, map<string, MyDB_TablePtr>& allTables, map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters, MyDB_SchemaPtr opSchema, vector<ExprTreePtr>& remainingDisjunctions) {
	MyDB_TableReaderWriterPtr inputTable = allTableReaderWriters[tableToProcess.first];
	vector<pair<string, string>> singleTableSet = vector<pair<string, string>>{ tableToProcess };
	vector<ExprTreePtr> CNF = getCNFOfTableSet(remainingDisjunctions, singleTableSet, tablesToProcess);
	for (auto e : CNF) {
		cout << "CNF:" << e->toRenamingString() << endl;
	}
	vector<string> exprsToCompute = getExprsToCompute(valuesToSelect, remainingDisjunctions, inputTable->getTable(), tableToProcess.second);
	for (auto e : exprsToCompute) {
		cout << "exprToCompute:" << e << endl;
	}
	MyDB_SchemaPtr outputSchema = makeScanSchema(exprsToCompute, inputTable->getTable()->getSchema(), tableToProcess.second);
	cout << outputSchema << endl;
	copySchema(outputSchema, opSchema);

	return make_shared <LogicalTableScan> (inputTable, make_shared <MyDB_Table> ("selectTable" + tableToProcess.second, "selectStorageLoc" + tableToProcess.second, outputSchema), 
		   make_shared <MyDB_Stats> (allTables[tableToProcess.first], tableToProcess.second), CNF, exprsToCompute);
}

LogicalOpPtr SFWQuery :: buildSingleJoinOp(LogicalOpPtr joinLHS, LogicalOpPtr joinRHS, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema, vector<pair<string, string>> tablesHaveProcessed, MyDB_SchemaPtr opSchema, vector<ExprTreePtr>& remainingDisjunctions) {
	vector<ExprTreePtr> CNF = getCNFOfTableSet(remainingDisjunctions, tablesHaveProcessed, tablesToProcess);
	vector<ExprTreePtr> exprsToCompute = getJoinExprsToCompute(valuesToSelect, remainingDisjunctions, leftSchema, rightSchema, tablesHaveProcessed);
	MyDB_SchemaPtr outputSchema = makeJoinSchema(exprsToCompute, leftSchema, rightSchema);
	copySchema(outputSchema, opSchema);

	return make_shared <LogicalJoin> (joinLHS, joinRHS, make_shared <MyDB_Table> ("joinTable" + to_string(tablesHaveProcessed.size()),
	"joinStorageLoc" + to_string(tablesHaveProcessed.size()), outputSchema), CNF, exprsToCompute);
}

LogicalOpPtr SFWQuery :: buildLeftDeepJoinOpTree(map<string, MyDB_TablePtr>& allTables, map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters, MyDB_SchemaPtr opSchema) {
	vector<pair<string, string>> tablesHaveProcessed;
	vector<ExprTreePtr> remainingDisjunctions = allDisjunctions;
	MyDB_SchemaPtr leftSchema = make_shared<MyDB_Schema>();
	MyDB_SchemaPtr rightSchema;
	MyDB_SchemaPtr joinSchema;
	LogicalOpPtr joinLHS = buildSingleScanOp(tablesToProcess[0], allTables, allTableReaderWriters, leftSchema, remainingDisjunctions);
	LogicalOpPtr joinRHS;
	tablesHaveProcessed.push_back(tablesToProcess[0]);

	for (int i = 1; i < tablesToProcess.size(); i++) {
		joinSchema = make_shared<MyDB_Schema>();
		rightSchema = make_shared<MyDB_Schema>();
		joinRHS = buildSingleScanOp(tablesToProcess[i], allTables, allTableReaderWriters, rightSchema, remainingDisjunctions);
		tablesHaveProcessed.push_back(tablesToProcess[i]);
		joinLHS = buildSingleJoinOp(joinLHS, joinRHS, leftSchema, rightSchema, tablesHaveProcessed, joinSchema, remainingDisjunctions);
		leftSchema = joinSchema;
	}

	copySchema(joinSchema, opSchema);
	return joinLHS;
}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
// 
// note that this implementation only works for two-table queries that do not have an aggregation
// 
LogicalOpPtr SFWQuery :: buildLogicalQueryPlan (map <string, MyDB_TablePtr> &allTables, map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters) {
	LogicalOpPtr finalProjectionOp;
	LogicalOpPtr selectOp;
	MyDB_SchemaPtr opSchema = make_shared<MyDB_Schema>();
	MyDB_SchemaPtr outputSchema;

	sortTablesBySize(allTableReaderWriters);
	if (tablesToProcess.size() == 1) {	// single table, just build a scan op
		vector<ExprTreePtr> remainingDisjunctions = allDisjunctions;
		selectOp = buildSingleScanOp(tablesToProcess[0], allTables, allTableReaderWriters, opSchema, remainingDisjunctions);
	} else {
		selectOp = buildLeftDeepJoinOpTree(allTables, allTableReaderWriters, opSchema);
	}

	outputSchema = makeOutputSchema(opSchema, valuesToSelect);

	if (areAggs()) {
		finalProjectionOp = make_shared<LogicalAggregate>(selectOp, make_shared <MyDB_Table> ("outputTable", "outputStorageLoc", outputSchema),
			valuesToSelect, groupingClauses);
	} else {
		finalProjectionOp = make_shared <LogicalTableProjection> (selectOp, 
			make_shared <MyDB_Table> ("outputTable", "outputStorageLoc", outputSchema), valuesToSelect);
	}

	return finalProjectionOp;
/*
	// first, make sure we have exactly two tables... this prototype only works on two tables!!
	if (tablesToProcess.size () != 2) {
		cout << "Sorry, this currently only works for two-table queries!\n";
		return nullptr;
	}

	// also, make sure that there are no aggregates in here
	bool areAggs = false;
	for (auto a : valuesToSelect) {
		if (a->hasAgg ()) {
			areAggs = true;
		}
	}
	if (groupingClauses.size () != 0 || areAggs) {
		cout << "Sorry, we can't handle aggs or groupings yet!\n";
		return nullptr;
	}	

	// find the two input tables
	MyDB_TablePtr leftTable = allTables[tablesToProcess[0].first];
	MyDB_TablePtr rightTable = allTables[tablesToProcess[1].first];
	
	// find the various parts of the CNF
	vector <ExprTreePtr> leftCNF; 
	vector <ExprTreePtr> rightCNF; 
	vector <ExprTreePtr> topCNF; 

	// loop through all of the disjunctions and break them apart
	for (auto a: allDisjunctions) {
		bool inLeft = a->referencesTable (tablesToProcess[0].second);
		bool inRight = a->referencesTable (tablesToProcess[1].second);
		if (inLeft && inRight) {
			cout << "top " << a->toString () << "\n";
			topCNF.push_back (a);
		} else if (inLeft) {
			cout << "left: " << a->toString () << "\n";
			leftCNF.push_back (a);
		} else {
			cout << "right: " << a->toString () << "\n";
			rightCNF.push_back (a);
		}
	}

	// now get the left and right schemas for the two selections
	MyDB_SchemaPtr leftSchema = make_shared <MyDB_Schema> ();
	MyDB_SchemaPtr rightSchema = make_shared <MyDB_Schema> ();
	MyDB_SchemaPtr totSchema = make_shared <MyDB_Schema> ();
	vector <string> leftExprs;
	vector <string> rightExprs;
		
	// and see what we need from the left, and from the right
	for (auto b: leftTable->getSchema ()->getAtts ()) {
		bool needIt = false;
		for (auto a: valuesToSelect) {
			if (a->referencesAtt (tablesToProcess[0].second, b.first)) {
				needIt = true;
			}
		}
		for (auto a: topCNF) {
			if (a->referencesAtt (tablesToProcess[0].second, b.first)) {
				needIt = true;
			}
		}
		if (needIt) {
			leftSchema->getAtts ().push_back (make_pair (b.first, b.second));
			totSchema->getAtts ().push_back (make_pair (tablesToProcess[0].second + "_" + b.first, b.second));
			leftExprs.push_back ("[" + b.first + "]");
			cout << "left expr: " << ("[" + b.first + "]") << "\n";
		}
	}

	cout << "left schema: " << leftSchema << "\n";

	// and see what we need from the right, and from the right
	for (auto b: rightTable->getSchema ()->getAtts ()) {
		bool needIt = false;
		for (auto a: valuesToSelect) {
			if (a->referencesAtt (tablesToProcess[1].second, b.first)) {
				needIt = true;
			}
		}
		for (auto a: topCNF) {
			if (a->referencesAtt (tablesToProcess[1].second, b.first)) {
				needIt = true;
			}
		}
		if (needIt) {
			rightSchema->getAtts ().push_back (make_pair (b.first, b.second));
			totSchema->getAtts ().push_back (make_pair (tablesToProcess[1].second + "_" + b.first, b.second));
			rightExprs.push_back ("[" + b.first + "]");
			cout << "right expr: " << ("[" + b.first + "]") << "\n";
		}
	}
	cout << "right schema: " << rightSchema << "\n";
		
	// now we gotta figure out the top schema... get a record for the top
	MyDB_Record myRec (totSchema);
	
	// and get all of the attributes for the output
	MyDB_SchemaPtr topSchema = make_shared <MyDB_Schema> ();
	int i = 0;
	for (auto a: valuesToSelect) {
		topSchema->getAtts ().push_back (make_pair ("att_" + to_string (i++), myRec.getType (a->toString ())));
	}
	cout << "top schema: " << topSchema << "\n";

	// and it's time to build the query plan
	LogicalOpPtr leftTableScan = make_shared <LogicalTableScan> (allTableReaderWriters[tablesToProcess[0].first], 
		make_shared <MyDB_Table> ("leftTable", "leftStorageLoc", leftSchema), 
		make_shared <MyDB_Stats> (leftTable, tablesToProcess[0].second), leftCNF, leftExprs);
	LogicalOpPtr rightTableScan = make_shared <LogicalTableScan> (allTableReaderWriters[tablesToProcess[1].first], 
		make_shared <MyDB_Table> ("rightTable", "rightStorageLoc", rightSchema), 
		make_shared <MyDB_Stats> (rightTable, tablesToProcess[1].second), rightCNF, rightExprs);
	LogicalOpPtr returnVal = make_shared <LogicalJoin> (leftTableScan, rightTableScan, 
		make_shared <MyDB_Table> ("topTable", "topStorageLoc", topSchema), topCNF, valuesToSelect);

	// done!!
	return returnVal;
*/
}

void SFWQuery :: print () {
	cout << "Selecting the following:\n";
	for (auto a : valuesToSelect) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "From the following:\n";
	for (auto a : tablesToProcess) {
		cout << "\t" << a.first << " AS " << a.second << "\n";
	}
	cout << "Where the following are true:\n";
	for (auto a : allDisjunctions) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "Group using:\n";
	for (auto a : groupingClauses) {
		cout << "\t" << a->toString () << "\n";
	}
}


SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf, struct ValueList *grouping) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions = cnf->disjunctions;
        groupingClauses = grouping->valuesToCompute;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
	allDisjunctions = cnf->disjunctions;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions.push_back (make_shared <BoolLiteral> (true));
}

#endif
