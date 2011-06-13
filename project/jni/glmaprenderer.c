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

#include "glhelper.h"
#include "glmaploader.h"
#include "glmaprenderer.h"
#include "shaders.h"

#define NROF_TILES_X 2
#define NROF_TILES_Y 2
#define NROF_TILES NROF_TILES_X*NROF_TILES_Y

GLuint gLineProgram;
GLuint gLinevPositionHandle;
GLuint gLinetexPositionHandle;
GLuint gLineColorHandle;
GLuint gLinecPositionHandle;
GLuint gLineWidthHandle;
GLuint gLineHeightOffsetHandle;
GLuint gLineScaleXHandle;
GLuint gLineScaleYHandle;
GLuint gPolygonProgram;
GLuint gPolygonvPositionHandle;
GLuint gPolygoncPositionHandle;
GLuint gPolygonScaleXHandle;
GLuint gPolygonScaleYHandle;
GLuint gPolygonFillProgram;
GLuint gPolygonFillvPositionHandle;
GLuint gPolygonFillColorHandle;
double xPos = 59.4;
double yPos = 17.87;
double zPos = 10.0;
double tile_size = 5000.0;
int width, height;
Tile **tiles;
static const GLfloat fullscreenCoords[] = {
    -1.0, 1.0, 
    1.0, 1.0, 
    -1.0, -1.0, 
    1.0, -1.0
};


int mapInit() {
    int i, j;
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    // Set up the program for rendering lines
    gLineProgram = createProgram(gLineVertexShader, gLineFragmentShader);
    if (!gLineProgram) {
        LOGE("Could not create program.");
        return 1;
    }
    gLinecPositionHandle = glGetUniformLocation(gLineProgram, "u_center");
    gLineScaleXHandle = glGetUniformLocation(gLineProgram, "scaleX");
    gLineScaleYHandle = glGetUniformLocation(gLineProgram, "scaleY");
    gLineHeightOffsetHandle = glGetUniformLocation(gLineProgram, "height_offset");
    gLineWidthHandle = glGetUniformLocation(gLineProgram, "width");
    gLinevPositionHandle = glGetAttribLocation(gLineProgram, "a_position");
    gLinetexPositionHandle = glGetAttribLocation(gLineProgram, "a_st");
    gLineColorHandle = glGetAttribLocation(gLineProgram, "a_color");
    checkGlError("glGetAttribLocation");

    // Set up the program for rendering polygons
    gPolygonProgram = createProgram(gPolygonVertexShader, gPolygonFragmentShader);
    if (!gPolygonProgram) {
        LOGE("Could not create program.");
        return 1;
    }
    gPolygoncPositionHandle = glGetUniformLocation(gPolygonProgram, "u_center");
    gPolygonScaleXHandle = glGetUniformLocation(gPolygonProgram, "scaleX");
    gPolygonScaleYHandle = glGetUniformLocation(gPolygonProgram, "scaleY");
    gPolygonvPositionHandle = glGetAttribLocation(gPolygonProgram, "a_position");
    checkGlError("glGetUniformLocation");

    // Set up the program for filling polygons
    gPolygonFillProgram = createProgram(gPolygonFillVertexShader, gPolygonFillFragmentShader);
    if (!gPolygonFillProgram) {
        LOGE("Could not create program.");
        return 1;
    }
    gPolygonFillvPositionHandle = glGetAttribLocation(gPolygonFillProgram, "a_position");
    gPolygonFillColorHandle = glGetUniformLocation(gPolygonFillProgram, "u_color");
    checkGlError("glGetUniformLocation");

    // Set up vertex buffer objects 
    GLuint vboIds[2*NROF_TILES];
    glGenBuffers(2*NROF_TILES, vboIds);

    // Set up the tile handles
    tiles = malloc(NROF_TILES_X * sizeof(Tile *));
    for (i = 0; i < NROF_TILES_X; i++) {
        tiles[i] = malloc(NROF_TILES_Y * sizeof(Tile));
        for (j = 0; j < NROF_TILES_Y; j++) {
            int n = i + j*NROF_TILES_X;
            tiles[i][j].lineVBO = vboIds[2*n];
            tiles[i][j].polygonVBO = vboIds[2*n+1];
            tiles[i][j].nrofLineVertices = 0;
            tiles[i][j].nrofPolygonVertices = 0;
            tiles[i][j].nrofPolygonLayers = 0;
            tiles[i][j].x = -1;
            tiles[i][j].y = -1;
            tiles[i][j].newData = 0;
            tiles[i][j].polygonLayers = NULL;
            tiles[i][j].lineVertices = NULL;
            tiles[i][j].polygonVertices = NULL;
        }
    }

    // Set general settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);

    LOGI("Initialization complete.\n");

    return 0;
}

int mapSetWindowSize(int w, int h) {
    LOGI("setWindowSize(%d, %d)", w, h);
    width = w;
    height = h;

    // Set up viewport
    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    return 0;
}

int mapMove(double x, double y, double z) {
    int i, j;
    char tilename[256];

    xPos = x;
    yPos = y;
    zPos = z;

    // Check if any new tiles need to be loaded
    for (i = 0; i < NROF_TILES_X; i++) {
        for (j = 0; j < NROF_TILES_Y; j++) {
            int tx, ty, s, t;

            tx = (x - 0.5*tile_size) / tile_size + i;
            ty = (y - 0.5*tile_size) / tile_size + j;

            s = tx % NROF_TILES_X;
            t = ty % NROF_TILES_Y;
            if (tiles[s][t].x != tx || tiles[s][t].y != ty) {
                // Load a tile from disk
                snprintf(tilename, sizeof(tilename)-1, "%d_%d", tx, ty);
                loadMapTile(tilename, &tiles[s][t]);
                tiles[s][t].x = tx;
                tiles[s][t].y = ty;
                tiles[s][t].newData = 1;
            }
        }
    }

    return 0;
}

