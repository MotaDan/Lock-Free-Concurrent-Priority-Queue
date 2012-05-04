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

void RemoveNode(struct Node *node, struct Node **prev, int level)
{
	// Local variables (for all functions/procedures)
	struct Node *last;
	
	while(true)
	{
		union Link empty;
		empty.p = NULL;
		empty.d = true;
		
		if(node->next[level].p == empty.p && node->next[level].d == true)
			break;
		
		last = ScanKey(prev, level, node->key);
		RELEASE_NODE(last);
		
		if(last != node || (node->next[level].p == empty.p && node->next[level].d == true))
			break;
		
		union Link prev_link = (*prev)->next[level];
		union Link new_link;
		new_link.p = node->next[level].p;
		new_link.d = false;
		if(__sync_bool_compare_and_swap((long long*)&(*prev)->next[level], (long long*)&prev_link, (long long*)&new_link))
		{
			node->next[level] = empty;
			break;
		}
		
		if(node->next[level].p == empty.p && node->next[level].d == empty.d)
			break;
		
		//Back-Off
	}
}

struct Node* DeleteMin()
{
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	int i;
	
	prev = COPY_NODE(head);
	
	while(true)
	{
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
		union VLink test_value= node1->value;
		
		if(test_value.d = false)
		{
			union VLink new_value;
			new_value.p = test_value.p;
			new_value.d = true;
			if(__sync_bool_compare_and_swap((long long*)&node1->value, (long long*)&test_value, (long long*)&new_value))
			{
				node1->prev = prev;
				break;
			}
			else
				goto retry;
		}
		else if(test_value.d == true)
			node1 = HelpDelete(node1, 0);
		RELEASE_NODE(prev);
		prev = node1;
	}
	
	for(i = 0; i < node1->level-1; i++)
	{
		union Link old_link;
		union Link new_link;
		 do
		 {
			 old_link = node1->next[i];
			 new_link = old_link;
			 new_link.d = true;
			 // Until d = true or CAS
		 } while((old_link.d != true) && !(__sync_bool_compare_and_swap((long long*)&node1->next[i], (long long*)&old_link, (long long*)&new_link)));
	}
	
	prev = COPY_NODE(head);
	for(i = node1->level-1; i > 0; i--)
		RemoveNode(node1, &prev, i);
	
	RELEASE_NODE(prev);
	RELEASE_NODE(node1);
	RELEASE_NODE(node1); /*Delete the node*/
	return (struct Node*)node1->value.p;
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