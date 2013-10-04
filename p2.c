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
/*typedef struct stk_ety {
  text_t *node;
  int side; // 0 if  left, 1 if right. Used to detect if the path followed left or right sub tree. Used for node updation.
} stack_entry;
*/
/**** Stack ***/
typedef struct st_t {
  m_tree_t *item;
  struct st_t *next; 
} stack_t;

stack_t *create_stack(void)
{
  stack_t *st;
  st = (stack_t*) malloc(sizeof(stack_t));
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

object_t *find_iterative(m_tree_t *tree, key_t query_key)
{  m_tree_t *tmp_node;
   if( tree->left == NULL )
     return(NULL);
   else
   {  tmp_node = tree;
      while( tmp_node->right != NULL )
      {   if( query_key < tmp_node->key )
               tmp_node = tmp_node->left;
          else
               tmp_node = tmp_node->right;
      }
      if( tmp_node->key == query_key )
         return( (object_t *) tmp_node->left );
      else
         return( NULL );
   }
}

object_t *find_recursive(m_tree_t *tree, key_t query_key)
{  if( tree->left == NULL || 
       (tree->right == NULL && tree->key != query_key ) )
      return(NULL);
   else if (tree->right == NULL && tree->key == query_key )
      return( (object_t *) tree->left );     
   else
   {  if( query_key < tree->key )
         return( find_recursive(tree->left, query_key) );
      else
         return( find_recursive(tree->right, query_key) );
   }
}

void addAssociatedInterval(m_tree_t* tmp_node, intervalListNode *newInterval) 
{
  intervalListNode *temp =  (intervalListNode*) tmp_node->left; 
  intervalListNode *newNode = (intervalListNode*)malloc(sizeof(intervalListNode));
  newNode->leftPoint = newInterval->leftPoint;
  newNode->rightPoint = newInterval->rightPoint;
  newNode->next = NULL;
  if(temp == NULL) 
    {
      tmp_node->left = (m_tree_t*) newNode;
      return;
    }
  while(temp->next != NULL)
    {
      temp = temp->next;
    }
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
}

int insert(m_tree_t *tree, key_t new_key, intervalListNode *newInterval)
{  m_tree_t *tmp_node;
  stack_t *s = create_stack();
   if( tree->left == NULL )
   {  
      intervalListNode *interval =(intervalListNode*)malloc(sizeof(intervalListNode));
      interval->leftPoint = newInterval->leftPoint;
      interval->rightPoint = newInterval->rightPoint;
      interval->next = NULL;
      tree->left = (m_tree_t *) interval;
      tree->key  = new_key;
      tree->right  = NULL; 
      tree->leftMin = newInterval->leftPoint;
      tree->rightMax = newInterval->rightPoint;
      tree->l = -2147483648;
      tree->r = 2147483647;
      updateLeafMeasure(tree);
   }
   else
   {  tmp_node = tree;
      while( tmp_node->right != NULL )
      { 
	push(tmp_node, s);
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
	 //old_leaf->measure = tmp_node->measure;

         new_leaf = get_node();
         //new_leaf->left = (m_tree_t *) newInterval; 
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

	 tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
	 tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);
	 push(tmp_node, s);
      }
   }
   // update measures. 

   while(!stack_empty(s))
     {
       tmp_node = top(s);
       pop(s);
       updateInternalMeasure(tmp_node);
       tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
       tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);       
     }

   remove_stack(s);
   return( 0 );
}

void padding ( char ch, int n )
{
  int i;
  for ( i = 0; i < n; i++ )
    putchar ( ch );
}

void printIntervals(intervalListNode* head)
{
  while(head!=NULL)
    {
      printf("(%d,%d)", head->leftPoint, head->rightPoint);
      head=head->next;
    }
}

