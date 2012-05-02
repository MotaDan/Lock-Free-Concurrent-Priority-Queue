#include "lock-free_priority_queue.h"

int maxLevel = 3;	// I have arbitrarily choosen 3. Idk how it should be set. maxLevel is used in the psuedo code

// Global variable
struct Node *head, *tail;

// Local variables (for all functions/procedures)
//struct Node *newNode, *savedNodes[maxLevel];
//struct Node *node1, *node2, *prev, *last;
//int i, level;

struct Node* CreateNode(int level, int key, void* value)
{
	union VLink vlink_value;
	vlink_value.p = value;
	vlink_value.d= false;
	
	struct Node* node = MALLOC_NODE();  //need this for safe pointer handling
	node->prev = NULL;
	node->validLevel = 0;
	node->level = level;
	node->key = key;
	node->next = malloc(sizeof(union Link) * level);
	node->value = vlink_value;
	return node;
}

struct Node* ReadNext(struct Node** node1, int level)
{
	// Local variables (for all functions/procedures)
	struct Node *node2;

	if((*node1)->value.d == true)
		*node1 = HelpDelete(*node1, level);
	
	node2 = READ_NODE(&(*node1)->next[level]);
	
	while(node2 == NULL)
	{
		*node1 = HelpDelete(*node1, level);
		node2 = READ_NODE(&(*node1)->next[level]);
	}
	return node2;
}

struct Node* ScanKey(struct Node** node1, int level, int key)
{
	// Local variables (for all functions/procedures)
	struct Node *node2;

	node2 = ReadNext(node1, level);
	while(node2->key < key)
	{
		RELEASE_NODE(*node1);
		*node1 = node2;
		node2 = ReadNext(node1, level);
	}
	return node2;
}

bool Insert(int key, void* value)
{
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	int i;

	// Choose level randomly according to the skip list distribution
	// OBVIOUSLY NOT RANDOM I DIDN'T KNOW WHAT TO DO
	int level = 3;
	
	newNode = CreateNode(level, key, value);
	COPY_NODE(newNode);
	node1 = COPY_NODE(head);
	
	for(i = maxLevel-1; i >= 1; i--)
	{
		node2 = ScanKey(&node1, 0, key);
		RELEASE_NODE(node2);
		if(i < level)
			savedNodes[i] = COPY_NODE(node1);
	}
	
	while(true)
	{
		node2 = ScanKey(&node1, 0, key);
		union VLink value2 = node2->value;
		
		if(value2.d == false && node2->key == key)
		{
			   if(__sync_bool_compare_and_swap((long long*)&(node2->value), *(long long*)&value2, *(long long*)value))
			   {
				   RELEASE_NODE(node1);
				   RELEASE_NODE(node2);
				   for(i = 1; i < level-1; i++)
					   RELEASE_NODE(savedNodes[i]);
				   RELEASE_NODE(newNode);
				   RELEASE_NODE(newNode);
				   return true;
			   }
			   else
			   {
				   RELEASE_NODE(node2);
				   continue;
			   }
		}
		
		union Link node2_link;
		node2_link.p = node2;
		node2_link.d = false;
		newNode->next[0] = node2_link;
		
		RELEASE_NODE(node2);
		
		union Link newNode_link;
		newNode_link.p = newNode;
		newNode_link.d = false;
		
		if(__sync_bool_compare_and_swap((long long*)&node1->next[0], *(long long*)&node2_link, *(long long*)&newNode_link))
		{
			RELEASE_NODE(node1);
			break;
		}
		// Back-Off
	}
	
	for(i = 1; i < level-1; i++)
	{
		newNode->validLevel = i;
		node1 = savedNodes[i];
		
		while(true)
		{
			node2 = ScanKey(&node1, i, key);
			
			union Link node2_link;
			node2_link.p = node2;
			node2_link.d = false;
			newNode->next[i] = node2_link;
			
			RELEASE_NODE(node2);
			
			if(newNode->value.d == true || __sync_bool_compare_and_swap((long long*)&node1->next[i], *(long long*)&node2, *(long long*)&newNode))
			{
				RELEASE_NODE(node1);
				break;
			}
			// Back-Off
		}
	}
	
	newNode->validLevel = level;
	
	if(newNode->value.d == true)
		newNode = HelpDelete(newNode, 0);
	
	RELEASE_NODE(newNode);
	
	return true;
}

int main(void)
{
	union VLink* value = malloc(sizeof(union VLink));
	struct Node* node = CreateNode(3, 4, value);
	int val = 10;
	if(Insert(5, &val))
		printf("Size of word %d", sizeof(long long*));
	free(node);
	free(value);
	return 0;
}

//INCORRECT IMPLEMENTATIONS OF MEMORY MANAGEMENT
struct Node* MALLOC_NODE()
{
	return malloc(sizeof(struct Node));
}

struct Node* READ_NODE(union Link* address)
{
	return address->p;
}

struct Node* COPY_NODE(struct Node* node)
{
	return node;
}

void RELEASE_NODE(struct Node* node)
{
	return;
}

struct Node* HelpDelete(struct Node* node, int level)
{
	return node;
}