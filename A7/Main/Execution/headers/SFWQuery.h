
#ifndef SFWQUERY_H
#define SFWQUERY_H

#include "ExprTree.h"
#include "MyDB_LogicalOps.h"

// structure that stores an entire SFW query
struct SFWQuery {

private:

	// the various parts of the SQL query
	vector <ExprTreePtr> valuesToSelect;
	vector <pair <string, string>> tablesToProcess;
	vector <ExprTreePtr> allDisjunctions;
	vector <ExprTreePtr> groupingClauses;
	int joinTableCount;

	bool areAggs();

	void sortTablesBySize(map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters);

	LogicalOpPtr buildSingleScanOp(pair<string, string> tableToProcess, map <string, MyDB_TablePtr> &allTables, map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters, MyDB_SchemaPtr selectSchema, vector<ExprTreePtr>& remainingDisjunctions);

	LogicalOpPtr buildSingleJoinOp(LogicalOpPtr joinLHS, LogicalOpPtr joinRHS, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema, vector<pair<string, string>> tablesHaveProcessed, MyDB_SchemaPtr opSchema, vector<ExprTreePtr>& remainingDisjunctions);

	LogicalOpPtr buildScanOp(pair<string, string> table, map<string, MyDB_TablePtr>& allTables, map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters, vector<ExprTreePtr> exprsToCompute, vector<ExprTreePtr> CNFSet, MyDB_SchemaPtr opSchema);

	LogicalOpPtr buildJoinOp(LogicalOpPtr leftOp, LogicalOpPtr rightOp, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema, vector<ExprTreePtr> exprsToCompute, vector<ExprTreePtr> topCNF, MyDB_SchemaPtr opSchema);

	LogicalOpPtr buildLeftDeepJoinOpTree(map <string, MyDB_TablePtr> &allTables, map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters, MyDB_SchemaPtr selectSchema);

	LogicalOpPtr buildJoinFromTwoTableSet(vector<pair<string, string>> leftTables, vector<pair<string, string>> rightTables, map<string, MyDB_TablePtr>& allTables, map<string, MyDB_TableReaderWriterPtr> allTableReaderWriters,
 	vector<pair<string, string>> allTableSet, vector<ExprTreePtr> CNFSet, vector<ExprTreePtr> exprsToCompute, MyDB_SchemaPtr opSchema);

	LogicalOpPtr buildOptimizedOpTree(map<string, MyDB_TablePtr>& allTables, map<string, MyDB_TableReaderWriterPtr>& allTableReaderWriters, vector<ExprTreePtr> CNFSet, vector<pair<string, string>> tableSet, vector<ExprTreePtr> exprsToCompute, MyDB_SchemaPtr opSchema);

public:
	SFWQuery () {}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause, 
		struct CNF *cnf, struct ValueList *grouping);

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause, 
		struct CNF *cnf);

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause);
	
	// builds and optimizes a logical query plan for a SFW query, returning the resulting logical query plan
	//
	// allTables: this is the list of all of the tables currently in the system
	// allTableReaderWriters: this is so we can store the info that we need to be able to execute the query
	LogicalOpPtr buildLogicalQueryPlan (map <string, MyDB_TablePtr> &allTables, 
		map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters);

	~SFWQuery () {}

	void print ();

	#include "FriendDecls.h"
};

#endif
