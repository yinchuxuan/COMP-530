
#ifndef AGG_CC
#define AGG_CC

#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include <unordered_map>

using namespace std;

Aggregate :: Aggregate (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
                vector<pair<MyDB_AggType, string>> aggsToComputeIn,
                vector<string>groupingsIn, string selectionPredicateIn) {
    if (outputIn->getTable()->getSchema()->getAtts().size() != aggsToComputeIn.size () + groupingsIn.size ()) {
        cout << "error, the output schema needs to have the same number of atts as (# of aggs to compute + # groups).\n";
        return;
    }

    input = inputIn;
    output = outputIn;
    aggsToCompute = aggsToComputeIn;
    groupings = groupingsIn;
    selectionPredicate = selectionPredicateIn;
}

void Aggregate :: run () {
    unordered_map<size_t, void*> hashtable;
    vector<MyDB_PageReaderWriterPtr> tmpPages;
    int aggregateAttNumber = aggsToCompute.size();
    int groupNumber = groupings.size();
    int i;

    // build a schema that contains all output and counts
    MyDB_SchemaPtr aggregateSchema = make_shared<MyDB_Schema>();
	for (auto &p : output->getTable ()->getSchema ()->getAtts ()) {
		aggregateSchema->appendAtt (p);
    }
    aggregateSchema->appendAtt(pair<string, MyDB_AttTypePtr>("counter", make_shared<MyDB_IntAttType>()));

    // build a schema that combined with original schama and aggregate schema
    MyDB_SchemaPtr combinedSchema = make_shared<MyDB_Schema>();
    for (auto &p : input->getTable ()->getSchema ()->getAtts ()) {
		combinedSchema->appendAtt (p);
    }
    for (auto &p : aggregateSchema->getAtts ()) {
		combinedSchema->appendAtt (p);
    }

    MyDB_RecordPtr originRecord = input->getEmptyRecord();
    MyDB_RecordPtr aggregateRecord = make_shared<MyDB_Record>(aggregateSchema);
    MyDB_RecordPtr combinedRecord = make_shared<MyDB_Record>(combinedSchema);
    MyDB_RecordPtr outputRecord = make_shared<MyDB_Record>(output->getTable()->getSchema());
    combinedRecord->buildFrom(originRecord, aggregateRecord);

	vector<func> groupAttributes;
	for (auto &p : groupings) {
		groupAttributes.push_back(originRecord->compileComputation(p));
	}
    vector<func> aggregateAttributes;
    for (auto &p : aggsToCompute) {
		aggregateAttributes.push_back(originRecord->compileComputation(p.second));
	}
    vector<func> updateAggregateAtts;
    for (int i = 0; i < aggregateAttNumber; i++) {
        updateAggregateAtts.push_back(combinedRecord->compileComputation("+ (" + aggsToCompute[i].second + ", [" + aggregateSchema->getAtts()[i + groupNumber].first + "])"));
    }
    vector<func> finalAggComps;
    for (int i = 0; i < aggregateAttNumber; i++) {
        if (aggsToCompute[i].first == sum) {
            finalAggComps.push_back(aggregateRecord->compileComputation("[" + aggregateSchema->getAtts()[i + groupNumber].first + "]"));
        }
        if (aggsToCompute[i].first == avg) {
            finalAggComps.push_back(aggregateRecord->compileComputation("/ ([" + aggregateSchema->getAtts()[i + groupNumber].first + "], [" + aggregateSchema->getAtts()[groupNumber + aggregateAttNumber].first + "])"));
        }
        if (aggsToCompute[i].first == cnt) {
            finalAggComps.push_back(aggregateRecord->compileComputation("[" + aggregateSchema->getAtts()[groupNumber + aggregateAttNumber].first + "]"));
        }
    }

    func inputPred = originRecord->compileComputation(selectionPredicate);
    func counterInitializer = aggregateRecord->compileComputation("int[1]");
    func counterIncrease = aggregateRecord->compileComputation("+ ([" + aggregateSchema->getAtts()[groupNumber + aggregateAttNumber].first + "], int[1])");

    // add all of the records to the hash table
	MyDB_RecordIteratorAltPtr myIter = input->getIteratorAlt();

    // Iterate over original table
	while (myIter->advance ()) {
        myIter->getCurrent(originRecord);

		// see if it is accepted by the predicate
		if (!inputPred()->toBool()) {
			continue;
		}

        // compute its hash
		size_t hashVal = 0;
		for (auto &f : groupAttributes) {
			hashVal ^= f()->hash();
		}

        // If this group haven't found yet, add a group
        if (!hashtable.count(hashVal)) {
            for (int i = 0; i < groupNumber; i++) {
                aggregateRecord->getAtt(i)->set(groupAttributes[i]());
            }
            for (int i = groupNumber; i < aggregateAttNumber + groupNumber; i++) {
                aggregateRecord->getAtt(i)->set(aggregateAttributes[i - groupNumber]());
            }
            aggregateRecord->getAtt(groupNumber + aggregateAttNumber)->set(counterInitializer());

            aggregateRecord->recordContentHasChanged();

            if (tmpPages.empty()) {
                tmpPages.push_back(make_shared<MyDB_PageReaderWriter>(true, *output->getBufferMgr()));
            }

            MyDB_PageReaderWriterPtr lastPage = tmpPages[tmpPages.size() - 1];
            if (!(hashtable[hashVal] = lastPage->appendAndReturnLocation(aggregateRecord))) {
                tmpPages.push_back(make_shared<MyDB_PageReaderWriter>(true, *output->getBufferMgr()));
                MyDB_PageReaderWriterPtr lastPage = tmpPages[tmpPages.size() - 1];
            }
        } else {    // update aggregate record
            aggregateRecord->fromBinary(hashtable[hashVal]);
            for (int i = groupNumber; i < aggregateAttNumber + groupNumber; i++) {
                aggregateRecord->getAtt(i)->set(updateAggregateAtts[i - groupNumber]());
            }
            aggregateRecord->getAtt(groupNumber + aggregateAttNumber)->set(counterIncrease());

            aggregateRecord->recordContentHasChanged();
            aggregateRecord->toBinary(hashtable[hashVal]);
        }
    }

    // write all aggregate records to output table
    for (auto p : hashtable) {
        aggregateRecord->fromBinary(p.second);

        for (i = 0; i < groupNumber; i++) {
            outputRecord->getAtt(i)->set (aggregateRecord->getAtt(i));
        }
        for (auto a : finalAggComps) {
            outputRecord->getAtt (i++)->set (a());
        }

        outputRecord->recordContentHasChanged ();
        output->append(outputRecord);
    }
}

#endif

