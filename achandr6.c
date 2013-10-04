#include <stdio.h>
#include <stdlib.h>

typedef int key_t;
typedef int object_t;
typedef struct interval_list_node {
  int leftPoint;
  int rightPoint;
  struct interval_list_node* next;
} intervalListNode;

typedef struct tr_n_t {
  key_t      key; 
  struct tr_n_t   *left;
  struct tr_n_t  *right;
  int height;
  int measure;
  int leftMin;
  int rightMax;
  int l; // the left of node interval
  int r; // the right of node interval
  /* possibly additional information */ 
} m_tree_t;


#define BLOCKSIZE 256

m_tree_t *root;

/**** Stack ***/
typedef struct st_t {
  m_tree_t *item;
  struct st_t *next; 
} stack_t;

stack_t *create_stack(void)
{
  stack_t *st;
  st = (stack_t*) malloc(sizeof(stack_t));
  if(st==NULL)
    printf("stack malloc failed!\n");
  st->next = NULL;
  return( st );
}

int stack_empty(stack_t *st)
{
  return( st->next == NULL );
}

void push( m_tree_t *x, stack_t *st)
{
  stack_t *tmp;
  tmp = (stack_t*) malloc(sizeof(stack_t));
  if(tmp==NULL)
    printf("stack malloc failed!\n");
  
  tmp->item = x;
  tmp->next = st->next;
  st->next = tmp;
}

m_tree_t *pop(stack_t *st)
{
  stack_t *tmp; m_tree_t* tmp_item;
  tmp = st->next;
  st->next = tmp->next;
  tmp_item = tmp->item;
  free(tmp);
  return( tmp_item );
}

m_tree_t* top(stack_t *st)
{
  return( st->next->item );
}

void remove_stack(stack_t *st)
{
  stack_t *tmp;
  do
    { tmp = st->next;
      free(st);
      st = tmp;
    }
  while ( tmp != NULL );
}
/****** End of stack ******/

m_tree_t *currentblock = NULL;
int    size_left;
m_tree_t *free_list = NULL;
int nodes_taken = 0;
int nodes_returned = 0;

m_tree_t *get_node()
{ m_tree_t *tmp;
  nodes_taken += 1;
  if( free_list != NULL )
    {  tmp = free_list;
      free_list = free_list -> right;
    }
  else
    {  if( currentblock == NULL || size_left == 0)
	{  currentblock = 
	    (m_tree_t *) malloc( BLOCKSIZE * sizeof(m_tree_t) );
	  if(currentblock == NULL)
	    printf("get node malloc failed!\n");
	  size_left = BLOCKSIZE;
	}
      tmp = currentblock++;
      size_left -= 1;
    }
  return( tmp );
}

void return_node(m_tree_t *node)
{  node->right = free_list;
  free_list = node;
  nodes_returned +=1;
}

m_tree_t *create_m_tree(void)
{  m_tree_t *tmp_node;
  tmp_node = get_node();
  tmp_node->left = NULL;
  return( tmp_node );
}

/**
 * Adds the given interval to the node in the tree
 */
void addAssociatedInterval(m_tree_t* tmp_node, intervalListNode *newInterval) 
{
  intervalListNode *temp =  (intervalListNode*) tmp_node->left; 
  intervalListNode *newNode = (intervalListNode*)malloc(sizeof(intervalListNode));
  if(newNode==NULL)
    printf("addAssociated malloc failed!\n");
 
  newNode->leftPoint = newInterval->leftPoint;
  newNode->rightPoint = newInterval->rightPoint;
  newNode->next = NULL;
  if(temp == NULL) 
    {
      tmp_node->left = (m_tree_t*) newNode;
      return;
    }
  
  newNode->next = temp->next;
  temp->next = newNode;
}

int min(int a, int b)
{
  if(a<b)
    return a;
  else
    return b;
}

int max(int a, int b)
{
  if(a>b)
    return a;
  else
    return b;
}

/**
 * Finds the minimum of all the leftPoints of the associated intervals.
 */
int calculateLeftMin(intervalListNode* head) 
{
  int min = head->leftPoint;
  head = head->next;
  while(head!=NULL)
    {
      if(head->leftPoint < min)
	min = head->leftPoint;
      head = head->next;
    }
  return min;
}

/**
 * Finds the maximum of all the rightPoints of the associated intervals.
 */
int calculateRightMax(intervalListNode* head) 
{
  int max = head->rightPoint;
  head = head->next;
  while(head!=NULL)
    {
      if(head->rightPoint > max)
	max = head->rightPoint;
      head = head->next;
    }
  return max;
}

