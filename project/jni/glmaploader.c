/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "glmaploader.h"
#include "glhelper.h"

void unpackLinesToPolygons(int nrofLines, LineDataFormat *lineData, Vec *points,
        LineVertex *lineVertices, int *nrofLineVertices) {
    int i, j, k;
    int n = 0;
    int ind = 0;
    Vec v, u, w, e;
    GLfloat a;

    for (i = 0; i < nrofLines; i++) {
        int length = lineData[i].length;
        GLfloat width = lineData[i].width;
        GLubyte outline_color[4];
        GLubyte fill_color[4];
        for (k = 0; k < 4; k++) {
            outline_color[k] = lineData[i].outline_color[k];
            fill_color[k] = lineData[i].fill_color[k];
        }
        if (lineData[i].bridge) {
            // Add an outline to all bridges
            outline_color[0] = 144;
            outline_color[1] = 144;
            outline_color[2] = 144;
            outline_color[3] = 255;
        }
        GLfloat z = lineData[i].height;

        // Calculate triangle corners for the given width
        v.x = points[n+1].x - points[n].x;
        v.y = points[n+1].y - points[n].y;
        a = sqrt(v.x*v.x + v.y*v.y);
        v.x = v.x/a;
        v.y = v.y/a;

        u.x = -v.y; u.y = v.x;

        if (!lineData[i].bridge && !lineData[i].tunnel) {
            // Add the first point twice to be able to draw with GL_TRIANGLE_STRIP
            lineVertices[ind].x = points[n].x + u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y + u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = -1.0;
            lineVertices[ind].ty = 1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
            // For rounded line ends
            lineVertices[ind].x = points[n].x + u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y + u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = -1.0;
            lineVertices[ind].ty = 1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
            lineVertices[ind].x = points[n].x - u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y - u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = 1.0;
            lineVertices[ind].ty = 1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
        } else {
            // Add the first point twice to be able to draw with GL_TRIANGLE_STRIP
            lineVertices[ind].x = points[n].x + u.x*width;
            lineVertices[ind].y = points[n].y + u.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = -1.0;
            lineVertices[ind].ty = 0.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
        }
        // Start of line
        lineVertices[ind].x = points[n].x + u.x*width;
        lineVertices[ind].y = points[n].y + u.y*width;
        lineVertices[ind].z = z;
        lineVertices[ind].tx = -1.0;
        lineVertices[ind].ty = 0.0;
        for (k = 0; k < 4; k++) {
            lineVertices[ind].fill_color[k] = fill_color[k];
            lineVertices[ind].outline_color[k] = outline_color[k];
        }
        ind++;
        lineVertices[ind].x = points[n].x - u.x*width;
        lineVertices[ind].y = points[n].y - u.y*width;
        lineVertices[ind].z = z;
        lineVertices[ind].tx = 1.0;
        lineVertices[ind].ty = 0.0;
        for (k = 0; k < 4; k++) {
            lineVertices[ind].fill_color[k] = fill_color[k];
            lineVertices[ind].outline_color[k] = outline_color[k];
        }
        ind++;
        n++;

        for (j = 1; j < length-1; j++) {
            // Unit vector pointing back to previous node
            v.x = points[n-1].x - points[n].x;
            v.y = points[n-1].y - points[n].y;
            a = sqrt(v.x*v.x + v.y*v.y);
            v.x = v.x/a;
            v.y = v.y/a;

            // Unit vector pointing forward to next node
            w.x = points[n+1].x - points[n].x;
            w.y = points[n+1].y - points[n].y;
            a = sqrt(w.x*w.x + w.y*w.y);
            w.x = w.x/a;
            w.y = w.y/a;

            // Sum of these two vectors points 
            u.x = v.x + w.x;
            u.y = v.y + w.y;
            a = -w.y*u.x + w.x*u.y;
            if (fabs(a) < 0.01) {
                // Almost straight, use normal vector
                u.x = -w.y; u.y = w.x;
            } else {
                // Normalize u, and project normal vector onto this
                u.x = u.x/a;
                u.y = u.y/a;
            }

            lineVertices[ind].x = points[n].x + u.x*width;
            lineVertices[ind].y = points[n].y + u.y*width;
            lineVertices[ind].z = z;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            lineVertices[ind].tx = -1.0;
            lineVertices[ind].ty = 0.0;
            ind++;
            lineVertices[ind].x = points[n].x - u.x*width;
            lineVertices[ind].y = points[n].y - u.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = 1.0;
            lineVertices[ind].ty = 0.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
            n++;
        }
        v.x = points[n-1].x - points[n].x;
        v.y = points[n-1].y - points[n].y;
        a = sqrt(v.x*v.x + v.y*v.y);
        v.x = v.x/a;
        v.y = v.y/a;

        u.x = v.y; u.y = -v.x;
        lineVertices[ind].x = points[n].x + u.x*width;
        lineVertices[ind].y = points[n].y + u.y*width;
        lineVertices[ind].z = z;
        lineVertices[ind].tx = -1.0;
        lineVertices[ind].ty = 0.0;
        for (k = 0; k < 4; k++) {
            lineVertices[ind].fill_color[k] = fill_color[k];
            lineVertices[ind].outline_color[k] = outline_color[k];
        }
        ind++;
        lineVertices[ind].x = points[n].x - u.x*width;
        lineVertices[ind].y = points[n].y - u.y*width;
        lineVertices[ind].z = z;
        lineVertices[ind].tx = 1.0;
        lineVertices[ind].ty = 0.0;
        for (k = 0; k < 4; k++) {
            lineVertices[ind].fill_color[k] = fill_color[k];
            lineVertices[ind].outline_color[k] = outline_color[k];
        }
        ind++;

        if (!lineData[i].bridge && !lineData[i].tunnel) {
            // For rounded line edges
            lineVertices[ind].x = points[n].x + u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y + u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = -1.0;
            lineVertices[ind].ty = -1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
            lineVertices[ind].x = points[n].x - u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y - u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = 1.0;
            lineVertices[ind].ty = -1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
            // Add the last vertex twice to be able to draw with GL_TRIANGLE_STRIP
            lineVertices[ind].x = points[n].x - u.x*width - v.x*width;
            lineVertices[ind].y = points[n].y - u.y*width - v.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = 1.0;
            lineVertices[ind].ty = -1.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
        } else {
            // Add the last vertex twice to be able to draw with GL_TRIANGLE_STRIP
            lineVertices[ind].x = points[n].x - u.x*width;
            lineVertices[ind].y = points[n].y - u.y*width;
            lineVertices[ind].z = z;
            lineVertices[ind].tx = 1.0;
            lineVertices[ind].ty = 0.0;
            for (k = 0; k < 4; k++) {
                lineVertices[ind].fill_color[k] = fill_color[k];
                lineVertices[ind].outline_color[k] = outline_color[k];
            }
            ind++;
        }
        n++;
    }
    *nrofLineVertices = ind;
}

