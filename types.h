#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <utility>
#include <map>
#include <bitset>
#include <vector>
#include <stack>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const Point dxdy[8] = { Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ), Point( -1, -1 ), Point( 1, -1 ), Point( -1, 1 ), Point( 1, 1 ) };
const int MAX_QUE_CAP = 1000000;//50000000;
const int MAX_HEAP_CAP = 1000000;
const double e = 2.718281828459;
const double PI = 3.14159265358;
const int PIXEL_CONNECT = 8;

const int COLOR_DIFF = 20;
const float CLUSTER_COVERING = 0.001;
const int CLUSTER_SIZE = 25;
const int CLUSTER_CORRELATION = 2;
const float COVERING_RATE = 0.05;
const float RELATION_PREV = 0.9;

#define INF 2000000000

struct TypeLink {
	int u, v;
	TypeLink *next;

	TypeLink() {
		u = 0;
		v = 0;
		next = NULL;
	}
	TypeLink( int _u, int _v, TypeLink *_next ) {
		u = _u;
		v = _v;
		next = _next;
	}
};

struct TypeLayer {
	float z_value;
	int idx;
	TypeLayer() {
		z_value = 0;
		idx = 0;
	}
	TypeLayer(float _z_value, int _idx) {
		z_value = _z_value;
		idx = _idx;
	}
};

class Vec3bCompare{
public:
	bool operator() (const Vec3b &a, const Vec3b &b) {
		for (int i = 0; i < 3; i++) {
			if (a.val[i] < b.val[i]) return true;
			if (a.val[i] > b.val[i]) return false;
		}
		return true;
	}
};

template<class T>class TypeQue {

private:
	T data[MAX_QUE_CAP];
	int p_front, p_size, p_rear;

public:
	TypeQue() {
		p_front = 0;
		p_size = 0;
		p_rear = 0;
	}
	bool empty() {
		if ( p_size == 0 ) return true;
		else return false;
	}
	void push( T x ) {
		data[p_rear++] = x;
		if ( p_rear == MAX_QUE_CAP ) p_rear = 0;
		p_size++;
		if ( p_size > MAX_QUE_CAP ) {
			cout << "que out of size !! " << endl;
			return;
		}
	}
	T front() {
		return data[p_front];
	}
	void pop() {
		p_front++;
		if ( p_front == MAX_QUE_CAP ) p_front = 0;
		p_size--;
	}
	int size() {
		return p_size;
	}
	void clear() {
		p_size = 0;
		p_front = 0;
		p_rear = 0;
	}
	void debug() {
		cout << "que : " << endl;
		for ( int i = p_front; i < p_rear; i++ ) cout << data[i] << endl;
	}
};

class TypeTarjan {

private:
	stack<int> dfsStack;
	bool *inStack;
	int *dfn, *min_dfn;
	int dfsIndex;
	int count;
	Mat relation, route;

	void dfsComponent(int idx) {

		dfn[idx] = min_dfn[idx] = dfsIndex++;
		inStack[idx] = true;
		dfsStack.push(idx);
		for (int i = 0; i < count; i++) {

			if (route.ptr<int>(idx)[i] == 0) continue;
			if (relation.ptr<float>(idx)[i] > 0) {

				if(dfn[i] == -1) {
					dfsComponent(i);
					min_dfn[idx] = min(min_dfn[idx], min_dfn[i]);
				} else if (inStack[i]) {
					min_dfn[idx] = min(min_dfn[idx], min_dfn[i]);
				}
			}
		}

		if (dfn[idx] == min_dfn[idx]) {
			int ele;
			do {
				ele = dfsStack.top(); dfsStack.pop();
				inStack[ele] = false;
				componentIndex[ele] = componentCount;
				component[componentCount].push_back(ele);
			} while (ele != idx);
			componentCount++;
		}
	}

public:
	int componentCount;
	int *componentIndex;
	vector<int> *component;

	void init(const int _count, const Mat &_relation, const Mat &_route) {
		count = _count;
		inStack = new bool[count];
		dfn = new int[count];
		min_dfn = new int[count];
		dfsIndex = 0;
		relation = _relation;
		route=  _route;

		componentCount = 0;
		componentIndex = new int[count];
		component = new vector<int>[count];
		for (int i = 0; i < count; i++) {
			inStack[i] = false;
			dfn[i] = min_dfn[i] = -1;
			componentIndex[i] = -1;
		}
	}

	void clear() {
		delete[] inStack;
		delete[] dfn;
		delete[] min_dfn;
		delete[] componentIndex;
		delete[] component;
	}

	void getComponent() {
		for (int i = 0; i < count; i++) {
			if (componentIndex[i] == -1) dfsComponent(i);
		}
	}

};

class TypeHeap {

private:
	TypeLayer data[MAX_HEAP_CAP];
	map<int, int> keyPos;
	int p_tail;

	void upElement( int pos ) {

		while ( pos > 1 ) {
			int fa_pos = pos >> 1;
			if ( data[fa_pos].z_value < data[pos].z_value ) {
				keyPos[data[fa_pos].idx] = pos;
				keyPos[data[pos].idx] = fa_pos;
				swap( data[fa_pos], data[pos] );
			} else break;
			pos = fa_pos;
		}
	}
	void downElement( int pos ) {

		while ( pos < p_tail ) {

			int son_pos = pos << 1;
			if ( son_pos > p_tail ) break;
			if ( son_pos < p_tail && data[son_pos].z_value < data[son_pos + 1].z_value ) son_pos++;
			if ( data[pos].z_value < data[son_pos].z_value ) {
				keyPos[data[son_pos].idx] = pos;
				keyPos[data[pos].idx] = son_pos;
				swap( data[son_pos], data[pos] );
			} else break;
			pos = son_pos;
		}
	}

public:
	TypeHeap() {
		p_tail = 0;
	}
	bool empty() {
		if ( p_tail == 0 ) return true;
		else return false;
	}
	void push( TypeLayer v ) {


		data[++p_tail] = v;
		keyPos[v.idx] = p_tail;
		if ( p_tail > MAX_HEAP_CAP ) {
			cout << "heap out of size !! " << endl;
			return;
		}
		upElement( p_tail );
	}

	int top() {
		return data[1].idx;
	}
	void pop() {
		keyPos[data[p_tail].idx] = 0;
		data[0] = data[p_tail--];
		downElement( 0 );
	}
	void update( TypeLayer v ) {
		data[keyPos[v.idx]].z_value = v.z_value;
		upElement( keyPos[v.idx] );
		downElement(keyPos[v.idx]);
	}
	int size() {
		return p_tail;
	}
	void clear() {
		p_tail = 0;
		keyPos.clear();
	}
	void debug() {
		cout << "heap : " << endl;
		for ( int i = 1; i <= p_tail; i++ ) cout << data[i].idx << " " << data[i].z_value << endl;
	}
};


#endif // TYPES_H

