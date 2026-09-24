/* ==========================================================================
 * 23_rig_face_sovereign_core.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : nested/Face_Pose_Body/rig_face_sovereign_core.c
 * Copias fundidas: 5
 * Funciones      : 35      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
#include "rig_face_sovereign.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static RigSovVec3 v3(float x, float y, float z) { RigSovVec3 r = {x,y,z}; return r; }
static RigSovVec3 v3_add(RigSovVec3 a, RigSovVec3 b) { return v3(a.x+b.x,a.y+b.y,a.z+b.z); }
static RigSovVec3 v3_sub(RigSovVec3 a, RigSovVec3 b) { return v3(a.x-b.x,a.y-b.y,a.z-b.z); }
static RigSovVec3 v3_scale(RigSovVec3 a, float s) { return v3(a.x*s,a.y*s,a.z*s); }
static float v3_dot(RigSovVec3 a, RigSovVec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static RigSovVec3 v3_cross(RigSovVec3 a, RigSovVec3 b) {
    return v3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
}
static float v3_len(RigSovVec3 a) { return sqrtf(v3_dot(a,a)); }
static RigSovVec3 v3_norm(RigSovVec3 a) { float l=v3_len(a); return l>1e-20f?v3_scale(a,1.0f/l):v3(0,1,0); }

static int checked_mul_size(size_t a, size_t b, size_t *out) {
    if (!out) return RIG_SOV_EINVAL;
    if (a && b > SIZE_MAX / a) return RIG_SOV_ERANGE;
    *out = a*b;
    return RIG_SOV_OK;
}

const char *rig_sov_error_string(int code) {
    switch (code) {
        case RIG_SOV_OK: return "ok";
        case RIG_SOV_EINVAL: return "invalid argument";
        case RIG_SOV_ENOMEM: return "out of memory";
        case RIG_SOV_EIO: return "I/O error";
        case RIG_SOV_EFORMAT: return "unsupported or corrupt format";
        case RIG_SOV_ERANGE: return "numeric or capacity range exceeded";
        case RIG_SOV_ESTATE: return "invalid state";
        case RIG_SOV_EUNSUPPORTED: return "unsupported on this platform";
        default: return "unknown error";
    }
}

void rig_sov_mesh_init(RigSovMesh *mesh) {
    if (mesh) memset(mesh,0,sizeof(*mesh));
}

void rig_sov_mesh_free(RigSovMesh *mesh) {
    size_t i;
    if (!mesh) return;
    free(mesh->vertices);
    free(mesh->triangles);
    for (i=0;i<mesh->morph_count;i++) {
        free(mesh->morphs[i].position_delta);
        free(mesh->morphs[i].normal_delta);
    }
    free(mesh->morphs);
    memset(mesh,0,sizeof(*mesh));
}

int rig_sov_mesh_reserve(RigSovMesh *mesh, size_t vertices, size_t triangles) {
    RigSovVertex *nv;
    RigSovTri *nt;
    size_t bytes;
    if (!mesh) return RIG_SOV_EINVAL;
    if (vertices > mesh->vertex_capacity) {
        size_t cap = mesh->vertex_capacity ? mesh->vertex_capacity : 256;
        while (cap < vertices) {
            if (cap > SIZE_MAX/2) return RIG_SOV_ERANGE;
            cap *= 2;
        }
        if (checked_mul_size(cap,sizeof(*nv),&bytes)!=RIG_SOV_OK) return RIG_SOV_ERANGE;
        nv=(RigSovVertex*)realloc(mesh->vertices,bytes);
        if(!nv) return RIG_SOV_ENOMEM;
        mesh->vertices=nv; mesh->vertex_capacity=cap;
    }
    if (triangles > mesh->triangle_capacity) {
        size_t cap = mesh->triangle_capacity ? mesh->triangle_capacity : 256;
        while (cap < triangles) {
            if (cap > SIZE_MAX/2) return RIG_SOV_ERANGE;
            cap *= 2;
        }
        if (checked_mul_size(cap,sizeof(*nt),&bytes)!=RIG_SOV_OK) return RIG_SOV_ERANGE;
        nt=(RigSovTri*)realloc(mesh->triangles,bytes);
        if(!nt) return RIG_SOV_ENOMEM;
        mesh->triangles=nt; mesh->triangle_capacity=cap;
    }
    return RIG_SOV_OK;
}

int rig_sov_mesh_append_vertex(RigSovMesh *mesh, const RigSovVertex *vertex, uint32_t *index_out) {
    int rc;
    if(!mesh||!vertex) return RIG_SOV_EINVAL;
    if(mesh->vertex_count>=UINT32_MAX) return RIG_SOV_ERANGE;
    rc=rig_sov_mesh_reserve(mesh,mesh->vertex_count+1,mesh->triangle_count);
    if(rc) return rc;
    mesh->vertices[mesh->vertex_count]=*vertex;
    if(index_out) *index_out=(uint32_t)mesh->vertex_count;
    mesh->vertex_count++;
    return RIG_SOV_OK;
}

int rig_sov_mesh_append_triangle(RigSovMesh *mesh, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    if(!mesh) return RIG_SOV_EINVAL;
    if(a>=mesh->vertex_count||b>=mesh->vertex_count||c>=mesh->vertex_count) return RIG_SOV_ERANGE;
    rc=rig_sov_mesh_reserve(mesh,mesh->vertex_count,mesh->triangle_count+1);
    if(rc) return rc;
    mesh->triangles[mesh->triangle_count++]=(RigSovTri){a,b,c};
    return RIG_SOV_OK;
}

int rig_sov_mesh_compute_normals(RigSovMesh *mesh) {
    size_t i;
    if(!mesh||!mesh->vertices||!mesh->triangles) return RIG_SOV_EINVAL;
    for(i=0;i<mesh->vertex_count;i++) mesh->vertices[i].normal=v3(0,0,0);
    for(i=0;i<mesh->triangle_count;i++) {
        RigSovTri t=mesh->triangles[i];
        RigSovVec3 a=mesh->vertices[t.a].position,b=mesh->vertices[t.b].position,c=mesh->vertices[t.c].position;
        RigSovVec3 n=v3_cross(v3_sub(b,a),v3_sub(c,a));
        mesh->vertices[t.a].normal=v3_add(mesh->vertices[t.a].normal,n);
        mesh->vertices[t.b].normal=v3_add(mesh->vertices[t.b].normal,n);
        mesh->vertices[t.c].normal=v3_add(mesh->vertices[t.c].normal,n);
    }
    for(i=0;i<mesh->vertex_count;i++) mesh->vertices[i].normal=v3_norm(mesh->vertices[i].normal);
    return RIG_SOV_OK;
}

int rig_sov_mesh_validate(const RigSovMesh *mesh, char *error, size_t error_size) {
    size_t i,j;
    if(error&&error_size) error[0]='\0';
    if(!mesh||!mesh->vertices||!mesh->triangles||mesh->vertex_count==0||mesh->triangle_count==0) {
        if(error&&error_size) snprintf(error,error_size,"mesh is empty or uninitialized");
        return RIG_SOV_EINVAL;
    }
    for(i=0;i<mesh->vertex_count;i++) {
        const RigSovVertex *v=&mesh->vertices[i];
        float sum=0.0f;
        if(!isfinite(v->position.x)||!isfinite(v->position.y)||!isfinite(v->position.z)||
           !isfinite(v->normal.x)||!isfinite(v->normal.y)||!isfinite(v->normal.z)||
           !isfinite(v->uv.x)||!isfinite(v->uv.y)) {
            if(error&&error_size) snprintf(error,error_size,"non-finite vertex at %zu",i);
            return RIG_SOV_EFORMAT;
        }
        for(j=0;j<4;j++) {
            if(v->weights[j]<-1e-5f||!isfinite(v->weights[j])) {
                if(error&&error_size) snprintf(error,error_size,"invalid skin weight at vertex %zu",i);
                return RIG_SOV_EFORMAT;
            }
            sum+=v->weights[j];
        }
        if(sum>0.0f&&fabsf(sum-1.0f)>2e-3f) {
            if(error&&error_size) snprintf(error,error_size,"skin weights do not sum to one at vertex %zu",i);
            return RIG_SOV_EFORMAT;
        }
    }
    for(i=0;i<mesh->triangle_count;i++) {
        RigSovTri t=mesh->triangles[i];
        if(t.a>=mesh->vertex_count||t.b>=mesh->vertex_count||t.c>=mesh->vertex_count||t.a==t.b||t.b==t.c||t.c==t.a) {
            if(error&&error_size) snprintf(error,error_size,"invalid triangle at %zu",i);
            return RIG_SOV_EFORMAT;
        }
    }
    return RIG_SOV_OK;
}

typedef struct { uint32_t a,b,mid; } EdgeMid;
static int edge_mid_get(RigSovMesh *out, EdgeMid **edges, size_t *count, size_t *cap, uint32_t a, uint32_t b, uint32_t *mid) {
    size_t i;
    RigSovVertex v;
    int rc;
    if(a>b){uint32_t t=a;a=b;b=t;}
    for(i=0;i<*count;i++) if((*edges)[i].a==a&&(*edges)[i].b==b){*mid=(*edges)[i].mid;return RIG_SOV_OK;}
    memset(&v,0,sizeof(v));
    v.position=v3_scale(v3_add(out->vertices[a].position,out->vertices[b].position),0.5f);
    v.normal=v3_norm(v3_add(out->vertices[a].normal,out->vertices[b].normal));
    v.uv.x=(out->vertices[a].uv.x+out->vertices[b].uv.x)*0.5f;
    v.uv.y=(out->vertices[a].uv.y+out->vertices[b].uv.y)*0.5f;
    for(i=0;i<4;i++){
        v.joints[i]=out->vertices[a].joints[i];
        v.weights[i]=(out->vertices[a].weights[i]+out->vertices[b].weights[i])*0.5f;
    }
    rc=rig_sov_mesh_append_vertex(out,&v,mid); if(rc) return rc;
    if(*count==*cap){size_t nc=*cap?*cap*2:1024;EdgeMid *ne=(EdgeMid*)realloc(*edges,nc*sizeof(**edges));if(!ne)return RIG_SOV_ENOMEM;*edges=ne;*cap=nc;}
    (*edges)[(*count)++]=(EdgeMid){a,b,*mid};
    return RIG_SOV_OK;
}

int rig_sov_mesh_subdivide(RigSovMesh *mesh, unsigned iterations) {
    unsigned it;
    if(!mesh||iterations>8) return RIG_SOV_EINVAL;
    for(it=0;it<iterations;it++) {
        RigSovMesh out; EdgeMid *edges=NULL; size_t edge_count=0,edge_cap=0,i; int rc=RIG_SOV_OK;
        rig_sov_mesh_init(&out);
        rc=rig_sov_mesh_reserve(&out,mesh->vertex_count+mesh->triangle_count*2,mesh->triangle_count*4); if(rc) goto fail;
        memcpy(out.vertices,mesh->vertices,mesh->vertex_count*sizeof(*mesh->vertices)); out.vertex_count=mesh->vertex_count;
        for(i=0;i<mesh->triangle_count;i++) {
            RigSovTri t=mesh->triangles[i]; uint32_t ab,bc,ca;
            if((rc=edge_mid_get(&out,&edges,&edge_count,&edge_cap,t.a,t.b,&ab)))goto fail;
            if((rc=edge_mid_get(&out,&edges,&edge_count,&edge_cap,t.b,t.c,&bc)))goto fail;
            if((rc=edge_mid_get(&out,&edges,&edge_count,&edge_cap,t.c,t.a,&ca)))goto fail;
            if((rc=rig_sov_mesh_append_triangle(&out,t.a,ab,ca)))goto fail;
            if((rc=rig_sov_mesh_append_triangle(&out,ab,t.b,bc)))goto fail;
            if((rc=rig_sov_mesh_append_triangle(&out,ca,bc,t.c)))goto fail;
            if((rc=rig_sov_mesh_append_triangle(&out,ab,bc,ca)))goto fail;
        }
        free(edges); rig_sov_mesh_free(mesh); *mesh=out; rig_sov_mesh_compute_normals(mesh); continue;
fail:
        free(edges); rig_sov_mesh_free(&out); return rc;
    }
    return RIG_SOV_OK;
}

int rig_sov_mesh_write_obj(const RigSovMesh *mesh, const char *path) {
    FILE *f; size_t i; int rc;
    if(!mesh||!path) return RIG_SOV_EINVAL;
    rc=rig_sov_mesh_validate(mesh,NULL,0); if(rc) return rc;
    f=fopen(path,"wb"); if(!f) return RIG_SOV_EIO;
    fprintf(f,"# Rig Face Sovereign OBJ\n");
    for(i=0;i<mesh->vertex_count;i++) fprintf(f,"v %.9g %.9g %.9g\n",mesh->vertices[i].position.x,mesh->vertices[i].position.y,mesh->vertices[i].position.z);
    for(i=0;i<mesh->vertex_count;i++) fprintf(f,"vt %.9g %.9g\n",mesh->vertices[i].uv.x,1.0f-mesh->vertices[i].uv.y);
    for(i=0;i<mesh->vertex_count;i++) fprintf(f,"vn %.9g %.9g %.9g\n",mesh->vertices[i].normal.x,mesh->vertices[i].normal.y,mesh->vertices[i].normal.z);
    for(i=0;i<mesh->triangle_count;i++){RigSovTri t=mesh->triangles[i];fprintf(f,"f %u/%u/%u %u/%u/%u %u/%u/%u\n",t.a+1,t.a+1,t.a+1,t.b+1,t.b+1,t.b+1,t.c+1,t.c+1,t.c+1);}
    if(fclose(f)!=0) return RIG_SOV_EIO;
    return RIG_SOV_OK;
}

static int ppm_token(FILE *f, char *buf, size_t n) {
    int c; size_t p=0;
    if(!f||!buf||n<2) return 0;
    do { c=fgetc(f); if(c=='#'){while(c!='\n'&&c!=EOF)c=fgetc(f);} } while(c!=EOF&&isspace((unsigned char)c));
    if(c==EOF) return 0;
    do { if(p+1<n)buf[p++]=(char)c; c=fgetc(f); } while(c!=EOF&&!isspace((unsigned char)c)&&c!='#');
    if(c=='#') while(c!='\n'&&c!=EOF)c=fgetc(f);
    buf[p]='\0'; return p>0;
}

int rig_sov_image_load(const char *path, RigSovImage *image) {
    FILE *f; char tok[64]; int ascii,channels,maxv; unsigned w,h; size_t count,i;
    if (!path || !image) return RIG_SOV_EINVAL;
    memset(image, 0, sizeof(*image));
    f=fopen(path,"rb"); if(!f) return RIG_SOV_EIO;
    if(!ppm_token(f,tok,sizeof(tok))){fclose(f);return RIG_SOV_EFORMAT;}
    if(!strcmp(tok,"P6")){ascii=0;channels=3;} else if(!strcmp(tok,"P5")){ascii=0;channels=1;} else if(!strcmp(tok,"P3")){ascii=1;channels=3;} else if(!strcmp(tok,"P2")){ascii=1;channels=1;} else {fclose(f);return RIG_SOV_EFORMAT;}
    if(!ppm_token(f,tok,sizeof(tok))||(w=(unsigned)strtoul(tok,NULL,10))==0||!ppm_token(f,tok,sizeof(tok))||(h=(unsigned)strtoul(tok,NULL,10))==0||!ppm_token(f,tok,sizeof(tok))||(maxv=atoi(tok))<=0||maxv>65535){fclose(f);return RIG_SOV_EFORMAT;}
    if(checked_mul_size((size_t)w*h,(size_t)channels,&count)){fclose(f);return RIG_SOV_ERANGE;}
    image->pixels=(uint8_t*)malloc(count); if(!image->pixels){fclose(f);return RIG_SOV_ENOMEM;}
    image->width=w;image->height=h;image->channels=(unsigned)channels;
    if(ascii){for(i=0;i<count;i++){long v;if(!ppm_token(f,tok,sizeof(tok))){rig_sov_image_free(image);fclose(f);return RIG_SOV_EFORMAT;}v=strtol(tok,NULL,10);if(v<0)v=0;if(v>maxv)v=maxv;image->pixels[i]=(uint8_t)((v*255L+maxv/2)/maxv);}}
    else if(maxv<=255){if(fread(image->pixels,1,count,f)!=count){rig_sov_image_free(image);fclose(f);return RIG_SOV_EIO;} if(maxv!=255)for(i=0;i<count;i++)image->pixels[i]=(uint8_t)((image->pixels[i]*255+maxv/2)/maxv);}
    else {for(i=0;i<count;i++){int hi=fgetc(f),lo=fgetc(f);int v;if(hi==EOF||lo==EOF){rig_sov_image_free(image);fclose(f);return RIG_SOV_EIO;}v=(hi<<8)|lo;image->pixels[i]=(uint8_t)((v*255L+maxv/2)/maxv);}}
    fclose(f); return RIG_SOV_OK;
}

int rig_sov_image_save_ppm(const char *path, const RigSovImage *image) {
    FILE *f; size_t pixels,i;
    if(!path||!image||!image->pixels||!image->width||!image->height||(image->channels!=1&&image->channels!=3)) return RIG_SOV_EINVAL;
    f=fopen(path,"wb"); if(!f) return RIG_SOV_EIO;
    pixels=(size_t)image->width*image->height;
    if(image->channels==3){fprintf(f,"P6\n%u %u\n255\n",image->width,image->height);if(fwrite(image->pixels,3,pixels,f)!=pixels){fclose(f);return RIG_SOV_EIO;}}
    else {fprintf(f,"P5\n%u %u\n255\n",image->width,image->height);if(fwrite(image->pixels,1,pixels,f)!=pixels){fclose(f);return RIG_SOV_EIO;}}
    i=(size_t)fclose(f); (void)i; return RIG_SOV_OK;
}

void rig_sov_image_free(RigSovImage *image) { if(image){free(image->pixels);memset(image,0,sizeof(*image));} }

static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}

int rig_sov_audio_load_wav(const char *path, RigSovAudio *audio) {
    FILE *f; uint8_t h[12],ch[8]; uint16_t fmt=0,channels=0,bits=0; uint32_t rate=0; uint8_t *data=NULL; uint32_t data_size=0; int got_fmt=0;
    if (!path || !audio) return RIG_SOV_EINVAL;
    memset(audio, 0, sizeof(*audio));
    f=fopen(path,"rb");if(!f)return RIG_SOV_EIO;
    if(fread(h,1,12,f)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4)){fclose(f);return RIG_SOV_EFORMAT;}
    while(fread(ch,1,8,f)==8){uint32_t sz=rd32(ch+4);long next=ftell(f)+(long)sz+(sz&1u);if(!memcmp(ch,"fmt ",4)){uint8_t b[40];if(sz<16||sz>sizeof(b)||fread(b,1,sz,f)!=sz){fclose(f);return RIG_SOV_EFORMAT;}fmt=rd16(b);channels=rd16(b+2);rate=rd32(b+4);bits=rd16(b+14);got_fmt=1;}else if(!memcmp(ch,"data",4)){data=(uint8_t*)malloc(sz?sz:1);if(!data){fclose(f);return RIG_SOV_ENOMEM;}if(fread(data,1,sz,f)!=sz){free(data);fclose(f);return RIG_SOV_EIO;}data_size=sz;}if(fseek(f,next,SEEK_SET)!=0)break;if(got_fmt&&data)break;}
    fclose(f);
    if(!got_fmt||!data||!channels||!rate||(fmt!=1&&fmt!=3)){free(data);return RIG_SOV_EFORMAT;}
    if(!((fmt==1&&(bits==8||bits==16||bits==24||bits==32))||(fmt==3&&bits==32))){free(data);return RIG_SOV_EFORMAT;}
    {size_t bytes_per=(size_t)bits/8,frames=data_size/(bytes_per*channels),i,c;float *samples=(float*)malloc(frames*sizeof(float));if(!samples){free(data);return RIG_SOV_ENOMEM;}for(i=0;i<frames;i++){double sum=0;for(c=0;c<channels;c++){const uint8_t *p=data+(i*channels+c)*bytes_per;float s=0;if(fmt==3){memcpy(&s,p,4);}else if(bits==8)s=((int)p[0]-128)/128.0f;else if(bits==16)s=(int16_t)rd16(p)/32768.0f;else if(bits==24){int32_t v=(int32_t)(p[0]|(p[1]<<8)|(p[2]<<16));if(v&0x800000)v|=~0xffffff;s=v/8388608.0f;}else{s=(int32_t)rd32(p)/2147483648.0f;}if(!isfinite(s))s=0;sum+=s;}samples[i]=(float)(sum/channels);}free(data);audio->samples=samples;audio->sample_count=frames;audio->sample_rate=rate;audio->channels=1;}
    return RIG_SOV_OK;
}

void rig_sov_audio_free(RigSovAudio *audio){if(audio){free(audio->samples);memset(audio,0,sizeof(*audio));}}

static uint64_t rng_step(uint64_t *s){uint64_t x=*s;x^=x>>12;x^=x<<25;x^=x>>27;*s=x;return x*2685821657736338717ULL;}
static float rng_signed(uint64_t *s){return (float)((rng_step(s)>>40)*(1.0/16777216.0)*2.0-1.0);}
static float sigmoidf_safe(float x){if(x>20)return 1;if(x<-20)return 0;return 1.0f/(1.0f+expf(-x));}

int rig_sov_mlp_init(RigSovMLP *net,unsigned inputs,unsigned hidden,unsigned outputs,uint64_t seed){size_t n1,n2,i;if(!net||!inputs||!hidden||!outputs)return RIG_SOV_EINVAL;memset(net,0,sizeof(*net));if(checked_mul_size(inputs,hidden,&n1)||checked_mul_size(hidden,outputs,&n2))return RIG_SOV_ERANGE;net->w1=(float*)malloc(n1*sizeof(float));net->b1=(float*)calloc(hidden,sizeof(float));net->w2=(float*)malloc(n2*sizeof(float));net->b2=(float*)calloc(outputs,sizeof(float));if(!net->w1||!net->b1||!net->w2||!net->b2){rig_sov_mlp_free(net);return RIG_SOV_ENOMEM;}net->input_count=inputs;net->hidden_count=hidden;net->output_count=outputs;if(!seed)seed=0x6a09e667f3bcc909ULL;for(i=0;i<n1;i++)net->w1[i]=rng_signed(&seed)*sqrtf(6.0f/(inputs+hidden));for(i=0;i<n2;i++)net->w2[i]=rng_signed(&seed)*sqrtf(6.0f/(hidden+outputs));return RIG_SOV_OK;}
void rig_sov_mlp_free(RigSovMLP *net){if(net){free(net->w1);free(net->b1);free(net->w2);free(net->b2);memset(net,0,sizeof(*net));}}

int rig_sov_mlp_infer(const RigSovMLP *net,const float *input,float *output){unsigned h,o,i;float *hidden;if(!net||!input||!output||!net->w1||!net->w2)return RIG_SOV_EINVAL;hidden=(float*)malloc(net->hidden_count*sizeof(float));if(!hidden)return RIG_SOV_ENOMEM;for(h=0;h<net->hidden_count;h++){float z=net->b1[h];for(i=0;i<net->input_count;i++)z+=input[i]*net->w1[h*net->input_count+i];hidden[h]=tanhf(z);}for(o=0;o<net->output_count;o++){float z=net->b2[o];for(h=0;h<net->hidden_count;h++)z+=hidden[h]*net->w2[o*net->hidden_count+h];output[o]=sigmoidf_safe(z);}free(hidden);return RIG_SOV_OK;}

int rig_sov_mlp_train(RigSovMLP *net,const float *x,const float *y,size_t sample_count,unsigned epochs,float lr,float l2){float *h,*out,*dh,*dout;unsigned e;size_t s;unsigned i,j,k;if(!net||!x||!y||!sample_count||epochs==0||lr<=0)return RIG_SOV_EINVAL;h=(float*)malloc(net->hidden_count*sizeof(float));out=(float*)malloc(net->output_count*sizeof(float));dh=(float*)malloc(net->hidden_count*sizeof(float));dout=(float*)malloc(net->output_count*sizeof(float));if(!h||!out||!dh||!dout){free(h);free(out);free(dh);free(dout);return RIG_SOV_ENOMEM;}for(e=0;e<epochs;e++){for(s=0;s<sample_count;s++){const float *in=x+s*net->input_count,*target=y+s*net->output_count;for(j=0;j<net->hidden_count;j++){float z=net->b1[j];for(i=0;i<net->input_count;i++)z+=in[i]*net->w1[j*net->input_count+i];h[j]=tanhf(z);}for(k=0;k<net->output_count;k++){float z=net->b2[k];for(j=0;j<net->hidden_count;j++)z+=h[j]*net->w2[k*net->hidden_count+j];out[k]=sigmoidf_safe(z);dout[k]=(out[k]-target[k])*out[k]*(1.0f-out[k]);}for(j=0;j<net->hidden_count;j++){float sum=0;for(k=0;k<net->output_count;k++)sum+=dout[k]*net->w2[k*net->hidden_count+j];dh[j]=sum*(1.0f-h[j]*h[j]);}for(k=0;k<net->output_count;k++){for(j=0;j<net->hidden_count;j++){size_t q=k*net->hidden_count+j;net->w2[q]-=lr*(dout[k]*h[j]+l2*net->w2[q]);}net->b2[k]-=lr*dout[k];}for(j=0;j<net->hidden_count;j++){for(i=0;i<net->input_count;i++){size_t q=j*net->input_count+i;net->w1[q]-=lr*(dh[j]*in[i]+l2*net->w1[q]);}net->b1[j]-=lr*dh[j];}}lr*=0.9995f;}free(h);free(out);free(dh);free(dout);return RIG_SOV_OK;}