void unpackPolygons(Tile *tile, int nrofPolygons, PolygonDataFormat *polygonData, Vec *points) {
    int i, j, k, l;
    int srcIdx = 0;
    int tgtIdx = 0;

    tile->nrofPolygonLayers = 0;
    tile->polygonLayers = NULL;

    // Scan through the polygons and set up the needed layers
    for (i = 0; i < nrofPolygons; i++) {
        int found = 0;
        int thisPolygonVertices = polygonData[i].size + 2; 

        for (l = 0; l < tile->nrofPolygonLayers; l++) {
            if (colorIsEqual(tile->polygonLayers[l].rgba, polygonData[i].rgba)) {
                tile->polygonLayers[l].nrofVertices += thisPolygonVertices;
                found = 1;
                break;
            }
        }

        if (!found) {
            tile->nrofPolygonLayers++;
            tile->polygonLayers = realloc(tile->polygonLayers, tile->nrofPolygonLayers * sizeof(PolygonLayer));
            l = tile->nrofPolygonLayers-1;
            tile->polygonLayers[l].nrofVertices = thisPolygonVertices;
            for (k = 0; k < 4; k++) tile->polygonLayers[l].rgba[k] = polygonData[i].rgba[k];
        }

        tile->nrofPolygonVertices += thisPolygonVertices;
    }

    LOGI("Unpacked: %d layers.\n", tile->nrofPolygonLayers);
    for (l = 0; l < tile->nrofPolygonLayers; l++) {
        LOGI("Unpacked: Layer %d has %d vertices.\n", l, tile->polygonLayers[l].nrofVertices);
    }

    // Set up start indices
    tile->polygonLayers[0].startVertex = 0;
    for (l = 1; l < tile->nrofPolygonLayers; l++) {
        tile->polygonLayers[l].startVertex = tile->polygonLayers[l-1].startVertex 
            + tile->polygonLayers[l-1].nrofVertices;
    }

    tile->polygonVertices = malloc(tile->nrofPolygonVertices * sizeof(PolygonVertex));


    // Load all the polygon vertices into the correct layer
    Vec origo = points[0];

    for (l = 0; l < tile->nrofPolygonLayers; l++) {
        srcIdx = 0;

        for (i = 0; i < nrofPolygons; i++) {
            if (colorIsEqual(tile->polygonLayers[l].rgba, polygonData[i].rgba)) {

                tile->polygonVertices[tgtIdx].x = origo.x;
                tile->polygonVertices[tgtIdx].y = origo.y;
                tgtIdx++;

                Vec startVertex = points[srcIdx];

                for (j = 0; j < polygonData[i].size; j++) {
                    tile->polygonVertices[tgtIdx].x = points[srcIdx + j].x;
                    tile->polygonVertices[tgtIdx].y = points[srcIdx + j].y;
                    tgtIdx++;
                }

                tile->polygonVertices[tgtIdx].x = startVertex.x;
                tile->polygonVertices[tgtIdx].y = startVertex.y;
                tgtIdx++;

            }

            srcIdx += polygonData[i].size;
        }

    }

    LOGI("Unpacked: %d polygon vertices.\n", tile->nrofPolygonVertices);
}

