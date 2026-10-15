#pragma once
#include "ios_touch_force.h"

typedef struct { int attempted[TOUCH_FORCE_COUNT], handles[TOUCH_FORCE_COUNT]; } touchForceMedia_t;
typedef int (*touchForceReadable_t)(const char *path,void *context);
typedef int (*touchForceRegister_t)(const char *stem,void *context);

static inline void TouchForceMedia_Reset(touchForceMedia_t *media) { memset(media,0,sizeof(*media)); }
static inline int TouchForceMedia_Resolve(touchForceMedia_t *media,int id,int live,
	touchForceReadable_t readable,touchForceRegister_t registerShader,void *context) {
	const char *stem=TouchForce_Icon(id);
	static const char *const extensions[]={".tga",".png",".jpg"};
	char path[64]; int i;
	if(!stem || !live) return 0;
	if(media->attempted[id-1]) return media->handles[id-1];
	media->attempted[id-1]=1;
	for(i=0;i<3;++i) {
		const size_t length=strlen(stem);
		memcpy(path,stem,length); strcpy(path+length,extensions[i]);
		if(readable(path,context)) { media->handles[id-1]=registerShader(stem,context); break; }
	}
	return media->handles[id-1];
}

typedef struct { float x,y,w,h; } touchForceQuad_t;
static inline touchForceQuad_t TouchForceMedia_Quad(float x,float y,float w,float h) {
	const float scale=.70710678f;
	touchForceQuad_t quad={x+w*(1-scale)/2,y+h*(1-scale)/2,w*scale,h*scale};
	return quad;
}
