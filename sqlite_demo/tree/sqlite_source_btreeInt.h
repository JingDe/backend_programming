/*
	sqlite源码 btree.c
*/
struct CellInfo{
	i64 nKey; // 
	u8* pPayload; // payload的首地址
	u32 nPayload; // payload的字节数
	u16 nLocal; // 此节点中的存储的部分payload的字节数
	u16 nSize; // 主b树页面的cell内容的字节数
};

/*
	插入相关方法：
		insertCell		
		sqlite3BtreeInsert                                                                                
		//pageInsertArray, btreeHeapInsert
*/

/*
向B树中插入一条记录。

对于一个table btree：

对于一个index btree,用来作为索引和WITHOUT ROWID table:

*/
int sqlite3BtreeInsert(
		BtCursor *pCur, // 插入到此cursor指向的表中
		const BtreePayload *pX, // 待插入的内容
		int flags, // true表示是append方式
		int seekResult) // MovetoUnpacked()返回的结果
{
	
}