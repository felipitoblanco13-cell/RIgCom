#include "rig_face_sovereign.h"

/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_math.h"
#include "rig_noext_mem.h"
#include <float.h>
#include "rig_lib.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { RigSovVec3 o,d; } Ray;
typedef struct { RigSovVec3 lo,hi; } Box;
typedef struct { Box box; int left,right; rl_size start,count; } BVHNode;
typedef struct { const RigSovMesh *mesh; rl_u32 *indices; BVHNode *nodes; rl_size node_count,node_cap; } BVH;
typedef struct { float t,u,v; rl_u32 tri; RigSovVec3 p,n; } Hit;

static RigSovVec3 v3__rig_dup_b4555146_2(float x,float y,float z){RigSovVec3 r={x,y,z};return r;}
static RigSovVec3 add__rig_variant_91c53d6e(RigSovVec3 a,RigSovVec3 b){return v3__rig_dup_b4555146_2(a.x+b.x,a.y+b.y,a.z+b.z);}
static RigSovVec3 sub(RigSovVec3 a,RigSovVec3 b){return v3__rig_dup_b4555146_2(a.x-b.x,a.y-b.y,a.z-b.z);}
static RigSovVec3 mul(RigSovVec3 a,float s){return v3__rig_dup_b4555146_2(a.x*s,a.y*s,a.z*s);}
static RigSovVec3 had(RigSovVec3 a,RigSovVec3 b){return v3__rig_dup_b4555146_2(a.x*b.x,a.y*b.y,a.z*b.z);}
static float dot(RigSovVec3 a,RigSovVec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static RigSovVec3 cross(RigSovVec3 a,RigSovVec3 b){return v3__rig_dup_b4555146_2(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static float len(RigSovVec3 a){return sqrtf(dot(a,a));}
static RigSovVec3 norm(RigSovVec3 a){float l=len(a);return l>1e-12f?mul(a,1.0f/l):v3__rig_dup_b4555146_2(0,1,0);}
static RigSovVec3 reflect3(RigSovVec3 d,RigSovVec3 n){return sub(d,mul(n,2.0f*dot(d,n)));}
static float clampf__rig_dup_29cb97ea_2(float x,float a,float b){return x<a?a:(x>b?b:x);}
static Box box_empty(void){Box b={v3__rig_dup_b4555146_2(FLT_MAX,FLT_MAX,FLT_MAX),v3__rig_dup_b4555146_2(-FLT_MAX,-FLT_MAX,-FLT_MAX)};return b;}
static void box_add(Box *b,RigSovVec3 p){if(p.x<b->lo.x)b->lo.x=p.x;if(p.y<b->lo.y)b->lo.y=p.y;if(p.z<b->lo.z)b->lo.z=p.z;if(p.x>b->hi.x)b->hi.x=p.x;if(p.y>b->hi.y)b->hi.y=p.y;if(p.z>b->hi.z)b->hi.z=p.z;}
static Box tri_box(const RigSovMesh *m,rl_u32 ti){RigSovTri t=m->triangles[ti];Box b=box_empty();box_add(&b,m->vertices[t.a].position);box_add(&b,m->vertices[t.b].position);box_add(&b,m->vertices[t.c].position);return b;}
static RigSovVec3 tri_centroid(const RigSovMesh *m,rl_u32 ti){RigSovTri t=m->triangles[ti];return mul(add__rig_variant_91c53d6e(add__rig_variant_91c53d6e(m->vertices[t.a].position,m->vertices[t.b].position),m->vertices[t.c].position),1.0f/3.0f);}
static Box box_union(Box a,Box b){box_add(&a,b.lo);box_add(&a,b.hi);return a;}

static const RigSovMesh *SORT_MESH;static int SORT_AXIS;
static int cmp_tri(const void *aa,const void *bb){rl_u32 a=*(const rl_u32*)aa,b=*(const rl_u32*)bb;RigSovVec3 ca=tri_centroid(SORT_MESH,a),cb=tri_centroid(SORT_MESH,b);float x=SORT_AXIS==0?ca.x:(SORT_AXIS==1?ca.y:ca.z),y=SORT_AXIS==0?cb.x:(SORT_AXIS==1?cb.y:cb.z);return x<y?-1:(x>y?1:0);}
static int node_new(BVH *b){if(b->node_count==b->node_cap){rl_size nc=b->node_cap?b->node_cap*2:256;BVHNode *nn=(BVHNode*)rl_realloc(b->nodes,nc*sizeof(*nn));if(!nn)return -1;b->nodes=nn;b->node_cap=nc;}rl_memset(&b->nodes[b->node_count],0,sizeof(*b->nodes));b->nodes[b->node_count].left=b->nodes[b->node_count].right=-1;return (int)b->node_count++;}
static int bvh_build_node(BVH *b,rl_size start,rl_size count){rl_size i;int ni=node_new(b);Box bounds=box_empty(),cb=box_empty();RigSovVec3 ext;if(ni<0)return -1;for(i=start;i<start+count;i++){Box tb=tri_box(b->mesh,b->indices[i]);bounds=box_union(bounds,tb);box_add(&cb,tri_centroid(b->mesh,b->indices[i]));}b->nodes[ni].box=bounds;b->nodes[ni].start=start;b->nodes[ni].count=count;if(count<=8)return ni;ext=sub(cb.hi,cb.lo);SORT_AXIS=(ext.y>ext.x&&ext.y>=ext.z)?1:(ext.z>ext.x?2:0);SORT_MESH=b->mesh;qsort(b->indices+start,count,sizeof(rl_u32),cmp_tri);{rl_size mid=count/2;int l=bvh_build_node(b,start,mid),r=bvh_build_node(b,start+mid,count-mid);if(l<0||r<0)return -1;b->nodes[ni].left=l;b->nodes[ni].right=r;b->nodes[ni].count=0;}return ni;}
static int bvh_init(BVH *b,const RigSovMesh *m){rl_size i;rl_memset(b,0,sizeof(*b));b->mesh=m;b->indices=(rl_u32*)rl_malloc(m->triangle_count*sizeof(rl_u32));if(!b->indices)return RIG_SOV_ENOMEM;for(i=0;i<m->triangle_count;i++)b->indices[i]=(rl_u32)i;if(bvh_build_node(b,0,m->triangle_count)<0){rl_free(b->indices);rl_free(b->nodes);rl_memset(b,0,sizeof(*b));return RIG_SOV_ENOMEM;}return RIG_SOV_OK;}
static void bvh_free(BVH *b){if(b){rl_free(b->indices);rl_free(b->nodes);rl_memset(b,0,sizeof(*b));}}

static int box_hit(Box b,Ray r,float tmax){float tmin=0.0001f;int a;for(a=0;a<3;a++){float o=a==0?r.o.x:(a==1?r.o.y:r.o.z),d=a==0?r.d.x:(a==1?r.d.y:r.d.z),lo=a==0?b.lo.x:(a==1?b.lo.y:b.lo.z),hi=a==0?b.hi.x:(a==1?b.hi.y:b.hi.z);if(fabsf(d)<1e-12f){if(o<lo||o>hi)return 0;}else{float inv=1.0f/d,t0=(lo-o)*inv,t1=(hi-o)*inv;if(t0>t1){float q=t0;t0=t1;t1=q;}if(t0>tmin)tmin=t0;if(t1<tmax)tmax=t1;if(tmax<tmin)return 0;}}return 1;}
static int tri_hit(const RigSovMesh *m,rl_u32 ti,Ray r,float tmax,Hit *h){RigSovTri t=m->triangles[ti];RigSovVec3 a=m->vertices[t.a].position,b=m->vertices[t.b].position,c=m->vertices[t.c].position,e1=sub(b,a),e2=sub(c,a),p=cross(r.d,e2);float det=dot(e1,p),inv,u,v,tt;if(fabsf(det)<1e-9f)return 0;inv=1.0f/det;u=dot(sub(r.o,a),p)*inv;if(u<0||u>1)return 0;{RigSovVec3 q=cross(sub(r.o,a),e1);v=dot(r.d,q)*inv;if(v<0||u+v>1)return 0;tt=dot(e2,q)*inv;}if(tt<0.0001f||tt>=tmax)return 0;h->t=tt;h->u=u;h->v=v;h->tri=ti;h->p=add__rig_variant_91c53d6e(r.o,mul(r.d,tt));h->n=norm(add__rig_variant_91c53d6e(mul(m->vertices[t.a].normal,1-u-v),add__rig_variant_91c53d6e(mul(m->vertices[t.b].normal,u),mul(m->vertices[t.c].normal,v))));if(dot(h->n,r.d)>0)h->n=mul(h->n,-1);return 1;}
static int bvh_hit(const BVH *b,Ray r,float tmax,Hit *best){int stack[128],sp=0,found=0;stack[sp++]=0;while(sp){int ni=stack[--sp];const BVHNode *n=&b->nodes[ni];if(!box_hit(n->box,r,tmax))continue;if(n->count){rl_size i;for(i=n->start;i<n->start+n->count;i++){Hit h;if(tri_hit(b->mesh,b->indices[i],r,tmax,&h)){*best=h;tmax=h.t;found=1;}}}else{if(sp+2>=128)continue;stack[sp++]=n->left;stack[sp++]=n->right;}}return found;}

static rl_u64 rng64(rl_u64 *s){rl_u64 x=*s;x^=x>>12;x^=x<<25;x^=x>>27;*s=x;return x*2685821657736338717ULL;}
static float rnd(rl_u64 *s){return (float)((rng64(s)>>40)*(1.0/16777216.0));}
static RigSovVec3 random_unit(rl_u64 *s){float z=1-2*rnd(s),a=(float)(2*M_PI)*rnd(s),r=sqrtf(fmaxf(0,1-z*z));return v3__rig_dup_b4555146_2(r*cosf(a),r*sinf(a),z);}
static RigSovVec3 cosine_dir(RigSovVec3 n,rl_u64 *s){float r=sqrtf(rnd(s)),a=(float)(2*M_PI)*rnd(s),x=r*cosf(a),y=r*sinf(a),z=sqrtf(fmaxf(0,1-r*r));RigSovVec3 t=norm(fabsf(n.y)<0.9f?cross(v3__rig_dup_b4555146_2(0,1,0),n):cross(v3__rig_dup_b4555146_2(1,0,0),n)),b=cross(n,t);return norm(add__rig_variant_91c53d6e(add__rig_variant_91c53d6e(mul(t,x),mul(b,y)),mul(n,z)));}
static RigSovVec3 environment(RigSovVec3 d){float t=0.5f*(d.y+1);return add__rig_variant_91c53d6e(mul(v3__rig_dup_b4555146_2(0.09f,0.12f,0.18f),1-t),mul(v3__rig_dup_b4555146_2(0.55f,0.68f,0.9f),t));}
static int shadowed(const BVH *b,RigSovVec3 p,RigSovVec3 n,RigSovVec3 target){RigSovVec3 d=sub(target,p);float dist=len(d);Ray r={add__rig_variant_91c53d6e(p,mul(n,0.0004f)),mul(d,1.0f/dist)};Hit h;return bvh_hit(b,r,dist-0.001f,&h);}

static RigSovVec3 trace(const RigSovPTScene *s,const BVH *b,Ray ray,unsigned bounces,rl_u64 *rng){RigSovVec3 L=v3__rig_dup_b4555146_2(0,0,0),through=v3__rig_dup_b4555146_2(1,1,1);unsigned bounce;for(bounce=0;bounce<bounces;bounce++){Hit h;if(!bvh_hit(b,ray,FLT_MAX,&h)){L=add__rig_variant_91c53d6e(L,had(through,environment(ray.d)));break;}/* explicit spherical light */{RigSovVec3 lp=add__rig_variant_91c53d6e(s->light_position,mul(random_unit(rng),s->light_radius));RigSovVec3 ld=sub(lp,h.p);float d2=dot(ld,ld),dist=sqrtf(d2);ld=mul(ld,1.0f/dist);float ndl=fmaxf(0,dot(h.n,ld));if(ndl>0&&!shadowed(b,h.p,h.n,lp)){RigSovVec3 c=mul(s->light_color,s->light_power*ndl/(4.0f*(float)M_PI*d2+1e-6f));L=add__rig_variant_91c53d6e(L,had(through,had(s->material.base_color,c)));}}
        {float metallic=clampf__rig_dup_29cb97ea_2(s->material.metallic,0,1),rough=clampf__rig_dup_29cb97ea_2(s->material.roughness,0.02f,1);float specProb=0.06f+0.70f*metallic;if(rnd(rng)<specProb){RigSovVec3 refl=reflect3(ray.d,h.n),jitter=mul(random_unit(rng),rough*rough);ray.d=norm(add__rig_variant_91c53d6e(refl,jitter));through=had(through,add__rig_variant_91c53d6e(mul(v3__rig_dup_b4555146_2(0.04f,0.04f,0.04f),1-metallic),mul(s->material.base_color,metallic)));through=mul(through,1.0f/specProb);}else{ray.d=cosine_dir(h.n,rng);through=had(through,s->material.base_color);through=mul(through,1.0f/(1.0f-specProb));}ray.o=add__rig_variant_91c53d6e(h.p,mul(h.n,0.0004f));}
        if(bounce>=3){float q=clampf__rig_dup_29cb97ea_2(fmaxf(through.x,fmaxf(through.y,through.z)),0.08f,0.95f);if(rnd(rng)>q)break;through=mul(through,1.0f/q);}if(s->material.emission>0)L=add__rig_variant_91c53d6e(L,mul(had(through,s->material.base_color),s->material.emission));}
    return L;}

void rig_sov_pt_scene_init(RigSovPTScene *s){if(!s)return;rl_memset(s,0,sizeof(*s));rig_sov_mesh_init(&s->mesh);s->material.base_color=v3__rig_dup_b4555146_2(0.55f,0.32f,0.24f);s->material.roughness=0.45f;s->light_position=v3__rig_dup_b4555146_2(-1,2.5f,2);s->light_color=v3__rig_dup_b4555146_2(1,0.92f,0.82f);s->light_radius=0.25f;s->light_power=80;s->camera_position=v3__rig_dup_b4555146_2(0,1.35f,3.2f);s->camera_target=v3__rig_dup_b4555146_2(0,1.0f,0);s->vertical_fov_deg=38;}
void rig_sov_pt_scene_free(RigSovPTScene *s){if(s){rig_sov_mesh_free(&s->mesh);rl_memset(s,0,sizeof(*s));}}

int rig_sov_pathtrace(const RigSovPTScene *s,const RigSovPTRenderOptions *o,RigSovImage *out){BVH b;RigSovVec3 forward,right,up;float aspect,scale;unsigned x,y,k;rl_u64 seed;int rc;if(!s||!o||!out||o->width<1||o->height<1||o->samples_per_pixel<1||o->max_bounces<1)return RIG_SOV_EINVAL;if(o->width>16384||o->height>16384||o->samples_per_pixel>100000)return RIG_SOV_ERANGE;if((rc=rig_sov_mesh_validate(&s->mesh,NULL,0)))return rc;rl_memset(out,0,sizeof(*out));if((rc=bvh_init(&b,&s->mesh)))return rc;out->width=o->width;out->height=o->height;out->channels=3;out->pixels=(rl_u8*)rl_malloc((rl_size)o->width*o->height*3);if(!out->pixels){bvh_free(&b);return RIG_SOV_ENOMEM;}forward=norm(sub(s->camera_target,s->camera_position));right=norm(cross(forward,v3__rig_dup_b4555146_2(0,1,0)));if(len(right)<0.01f)right=v3__rig_dup_b4555146_2(1,0,0);up=norm(cross(right,forward));aspect=(float)o->width/o->height;scale=tanf(s->vertical_fov_deg*(float)M_PI/360.0f);seed=o->seed?o->seed:0x13198a2e03707344ULL;for(y=0;y<o->height;y++)for(x=0;x<o->width;x++){RigSovVec3 c=v3__rig_dup_b4555146_2(0,0,0);rl_u64 prng=seed^((rl_u64)y<<32)^x*0x9e3779b97f4a7c15ULL;for(k=0;k<o->samples_per_pixel;k++){float sx=(2.0f*((x+rnd(&prng))/o->width)-1.0f)*aspect*scale,sy=(1.0f-2.0f*((y+rnd(&prng))/o->height))*scale;Ray r={s->camera_position,norm(add__rig_variant_91c53d6e(forward,add__rig_variant_91c53d6e(mul(right,sx),mul(up,sy))))};c=add__rig_variant_91c53d6e(c,trace(s,&b,r,o->max_bounces,&prng));}c=mul(c,1.0f/o->samples_per_pixel);/* ACES-like and gamma */{float vals[3]={c.x,c.y,c.z};unsigned z;for(z=0;z<3;z++){float q=vals[z];q=(q*(2.51f*q+0.03f))/(q*(2.43f*q+0.59f)+0.14f);q=powf(clampf__rig_dup_29cb97ea_2(q,0,1),1.0f/2.2f);out->pixels[((rl_size)y*o->width+x)*3+z]=(rl_u8)(q*255+0.5f);}}}bvh_free(&b);return RIG_SOV_OK;}
