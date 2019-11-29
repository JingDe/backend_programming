

typedef struct zset{
	dict *dict;
	zskiplist *zsl;
}zset;