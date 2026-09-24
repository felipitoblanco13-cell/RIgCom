#include "rig_face_sovereign.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t *p; size_t n,cap; } Blob;
typedef struct { char *p; size_t n,cap; } Text;

static int blob_reserve(Blob *b,size_t add){size_t need=b->n+add;if(need<b->n)return RIG_SOV_ERANGE;if(need>b->cap){size_t nc=b->cap?b->cap:4096;while(nc<need){if(nc>SIZE_MAX/2)return RIG_SOV_ERANGE;nc*=2;}uint8_t *np=(uint8_t*)realloc(b->p,nc);if(!np)return RIG_SOV_ENOMEM;b->p=np;b->cap=nc;}return RIG_SOV_OK;}
static int blob_add(Blob *b,const void *p,size_t n){int rc=blob_reserve(b,n);if(rc)return rc;memcpy(b->p+b->n,p,n);b->n+=n;return RIG_SOV_OK;}
static int blob_zero_align(Blob *b,size_t a){uint8_t z=0;while(b->n%a){int rc=blob_add(b,&z,1);if(rc)return rc;}return RIG_SOV_OK;}
static int text_reserve(Text *t,size_t add){size_t need=t->n+add+1;if(need<t->n)return RIG_SOV_ERANGE;if(need>t->cap){size_t nc=t->cap?t->cap:4096;while(nc<need){if(nc>SIZE_MAX/2)return RIG_SOV_ERANGE;nc*=2;}char *np=(char*)realloc(t->p,nc);if(!np)return RIG_SOV_ENOMEM;t->p=np;t->cap=nc;}return RIG_SOV_OK;}
static int textf(Text *t,const char *fmt,...){va_list ap,cp;int n,rc;va_start(ap,fmt);va_copy(cp,ap);n=vsnprintf(NULL,0,fmt,cp);va_end(cp);if(n<0){va_end(ap);return RIG_SOV_EIO;}rc=text_reserve(t,(size_t)n);if(rc){va_end(ap);return rc;}vsnprintf(t->p+t->n,t->cap-t->n,fmt,ap);va_end(ap);t->n+=(size_t)n;return RIG_SOV_OK;}
static int json_string(Text *t,const char *s){const unsigned char *p=(const unsigned char*)(s?s:"");int rc=textf(t,"\"");if(rc)return rc;for(;*p;p++){if(*p=='\"'||*p=='\\')rc=textf(t,"\\%c",*p);else if(*p=='\n')rc=textf(t,"\\n");else if(*p=='\r')rc=textf(t,"\\r");else if(*p=='\t')rc=textf(t,"\\t");else if(*p<32)rc=textf(t,"\\u%04x",*p);else rc=textf(t,"%c",*p);if(rc)return rc;}return textf(t,"\"");}
static void wr32(FILE *f,uint32_t v){uint8_t b[4]={(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)};fwrite(b,1,4,f);}

