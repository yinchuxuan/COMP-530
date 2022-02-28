
#ifndef BPLUS_C
#define BPLUS_C

#include "MyDB_INRecord.h"
#include "MyDB_BPlusTreeReaderWriter.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_PageListIteratorSelfSortingAlt.h"
#include "RecordComparator.h"
#include <stack>

MyDB_BPlusTreeReaderWriter :: MyDB_BPlusTreeReaderWriter (string orderOnAttName, MyDB_TablePtr forMe, 
	MyDB_BufferManagerPtr myBuffer) : MyDB_TableReaderWriter (forMe, myBuffer) {

	// find the ordering attribute
	auto res = forMe->getSchema ()->getAttByName (orderOnAttName);

	// remember information about the ordering attribute
	orderingAttType = res.second;
	whichAttIsOrdering = res.first;

	// and the root location
	rootLocation = getTable ()->getRootLocation ();

	rootLocation = 0;
	getTable()->setRootLocation(rootLocation);
	int initial_page = getNumPages();				// Use the last page as the first leaf page
	MyDB_INRecordPtr new_in_record = getINRecord();

	(*this)[rootLocation].setType(DirectoryPage);	// Root page should be a directory page
	(*this)[initial_page].setType(RegularPage);

	new_in_record->setPtr(initial_page);			// Create a max in_record pointing to the initial page 

	append(rootLocation, new_in_record);
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter::generateRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high, bool isSorting) {
	vector<MyDB_PageReaderWriter> pageList;
	MyDB_RecordPtr tmp_record = getEmptyRecord();
	MyDB_INRecordPtr low_tmp_record = getINRecord();
	MyDB_INRecordPtr high_tmp_record = getINRecord();
	MyDB_RecordPtr lhs = getEmptyRecord();
	MyDB_RecordPtr rhs = getEmptyRecord();

    low_tmp_record->setKey(low);
    high_tmp_record->setKey(high);
	discoverPages(rootLocation, pageList, low, high);

    return make_shared<MyDB_PageListIteratorSelfSortingAlt>(pageList, lhs, rhs, buildComparator(lhs, rhs), tmp_record, buildComparator(tmp_record, low_tmp_record), buildComparator(high_tmp_record, tmp_record), isSorting);
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter::getSortedRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high) {
    return generateRangeIteratorAlt(low, high, true);
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter::getRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high) {
    return generateRangeIteratorAlt(low, high, false);
}

bool MyDB_BPlusTreeReaderWriter :: discoverPages (int page, vector<MyDB_PageReaderWriter>&pageList, MyDB_AttValPtr low, MyDB_AttValPtr high) {
	if ((*this)[page].getType() == RegularPage) {
		pageList.push_back((*this)[page]);
		return true;
	} else {
		MyDB_RecordIteratorAltPtr iterator = (*this)[page].getIteratorAlt();
		MyDB_INRecordPtr in_record = getINRecord();
		MyDB_INRecordPtr low_tmp_record = getINRecord();
		low_tmp_record->setKey(low);
		MyDB_INRecordPtr high_tmp_record = getINRecord();
		high_tmp_record->setKey(high);
		bool result = false;

		while (iterator->advance()) {
			iterator->getCurrent(in_record);

			if (buildComparator(high_tmp_record, in_record)()) {
				result = discoverPages(in_record->getPtr(), pageList, low, high) || result;
				break;
			}

			if (!buildComparator(in_record, low_tmp_record)()) {
				result = discoverPages(in_record->getPtr(), pageList, low, high) || result;
			}
		}

		return result;
	}
}