int loadMapTile(char *tilename, Tile *tile) {
    // Load map data from files
    FILE *fp;
    int fd;
    int filesize;
    int bytes_read;
    void *filecontent;
    char filename[4096];
    char tiledir[] = "/sdcard/GLMap/tiles";
    int nrofLines = 0;
    int nrofPolygons = 0;
    int nrofPolygonVertices = 0;
    int i, k;
    struct stat st;

    // Read in line data
    snprintf(filename, sizeof(filename)-1, "%s/%s.line", tiledir, tilename);
    LOGI("Reading map line data from file '%s'.\n", filename);

    /* Open file descriptor and stat the file to get size */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        // TODO Error, do something here
    }
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        // TODO Error, handle
    }
    filesize = st.st_size;

    /* mmap file contents */
    filecontent = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if ((filecontent == MAP_FAILED) || (filecontent == NULL))
    {
        close(fd);
        // TODO Error handle
    }

    int nrofLinePoints;
    LineDataFormat *lineData;
    GLfloat *linePoints;
    nrofLines = *(int *)(filecontent);
    nrofLinePoints = *(int *)(filecontent + sizeof(int));
    LOGI("Found: %d lines, %d vertices.\n", nrofLines, nrofLinePoints);

    lineData = (filecontent + 2*sizeof(int));
    linePoints = (filecontent + 2*sizeof(int) + nrofLines * sizeof(LineDataFormat));

    LOGI("Finished reading.\n");

    // For each line, we get at most twice the number of points, plus one extra node in the beginning and end
    tile->nrofLineVertices = 2*nrofLinePoints + 6*nrofLines;
    if (tile->lineVertices)
        free(tile->lineVertices);
    tile->lineVertices = malloc(tile->nrofLineVertices * sizeof(LineVertex));
    LOGI("Parsing map line data.\n");
    unpackLinesToPolygons(nrofLines, lineData, (Vec *)linePoints, tile->lineVertices, &tile->nrofLineVertices);
    LOGI("Finished parsing.\n");

    munmap(filecontent, filesize);
    close(fd);

    // Read in polygon data
    snprintf(filename, sizeof(filename)-1, "%s/%s.poly", tiledir, tilename);
    LOGI("Reading map polygon data from file '%s'.\n", filename);

    /* Open file descriptor and stat the file to get size */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        // TODO Error, do something here
    }
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        // TODO Error, handle
    }
    filesize = st.st_size;

    /* mmap file contents */
    filecontent = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if ((filecontent == MAP_FAILED) || (filecontent == NULL))
    {
        close(fd);
        // TODO Error handle
    }
    nrofPolygons = *(int *)(filecontent);
    nrofPolygonVertices = *(int *)(filecontent + sizeof(int));
    LOGI("Found: %d polygons, %d vertices.\n", nrofPolygons, nrofPolygonVertices);

    PolygonDataFormat *polygonData;
    GLfloat *vertices;
    polygonData = (filecontent + 2*sizeof(int));
    vertices = (filecontent + 2*sizeof(int) + nrofPolygons*sizeof(PolygonDataFormat));
    LOGI("Finished reading.\n");

    LOGI("Parsing map polygon data.\n");
    if (tile->polygonVertices) 
        free(tile->polygonVertices);

    unpackPolygons(tile, nrofPolygons, polygonData, (Vec *)vertices);
    LOGI("Finished parsing.\n");

    munmap(filecontent, filesize);
    close(fd);

    return 0;
}

