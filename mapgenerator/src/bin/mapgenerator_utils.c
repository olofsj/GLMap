
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mapgenerator.h"

#define EARTH_RADIUS 6371009

// Calculate the distance between two points on the earths surface
double distance(double from_lat, double from_lon, double to_lat, double to_lon) {
    // Earth radius
    double lambda, phi, phi_m;

    phi_m = (to_lat + from_lat)/360.0*M_PI;
    phi = (to_lat - from_lat)/180.0*M_PI;
    lambda = (to_lon - from_lon)/180.0*M_PI;

    return EARTH_RADIUS*sqrt(phi*phi + pow(cos(phi_m)*lambda, 2));
}

// Effective distance, with any penalties from the given profile
double effective_distance(RoutingProfile *profile, RoutingTagSet *tagset, 
        double from_lat, double from_lon, double to_lat, double to_lon) {
    double dist;
    int i;

    dist = distance(from_lat, from_lon, to_lat, to_lon);

    for (i = 0; i < tagset->size; i++) {
        dist *= profile->penalty[tagset->tags[i]];
    }

    return dist;
}

List * list_sorted_insert(List *list, void *data, List_Compare_Cb compare) {
    List *cn;
    List *l;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = NULL;
    l->prev = NULL;

    if (!list) {
        return l;
    }

    // Check the head of the list
    if (compare(data, list->data) <= 0) {
        l->next = list;
        list->prev = l;
        return l;
    }

    // Walk the list
    cn = list;
    while (cn->next) {
        if (compare(data, cn->next->data) <= 0) {
            l->next = cn->next;
            cn->next = l;
            l->prev = cn->next;
            l->next->prev = l;
            return list;
        }
        cn = cn->next;
    }

    // Not found, append to end
    cn->next = l;
    l->prev = cn;
    return list;
}

List * list_sorted_merge(List *list1, List *list2, List_Compare_Cb compare) {
    List *l, *result;

    if (!list1)
        return list2;
    if (!list2)
        return list1;

    // Get the start of the resulting list
    if (compare(list1->data, list2->data) <= 0) {
        result = list1;
        l = list1;
        list1 = list1->next;
    } else {
        result = list2;
        l = list2;
        list2 = list2->next;
    }

    // Merge the two lists
    while (list1 && list2) {
        if (compare(list1->data, list2->data) <= 0) {
            l->next = list1;
            list1->prev = l;
            list1 = list1->next;
        } else {
            l->next = list2;
            list2->prev = l;
            list2 = list2->next;
        }
        l = l->next;
    }

    // Add the remaining tail
    if (list1) {
        l->next = list1;
        list1->prev = l;
    }

    if (list2) {
        l->next = list2;
        list2->prev = l;
    }

    return result;
}

List * list_merge_sort(List *list, int size, List_Compare_Cb compare) {
    List *right, *left, *result;
    int i;

    if (size <= 1)
        return list;

    // Split the list into two halves
    left = list;
    right = list;
    for (i = 0; i < size/2; i++)
        right = right->next;

    right->prev->next = NULL;
    right->prev = NULL;

    // Sort the halves separately
    left = list_merge_sort(left, size/2, compare);
    right = list_merge_sort(right, (size+1)/2, compare);

    // Merge the sorted halves
    return list_sorted_merge(left, right, compare);
}

List * list_sort(List *list, List_Compare_Cb compare) {
    return list_merge_sort(list, list_count(list), compare);
}

List * list_prepend(List *list, void *data) {
    List *l;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = list;
    l->prev = NULL;

    if (list)
        list->prev = l;

    return l;
}

List * list_append(List *list, void *data) {
    List *l;
    List *ll;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = NULL;
    l->prev = NULL;

    if (!list)
        return l;

    ll = list;
    while (ll->next) ll = ll->next;
    ll->next = l;
    l->prev = ll;

    return list;
}

List * list_find(List *list, void *data, List_Compare_Cb compare) {
    List *l;

    l = list;
    while (l) {
        if (compare(l->data, data) == 0)
            return l;
        l = l->next;
    }

    return NULL;
}

int list_count(List *list) {
    List *l;
    int count = 0;

    l = list;
    while (l) {
        count ++;
        l = l->next;
    }

    return count;
}

int routing_index_bsearch(RoutingNode *nodes, int id, int low, int high) {
    int mid;

    if (high < low)
        return -1; // not found

    mid = low + ((high - low) / 2);
    if (nodes[mid].id > id) {
        return routing_index_bsearch(nodes, id, low, mid-1);
    } else if (nodes[mid].id < id) {
        return routing_index_bsearch(nodes, id, mid+1, high);
    } else {
        return mid;
    }
}

int routing_index_find_node(RoutingIndex *ri, int id) {
    return routing_index_bsearch(ri->nodes, id, 0, ri->nrof_nodes-1);
}

