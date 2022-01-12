
#ifndef STACK_H
#define STACK_H

#include <iostream>

// this is the node class used to build up the LIFO stack
template <class Data>
class Node {

private:

	Data holdMe;
	Node<Data>* next;
	
public:

	Node(Data data, Node<Data>* ptr):holdMe(data), next(ptr) {}

	Data getData() {
		return holdMe;
	}

	void setData(Data data) {
		holdMe = data;
	}

	Node<Data>* getNext() {
		return next;
	}

	void setNext(Node<Data>* ptr) {
		next = ptr;
	}
};

// a simple LIFO stack
template <class Data> 
class Stack {

	Node <Data> *head;

public:

	// destroys the stack
	~Stack () {
		Node<Data>* tmp;

		while (head) {
			tmp = head->getNext();
			delete head;
			head = tmp;
		}
	}

	// creates an empty stack
	Stack () { 
		head = nullptr;
	}

	// adds pushMe to the top of the stack
	void push (Data pushMe) { 
		Node<Data>* tmp = new Node<Data>(pushMe, head);
		head = tmp;
	}

	// return true if there are not any items in the stack
	bool isEmpty () {
		return !head; 
	}

	// pops the item on the top of the stack off, returning it...
	// if the stack is empty, the behavior is undefined
	Data pop () {
		if (isEmpty()) {
			std::cout << "Error in pop(): the stack is empty!" << std::endl;
		}

		Node<Data>* tmp = head;
		Data result = tmp->getData();
		head = head->getNext();
		delete tmp;

		return result;
	}
};

#endif
