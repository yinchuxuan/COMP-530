
#ifndef RECORD_ITER_RESTRICTED_ALT_H
#define RECORD_ITER_RESTRICTED_ALT_H

#include "MyDB_RecordIteratorAlt.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include <vector>

using namespace std;

class MyDB_RecordIteratorRestrictedAlt : public MyDB_RecordIteratorAlt {

public:

        // load the current record into the parameter
    void getCurrent (MyDB_RecordPtr intoMe) override {
		myIter->getCurrent (intoMe);
	}

        // after a call to advance (), a call to getCurrentPointer () will get the address
        // of the record.  At a later time, it is then possible to reconstitute the record
        // by calling MyDB_Record.fromBinary (obtainedPointer)... ASSUMING that the page that
        // the record is located on has not been swapped out
        void *getCurrentPointer () {
		return myIter->getCurrentPointer ();
	}

        // advance to the next record... returns true if there is a next record, and
        // false if there are no more records to iterate over.  Not that this cannot
        // be called until after getCurrent () has been called
    bool advance () override {
		while (true) {
			if (myIter->advance ()) {
				myIter->getCurrent (myRec);
				if (!lowComparator () && !highComparator ()) {
					return true;
				}
			} else {
				return false;
			}
		}
	}

	// destructor and contructor
	MyDB_RecordIteratorRestrictedAlt (vector <MyDB_PageReaderWriter> &forUsIn, MyDB_RecordPtr myRecIn, function <bool ()> lowComparatorIn, 
		function <bool ()> highComparatorIn) {

		// just remember all of the parameters
		forUs = forUsIn;
		lowComparator = lowComparatorIn;
		highComparator = highComparatorIn;
		myRec = myRecIn;
        myIter = getIteratorAlt(forUs);
	}

	~MyDB_RecordIteratorRestrictedAlt() {}

private:

	MyDB_RecordIteratorAltPtr myIter;
	vector <MyDB_PageReaderWriter> forUs;
	function <bool ()> lowComparator;
	function <bool ()> highComparator;
	MyDB_RecordPtr myRec;
};

#endif