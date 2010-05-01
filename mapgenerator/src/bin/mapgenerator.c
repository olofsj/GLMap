
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <expat.h>
#include <math.h>
#include "mapgenerator.h"
#include <proj_api.h>

#define BUFF_SIZE 1048576

typedef struct _WayNode WayNode;
typedef struct _Vec Vec;
typedef struct _Way Way;
typedef struct _TempRoutingWay TempRoutingWay;

struct _Vec {
    double x;
    double y;
};

struct _WayNode {
    int id;
    WayNode *next;
    WayNode *prev;
};

struct _Way {
    WayNode *start;
    WayNode *end;
    int oneway;
    int size;
    RoutingTagSet *tagset;
};

struct _TempRoutingWay {
    int node_id;
    RoutingWay *way;
};

char *tag_keys[] = TAG_KEYS;
char *tag_values[] = TAG_VALUES;

TAG used_highways[] = { highway_motorway, highway_motorway_link, highway_trunk,
    highway_trunk_link, highway_primary, highway_primary_link,
    highway_secondary, highway_secondary_link, highway_tertiary,
    highway_unclassified, highway_road, highway_residential,
    highway_living_street, highway_service, highway_track, highway_pedestrian,
    highway_services, highway_path, highway_cycleway, highway_footway,
    highway_bridleway, highway_byway, highway_steps };
int nrof_used_highways = 23;

/* Global variables */
int depth;
int node_count;
List *node_list;
RoutingNode **nodes;
List *way_list;
List *lengths;
Way way;
int *tagsetindex;
RoutingTagSet *tagsets;
int tagsetsize;
int nrof_tagsets;
projPJ pj_merc, pj_latlong;

double center_x = 1991418.0;
double center_y = 8267328.0;
double scale = 1.0;
 
int
node_sort_cb(const void *n1, const void *n2)
{
    const RoutingNode *m1 = NULL;
    const RoutingNode *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->id > m2->id)
        return 1;
    if (m1->id < m2->id)
        return -1;
    return 0;
}

int
way_sort_cb(const void *n1, const void *n2)
{
    const TempRoutingWay *m1 = NULL;
    const TempRoutingWay *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->node_id > m2->node_id)
        return 1;
    if (m1->node_id < m2->node_id)
        return -1;
    return 0;
}

RoutingNode *
node_find(RoutingNode** nodes, int id, int low, int high) {
    int mid;

    if (high < low)
        return NULL; // not found

    mid = low + ((high - low) / 2);
    if (nodes[mid]->id > id) {
        return node_find(nodes, id, low, mid-1);
    } else if (nodes[mid]->id < id) {
        return node_find(nodes, id, mid+1, high);
    } else {
        return nodes[mid];
    }
}

RoutingNode *get_node(int id) {
    return node_find(nodes, id, 0, node_count-1);
}


void
nodeparser_start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "node")) {
      RoutingNode *node;

      node_count++;

      node = malloc(sizeof(RoutingNode));
      node->way.start = 0;
      node->way.end = 0;

      /* Check all the attributes for this node */
      for (i = 0; attr[i]; i += 2) {
          if (!strcmp(attr[i], "id")) 
              sscanf(attr[i+1], "%d", &(node->id));
          if (!strcmp(attr[i], "lat")) 
              sscanf(attr[i+1], "%lf", &(node->lat));
          if (!strcmp(attr[i], "lon")) 
              sscanf(attr[i+1], "%lf", &(node->lon));
      }

      // Convert to Spherical Mercator projection
      node->x = node->lon * DEG_TO_RAD;
      node->y = node->lat * DEG_TO_RAD;
      pj_transform(pj_latlong, pj_merc, 1, 1, &(node->x), &(node->y), NULL );

      node_list = list_prepend(node_list, node);
  }

  depth++;
}

void
nodeparser_end(void *data, const char *el) {

    depth--;
}

