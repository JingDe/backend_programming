// g++ BPlusTreeForPager.c  -I ../pager -I ../include/

#include"BPlusTree.h"
#include"pager.h"

static const int THE_ORDER=10;
// 等于pager模块定义的 SQL_DEFAULT_PAGE_SIZE
static const int PAGE_SIZE=2048; 
static const int HEADER_SIZE=256; // 

/*
struct FOR_CALCULATE_HEADER_SIZE
{
	u8 is_leaf; // 0表示内部节点，1表示叶子结点
	u16 cell_size; // 节点的cell的个数
	u16 first_cell_pos; // cell区域的起始地址
	u8 frag_size; // 空闲的碎片空间的总大小
	Pgno right_child; // 对内部节点，最右边的儿子节点的页面编号
};

static const int HEADER_SIZE=sizeof(FOR_CALCULATE_HEADER_SIZE);
*/

struct Node{
	u8 is_leaf; // 0表示内部节点，1表示叶子结点
	u16 cell_size; // 节点的cell的个数
	u16 first_cell_pos; // cell区域的起始地址
	u8 frag_size; // 空闲的碎片空间的总大小
	Pgno right_child; // 对内部节点，最右边的儿子节点的页面编号
	
	u8 content[PAGE_SIZE-HEADER_SIZE];
};

/* 内部节点：键，键对应的儿子节点页面的编号。
键：对应儿子节点的最大cell的key。
*/
struct IntCell{
	Key key;
	Pgno child; // unsigned int
};

/* 叶子结点： 键，数据大小，数据。 
sqlite的实际payload
*/
struct LeafCell{
	Key key;
	u32 size;
	u8 *data_point;
};

// forward declaration
Tree* getTree(Pager *pager);
Node* getLeafNode();
bool search(Node* node, Key k, void *&p_data, int &size_data);
bool insert(Tree* &root, Key k, void *p_data, int size_data);
bool insert2(Node* node, Key k, void *p_data, int size_data, Node* &father);
bool insertLeafCell(Node *node, Key k, void *p_data, int size_data);
void splitLeafNode(Node *node, Node* &father);
void splitIntNode(Node *node, Node* &father);
bool getKthLeafCell(Node* node, int kth, LeafCell *cell);
bool getKthIntCell(Node* node, int kth, IntCell *cell);
bool removeKthLeafCell(Node *node, int kth);
bool insertIntCell(Node* node, Key k, Pgno child);
bool spaceCheck(Node* node, int size);
void spaceCleanUp(Node* node);

void read(void *from, int off_set, int size, void *to);
void write(void* from, void *to, int off_set, int size);



// API
Tree* getTree(Pager *pager)
{
	return getLeafNode();
}

Node* getLeafNode()
{
	return 0;
}

bool search(Node* node, Key k, void *&p_data, int &size_data)
{
	if(node->is_leaf)
	{
		struct LeafCell cell;
		for(int i=0; i<node->cell_size; i++)
		{
			if(getKthLeafCell(node, i, &cell))
			{
				if(cell.key==k)
				{
					p_data=cell.data_point;
					size_data==cell.size;
					return 1;
				}
			}
		}
		
		size_data=0;
		p_data=(void*)0;
		return 0;
	}
	else
	{
		struct IntCell cell;
		for(int i=0; i<node->cell_size; ++i)
		{
			if(getKthIntCell(node, i, &cell))
			{
				if(k>cell.key)
					continue;
				else
					return search(cell.child, k, p_data, size_data);
			}
		}
		return search(node->right_child, k, p_data, size_data);
	}
}

bool insert(Tree* &root, Key k, void *p_data, int size_data)
{
	Node *p=0;
	if(!insert2(root, k, p_data, size_data, p))
		return 0;
	
	if(p) // 说明插入k导致了节点分裂，导致树的高度增加
		root=p;
	return 1;
}


// 内部函数

bool insert2(Node* node, Key k, void *p_data, int size_data, Node* &father)
{
	if(node->is_leaf)
	{
		if(!insertLeafCell(node, k, p_data, size_data)) // 重复插入
			return 0;
		if(node->cell_size>=Node::THE_ORDER)
			splitLeafNode(node, father);
		return 1;
	}
	else
	{
		struct IntCell cell;
		for(int kth=0; kth<node->cell_size; ++kth)
		{
			getKthIntCell(node, kth, &cell);
			if(k>cell.key)
				continue;
			if(!insert2(cell.child, k, p_data, size_data, node))
				return 0;
			goto INSERTED;
		}
		
		if(!insert2(node->right_child, k, p_data, size_data, node))
			return 0;
INSERTED:
		if(node->cell_size>=THE_ORDER)
			splitIntNode(node, father);
		return 1;
	}
}

