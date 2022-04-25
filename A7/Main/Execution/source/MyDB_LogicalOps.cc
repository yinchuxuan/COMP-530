
#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"
#include "Aggregate.h"
#include "RegularSelection.h"
#include "ScanJoin.h"
#include "BPlusSelection.h"
#include "SortMergeJoin.h"
#include "ExprTree.h"

vector<string> exprsToStrings(vector<ExprTreePtr>& expressions, bool isRenaming) {
	vector<string> result;
	for (auto expression : expressions) {
		if (isRenaming) {
			result.push_back(expression->toRenamingString());
		} else {
			result.push_back(expression->toString());
		}
	}
	return result;
}

void getAggsFromExpr(ExprTreePtr expression, vector<pair<MyDB_AggType, string>>& result) {
	if (expression->isAvg()) {
		result.push_back(pair<MyDB_AggType, string>(avg, expression->getChild()->toString()));
	} else if (expression->isSum()) {
		result.push_back(pair<MyDB_AggType, string>(sum, expression->getChild()->toString()));
	} else {
		if (expression->getChild()) {
			getAggsFromExpr(expression->getChild(), result);
		} 
		if (expression->getLHS()) {
			getAggsFromExpr(expression->getLHS(), result);
		} 
		if (expression->getRHS()) {
			getAggsFromExpr(expression->getRHS(), result);
		}
	}
}

