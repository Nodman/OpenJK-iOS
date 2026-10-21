#!/usr/bin/env python3
"""Exercise the actual SP/MP iOS menu patch, visibility and focus functions."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
out = root / 'build-desktop' / 't11-16-host'
out.mkdir(parents=True, exist_ok=True)


def function(source, signature):
    start = source.index(signature)
    while source[start + len(signature):].lstrip().startswith(';'):
        start = source.index(signature, start + len(signature))
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


stubs = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
typedef int qboolean;
enum { qfalse, qtrue, ITEM_TYPE_MULTI, ITEM_TYPE_TEXT };
enum { CVAR_SHOW=1, CVAR_HIDE=2, CVAR_ENABLE=4, CVAR_DISABLE=8, CVAR_SUBSTRING=16 };
enum { WINDOW_HASFOCUS=1, WINDOW_VISIBLE=2, WINDOW_DECORATION=4, WINDOW_INACTIVE=8 };
#define CHAN_LOCAL_SOUND 1
#define Q_stricmp strcasecmp
#define Q_strncpyz(dst,src,n) snprintf(dst,n,"%s",src)
#define COM_BeginParseSession() ((void)0)
#define COM_EndParseSession() ((void)0)
typedef int sfxHandle_t;
typedef struct { float x,y,w,h; } rectDef_t;
typedef struct { const char *name; rectDef_t rect; int flags; } window_t;
typedef struct { int strDef,count; float cvarValue[32]; const char *cvarList[32]; } multiDef_t;
typedef struct { window_t window; int type; const char *cvar,*text,*descText,*cvarTest,*enableCvar,*onFocus; int cvarFlags,disabled,focusSound; void *parent; TYPE_DATA float textalignx,textscale; int font,iMenuFont; rectDef_t textRect; } itemDef_t;
typedef struct { window_t window; int itemCount,cursorItem; itemDef_t *items[1]; } menuDef_t;
static menuDef_t Menus[4];
static int menuCount;
static int glowSupported, maxRefresh=120;
static const char *activeProvider="", *requestedProvider="";
static float getValue(const char *name) {
 if(!strcmp(name,"r_iosNativeWidth")) return 1920;
 if(!strcmp(name,"r_iosNativeHeight")) return 1080;
 if(!strcmp(name,"ui_iosProMotion")) return 1;
 if(!strcmp(name,"ui_r_mode")) return 0;
 if(!strcmp(name,"r_iosBaseAutoScale") || !strcmp(name,"r_renderScale")) return 1;
 assert(0);return 0;
}
static void getString(const char *name,char *value,int size) {
 if(!strcmp(name,"r_iosDynamicGlow")) snprintf(value,size,"%d",glowSupported);
 else if(!strcmp(name,"r_iosMaxRefresh")) snprintf(value,size,"%d",maxRefresh);
 else if(!strcmp(name,"r_glesProviderActive")) snprintf(value,size,"%s",activeProvider);
 else if(!strcmp(name,"r_glesProvider")) snprintf(value,size,"%s",requestedProvider);
 else assert(0);
}
static int textWidth(const char *text,float scale,int font) { (void)scale;(void)font;return (int)strlen(text); }
static void sound(int s,int c) { (void)s;(void)c; }
typedef struct { float (*getCVarValue)(const char *); int (*textWidth)(const char *,float,int); void (*getCVarString)(const char *,char *,int); struct { int itemFocusSound; } Assets; void (*startLocalSound)(int,int); } display_t;
static display_t display={getValue,textWidth,getString,{0},sound}, *DC=&display;
static int String_Parse(PARSE_CHAR **p,const char **v) { if(!**p) return 0;*v=*p;*p+=strlen(*p);return 1; }
static int Rect_ContainsPoint(rectDef_t *r,float x,float y) { (void)r;(void)x;(void)y;return 1; }
static itemDef_t *Menu_ClearFocus(menuDef_t *menu) { menu->items[0]->window.flags &= ~WINDOW_HASFOCUS; return NULL; }
static void Item_RunScript(itemDef_t *item,const char *script) { (void)item;(void)script; }
'''

checks = r'''
static void checkProMotion(itemDef_t *item,multiDef_t *multi,const char *help) {
 assert(!strcmp(item->descText,help));
 assert(!strcmp(item->text,"120 Hz (ProMotion)"));
 assert(!strcmp(item->cvar,"ui_iosProMotion"));
 assert(!strcmp(item->cvarTest,"r_iosMaxRefresh") && !strcmp(item->enableCvar,"120") && item->cvarFlags==CVAR_SHOW);
 assert(!multi->strDef && multi->count==2 && multi->cvarValue[0]==0 && multi->cvarValue[1]==1);
 assert(!strcmp(multi->cvarList[0],"Off") && !strcmp(multi->cvarList[1],"On"));
 maxRefresh=60;assert(!Item_EnableShowViaCvar(item,CVAR_SHOW));
 maxRefresh=120;assert(Item_EnableShowViaCvar(item,CVAR_SHOW));
}
static void checkProviders(void) {
 const char *names[]={"setupMenu","ingameSetupMenu","setup_menu","ingame_setup"};
 const char *active[]={"angle","eagl","ANGLE","","unexpected"};
 const char *requested[]={"eagl","angle","eagl","angle","angle"};
 const char *angle="ANGLE: request 120 Hz; actual rate varies by device.";
 const char *eagl="EAGL: iPad up to 120 Hz; iPhone may stay at 60.";
 const char *unknown="120 Hz depends on the active graphics provider.";
 const char *expected[5];
 itemDef_t items[4];multiDef_t multis[4];const char *saved[4];int i,j;
 expected[0]=angle;expected[1]=eagl;expected[2]=angle;expected[3]=unknown;expected[4]=unknown;
 memset(Menus,0,sizeof Menus);memset(items,0,sizeof items);memset(multis,0,sizeof multis);menuCount=4;
 for(i=0;i<4;i++) {
  menuDef_t *menu=&Menus[i];itemDef_t *item=&items[i];
  menu->window.name=names[i];menu->itemCount=1;menu->items[0]=item;
  item->parent=menu;item->type=ITEM_TYPE_MULTI;item->window.name="fullscreen";item->cvar="ui_r_fullscreen";
  SET_MULTI_POINTER
 }
 for(j=0;j<5;j++) {
  activeProvider=active[j];requestedProvider=requested[j];
  for(i=0;i<4;i++) {
   UI_IOS_PatchVideoMenu(&Menus[i]);checkProMotion(&items[i],&multis[i],expected[j]);saved[i]=items[i].descText;
  }
  /* Stack churn and a second load must leave a persistent help string. */
  { volatile char scratch[1024];for(i=0;i<1024;i++) scratch[i]=(char)i;(void)scratch; }
  for(i=0;i<4;i++) { assert(!strcmp(saved[i],expected[j]));UI_IOS_PatchVideoMenu(&Menus[i]);checkProMotion(&items[i],&multis[i],expected[j]); }
 }
 activeProvider="";UI_IOS_RefreshVideoMenus(qtrue);
 for(i=0;i<4;i++) saved[i]=items[i].descText;
 for(i=0;i<4;i++) checkProMotion(&items[i],&multis[i],unknown);
 activeProvider="angle";UI_IOS_RefreshVideoMenus(qfalse);
 for(i=0;i<4;i++) checkProMotion(&items[i],&multis[i],angle);
 activeProvider="eagl";requestedProvider="angle";UI_IOS_RefreshVideoMenus(qfalse);
 for(i=0;i<4;i++) {
  assert(items[i].descText==saved[i]);checkProMotion(&items[i],&multis[i],eagl);
 }
 activeProvider="unknown";UI_IOS_RefreshVideoMenus(qfalse);
 for(i=0;i<4;i++) checkProMotion(&items[i],&multis[i],unknown);
 UI_IOS_RefreshVideoMenus(qfalse);
 for(i=0;i<4;i++) checkProMotion(&items[i],&multis[i],unknown);
 items[0].descText="sentinel";UI_IOS_RefreshVideoMenus(qtrue);checkProMotion(&items[0],&multis[0],unknown);
 /* Invalid and unrelated controls keep their original text and binding. */
 for(j=0;j<7;j++) {
  itemDef_t item=items[0];menuDef_t menu=Menus[0];
  item.descText="sentinel";item.cvar="ui_r_fullscreen";item.parent=&menu;menu.items[0]=&item;
  switch(j) {
   case 0:menu.window.name="custom";break;
   case 1:item.type=ITEM_TYPE_TEXT;break;
   case 2:item.window.name=NULL;break;
   case 3:item.cvar=NULL;break;
   case 4:item.parent=NULL;break;
   case 5:CLEAR_MULTI_POINTER break;
   case 6:menu.items[0]=NULL;break;
  }
  UI_IOS_PatchVideoMenu(&menu);assert(!strcmp(item.descText,"sentinel"));
  assert(item.cvar==NULL || !strcmp(item.cvar,"ui_r_fullscreen"));
 }
 { itemDef_t item=items[0];item.descText="sentinel";item.cvar="unrelated";Menus[0].items[0]=&item;
   UI_IOS_PatchVideoMenu(&Menus[0]);assert(!strcmp(item.descText,"sentinel"));assert(!strcmp(item.cvar,"unrelated")); }
 menuCount=0;UI_IOS_PatchVideoMenu(NULL);
 puts("PASS: actual SP/MP ProMotion patch+refresh, providers/mismatches/unknown/transitions/force, persistent help, bindings/values/visibility, invalid controls; no cvar-write API exposed");
}
int main(void) {
 checkProviders();
 const char *names[]={"setupMenu","ingameSetupMenu","setup_menu","ingame_setup"};
 int i;
 for(i=0;i<4;i++) {
  menuDef_t menu;itemDef_t item;multiDef_t multi;
  memset(&menu,0,sizeof menu);memset(&item,0,sizeof item);memset(&multi,0,sizeof multi);
  menu.window.name=names[i];menu.itemCount=1;menu.items[0]=&item;
  item.parent=&menu;item.window.name="dynamic_glow";item.cvar=i%2?"R_DynamicGlow":"r_dynamicglow";
  item.type=ITEM_TYPE_MULTI;item.window.flags=WINDOW_VISIBLE;
  ASSIGNMENT
  multi.count=2;multi.cvarValue[1]=1;
  UI_IOS_PatchVideoMenu(&menu);
  assert(!strcmp(item.cvarTest,"r_iosDynamicGlow") && !strcmp(item.enableCvar,"1") && item.cvarFlags==CVAR_SHOW);
  assert(multi.count==2 && multi.cvarValue[1]==1);
  assert(!strcmp(item.cvar,i%2?"R_DynamicGlow":"r_dynamicglow"));
  glowSupported=0;
  assert(!Item_EnableShowViaCvar(&item,CVAR_SHOW));assert(!Item_SetFocus(&item,0,0));
  assert(!(item.window.flags & WINDOW_HASFOCUS));
  glowSupported=1;
  assert(Item_EnableShowViaCvar(&item,CVAR_SHOW));assert(Item_SetFocus(&item,0,0));
  item.window.flags=WINDOW_VISIBLE;
  item.cvarTest=NULL;item.enableCvar=NULL;item.cvarFlags=0;
  item.cvar="other";UI_IOS_PatchVideoMenu(&menu);assert(!item.cvarTest);
  item.cvar="r_dynamicglow";item.window.name="other";UI_IOS_PatchVideoMenu(&menu);assert(!item.cvarTest);
  item.window.name="dynamic_glow";menu.window.name="custom";UI_IOS_PatchVideoMenu(&menu);assert(!item.cvarTest);
  menu.window.name=names[i];item.type=ITEM_TYPE_TEXT;UI_IOS_PatchVideoMenu(&menu);assert(!item.cvarTest);
  item.type=ITEM_TYPE_MULTI;CLEAR_DATA
  UI_IOS_PatchVideoMenu(&menu);assert(!item.cvarTest);
 }
 UI_IOS_PatchVideoMenu(NULL);
 puts("PASS: four stock menus, mixed-case glow cvar, hidden/focus blocked at 0, shown/focusable at 1, unrelated items unchanged");
}
'''

failures = []
for tree, extension, compiler in [('code', 'cpp', 'c++'), ('codemp', 'c', 'cc')]:
    source = (root / tree / ('ui/ui_shared.' + extension)).read_text()
    start = source.index('static char iosVideoLabels')
    end = source.index('void UI_IOS_RefreshVideoMenus', start)
    block = source[start:end] + function(source, 'void UI_IOS_RefreshVideoMenus( qboolean force )')
    block += function(source, 'qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag)')
    block += function(source, 'qboolean Item_SetFocus(itemDef_t *item, float x, float y)')
    data_type = 'void *typeData;' if tree == 'code' else 'union { multiDef_t *multi; } typeData;'
    data = 'item.typeData' if tree == 'code' else 'item.typeData.multi'
    scale_header = '#include "' + str(root / 'shared/qcommon/ios_render_scale.h') + '"\n'
    harness = (scale_header + stubs + block + checks).replace('TYPE_DATA', data_type)
    harness = harness.replace('PARSE_CHAR', 'const char' if tree == 'code' else 'char')
    harness = harness.replace('ASSIGNMENT', data + '=&multi;').replace('CLEAR_DATA', data + '=NULL;')
    harness = harness.replace('SET_MULTI_POINTER', data.replace('item.', 'item->') + '=&multis[i];')
    harness = harness.replace('CLEAR_MULTI_POINTER', data + '=NULL;')
    path = out / (tree + '-glow.' + extension)
    path.write_text(harness)
    binary = out / (tree + '-glow')
    subprocess.run([compiler, '-std=c++11' if tree == 'code' else '-std=c99',
                    '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True, cwd=root)
    print(tree + ':', flush=True)
    result = subprocess.run([str(binary)], cwd=root)
    if result.returncode:
        failures.append(tree)
if failures:
    raise SystemExit('FAIL: ' + ', '.join(failures))
