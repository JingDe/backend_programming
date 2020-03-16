#ifndef DBSTREAM_H
#define DBSTREAM_H

#include<stdint.h>
#include <string>
#include <list>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <bitset>
#include <string.h>
//#include "compile.h"
//#include "owlog.h"

using namespace std;

typedef list<string> StrList; //wyb modify

#define MAX_BLOCK_SIZE 4096

class DBOutStream 
{
public:
	DBOutStream()/*:m_logger("clib.dbstream.out") */
	{
		m_capacity = MAX_BLOCK_SIZE;
		m_data = new char[m_capacity];
		memset(m_data, 0, m_capacity);
		m_cursor = m_data;
	}
	virtual ~DBOutStream() {
		delete[] m_data;
	}
	
	char *getData() {
		return m_data;
	}
	int getSize() {
		return (m_cursor - m_data);
	}
	
	inline void save(void *p, int size); 
	
	DBOutStream &operator <<(bool i);
	DBOutStream &operator <<(char i);
	DBOutStream &operator <<(unsigned char i);
	DBOutStream &operator <<(unsigned short i);
	DBOutStream &operator <<(uint64_t i);
	DBOutStream &operator <<(uint32_t i);
	DBOutStream &operator <<(int8_t i);
	DBOutStream &operator <<(int16_t i);
	DBOutStream &operator <<(int32_t i);
	DBOutStream &operator <<(int64_t i);

	DBOutStream &operator <<(const char *str);
	DBOutStream &operator <<(const string &str);
	DBOutStream &operator <<(const StrList &strList);
	
private:
	//char m_data[MAX_BLOCK_SIZE];
	char *m_data;
	char *m_cursor;
	int m_capacity;
//	OWLog	m_logger;

};


class DBInStream 
{
public:
	DBInStream(void *d, int n)/*:m_logger("clib.dbstream.in") */
	{
		m_cursor = m_data = (char *) d;
		m_datalen = n;
		m_loadError = 0;
	}
    virtual  ~DBInStream(){}
	inline void load(void *p, int size);
		
	DBInStream &operator >>(bool &i);
	DBInStream &operator >>(char &i);
	DBInStream &operator >>(unsigned char &i);
	DBInStream &operator >>(unsigned short &i);
	DBInStream &operator >>(uint64_t &i);
	DBInStream &operator >>(uint32_t &i);
	DBInStream &operator >>(int8_t &i);
	DBInStream &operator >>(int16_t &i);
	DBInStream &operator >>(int32_t &i);
	DBInStream &operator >>(int64_t &i);

	DBInStream &operator >>(char *&str);
	DBInStream &operator >>(string &str);
	DBInStream &operator >>(StrList &strList);
	
public:
	int m_loadError;
	
private:
	char *m_data;
	char *m_cursor;
	int m_datalen;
//	OWLog	m_logger;
};


class DBSerialize {
public:
	void save_with_version(DBOutStream &out){
		save(out);
		out << this->m_data_version;
		//printf("out length is %d\n",out.getSize());
	};
	void load_with_version(DBInStream &in){
		load(in);
		in >> this->m_data_version;
		//printf("load version is %ld\n", (uint64_t_t)this->m_data_version);
	};
	virtual void save(DBOutStream &out) const = 0;
	virtual void load(DBInStream &in) = 0;

	uint64_t get_data_version(){
		return m_data_version;
	};

	void set_data_version(uint64_t m_data_version){
		this->m_data_version = m_data_version;
	};

	bool is_support_data_version(){
		return support_data_version ;
	};

	void set_support_data_version(bool supported){
		 support_data_version = supported;
	};

	void clone_content(DBSerialize* value){
		int64_t data_version = this->m_data_version;
		value  = this;
		value->m_data_version = data_version;
	};
    DBSerialize():support_data_version(false){
 		m_data_version = 0;
    };

    virtual ~DBSerialize(){}
protected:
 	int64_t m_data_version;
 	bool  support_data_version;
};

#endif
