/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

typedef struct _Tile Tile;
typedef struct _Vec Vec;
typedef struct _LineVertex LineVertex;
typedef struct _LineDataFormat LineDataFormat;
typedef struct _PolygonLayer PolygonLayer;
typedef struct _PolygonVertex PolygonVertex;
typedef struct _PolygonDataFormat PolygonDataFormat;

struct _Tile {
    int x;
    int y;
    GLuint lineVBO;
    GLuint polygonVBO;
    GLuint nrofLineVertices;
    GLuint nrofPolygonLayers;
    GLuint nrofPolygonVertices;
    GLubyte newData;
    PolygonLayer *polygonLayers;
    LineVertex *lineVertices;
    PolygonVertex *polygonVertices;
};

struct _PolygonLayer {
    GLuint startVertex;
    GLuint nrofVertices;
    GLubyte rgba[4];
};

struct _Vec {
    GLfloat x;
    GLfloat y;
};

struct _LineVertex {
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat tx;
    GLfloat ty;
    GLubyte outline_color[4];
    GLubyte fill_color[4];
};

struct _LineDataFormat {
    int length;
    GLfloat width;
    GLfloat height;
    GLubyte outline_color[4];
    GLubyte fill_color[4];
    int bridge;
    int tunnel;
};

struct _PolygonVertex {
    GLfloat x;
    GLfloat y;
};

struct _PolygonDataFormat {
    int size;
    GLubyte rgba[4];
};


int loadMapTile(char *tilename, Tile *tile);

void unpackPolygons(Tile *tile, int nrofPolygons, PolygonDataFormat *polygonData, Vec *points);

void unpackLinesToPolygons(int nrofLines, LineDataFormat *lineData, Vec *points,
        LineVertex *lineVertices, int *nrofLineVertices);