void MyDB_BPlusTreeReaderWriter :: append (MyDB_RecordPtr record) {
	if (getNumPages() == 1) {	// Needs initialization
		rootLocation = 0;
		getTable()->setRootLocation(rootLocation);
		int initial_page = getNumPages();				// Use the last page as the first leaf page
		MyDB_INRecordPtr new_in_record = getINRecord();

		(*this)[rootLocation].clear();
		(*this)[rootLocation].setType(DirectoryPage);	// Root page should be a directory page
		(*this)[initial_page].clear();
		(*this)[initial_page].setType(RegularPage);

		new_in_record->setPtr(initial_page);			// Create a max in_record pointing to the initial page 

		(*this)[rootLocation].append(new_in_record);
	}

	MyDB_RecordPtr new_in_record = append(rootLocation, record);

	if (new_in_record != nullptr) {	// Splitting on root 
		int new_root_page_num = getNumPages();		// Create a new root page
		MyDB_PageReaderWriter new_root = (*this)[new_root_page_num];	
		MyDB_INRecordPtr max_record = getINRecord();

		new_root.setType(DirectoryPage);
		max_record->setPtr(rootLocation);
		new_root.append(new_in_record);
		new_root.append(max_record);
		
		rootLocation = new_root_page_num;
		getTable()->setRootLocation(new_root_page_num);
	}		
}

int MyDB_BPlusTreeReaderWriter :: countRecordNumberInRegularPage(MyDB_PageReaderWriter& page) {
	int count = 0;
	MyDB_RecordPtr tmp_record = getEmptyRecord();
	MyDB_RecordIteratorAltPtr iterator = page.getIteratorAlt();

	while (iterator->advance()) {
		count++;
		iterator->getCurrent(tmp_record);
	}

	return count;
}

int MyDB_BPlusTreeReaderWriter :: countRecordNumberInDirectoryPage(MyDB_PageReaderWriter& page) {
	int count = 0;
	MyDB_RecordPtr tmp_record = getINRecord();
	MyDB_RecordIteratorAltPtr iterator = page.getIteratorAlt();

	while (iterator->advance()) {
		count++;
		iterator->getCurrent(tmp_record);
	}

	return count;
}

inline void MyDB_BPlusTreeReaderWriter :: appendRecordToPage(MyDB_PageReaderWriter& page, MyDB_RecordPtr record) {
	if (!page.append(record)) {
		cerr << "page appends too many records!" << endl;
		exit(1);
	}
}

inline bool MyDB_BPlusTreeReaderWriter :: getNextRecord(MyDB_RecordIteratorAltPtr iterator, MyDB_RecordPtr& tmp_record) {
	if (iterator->advance()) {
		iterator->getCurrent(tmp_record);
		return true;
	}

	return false;
}

