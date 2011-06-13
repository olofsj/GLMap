/*
 * Copyright (C) 2011 Olof Sjöbergh
 *
 */

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
    "void main() {\n"
    "  vec4 a = a_position; \n"
    "  a.x = scaleX*(a.x - u_center.x);\n"
    "  a.y = scaleY*(a.y - u_center.y);\n"
    "  a.z = 1.0;\n"
    "  gl_Position = a;\n"
    "}\n";

static const char gPolygonFragmentShader[] = 
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "}\n";

static const char gPolygonFillVertexShader[] = 
    "uniform vec4 u_color;\n"
    "attribute vec4 a_position;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  v_color = u_color;\n"
    "  gl_Position = a_position;\n"
    "}\n";

static const char gPolygonFillFragmentShader[] = 
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_color;\n"
    "}\n";