vector<pair<MyDB_AggType, string>> getAggsFromExprs(vector<ExprTreePtr>& expressions) {
	vector<pair<MyDB_AggType, string>> result;
	for (auto expression : expressions) {
		getAggsFromExpr(expression, result);	
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

void substituteExprInExpr(ExprTreePtr& exprToCompute, string oldExpr, ExprTreePtr newExpr) {
	ExprTreePtr tmp;
	if (exprToCompute->toString() == oldExpr) {
		exprToCompute = newExpr;
	} else {
		if (exprToCompute->getChild()) {
			tmp = exprToCompute->getChild();
			substituteExprInExpr(tmp, oldExpr, newExpr);
			exprToCompute->setChild(tmp);
		} 
		if (exprToCompute->getLHS()) {
			tmp = exprToCompute->getLHS();
			substituteExprInExpr(tmp, oldExpr, newExpr);
			exprToCompute->setLHS(tmp);
		}
		if (exprToCompute->getRHS()) {
			tmp = exprToCompute->getRHS();
			substituteExprInExpr(tmp, oldExpr, newExpr);
			exprToCompute->setRHS(tmp);
		}
	}
}

void substituteExprInExprs(vector<ExprTreePtr>& exprsToCompute, string oldExpr, ExprTreePtr newExpr) {
	for (auto& expression : exprsToCompute) {
		substituteExprInExpr(expression, oldExpr, newExpr);	
	}
}

MyDB_AttTypePtr getExprType(string expr, MyDB_SchemaPtr tableSchema) {
	MyDB_Record myRec(tableSchema);
	return myRec.getType(expr);
}

MyDB_TableReaderWriterPtr makeAggTable(vector<pair<MyDB_AggType, string>> aggsToCompute, vector<string>& groupings, MyDB_TableReaderWriterPtr inputTable) {
	MyDB_SchemaPtr aggTableSchema = make_shared<MyDB_Schema>();
	MyDB_SchemaPtr inputTableSchema = inputTable->getTable()->getSchema();
	int aggregate_index = 0;
	int grouping_index = 0;

	for (auto grouping : groupings) {
		aggTableSchema->getAtts().push_back(make_pair("group" + to_string(grouping_index++), getExprType(grouping, inputTableSchema)));
	}
	for (auto aggAtt : aggsToCompute) {
		aggTableSchema->getAtts().push_back(make_pair("agg" + to_string(aggregate_index++), getExprType(aggAtt.second, inputTableSchema)));
	}

	MyDB_TablePtr aggTable = make_shared<MyDB_Table>("aggTable", "aggTableStorageLoc", aggTableSchema);
	return make_shared<MyDB_TableReaderWriter>(aggTable, inputTable->getBufferMgr());
}

void substituteAggsInProjections(vector<ExprTreePtr>& exprsToCompute, vector<pair<MyDB_AggType, string>> aggsToCompute, MyDB_SchemaPtr aggOutSchema, int groupsNumber) {
	for (int i = 0; i < aggsToCompute.size(); i++) {
		string aggString;
		if (aggsToCompute[i].first == avg) {
			aggString = "avg(" + aggsToCompute[i].second + ")";
		} else {
			aggString = "sum(" + aggsToCompute[i].second + ")";
		}
		substituteExprInExprs(exprsToCompute, aggString, make_shared<Identifier>("agg", aggOutSchema->getAtts()[i + groupsNumber].first.c_str()));
	} 
}

void substituteGroupsInProjections(vector<ExprTreePtr>& exprsToCompute, vector<string> groupings, MyDB_SchemaPtr aggOutSchema) {
	for (int i = 0; i < groupings.size(); i++) {
		substituteExprInExprs(exprsToCompute, groupings[i], make_shared<Identifier>("group", aggOutSchema->getAtts()[i].first.c_str()));
	} 
}

bool isAttInTable(ExprTreePtr expression, MyDB_SchemaPtr tableSchema, bool isRenaming) {
	string exprString;
	for (auto att : tableSchema->getAtts()) {
		if (isRenaming) {
			exprString = expression->toRenamingString();
		} else {
			exprString = expression->toString(); 
		}

		if (exprString == ("[" + att.first + "]")) {
			return true;
		}
	}

	return false;
}

vector<pair<string, string>> makeEqualityChecks(vector<ExprTreePtr>& expressions, bool isRenaming, MyDB_SchemaPtr leftSchema, MyDB_SchemaPtr rightSchema) {
	vector<pair<string, string>> result;
	for (auto expression : expressions) {
		if (expression->isEq()) {
			ExprTreePtr lhs = expression->getLHS();
			ExprTreePtr rhs = expression->getRHS();
			bool isEqualityCheck = false;

			if (isAttInTable(lhs, leftSchema, isRenaming) && isAttInTable(rhs, rightSchema, isRenaming)) {
				isEqualityCheck = true;	
			} else if (isAttInTable(lhs, rightSchema, isRenaming) && isAttInTable(rhs, leftSchema, isRenaming)) {
				isEqualityCheck = true;
				swap(lhs, rhs);
			}

			if (isEqualityCheck) {
				if (isRenaming) {
					result.push_back(pair<string, string>(lhs->toRenamingString(), rhs->toRenamingString()));
				} else {
					result.push_back(pair<string, string>(lhs->toString(), rhs->toString()));
				}
			}
		}
	}
	return result;
}

string makeSelectionPredicate(vector<ExprTreePtr>& expressions, bool isRenaming) {
	string result = "";
	if (expressions.size() == 0) {
		return "bool[true]";
	}

	if (isRenaming) {
		result = expressions[0]->toRenamingString();
	} else {
		result = expressions[0]->toString();
	}

	for (int i = 1; i < expressions.size(); i++) {
		if (isRenaming) {
			result = "&& ( " + result + ", " + expressions[i]->toRenamingString() + ")";
		} else {
			result = "&& ( " + result + ", " + expressions[i]->toString() + ")";
		}
	}
	
	return result;
}

bool isTableFitInBuffer(MyDB_TableReaderWriterPtr table) {
	MyDB_BufferManagerPtr bufferManager = table->getBufferMgr();
	return table->getNumPages() < bufferManager->getPageSize() / 2;
}

bool isUsingBPlusTree(MyDB_TableReaderWriterPtr table) {
	return true;
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
	vector<string> groupingStrings = exprsToStrings(groupings, false);
	vector<pair<MyDB_AggType, string>> aggsToCompute = getAggsFromExprs(exprsToCompute);
	MyDB_TableReaderWriterPtr aggOutputTable = makeAggTable(aggsToCompute, groupingStrings, inputTable);
	substituteAggsInProjections(exprsToCompute, aggsToCompute, aggOutputTable->getTable()->getSchema(), groupings.size());
	substituteGroupsInProjections(exprsToCompute, groupingStrings, aggOutputTable->getTable()->getSchema());
	vector<string> projections = exprsToStrings(exprsToCompute, true); 

	cout << "start aggregate!" << endl;
	for (auto a : aggsToCompute) {
		cout << "aggs to compute:" << a.second << endl;
	}
	for (auto a : groupingStrings) {
		cout << "grouping:" << a << endl;
	}
	for (auto b : projections) {
		cout << "projection:" << b << endl;
	}
	cout << "input schema:" << inputTable->getTable()->getSchema() << endl;
	cout << "output schema:" << outputTable->getTable()->getSchema() << endl;
	cout << "aggout schema:" << aggOutputTable->getTable()->getSchema() << endl;

	Aggregate aggregateOp(inputTable, aggOutputTable, aggsToCompute, groupingStrings, "bool[true]");
	aggregateOp.run();
	RegularSelection regularSelectionOp(aggOutputTable, outputTable, "bool[true]", projections);
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
	vector<string> projections = exprsToStrings(exprsToCompute, false);
	vector<pair<string, string>> equalityChecks = makeEqualityChecks(outputSelectionPredicate, false, leftTable->getTable()->getSchema(), rightTable->getTable()->getSchema());
	string finalSelectionPredicate = makeSelectionPredicate(outputSelectionPredicate, false);
	MyDB_TableReaderWriterPtr smallTable;
	MyDB_TableReaderWriterPtr largeTable;
	cout << "start join!" << endl;
	cout << "left schema: " << leftTable->getTable()->getSchema() << endl;
	cout << "right schema: " << rightTable->getTable()->getSchema() << endl;
	cout << "output schema: " << outputTable->getTable()->getSchema() << endl; 
	for (auto e : equalityChecks) {
		cout << "equality check:" << e.first << "=" << e.second << endl;
	}
	for (auto p : projections) {
		cout << "projection: " << p << endl;
	}

	cout << "final predicate:" << finalSelectionPredicate << endl;

	getSmallAndLargeTable(leftTable, rightTable, smallTable, largeTable);
	if (isTableFitInBuffer(smallTable)) {		// run scan join
		ScanJoin scanJoinOp(leftTable, rightTable, outputTable, finalSelectionPredicate, projections, equalityChecks, "bool[true]", "bool[true]");
		scanJoinOp.run();
	} else {		// run sort merge join
		SortMergeJoin sortMergeJoinOp(leftTable, rightTable, outputTable, finalSelectionPredicate, projections, equalityChecks[0], "bool[true]", "bool[true]");
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
	cout << "start scan!" << endl;

	MyDB_TableReaderWriterPtr outputTable = make_shared<MyDB_TableReaderWriter>(outputSpec, inputSpec->getBufferMgr());
	string selectionPredicate = makeSelectionPredicate(selectionPred, true);

	cout << "input schema: " << inputSpec->getTable()->getSchema() << endl;
	cout << "output schema: " << outputSpec->getSchema() << endl;
	cout << "selection predicate: " << selectionPredicate << endl;
	for (auto e : exprsToCompute) {
		cout << "expressions to compute: " << e << endl;
	}

	RegularSelection regularSelectionOp(inputSpec, outputTable, selectionPredicate, exprsToCompute);
	regularSelectionOp.run();

	return outputTable;
}

// this costs the table scan returning the compute set of statistics for the output
pair <double, MyDB_StatsPtr> LogicalTableProjection :: cost () {
	return inputOp->cost();	
}

MyDB_TableReaderWriterPtr LogicalTableProjection :: execute () {
	MyDB_TableReaderWriterPtr inputTable = inputOp->execute();
	MyDB_TableReaderWriterPtr outputTable = make_shared<MyDB_TableReaderWriter>(outputSpec, inputTable->getBufferMgr());
	vector<string> projections = exprsToStrings(exprsToCompute, false);

	cout << "start projection!" << endl;
	cout << "input schema: " << inputTable->getTable()->getSchema() << endl;
	cout << "output schema: " << outputSpec->getSchema() << endl;
	for (auto e : projections) {
		cout << "expressions to compute: " << e << endl;
	}

	RegularSelection regularSelectionOp(inputTable, outputTable, "bool[true]", projections);
	regularSelectionOp.run();

	return outputTable;
}

#endif
