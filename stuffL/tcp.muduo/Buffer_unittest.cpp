#include"Buffer.h"

#include<cassert>

#include"testharness.leveldb/testharness.h"

class BufferTest {};

TEST(BufferTest, tBufferAppendRetrieve)
{
	Buffer buf;
	BOOST_CHECK_EQUAL(buf.readableBytes(), 0);
	BOOST_CHECK_EQUAL(buf.writableBytes(), Buffer::kInitialSize);
	BOOST_CHECK_EQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

	const string str(200, 'x');
	buf.append(str);
	BOOST_CHECK_EQUAL(buf.readableBytes(), str.size());
	BOOST_CHECK_EQUAL(buf.writableBytes(), Buffer::kInitialSize - str.size());
	BOOST_CHECK_EQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

	const string str2 = buf.retrieveAsString(50);
	BOOST_CHECK_EQUAL(str2.size(), 50);
	BOOST_CHECK_EQUAL(buf.readableBytes(), str.size() - str2.size());
	BOOST_CHECK_EQUAL(buf.writableBytes(), Buffer::kInitialSize - str.size());
	BOOST_CHECK_EQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + str2.size());
	BOOST_CHECK_EQUAL(str2, string(50, 'x'));

	buf.append(str);
	BOOST_CHECK_EQUAL(buf.readableBytes(), 2 * str.size() - str2.size());
	BOOST_CHECK_EQUAL(buf.writableBytes(), Buffer::kInitialSize - 2 * str.size());
	BOOST_CHECK_EQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + str2.size());

	const string str3 = buf.retrieveAllAsString();
	BOOST_CHECK_EQUAL(str3.size(), 350);
	BOOST_CHECK_EQUAL(buf.readableBytes(), 0);
	BOOST_CHECK_EQUAL(buf.writableBytes(), Buffer::kInitialSize);
	BOOST_CHECK_EQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);
	BOOST_CHECK_EQUAL(str3, string(350, 'x'));
}

int main()
{
	testBufferAppendRetrieve();
}