/**
 * The measure is updated using the leftMin, rightMax, l and r of the leaf. 
 */
void updateLeafMeasure(m_tree_t *tmp_node) 
{
  int left, right;
  if(tmp_node->leftMin < tmp_node->l)
    left = tmp_node->l;
  else
    left = tmp_node->leftMin;

  if(tmp_node->rightMax > tmp_node->r)
    right = tmp_node->r;
  else
    right = tmp_node->rightMax;

  tmp_node->measure = right - left;
}

/**
 * The measure of the internal node is updated using the left measure,
 * right measure, l and r of the node. 
 */
void updateInternalMeasure(m_tree_t *n)
{
  if(n->right->leftMin < n->l && n->left->rightMax >= n->r)
    n->measure = n->r - n->l;
  if(n->right->leftMin >= n->l && n->left->rightMax >= n->r)
    n->measure = n->r - n->key + n->left->measure;
  if(n->right->leftMin < n->l && n->left->rightMax < n->r)
    n->measure = n->right->measure + n->key - n->l;
  if(n->right->leftMin >= n->l && n->left->rightMax < n->r)
    n->measure = n->right->measure + n->left->measure;
}

/*
 * The node on which the rotation is called will still have the same
 * l, r, leftMin, rightMax and measure as when it was called as these 
 * characteristics of the subtree should remain independent of rotation.
 * But these characteristics of the other node involved in the rotation
 * would vary. 
 */
void left_rotation(m_tree_t *n)
{ 
  m_tree_t *tmp_node;
  key_t tmp_key;
  tmp_node = n->left;
  tmp_key = n->key;
  n->left = n->right;
  n->key = n->right->key;
  n->right = n->left->right;
  n->left->right = n->left->left;
  n->left->left = tmp_node;
  n->left->key = tmp_key;

  n->left->l = n->l;
  n->left->r = n->key;

  n->left->leftMin = min(n->left->left->leftMin,n->left->right->leftMin); 
  n->left->rightMax = max(n->left->left->rightMax,n->left->right->rightMax);
  updateInternalMeasure(n->left);
}

void right_rotation(m_tree_t *n)
{ 
  m_tree_t *tmp_node;
  key_t tmp_key;
  tmp_node = n->right;
  tmp_key = n->key;
  n->right = n->left;
  n->key = n->left->key;
  n->left = n->right->left;
  n->right->left = n->right->right;
  n->right->right = tmp_node;
  n->right->key = tmp_key;

  n->right->l = n->key;
  n->right->r = n->r;

  n->right->leftMin = min(n->right->left->leftMin,n->right->right->leftMin); 
  n->right->rightMax = max(n->right->left->rightMax,n->right->right->rightMax);
  updateInternalMeasure(n->right);
}

