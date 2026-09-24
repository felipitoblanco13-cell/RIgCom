#include "rig_face_sovereign.h"

#include "rig_math.h"
#include "rig_noext_io.h"
#include "rig_lib.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static RigSovVec3 v3(float x,float y,float z){RigSovVec3 r={x,y,z};return r;}
static RigSovVec3 add3(RigSovVec3 a,RigSovVec3 b){return v3(a.x+b.x,a.y+b.y,a.z+b.z);}
static RigSovVec3 sub3(RigSovVec3 a,RigSovVec3 b){return v3(a.x-b.x,a.y-b.y,a.z-b.z);}
static RigSovVec3 mul3(RigSovVec3 a,float s){return v3(a.x*s,a.y*s,a.z*s);}
static float dot3(RigSovVec3 a,RigSovVec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static RigSovVec3 cross3(RigSovVec3 a,RigSovVec3 b){return v3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static float len3(RigSovVec3 a){return sqrtf(dot3(a,a));}
static RigSovVec3 norm3(RigSovVec3 a){float l=len3(a);return l>1e-9f?mul3(a,1.0f/l):v3(0,1,0);}
static float clampf__rig_dup_29cb97ea(float x,float a,float b){return x<a?a:(x>b?b:x);}

static RigSovMat4 inverse_translation(RigSovVec3 p){RigSovMat4 m={{1,0,0,0,0,1,0,0,0,0,1,0,-p.x,-p.y,-p.z,1}};return m;}

RigSovBodyParams rig_sov_body_default_params(void){
    RigSovBodyParams p;
    p.height_m=1.75f;p.shoulder_width=0.43f;p.hip_width=0.34f;p.torso_length=0.56f;
    p.arm_length=0.62f;p.leg_length=0.88f;p.muscularity=0.45f;p.body_fat=0.22f;
    p.sex_factor=0.5f;p.age_factor=0.30f;p.radial_segments=20;p.longitudinal_segments=8;
    return p;
}

static void set_joint(RigSovSkeleton *s,rl_size i,const char *name,int parent,RigSovVec3 p){
    if(!s||i>=RIG_SOV_MAX_JOINTS)return;
    snprintf(s->joints[i].name,sizeof(s->joints[i].name),"%s",name);
    s->joints[i].parent=parent;s->joints[i].world_position=p;s->joints[i].inverse_bind=inverse_translation(p);
    if(s->joint_count<=i)s->joint_count=i+1;
}

enum {
    J_ROOT=0,J_PELVIS,J_SPINE1,J_SPINE2,J_CHEST,J_NECK,J_HEAD,
    J_CLAV_L,J_UPARM_L,J_FOREARM_L,J_HAND_L,
    J_CLAV_R,J_UPARM_R,J_FOREARM_R,J_HAND_R,
    J_THIGH_L,J_SHIN_L,J_FOOT_L,J_TOE_L,
    J_THIGH_R,J_SHIN_R,J_FOOT_R,J_TOE_R,
    J_JAW,J_EYE_L,J_EYE_R,J_COUNT
};

static void build_skeleton(const RigSovBodyParams *p,RigSovSkeleton *s){
    float h=p->height_m;
    float foot=0.035f*h,ankle=0.055f*h,knee=0.285f*h,hip=0.515f*h;
    float waist=hip+0.11f*h,chest=hip+0.24f*h,neck=hip+0.36f*h,head=hip+0.45f*h;
    float sx=p->shoulder_width*0.5f,hx=p->hip_width*0.5f;
    rl_memset(s,0,sizeof(*s));
    set_joint(s,J_ROOT,"root",-1,v3(0,0,0));
    set_joint(s,J_PELVIS,"pelvis",J_ROOT,v3(0,hip,0));
    set_joint(s,J_SPINE1,"spine_01",J_PELVIS,v3(0,waist,0));
    set_joint(s,J_SPINE2,"spine_02",J_SPINE1,v3(0,(waist+chest)*0.5f,0));
    set_joint(s,J_CHEST,"chest",J_SPINE2,v3(0,chest,0));
    set_joint(s,J_NECK,"neck",J_CHEST,v3(0,neck,0));
    set_joint(s,J_HEAD,"head",J_NECK,v3(0,head,0));
    set_joint(s,J_CLAV_L,"clavicle_L",J_CHEST,v3(-sx*0.35f,chest+0.015f*h,0));
    set_joint(s,J_UPARM_L,"upper_arm_L",J_CLAV_L,v3(-sx,chest,0));
    set_joint(s,J_FOREARM_L,"forearm_L",J_UPARM_L,v3(-sx-p->arm_length*0.47f,chest-0.02f*h,0));
    set_joint(s,J_HAND_L,"hand_L",J_FOREARM_L,v3(-sx-p->arm_length*0.90f,chest-0.03f*h,0));
    set_joint(s,J_CLAV_R,"clavicle_R",J_CHEST,v3(sx*0.35f,chest+0.015f*h,0));
    set_joint(s,J_UPARM_R,"upper_arm_R",J_CLAV_R,v3(sx,chest,0));
    set_joint(s,J_FOREARM_R,"forearm_R",J_UPARM_R,v3(sx+p->arm_length*0.47f,chest-0.02f*h,0));
    set_joint(s,J_HAND_R,"hand_R",J_FOREARM_R,v3(sx+p->arm_length*0.90f,chest-0.03f*h,0));
    set_joint(s,J_THIGH_L,"thigh_L",J_PELVIS,v3(-hx*0.58f,hip,0));
    set_joint(s,J_SHIN_L,"shin_L",J_THIGH_L,v3(-hx*0.58f,knee,0.01f*h));
    set_joint(s,J_FOOT_L,"foot_L",J_SHIN_L,v3(-hx*0.58f,ankle,0.015f*h));
    set_joint(s,J_TOE_L,"toe_L",J_FOOT_L,v3(-hx*0.58f,foot,0.11f*h));
    set_joint(s,J_THIGH_R,"thigh_R",J_PELVIS,v3(hx*0.58f,hip,0));
    set_joint(s,J_SHIN_R,"shin_R",J_THIGH_R,v3(hx*0.58f,knee,0.01f*h));
    set_joint(s,J_FOOT_R,"foot_R",J_SHIN_R,v3(hx*0.58f,ankle,0.015f*h));
    set_joint(s,J_TOE_R,"toe_R",J_FOOT_R,v3(hx*0.58f,foot,0.11f*h));
    set_joint(s,J_JAW,"jaw",J_HEAD,v3(0,head-0.055f*h,0.035f*h));
    set_joint(s,J_EYE_L,"eye_L",J_HEAD,v3(-0.018f*h,head+0.015f*h,0.055f*h));
    set_joint(s,J_EYE_R,"eye_R",J_HEAD,v3(0.018f*h,head+0.015f*h,0.055f*h));
}

static void skin_single(RigSovVertex *v,unsigned j){rl_memset(v->joints,0,sizeof(v->joints));rl_memset(v->weights,0,sizeof(v->weights));v->joints[0]=(rl_u16)j;v->weights[0]=1.0f;}
static void skin_pair(RigSovVertex *v,unsigned a,unsigned b,float t){t=clampf__rig_dup_29cb97ea(t,0,1);rl_memset(v->joints,0,sizeof(v->joints));rl_memset(v->weights,0,sizeof(v->weights));v->joints[0]=(rl_u16)a;v->joints[1]=(rl_u16)b;v->weights[0]=1-t;v->weights[1]=t;}

static int append_ellipsoid(RigSovMesh *m,RigSovVec3 c,RigSovVec3 r,unsigned rings,unsigned seg,unsigned joint){
    rl_size base=m->vertex_count;unsigned y,x;int rc;
    if(rings<3||seg<6)return RIG_SOV_EINVAL;
    rc=rig_sov_mesh_reserve(m,base+(rl_size)(rings+1)*(seg+1),m->triangle_count+(rl_size)rings*seg*2);if(rc)return rc;
    for(y=0;y<=rings;y++){
        float v=(float)y/rings,phi=(float)M_PI*v;
        for(x=0;x<=seg;x++){
            float u=(float)x/seg,th=(float)(2.0*M_PI)*u;
            RigSovVertex q;RigSovVec3 n=v3(sinf(phi)*cosf(th),cosf(phi),sinf(phi)*sinf(th));rl_memset(&q,0,sizeof(q));
            q.position=add3(c,v3(n.x*r.x,n.y*r.y,n.z*r.z));
            q.normal=norm3(v3(n.x/(r.x?r.x:1),n.y/(r.y?r.y:1),n.z/(r.z?r.z:1)));
            q.uv=(RigSovVec2){u,v};skin_single(&q,joint);m->vertices[m->vertex_count++]=q;
        }
    }
    for(y=0;y<rings;y++)for(x=0;x<seg;x++){
        rl_u32 a=(rl_u32)(base+y*(seg+1)+x),b=a+1,c0=(rl_u32)(base+(y+1)*(seg+1)+x),d=c0+1;
        m->triangles[m->triangle_count++]=(RigSovTri){a,c0,b};m->triangles[m->triangle_count++]=(RigSovTri){b,c0,d};
    }
    return RIG_SOV_OK;
}

static int append_capsule(RigSovMesh *m,RigSovVec3 a,RigSovVec3 b,float ra,float rb,unsigned rings,unsigned seg,unsigned ja,unsigned jb){
    RigSovVec3 axis=sub3(b,a),w=norm3(axis),ref=fabsf(w.y)<0.9f?v3(0,1,0):v3(1,0,0),u=norm3(cross3(ref,w)),v=cross3(w,u);
    rl_size base=m->vertex_count;unsigned y,x;int rc;
    if(rings<2||seg<6||len3(axis)<1e-6f)return RIG_SOV_EINVAL;
    rc=rig_sov_mesh_reserve(m,base+(rl_size)(rings+1)*(seg+1),m->triangle_count+(rl_size)rings*seg*2);if(rc)return rc;
    for(y=0;y<=rings;y++){
        float t=(float)y/rings,rad=ra+(rb-ra)*t;RigSovVec3 center=add3(a,mul3(axis,t));
        for(x=0;x<=seg;x++){
            float q=(float)(2.0*M_PI)*x/seg,cs=cosf(q),sn=sinf(q);RigSovVec3 radial=add3(mul3(u,cs),mul3(v,sn));RigSovVertex z;rl_memset(&z,0,sizeof(z));
            z.position=add3(center,mul3(radial,rad));z.normal=radial;z.uv=(RigSovVec2){(float)x/seg,t};skin_pair(&z,ja,jb,t);m->vertices[m->vertex_count++]=z;
        }
    }
    for(y=0;y<rings;y++)for(x=0;x<seg;x++){rl_u32 p=(rl_u32)(base+y*(seg+1)+x),q=p+1,r0=(rl_u32)(base+(y+1)*(seg+1)+x),s=r0+1;m->triangles[m->triangle_count++]=(RigSovTri){p,r0,q};m->triangles[m->triangle_count++]=(RigSovTri){q,r0,s};}
    return RIG_SOV_OK;
}

static int append_torso(RigSovMesh *m,const RigSovSkeleton *s,const RigSovBodyParams *p){
    unsigned rings=p->longitudinal_segments<6?6:p->longitudinal_segments,seg=p->radial_segments<12?12:p->radial_segments,y,x;rl_size base=m->vertex_count;int rc;
    float y0=s->joints[J_PELVIS].world_position.y-0.03f*p->height_m,y1=s->joints[J_NECK].world_position.y;
    rc=rig_sov_mesh_reserve(m,base+(rl_size)(rings+1)*(seg+1),m->triangle_count+(rl_size)rings*seg*2);if(rc)return rc;
    for(y=0;y<=rings;y++){
        float t=(float)y/rings,yy=y0+(y1-y0)*t;
        float hip=p->hip_width*0.52f,waist=p->hip_width*(0.40f+0.08f*p->body_fat),chest=p->shoulder_width*(0.47f+0.04f*p->muscularity);
        float w;
        if(t<0.35f){float q=t/0.35f;w=hip+(waist-hip)*q;}else{float q=(t-0.35f)/0.65f;w=waist+(chest-waist)*sinf(q*(float)M_PI*0.65f);}
        float depth=w*(0.52f+0.20f*p->body_fat-0.05f*p->sex_factor);
        for(x=0;x<=seg;x++){
            float u=(float)x/seg,th=(float)(2.0*M_PI)*u,cs=cosf(th),sn=sinf(th);RigSovVertex z;rl_memset(&z,0,sizeof(z));
            z.position=v3(cs*w,yy,sn*depth);z.normal=norm3(v3(cs/w,0,sn/depth));z.uv=(RigSovVec2){u,t};
            if(t<0.28f)skin_pair(&z,J_PELVIS,J_SPINE1,t/0.28f);else if(t<0.62f)skin_pair(&z,J_SPINE1,J_SPINE2,(t-0.28f)/0.34f);else skin_pair(&z,J_SPINE2,J_CHEST,(t-0.62f)/0.38f);
            m->vertices[m->vertex_count++]=z;
        }
    }
    for(y=0;y<rings;y++)for(x=0;x<seg;x++){rl_u32 a=(rl_u32)(base+y*(seg+1)+x),b=a+1,c=(rl_u32)(base+(y+1)*(seg+1)+x),d=c+1;m->triangles[m->triangle_count++]=(RigSovTri){a,c,b};m->triangles[m->triangle_count++]=(RigSovTri){b,c,d};}
    return RIG_SOV_OK;
}

int rig_sov_generate_complete_body(const RigSovBodyParams *in,RigSovMesh *mesh,RigSovSkeleton *skeleton){
    RigSovBodyParams p=in?*in:rig_sov_body_default_params();int rc;unsigned seg,rings;float h,bulk,fat;
    if(!mesh||!skeleton)return RIG_SOV_EINVAL;
    if(!(p.height_m>=0.5f&&p.height_m<=2.5f))return RIG_SOV_ERANGE;
    if (p.radial_segments < 8) p.radial_segments = 8;
    if (p.radial_segments > 128) p.radial_segments = 128;
    if (p.longitudinal_segments < 4) p.longitudinal_segments = 4;
    if (p.longitudinal_segments > 64) p.longitudinal_segments = 64;
    p.muscularity=clampf__rig_dup_29cb97ea(p.muscularity,0,1);p.body_fat=clampf__rig_dup_29cb97ea(p.body_fat,0,1);p.sex_factor=clampf__rig_dup_29cb97ea(p.sex_factor,0,1);p.age_factor=clampf__rig_dup_29cb97ea(p.age_factor,0,1);
    rig_sov_mesh_free(mesh);rig_sov_mesh_init(mesh);build_skeleton(&p,skeleton);seg=p.radial_segments;rings=p.longitudinal_segments;h=p.height_m;bulk=0.85f+0.3f*p.muscularity;fat=0.9f+0.35f*p.body_fat;
    if((rc=append_torso(mesh,skeleton,&p)))goto fail;
    if((rc=append_ellipsoid(mesh,skeleton->joints[J_PELVIS].world_position,v3(p.hip_width*0.55f,h*0.085f,p.hip_width*0.35f*fat),rings,seg,J_PELVIS)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_NECK].world_position,skeleton->joints[J_HEAD].world_position,h*0.045f,h*0.055f,rings/2+2,seg,J_NECK,J_HEAD)))goto fail;
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_HEAD].world_position,v3(0,h*0.035f,0.005f*h)),v3(h*0.073f,h*0.10f,h*0.083f),rings+4,seg+4,J_HEAD)))goto fail;
    /* jaw and facial projection */
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_JAW].world_position,v3(0,-0.005f*h,0.012f*h)),v3(h*0.057f,h*0.045f,h*0.055f),rings/2+2,seg,J_JAW)))goto fail;
    /* shoulders and arms */
    if((rc=append_capsule(mesh,skeleton->joints[J_CLAV_L].world_position,skeleton->joints[J_UPARM_L].world_position,h*0.045f,h*0.055f,rings/2+2,seg,J_CLAV_L,J_UPARM_L)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_UPARM_L].world_position,skeleton->joints[J_FOREARM_L].world_position,h*0.052f*bulk,h*0.040f*bulk,rings,seg,J_UPARM_L,J_FOREARM_L)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_FOREARM_L].world_position,skeleton->joints[J_HAND_L].world_position,h*0.041f*bulk,h*0.026f,rings,seg,J_FOREARM_L,J_HAND_L)))goto fail;
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_HAND_L].world_position,v3(-h*0.035f,0,0)),v3(h*0.055f,h*0.018f,h*0.04f),rings/2+2,seg,J_HAND_L)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_CLAV_R].world_position,skeleton->joints[J_UPARM_R].world_position,h*0.045f,h*0.055f,rings/2+2,seg,J_CLAV_R,J_UPARM_R)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_UPARM_R].world_position,skeleton->joints[J_FOREARM_R].world_position,h*0.052f*bulk,h*0.040f*bulk,rings,seg,J_UPARM_R,J_FOREARM_R)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_FOREARM_R].world_position,skeleton->joints[J_HAND_R].world_position,h*0.041f*bulk,h*0.026f,rings,seg,J_FOREARM_R,J_HAND_R)))goto fail;
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_HAND_R].world_position,v3(h*0.035f,0,0)),v3(h*0.055f,h*0.018f,h*0.04f),rings/2+2,seg,J_HAND_R)))goto fail;
    /* legs and feet */
    if((rc=append_capsule(mesh,skeleton->joints[J_THIGH_L].world_position,skeleton->joints[J_SHIN_L].world_position,h*0.075f*bulk*fat,h*0.052f*bulk,rings+2,seg,J_THIGH_L,J_SHIN_L)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_SHIN_L].world_position,skeleton->joints[J_FOOT_L].world_position,h*0.055f*bulk,h*0.032f,rings+2,seg,J_SHIN_L,J_FOOT_L)))goto fail;
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_FOOT_L].world_position,v3(0,-h*0.01f,h*0.07f)),v3(h*0.045f,h*0.026f,h*0.095f),rings/2+2,seg,J_FOOT_L)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_THIGH_R].world_position,skeleton->joints[J_SHIN_R].world_position,h*0.075f*bulk*fat,h*0.052f*bulk,rings+2,seg,J_THIGH_R,J_SHIN_R)))goto fail;
    if((rc=append_capsule(mesh,skeleton->joints[J_SHIN_R].world_position,skeleton->joints[J_FOOT_R].world_position,h*0.055f*bulk,h*0.032f,rings+2,seg,J_SHIN_R,J_FOOT_R)))goto fail;
    if((rc=append_ellipsoid(mesh,add3(skeleton->joints[J_FOOT_R].world_position,v3(0,-h*0.01f,h*0.07f)),v3(h*0.045f,h*0.026f,h*0.095f),rings/2+2,seg,J_FOOT_R)))goto fail;
    rc=rig_sov_mesh_compute_normals(mesh);if(rc)goto fail;
    return rig_sov_mesh_validate(mesh,NULL,0);
