//#include "lock-free_priority_queue.h"
#include "lock-free_priority_queue_test.h"

int maxLevel = 1;	// I have arbitrarily choosen 3. Idk how it should be set. maxLevel is used in the psuedo code

// Global variable
struct Node *head, *tail;

// Local variables (for all functions/procedures)
//struct Node *newNode, *savedNodes[maxLevel];
//struct Node *node1, *node2, *prev, *last;
//int i, level;

struct Node* CreateNode(int level, int key, void* value)
{printf("create node\n");
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
{printf("read next\n");
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
{printf("scankey\n");
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
{printf("insert\n");
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	int i;

	// Choose level randomly according to the skip list distribution
	// OBVIOUSLY NOT RANDOM I DIDN'T KNOW WHAT TO DO
	int level = 1;
	
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
		
		if(node2->value.d == false && node2->key == key)
		{
			   if(__sync_bool_compare_and_swap((long long*)&(node2->value), *(long long*)&node2->value, *(long long*)value))
			   {
				   RELEASE_NODE(node1);
				   RELEASE_NODE(node2);
				   for(i = 1; i <= level-1; i++)
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
		
		newNode->next[0] = node1->next[0];
		
		RELEASE_NODE(node2);
		
		union Link newNode_link;
		newNode_link.p = newNode;
		newNode_link.d = false;
		
		if(__sync_bool_compare_and_swap((long long*)&node1->next[0], *(long long*)&node1->next[0], *(long long*)&newNode_link))
		{
			RELEASE_NODE(node1);
			break;
		}
		// Back-Off
	}
	
	for(i = 1; i <= level-1; i++)
	{
		newNode->validLevel = i;
		node1 = savedNodes[i];
		
		while(true)
		{
			node2 = ScanKey(&node1, i, key);
			newNode->next[i] = node1->next[i];
			
			RELEASE_NODE(node2);
			
			if(newNode->value.d == true || __sync_bool_compare_and_swap((long long*)&node1->next[i], *(long long*)&node1->next[i], *(long long*)&newNode))
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

void RemoveNode(struct Node *node, struct Node **prev, int level)
{printf("remove node\n");
	// Local variables (for all functions/procedures)
	struct Node *last;
	
	union Link empty_link;
	empty_link.p = NULL;
	empty_link.d = true;
	
	while(true)
	{
		if(node->next[level].p == empty_link.p && node->next[level].d == true)
			break;
		
		last = ScanKey(prev, level, node->key);
		RELEASE_NODE(last);
		
		if(last != node || (node->next[level].p == empty_link.p && node->next[level].d == true))
			break;
		
		union Link new_link;
		new_link.p = node->next[level].p;
		new_link.d = false;
		if(__sync_bool_compare_and_swap((long long*)&(*prev)->next[level], *(long long*)&(*prev)->next[level], *(long long*)&new_link))
		{
			node->next[level] = empty_link;
			break;
		}
		
		if(node->next[level].p == empty_link.p && node->next[level].d == empty_link.d)
			break;
		
		//Back-Off
	}
}

void* DeleteMin()
{printf("delete min\n");
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	int i;
	
	prev = COPY_NODE(head);
	
	while(true)
	{
		// Checking if the head is the same as the tail
		node1 = ReadNext(&prev,0);
		if(node1 == tail)
		{
			RELEASE_NODE(prev);
			RELEASE_NODE(node1);
			return NULL;
		}
retry:
		if(node1 != prev->next[0].p)
		{
			RELEASE_NODE(node1);
			continue;
		}
		
		if(node1->value.d == false)
		{
			union VLink new_value;
			new_value.p = node1->value.p;
			new_value.d = true;
			if(__sync_bool_compare_and_swap((long long*)&node1->value, *(long long*)&node1->value, *(long long*)&new_value))
			{
				node1->prev = prev;
				break;
			}
			else
				goto retry;
		}
		else if(node1->value.d == true)
			node1 = HelpDelete(node1, 0);
		RELEASE_NODE(prev);
		prev = node1;
	}
	
	for(i = 0; i <= node1->level-1; i++)
	{
		union Link new_link;
		new_link.d = true;
		 do
		 {
			 new_link = node1->next[i];
			 // Until d = true or CAS
		 } while((node1->next[i].d != true) && !(__sync_bool_compare_and_swap((long long*)&node1->next[i], *(long long*)&node1->next[i], *(long long*)&new_link)));
	}
	
	prev = COPY_NODE(head);
	for(i = node1->level-1; i >= 0; i--)
		RemoveNode(node1, &prev, i);
	
	union VLink value = node1->value;
	
	RELEASE_NODE(prev);
	RELEASE_NODE(node1);
	RELEASE_NODE(node1); /*Delete the node*/
		
	return value.p;
}

struct Node* HelpDelete(struct Node* node, int level)
{printf("help delete\n");
	// Local variables (for all functions/procedures)
	struct Node *node2, *prev;
	int i;
	
	union Link new_link;
	new_link.d = true;
	
	for(i = level; i <= node->level-1; i++)
	{
		do
		{
			new_link.p = node->next[i].p;
		} while(node->next[i].d != true && !(__sync_bool_compare_and_swap((long long*)&node->next[i], *(long long*)&node->next[i], *(long long*)&new_link)));
	}
	
	prev = node->prev;
	
	if(!prev || level >= prev->validLevel)
	{
		prev = COPY_NODE(head);
		
		for(i = maxLevel-1; i >= level; i--)
		{
			node2 = ScanKey(&prev, i, node->key);
			RELEASE_NODE(node2);
		}
	}
	else
		COPY_NODE(prev);
	
	RemoveNode(node, &prev, level);
	RELEASE_NODE(node);
	
	return prev;
}

int main(void)
{
	int i, val0 = 0, val1 = 1, val2 = 2, val3 = 3;
	// struct Node* node = CreateNode(1, 1, &val1);
	// struct Node* node2 = CreateNode(1, 4, &val2);
	// struct Node* node3 = CreateNode(1, 6, &val3);
	
	head = CreateNode(1, -1000000, &val0);
	tail = CreateNode(1, 1000000, &val0);
	head->next[0].p = tail;
	head->next[0].d = false;
	
	// node->next[0].p = node2;
	// node2->next[0].p = node3;
	
	// printf("Create Node Test\n");
	// PrintNodeInfo(node);
	
	// printf("ReadNext Test\n");
	// PrintNodeInfo(node);
	// PrintNodeInfo(ReadNext(&node, 0));
	
	// printf("ScanKey Test\n");
	// PrintNodeInfo(ScanKey(&node, 0, 2));
	
	// printf("RemoveNode Test\n");
	// PrintNodeInfo(node2);
	// RemoveNode(node3, &node2, 0);
	// PrintNodeInfo(node2);
	// PrintNodeInfo(node3);
	
	// printf("HelpDelete Test\n");
	// PrintNodeInfo(HelpDelete(node2, 0));
	// PrintNodeInfo(node2);
	
	// printf("DeleteMin Test\n");
	// printf("Minimum value %d\n", *(int*)DeleteMin());
	
	Insert(5, &val1);
	Insert(2, &val2);
	Insert(50, &val3);
	
	PrintList(head);
	printf("Minimum value %d\n", *(int*)DeleteMin());
	PrintList(head);
	
	return 0;
}

//INCORRECT IMPLEMENTATIONS OF MEMORY MANAGEMENT
struct Node* MALLOC_NODE()
{
	return malloc(sizeof(struct Node));
}

struct Node* READ_NODE(union Link* address)
{
	if(address->d)
		return NULL;
	
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