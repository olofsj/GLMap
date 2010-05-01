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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// For anti-aliased rendering with textures
GLuint glAA_texture[8];        // the AA sphere texIDs
unsigned char *glAA_AAtex[8];  // AA texture buffer

// the point half-width in px. must be a power of two <= 256.
#define phf 32
// point size, size minus one, half size squared, double size, point center, radius squared
#define psz (phf * 2)
#define psm (psz - 1)
#define phs (phf * phf)
#define pdb (psz * 2)
#define pct (psz + phf)
#define prs ((phf-1)*(phf-1))

static float			logtbl[pdb];				// for sphere lookups

void glAATexInit() {
	int i;
	for (i=1; i<=pdb; i++){							// lookup table has 4x the bitmap resolution
		logtbl[i-1] = log((float)i/pdb);
	}
}

// reciprocal square root at full 32 bit precision
static inline float frsqrtes_nr(float x) {
	return 1.0f/sqrtf(x);
}

// glAA texture functions.
//
// The concept here is simple but the implementation is a bit tricky. What we want is an AA circle for a point texture
// (lines/tris get strips of the circle stretched over them.) To work with fractional positioning, there must be
// a 1 px empty border around the texture to filter with. Unfortunately, hardware border support isn't reliable.
// Mipmapping further complicates the issue. There are some GL extensions that could make this easier but
// I use a slightly wasteful wraparound layout here that works on ALL renderers:
//
// BBb		B = empty blackness
// BOb		O = the AA circle
// bbb		b = virtual (wraparound) empty blackness
//
// This layout will retain a 1 px border at every mip level on all hardware.
// For a 32 x 32 circle, this means a  64x 64 alpha mip pyramid, or ~  5k of VRAM.
// For a 256x256 circle, this means a 512x512 alpha mip pyramid, or ~341k of VRAM.
//
inline float ifun(float x, float y, float F) {   		// compute falloff at x,y with exponent F [-1..1]
	float S = (x*x + y);
	float L;
	if (S) {											// estimate sqrt, accurate to about 1/4096
		L = frsqrtes_nr(S);
		L = L * S;
	}
	else 
		L = 0.0f;
	if (F == 0.0f)										// intensity: [-1..0..1] = bloom..normal..fuzzy
		S = 0.0f;
	else if (F > 0.0f)
		// this is just pow(L/phf, F) but with a lookup table
		S = exp(logtbl[(int)(L*4.0f)]/F);
	else
		// this is a bit more complex-- logarithm ramp up to a solid dot with slight S blend at cusp
		S = L<(1.0f+F)*phf ? 0.0f : exp((1.0f+F*0.9f)/(L/phf-1.0f-F) * logtbl[(int)(L*4.0f)]);
	return 1.0f-S;
}


// 8-way symmetric macro
#define putpixel(x, y, I) {					   \
	unsigned char c = I*255;				   \
	texture[(pct   + x) + (pct   + y)*pdb] =   \
	texture[(pct-1 - x) + (pct   + y)*pdb] =   \
	texture[(pct   + x) + (pct-1 - y)*pdb] =   \
	texture[(pct-1 - x) + (pct-1 - y)*pdb] =   \
	texture[(pct   + y) + (pct   + x)*pdb] =   \
	texture[(pct-1 - y) + (pct   + x)*pdb] =   \
	texture[(pct   + y) + (pct-1 - x)*pdb] =   \
	texture[(pct-1 - y) + (pct-1 - x)*pdb] = c;\
}

// Generate the textures needed for anti-aliasing
void glAAGenerateAATex(float F, GLuint id, float alias) {			// compute the AA sphere
    if (id < 8) {
        if (!glAA_AAtex[id]) glAA_AAtex[id] = calloc(1, pdb*pdb);
        unsigned char *texture = glAA_AAtex[id];
        if (texture) {

            int x = phf-1;
            int y = 0;
            int ax;
            float T = 0.0f;
            float ys = 0.0f;
            while (x > y) {
                ys = (float)(y*y);
                float L = sqrt(prs - ys);
                float D = ceilf(L)-L;
                if (D < T) x--;
                for(ax=y; ax<x; ax++)
                    putpixel(ax, y, ifun(ax, ys, F));						// fill wedge
                putpixel(x, y, (1.0f-D)*(ifun(x, ys, F)));					// outer edge
                T = D;
                y++;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            if (!glAA_texture[id]) {
                glGenTextures(1, &glAA_texture[id]);
            }
            glBindTexture(GL_TEXTURE_2D, glAA_texture[id]);
            // Ideally we could use GL_CLAMP_TO_BORDER, but this is not available on GF2MX and GF4MX.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // Clamp the max mip level to 2x2 (1 px with 1 px border all around...)
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (int)log2(psz));

            // Use hardware filtering and client storage for faster texture generation.
            //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
            // Generate the full texture mipmap pyramid the first time.
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, pdb, pdb, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture);
            glGenerateMipmap(GL_TEXTURE_2D);
            /*
            }
            else {
                glBindTexture(GL_TEXTURE_2D, glAA_texture[id]);
                // Update only the sphere area after the first time.
                glPixelStorei(GL_UNPACK_ROW_LENGTH, pdb);
                glTexSubImage2D(GL_TEXTURE_2D, 0, psz, psz, psz, psz, GL_ALPHA, GL_UNSIGNED_BYTE, texture+pdb*psz+psz);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            }
            */

            // Set filtering depending on our aliasing mode.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (alias>0.66)?GL_NEAREST:GL_LINEAR);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (alias>0.33)?((alias>0.66)?GL_NEAREST:GL_LINEAR_MIPMAP_NEAREST):GL_LINEAR_MIPMAP_LINEAR);
            // now fix the upper mip levels so that 1 px points/lines stay bright:
            // at 1 px, the circle texture's default brightness is pi/4 = 0.78. This is geometrically
            // correct, but for usability reasons we want 1 px wide lines to display at full 1.0 brightness.
            // the following is an approximation sampling of the texture to do that, getting values that
            // "look ok" for exponential falloff in either direction.
            static unsigned char mipfix[4];
            unsigned char fix = (texture[(int)(psz+phf*0.2f)+pct*pdb]+texture[(int)(psz+phf*0.7f)+pct*pdb])>>1;
            mipfix[0]=mipfix[1]=mipfix[2]=mipfix[3]=fix;
            glTexSubImage2D(GL_TEXTURE_2D, lround(log(phf)*M_LOG2E), 2, 2, 2, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix);
            glTexSubImage2D(GL_TEXTURE_2D, lround(log(psz)*M_LOG2E), 1, 1, 1, 1, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix);
        }
    }
    glGetError();
}