int insert(m_tree_t *tree, key_t new_key, intervalListNode *newInterval)
{  m_tree_t *tmp_node;
  stack_t *s = create_stack();
  stack_t *rotate_s = create_stack();
  if( tree->left == NULL )
    {  
      intervalListNode *interval =(intervalListNode*)malloc(sizeof(intervalListNode));
      if(interval==NULL)
	printf("insert malloc failed!\n");
 
      interval->leftPoint = newInterval->leftPoint;
      interval->rightPoint = newInterval->rightPoint;
      interval->next = NULL;
      tree->left = (m_tree_t *) interval;
      tree->key  = new_key;
      tree->right  = NULL; 
      tree->leftMin = newInterval->leftPoint;
      tree->rightMax = newInterval->rightPoint;
      tree->l = -2147483648; // to represent -infinity
      tree->r = 2147483647; // to represent infinity
      updateLeafMeasure(tree);
    }
  else
    {  tmp_node = tree;
      while( tmp_node->right != NULL )
	{ 
	  push(tmp_node, s);
	  push(tmp_node, rotate_s);
	  if( new_key < tmp_node->key )
	    tmp_node = tmp_node->left;
          else
	    tmp_node = tmp_node->right;
	
	}
      /* found the candidate leaf. Test whether key distinct */
      if( tmp_node->key == new_key )
	{  
	  // if the key exists, then add the current interval to the interval list. 
	  addAssociatedInterval(tmp_node, newInterval);
	  tmp_node->leftMin = min(tmp_node->leftMin, newInterval->leftPoint);
	  tmp_node->rightMax = max(tmp_node->rightMax, newInterval->rightPoint);
	  updateLeafMeasure(tmp_node);
	  
	}
      /* key is distinct, now perform the insert */
      else
	{  
	  m_tree_t *old_leaf, *new_leaf;
	  old_leaf = get_node();
	  old_leaf->left = tmp_node->left; 
	  old_leaf->key = tmp_node->key;
	  old_leaf->right  = NULL;
	  old_leaf->height = 0;
	  old_leaf->leftMin = tmp_node->leftMin;
	  old_leaf->rightMax = tmp_node->rightMax;

	  new_leaf = get_node();
	  addAssociatedInterval(new_leaf, newInterval);
	  new_leaf->key = new_key;
	  new_leaf->right  = NULL;
	  new_leaf->height = 0;
	  new_leaf->leftMin = newInterval->leftPoint;
	  new_leaf->rightMax = newInterval->rightPoint;

	  if( tmp_node->key < new_key )
	    {   tmp_node->left  = old_leaf;
	      tmp_node->right = new_leaf;
	      tmp_node->key = new_key;
	      old_leaf->l = tmp_node->l;
	      old_leaf->r = new_key;
	      new_leaf->l = new_key;
	      new_leaf->r = tmp_node->r;
	    } 
	  else
	    {   tmp_node->left  = new_leaf;
	      tmp_node->right = old_leaf;
	      new_leaf->l = tmp_node->l;
	      new_leaf->r = tmp_node->key;
	      old_leaf->l = tmp_node->key;
	      old_leaf->r = tmp_node->r;
	    } 
	  updateLeafMeasure(old_leaf);
	  updateLeafMeasure(new_leaf);

	  tmp_node->height = 1;
	  tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
	  tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);
	  //push(tmp_node, s);
	 
	 
	  updateInternalMeasure(tmp_node);
	  tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
	  tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);       
	}
      
      // update measures 

      while(!stack_empty(s))
	{
	  tmp_node = top(s);
	  pop(s);
	  updateInternalMeasure(tmp_node);
	  tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
	  tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);       
	}

      // rebalance

      int finished = 0;
      while(!stack_empty(rotate_s) && !finished)
	{
	  tmp_node = top(rotate_s);
	  pop(rotate_s);
	  int tmp_height, old_height;
	  old_height= tmp_node->height;
	  if( tmp_node->left->height - tmp_node->right->height == 2 )
	    { 
	      if( tmp_node->left->left->height - tmp_node->right->height == 1 )
		{ 
		  right_rotation( tmp_node );
		  tmp_node->right->height = tmp_node->right->left->height + 1;
		  tmp_node->height = tmp_node->right->height + 1;
		}
	      else
		{ 
		  left_rotation( tmp_node->left );
		  right_rotation( tmp_node );
		  tmp_height = tmp_node->left->left->height;
		  tmp_node->left->height = tmp_height + 1;
		  tmp_node->right->height = tmp_height + 1;
		  tmp_node->height = tmp_height + 2;
		}
	    }
	  else if( tmp_node->left->height - tmp_node->right->height == -2 )
	    { 
	      if( tmp_node->right->right->height - tmp_node->left->height == 1 )
		{ 
		  left_rotation( tmp_node );
		  tmp_node->left->height = tmp_node->left->right->height + 1;
		  tmp_node->height = tmp_node->left->height + 1;
		}
	      else
		{ 
		  right_rotation( tmp_node->right );
		  left_rotation( tmp_node );
		  tmp_height = tmp_node->right->right->height;
		  tmp_node->left->height = tmp_height + 1;
		  tmp_node->right->height = tmp_height + 1;
		  tmp_node->height = tmp_height + 2;
		}
	    }
	  else /* update height even if there was no rotation */
	    { 
	      if( tmp_node->left->height > tmp_node->right->height )
		tmp_node->height = tmp_node->left->height + 1;
	      else
		tmp_node->height = tmp_node->right->height + 1;
	    }
	  if( tmp_node->height == old_height )
	    finished = 1;


	}
   
    }
  remove_stack(s);
  remove_stack(rotate_s);
  return 0;
}

void insert_interval(m_tree_t *tree, int a, int b) 
{
  intervalListNode *interval = (intervalListNode*) malloc(sizeof(intervalListNode));
  interval->leftPoint = a;
  interval->rightPoint = b;
  interval->next = NULL;
  insert(tree, a, interval);
  insert(tree, b, interval);
  free(interval);
}

