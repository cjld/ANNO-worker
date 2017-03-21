#pragma once

class CmUFSet
{
public:
	CmUFSet(int s);
	~CmUFSet(void);
	void Union(int x1, int x2);
	int Find(int x);

private:
	int *parent; // ¸¸½áµã
	int size;
};
