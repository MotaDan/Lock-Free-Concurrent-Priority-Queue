#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

union Link
{
	long long dword;
	//<p,d>: <pointer to Node, boolean>
	struct
	{
		struct Node* p;
		bool d;
	};
};
	
union VLink
{
	long long dword;
	//<p,d>: <pointer to Value, boolean>
	struct
	{
		void* p;	// Pointer to value
		bool d;
	};
};
		
struct Node
{
	int key, level, validLevel;
	union VLink value;
	union Link* next;
	struct Node* prev;
};

// Allocates a new node from the memory pool and returns a corresponsing pointer with a reference 
// count of one.
struct Node* MALLOC_NODE();
// Atomically de-references the given link and increases the reference counter for the corresponding 
// node. In case the deletion mark of the link is set returns NULL.
struct Node* READ_NODE(union Link* address);
// Increases the reference counter for the corresponding given node.
struct Node* COPY_NODE(struct Node* node);
// Decrements the reference counter on the corresponding given node. If the reference counter reaches 
// zero, the function will recursively call RELEASSE_NDOE on the nodes that this node has owned 
// pointers to (i.e. the prev pointer), and then it reclaims the node.
void RELEASE_NODE(struct Node* node);
struct Node* HelpDelete(struct Node* node, int level);
struct Node* CreateNode(int level, int key, void* value);
struct Node* ReadNext(struct Node** node1, int level);
struct Node* ScanKey(struct Node** node1, int level, int key);
bool Insert(int key, void* value);