void deleteIntervalFromNode(m_tree_t* tmp_node, intervalListNode* interval)
{
  intervalListNode* temp = (intervalListNode*)tmp_node->left;
  intervalListNode* next = temp->next;
  if(temp!= NULL && temp->leftPoint == interval->leftPoint && temp->rightPoint == interval->rightPoint)
    {
      tmp_node->left = (m_tree_t*) next;
      free(temp);
      return;
    }
  
  while(next!=NULL)
    {
      if(next->leftPoint == interval->leftPoint && next->rightPoint == interval->rightPoint)
	{
	  temp->next = next->next;
	  free(next);
	  return;
	}
      temp = temp->next;
      next = next->next;
    }
}

void *delete(m_tree_t *tree, key_t delete_key, intervalListNode* interval)
{  m_tree_t *tmp_node, *upper_node, *other_node;
  stack_t *s = create_stack();
  stack_t *rotate_s = create_stack();
  if( tree->left == NULL )
    return;
  else if( tree->right == NULL )
    {  
      if(  tree->key == delete_key )
	{ 
	  
	  free(tree->left);
	  tree->left = NULL;
	  tree->key  = 0;
	  tree->right  = NULL; 
	  tree->leftMin = 0;
	  tree->rightMax = 0;
	  tree->l = -2147483648;
	  tree->r = 2147483647;
	  tree->measure = 0;
	}  
      return;
     
    }
  else
    {  tmp_node = tree;
      while( tmp_node->right != NULL )
	{   upper_node = tmp_node;
	  push(tmp_node, s);
	  push(tmp_node, rotate_s);
          if( delete_key < tmp_node->key )
	    {  tmp_node   = upper_node->left; 
	      other_node = upper_node->right;
	    } 
          else
	    {  tmp_node   = upper_node->right; 
	      other_node = upper_node->left;
	    } 
	}
      if( tmp_node->key != delete_key )
	return;


      deleteIntervalFromNode(tmp_node, interval);

      if(tmp_node->left == NULL)
	{  upper_node->key   = other_node->key;
	  upper_node->left  = other_node->left;
	  upper_node->right = other_node->right;
	  upper_node->leftMin = other_node->leftMin;
	  upper_node->rightMax = other_node->rightMax;
	  upper_node->measure = other_node->measure;
	  upper_node->height = other_node->height;
	 
	  if(upper_node->right != NULL)
	    {
	      upper_node->right->r = upper_node->r;
	      upper_node->left->l = upper_node->l;
	    }	     

	  updateLeafMeasure(upper_node);

	  return_node( tmp_node );
	  return_node( other_node );
	  pop(s); // removing the upper_node from the stack so as to avoid updating measure like an internal node
	  pop(rotate_s);
	}
      else
	{
	  tmp_node->leftMin = calculateLeftMin((intervalListNode*)tmp_node->left);
	  tmp_node->rightMax = calculateRightMax((intervalListNode*)tmp_node->left);
	  updateLeafMeasure(tmp_node);
	  //push(s, upper_node);
	}
   
      //update measure

      while(!stack_empty(s))
	{
	  tmp_node = top(s);
	  pop(s);
	  updateInternalMeasure(tmp_node);
	  tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
	  tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);         
	}
      
      int finished = 0;
      while(!stack_empty(rotate_s) && !finished)
	{
	  tmp_node = top(rotate_s);
	  pop(rotate_s);

	  int tmp_height, old_height;
	  old_height= tmp_node->height;
	  if( tmp_node->left != NULL && tmp_node->right != NULL && tmp_node->left->left != NULL && tmp_node->left->height - tmp_node->right->height == 2 )
	    { 
	      if( tmp_node->left->left->height - tmp_node->right->height == 1 )
		{ 
		  right_rotation( tmp_node );
		  tmp_node->right->height = tmp_node->right->left->height + 1;
		  tmp_node->height = tmp_node->right->height + 1;
		}
	      else
		{ 
		  left_rotation( tmp_node->left );
		  right_rotation( tmp_node );
		  tmp_height = tmp_node->left->left->height;
		  tmp_node->left->height = tmp_height + 1;
		  tmp_node->right->height = tmp_height + 1;
		  tmp_node->height = tmp_height + 2;
		}
	    }
	  else if( tmp_node->left != NULL && tmp_node->right != NULL && tmp_node->right->right != NULL && tmp_node->left->height - tmp_node->right->height == -2 )
	    { 
	      if( tmp_node->right->right->height - tmp_node->left->height == 1 )
		{ 
		  left_rotation( tmp_node );
		  tmp_node->left->height = tmp_node->left->right->height + 1;
		  tmp_node->height = tmp_node->left->height + 1;
		}
	      else
		{ 
		  right_rotation( tmp_node->right );
		  left_rotation( tmp_node );
		  tmp_height = tmp_node->right->right->height;
		  tmp_node->left->height = tmp_height + 1;
		  tmp_node->right->height = tmp_height + 1;
		  tmp_node->height = tmp_height + 2;
		}
	    }
	  else if( tmp_node->left != NULL && tmp_node->right != NULL)/* update height even if there was no rotation */
	    { 
	      if( tmp_node->left->height > tmp_node->right->height )
		{
		  tmp_node->height = tmp_node->left->height + 1;
		}
	      else
		{
		  tmp_node->height = tmp_node->right->height + 1;
		}
	    }
	  if( tmp_node->height == old_height )
	    finished = 1;

	}

    }
  remove_stack(s);
  remove_stack(rotate_s);
}

