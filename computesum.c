#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    int parent_id;
    int sum;
    int num_children;
    foothread_mutex_t mutex;
    foothread_barrier_t barrier;
    
} Node;
foothread_mutex_t input_mutex;
foothread_mutex_t internal_mutex;
foothread_mutex_t internal_mutex1;
foothread_mutex_t internal_mutex2;
foothread_mutex_t internal_mutex3;
foothread_mutex_t internal_mutex4;
foothread_mutex_t internal_mutex5;
foothread_mutex_t internal_mutex6;
foothread_mutex_t print_mutex;
Node* nodes;
foothread_barrier_t real_barrier,child_barrier;
int leaf_node_func(void* arg) {
    Node* node = (Node*)arg;
    
    foothread_mutex_lock(&input_mutex);
    //printf("hi\n");
    printf("Enter value for leaf node %d: ", node->id);
    fflush(stdout);
    scanf("%d", &node->sum);
    
    
    
    nodes[node->parent_id].sum += node->sum;
    
    
    Node* nodee=&nodes[node->parent_id];
    // printf(" parent of leaf: %d",nodee->id);
    
    foothread_mutex_unlock(&input_mutex);
    
    
    foothread_barrier_wait(&(nodee->barrier));
    
    foothread_barrier_wait(&real_barrier);
    foothread_exit(NULL);
    return 0;
}

int internal_node_func(void* arg) {
    Node* node = (Node*)arg;
    
    
    foothread_barrier_wait(&(node->barrier));
    
    foothread_mutex_lock(&internal_mutex);
    foothread_mutex_lock(&internal_mutex2);
    
    if(nodes[node->parent_id].id!=node->id){
        nodes[node->parent_id].sum += node->sum;
    }
    
    
    printf("Internal node %d gets the partial sum %d from its children\n",node->id,node->sum);
    fflush(stdout);
    
    foothread_mutex_unlock(&internal_mutex2);
    foothread_mutex_unlock(&internal_mutex);
    if(nodes[node->parent_id].id==node->id){
        foothread_barrier_wait(&real_barrier);
        foothread_exit(NULL);
    }
    Node* nodee=&nodes[node->parent_id];
    foothread_barrier_wait(&(nodee->barrier));
    
    foothread_barrier_wait(&real_barrier);
    foothread_exit(NULL);
    return 0;
}

int main() {
    int n;
    FILE* file = fopen("tree.txt", "r");
    fscanf(file, "%d", &n);
    foothread_mutex_init(&print_mutex);
    nodes = malloc(n * sizeof(Node));

    for (int i = 0; i < n; i++) {
        nodes[i].id = i;
        nodes[i].sum = 0;
        nodes[i].num_children = 0;
        foothread_mutex_init(&(nodes[i].mutex));
    }
    foothread_mutex_init(&input_mutex);
    foothread_mutex_init(&internal_mutex);
    foothread_mutex_init(&internal_mutex2);
    int root_id;
    for (int i = 0; i < n; i++) {
        int x;
        fscanf(file, "%d", &x);
        fscanf(file, "%d", &(nodes[x].parent_id));
        if (nodes[x].parent_id == x) {
            root_id = x;
        } else {
            nodes[nodes[x].parent_id].num_children++;
        }
    }
    
       
    fclose(file);
    foothread_barrier_init(&real_barrier, n);
    for (int i = 0; i < n; i++) {
        //printf("No of children for node for %d :%d\n",nodes[i].id,nodes[i].num_children);
    
        foothread_barrier_init(&(nodes[i].barrier), nodes[i].num_children+1);
    }

    int num_of_leafs=0;
    foothread_t threads[n];
    for (int i = 0; i < n; i++) {
        if (nodes[i].num_children == 0) {
            num_of_leafs++;
        }
    }
    foothread_barrier_init(&child_barrier, n-1);
    for (int i = 0; i < n; i++) {
    
        if (nodes[i].num_children == 0) {
            
            foothread_create(&(threads[i]), NULL, leaf_node_func, &(nodes[i]));
        } else {
            
            foothread_create(&(threads[i]), NULL, internal_node_func, &(nodes[i]));
        }
    
    }
    
    foothread_barrier_wait(&real_barrier);

    printf("sum at root ( node %d ): %d\n",root_id, nodes[root_id].sum);

    for (int i = 0; i < n; i++) {
        foothread_mutex_destroy(&(nodes[i].mutex));
        foothread_barrier_destroy(&(nodes[i].barrier));
    }
    foothread_mutex_destroy(&print_mutex);
    free(nodes);

    return 0;
}