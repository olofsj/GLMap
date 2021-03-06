
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
#include <triangle.h>

#define BUFF_SIZE 1048576

typedef struct _WayNode WayNode;
typedef struct _Tile Tile;
typedef struct _Vec Vec;
typedef struct _Way Way;
typedef struct _TempRoutingWay TempRoutingWay;
typedef struct _MapWay MapWay;
typedef struct _MapPolygon MapPolygon;

struct _Tile {
    List *polygons;
    List *ways;
    int x;
    int y;
};

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

struct _MapWay {
    int length;
    float width;
    float height;
    unsigned char outline_color[4];
    unsigned char fill_color[4];
    int bridge;
    int tunnel;
    float *vertices;
    RoutingTagSet *tagset;
};

struct _MapPolygon {
    int size;
    unsigned char rgba[4];
    float *vertices;
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
double highway_widths[] = { 
   20.0, // highway_motorway
   16.0, // highway_motorway_link
   16.0, // highway_trunk
   16.0, // highway_trunk_link
   14.0, // highway_primary
   14.0, // highway_primary_link
   12.0, // highway_secondary
   12.0, // highway_secondary_link
   10.0, // highway_tertiary
   10.0, // highway_unclassified
   10.0, // highway_road
   8.0,  // highway_residential
   8.0,  // highway_living_street
   8.0,  // highway_service
   6.0,  // highway_track
   6.0,  // highway_pedestrian
   6.0,  // highway_services
   4.0,  // highway_path
   6.0,  // highway_cycleway
   6.0,  // highway_footway
   6.0,  // highway_bridleway
   6.0,  // highway_byway
   6.0   // highway_steps 
};
double highway_height_offsets[] = { 
   0.0, // highway_motorway
   0.0, // highway_motorway_link
   0.0, // highway_trunk
   0.0, // highway_trunk_link
   0.0, // highway_primary
   0.0, // highway_primary_link
   0.0, // highway_secondary
   0.0, // highway_secondary_link
   0.0, // highway_tertiary
   0.0, // highway_unclassified
   0.0, // highway_road
   0.0,  // highway_residential
   0.0,  // highway_living_street
   0.0,  // highway_service
   -0.1,  // highway_track
   -0.1,  // highway_pedestrian
   0.0,  // highway_services
   -0.2,  // highway_path
   -0.1,  // highway_cycleway
   -0.1,  // highway_footway
   -0.1,  // highway_bridleway
   -0.0,  // highway_byway
   -0.1  // highway_steps 
};
unsigned char highway_fill_colors[] = { 
    225, 225, 225, 255, // highway_motorway
    225, 225, 225, 255, // highway_motorway_link
    225, 225, 225, 255, // highway_trunk
    225, 225, 225, 255, // highway_trunk_link
    225, 225, 225, 255, // highway_primary
    225, 225, 225, 255, // highway_primary_link
    225, 225, 225, 255, // highway_secondary
    225, 225, 225, 255, // highway_secondary_link
    225, 225, 225, 255, // highway_tertiary
    225, 225, 225, 255, // highway_unclassified
    225, 225, 225, 255, // highway_road
    225, 225, 225, 255, // highway_residential
    225, 225, 225, 255, // highway_living_street
    225, 225, 225, 255, // highway_service
    184, 166, 119, 255, // highway_track
    145, 145, 145, 255, // highway_pedestrian
    145, 145, 145, 255, // highway_services
    184, 166, 119, 255, // highway_path
    145, 145, 145, 255, // highway_cycleway
    145, 145, 145, 255, // highway_footway
    184, 166, 119, 255, // highway_bridleway
    145, 145, 145, 255, // highway_byway
    110, 110, 110, 255  // highway_steps 
};
unsigned char highway_outline_colors[] = { 
    194, 194, 194, 255, // highway_motorway
    194, 194, 194, 255, // highway_motorway_link
    194, 194, 194, 255, // highway_trunk
    194, 194, 194, 255, // highway_trunk_link
    194, 194, 194, 255, // highway_primary
    194, 194, 194, 255, // highway_primary_link
    194, 194, 194, 255, // highway_secondary
    194, 194, 194, 255, // highway_secondary_link
    194, 194, 194, 255, // highway_tertiary
    194, 194, 194, 255, // highway_unclassified
    194, 194, 194, 255, // highway_road
    194, 194, 194, 255, // highway_residential
    194, 194, 194, 255, // highway_living_street
    194, 194, 194, 255, // highway_service
    0, 0, 0, 0, // highway_track
    0, 0, 0, 0, // highway_pedestrian
    0, 0, 0, 0, // highway_services
    0, 0, 0, 0, // highway_path
    0, 0, 0, 0, // highway_cycleway
    0, 0, 0, 0, // highway_footway
    0, 0, 0, 0, // highway_bridleway
    0, 0, 0, 0, // highway_byway
    0, 0, 0, 0  // highway_steps 
};

TAG used_polygons[] = { natural_land, natural_water, natural_wetland, natural_wood,
    landuse_farm, landuse_farmland, landuse_farmyard, landuse_forest, 
    landuse_meadow, landuse_orchard, landuse_village_green, landuse_vineyard,
    building_yes
};
int nrof_used_polygons = 13;
unsigned char polygon_colors[] = { 
  250, 245, 233, 255, // natural_land
  180, 203, 220, 255, // natural_water
  180, 203, 220, 255, // natural_wetland
  181, 201, 135, 255, // natural_wood 
  250, 240, 213, 255, // landuse_farm
  250, 240, 213, 255, // landuse_farmland
  250, 240, 213, 255, // landuse_farmyard
  181, 201, 135, 255, // landuse_forest
  200, 216, 162, 255, // landuse_meadow
  200, 216, 162, 255, // landuse_orchard
  200, 216, 162, 255, // landuse_village_green
  200, 216, 162, 255, // landuse_vineyard 
  170, 170, 170, 255  // building_yes
};

/* Global variables */
int depth;
int node_count;
List *node_list;
RoutingNode **nodes;
List *way_list;
List *mapways;
List *polygons;
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

int polygon_type_is_used(Way way) {
    int i,j;

    for (i = 0; i < nrof_used_polygons; i++) {
        for (j = 0; j < way.tagset->size; j++) {
            if (used_polygons[i] == way.tagset->tags[j])
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
    int i, j, index;
    WayNode *cn;
    RoutingNode *nd, *nn, *pn;
    Vec v, u, w, e;
    double a;

    if (!strcmp(el, "way")) {
        if (way_type_is_used(way)) {
            int error = 0;

            // Add the tagset to the index
            int tagset = add_tagset_to_index(way);

            float width = 10.0;
            MapWay* mapway = malloc(sizeof(MapWay));
            mapway->length = 0;
            mapway->width = 5.0;
            mapway->tunnel = 0;
            mapway->bridge = 0;
            mapway->height = 0;
            mapway->outline_color[0] = 0;
            mapway->outline_color[1] = 0;
            mapway->outline_color[2] = 0;
            mapway->outline_color[3] = 0;
            mapway->fill_color[0] = 0;
            mapway->fill_color[1] = 0;
            mapway->fill_color[2] = 0;
            mapway->fill_color[3] = 0;
            for (i = 0; i < nrof_used_highways; i++) {
                for (j = 0; j < way.tagset->size; j++) {
                    if (used_highways[i] == way.tagset->tags[j]) {
                        mapway->width = highway_widths[i];
                        mapway->height += highway_height_offsets[i];
                        mapway->outline_color[0] = highway_outline_colors[4*i+0];
                        mapway->outline_color[1] = highway_outline_colors[4*i+1];
                        mapway->outline_color[2] = highway_outline_colors[4*i+2];
                        mapway->outline_color[3] = highway_outline_colors[4*i+3];
                        mapway->fill_color[0] = highway_fill_colors[4*i+0];
                        mapway->fill_color[1] = highway_fill_colors[4*i+1];
                        mapway->fill_color[2] = highway_fill_colors[4*i+2];
                        mapway->fill_color[3] = highway_fill_colors[4*i+3];
                    }
                }
            }
            // Tunnel, bridge and layer
            for (j = 0; j < way.tagset->size; j++) {
                if (bridge_yes == way.tagset->tags[j]) {
                    mapway->bridge = 1;
                }
                else if (tunnel_yes == way.tagset->tags[j]) {
                    mapway->tunnel = 1;
                }
            }
            if (mapway->tunnel || mapway->bridge) {
                int no_layer = 1;
                for (j = 0; j < way.tagset->size; j++) {
                    if (layer_m5 == way.tagset->tags[j]) {
                        mapway->height += -5.0;
                        no_layer = 0;
                    }
                    else if (layer_m4 == way.tagset->tags[j]) {
                        mapway->height += -4.0;
                        no_layer = 0;
                    }
                    else if (layer_m3 == way.tagset->tags[j]) {
                        mapway->height += -3.0;
                        no_layer = 0;
                    }
                    else if (layer_m2 == way.tagset->tags[j]) {
                        mapway->height += -2.0;
                        no_layer = 0;
                    }
                    else if (layer_m1 == way.tagset->tags[j]) {
                        mapway->height += -1.0;
                        no_layer = 0;
                    }
                    else if (layer_0 == way.tagset->tags[j]) {
                        mapway->height += 0.0;
                        no_layer = 0;
                    }
                    else if (layer_1 == way.tagset->tags[j]) {
                        mapway->height += 1.0;
                        no_layer = 0;
                    }
                    else if (layer_2 == way.tagset->tags[j]) {
                        mapway->height += 2.0;
                        no_layer = 0;
                    }
                    else if (layer_3 == way.tagset->tags[j]) {
                        mapway->height += 3.0;
                        no_layer = 0;
                    }
                    else if (layer_4 == way.tagset->tags[j]) {
                        mapway->height += 4.0;
                        no_layer = 0;
                    }
                    else if (layer_5 == way.tagset->tags[j]) {
                        mapway->height += 5.0;
                        no_layer = 0;
                    }
                }
                if (no_layer) {
                    if (mapway->bridge) mapway->height += 1.0;
                    if (mapway->tunnel) mapway->height -= 1.0;
                }
            }

            mapway->length = 0;
            for (cn = way.start; cn; cn = cn->next)
                mapway->length += 1;
            mapway->vertices = malloc(mapway->length * 2 * sizeof(float));
            i = 0;
            for (cn = way.start; cn; cn = cn->next) {
                // Get the node
                nd = get_node(cn->id);
                if (!nd) {
                    // Node not found in index, abort
                    error = 1;
                    break;
                }
                //mapway->vertices[i++] = scale*(nd->x - center_x);
                //mapway->vertices[i++] = scale*(nd->y - center_y);
                mapway->vertices[i++] = nd->x;
                mapway->vertices[i++] = nd->y;
            }

            if (!error) {
                mapways = list_append(mapways, mapway);
            }
            else {
                free(mapway->vertices);
                free(mapway);
            }
        }
        else if (polygon_type_is_used(way) && way.size > 2) {
            int error = 0;

            int size = way.size - 1; // Last point is repeat of first
            MapPolygon *polygon = malloc(sizeof(MapPolygon));
            polygon->size = size;
            polygon->vertices = malloc(2 * size * sizeof(float));
            for (cn = way.start, i = 0; i < size; cn = cn->next, i++) {
                nd = get_node(cn->id);
                if (!nd) {
                    // Node not found in index, abort
                    error = 1;
                    break;
                }
                polygon->vertices[i*2] = nd->x;
                polygon->vertices[i*2 + 1] = nd->y;
            }
            polygon->rgba[0] = 0;
            polygon->rgba[1] = 0;
            polygon->rgba[2] = 0;
            polygon->rgba[3] = 0;
            for (i = 0; i < nrof_used_polygons; i++) {
                for (j = 0; j < way.tagset->size; j++) {
                    if (used_polygons[i] == way.tagset->tags[j]) {
                        polygon->rgba[0] = polygon_colors[4*i+0];
                        polygon->rgba[1] = polygon_colors[4*i+1];
                        polygon->rgba[2] = polygon_colors[4*i+2];
                        polygon->rgba[3] = polygon_colors[4*i+3];
                    }
                }
            }

            if (error) {
                free(polygon->vertices);
                free(polygon);
            } else {
                polygons = list_append(polygons, polygon);
            }

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
    char *filename;
    int i, j, ti, tj;
    int done;
    int len;
    List *cn, *l;
    RoutingWay *w;
    RoutingNode *nd;
    mapways = NULL;
    polygons = NULL;

    
    printf("Mapgenerator\n");

    if (argc > 1) {
        filename = argv[1];
    } else {
        printf("Input file must be specified.\n");
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

    printf("filesize: %d\n", osmfile.size);


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
    printf("Parsing nodes from XML file...\n");
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
    printf("Sorting list of nodes...\n");
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
    printf("Parsing ways from XML file...\n");
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

    // Calculate array sizes
    l = mapways;
    int nrof_lines = 0;
    int nrof_nodes = 0;
    while (l) {
        MapWay *mapway = l->data;
        l = l->next;
        nrof_nodes += mapway->length;
        nrof_lines++;
    }

    l = polygons;
    int nrof_vertices = 0;
    int nrof_polygons = 0;
    while (l) {
        MapPolygon *polygon = l->data;
        nrof_vertices += polygon->size;
        nrof_polygons++;
        l = l->next;
    }

    // Determine bounding box for all points
    double max_x, max_y, min_x, min_y;
    min_x = nodes[0]->x;
    max_x = nodes[0]->x;
    min_y = nodes[0]->y;
    max_y = nodes[0]->y;
    for (i = 0; i < node_count; i++) {
        if (nodes[i]->x > max_x)
            max_x = nodes[i]->x;
        if (nodes[i]->x < min_x)
            min_x = nodes[i]->x;
        if (nodes[i]->y > max_y)
            max_y = nodes[i]->y;
        if (nodes[i]->y < min_y)
            min_y = nodes[i]->y;
    }
    printf("Bounding box: %lf, %lf, %lf, %lf\n", min_x, min_y, max_x, max_y);

    // Set up the tiles
    double tile_size = 5000.0;
    int start_tile_x = min_x / tile_size;
    int start_tile_y = min_y / tile_size;
    int nrof_tiles_x = ceil(max_x / tile_size) - start_tile_x;
    int nrof_tiles_y = ceil(max_y / tile_size) - start_tile_y;
    int nrof_tiles = nrof_tiles_x * nrof_tiles_y;
    Tile **tiles;
    tiles = malloc(nrof_tiles_x * sizeof(Tile *));
    for (i = 0; i < nrof_tiles_x; i++) {
        tiles[i] = malloc(nrof_tiles_y * sizeof(Tile));
        for (j = 0; j < nrof_tiles_y; j++) {
            tiles[i][j].polygons = NULL;
            tiles[i][j].ways = NULL;
            tiles[i][j].x = start_tile_x + i;
            tiles[i][j].y = start_tile_y + j;
        }
    }

    printf("Splitting data into %dx%d tiles\n", nrof_tiles_x, nrof_tiles_y);

    for (i = 0, l=mapways; i < nrof_lines; i++, l = l->next) {
        MapWay *mapway = l->data;
        // FIXME: Storing the whole way in the tile where the first node is located...
        ti = (int)(mapway->vertices[0]/tile_size) - start_tile_x;
        tj = (int)(mapway->vertices[1]/tile_size) - start_tile_y;
        tiles[ti][tj].ways = list_append(tiles[ti][tj].ways, mapway);
        // FIXME: convert with scale and center coordinates?
    }
    for (i = 0, l=polygons; i < nrof_polygons; i++, l = l->next) {
        MapPolygon *polygon = l->data;
        ti = (int)(polygon->vertices[0]/tile_size) - start_tile_x;
        tj = (int)(polygon->vertices[1]/tile_size) - start_tile_y;
        tiles[ti][tj].polygons = list_append(tiles[ti][tj].polygons, polygon);
    }

    // Write to output files
    FILE *fp;

    for (ti = 0; ti < nrof_tiles_x; ti++) {
        for (tj = 0; tj < nrof_tiles_y; tj++) {
            // Calculate array sizes
            l = tiles[ti][tj].ways;
            int nrof_lines = 0;
            int nrof_nodes = 0;
            while (l) {
                MapWay *mapway = l->data;
                l = l->next;
                nrof_nodes += mapway->length;
                nrof_lines++;
            }

            l = tiles[ti][tj].polygons;
            int nrof_vertices = 0;
            int nrof_polygons = 0;
            while (l) {
                MapPolygon *polygon = l->data;
                nrof_vertices += polygon->size;
                nrof_polygons++;
                l = l->next;
            }

            // Write lines
            char filename[4096];
            snprintf(filename, sizeof(filename)-1, "%d_%d.line", 
                    tiles[ti][tj].x, tiles[ti][tj].y);

            printf("Storing %d lines, %d vertices\n", nrof_lines, nrof_nodes);
            printf("Writing output (%s)...\n", filename);
            fp = fopen(filename, "w");
            if (!fp) {
                fprintf(stderr, "Can't open output file for writing.\n");
                exit(-1);
            }
            fwrite(&nrof_lines, sizeof(int), 1, fp);
            fwrite(&nrof_nodes, sizeof(int), 1, fp);
            for (i = 0, l=tiles[ti][tj].ways; i < nrof_lines; i++, l = l->next) {
                MapWay *mapway = l->data;
                fwrite(&(mapway->length), sizeof(int), 1, fp);
                fwrite(&(mapway->width), sizeof(float), 1, fp);
                fwrite(&(mapway->height), sizeof(float), 1, fp);
                fwrite(&(mapway->outline_color), sizeof(unsigned char), 4, fp);
                fwrite(&(mapway->fill_color), sizeof(unsigned char), 4, fp);
                fwrite(&(mapway->bridge), sizeof(int), 1, fp);
                fwrite(&(mapway->tunnel), sizeof(int), 1, fp);
            }
            for (i = 0, l=tiles[ti][tj].ways; i < nrof_lines; i++, l = l->next) {
                MapWay *mapway = l->data;
                fwrite(mapway->vertices, sizeof(float), 2*mapway->length, fp);
            }
            fclose(fp);


            // Write polygons
            snprintf(filename, sizeof(filename)-1, "%d_%d.poly", 
                    tiles[ti][tj].x, tiles[ti][tj].y);
            printf("Writing output (%s)...\n", filename);
            fp = fopen(filename, "w");
            if (!fp) {
                fprintf(stderr, "Can't open output file for writing.\n");
                exit(-1);
            }
            fwrite(&nrof_polygons, sizeof(int), 1, fp);
            fwrite(&nrof_vertices, sizeof(int), 1, fp);
            for (i = 0, l=tiles[ti][tj].polygons; i < nrof_polygons; i++, l = l->next) {
                MapPolygon *polygon = l->data;
                fwrite(&(polygon->size), sizeof(int), 1, fp);
                fwrite(&(polygon->rgba), sizeof(unsigned char), 4, fp);
            }
            for (i = 0, l=tiles[ti][tj].polygons; i < nrof_polygons; i++, l = l->next) {
                MapPolygon *polygon = l->data;
                fwrite(polygon->vertices, sizeof(float), 2*polygon->size, fp);
            }
            fclose(fp);
        }
    }
}

