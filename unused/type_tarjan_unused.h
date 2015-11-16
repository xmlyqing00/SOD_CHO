#ifndef TYPE_TARJAN_H
#define TYPE_TARJAN_H

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

#endif // TYPE_TARJAN_H

