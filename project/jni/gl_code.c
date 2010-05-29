/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libglmap"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    GLint error;
    for (error = glGetError(); error; error = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

static const char gLineVertexShader[] = 
    "uniform vec4 u_center;\n"
    "uniform float scaleX;\n"
    "uniform float scaleY;\n"
    "uniform float height_offset;\n"
    "attribute vec4 a_position;\n"
    "attribute vec2 a_st;\n"
    "attribute vec4 a_color;\n"
    "varying vec2 v_st;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  vec4 a = a_position; \n"
    "  a.x = scaleX*(a.x - u_center.x);\n"
    "  a.y = scaleY*(a.y - u_center.y);\n"
    "  a.z = -(a.z + height_offset)/10.0 + 0.5;\n"
    "  v_st = a_st;\n"
    "  v_color = a_color;\n"
    "  gl_Position = a;\n"
    "}\n";

static const char gLineFragmentShader[] = 
    "#extension GL_OES_standard_derivatives : enable\n"
    "precision mediump float;\n"
    "uniform float width;\n"
    "varying vec2 v_st;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  vec2 st_width = fwidth(v_st);\n"
    "  float fuzz = max(st_width.s, st_width.t);\n"
    "  float alpha = 1.0 - smoothstep(width - fuzz, width + fuzz, length(v_st));\n"
    "  vec4 color = v_color * alpha;\n"
    "  if (color.a < 0.2) {\n"
    "    discard;\n"
    "  } else {\n"
    "    gl_FragColor = color;\n"
    "  }\n"
    "}\n";

static const char gPolygonVertexShader[] = 
    "uniform vec4 u_center;\n"
    "uniform float scaleX;\n"
    "uniform float scaleY;\n"
    "attribute vec4 a_position;\n"
    "attribute vec4 a_color;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  vec4 a = a_position; \n"
    "  a.x = scaleX*(a.x - u_center.x);\n"
    "  a.y = scaleY*(a.y - u_center.y);\n"
    "  a.z = 1.0;\n"
    "  v_color = a_color;\n"
    "  gl_Position = a;\n"
    "}\n";

static const char gPolygonFragmentShader[] = 
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_color;\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

#define NROF_VBOS 2

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
GLuint gPolygonColorHandle;
GLuint gPolygoncPositionHandle;
GLuint gPolygonScaleXHandle;
GLuint gPolygonScaleYHandle;
double xPos = 59.4;
double yPos = 17.87;
double zPos = 10.0;
double tile_size = 10000.0;
int width, height;
int nrofPolygonTriangles[NROF_VBOS];
int nrofLineVertices[NROF_VBOS];
GLuint lineVBOs[NROF_VBOS];
GLuint polygonVBOs[NROF_VBOS];
int vboIsUsed[NROF_VBOS];
int current_tile_x = 0;
int current_tile_y = 0;


typedef struct _Vec Vec;
typedef struct _LineVertex LineVertex;
typedef struct _LineDataFormat LineDataFormat;
typedef struct _PolygonVertex PolygonVertex;
typedef struct _PolygonDataFormat PolygonDataFormat;

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
    GLubyte rgba[4];
};
struct _PolygonDataFormat {
    int size;
    GLubyte rgba[4];
};

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

void unpackPolygons(int nrofPolygons, PolygonDataFormat *polygonData, Vec *points, PolygonVertex *polygonVertices) {
    int i, j, k;
    int ind = 0;

    for (i = 0; i < nrofPolygons; i++) {
        for (j = 0; j < 3*polygonData[i].size; j++) {
            polygonVertices[ind].x = points[ind].x;
            polygonVertices[ind].y = points[ind].y;
            for (k = 0; k < 4; k++) polygonVertices[ind].rgba[k] = polygonData[i].rgba[k];
            ind++;
        }
    }
}