void
wayparser_start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "way")) {
      way.size = 0;
      way.start = NULL;
      way.end = NULL;
      way.oneway = 0;
      way.tagset = malloc(sizeof(RoutingTagSet));
      way.tagset->size = 0;
  }
  else if (!strcmp(el, "tag") && way.size != -1) {
      if (!strcmp(attr[0], "k") && !strcmp(attr[1], "oneway") &&
              !strcmp(attr[2], "v") && 
              (!strcmp(attr[3], "yes") || !strcmp(attr[3], "true")) ) {
          // Current way is oneway
          way.oneway = 1;
      }
      // Add recognized tags
      for (i = 0; i < NROF_TAGS; i++) {
          if (!strcmp(attr[0], "k") && !strcmp(attr[1], tag_keys[i]) &&
                  !strcmp(attr[2], "v") && !strcmp(attr[3], tag_values[i])) {
              way.tagset->size++;
              way.tagset = realloc(way.tagset, sizeof(RoutingNode) + way.tagset->size*sizeof(TAG));
              way.tagset->tags[way.tagset->size-1] = i;
          }
      }
  }
  else if (!strcmp(el, "nd") && way.size != -1) {
      // Add a node to the current way
      way.size++;
      if (!way.start) {
          way.start = malloc(sizeof(WayNode));
          way.end = way.start;
          way.start->prev = NULL;
          way.start->next = NULL;
      } else {
          way.end->next = malloc(sizeof(WayNode));
          way.end->next->next = NULL;
          way.end->next->prev = way.end;
          way.end = way.end->next;
      }

      /* Check all the attributes for this node */
      for (i = 0; attr[i]; i += 2) {
          if (!strcmp(attr[i], "ref")) 
              sscanf(attr[i+1], "%d", &(way.end->id));
      }
  }

  depth++;
}

int way_type_is_used(Way way) {
    int i,j;

    for (i = 0; i < nrof_used_highways; i++) {
        for (j = 0; j < way.tagset->size; j++) {
            if (used_highways[i] == way.tagset->tags[j])
                return 1;
        }
    }
    
    return 0;
}

int add_tagset_to_index(Way way) {
    int i, j, k;
    
    // Check if such a tagset exists in the index
    for (i = 0; i < nrof_tagsets; i++) {
        RoutingTagSet *ts = (void *)tagsets + tagsetindex[i];
        if (ts->size == way.tagset->size) {
            int is_same = 1;
            for (j = 0; j < way.tagset->size; j++) {
                int has_tag = 0;
                for (k = 0; k < ts->size; k++) {
                    if (ts->tags[k] == way.tagset->tags[j])
                        has_tag = 1;
                }
                if (has_tag == 0)
                    is_same = 0;
            }
            if (is_same) 
                return tagsetindex[i];
        }
    }

    // Not found, add a new tagset to the end of the index
    int newtagsetsize = tagsetsize + sizeof(RoutingTagSet) + way.tagset->size*sizeof(TAG);
    tagsets = realloc(tagsets, newtagsetsize);

    nrof_tagsets++;
    tagsetindex = realloc(tagsetindex, nrof_tagsets*sizeof(int));
    tagsetindex[nrof_tagsets-1] = tagsetsize;
    RoutingTagSet *ts = (void *)tagsets + tagsetindex[nrof_tagsets-1];

    memcpy(ts, way.tagset, sizeof(RoutingTagSet) + way.tagset->size*sizeof(TAG));
    tagsetsize = newtagsetsize;

    return tagsetindex[nrof_tagsets-1];
}

void
wayparser_end(void *data, const char *el) {
    int i, index;
    WayNode *cn;
    RoutingNode *nd, *nn, *pn;
    Vec v, u, w, e;
    double a;

    if (!strcmp(el, "way")) {
        if (way_type_is_used(way)) {
            // Add the tagset to the index
            int tagset = add_tagset_to_index(way);

            float width = 10.0;
            int* length = malloc(sizeof(int));
            *length = 0;

            // Calculate triangle corners for the given width
            for (cn = way.start; cn; cn = cn->next) {
                // Get the node
                nd = get_node(cn->id);
                printf("%ff, %ff, ", scale*(nd->x - center_x), scale*(nd->y - center_y));
                *length += 1;
            }

            lengths = list_append(lengths, length);
        }

        // Free the nodes
        free(way.tagset);
        cn = way.start;
        while (cn) {
            WayNode *next;
            next = cn->next;
            free(cn);
            cn = next;
        }
        way.size = -1;
    }

    depth--;
}


