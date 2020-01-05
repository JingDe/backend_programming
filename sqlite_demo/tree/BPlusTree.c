
/*
B+树节点的结构：
	头部
	cell指针数组,每个指针占2字节，表示第几个cell的偏移位置
	未分配区域
	cell内容区域
	
头部：
	。。。
	
叶子节点的cell结构：
	4字节：	key
	4字节：	数据的字节数
	？	：	数据

内部节点的cell结构：
	4字节：	key
	4字节：	指向儿子节点的指针
*/

/*头部大小计算的辅助结构体。
由于内存对齐，不能直接相加字节数来计算。*/
struct FOR_CALCULATE_HEADER_SIZE{
	byte is_leaf; // 0表示内部节点，1表示叶节点
	short cell_size; // 此节点中的cell的个数
	short first_cell_pos; // cell区域的首字节 ？地址？偏移
	byte frag_size; // 空闲的碎片空间的字节大小
	Node *right_child; // 右儿子节点，叶子节点忽略
};
const int HEADER_SIZE=sizeof(FOR_CALCULATE_HEADER_SIZE);



/* B+树节点：
一个节点对应一个页面。

*/
struct Node{
	static const int THE_ORDER=10; //  branching因子, 叶子节点的个数限制
	static const int PAGE_SIZE=1448; // 每个节点的大小,页面大小
	
	// 节点头部区域
	byte is_leaf;
	short cell_size; // TODO
	short first_cell_pos; // TODO
	byte frag_size; // TODO
	Node *right_child;
	
	byte content[PAGE_SIZE-HEADER_SIZE];
} *root=0;


/*
cell结构：
*/
struct IntCell{
	Key key; // 4字节key
	Node* child_point; // 儿子节点指针
};

struct LeafCell{
	Key key; // 4个字节
	int size; // 数据长度
	byte *data_point; // 数据
};


Tree* getTree()
{
	return getLeafNode();
}

// 创建一个叶子节点
Node* getLeafNode()
{
	Node *p=new Node();
	p->is_leaf=1; // 叶子节点
	p->cell_size=0；// 初始cell个数为0
	p->frag_sizse=0; // 空闲碎片空间大小为0
	p->right_child=0;
	p->first_cell_pos=Node::PAGE_SIZE;
	return p;
}

// 创建一个内部节点
Node* getIntNode()
{
	Node *p=new Node();
	p->is_leaf=0;
	p->cell_size=0;
	p->frag_size=0;
	p->right_child=0;
	p->first_cell_pos=Node::PAGE_SIZE;
	return p;
}

