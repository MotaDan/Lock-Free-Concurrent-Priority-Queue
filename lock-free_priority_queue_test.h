#include "lock-free_priority_queue.h"

void PrintNodeInfo(struct Node* node)
{
	int i;
	
	if(!node)
	{
		printf("node is NULL\n");
		return;
	}
	
	printf("Pieces of a node\n");
	printf("Key %d, Level %d, Valid Level %d\n", node->key, node->level, node->validLevel);
	printf("VLink value %d, boolean %d\n", *(int*)node->value.p, node->value.d);
	for(i = 0; i < node->level; i++)
		printf("Link pointer %d, boolean %d\n", (int)node->next[i].p, node->next->d);
	printf("Node prev pointer %d\n\n", (int)node->prev);
}


void PrintList(struct Node* node)
{
	struct Node *node2 = node;
	
	printf("\nPrinting List of nodes\n");
	do
	{
		PrintNodeInfo(node2);
		node2 = node2->next[0].p;
	} while(node2 != NULL);
}