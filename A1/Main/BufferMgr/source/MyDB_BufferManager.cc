
#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include <string>

using namespace std;

MyDB_PageHandle MyDB_BufferManager :: getPage (MyDB_TablePtr table, long i) {
	MyDB_PageKey pageKey = generatePageKey(table->getName(), i);

	if (!pageHashMap.count(pageKey)) {
		pageHashMap[pageKey] = new MyDB_Page(table, i, NORMAL);
	}

	pageHashMap[pageKey]->handleRefrence++;
	return make_shared<MyDB_PageHandleBase>(pageKey, this);
}

MyDB_PageHandle MyDB_BufferManager :: getPage () {
	long index = anonymous_index;
	MyDB_PageKey pageKey;

	while (pageHashMap.count(generatePageKey(tempFile, index))) {
		index++;
	}

	pageKey = generatePageKey(tempFile, index);
	pageHashMap[pageKey] = new MyDB_Page(index, ANONYMOUS);
	pageHashMap[pageKey]->handleRefrence++;
	anonymous_index = index + 1;
	return make_shared<MyDB_PageHandleBase>(pageKey, this);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage (MyDB_TablePtr table, long i) {
	MyDB_PageKey pageKey = generatePageKey(table->getName(), i);

	if (!pageHashMap.count(pageKey)) {
		pageHashMap[pageKey] = new MyDB_Page(table, i, NORMAL);
	} 
	
	if (!pageHashMap[pageKey]->bufferNode) {
		loadPage(pageHashMap[pageKey]);
	}
	
	if (pageHashMap[pageKey]->bufferNode->status != PINNED) {
		pinBufferNode(pageHashMap[pageKey]->bufferNode);
	}

	pageHashMap[pageKey]->handleRefrence++;
	return make_shared<MyDB_PageHandleBase>(pageKey, this);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage () {
	long index = anonymous_index;
	MyDB_PageKey pageKey;

	while (pageHashMap.count(generatePageKey(tempFile, index))) {
		index++;
	}

	pageKey = generatePageKey(tempFile, index);

	if (!pageHashMap.count(pageKey)) {
		pageHashMap[pageKey] = new MyDB_Page(index, ANONYMOUS);
	} 
	
	if (!pageHashMap[pageKey]->bufferNode) {
		loadPage(pageHashMap[pageKey]);
	}
	
	if (pageHashMap[pageKey]->bufferNode->status != PINNED) {
		pinBufferNode(pageHashMap[pageKey]->bufferNode);
	}

	pageHashMap[pageKey]->handleRefrence++;
	anonymous_index = index + 1;
	return make_shared<MyDB_PageHandleBase>(pageKey, this);
}

void MyDB_BufferManager :: unpin (MyDB_PageHandle unpinMe) {
	MyDB_Page* page;

	if (!pageHashMap.count(unpinMe->pageKey)) {
		// Error: unpin a not existed page!
		cout << "Error: can not unpin an not existed page!" << endl;
		return;
	}

	page = pageHashMap[unpinMe->pageKey];

	if (!page->bufferNode) {
		// Error: unpin a not buffered page!
		cout << "Error: can not unpin a not buffered page!" << endl;
		return;
	}
	if (page->bufferNode->status != PINNED) {
		// Error: unpin a not pinned page! 
		cout << "Error: can not unpin a not pinned page!" << endl;
		return;
	}	

	unpinBufferNode(page->bufferNode);
}

MyDB_BufferManager :: MyDB_BufferManager (size_t pgSize, size_t pgNum, string tmpFile): pageSize(pgSize), numPages(pgNum), tempFile(tmpFile), anonymous_index(0) {
	if (pgNum > 0) {
		bufferListHead = new MyDB_BufferNode(pgSize);
		emptyBufferNodes.push(bufferListHead);

		MyDB_BufferNode* tmp = bufferListHead;
		for (size_t i = 0; i < pgNum - 1; i++) {
			tmp->next = new MyDB_BufferNode(pgSize);
			tmp->next->prev = tmp;
			tmp = tmp->next;
			emptyBufferNodes.push(tmp);
		}

		bufferListTail = tmp;
		bufferListTail->next = bufferListHead;
		bufferListHead->prev = bufferListTail;	
	}
}

MyDB_BufferManager :: ~MyDB_BufferManager () {
	if (bufferListHead) {
		while (bufferListHead != bufferListTail) {
			MyDB_BufferNode* tmp = bufferListHead;
			bufferListHead = bufferListHead->next;
			delete tmp;
		}

		delete bufferListHead;
	}
}

void* MyDB_BufferManager :: getBuffer(MyDB_PageKey pageKey) {
	MyDB_Page* page;

	if (!pageHashMap.count(pageKey)) {
		// Error: This page is not exist!
	}
	page = pageHashMap[pageKey];

	if (!page->bufferNode) {
		loadPage(page);
	}

	return static_cast<void*>(page->bufferNode);
}

void MyDB_BufferManager :: writeBuffer(MyDB_PageKey pageKey) {
	MyDB_Page* page;

	if (!pageHashMap.count(pageKey)) {
		// Error: This page is not exist!
		cout << "Error: can not write to a not existed page!" << endl;
		return;
	}
	page = pageHashMap[pageKey];

	if (!page->bufferNode) {
		// Error: This page is not in buffer!
		cout << "Error: can not write to a not buffered page!" << endl;
		return;
	}

	page->isWritten = true;
}

void MyDB_BufferManager :: decreaseReference(MyDB_PageKey pageKey) {
	MyDB_Page* page;

	if (!pageHashMap.count(pageKey)) {
		// Error: This page is not exist!
		cout << "Error: can not decrease reference to a not existed page!" << endl;
		return;
	}
	page = pageHashMap[pageKey];

	if (!(--page->handleRefrence) && page->type == ANONYMOUS) {
		evictPage(page);
	}
}

void MyDB_BufferManager :: pinBufferNode(MyDB_BufferNode* bufferNode) {
	bufferNode->status = PINNED;

	if (bufferListHead == bufferListTail) {
		// Error: there will be no buffer memory remained!
		cout << "Error: can not pin page; there will be no buffer memory remained!" << endl;
		return;
	} else {
		bufferNode->prev->next = bufferNode->next;
		bufferNode->next->prev = bufferNode->prev;
	}
}

void MyDB_BufferManager :: unpinBufferNode(MyDB_BufferNode* bufferNode) {
	bufferNode->status = UNPINNED;

	// Insert buffer node to the head of buffer list
	bufferNode->next = bufferListHead;
	bufferNode->prev = bufferListTail;
	bufferListHead->prev = bufferNode;
	bufferListTail->next = bufferNode;
	bufferListHead = bufferNode;
}

void MyDB_BufferManager :: loadPage(MyDB_Page* page) {
	MyDB_BufferNode* bufferNode;

	if (!emptyBufferNodes.size()) {		// should evict page
		evictPage(bufferListTail->page);
	}
	bufferNode = emptyBufferNodes.top();
	emptyBufferNodes.pop();

	bufferNode->page = page;
	bufferNode->status = UNPINNED;
	readPage(page->table->getStorageLoc(), page->index * pageSize, pageSize, bufferNode->buffer);
	if (bufferNode != bufferListHead) {
		bufferNode->prev->next = bufferNode->next;
		bufferNode->next->prev = bufferNode->prev;
		bufferNode->next = bufferListHead;
		bufferNode->prev = bufferListHead->prev;
		bufferListHead->prev->next = bufferNode;
		bufferListHead->prev = bufferNode;
		bufferListHead = bufferNode;
		bufferListTail = bufferNode->prev;
	}
}

void MyDB_BufferManager :: evictPage(MyDB_Page* page) {
	if (page->isWritten) {
		writePage(page->table->getStorageLoc(), page->index * pageSize, pageSize, page->bufferNode->buffer);
	}

	page->bufferNode->status = EMPTY;
	emptyBufferNodes.push(page->bufferNode);
	page->bufferNode = nullptr;

	if (!page->handleRefrence) {	// should kill page
		if (page->type == ANONYMOUS) {	// should recycle slot in temp file
			anonymous_index = page->index;
		}

		pageHashMap.erase(generatePageKey(page->table->getName(), page->index));
		delete page;
	}
}

#endif


