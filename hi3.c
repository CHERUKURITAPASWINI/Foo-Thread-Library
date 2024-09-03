#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_NODES 100 // Maximum number of nodes in the tree
#define MAX_CHILDREN 10 // Maximum number of children for a node

// Struct to hold node information
typedef struct {
    int id;
    int parent;
    int children[MAX_CHILDREN];
    int num_children;

    int value;
    foothread_mutex_t lock;
    int partial_sum;
} TreeNode;

// Global variables
int num_nodes;
TreeNode nodes[MAX_NODES];

int user_input_sum = 0;
foothread_barrier_t barrier;
foothread_mutex_t input_mutex;
// Function to read tree information from file
void read_tree_info(FILE *file, int parent[], int no_of_children[]) {
    
    for (int i = 0; i < num_nodes; i++) {
        int x,y;
        fscanf(file, "%d", x);
        fscanf(file, "%d", y);
        parent[x]=y;
        no_of_children[y]++;
        
    }
}

// Function for leaf nodes to read user input
int leaf_thread_start(void *arg) {
    TreeNode *node = (TreeNode *)arg;
    foothread_mutex_lock(&input_mutex);
    printf("Enter value for leaf node %d: ", node->id);
    scanf("%d", &node->value);
    foothread_mutex_unlock(&input_mutex);
    foothread_mutex_lock(&node->lock);
    nodes[node->parent].partial_sum += node->value;
    foothread_mutex_unlock(&node->lock);

    foothread_barrier_wait(&barrier);

    return 0;
}

// Function for internal nodes to compute partial sum
int internal_thread_start(void *arg) {
    TreeNode *node = (TreeNode *)arg;

    foothread_barrier_wait(&barrier); // Wait for leaf nodes to finish reading user input

    int partial_sum = 0;
    for (int i = 0; i < node->num_children; i++) {
        foothread_mutex_lock(&nodes[node->children[i]].lock);
        partial_sum += nodes[node->children[i]].partial_sum;
        foothread_mutex_unlock(&nodes[node->children[i]].lock);
    }

    foothread_mutex_lock(&node->lock);
    node->partial_sum = partial_sum;
    foothread_mutex_unlock(&node->lock);

    foothread_barrier_wait(&barrier);

    return NULL;
}

int main() {
    FILE *file = fopen("tree.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file.\n");
        return 1;
    }
    fscanf(file, "%d", &num_nodes);
    int parent[num_nodes];
    int no_of_children[num_nodes];
    memset(no_of_children,0,sizeof(no_of_children));
    read_tree_info(file,parent,no_of_children);
    fclose(file);

    // Initialize barrier
    foothread_barrier_init(&barrier, num_nodes);
    foothread_mutex_init(&input_mutex);
    // Create threads for each node
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].num_children == 0) {
            foothread_t thread;
            foothread_create(&thread, NULL, leaf_thread_start, &nodes[i]);
        } else {
            foothread_t thread;
            foothread_create(&thread, NULL, internal_thread_start, &nodes[i]);
        }
    }

    foothread_exit(); // Synchronize threads

    // Print total sum at root node
    printf("Total sum: %d\n", nodes[0].partial_sum);

    // Cleanup resources
    for (int i = 0; i < num_nodes; i++) {
        foothread_mutex_destroy(&nodes[i].lock);
    }
    foothread_barrier_destroy(&barrier);
    foothread_mutex_destroy(&input_mutex);
    return 0;
}
