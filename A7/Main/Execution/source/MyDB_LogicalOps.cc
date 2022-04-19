
#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"

vector<string> exprsToStrings(vector<ExprTreePtr>& expressions) {
	vector<string> result;
	for (auto expression : expressions) {
		result.push_back(expression->toString());
	}
	return result;
}

vector<pair<MyDB_AggType, string>> exprsToAggs(vector<ExprTreePtr>& expressions) {
	vector<pair<MyDB_AggType, string>> result;
	for (auto expression : expressions) {
		if (expression->isAvg()) {
			result.push_back(pair<MyDB_AggType, string>(avg, expression->toString()));
		}
		if (expression->isSum()) {
			result.push_back(pair<MyDB_AggType, string>(cnt, expression->toString()));
		}
	}
	return result;
}

vector<string> projectionsFromTable(MyDB_TablePtr table) {
	vector<string> result;
	for (auto atts : table->getSchema()->getAtts()) {
		result.push_back(atts.first);
	}
	return result;
}

MyDB_TableReaderWriterPtr makeAggTable(vector<pair<MyDB_AggType, string>> aggsToCompute, vector<string>& groupings) {

}

vector<pair<string, string>> makeEqualityChecks(vector<ExprTreePtr>& expressions) {
	vector<pair<string, string>> result;
	for (auto expression : expressions) {
		if (expression->isEq()) {
			result.push_back(pair<string, string>(expression->getLHS()->toString(), expression->getRHS()->toString()));
		}
	}
	return result;
}

string makeSelectionPredicate(vector<ExprTreePtr>& expressions) {
	string result = "";
	if (expressions.size() > 0) {
		result = expressions[0]->toString();

		for (int i = 1; i < expressions.size(); i++) {
			result = "&& ( " + result + ", " + expressions[i].toString() + ")";
		}
	}
	
	return result;
}

bool isTableFitInBuffer(MyDB_TableReaderWriterPtr table) {
	MyDB_BufferManagerPtr bufferManager = table->getBufferMgr();
	return table->getNumPages() < bufferManager->getPageSize() / 2;
}

void getSmallAndLargeTable(MyDB_TableReaderWriterPtr leftTable, MyDB_TableReaderWriterPtr rightTable, MyDB_TableReaderWriterPtr& smallTable, MyDB_TableReaderWriterPtr& largeTable) {
	if (leftTable->getNumPages() < rightTable->getNumPages()) {
		smallTable = leftTable;
		largeTable = rightTable;
	} else {
		smallTable = rightTable;
		largeTable = leftTable;
	}
}

void killTable(MyDB_TableReaderWriterPtr table) {
	MyDB_BufferManagerPtr bufferManager = table->getBufferMgr();
	bufferManager->killTable(table->getTable());
}

// fill this out!  This should actually run the aggregation via an appropriate RelOp, and then it is going to
// have to unscramble the output attributes and compute exprsToCompute using an execution of the RegularSelection 
// operation (why?  Note that the aggregate always outputs all of the grouping atts followed by the agg atts.
// After, a selection is required to compute the final set of aggregate expressions)
//
// Note that after the left and right hand sides have been executed, the temporary tables associated with the two 
// sides should be deleted (via a kill to killFile () on the buffer manager)
MyDB_TableReaderWriterPtr LogicalAggregate :: execute () {
	MyDB_TableReaderWriterPtr inputTable = inputOp->execute();
	MyDB_TableReaderWriterPtr outputTable = make_shared<MyDB_TableReaderWriter>(outputSpec, inputTable->getBufferMgr());
	vector<string> groupingStrings = exprsToStrings(groupings);
	vector<pair<MyDB_AggType, string>> aggsToCompute = exprsToAggs(exprsToCompute);
	vector<string> projections = projectionsFromTable(outputSpec); 
	MyDB_TableReaderWriterPtr aggOutputTable;

	Aggregate aggregateOp(inputTable, aggOutputTable, aggsToCompute, groupings, "bool[true]");
	aggregateOp.run();
	RegularSelection regularSelectionOp(aggOutputTable, outputTable, "bool[ture]", projections);
	regularSelectionOp.run();

	killTable(inputTable);
	killTable(aggOutputTable);
	return outputTable;
}
// we don't really count the cost of the aggregate, so cost its subplan and return that
pair <double, MyDB_StatsPtr> LogicalAggregate :: cost () {
	return inputOp->cost ();
}
	
// this costs the entire query plan with the join at the top, returning the compute set of statistics for
// the output.  Note that it recursively costs the left and then the right, before using the statistics from
// the left and the right to cost the join itself
pair <double, MyDB_StatsPtr> LogicalJoin :: cost () {
	auto left = leftInputOp->cost ();
	auto right = rightInputOp->cost ();
	MyDB_StatsPtr outputStats = left.second->costJoin (outputSelectionPredicate, right.second);
	return make_pair (left.first + right.first + outputStats->getTupleCount (), outputStats);
}
	
// Fill this out!  This should recursively execute the left hand side, and then the right hand side, and then
// it should heuristically choose whether to do a scan join or a sort-merge join (if it chooses a scan join, it
// should use a heuristic to choose which input is to be hashed and which is to be scanned), and execute the join.
// Note that after the left and right hand sides have been executed, the temporary tables associated with the two 
// sides should be deleted (via a kill to killFile () on the buffer manager)
MyDB_TableReaderWriterPtr LogicalJoin :: execute () {
	MyDB_TableReaderWriterPtr leftTable = leftInputOp->execute();
	MyDB_TableReaderWriterPtr rightTable = rightInputOp->execute();
	MyDB_TableReaderWriterPtr outputTable = make_shared<MyDB_TableReaderWriter>(outputSpec, leftTable->getBufferMgr());
	vector<string> projections = exprsToStrings(exprsToCompute);
	vector<pair<string, string>> equalityChecks = makeEqualityChecks(outputSelectionPredicate);
	string finalSelectionPredicate = makeSelectionPredicate(outputSelectionPredicate);
	MyDB_TableReaderWriterPtr smallTable;
	MyDB_TableReaderWriterPtr largeTable;

	getSmallAndLargeTable(leftTable, rightTable, smallTable, largeTable);
	if (isTableFitInBuffer(smallTable)) {		// run scan join
		ScanJoin scanJoinOp(smallTable, largeTable, outputTable, finalSelectionPredicate, projections, equalityChecks, "bool[true]", "bool[true]");
		scanJoinOp.run();
	} else {		// run sort merge join
		SortMergeJoin sortMergeJoinOp(smallTable, largeTable, outputTable, finalSelectionPredicate, projections, equalityChecks[0], "bool[true]", "bool[true]");
		sortMergeJoinOp.run();
	}

	killTable(leftTable);
	killTable(rightTable);
	return outputTable;
}

// this costs the table scan returning the compute set of statistics for the output
pair <double, MyDB_StatsPtr> LogicalTableScan :: cost () {
	MyDB_StatsPtr returnVal = inputStats->costSelection (selectionPred);
	return make_pair (returnVal->getTupleCount (), returnVal);	
}

// fill this out!  This should heuristically choose whether to use a B+-Tree (if appropriate) or just a regular
// table scan, and then execute the table scan using a relational selection.  Note that a desirable optimization
// is to somehow set things up so that if a B+-Tree is NOT used, that the table scan does not actually do anything,
// and the selection predicate is handled at the level of the parent (by filtering, for example, the data that is
// input into a join)
MyDB_TableReaderWriterPtr LogicalTableScan :: execute () {

	return nullptr;
}

#endif