void delete_interval(m_tree_t* t, int a, int b)
{
  intervalListNode *interval = (intervalListNode*) malloc(sizeof(intervalListNode));
  interval->leftPoint = a;
  interval->rightPoint = b;
  interval->next = NULL;
  delete(t, a, interval);
  delete(t, b, interval);
  free(interval);
}

void remove_tree(m_tree_t *tree)
{  m_tree_t *current_node, *tmp;
  if( tree->left == NULL )
    return_node( tree );
  else
    {  current_node = tree;
      while(current_node->right != NULL )
	{  if( current_node->left->right == NULL )
	    {  return_node( current_node->left );
	      tmp = current_node->right;
	      return_node( current_node );
	      current_node = tmp;
	    }
	  else
	    {  tmp = current_node->left;
	      current_node->left = tmp->right;
	      tmp->right = current_node; 
	      current_node = tmp;
	    }
	}
      return_node( current_node );
    }
}

int query_length(m_tree_t* t)
{
  if(t!=NULL)
    return t->measure;
  else 
    return -1;
}

int main()
{  int i; m_tree_t *t; ;
  printf("starting \n");
  t = create_m_tree();
  for(i=0; i< 50; i++ )
    insert_interval( t, 2*i, 2*i +1 );
  printf("inserted first 50 intervals, total length is %d, should be 50.\n", query_length(t));
  insert_interval( t, 0, 100 );
  printf("inserted another interval, total length is %d, should be 100.\n", query_length(t));
  for(i=1; i< 50; i++ )
    insert_interval( t, 199 - (3*i), 200 ); //[52,200] is longest
  printf("inserted further 49 intervals, total length is %d, should be 200.\n", query_length(t));
  for(i=2; i< 50; i++ )
    delete_interval(t, 2*i, 2*i +1 );
  delete_interval(t,0,100);
  printf("deleted some intervals, total length is %d, should be 150.\n", query_length(t));
  insert_interval( t, 1,2 ); 
  for(i=49; i>0; i-- )
    delete_interval( t, 199 - (3*i), 200 ); 
  insert_interval( t, 0,2 );
  insert_interval( t, 1,5 );  
  printf("deleted some intervals, total length is %d, should be 5.\n", query_length(t));
  insert_interval( t, 0, 100 );
  printf("inserted another interval, total length is %d, should be 100.\n", query_length(t));
  for(i=0; i<=3000; i++ )
    insert_interval( t, 2000+i, 3000+i ); 
  printf("inserted 3000 intervals, total length is %d, should be 4100.\n", query_length(t));
  for(i=0; i<=3000; i++ )
    delete_interval( t, 2000+i, 3000+i ); 
  printf("deleted 3000 intervals, total length is %d, should be 100.\n", query_length(t));
  for(i=0; i<=100; i++ )
    insert_interval( t, 10*i, 10*i+100 ); 
  printf("inserted another 100 intervals, total length is %d, should be 1100.\n", query_length(t));
  delete_interval( t, 1,2 ); 
  delete_interval( t, 0,2 ); 
  delete_interval( t, 2,3 ); 
  delete_interval( t, 0,1 ); 
  delete_interval( t, 1,5 );
  printf("deleted some intervals, total length is %d, should be still 1100.\n", query_length(t)); 
  for(i=0; i<= 100; i++ )
    delete_interval(t, 10*i, 10*i+100);
  delete_interval(t,0,100);
  printf("deleted last interval, total length is %d, should be 0.\n", query_length(t));
  for( i=0; i<100000; i++)
    {  insert_interval(t, i, 1000000);
    }
  printf("inserted again 100000 intervals, total length is %d, should be 1000000.\n", query_length(t));
  printf("End Test\n");
}

