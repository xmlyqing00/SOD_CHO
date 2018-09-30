#ifndef TYPE_QUE_H
#define TYPE_QUE_H

#include "comman.h"

template<class T>class TypeQue {

private:
	vector<T> data;
	int p_front, p_size;

public:
	TypeQue() {
		p_front = 0;
		p_size = 0;
		data.clear();
	}
	bool empty() {
		if ( p_size == 0 ) return true;
		else return false;
	}
	void push( T x ) {
		data.push_back(x);
		p_size++;
	}
	T front() {
		return data[p_front];
	}
	void pop() {
		p_front++;
		p_size--;
	}
	int size() {
		return p_size;
	}
	void clear() {
		p_size = 0;
		p_front = 0;
		data.clear();
	}
	void debug() {
		cout << "que : " << endl;
		for ( int i = p_front; i < p_size; i++ ) cout << data[i] << endl;
	}
};

// TypeQue<int> &que = *(new TypeQue<int>);
// delete &que;

#endif // TYPE_QUE_H