inline bool MyDB_BPlusTreeReaderWriter :: getNextINRecord(MyDB_RecordIteratorAltPtr iterator, MyDB_INRecordPtr& tmp_record) {
	if (iterator->advance()) {
		iterator->getCurrent(tmp_record);
		return true;
	}

	return false;
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter :: split (MyDB_PageReaderWriter splitted_page, MyDB_RecordPtr record) {
	if (splitted_page.getType() == RegularPage) {
		MyDB_RecordPtr lhs = getEmptyRecord();
		MyDB_RecordPtr rhs = getEmptyRecord();
		MyDB_PageReaderWriterPtr tmp_page = splitted_page.sort(buildComparator(lhs, rhs), lhs, rhs);
		int count = countRecordNumberInRegularPage(*tmp_page);
		MyDB_RecordIteratorAltPtr iterator = tmp_page->getIteratorAlt();
		MyDB_AttValPtr median_record_value;
		MyDB_RecordPtr tmp_record = getEmptyRecord();
		MyDB_PageReaderWriter new_page = (*this)[getNumPages()];
		int i;
	
		tmp_page->setType(RegularPage);
		new_page.setType(RegularPage);
		splitted_page.clear();
		splitted_page.setType(RegularPage);

		// Load first half records into new page 
		for (i = 0; i < count / 2; i++) {
			getNextRecord(iterator, tmp_record);
			appendRecordToPage(new_page, tmp_record);	
		}

		getNextRecord(iterator, tmp_record);	

		// find median value
		if (buildComparator(record, tmp_record)()) {
			if (count % 2) {
				appendRecordToPage(new_page, record);
				median_record_value = getKey(tmp_record);
				appendRecordToPage(splitted_page, tmp_record);
			} else {
				median_record_value = getKey(record);
				appendRecordToPage(splitted_page, record);
				appendRecordToPage(splitted_page, tmp_record);
			}
		} else {
			appendRecordToPage(splitted_page, record);

			if (count % 2) {
				appendRecordToPage(new_page, tmp_record);	

				if (getNextRecord(iterator, tmp_record)) {
					if (buildComparator(record, tmp_record)()) {
						median_record_value = getKey(record);
					} else {
						median_record_value = getKey(tmp_record);
					}

					appendRecordToPage(splitted_page, tmp_record);
				} else {
					median_record_value = getKey(record);
				}
			} else {
				if (buildComparator(record, tmp_record)()) {
					median_record_value = getKey(record);
				} else {
					median_record_value = getKey(tmp_record);
				}

				appendRecordToPage(splitted_page, tmp_record);
			}
		}

		// append rest of the content to original page
		while (getNextRecord(iterator, tmp_record)) {
			appendRecordToPage(splitted_page, tmp_record);
		}
		
		MyDB_INRecordPtr new_in_record = getINRecord();
		new_in_record->setKey(median_record_value);
		new_in_record->setPtr(getNumPages() - 1);

		return new_in_record;
	} else {
		MyDB_INRecordPtr lhs = getINRecord();
		MyDB_INRecordPtr rhs = getINRecord();
		MyDB_PageReaderWriterPtr tmp_page = splitted_page.sort(buildComparator(lhs, rhs), lhs, rhs);
		int count = countRecordNumberInDirectoryPage(*tmp_page) - 1;
		MyDB_RecordIteratorAltPtr iterator = tmp_page->getIteratorAlt();
		MyDB_AttValPtr median_record_value;
		MyDB_INRecordPtr tmp_record = getINRecord();
		MyDB_INRecordPtr new_max_record = getINRecord();
		MyDB_PageReaderWriter new_page = (*this)[getNumPages()];
		int i;

		tmp_page->setType(DirectoryPage);
		new_page.setType(DirectoryPage);
		splitted_page.clear();
		splitted_page.setType(DirectoryPage);

		for (i = 0; i < count / 2; i++) {
			getNextINRecord(iterator, tmp_record);
			appendRecordToPage(new_page, tmp_record);		
		}

		getNextINRecord(iterator, tmp_record);	

		// find median value
		if (buildComparator(record, tmp_record)()) {
			if (count % 2) {
				appendRecordToPage(new_page, record);
				median_record_value = getKey(tmp_record);
				new_max_record->setPtr(tmp_record->getPtr());
			} else {
				median_record_value = getKey(record);
				new_max_record->setPtr(record->getAtt(1)->toInt());
				appendRecordToPage(splitted_page, tmp_record);
			}
		} else {
			if (count % 2) {
				appendRecordToPage(new_page, tmp_record);	

				if (getNextINRecord(iterator, tmp_record)) {
					if (buildComparator(record, tmp_record)()) {
						median_record_value = getKey(record);
						new_max_record->setPtr(record->getAtt(1)->toInt());
						appendRecordToPage(splitted_page, tmp_record);
					} else {
						median_record_value = getKey(tmp_record);
						new_max_record->setPtr(tmp_record->getPtr());
						appendRecordToPage(splitted_page, record);
					}
				} else {
					median_record_value = getKey(record);
				}
			} else {
				if (buildComparator(record, tmp_record)()) {
					median_record_value = getKey(record);
					new_max_record->setPtr(record->getAtt(1)->toInt());
					appendRecordToPage(splitted_page, tmp_record);
				} else {
					median_record_value = getKey(tmp_record);
					new_max_record->setPtr(tmp_record->getPtr());
					appendRecordToPage(splitted_page, record);
				}
			}
		}

		appendRecordToPage(new_page, new_max_record);

		// append rest of the content to original page
		while (getNextINRecord(iterator, tmp_record)) {
			appendRecordToPage(splitted_page, tmp_record);
		}
		
		MyDB_INRecordPtr new_in_record = getINRecord();
		new_in_record->setKey(median_record_value);
		new_in_record->setPtr(getNumPages() - 1);

		return new_in_record;
	}
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter :: append (int pageNum, MyDB_RecordPtr record) {
	MyDB_RecordPtr result = nullptr;

	if ((*this)[pageNum].getType() == RegularPage) {
		if (!(*this)[pageNum].append(record)) {
			result = split((*this)[pageNum], record);
		}
	} else {
		MyDB_RecordIteratorAltPtr iterator = (*this)[pageNum].getIteratorAlt();
		MyDB_INRecordPtr tmp_in_record = getINRecord();

		while (iterator->advance()) {
			iterator->getCurrent(tmp_in_record);

			if (buildComparator(record, tmp_in_record)()) {		// Should append
				result = append(tmp_in_record->getPtr(), record);
				
				if (result != nullptr) {	// Splitting happens!
					if (!(*this)[pageNum].append(result)) {
						result = split((*this)[pageNum], result);
					} else {
						MyDB_INRecordPtr lhs = getINRecord();
						MyDB_INRecordPtr rhs = getINRecord();
						(*this)[pageNum].sortInPlace(buildComparator(lhs, rhs), lhs, rhs);
						result = nullptr;
					}		
				}

				break;
			}
		}
	}

	return result;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter :: getINRecord () {
	return make_shared <MyDB_INRecord> (orderingAttType->createAttMax ());
}

void MyDB_BPlusTreeReaderWriter :: printTree () {
	printDirectory((*this)[rootLocation]);
}

void MyDB_BPlusTreeReaderWriter :: printDirectory(MyDB_PageReaderWriter directory) {
	if (directory.getType() == DirectoryPage) {

		MyDB_RecordIteratorAltPtr iterator = directory.getIteratorAlt();
		MyDB_INRecordPtr in_record = getINRecord();

		while (iterator->advance()) {
			iterator->getCurrent(in_record);

			cout << "(";
			if (orderingAttType->promotableToInt()) {
				cout << in_record->getKey()->toInt();
			} else if (orderingAttType->promotableToDouble()) {
				cout << in_record->getKey()->toDouble();
			} else if (orderingAttType->promotableToString()) {
				cout << in_record->getKey()->toString();
			}
			cout << "," << in_record->getPtr() << ") ";
		}

		cout << endl;
		
		iterator = directory.getIteratorAlt();
		while (iterator->advance()) {
			iterator->getCurrent(in_record);
			printDirectory((*this)[in_record->getPtr()]);
		}
	}
}

MyDB_AttValPtr MyDB_BPlusTreeReaderWriter :: getKey (MyDB_RecordPtr fromMe) {

	// in this case, got an IN record
	if (fromMe->getSchema () == nullptr) 
		return fromMe->getAtt (0)->getCopy ();

	// in this case, got a data record
	else 
		return fromMe->getAtt (whichAttIsOrdering)->getCopy ();
}

function <bool ()>  MyDB_BPlusTreeReaderWriter :: buildComparator (MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {

	MyDB_AttValPtr lhAtt, rhAtt;

	// in this case, the LHS is an IN record
	if (lhs->getSchema () == nullptr) {
		lhAtt = lhs->getAtt (0);	

	// here, it is a regular data record
	} else {
		lhAtt = lhs->getAtt (whichAttIsOrdering);
	}

	// in this case, the LHS is an IN record
	if (rhs->getSchema () == nullptr) {
		rhAtt = rhs->getAtt (0);	

	// here, it is a regular data record
	} else {
		rhAtt = rhs->getAtt (whichAttIsOrdering);
	}
	
	// now, build the comparison lambda and return
	if (orderingAttType->promotableToInt ()) {
		return [lhAtt, rhAtt] {return lhAtt->toInt () < rhAtt->toInt ();};
	} else if (orderingAttType->promotableToDouble ()) {
		return [lhAtt, rhAtt] {return lhAtt->toDouble () < rhAtt->toDouble ();};
	} else if (orderingAttType->promotableToString ()) {
		return [lhAtt, rhAtt] {return lhAtt->toString () < rhAtt->toString ();};
	} else {
		cout << "This is bad... cannot do anything with the >.\n";
		exit (1);
	}
}


#endif