int loadMapTile(char *tile, GLuint lineVBO, GLuint polygonVBO, 
        GLuint *nrofPolygonTriangles, GLuint *nrofLineVertices) {
    // Load map data from files
    FILE *fp;
    int bytes_read;
    char filename[4096];
    char tiledir[] = "/sdcard/GLMap/tiles";
    int nrofLines = 0;
    int nrofPolygons = 0;

    // FIXME: Check that file exists etc.
    snprintf(filename, sizeof(filename)-1, "%s/%s.line", tiledir, tile);
    LOGI("Reading map line data from file '%s'.\n", filename);
    fp = fopen(filename, "r");
    int nrofLinePoints;
    bytes_read = fread(&nrofLines, sizeof(int), 1, fp);
    bytes_read = fread(&nrofLinePoints, sizeof(int), 1, fp);
    LOGI("Found: %d lines, %d vertices.\n", nrofLines, nrofLinePoints);

    LineDataFormat *lineData;
    GLfloat *linePoints;
    LineVertex *lineVertices = NULL;
    lineData = malloc(nrofLines * sizeof(LineDataFormat));
    linePoints = malloc(2 * nrofLinePoints * sizeof(GLfloat));
    bytes_read = fread(lineData, sizeof(LineDataFormat), nrofLines, fp);
    bytes_read = fread(linePoints, sizeof(GLfloat), 2*nrofLinePoints, fp);
    fclose(fp);
    LOGI("Finished reading.\n");

    // For each line, we get at most twice the number of points, plus one extra node in the beginning and end
    *nrofLineVertices = 2*nrofLinePoints + 6*nrofLines;
    lineVertices = malloc(*nrofLineVertices * sizeof(LineVertex));
    LOGI("Parsing map line data.\n");
    unpackLinesToPolygons(nrofLines, lineData, (Vec *)linePoints, lineVertices, nrofLineVertices);
    LOGI("Finished parsing.\n");

    // Upload data to graphics core vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, *nrofLineVertices * sizeof(LineVertex),
            lineVertices, GL_DYNAMIC_DRAW);
    //checkGlError("glBufferData line");

    free(lineVertices);
    free(lineData);
    free(linePoints);

    // Read in polygon data
    snprintf(filename, sizeof(filename)-1, "%s/%s.poly", tiledir, tile);
    LOGI("Reading map polygon data from file '%s'.\n", filename);
    fp = fopen(filename, "r");
    bytes_read = fread(&nrofPolygons, sizeof(int), 1, fp);
    bytes_read = fread(nrofPolygonTriangles, sizeof(int), 1, fp);
    LOGI("Found: %d polygons, %d vertices.\n", nrofPolygons, *nrofPolygonTriangles);

    PolygonDataFormat *polygonData;
    GLfloat *vertices;
    PolygonVertex *polygonVertices = NULL;
    polygonData = malloc(nrofPolygons * sizeof(PolygonDataFormat));
    vertices = malloc(6 * *nrofPolygonTriangles * sizeof(GLfloat));
    bytes_read = fread(polygonData, sizeof(PolygonDataFormat), nrofPolygons, fp);
    bytes_read = fread(vertices, sizeof(GLfloat), 6 * *nrofPolygonTriangles, fp);
    fclose(fp);
    LOGI("Finished reading.\n");

    LOGI("Parsing map polygon data.\n");
    polygonVertices = malloc(3 * *nrofPolygonTriangles * sizeof(PolygonVertex));
    unpackPolygons(nrofPolygons, polygonData, (Vec *)vertices, polygonVertices);
    LOGI("Finished parsing.\n");

    // Upload data to graphics core vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, polygonVBO);
    glBufferData(GL_ARRAY_BUFFER, 3 * *nrofPolygonTriangles * sizeof(PolygonVertex),
            polygonVertices, GL_DYNAMIC_DRAW);
    //checkGlError("glBufferData polygon");

    free(polygonVertices);
    free(polygonData);
    free(vertices);

    return 0;
}

int init() {
    int i;
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
    checkGlError("glGetAttribLocation");

    gLinetexPositionHandle = glGetAttribLocation(gLineProgram, "a_st");
    checkGlError("glGetAttribLocation");

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
    checkGlError("glGetAttribLocation");

    gPolygonColorHandle = glGetAttribLocation(gPolygonProgram, "a_color");
    checkGlError("glGetAttribLocation");

    // Set up vertex buffer objects 
    GLuint vboIds[2*NROF_VBOS];
    glGenBuffers(2*NROF_VBOS, vboIds);
    for (i = 0; i < NROF_VBOS; i++) {
        lineVBOs[i] = vboIds[2*i];
        polygonVBOs[i] = vboIds[2*i+1];
        vboIsUsed[i] = 0;
        nrofLineVertices[i] = 0;
        nrofPolygonTriangles[i] = 0;
    }

    return 0;
}

