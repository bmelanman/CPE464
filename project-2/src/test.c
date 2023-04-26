
#include "dictionary.h"

int main(void) {

    dict *dict1 = create_new_dict(3);

    /* Check insert */
    dict_insert(dict1, "a", 1);
    dict_insert(dict1, "b", 2);
    dict_insert(dict1, "c", 3);
    dict_insert(dict1, "d", 4);

    printf("cap: %d, size: %d\n", dict1->cap, dict1->size);

    /* This shouldn't do anything, nor should it increase the size */
    dict_insert(dict1, "a", 5);

    printf("cap: %d, size: %d\n\n", dict1->cap, dict1->size);

    /* Test search */
    printf("search a: %d\n", dict_search(dict1, "a"));
    printf("search b: %d\n", dict_search(dict1, "b"));
    printf("search c: %d\n", dict_search(dict1, "c"));
    printf("search d: %d\n", dict_search(dict1, "d"));
    printf("search e: %d\n\n", dict_search(dict1, "e"));

    /* Test remove */
    printf("remove b: %d\n", dict_remove(dict1, "b"));
    /* This should return -1 (not found) */
    printf("remove b: %d\n", dict_remove(dict1, "b"));

    free_hash(dict1);

    return 0;
}
