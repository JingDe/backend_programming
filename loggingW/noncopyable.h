#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_
// ˽�еĿ�������Ϳ�����ֵ����ֹnoncopyable������
// ���м̳�noncopyable���࣬���������಻�ܷ���noncopyable�Ŀ�������Ϳ�����ֵ��
// �����������Զ����ɿ�������Ϳ�����ֵ


class noncopyable
{
protected: // �����Ĺ����������������������Է��ʣ����Թ��������
	noncopyable() {}
	~noncopyable() {}

private:
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
};


// �����public��protected��Ա����������Ե���
// �����private��Ա�������಻�ܵ���

// ���϶����ּ̳з�ʽ��һ��


#endif