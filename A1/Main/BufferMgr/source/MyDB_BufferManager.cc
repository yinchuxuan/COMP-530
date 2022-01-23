
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

	pageHashMap[pageKey]->handleReference++;
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
	pageHashMap[pageKey]->handleReference++;
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

	pageHashMap[pageKey]->handleReference++;
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

	pageHashMap[pageKey]->handleReference++;
	anonymous_index = index + 1;
	return make_shared<MyDB_PageHandleBase>(pageKey, this);
}

void MyDB_BufferManager :: unpin (MyDB_PageHandle unpinMe) {
	MyDB_Page* page;

	if (!pageHashMap.count(unpinMe->pageKey)) {
		// Error: unpin a not existed page!
		cout << "Error in unpin: can not unpin an not existed page!" << endl;
		return;
	}

	page = pageHashMap[unpinMe->pageKey];

	if (!page->bufferNode) {
		// Error: unpin a not buffered page!
		cout << "Error in unpin: can not unpin a not buffered page!" << endl;
		return;
	}
	if (page->bufferNode->status != PINNED) {
		// Error: unpin a not pinned page! 
		cout << "Error in unpin: can not unpin a not pinned page!" << endl;
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
	for (auto& pair : pageHashMap) {
		MyDB_Page* page = pair.second;

		if (page->isWritten) {	// should write back
			if (page->type == NORMAL) {
				writePage(page->table->getStorageLoc(), page->index * pageSize, pageSize, page->bufferNode->buffer);
			} else {
				writePage(tempFile, page->index * pageSize, pageSize, page->bufferNode->buffer);
			}
		}
		if (page->bufferNode->status == PINNED) {	// should unpin buffer
			unpinBufferNode(page->bufferNode);
		}

		delete page;
	}

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
		cout << "Error in getBuffer: This page is not exist!" << endl;
	}
	page = pageHashMap[pageKey];

	if (!page->bufferNode) {
		loadPage(page);
	}

	return static_cast<void*>(page->bufferNode->buffer);
}

void MyDB_BufferManager :: writeBuffer(MyDB_PageKey pageKey) {
	MyDB_Page* page;

	if (!pageHashMap.count(pageKey)) {
		// Error: This page is not exist!
		cout << "Error in writeBuffer: can not write to a not existed page!" << endl;
		return;
	}
	page = pageHashMap[pageKey];

	if (!page->bufferNode) {
		// Error: This page is not in buffer!
		cout << "Error in writeBuffer: can not write to a not buffered page!" << endl;
		return;
	}

	page->isWritten = true;
}

void MyDB_BufferManager :: decreaseReference(MyDB_PageKey pageKey) {
	MyDB_Page* page;

	if (!pageHashMap.count(pageKey)) {
		// Error: This page is not exist!
		cout << "Error in decreaseReference: can not decrease reference to a not existed page!" << endl;
		return;
	}
	page = pageHashMap[pageKey];

	if (!(--page->handleReference) && page->bufferNode && page->bufferNode->status == PINNED) {		// should unpin pinned page
		unpinBufferNode(page->bufferNode);
	}
	if (!page->handleReference && page->type == ANONYMOUS) {		// should destroy anonymous page
		evictPage(page);
	}
}

void MyDB_BufferManager :: pinBufferNode(MyDB_BufferNode* bufferNode) {
	bufferNode->status = PINNED;

	if (bufferListHead == bufferListTail) {
		// Error: there will be no buffer memory remained!
		cout << "Error in pinBufferNode: can not pin page; there will be no buffer memory remained!" << endl;
		return;
	} else {
		if (bufferNode == bufferListHead) {
			bufferListHead = bufferNode->next;
		}
		if (bufferNode == bufferListTail) {
			bufferListTail = bufferNode->prev;
		}

		bufferNode->prev->next = bufferNode->next;
		bufferNode->next->prev = bufferNode->prev;
		bufferNode->next = nullptr;
		bufferNode->prev = nullptr;
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
	if (page->type == NORMAL) {
		readPage(page->table->getStorageLoc(), page->index * pageSize, pageSize, bufferNode->buffer);
	} else {
		readPage(tempFile, page->index * pageSize, pageSize, bufferNode->buffer);
	}

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

	page->bufferNode = bufferNode;
}

void MyDB_BufferManager :: evictPage(MyDB_Page* page) {
	if (page->isWritten) {
		if (page->type == NORMAL) {
			writePage(page->table->getStorageLoc(), page->index * pageSize, pageSize, page->bufferNode->buffer);
		} else {
			writePage(tempFile, page->index * pageSize, pageSize, page->bufferNode->buffer);
		}
	}

	page->bufferNode->status = EMPTY;
	emptyBufferNodes.push(page->bufferNode);
	page->bufferNode = nullptr;

	if (!page->handleReference) {	// should kill page
		if (page->type == ANONYMOUS) {	// should recycle slot in temp file
			anonymous_index = page->index;
		}

		if (page->type == NORMAL) {
			pageHashMap.erase(generatePageKey(page->table->getName(), page->index));
		} else {
			pageHashMap.erase(generatePageKey(tempFile, page->index));
		}
		delete page;
	}
}

#endif


