#ifndef ENV_H_
#define ENV_H_

class Env {
public:
	static Env* Default();

	virtual void Schedule(void(*function)(void* arg), void* arg) = 0;
};

#endif