int setWindowSize(int w, int h) {
    LOGI("setWindowSize(%d, %d)", w, h);
    width = w;
    height = h;

    // Set up viewport
    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    return 0;
}

void renderFrame() {
    double x, y, z;
    int i, j;
    x = xPos;
    y = yPos;
    z = zPos;
    char tilename[256];

    i = x / tile_size;
    j = y / tile_size;

    if (i != current_tile_x || j != current_tile_y) {
        int vbo;
        snprintf(tilename, sizeof(tilename)-1, "%d_%d", i, j);

        vbo = 0;
        while (vboIsUsed[vbo]) {
            vbo++;
        }
        vboIsUsed[vbo] = 1;
        vboIsUsed[(vbo+1)%NROF_VBOS] = 0;
        LOGI("Loading tile %s for (%lf, %lf) into VBOs (%d, %d)\n", tilename, x, y,
                lineVBOs[vbo], polygonVBOs[vbo]);

        loadMapTile(tilename, lineVBOs[vbo], polygonVBOs[vbo], 
                &nrofPolygonTriangles[vbo], &nrofLineVertices[vbo]);

        current_tile_x = i;
        current_tile_y = j;

    }

    glClearColor(0.98039, 0.96078, 0.91373, 1.0);

    checkGlError("glClearColor");
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);

    // Draw polygons
    glUseProgram(gPolygonProgram);
    checkGlError("glUseProgram");

    glUniform4f(gPolygoncPositionHandle, x, y, 0.0, 0.0);
    glUniform1f(gPolygonScaleXHandle, z*(float)(height)/(float)(width));
    glUniform1f(gPolygonScaleYHandle, z);

    for (i = 0; i < NROF_VBOS; i++) {
        if (!vboIsUsed[i]) 
            continue;

        glBindBuffer(GL_ARRAY_BUFFER, polygonVBOs[i]);
        checkGlError("glBindBuffer polygon");
        glVertexAttribPointer(gPolygonvPositionHandle, 2, GL_FLOAT, GL_FALSE,
                sizeof(PolygonVertex), BUFFER_OFFSET(0));
        glEnableVertexAttribArray(gPolygonvPositionHandle);
        glVertexAttribPointer(gPolygonColorHandle, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                sizeof(PolygonVertex), BUFFER_OFFSET(8));
        glEnableVertexAttribArray(gPolygonColorHandle);
        checkGlError("glEnableVertexAttribArray polygon");
        glDrawArrays(GL_TRIANGLES, 0, 3*nrofPolygonTriangles[i]);
        checkGlError("glDrawArrays polygon");
    }

    // Draw lines
    glUseProgram(gLineProgram);
    checkGlError("glUseProgram");

    glUniform4f(gLinecPositionHandle, x, y, 0.0, 0.0);
    glUniform1f(gLineScaleXHandle, z*(float)(height)/(float)(width));
    glUniform1f(gLineScaleYHandle, z);

    for (i = 0; i < NROF_VBOS; i++) {
        if (!vboIsUsed[i]) 
            continue;

        glBindBuffer(GL_ARRAY_BUFFER, lineVBOs[i]);
        checkGlError("glBindBuffer line");

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
        glDrawArrays(GL_TRIANGLE_STRIP, 0, nrofLineVertices[i]);

        // Draw fill
        glVertexAttribPointer(gLineColorHandle, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                sizeof(LineVertex), BUFFER_OFFSET(24));
        glEnableVertexAttribArray(gLineColorHandle);
        glUniform1f(gLineWidthHandle, 0.50);
        glUniform1f(gLineHeightOffsetHandle, 0.0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, nrofLineVertices[i]);
        checkGlError("glDrawArrays lines");
    }
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_init(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_setWindowSize(JNIEnv * env, jobject obj,  jint width, jint height);
JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_step(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_move(JNIEnv * env, jobject obj, jdouble x, jdouble y, jdouble z);

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_init(JNIEnv * env, jobject obj)
{
    init();
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_setWindowSize(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setWindowSize(width, height);
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_android_glmap_GLMapLib_move(JNIEnv * env, jobject obj, jdouble x, jdouble y, jdouble z)
{
    xPos = x;
    yPos = y;
    zPos = z;
}
