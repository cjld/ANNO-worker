#include "CmUFSet.h"

CmUFSet::CmUFSet(int s)
{
	size = s;
	parent = new int [size+1];
	for(int i = 0; i <= size; i++)
		parent[i] = -1;
}

CmUFSet::~CmUFSet(void)
{
	delete []parent;
}

void CmUFSet::Union(int x1, int x2)
{
	int Root1 = Find(x1), Root2 = Find(x2);
	if (Root1 == Root2)
		return;
	int temp = parent[Root1] + parent[Root2];
	if(parent[Root1] < parent[Root2]) //Root1 has more nodes
		parent[Root2] = Root1, parent[Root1] = temp;
	else 
		parent[Root1] = Root2, parent[Root2] = temp;
}

int CmUFSet::Find(int x)
{
	int j = x;
	for(; parent[j] >= 0; j = parent[j]); //Find root
	while(x != j){
		int temp = parent[x];
		parent[x] = j;
		x = temp;
	}
	return j;
}
