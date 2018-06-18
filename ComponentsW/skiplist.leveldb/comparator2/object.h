#ifndef OBJECT_H_
#define OBJECT_H_

class Object {
public:
	explicit Object(int a) :x_(a) {}
	int Data() const { return x_;  }

private:
	int x_;
};



#endif