void structure ( m_tree_t *root, int level )
{
  int i;
  if ( root->right == NULL ) 
    {
      padding ( '\t', level );
      printf ( "%d m=%d l=%d r=%d lmin=%d rmax=%d", root->key, root->measure, root->l, root->r, root->leftMin, root->rightMax );
      printIntervals((intervalListNode*)root->left);
      printf("\n");
    }
  else 
    {
      structure ( root->right, level + 1 );
      padding ( '\t', level );
      printf ( "%d m=%d l=%d r=%d lmin=%d rmax=%d\n", root->key, root->measure, root->l, root->r, root->leftMin, root->rightMax );
      structure ( root->left, level + 1 );
    }
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

void* delete(m_tree_t *tree, key_t delete_key, intervalListNode* interval)
{ 
 m_tree_t *tmp_node, *upper_node, *other_node;
  //object_t *deleted_object;
  stack_t *s = create_stack();
   if( tree->left == NULL )
      return;
   else if( tree->right == NULL )
   {  
     if(  tree->key == delete_key )
       {  
	 free(tree->left);
	 //deleted_object = (object_t *) tree->left;
         tree->left = NULL;
       }  
     return;
     
   }
   else
   {  tmp_node = tree;
      while( tmp_node->right != NULL )
      {   upper_node = tmp_node;
	  push(tmp_node, s);
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
	 //deleted_object = (object_t *) tmp_node->left;
         return_node( tmp_node );
         return_node( other_node );
	 // return( deleted_object );
	 pop(s); // removing the upper_node from the stack so as to avoid updating measure like an internal node
      }
      else
      {
	tmp_node->leftMin = calculateLeftMin((intervalListNode*)tmp_node->left);
	tmp_node->rightMax = calculateRightMax((intervalListNode*)tmp_node->left);
	updateLeafMeasure(tmp_node);
	//push(s, upper_node);
      }
   }
   
   //update measure

   while(!stackEmpty(s))
     {
       tmp_node = top(s);
       pop(s);
       //printf("update measure in delete called for %d\n", tmp_node->key);
       updateInternalMeasure(tmp_node);
       tmp_node->leftMin = min(tmp_node->left->leftMin,tmp_node->right->leftMin); 
       tmp_node->rightMax = max(tmp_node->left->rightMax,tmp_node->right->rightMax);       
     }
   removeStack(s);
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

m_tree_t *interval_find(m_tree_t *tree, key_t a, key_t b)
{  m_tree_t *tr_node;
   m_tree_t *node_stack[200]; int stack_p = 0;
   m_tree_t *result_list, *tmp, *tmp2;
   result_list = NULL;
   node_stack[stack_p++] = tree;
   while( stack_p > 0 )
   {  tr_node = node_stack[--stack_p];
      if( tr_node->right == NULL )
      {  /* reached leaf, now test */
	 if( a <= tr_node->key && tr_node->key < b )
         {  tmp = get_node();        /* leaf key in interval */
            tmp->key  = tr_node->key; /* copy to output list */  
	    tmp->left = tr_node->left;   
            tmp->right = result_list;
            result_list = tmp;
         }
      } /* not leaf, might have to follow down */
      else if ( b <= tr_node->key ) /* entire interval left */
         node_stack[stack_p++] = tr_node->left;
      else if ( tr_node->key <= a ) /* entire interval right*/
         node_stack[stack_p++] = tr_node->right;
      else   /* node key in interval, follow left and right */
      {  node_stack[stack_p++] = tr_node->left;
         node_stack[stack_p++] = tr_node->right;
      }
   }
   return( result_list );
}

void check_tree( m_tree_t *tr, int depth, int lower, int upper )
{  if( tr->left == NULL )
   {  printf("Tree Empty\n"); return; }
   if( tr->key < lower || tr->key >= upper )
         printf("Wrong Key Order \n");
   if( tr->right == NULL )
   {  if( *( (int *) tr->left) == 10*tr->key + 2 )
         printf("%d(%d)  ", tr->key, depth );
      else
         printf("Wrong Object \n");
   }
   else
   {  check_tree(tr->left, depth+1, lower, tr->key ); 
      check_tree(tr->right, depth+1, tr->key, upper ); 
   }
}

int query_length(m_tree_t* t)
{
  if(t!=NULL)
    return t->measure;
  else 
    return -1;
}

/*int main()
{
  /*  int i; m_tree_t *t; ;
  printf("starting \n");
  t = create_m_tree();
  for(i=0; i< 3000; i++ )
    insert_interval( t, 2*i, 2*i +1 );
  printf("inserted first 50 intervals, total length is %d, should be 50.\n", query_length(t));
  insert_interval( t, 0, 100 );
  printf("inserted another interval, total length is %d, should be 100.\n", query_length(t));
  for(i=1; i< 50; i++ )
    insert_interval( t, 199 - (3*i), 200 ); 
  printf("inserted further 49 intervals, total length is %d, should be 200.\n", query_length(t));  
  */
/*
  root = create_m_tree();
  printf("inserting (5,30)\n");
  insert_interval(root, 5, 30);
  structure(root, 0);
  printf("inserting (15,40)\n");
  insert_interval(root, 15,40);
  structure(root, 0);
  printf("inserting (25,50)\n");
  insert_interval(root, 25,50);
  structure(root, 0);
  printf("inserting (35,60)\n");
  insert_interval(root, 35,60);
  structure(root, 0);
  printf("inserting (45,70)\n");
  insert_interval(root, 45,70);
  structure(root, 0);

  //printf("deleting (0,5)\n");
  //delete_interval(root, 0,5);
  //structure(root, 0);
}
*/

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

/*int main()
{  m_tree_t *searchtree;
   char nextop;
   searchtree = create_tree();
   printf("Made Tree\n");
   printf("In the following, the key n is associated wth the objecct 10n+2\n");
   while( (nextop = getchar())!= 'q' )
   { if( nextop == 'i' )
     { int inskey,  *insobj, success;
       insobj = (int *) malloc(sizeof(int));
       scanf(" %d", &inskey);
       *insobj = 10*inskey+2;
       success = insert( searchtree, inskey, insobj );
       if ( success == 0 )
         printf("  insert successful, key = %d, object value = %d, \n",
        	  inskey, *insobj);
       else
           printf("  insert failed, success = %d\n", success);
     }  
     if( nextop == 'f' )
     { int findkey, *findobj;
       scanf(" %d", &findkey);
       findobj = find_iterative( searchtree, findkey);
       if( findobj == NULL )
         printf("  find (iterative) failed, for key %d\n", findkey);
       else
         printf("  find (iterative) successful, found object %d\n", *findobj);
       findobj = find_recursive( searchtree, findkey);
       if( findobj == NULL )
         printf("  find (recursive) failed, for key %d\n", findkey);
       else
         printf("  find (recursive) successful, found object %d\n", *findobj);
     }
     if( nextop == 'v' )
     { int a, b;  m_tree_t *results, *tmp;
       scanf(" %d %d", &a, &b);
       results = interval_find( searchtree, a, b );
       if( results == NULL )
          printf("  no keys found in the interval [%d,%d[\n", a, b);
       else
       {  printf("  the following keys found in the interval [%d,%d[\n", a, b);
          while( results != NULL )
	  {  printf("(%d,%d) ", results->key, *((int *) results->left) );
             tmp = results;
	     results = results->right;
             return_node( tmp );
          }
          printf("\n");
       }
     }
     if( nextop == 'd' )
     { int delkey, *delobj;
       scanf(" %d", &delkey);
       delobj = _delete( searchtree, delkey);
       if( delobj == NULL )
         printf("  delete failed for key %d\n", delkey);
       else
         printf("  delete successful, deleted object %d for key %d\n", *delobj, delkey);
     }
     if( nextop == '?' )
     {  printf("  Checking tree\n"); 
        check_tree(searchtree,0,-1000,1000);
        printf("\n");
        if( searchtree->left != NULL )
 	   printf("key in root is %d\n",	 searchtree->key );
        printf("  Finished Checking tree\n"); 
     }
   }

   remove_tree( searchtree );
   printf("Removed tree.\n");
   printf("Total number of nodes taken %d, total number of nodes returned %d\n",
	  nodes_taken, nodes_returned );
   return(0);
}
*/
