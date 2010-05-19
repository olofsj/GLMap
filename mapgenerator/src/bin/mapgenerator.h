
#ifndef MAPGENERATOR_H_
#define MAPGENERATOR_H_

typedef struct _RoutingNode RoutingNode;
typedef struct _Route Route;
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingWay RoutingWay;
typedef struct _RoutingTagSet RoutingTagSet;
typedef struct _RoutingProfile RoutingProfile;
typedef struct _File File;
typedef struct _List List;
typedef int (*List_Compare_Cb) (const void *a, const void *b);

typedef enum { highway_motorway, highway_motorway_link, highway_trunk,
    highway_trunk_link, highway_primary, highway_primary_link,
    highway_secondary, highway_secondary_link, highway_tertiary,
    highway_unclassified, highway_road, highway_residential,
    highway_living_street, highway_service, highway_track, highway_pedestrian,
    highway_raceway, highway_services, highway_bus_guideway, highway_path,
    highway_cycleway, highway_footway, highway_bridleway, highway_byway,
    highway_steps, highway_mini_roundabout, highway_stop,
    highway_traffic_signals, highway_crossing, highway_motorway_junction,
    highway_incline, highway_incline_steep, highway_ford, highway_bus_stop,
    highway_turning_circle, highway_construction, highway_proposed,
    highway_emergency_access_point, highway_speed_camera, traffic_calming_yes,
    traffic_calming_bump, traffic_calming_chicane, traffic_calming_cushion,
    traffic_calming_hump, traffic_calming_rumble_strip, traffic_calming_table,
    traffic_calming_choker, smoothness_excellent, smoothness_good,
    smoothness_intermediate, smoothness_bad, smoothness_very_bad,
    smoothness_horrible, smoothness_very_horrible, smoothness_impassable,
    landuse_allotments, landuse_basin, landuse_brownfield, landuse_cemetery,
    landuse_commercial, landuse_construction, landuse_farm, landuse_farmland,
    landuse_farmyard, landuse_forest, landuse_garages, landuse_grass,
    landuse_greenfield, landuse_greenhouse_horticulture, landuse_industrial,
    landuse_landfill, landuse_meadow, landuse_military, landuse_orchard,
    landuse_quarry, landuse_railway, landuse_recreation_ground, landuse_reservoir,
    landuse_residential, landuse_retail, landuse_salt_pond, landuse_village_green,
    landuse_vineyard, natural_bay, natural_beach, natural_cave_entrance,
    natural_cliff, natural_coastline, natural_fell, natural_glacier,
    natural_heath, natural_land, natural_marsh, natural_mud, natural_peak,
    natural_sand, natural_scree, natural_scrub, natural_spring, natural_stone,
    natural_tree, natural_volcano, natural_water, natural_wetland, natural_wood,
    bridge_yes, tunnel_yes, layer_m5, layer_m4, layer_m3, layer_m2, layer_m1,
    layer_0, layer_1, layer_2, layer_3, layer_4, layer_5, building_yes
} TAG;
#define TAG_KEYS { "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "landuse", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "natural", "bridge", "tunnel", "layer", "layer", "layer", "layer", "layer", "layer", "layer", "layer", "layer", "layer", "layer", "building" }
#define TAG_VALUES { "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "road", "residential", "living_street", "service", "track", "pedestrian", "raceway", "services", "bus_guideway", "path", "cycleway", "footway", "bridleway", "byway", "steps", "mini_roundabout", "stop", "traffic_signals", "crossing", "motorway_junction", "incline", "incline_steep", "ford", "bus_stop", "turning_circle", "construction", "proposed", "emergency_access_point", "speed_camera", "yes", "bump", "chicane", "cushion", "hump", "rumble_strip", "table", "choker", "excellent", "good", "intermediate", "bad", "very_bad", "horrible", "very_horrible", "impassable", "allotments", "basin", "brownfield", "cemetery", "commercial", "construction", "farm", "farmland", "farmyard", "forest", "garages", "grass", "greenfield", "greenhouse_horticulture", "industrial", "landfill", "meadow", "military", "orchard", "quarry", "railway", "recreation_ground", "reservoir", "residential", "retail", "salt_pond", "village_green", "vineyard", "bay", "beach", "cave_entrance", "cliff", "coastline", "fell", "glacier", "heath", "land", "marsh", "mud", "peak", "sand", "scree", "scrub", "spring", "stone", "tree", "volcano", "water", "wetland", "wood", "yes", "yes", "-5", "-4", "-3", "-2", "-1", "0", "1", "2", "3", "4", "5", "yes" }
#define NROF_TAGS 119


struct _RoutingNode {
    unsigned int id;       // OSM id
    struct {
        unsigned int start; // The index of the first way leading from this node
        unsigned int end;   // The index of the first way not belonging to this node
    } way;
    double lat;
    double lon;
    double x;
    double y;
} __attribute__ ((__packed__));

struct _Route {
    int nrof_nodes;
    double length;
    RoutingNode *nodes;
};

struct _RoutingIndex {
    unsigned int nrof_ways;
    unsigned int nrof_nodes;
    RoutingWay *ways;
    RoutingNode *nodes;
    RoutingTagSet *tagsets;
};

struct _RoutingWay {
    unsigned int next; // The index of the node this leads to
    unsigned char tagset;
} __attribute__ ((__packed__));

struct _RoutingTagSet {
    unsigned int size;
    TAG tags[0];
} __attribute__ ((__packed__));

struct _RoutingProfile {
    char *name;
    double penalty[NROF_TAGS]; // A penalty for each tag
    double max_route_length; // Give up if no route shorter than this is found
};

struct _File {
    char *file;
    int fd;
    char *content;
    int size;
};

struct _List {
    List *prev;
    List *next;
    void *data;
};


double distance(double from_lat, double from_lon, double to_lat, double to_lon);
double effective_distance(RoutingProfile *profile, RoutingTagSet *tagset, 
        double from_lat, double from_lon, double to_lat, double to_lon);
List * list_sorted_insert(List *list, void *data, List_Compare_Cb compare);
List * list_sorted_merge(List *list1, List *list2, List_Compare_Cb compare);
List * list_merge_sort(List *list, int size, List_Compare_Cb compare);
List * list_sort(List *list, List_Compare_Cb compare);
List * list_prepend(List *list, void *data);
List * list_append(List *list, void *data);
List * list_find(List *list, void *data, List_Compare_Cb compare);
int list_count(List *list);

int routing_index_bsearch(RoutingNode* nodes, int id, int low, int high);
int routing_index_find_node(RoutingIndex* ri, int id);

#endif /* MAPGENERATOR_H_ */