bool insertLeafCell(Node *node, Key k, void *p_data, int size_data)
{
	assert(node  &&  p_data);
	assert(node->is_leaf==1);
	if(!spaceCheck(node, size_data+8))
		return 0;
	
	u8 *bp=(u8*)node;
	struct LeafCell cell;
	int kth=0;
	while(kth<node->cell_size)
	{
		getKthLeafCell(node, kth, &cell);
		if(cell.key==k)
			return 0;
		if(cell.key>k) // 找到第kth个cell，其键大于新插入k
			break;
		++kth;
	}
	
	for(int i=node->cell_size-1; i>=kth; --i)
	{
		u8 *p=bp+HEADER_SIZE+i*2;
		memcpy(p+2, p, 2);
	}
	node->first_cell_pos -= size_data+8;
	write(&k, bp, node->first_cell_pos, sizeof(Key));
	write(&size_data, bp, node->first_cell_pos+4, 4);
	memcpy((u8*)node+node->first_cell_pos+8, p_data, size_data);
	write(&node->first_cell_pos, bp, HEADER_SIZE+kth*2, 2);
	++node->cell_size;
	return 1;
}

void splitLeafNode(Node *node, Node* &father)
{
	assert(node);
	assert(node->is_leaf==1);
	assert(node->cell_size>=Node::THE_ORDER);
	
	struct LeafCell cell;
	Node *new_node=getLeafNode();
	while(node->cell_size>Node::THE_ORDER/2)
	{
		int kth=Node::THE_ORDER/2;
		getKthLeafCell(node, kth, &cell);
		insertLeafCell(new_node, cell.key, cell.data_point, cell.size);
		removeKthLeafCell(node, kth);
	}
	
	// 获得分裂后node的最大cell
	getKthLeafCell(node, node->cell_size-1, &cell);
	if(father)
	{
		int kth=getKnumByChild(father, node):
		if(kth==-1)
		{
			assert(node==father->right_child);
			
			insertIntCell(father, cell.key, node);
			father->right_child=new_node;
		}
		else
		{
			struct IntCell intcell;
			// intcell是分裂后new_node的最大cell
			getKthIntCell(father, kth, &intcell);
			removeKthLeafCell(father, kth);
			
			insertIntCell(father, intcell.key, new_node);
			insertIntCell(father, cell.key, node);
		}
	}
	else
	{
		father=getIntNode();
		
		father->right_child=new_node;
		insertIntCell(father, cell.key, node);
	}
}

void splitIntNode(Node *node, Node* &father)
{
	assert(node);
	assert(node->is_leaf==0);
	assert(node->cell_size>=Node::THE_ORDER);
	
	struct IntCell cell;
	Node *new_node=getIntNode();
	while(node->cell_size > Node::THE_ORDER/2+1)
	{
		int kth=Node::THE_ORDER/2+1;
		getKthIntCell(node, kth, &cell);
		insertIntCell(new_node, cell.key, cell.child_point);
		removeKthIntCell(node, kth);
	}
	new_node->right_child=node->right_child;
	
	// 获得分裂后node节点的最大cell, 移做right_child
	getKthIntCell(node, Node::THE_ORDER/2, &cell);
	removeKthIntCell(node, Node::THE_ORDER/2);
	node->right_child=cell.child_point;
	
	if(father)
	{
		int kth=getKnumByChild(father, node);
		if(kth==-1)
		{
			assert(node==father->right_child);
			// 
			insertIntCell(father, cell.key, node);
			father->right_child=new_node;
		}
		else
		{
			struct IntCell intcell;
			getKthIntCell(father, kth, &intcell);
			removeKthIntCell(father, kth);
			// 分裂前node的最大cell, 成为分裂后new_node的最大cell
			insertIntCell(father, intcell.key, new_node);
			// 分裂后node的最大cell, 一定满足小于"移做right_child的cell的key"
			insertIntCell(father, cell.key, node);
		}
	}
	else
	{
		father=getIntNode();
		
		father->right_child=new_node;
		// 分裂后node的最大cell, 一定满足小于"移做right_child的cell的key"
		insertIntCell(father, cell.key, node);
	}
}