void mapRenderFrame() {
    double x, y, z;
    int i, j, l;
    char tilename[256];

    x = xPos;
    y = yPos;
    z = zPos;

    // Check if any new tiles need to be loaded into graphics memory
    for (i = 0; i < NROF_TILES_X; i++) {
        for (j = 0; j < NROF_TILES_Y; j++) {
            if (tiles[i][j].newData) {
                // Upload line data to graphics core vertex buffer object
                glBindBuffer(GL_ARRAY_BUFFER, tiles[i][j].lineVBO);
                glBufferData(GL_ARRAY_BUFFER, tiles[i][j].nrofLineVertices * sizeof(LineVertex),
                        tiles[i][j].lineVertices, GL_DYNAMIC_DRAW);

                // Upload polygon data to graphics core vertex buffer object
                glBindBuffer(GL_ARRAY_BUFFER, tiles[i][j].polygonVBO);
                glBufferData(GL_ARRAY_BUFFER, tiles[i][j].nrofPolygonVertices * sizeof(PolygonVertex),
                        tiles[i][j].polygonVertices, GL_DYNAMIC_DRAW);

                tiles[i][j].newData = 0;
            }
        }
    }

    // Clear the buffers
    glClearColor(0.98039, 0.96078, 0.91373, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Draw polygons
    for (i = 0; i < NROF_TILES_X; i++) {
        for (j = 0; j < NROF_TILES_Y; j++) {
            for (l = 0; l < tiles[i][j].nrofPolygonLayers; l++) {
                PolygonLayer *layer = &tiles[i][j].polygonLayers[l];

                // Draw into stencil buffer to find covered areas
                // This uses the method described here:
                // http://www.glprogramming.com/red/chapter14.html#name13

                glUseProgram(gPolygonProgram);
                glUniform4f(gPolygoncPositionHandle, x, y, 0.0, 0.0);
                glUniform1f(gPolygonScaleXHandle, z*(float)(height)/(float)(width));
                glUniform1f(gPolygonScaleYHandle, z);

                glBindBuffer(GL_ARRAY_BUFFER, tiles[i][j].polygonVBO);
                glVertexAttribPointer(gPolygonvPositionHandle, 2, GL_FLOAT, GL_FALSE,
                        0, BUFFER_OFFSET(0));
                glEnableVertexAttribArray(gPolygonvPositionHandle);

                glEnable(GL_STENCIL_TEST);
                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_FALSE);

                glStencilFunc(GL_NEVER, 0, 1);
                glStencilOp(GL_INVERT, GL_INVERT, GL_INVERT);

                glDrawArrays(GL_TRIANGLE_FAN, layer->startVertex, layer->nrofVertices);

                // Draw with the color to fill them

                glUseProgram(gPolygonFillProgram);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glUniform4f(gPolygonFillColorHandle,
                        (GLfloat)(layer->rgba[0])/255.0,
                        (GLfloat)(layer->rgba[1])/255.0,
                        (GLfloat)(layer->rgba[2])/255.0,
                        (GLfloat)(layer->rgba[3])/255.0);
                glVertexAttribPointer(gPolygonFillvPositionHandle, 2, GL_FLOAT, GL_FALSE, 
                        0, fullscreenCoords);
                glEnableVertexAttribArray(gPolygonFillvPositionHandle);

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);

                glStencilFunc(GL_EQUAL, 1, 1);
                glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glDisable(GL_STENCIL_TEST);
            }
        }
    }

    // Draw lines
    glUseProgram(gLineProgram);
    checkGlError("glUseProgram");

    glUniform4f(gLinecPositionHandle, x, y, 0.0, 0.0);
    glUniform1f(gLineScaleXHandle, z*(float)(height)/(float)(width));
    glUniform1f(gLineScaleYHandle, z);

    for (i = 0; i < NROF_TILES_X; i++) {
        for (j = 0; j < NROF_TILES_Y; j++) {

            glBindBuffer(GL_ARRAY_BUFFER, tiles[i][j].lineVBO);

            glVertexAttribPointer(gLinevPositionHandle, 3, GL_FLOAT, GL_FALSE, 
                    sizeof(LineVertex), BUFFER_OFFSET(0));
            glEnableVertexAttribArray(gLinevPositionHandle);
            glVertexAttribPointer(gLinetexPositionHandle, 2, GL_FLOAT, GL_FALSE, 
                    sizeof(LineVertex), BUFFER_OFFSET(12));
            glEnableVertexAttribArray(gLinetexPositionHandle);

            // Draw outlines
            glVertexAttribPointer(gLineColorHandle, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                    sizeof(LineVertex), BUFFER_OFFSET(20));
            glEnableVertexAttribArray(gLineColorHandle);
            glUniform1f(gLineWidthHandle, 1.0);
            glUniform1f(gLineHeightOffsetHandle, 0.0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, tiles[i][j].nrofLineVertices);

            // Draw fill
            glVertexAttribPointer(gLineColorHandle, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                    sizeof(LineVertex), BUFFER_OFFSET(24));
            glEnableVertexAttribArray(gLineColorHandle);
            glUniform1f(gLineWidthHandle, 0.50);
            glUniform1f(gLineHeightOffsetHandle, 0.0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, tiles[i][j].nrofLineVertices);
            checkGlError("glDrawArrays lines");
        }
    }
}