int
main(int argc, char **argv)
{
    File osmfile;
    FILE *osmfilepointer;
    struct stat st;
    char *filename, *outfilename;
    int i, j;
    int done;
    int len;
    List *cn, *l;
    RoutingWay *w;
    RoutingNode *nd;
    lengths = NULL;

    
    //printf("Whichway Create Index\n");

    if (argc > 2) {
        filename = argv[1];
        outfilename = argv[2];
    } else {
        //printf("Input and output files must be specified.\n");
        return 0;
    }

    // Initialize projections
    if (!(pj_merc = pj_init_plus("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs")) ) {
        printf("Can't init Spherical Mercator projection.");
        exit(1);
    }
    if (!(pj_latlong = pj_init_plus("+proj=latlong +datum=WGS84")) ) {
        printf("Can't init latlong projection");
        exit(1);
    }


    osmfile.file = strdup(filename);
    osmfile.fd = -1;
    osmfile.size = 0;

    /* Open file descriptor and stat the file to get size */
    osmfile.fd = open(osmfile.file, O_RDONLY);
    if (osmfile.fd < 0)
        return 0;
    if (fstat(osmfile.fd, &st) < 0) {
        close(osmfile.fd);
        return 0;
    }
    osmfile.size = st.st_size;

    osmfilepointer = fopen(osmfile.file, "r");
    if (!osmfilepointer) {
        fprintf(stderr, "Can't open file\n");
        exit(-1);
    }

    //printf("filesize: %d\n", osmfile.size);


    // Set up an XML parser for the nodes
    XML_Parser nodeparser = XML_ParserCreate(NULL);
    if (!nodeparser) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(nodeparser, nodeparser_start, nodeparser_end);

    depth = 0;
    node_count = 0;
    node_list = NULL;

    /* Parse the XML document */
    //printf("Parsing nodes from XML file...\n");
    for (;;) {
        int bytes_read;
        void *buff = XML_GetBuffer(nodeparser, BUFF_SIZE);
        if (!buff) {
            fprintf(stderr, "Couldn't allocate memory for buffer\n");
            exit(-1);
        }
        bytes_read = fread(buff, 1, BUFF_SIZE, osmfilepointer);
        if (bytes_read < 0) {
            fprintf(stderr, "Can't read from file\n");
            exit(-1);
        }

        if (! XML_ParseBuffer(nodeparser, bytes_read, bytes_read == 0)) {
            fprintf(stderr, "Parse error at line %d:\n%s\n",
                    (int)XML_GetCurrentLineNumber(nodeparser),
                    XML_ErrorString(XML_GetErrorCode(nodeparser)));
            exit(-1);
        }

        if (bytes_read == 0)
            break;
    }


    // Create an indexed sorted list of nodes
    //printf("Sorting list of nodes...\n");
    node_list = list_sort(node_list, node_sort_cb);
    
    nodes = malloc(node_count * sizeof(RoutingNode *));
    i = 0;
    cn = node_list;
    while (cn) {
        nodes[i++] = (RoutingNode *)(cn->data);
        cn = cn->next;
    }

    // Set up an XML parser for the ways
    XML_Parser wayparser = XML_ParserCreate(NULL);
    if (!wayparser) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(wayparser, wayparser_start, wayparser_end);

    depth = 0;
    way.size = -1;
    way_list = NULL;
    tagsets = NULL;
    tagsetindex = NULL;
    tagsetsize = 0;
    nrof_tagsets = 0;

    /* Parse the XML document */
    //printf("Parsing ways from XML file...\n");
    fseek(osmfilepointer, 0, SEEK_SET);
    for (;;) {
        int bytes_read;
        void *buff = XML_GetBuffer(wayparser, BUFF_SIZE);
        if (!buff) {
            fprintf(stderr, "Couldn't allocate memory for buffer\n");
            exit(-1);
        }
        bytes_read = fread(buff, 1, BUFF_SIZE, osmfilepointer);
        if (bytes_read < 0) {
            fprintf(stderr, "Can't read from file\n");
            exit(-1);
        }

        if (! XML_ParseBuffer(wayparser, bytes_read, bytes_read == 0)) {
            fprintf(stderr, "Parse error at line %d:\n%s\n",
                    (int)XML_GetCurrentLineNumber(nodeparser),
                    XML_ErrorString(XML_GetErrorCode(nodeparser)));
            exit(-1);
        }

        if (bytes_read == 0)
            break;
    }

    printf("\n");
    l = lengths;
    int nrof_segments = 0;
    int nrof_nodes = 0;
    while (l) {
        int *length = l->data;
        printf("%d, ", *length);
        l = l->next;
        nrof_nodes += *length;
        nrof_segments++;
    }
    printf("\n");
    printf("%d\n", nrof_nodes);
    printf("%d\n", nrof_segments);

}