fail:rig_sov_mesh_free(mesh);return rc;
}

int rig_sov_generate_head_from_reconstruction(const RigSovReconParams *p,unsigned rings,unsigned sectors,RigSovMesh *mesh){
    unsigned y,x;rl_size base;int rc;float fw,fh,depth;
    if (!p || !mesh) return RIG_SOV_EINVAL;
    if (rings < 16) rings = 16;
    if (sectors < 24) sectors = 24;
    if (rings > 256 || sectors > 512) return RIG_SOV_ERANGE;
    rig_sov_mesh_free(mesh);rig_sov_mesh_init(mesh);fw=p->face_width>0?p->face_width:0.16f;fh=p->face_height>0?p->face_height:0.22f;depth=p->depth>0?p->depth:fw*0.72f;
    rc=rig_sov_mesh_reserve(mesh,(rl_size)(rings+1)*(sectors+1),(rl_size)rings*sectors*2);if(rc)return rc;base=0;
    for(y=0;y<=rings;y++){
        float v=(float)y/rings,phi=(float)M_PI*v,ny=cosf(phi),sy=sinf(phi);
        for(x=0;x<=sectors;x++){
            float u=(float)x/sectors,th=(float)(2*M_PI)*u,nx=sy*cosf(th),nz=sy*sinf(th);RigSovVertex q;float jawT=clampf__rig_dup_29cb97ea((-ny-0.05f)/0.85f,0,1),jawScale=1.0f-jawT*(1.0f-clampf__rig_dup_29cb97ea(p->jaw_width/fw,0.45f,1.15f));float z=nz*depth*0.5f;
            rl_memset(&q,0,sizeof(q));
            q.position.x=nx*fw*0.5f*jawScale;
            q.position.y=ny*fh*0.5f;
            q.position.z=z;
            /* brow, nose, lips, chin: smooth forward deformations on the anterior half */
            if(nz>0){float xx=q.position.x/(fw*0.5f),yy=q.position.y/(fh*0.5f);float nose=expf(-(xx*xx/0.035f+(yy-0.02f)*(yy-0.02f)/0.16f));float brow=expf(-(xx*xx/0.35f+(yy-0.27f)*(yy-0.27f)/0.025f));float lips=expf(-(xx*xx/0.23f+(yy+0.28f)*(yy+0.28f)/0.015f));float chin=expf(-(xx*xx/0.18f+(yy+0.68f)*(yy+0.68f)/0.055f));q.position.z+=depth*(0.18f*nose+0.035f*brow+0.045f*lips+0.025f*chin);q.position.x+=p->asymmetry*fw*0.015f*(1.0f-yy*yy);}
            q.normal=norm3(v3(nx/(fw*fw),ny/(fh*fh),nz/(depth*depth)));q.uv=(RigSovVec2){u,v};skin_single(&q,J_HEAD);mesh->vertices[mesh->vertex_count++]=q;
        }
    }
    for(y=0;y<rings;y++)for(x=0;x<sectors;x++){rl_u32 a=(rl_u32)(base+y*(sectors+1)+x),b=a+1,c=(rl_u32)(base+(y+1)*(sectors+1)+x),d=c+1;mesh->triangles[mesh->triangle_count++]=(RigSovTri){a,c,b};mesh->triangles[mesh->triangle_count++]=(RigSovTri){b,c,d};}
    rig_sov_mesh_compute_normals(mesh);return rig_sov_mesh_validate(mesh,NULL,0);
}