int min_lat_bsearch(RoutingNode **nodes, double lat, int low, int high) {
    int mid;

    if (high < low) {
        return high; // no exact match, return closest
    }

    mid = low + ((high - low) / 2);
    if (nodes[mid]->lat > lat) {
        return min_lat_bsearch(nodes, lat, low, mid-1);
    } else if (nodes[mid]->lat < lat) {
        return min_lat_bsearch(nodes, lat, mid+1, high);
    } else {
        return mid;
    }
}

int
node_sort_by_lat_cb(const void *n1, const void *n2)
{
    const RoutingNode *m1 = NULL;
    const RoutingNode *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->lat > m2->lat)
        return 1;
    if (m1->lat < m2->lat)
        return -1;
    return 0;
}

RoutingNode ** ww_nodes_get_sorted_by_lat(RoutingIndex *ri) {
    int i;
    RoutingNode **nodes;
    List *node_list, *cn;

    node_list = NULL;
    for (i = 0; i < ri->nrof_nodes; i++) {
        node_list = list_prepend(node_list, &ri->nodes[i]);
    }

    node_list = list_sort(node_list, node_sort_by_lat_cb);
    
    nodes = malloc(ri->nrof_nodes * sizeof(RoutingNode *));
    i = 0;
    cn = node_list;
    while (cn) {
        nodes[i++] = (RoutingNode *)(cn->data);
        cn = cn->next;
    }

    return nodes;
}

RoutingNode * ww_find_closest_node(RoutingIndex *ri, RoutingNode **sorted_by_lat, double lat, double lon) {

    if (!sorted_by_lat) {
        sorted_by_lat = ww_nodes_get_sorted_by_lat(ri);
    }

    int min_lat_index, i;
    RoutingNode *nd, *closest;
    double min_d, distance_to_lat_factor;

    distance_to_lat_factor = 180.0/(M_PI*EARTH_RADIUS);

    // Get the index of the node with latitude closest to the sought latitude
    min_lat_index = min_lat_bsearch(sorted_by_lat, lat, 0, ri->nrof_nodes-1);
    if (min_lat_index < 0 || min_lat_index >= ri->nrof_nodes)
        return NULL;

    closest = sorted_by_lat[min_lat_index];
    min_d = distance(lat, lon, closest->lat, closest->lon);

    // Check until we find the closest node
    i = min_lat_index-1;
    nd = sorted_by_lat[i];
    while (i >= 0 && abs(nd->lat - lat) < min_d*distance_to_lat_factor) {
        double d;
        d = distance(lat, lon, nd->lat, nd->lon);
        if (d < min_d) {
            closest = nd;
            min_d = d;
        }
        nd = sorted_by_lat[--i];
    }

    i = min_lat_index+1;
    nd = sorted_by_lat[i];
    while (i < ri->nrof_nodes && abs(nd->lat - lat) < min_d*distance_to_lat_factor) {
        double d;
        d = distance(lat, lon, nd->lat, nd->lon);
        if (d < min_d) {
            closest = nd;
            min_d = d;
        }
        nd = sorted_by_lat[++i];
    }

    return closest;

}

int * ww_find_nodes(RoutingIndex *ri, RoutingNode **sorted_by_lat, 
        double lat, double lon, double radius) {

    if (!sorted_by_lat) {
        sorted_by_lat = ww_nodes_get_sorted_by_lat(ri);
    }

    int min_lat_index, i, *result;
    RoutingNode *nd, *closest;
    double min_d, max_lat_diff, distance_to_lat_factor;
    List *l, *nodes;
        
    distance_to_lat_factor = 180.0/(M_PI*EARTH_RADIUS);
    max_lat_diff = radius * distance_to_lat_factor;

    nodes = NULL;

    // Get the index of the node with latitude closest to the sought latitude
    min_lat_index = min_lat_bsearch(sorted_by_lat, lat, 0, ri->nrof_nodes-1);
    if (min_lat_index < 0 || min_lat_index >= ri->nrof_nodes)
        return NULL;

    // Check all nodes which could be close enough, given only latitude
    i = min_lat_index;
    nd = sorted_by_lat[i];
    while (i >= 0 && abs(nd->lat - lat) < max_lat_diff) {
        double d;
        d = distance(lat, lon, nd->lat, nd->lon);
        if (d < radius) {
            nodes = list_prepend(nodes, &(nd->id));
        }
        nd = sorted_by_lat[--i];
    }

    i = min_lat_index+1;
    nd = sorted_by_lat[i];
    while (i < ri->nrof_nodes && abs(nd->lat - lat) < max_lat_diff) {
        double d;
        d = distance(lat, lon, nd->lat, nd->lon);
        if (d < radius) {
            nodes = list_prepend(nodes, &(nd->id));
        }
        nd = sorted_by_lat[++i];
    }

    // Build return array
    result = malloc(sizeof(int) * (list_count(nodes)+1));
    for (i = 0, l = nodes; l; i++, l = l->next) {
        int *id;
        id = l->data;
        result[i] = *id;
    }
    result[i] = -1;

    // Free list
    l = nodes;
    while (l) {
        List *ll = l->next;
        free(l);
        l = ll;
    }

    return result;
}