int rig_sov_export_glb(const RigSovMesh *m,const RigSovSkeleton *s,const RigSovMaterial *mat,const char *path){
    Blob bin={0};Text js={0};size_t off[7]={0},len[7]={0},i,j;float minp[3],maxp[3];int has_skin=s&&s->joint_count>0;int rc;FILE *f=NULL;uint32_t json_len,bin_len,total;RigSovMaterial def={{0.55f,0.32f,0.24f},0.45f,0,0};
    if (!m || !path) return RIG_SOV_EINVAL;
    if ((rc = rig_sov_mesh_validate(m, NULL, 0))) return rc;
    if (has_skin && s->joint_count > RIG_SOV_MAX_JOINTS) return RIG_SOV_ERANGE;
    if (!mat) mat = &def;
    minp[0]=maxp[0]=m->vertices[0].position.x;minp[1]=maxp[1]=m->vertices[0].position.y;minp[2]=maxp[2]=m->vertices[0].position.z;
    for(i=0;i<m->vertex_count;i++){const RigSovVec3 p=m->vertices[i].position;if(p.x<minp[0])minp[0]=p.x;if(p.y<minp[1])minp[1]=p.y;if(p.z<minp[2])minp[2]=p.z;if(p.x>maxp[0])maxp[0]=p.x;if(p.y>maxp[1])maxp[1]=p.y;if(p.z>maxp[2])maxp[2]=p.z;}
    /* Interleaving is intentionally avoided: accessors remain directly consumable by all glTF 2.0 readers. */
#define ADD_SECTION(ID,TYPE,EXPR) do{off[ID]=bin.n;for(i=0;i<m->vertex_count;i++){TYPE v=(EXPR);if((rc=blob_add(&bin,&v,sizeof(v))))goto done;}len[ID]=bin.n-off[ID];if((rc=blob_zero_align(&bin,4)))goto done;}while(0)
    {typedef struct{float x,y,z;} F3;ADD_SECTION(0,F3,((F3){m->vertices[i].position.x,m->vertices[i].position.y,m->vertices[i].position.z}));}
    {typedef struct{float x,y,z;} F3;ADD_SECTION(1,F3,((F3){m->vertices[i].normal.x,m->vertices[i].normal.y,m->vertices[i].normal.z}));}
    {typedef struct{float x,y;} F2;ADD_SECTION(2,F2,((F2){m->vertices[i].uv.x,m->vertices[i].uv.y}));}
    if(has_skin){typedef struct{uint16_t x,y,z,w;} U4;ADD_SECTION(3,U4,((U4){m->vertices[i].joints[0],m->vertices[i].joints[1],m->vertices[i].joints[2],m->vertices[i].joints[3]}));}
    if(has_skin){typedef struct{float x,y,z,w;} F4;ADD_SECTION(4,F4,((F4){m->vertices[i].weights[0],m->vertices[i].weights[1],m->vertices[i].weights[2],m->vertices[i].weights[3]}));}
#undef ADD_SECTION
    off[5]=bin.n;for(i=0;i<m->triangle_count;i++){uint32_t idx[3]={m->triangles[i].a,m->triangles[i].b,m->triangles[i].c};if((rc=blob_add(&bin,idx,sizeof(idx))))goto done;}len[5]=bin.n-off[5];if((rc=blob_zero_align(&bin,4)))goto done;
    if(has_skin){off[6]=bin.n;for(i=0;i<s->joint_count;i++){if((rc=blob_add(&bin,s->joints[i].inverse_bind.m,sizeof(s->joints[i].inverse_bind.m))))goto done;}len[6]=bin.n-off[6];if((rc=blob_zero_align(&bin,4)))goto done;}
    /* JSON */
    if((rc=textf(&js,"{\"asset\":{\"version\":\"2.0\",\"generator\":\"Rig Face Sovereign 1.0\"},\"buffers\":[{\"byteLength\":%zu}],\"bufferViews\":[",bin.n)))goto done;
    {
        int first_view = 1;
        for (i = 0; i < (has_skin ? 7u : 6u); i++) {
            if (!has_skin && (i == 3u || i == 4u)) continue;
            if (!first_view && (rc = textf(&js, ","))) goto done;
            first_view = 0;
            if ((rc = textf(&js, "{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu", off[i], len[i]))) goto done;
            if (i <= 4u) rc = textf(&js, ",\"target\":34962}");
            else if (i == 5u) rc = textf(&js, ",\"target\":34963}");
            else rc = textf(&js, "}");
            if (rc) goto done;
        }
    }
    if((rc=textf(&js,"],\"accessors\":[")))goto done;
    if((rc=textf(&js,"{\"bufferView\":0,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC3\",\"min\":[%.9g,%.9g,%.9g],\"max\":[%.9g,%.9g,%.9g]},",m->vertex_count,minp[0],minp[1],minp[2],maxp[0],maxp[1],maxp[2])))goto done;
    if((rc=textf(&js,"{\"bufferView\":1,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC3\"},",m->vertex_count)))goto done;
    if((rc=textf(&js,"{\"bufferView\":2,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC2\"},",m->vertex_count)))goto done;
    if(has_skin){if((rc=textf(&js,"{\"bufferView\":3,\"componentType\":5123,\"count\":%zu,\"type\":\"VEC4\"},",m->vertex_count)))goto done;if((rc=textf(&js,"{\"bufferView\":4,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC4\"},",m->vertex_count)))goto done;}
    if((rc=textf(&js,"{\"bufferView\":%d,\"componentType\":5125,\"count\":%zu,\"type\":\"SCALAR\"}",has_skin?5:3,m->triangle_count*3)))goto done;
    if(has_skin){if((rc=textf(&js,",{\"bufferView\":6,\"componentType\":5126,\"count\":%zu,\"type\":\"MAT4\"}",s->joint_count)))goto done;}
    if((rc=textf(&js,"],\"materials\":[{\"name\":\"SovereignMaterial\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[%.7g,%.7g,%.7g,1],\"metallicFactor\":%.7g,\"roughnessFactor\":%.7g}",mat->base_color.x,mat->base_color.y,mat->base_color.z,mat->metallic,mat->roughness)))goto done;
    if(mat->emission>0){if((rc=textf(&js,",\"emissiveFactor\":[%.7g,%.7g,%.7g]",mat->base_color.x*mat->emission,mat->base_color.y*mat->emission,mat->base_color.z*mat->emission)))goto done;}
    if((rc=textf(&js,"}],\"meshes\":[{\"name\":\"SovereignHuman\",\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2")))goto done;
    if(has_skin){if((rc=textf(&js,",\"JOINTS_0\":3,\"WEIGHTS_0\":4")))goto done;}
    if((rc=textf(&js,"},\"indices\":%d,\"material\":0,\"mode\":4}]}],\"nodes\":[",has_skin?5:3)))goto done;
    if(has_skin){for(i=0;i<s->joint_count;i++){RigSovVec3 lp=s->joints[i].world_position;if(s->joints[i].parent>=0){RigSovVec3 pp=s->joints[s->joints[i].parent].world_position;lp.x-=pp.x;lp.y-=pp.y;lp.z-=pp.z;}if(i)if((rc=textf(&js,",")))goto done;if((rc=textf(&js,"{\"name\":")))goto done;if((rc=json_string(&js,s->joints[i].name)))goto done;if((rc=textf(&js,",\"translation\":[%.9g,%.9g,%.9g]",lp.x,lp.y,lp.z)))goto done;{int first=1;for(j=0;j<s->joint_count;j++)if(s->joints[j].parent==(int)i){if(first){if((rc=textf(&js,",\"children\":[")))goto done;first=0;}else if((rc=textf(&js,",")))goto done;if((rc=textf(&js,"%zu",j)))goto done;}if(!first)if((rc=textf(&js,"]")))goto done;}if((rc=textf(&js,"}")))goto done;}
        if((rc=textf(&js,",{\"name\":\"SovereignMesh\",\"mesh\":0,\"skin\":0}")))goto done;
    }else{if((rc=textf(&js,"{\"name\":\"SovereignMesh\",\"mesh\":0}")))goto done;}
    if((rc=textf(&js,"]")))goto done;
    if(has_skin){if((rc=textf(&js,",\"skins\":[{\"name\":\"SovereignSkin\",\"inverseBindMatrices\":6,\"skeleton\":0,\"joints\":[")))goto done;for(i=0;i<s->joint_count;i++){if(i)if((rc=textf(&js,",")))goto done;if((rc=textf(&js,"%zu",i)))goto done;}if((rc=textf(&js,"]}]")))goto done;}
    if ((rc = textf(&js, ",\"scenes\":[{\"nodes\":["))) goto done;
    if (has_skin) {
        if ((rc = textf(&js, "0,%zu", s->joint_count))) goto done;
    } else if ((rc = textf(&js, "0"))) goto done;
    if ((rc = textf(&js, "]}],\"scene\":0}"))) goto done;
    while(js.n%4){if((rc=textf(&js," ")))goto done;}while(bin.n%4){if((rc=blob_zero_align(&bin,4)))goto done;}
    if(js.n>UINT32_MAX||bin.n>UINT32_MAX||12u+8u+js.n+8u+bin.n>UINT32_MAX){rc=RIG_SOV_ERANGE;goto done;}json_len=(uint32_t)js.n;bin_len=(uint32_t)bin.n;total=12+8+json_len+8+bin_len;
    f=fopen(path,"wb");if(!f){rc=RIG_SOV_EIO;goto done;}wr32(f,0x46546c67);wr32(f,2);wr32(f,total);wr32(f,json_len);wr32(f,0x4e4f534a);if(fwrite(js.p,1,json_len,f)!=json_len){rc=RIG_SOV_EIO;goto done;}wr32(f,bin_len);wr32(f,0x004e4942);if(fwrite(bin.p,1,bin_len,f)!=bin_len){rc=RIG_SOV_EIO;goto done;}rc=RIG_SOV_OK;
done:if(f&&fclose(f)!=0&&rc==RIG_SOV_OK)rc=RIG_SOV_EIO;free(bin.p);free(js.p);return rc;
}