bool search(Node* node, Key k, void* &p_data, int &size_data)
{
	// 在叶子节点中搜索关键字k
	if(node->is_leaf)
	{
		struct LeafCell cell;
		for(int i=0; i<node->cell_size; ++i)
		{
			if(getKthLeafCell(node, i, &cell))
			{
				if(cell.key==k)
				{
					p_data=cell.data_point;
					size_data=cell.size;
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
			// 遍历内部节点的每个cell
			if(getKthIntCell(node, i, &cell))
			{
				// 忽略小于查找key的cell，直到找到第一个大于等于key的cell
				if(k>cell.key)
					continue;
				else
					// 在此cell指向的儿子节点中继续查找
					return search(cell.child_point, k, p_data， size_data);
			}
		}
		
		// ?? TODO 在右儿子节点中继续查找
		return search(node->right_child, k, p_data, size_data);
	}
}

bool insert(Tree *&root, Key k, void *p_data, int size_data)
{
	Node *p=0;
	if(!insert2(root, k, p_data, size_data, p))
		return 0;
	if(p)
		root=p;
	return 1;
}

bool insert2(Node* node, Key k, void *p_data, int size_data, Node *&father)
{
	if(node->is_leaf) // 在叶子节点中插入kv对
	{
		if(!insertLeafCell(node, k, p_data, size_data))
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
			
		}
	}
}

// 向叶子节点node中插入键值对
bool insertLeafCell(Node* node, Key k, void *p_data, int size_data)
{
	assert(node  &&  p_data);
	assert(node->is_leaf==1);
	if(!spaceCheck(node, size_data+8))
		return 0;
	
	byte *bp=(byte*)node;
	struct LeafCell cell;
	int kth=0;
	while(kth<node->cell_size)
	{
		getKthLeafCell(node, kth, &cell);
		if(cell.key==k)
			return 0;
		if(cell.key>k) // 找到k的插入位置
			break;
		++kth;
	}
	// 将kth往后的cell指针向后移动一位
	for(int i=node->cell_size-1; i>=kth; --i)
	{
		byte *p=bp+HEADER_SIZE+i*2;
		memcpy(p+2; p, 2);
	}
	node->first_cell_pos-=size_data+8; // 插入的cell大小（4字节Key+4字节data长度size_data+size_data长度data）
	int2Bytes(bp, node->frist_cell_pos, sizeof(Key), k);
	int2Bytes(bp, node->first_cell_pos+4, 4, size_data);
	memcpy((byte*)node+node->first_cell_pos+8, p_data, size_data);
	
	int2Bytes(bp, HEADER_SIZE+kth*2, 2, node->first_cell_pos);
	++(node->cell_size);
	return 1;
}


bool insertIntCell(Node *node, Key k, Node *child)
{
	assert(node  &&  child);
	assert(node->is_leaf==0);
	if(!spaceCheck(node, 8))
		return 0;
	
	byte *bp=(byte*)node;
	struct IntCell cell;
	int kth=0;
	while(kth < node->cell_size)
	{
		getKthIntCell(node, kth, &cell);
		if(cell.key>k)
			break;
		kth++;
	}
	for(int i=node->cell_size-1; i>=kth; --i)
	{
		byte *p=bp+HEADER_SIZE+i*2;
		memcpy(p+2, p, 2);
	}
	
	// 写入cell内容
	node->first_cell_pos-=8;
	int2Bytes(bp, node->first_cell_pos, 4, k);
	int2Bytes(bp, node->first_cell_pos+4, 4, (int)child);
	// 写入cell指针
	int2Bytes(bp, HEADER_SIZE+kth*2, 2, node->first_cell_pos);
	++node->cell_size;
	return 1;
}

/* 分裂节点node， 
*/
void splitLeafNode(Node* node, Node* &father)
{
	assert(node);
	assert(node->is_leaf==1);
	assert(node->cell_size >= Node::THE_ORDER);
	
	struct LeafCell cell;
	Node* new_node=getLeafNode();
	// 从左往后删除node的cell，并插入到new_node中
	while(node->cell_size>Node::THE_ORDER/2)
	{
		int kth=Node::THE_ORDER/2;
		getKthLeafCell(node, kth, &cell);
		insertLeafCell(new_node, cell.key, cell.data_point， cell.size);
		removeKthLeafCell(node, kth);
	}
	
	// cell_size等于THE_ORDER
	//获得node的最大key对应的cell
	getKthLeafCell(node, node->cell_size-1, &cell);
	
	if(father)
	{
		int kth=getKnumByChild(father, node);
		if(kth==-1) // 说明node是father的right_child
		{
			assert(node==father->right_child);
			// 将node插入到father的cell数组中，而将新节点作为right_child
			insertIntCell(father, cell.key, node);
			father->right_child=new_node;
		}
		else // 说明node时father的第kth个cell
		{
			// 先将node从cell数组中删除，因为node的最大key变化了
			struct IntCell intcell;
			getKthIntCell(father, kth, &intcell);
			removeKthIntCell(father, kth);
			// 再重新插入两个儿子节点
			insertIntCell(father, intcell.key, new_node);
			insertIntCell(father, cell.key, node);
		}
	}
	else
	{
		// 创建父节点
		father=getIntNode();
		father->right_child=new_node; // 父节点的右儿子节点指向分裂处的新节点
		insertIntCell(father, cell.key, node); // 父节点中插入一个cell，键是node的cell中的最大key，值是node节点
	}
}


/* 检查child是否是father的一个子节点，不是返回-1，是则返回是第几个子节点。 */
int getKnumByChild(Node *node, Node *child)
{
	assert(node  &&  childe);
	assert(node->is_leaf==0);
	
	struct IntCell cell;
	for(int i=0; i<node->cell_size; ++i)
	{
		getKthIntCell(node, i, &cell);
		if(cell.child_point==child)
			return i;
	}
	return -1;
}

// 删除叶子节点node的第kth个cell
bool removeKthLeafCell(Node* node, int kth)
{
	assert(node);
	assert(node->is_leaf==1);
	assert(kth>=0  &&  kth<node->cell_size);
	
	struct LeafCell cell;
	getKthLeafCell(node, kth, &cell);
	
	byte *bp=(byte*)node;
	for(int i=kth; i<node->cell_size; ++i)
	{
		byte *p=bp+HEADER_SIZE+i*2;
		memcpy(p, p+2, 2);
	}
	--node->cell_size;
	node->frag_size+=8+cell.size;
	return 1;
}

bool removeKthIntCell(Node *node, int kth)
{
	assert(node);
	assert(node->is_leaf==0);
	assert(kth>=0  &&  kth<node->cell_size);
	
	byte *bp=(byte*)node;
	for(int i=kth; i<node->cell_size; ++i)
	{
		byte *p=bp+HEADER_SIZE+i*2;
		memcpy(p, p+2; 2);
	}
	--node->cell_size;
	node->frag_size+=sizeof(IntCell);
	return 1;
}

// 获得某个叶子节点的第k个cell
bool getKthLeafCell(Node* node, int kth, LeafCell* cell)
{
	assert(node  &&  cell);
	assert(node->is_leaf==1);
	assert(kth>=0  &&  kth<node->cell_size);
	
	byte *bp=(byte*)node;
	int p_offset=HEADER_SIZE;
	p_offset+=kth*2; // p_offset 等于第k个cell指针在节点node中的偏移位置
	
	// 读取第k个cell指针的内容，即第k个cell的偏移位置
	int cell_offset=bytes2Int(node, p_offset, 2);
	// 读取第k个cell的前4个字节，是key
	cell->key=bytes2Int(node, cell_offset, 4);
	cell->size=bytes2Int(node, cell_offset+4, 4);
	/*cell的数据存储在紧接在key,数据大小size之后的内存中，所以设置数据地址data_point*/
	cell->data_point=bp+cell_offset+8;
	return 1;
}

// 检查节点是否有足够空间存储size字节
bool spaceCheck(Node *node, int size)
{
	// 当空闲碎片空间大小超过1<<6，执行回收
	if(node->frag_size>(1<<6))
		spaceCleanUp(node);
	
	// 空闲空间 小于 size
	if(node->first_cell_pos-HEADER_SIZE-node->cell_size*2<size)
	{
		printf("%d\n", node->is_leaf);
		printf("%d\n", node->cell_size);
		printf("%d\n", node->first_cell_pos);
		puts("size error");
		exit(0);
	}
	return 1;
}

// 清理节点的空闲碎片空间
void spaceCleanUp(Node* node)
{
	static Node *leafnode=getdLeafNode();
	static Node *intnode=getIntNode();
	static Node *temp_node=getIntNode();
	static LeafCell leafcell;
	static IntCell intcell;
	
	if(node->is_leaf)
	{
		// 将temp_node初始化为一个空的叶子节点
		memcpy(temp_node, leafnode, Node::PAGE_SIZE);
		for(int i=0; i<node->cell_size; ++i)
		{
			// 获得node的每一个cell
			getKthLeafCell(node, i, &leafcell);
			// 将每一个cell重新插入到temp_node节点中（此处调用insertLeafCell的时候，spaceCheck大概不需要调用spaceCleanUp）
			insertLeafCell(temp_node, leafcell.key, leafcell.data_point, leafcell.size);
		}
	}
	else
	{
		
	}
	
	memcpy(node, temp_node, Node::PAGE_SIZE);
}

/* 读取相对于地址p偏移量为offset的地址处size大小的内容，int形式返回*/
int bytes2Int(void* f, int offset, int size)
{
	assert(f);
	assert(size==1  ||  size==2  ||  size==4);
	
	byte *by=(byte*)f;
	if(size==1)
	{
		byte bx;
		memcpy(&bx, bf+offset, 1);
		return (int)bx;
	}
	else if(size==2)
	{
		short sx;
		memcpy(&sx, bf+offset, 2);
		return (int)sx;
	}
	else if(size==4)
	{
		int ix;
		memcpy(&ix, bf+offset, 4);
		return ix;
	}
}

/* 在相对地址t偏移位置offset处的size字节内存中写入整数number
number可以是byte/short/int类型 
*/
void int2Bytes(void *t, int offset, int size, int number)
{
	assert(t);
	assert(size==1  ||  size==2  ||  size==4);
	
	byte *bt=(byte*)t;
	if(size==1)
	{
		byte bx=(byte)number;
		memcpy(bt+offset, &bx, 1);
	}
	else if(size==2)
	{
		short sx(short)number;
		memcpy(bt+offset, &sx, 2);
	}
	else if(size==4)
	{
		memcpy(bt+offset, &number, 4);
	}
}

void printfNode(Node* node)
{
	for(int i=0; i<node->cell_size; ++i)
	{
		struct IntCell intcell;
		struct LeafCell leafcell;
		
		if(node->is_leaf)
		{
			getKthLeafCell(node, i, &leafcell);
			printf("%d\n", leafcell.key);
		}
		else
		{
			getKthIntCell(node, i, &intcell);
			printf("%d\n", intcell.key);
		}
	}
	puts(""); // 输出换行符
}

void printfTree(Node* node, int indent)
{
	for(int i=0; i<node->cell_size; ++i)
	{
		struct IntCell intcell;
		struct LeafCell leafcell;
		
		if(node->is_leaf)
		{
			getKthLeafCell(node, i, &leafcell);
			for(int j=0; j<indent; ++j)
				printf(" ");
			printf("%d\n", leafcell.key);
		}
		else
		{
			getKthIntCell(node, i, &intcell);
			// 打印第i个儿子节点紫薯
			printfTree(intcell.child_point, indent+2);
			for(int j=0; j<dent; ++j)
				printf(" ");
			printf("%d\n", intcell.key);
		}
	}
	
	if(node->is_leaf==0)
	{
		if(node->right_child)
		{
			printfTree(node->right_child, indent+2);
		}
		else
		{
			for(int j=0; j<indent+2; ++j)
				printf(" ");
			puts("null");
		}
	}
}

Node* getKthChild(Node *node, int kth)
{
	if(kth==node->cell_size)
		return node->right_child;
	
	struct IntCell intcell;
	getKthIntCell(node, kth, &intcell);
	return intcell.child_point;
}