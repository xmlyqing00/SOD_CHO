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
const float RESIZE_RATE = 0.5;

const int CONNECTED_COUNT = 10;
const float CLUSTER_CONNECTED = 0.001;
const int COLOR_DIFF = 20;
const float CLUSTER_COVERING = 0.01;
const int CLUSTER_SIZE = 25;
const int CLUSTER_CORRELATION = 2;
const float COVERING_RATE = 0.001;
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
	vector<T> data;
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
		p_rear = 0;
		data.clear();
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
			if (relation.ptr<float>(idx)[i] >= 0) {

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
		//for (int i = 0; i < count; i++) cout << componentIndex[i] << endl;
	}

};



#endif // TYPES_H

