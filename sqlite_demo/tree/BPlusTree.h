#ifndef SQLITE_DEMO_BPLUS_TREE_H
#define SQLITE_DEMO_BPLUS_TREE_H

typedef unsigned char byte;
typedef struct Node Node;
typedef struct Node Tree;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef u32 Key;

// ****** API *********

/* 在B+树中查找关键字。查找不到返回false，查找到，设置p_data为对应数据，设置size_data为数据大小，返回true。 */
bool search(Tree *t, Key k, void* &p_data, int &size_data);

/* 插入kv对，当key已经存在返回false，否则B+树将复制对应value. */
bool insert(Tree *&t, Key k, void *p_data, int size_data);

// TODO 删除key
// bool remove(Tree *&t, Key k);

/* 创建B+树，返回根节点指针 */
Tree *getTree();



// ******* 测试函数 ********
void printfNode(Node *node);
void printfTree(Tree *root, int indent=0);
Node* getKthChild(Node *node, int kth);

#endif
