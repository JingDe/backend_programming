#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_
// 私有的拷贝构造和拷贝赋值，阻止noncopyable被拷贝
// 所有继承noncopyable的类，由于派生类不能访问noncopyable的拷贝构造和拷贝赋值，
// 编译器不能自动生成拷贝构造和拷贝赋值


class noncopyable
{
protected: // 保护的构造和析构函数，派生类可以访问，可以构造和析构
	noncopyable() {}
	~noncopyable() {}
	
private:
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
};


// 基类的public和protected成员，派生类可以调用
// 基类的private成员，派生类不能调用

// 以上对三种继承方式都一样


#endif