bool getKthLeafCell(Node* node, int kth, LeafCell *cell)
{
	assert(node  &&  cell);
	assert(node->is_leaf==1);
	assert(kth>=0  &&  kth<node->cell_size);
	
	u8* bp=(u8*)node;
	int p_offset=HEADER_SIZE;
	p_offset+=kth*2;
	
	int cell_offset;
	// 读取cell指针的内容，即cell的地址
	read(node, p_offset, 2, &cell_offset);
	
	// 依次读取cell内容：key， data长度，data
	read(node, cell_offset, 4, &cell->key);
	read(node, cell_offset+4, 4, &cell->size);
	cell->data_point=bp+cell_offset+8; // 只需要data的指针
	
	return 1;
}

bool getKthIntCell(Node* node, int kth, IntCell *cell)
{
	assert(node  &&  cell);
	assert(node->is_leaf==0);
	assert(kth>=0  &&  kth<node->cell_size);
	
	int p_offset=HEADER_SIZE;
	p_offset+=kth*2;
	
	u16 cell_offset;
	read(node, p_offset, 2, &cell_offset);
	
	read(node, cell_offset, 4, &cell->key);
	read(node, cell_offset+4, 4, &cell->child);
	return true;
}

bool removeKthLeafCell(Node *node, int kth)
{
	assert(node);
	assert(node->is_leaf==1);
	assert(kth>=0  &&  kth<node->cell_size);
	
	struct LeafCell cell;
	getKthLeafCell(node, kth, &cell);
	
	u8 *bp=(u8*)node;
	for(int i=kth; i<node->cell_size; ++i)
	{
		u8 *p=bp+HEADER_SIZE+i*2;
		memcpy(p, p+2, 2);
	}
	--node->cell_size;
	node->frag_size+=8+cell.size;
	return 1;
}

// 向内部节点中插入一个cell，指向最大关键字为k的儿子节点页面child
bool insertIntCell(Node* node, Key k, Pgno child)
{
	assert(node);
	assert(node->is_leaf==0);
	if(!spaceCheck(node, 8))
		return 0;
	
	u8 *bp=(u8*)node;
	struct IntCell cell;
	int kth=0;
	while(kth<node->cell_size)
	{
		getKthIntCell(node, kth, &cell);
		if(cell.key>k)
			break;
		kth++;
	}
	
	for(int i=node->cell_size-1; i>=kth; --i)
	{
		u8 *p=bp+HEADER_SIZE+i*2;
		memcpy(p+2, p, 2);
	}
	
	node->first_cell_pos-=8;
	write(&k, bp, node->first_cell_pos, 4);
	write(&child, bp, node->first_cell_pos+4, 4);
	write(&node->first_cell_pos, bp, HEADER_SIZE+kth*2, 2);
	
	++node->cell_size;
	return 1;
}

bool spaceCheck(Node* node, int size)
{
	if(node->frag_size>(1<<6))
	{
		spaceCleanUp(node);
	}
	
	if(node->first_cell_pos-HEADER_SIZE-node->cell_size*2 < size)
	{
		printf("%d\n", node->is_leaf);
		printf("%d\n", node->cell_size);
		printf("%d\n", node->first_cell_pos);
		puts("size error");
		exit(0);
	}
	return 1;
}

void spaceCleanUp(Node* node)
{
	static Node* leafnode=getLeafNode();
	static Node* intnode=getIntNode();
	static Node* temp_node=getIntNode();
	static LeafCell leafcell;
	static IntCell intcell;
	
	if(node->is_leaf)
	{
		// 对于叶子结点：将原始节点中的cell重新插入一个空白节点
		memcpy(temp_node, leafnode, PAGE_SIZE);
		for(int i=0; i<node->cell_size; i++)
		{
			getKthLeafCell(node, i, &leafcell);
			insertLeafCell(temp_node, leafcell.key, leafcell.data_point, leafcell.size);
		}
	}
	else
	{
		// 对于内部节点，除了cell重新插入，还要移动right_child给新节点
		memcpy(temp_node, intnode, PAGE_SIZE);
		for(int i=0; i<node->cell_size; ++i)
		{
			getKthIntCell(node, i, &intcell);
			insertIntCell(temp_node, intcell.key, intcell.child_point);
		}
		temp_node->right_child=node->right_child;
	}
	memcpy(node, temp_node, Node::PAGE_SIZE);
}


// helper function
// 从相对from偏移off_set的地址处读取size字节，到to中
void read(void *from, int off_set, int size, void *to)
{
	memcpy(to, form+off_set, size);
}

// 将from开始的size字节，写到相对to偏移位置off_set的内存中
void write(void* from, void *to, int off_set, int size)
{
	memcpy(to+off_set, from, size);
}



int main()
{
	return 0;
}

