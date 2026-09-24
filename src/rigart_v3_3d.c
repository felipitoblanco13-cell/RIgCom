/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define _POSIX_C_SOURCE 200809L

#include "rigart_v3_3d.h"
#include "../include/wsserver.h"
#include "../include/rigart_geo_extra.h"

#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "../include/riglib_math.h"
#include "rig_noext_types.h"

#ifndef PHI3
#  define PHI3      1.6180339887498948482
#  define PHI3_INV  0.6180339887498948482
#  define PHI3_2    2.6180339887498948482
#  define SCHUMANN3 7.83
#  define TAU3      6.28318530717958647692
#  define PI3       3.14159265358979323846
#  define SQRT2_3   1.41421356237309504880
#  define LN_PHI3   0.48121182505960344749
#endif

typedef struct { char *buf; size_t pos; size_t cap; } Buf3d;

static Buf3d bd_new__rig_variant_a16f5087(size_t cap)
{
    Buf3d b; b.buf = calloc(1, cap); b.pos = 0; b.cap = cap; return b;
}
static void bd_cat__rig_variant_99ba61aa(Buf3d *b, const char *s)
{
    if (!b->buf || !s) return 0;
    size_t n = rl_strlen(s);
    if (b->pos + n + 1 >= b->cap) return 0;
    rl_memcpy(b->buf + b->pos, s, n);
    b->pos += n;
    return 0;
}
static void bd_printf__rig_variant_87abe70e(Buf3d *b, const char *fmt, ...)
{
    if (!b->buf) return 0;
    char tmp[8192];
    va_list ap; va_start(ap, fmt);
    vrl_snprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    bd_cat(b, tmp);
    return 0;
}
static char *bd_done__rig_dup_51723c4f(Buf3d *b)
{
    if (!b->buf) return NULL;
    b->buf[b->pos] = '\0';
    return b->buf;
}
static void rdset_js__rig_variant_ae758834(RIgArtResultV3 *r, Buf3d *b)
{
    r->js      = bd_done(b);
    r->js_size = b->pos;
    r->ok      = (b->buf && b->pos > 0);
    r->certeza = (float)PHI3_INV;
    return 0;
}
static void rdset_html__rig_variant_0ce90258(RIgArtResultV3 *r, Buf3d *b)
{
    r->html      = bd_done(b);
    r->html_size = b->pos;
    r->ok        = (b->buf && b->pos > 0);
    r->certeza   = (float)PHI3_INV;
    return 0;
}
const char *rigart_geo_name__rig_dup_25b55c73(RIgArtGeo g)
{
    static const char *n[] = {
        "sphere","cube","cylinder","plane","torus",
        "phi-spiral","lemniscate","dodecahedron","icosphere","cone"
    };
    return (g < RIGART_GEO_COUNT) ? n[g] : "unknown";
}

int rigart_scene_js__rig_variant_da8f2786(const RIgArtSceneCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3d js = bd_new__rig_variant_a16f5087(256 * 1024);

    bd_printf__rig_variant_87abe70e(&js,
        "/* ============================================================\n"
        "   RIgArt v3.0 — §34 Scene Engine · φ=%.10f\n"
        "   Honor 400 · Adreno 720 · WebGL2 · GLSL ES 3.00\n"
        "   Auto-generado por rigart_scene_js__rig_variant_da8f2786()\n"
        "   ============================================================ */\n"
        "'use strict';\n\n",
        PHI3);

    bd_printf__rig_variant_87abe70e(&js,
        "/* ── Constantes φ ─────────────────────────────────────────── */\n"
        "const RG_PHI    = %.10f;\n"
        "const RG_INV    = %.10f;\n"
        "const RG_TAU    = %.10f;\n"
        "const RG_SCH    = %.2f;  /* Schumann resonance Hz */\n\n",
        PHI3, PHI3_INV, TAU3, SCHUMANN3);

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── Mat4 — operaciones column-major (igual que WebGL) ─────── */\n"
        "const Mat4 = {\n"
        "  identity() {\n"
        "    return new Float32Array([\n"
        "      1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1\n"
        "    ]);\n"
        "  },\n"
        "  multiply(a, b) {\n"
        "    const r = new Float32Array(16);\n"
        "    for (let i=0;i<4;i++) for (let j=0;j<4;j++) {\n"
        "      let s=0;\n"
        "      for (let k=0;k<4;k++) s+=a[i+k*4]*b[k+j*4];\n"
        "      r[i+j*4]=s;\n"
        "    }\n"
        "    return r;\n"
        "  },\n"
        "  perspective(fovY, aspect, near, far) {\n"
        "    const f = 1.0/Math.tan(fovY*0.5);\n"
        "    const nf = 1.0/(near-far);\n"
        "    const m = new Float32Array(16);\n"
        "    m[0]=f/aspect; m[5]=f;\n"
        "    m[10]=(far+near)*nf; m[11]=-1;\n"
        "    m[14]=2*far*near*nf;\n"
        "    return m;\n"
        "  },\n"
        "  lookAt(eye, center, up) {\n"
        "    const z = Vec3.normalize(Vec3.sub(eye, center));\n"
        "    const x = Vec3.normalize(Vec3.cross(up, z));\n"
        "    const y = Vec3.cross(z, x);\n"
        "    const m = new Float32Array(16);\n"
        "    m[0]=x[0]; m[4]=x[1]; m[8]=x[2];\n"
        "    m[1]=y[0]; m[5]=y[1]; m[9]=y[2];\n"
        "    m[2]=z[0]; m[6]=z[1]; m[10]=z[2];\n"
        "    m[12]=-Vec3.dot(x,eye);\n"
        "    m[13]=-Vec3.dot(y,eye);\n"
        "    m[14]=-Vec3.dot(z,eye);\n"
        "    m[15]=1;\n"
        "    return m;\n"
        "  },\n"
        "  fromQuat(q) {\n"
        "    const [x,y,z,w]=q;\n"
        "    const m=new Float32Array(16);\n"
        "    m[0]=1-2*(y*y+z*z); m[1]=2*(x*y+z*w); m[2]=2*(x*z-y*w);\n"
        "    m[4]=2*(x*y-z*w);   m[5]=1-2*(x*x+z*z); m[6]=2*(y*z+x*w);\n"
        "    m[8]=2*(x*z+y*w);   m[9]=2*(y*z-x*w);   m[10]=1-2*(x*x+y*y);\n"
        "    m[15]=1;\n"
        "    return m;\n"
        "  },\n"
        "  translation(tx,ty,tz) {\n"
        "    const m=Mat4.identity();\n"
        "    m[12]=tx; m[13]=ty; m[14]=tz;\n"
        "    return m;\n"
        "  },\n"
        "  scale(sx,sy,sz) {\n"
        "    const m=Mat4.identity();\n"
        "    m[0]=sx; m[5]=sy; m[10]=sz;\n"
        "    return m;\n"
        "  },\n"
        "  normalMatrix(m) {\n"
        "    /* inversa-transpuesta 3x3 de la matriz model */\n"
        "    const [a,b,c,,d,e,f,,g,h,i]=m;\n"
        "    const det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);\n"
        "    if(Math.abs(det)<1e-8) return new Float32Array([1,0,0,0,1,0,0,0,1]);\n"
        "    const id=1/det;\n"
        "    return new Float32Array([\n"
        "      (e*i-f*h)*id, -(b*i-c*h)*id,  (b*f-c*e)*id,\n"
        "     -(d*i-f*g)*id,  (a*i-c*g)*id, -(a*f-c*d)*id,\n"
        "      (d*h-e*g)*id, -(a*h-b*g)*id,  (a*e-b*d)*id\n"
        "    ]);\n"
        "  }\n"
        "};\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── Vec3 ──────────────────────────────────────────────────── */\n"
        "const Vec3 = {\n"
        "  add(a,b){return[a[0]+b[0],a[1]+b[1],a[2]+b[2]]},\n"
        "  sub(a,b){return[a[0]-b[0],a[1]-b[1],a[2]-b[2]]},\n"
        "  scale(a,s){return[a[0]*s,a[1]*s,a[2]*s]},\n"
        "  dot(a,b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]},\n"
        "  cross(a,b){return[\n"
        "    a[1]*b[2]-a[2]*b[1],\n"
        "    a[2]*b[0]-a[0]*b[2],\n"
        "    a[0]*b[1]-a[1]*b[0]\n"
        "  ]},\n"
        "  length(a){return Math.sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2])},\n"
        "  normalize(a){\n"
        "    const l=Vec3.length(a)||1e-8;\n"
        "    return[a[0]/l,a[1]/l,a[2]/l];\n"
        "  }\n"
        "};\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── Quaternion — [x,y,z,w] ────────────────────────────────── */\n"
        "const Quat = {\n"
        "  identity(){return[0,0,0,1]},\n"
        "  fromAxisAngle(axis,angle){\n"
        "    const s=Math.sin(angle*0.5);\n"
        "    return[axis[0]*s,axis[1]*s,axis[2]*s,Math.cos(angle*0.5)];\n"
        "  },\n"
        "  multiply(a,b){\n"
        "    const[ax,ay,az,aw]=a,[bx,by,bz,bw]=b;\n"
        "    return[\n"
        "      aw*bx+ax*bw+ay*bz-az*by,\n"
        "      aw*by-ax*bz+ay*bw+az*bx,\n"
        "      aw*bz+ax*by-ay*bx+az*bw,\n"
        "      aw*bw-ax*bx-ay*by-az*bz\n"
        "    ];\n"
        "  },\n"
        "  normalize(q){\n"
        "    const l=Math.sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3])||1e-8;\n"
        "    return q.map(v=>v/l);\n"
        "  },\n"
        "  slerp(a,b,t){\n"
        "    let d=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];\n"
        "    if(d<0){b=b.map(v=>-v);d=-d;}\n"
        "    if(d>0.9995) return Quat.normalize(a.map((v,i)=>v+(b[i]-v)*t));\n"
        "    const th=Math.acos(d), s=Math.sin(th);\n"
        "    const wa=Math.sin((1-t)*th)/s, wb=Math.sin(t*th)/s;\n"
        "    return a.map((v,i)=>v*wa+b[i]*wb);\n"
        "  }\n"
        "};\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── RgNode — unidad atómica del scene graph ────────────────── */\n"
        "class RgNode {\n"
        "  constructor(name='node') {\n"
        "    this.name      = name;\n"
        "    this.pos       = [0,0,0];\n"
        "    this.rot       = Quat.identity();\n"
        "    this.scl       = [1,1,1];\n"
        "    this.localMat  = Mat4.identity();\n"
        "    this.worldMat  = Mat4.identity();\n"
        "    this.normalMat = new Float32Array(9);\n"
        "    this.parent    = null;\n"
        "    this.children  = [];\n"
        "    this.mesh      = null;   /* { vao, indexCount, material } */\n"
        "    this.visible   = true;\n"
        "    this.castShadow  = true;\n"
        "    this.boundRadius = 1.0;  /* para frustum culling */\n"
        "    this._dirty    = true;\n"
        "  }\n"
        "  add(child) {\n"
        "    child.parent = this;\n"
        "    this.children.push(child);\n"
        "    return this;\n"
        "  }\n"
        "  remove(child) {\n"
        "    this.children = this.children.filter(c=>c!==child);\n"
        "    child.parent = null;\n"
        "  }\n"
        "  setPos(x,y,z) { this.pos=[x,y,z]; this._dirty=true; return this; }\n"
        "  setScl(x,y,z) { this.scl=[x,y,z]; this._dirty=true; return this; }\n"
        "  setRot(axis,angle) {\n"
        "    this.rot=Quat.fromAxisAngle(axis,angle);\n"
        "    this._dirty=true; return this;\n"
        "  }\n"
        "  rotateBy(axis,angle) {\n"
        "    this.rot=Quat.multiply(this.rot,Quat.fromAxisAngle(axis,angle));\n"
        "    this._dirty=true; return this;\n"
        "  }\n"
        "  /* Recalcula matrices local→world — solo si _dirty */\n"
        "  updateTransform(parentWorld) {\n"
        "    if (this._dirty || parentWorld) {\n"
        "      const T = Mat4.translation(...this.pos);\n"
        "      const R = Mat4.fromQuat(this.rot);\n"
        "      const S = Mat4.scale(...this.scl);\n"
        "      this.localMat = Mat4.multiply(Mat4.multiply(T,R),S);\n"
        "      this.worldMat = parentWorld\n"
        "        ? Mat4.multiply(parentWorld, this.localMat)\n"
        "        : this.localMat;\n"
        "      this.normalMat = Mat4.normalMatrix(this.worldMat);\n"
        "      this._dirty = false;\n"
        "    }\n"
        "    for (const ch of this.children)\n"
        "      ch.updateTransform(this.worldMat);\n"
        "  }\n"
        "}\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── RgCamera — perspectiva con orbit touch/mouse/gyro ──────── */\n"
        "class RgCamera {\n"
        "  constructor(canvas) {\n"
        "    this.canvas  = canvas;\n"
        "    this.fovY    = Math.PI / (RG_PHI + 1); /* ~62° — proporción φ */\n"
        "    this.near    = 0.1;\n"
        "    this.far     = 1000.0;\n"
        "    this.target  = [0,0,0];\n"
        "    this.theta   = 0.4;   /* azimuth rad */\n"
        "    this.phi     = 0.9;   /* elevación rad */\n"
        "    this.radius  = 5.0;\n"
        "    this.minR    = 0.5;\n"
        "    this.maxR    = 200.0;\n"
        "    this._drag   = null;\n"
        "    this._projMat = Mat4.identity();\n"
        "    this._viewMat = Mat4.identity();\n"
        "    this._bindEvents();\n"
        "  }\n"
        "  get eye() {\n"
        "    const sp=Math.sin(this.phi), cp=Math.cos(this.phi);\n"
        "    const st=Math.sin(this.theta), ct=Math.cos(this.theta);\n"
        "    return Vec3.add(this.target,[\n"
        "      this.radius*sp*ct,\n"
        "      this.radius*cp,\n"
        "      this.radius*sp*st\n"
        "    ]);\n"
        "  }\n"
        "  update(w,h) {\n"
        "    this._projMat = Mat4.perspective(this.fovY, w/h, this.near, this.far);\n"
        "    this._viewMat = Mat4.lookAt(this.eye, this.target, [0,1,0]);\n"
        "  }\n"
        "  get proj()  { return this._projMat; }\n"
        "  get view()  { return this._viewMat; }\n"
        "  /* Frustum culling — esfera en world space */\n"
        "  inFrustum(center, radius) {\n"
        "    /* Planos del frustum calculados inline — Adreno friendly */\n"
        "    const vp=Mat4.multiply(this._projMat, this._viewMat);\n"
        "    const planes=[\n"
        "      [vp[3]+vp[0],vp[7]+vp[4],vp[11]+vp[8],vp[15]+vp[12]],\n"
        "      [vp[3]-vp[0],vp[7]-vp[4],vp[11]-vp[8],vp[15]-vp[12]],\n"
        "      [vp[3]+vp[1],vp[7]+vp[5],vp[11]+vp[9],vp[15]+vp[13]],\n"
        "      [vp[3]-vp[1],vp[7]-vp[5],vp[11]-vp[9],vp[15]-vp[13]],\n"
        "      [vp[3]+vp[2],vp[7]+vp[6],vp[11]+vp[10],vp[15]+vp[14]],\n"
        "      [vp[3]-vp[2],vp[7]-vp[6],vp[11]-vp[10],vp[15]-vp[14]]\n"
        "    ];\n"
        "    for (const [a,b,c,d] of planes) {\n"
        "      const l=Math.sqrt(a*a+b*b+c*c);\n"
        "      if((a*center[0]+b*center[1]+c*center[2]+d)/l < -radius) return false;\n"
        "    }\n"
        "    return true;\n"
        "  }\n"
        "  _bindEvents() {\n"
        "    const c=this.canvas;\n"
        "    /* Mouse orbit */\n"
        "    c.addEventListener('mousedown', e=>{\n"
        "      this._drag={x:e.clientX,y:e.clientY,th:this.theta,ph:this.phi};\n"
        "    });\n"
        "    window.addEventListener('mousemove', e=>{\n"
        "      if(!this._drag) return;\n"
        "      const dx=(e.clientX-this._drag.x)*0.005;\n"
        "      const dy=(e.clientY-this._drag.y)*0.005;\n"
        "      this.theta=this._drag.th+dx;\n"
        "      this.phi=Math.max(0.05,Math.min(Math.PI-0.05,this._drag.ph+dy));\n"
        "    });\n"
        "    window.addEventListener('mouseup', ()=>this._drag=null);\n"
        "    /* Wheel zoom */\n"
        "    c.addEventListener('wheel', e=>{\n"
        "      this.radius=Math.max(this.minR,Math.min(this.maxR,\n"
        "        this.radius*Math.pow(RG_PHI,e.deltaY*0.001)));\n"
        "      e.preventDefault();\n"
        "    },{passive:false});\n"
        "    /* Touch orbit */\n"
        "    let _t0=null,_t1=null;\n"
        "    c.addEventListener('touchstart', e=>{\n"
        "      e.preventDefault();\n"
        "      if(e.touches.length===1){\n"
        "        _t0={x:e.touches[0].clientX,y:e.touches[0].clientY,\n"
        "             th:this.theta,ph:this.phi};\n"
        "      } else if(e.touches.length===2){\n"
        "        _t1=Math.hypot(\n"
        "          e.touches[0].clientX-e.touches[1].clientX,\n"
        "          e.touches[0].clientY-e.touches[1].clientY);\n"
        "      }\n"
        "    },{passive:false});\n"
        "    c.addEventListener('touchmove', e=>{\n"
        "      e.preventDefault();\n"
        "      if(e.touches.length===1 && _t0){\n"
        "        const dx=(e.touches[0].clientX-_t0.x)*0.007;\n"
        "        const dy=(e.touches[0].clientY-_t0.y)*0.007;\n"
        "        this.theta=_t0.th+dx;\n"
        "        this.phi=Math.max(0.05,Math.min(Math.PI-0.05,_t0.ph+dy));\n"
        "      } else if(e.touches.length===2 && _t1!=null){\n"
        "        const d=Math.hypot(\n"
        "          e.touches[0].clientX-e.touches[1].clientX,\n"
        "          e.touches[0].clientY-e.touches[1].clientY);\n"
        "        this.radius=Math.max(this.minR,Math.min(this.maxR,\n"
        "          this.radius*(_t1/d)));\n"
        "        _t1=d;\n"
        "      }\n"
        "    },{passive:false});\n"
        "    c.addEventListener('touchend', ()=>{_t0=null;_t1=null;});\n"
        "    /* Gyroscopio Honor 400 — tilt orbit */\n"
        "    if(window.DeviceOrientationEvent){\n"
        "      window.addEventListener('deviceorientation', e=>{\n"
        "        if(this._drag||_t0) return; /* no interferir con drag manual */\n"
        "        const bx=(e.beta||0)/180*Math.PI;\n"
        "        const gx=(e.gamma||0)/90*Math.PI;\n"
        "        this.theta += gx*0.002;\n"
        "        this.phi    = Math.max(0.3,Math.min(Math.PI*0.7,\n"
        "          1.0+bx*0.003));\n"
        "      });\n"
        "    }\n"
        "  }\n"
        "}\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ════════════════════════════════════════════════════════════\n"
        "   §34 · GEOMETRÍA PROCEDURAL — sin dependencias externas\n"
        "   Cada función retorna { positions, normals, uvs, tangents,\n"
        "   indices } como Float32Array / Uint16Array.\n"
        "   ════════════════════════════════════════════════════════════ */\n"
        "const RgGeo = {\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Esfera UV — subdivisión latitud/longitud */\n"
        "  sphere(radius=1, rings=24, sectors=32) {\n"
        "    const pos=[], nrm=[], uv=[], tan=[], idx=[];\n"
        "    for(let r=0;r<=rings;r++) {\n"
        "      const phi=Math.PI*r/rings;\n"
        "      const sp=Math.sin(phi), cp=Math.cos(phi);\n"
        "      for(let s=0;s<=sectors;s++) {\n"
        "        const theta=RG_TAU*s/sectors;\n"
        "        const st=Math.sin(theta), ct=Math.cos(theta);\n"
        "        const x=radius*sp*ct, y=radius*cp, z=radius*sp*st;\n"
        "        pos.push(x,y,z);\n"
        "        nrm.push(sp*ct,cp,sp*st);\n"
        "        uv.push(s/sectors, r/rings);\n"
        "        tan.push(-st,0,ct,1);\n"
        "      }\n"
        "    }\n"
        "    for(let r=0;r<rings;r++) for(let s=0;s<sectors;s++) {\n"
        "      const a=r*(sectors+1)+s;\n"
        "      idx.push(a,a+sectors+1,a+1, a+1,a+sectors+1,a+sectors+2);\n"
        "    }\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  },\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Cubo — 6 caras con normales y tangentes correctas por cara */\n"
        "  cube(size=1) {\n"
        "    const h=size*0.5;\n"
        "    const faces=[\n"
        "      {n:[0,0,1],t:[1,0,0],u:[0,1,0]},  /* front  */\n"
        "      {n:[0,0,-1],t:[-1,0,0],u:[0,1,0]}, /* back   */\n"
        "      {n:[1,0,0],t:[0,0,-1],u:[0,1,0]},  /* right  */\n"
        "      {n:[-1,0,0],t:[0,0,1],u:[0,1,0]},  /* left   */\n"
        "      {n:[0,1,0],t:[1,0,0],u:[0,0,1]},   /* top    */\n"
        "      {n:[0,-1,0],t:[1,0,0],u:[0,0,-1]}  /* bottom */\n"
        "    ];\n"
        "    const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "    faces.forEach(({n,t,u})=>{\n"
        "      const base=pos.length/3;\n"
        "      const corners=[[-1,-1],[1,-1],[1,1],[-1,1]];\n"
        "      corners.forEach(([a,b])=>{\n"
        "        pos.push(\n"
        "          n[0]*h+t[0]*a*h+u[0]*b*h,\n"
        "          n[1]*h+t[1]*a*h+u[1]*b*h,\n"
        "          n[2]*h+t[2]*a*h+u[2]*b*h);\n"
        "        nrm.push(...n);\n"
        "        uv.push((a+1)*0.5,(b+1)*0.5);\n"
        "        tan.push(...t,1);\n"
        "      });\n"
        "      idx.push(base,base+1,base+2, base,base+2,base+3);\n"
        "    });\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  },\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Toro — proporción mayor/menor = φ */\n"
        "  torus(R=RG_PHI, r=1, segs=48, sides=24) {\n"
        "    const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "    for(let i=0;i<=segs;i++) {\n"
        "      const u=RG_TAU*i/segs;\n"
        "      const cu=Math.cos(u), su=Math.sin(u);\n"
        "      for(let j=0;j<=sides;j++) {\n"
        "        const v=RG_TAU*j/sides;\n"
        "        const cv=Math.cos(v), sv=Math.sin(v);\n"
        "        const x=(R+r*cv)*cu, y=r*sv, z=(R+r*cv)*su;\n"
        "        pos.push(x,y,z);\n"
        "        nrm.push(cu*cv,sv,su*cv);\n"
        "        uv.push(i/segs,j/sides);\n"
        "        tan.push(-su,0,cu,1);\n"
        "      }\n"
        "    }\n"
        "    for(let i=0;i<segs;i++) for(let j=0;j<sides;j++) {\n"
        "      const a=i*(sides+1)+j;\n"
        "      idx.push(a,a+sides+1,a+1, a+1,a+sides+1,a+sides+2);\n"
        "    }\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  },\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Espiral logarítmica φ — tubo 3D */\n"
        "  phiSpiral(turns=3, tubeR=0.08, segs=180, sides=12) {\n"
        "    const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "    const spine=[];\n"
        "    for(let i=0;i<=segs;i++) {\n"
        "      const t=i/segs, angle=RG_TAU*turns*t;\n"
        "      const r=Math.pow(RG_PHI, angle/(RG_TAU))*0.2;\n"
        "      spine.push([r*Math.cos(angle), r*Math.sin(angle)*0.3, r*Math.sin(angle)]);\n"
        "    }\n"
        "    for(let i=0;i<=segs;i++) {\n"
        "      const p=spine[Math.min(i,segs)];\n"
        "      const pn=spine[Math.min(i+1,segs)];\n"
        "      const tDir=Vec3.normalize(Vec3.sub(pn,p));\n"
        "      const arb=Math.abs(tDir[1])<0.9?[0,1,0]:[1,0,0];\n"
        "      const binorm=Vec3.normalize(Vec3.cross(tDir,arb));\n"
        "      const norm=Vec3.normalize(Vec3.cross(binorm,tDir));\n"
        "      for(let j=0;j<=sides;j++) {\n"
        "        const a=RG_TAU*j/sides;\n"
        "        const ca=Math.cos(a), sa=Math.sin(a);\n"
        "        const n=[norm[0]*ca+binorm[0]*sa,norm[1]*ca+binorm[1]*sa,norm[2]*ca+binorm[2]*sa];\n"
        "        pos.push(p[0]+n[0]*tubeR, p[1]+n[1]*tubeR, p[2]+n[2]*tubeR);\n"
        "        nrm.push(...n);\n"
        "        uv.push(i/segs, j/sides);\n"
        "        tan.push(...tDir,1);\n"
        "      }\n"
        "    }\n"
        "    for(let i=0;i<segs;i++) for(let j=0;j<sides;j++) {\n"
        "      const a=i*(sides+1)+j;\n"
        "      idx.push(a,a+sides+1,a+1, a+1,a+sides+1,a+sides+2);\n"
        "    }\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  },\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Lemniscata 3D (∞) — tubo */\n"
        "  lemniscate(scale=1.5, tubeR=0.07, segs=240, sides=10) {\n"
        "    const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "    const spine=[];\n"
        "    for(let i=0;i<=segs;i++) {\n"
        "      const t=RG_TAU*i/segs;\n"
        "      const s2=Math.sin(t)*Math.sin(t), d=1+s2;\n"
        "      spine.push([\n"
        "        scale*Math.cos(t)/d,\n"
        "        scale*0.3*Math.sin(t*2)/(d*1.5),\n"
        "        scale*Math.cos(t)*Math.sin(t)/d\n"
        "      ]);\n"
        "    }\n"
        "    for(let i=0;i<=segs;i++) {\n"
        "      const p=spine[i%segs];\n"
        "      const pn=spine[(i+1)%segs];\n"
        "      const tDir=Vec3.normalize(Vec3.sub(pn,p));\n"
        "      const arb=Math.abs(tDir[1])<0.9?[0,1,0]:[1,0,0];\n"
        "      const binorm=Vec3.normalize(Vec3.cross(tDir,arb));\n"
        "      const norm=Vec3.normalize(Vec3.cross(binorm,tDir));\n"
        "      for(let j=0;j<=sides;j++) {\n"
        "        const a=RG_TAU*j/sides;\n"
        "        const ca=Math.cos(a),sa=Math.sin(a);\n"
        "        const n=[norm[0]*ca+binorm[0]*sa,norm[1]*ca+binorm[1]*sa,norm[2]*ca+binorm[2]*sa];\n"
        "        pos.push(p[0]+n[0]*tubeR,p[1]+n[1]*tubeR,p[2]+n[2]*tubeR);\n"
        "        nrm.push(...n); uv.push(i/segs,j/sides); tan.push(...tDir,1);\n"
        "      }\n"
        "    }\n"
        "    for(let i=0;i<segs;i++) for(let j=0;j<sides;j++) {\n"
        "      const a=i*(sides+1)+j;\n"
        "      idx.push(a,a+sides+1,a+1, a+1,a+sides+1,a+sides+2);\n"
        "    }\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  },\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "  /* Plano — W×H subdividido */\n"
        "  plane(w=2, h=2, segW=1, segH=1) {\n"
        "    const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "    for(let j=0;j<=segH;j++) for(let i=0;i<=segW;i++) {\n"
        "      pos.push(w*(i/segW-0.5),0,h*(j/segH-0.5));\n"
        "      nrm.push(0,1,0); uv.push(i/segW,j/segH); tan.push(1,0,0,1);\n"
        "    }\n"
        "    for(let j=0;j<segH;j++) for(let i=0;i<segW;i++) {\n"
        "      const a=j*(segW+1)+i;\n"
        "      idx.push(a,a+segW+1,a+1, a+1,a+segW+1,a+segW+2);\n"
        "    }\n"
        "    return {pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "            uv:new Float32Array(uv),tan:new Float32Array(tan),\n"
        "            idx:new Uint16Array(idx)};\n"
        "  }\n"
        "};\n\n");

    bd_cat__rig_variant_99ba61aa(&js,
        "/* ── RgMesh — sube geometría a WebGL2 VAO ───────────────────── */\n"
        "function rgUploadMesh(gl, geo, prog) {\n"
        "  const vao = gl.createVertexArray();\n"
        "  gl.bindVertexArray(vao);\n"
        "  function buf(data, attrib, size) {\n"
        "    const b = gl.createBuffer();\n"
        "    gl.bindBuffer(gl.ARRAY_BUFFER, b);\n"
        "    gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);\n"
        "    const loc = gl.getAttribLocation(prog, attrib);\n"
        "    if (loc >= 0) {\n"
        "      gl.enableVertexAttribArray(loc);\n"
        "      gl.vertexAttribPointer(loc, size, gl.FLOAT, false, 0, 0);\n"
        "    }\n"
        "  }\n"
        "  buf(geo.pos, 'a_pos',     3);\n"
        "  buf(geo.nrm, 'a_normal',  3);\n"
        "  buf(geo.uv,  'a_uv',      2);\n"
        "  buf(geo.tan, 'a_tangent', 4);\n"
        "  const ib = gl.createBuffer();\n"
        "  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib);\n"
        "  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, geo.idx, gl.STATIC_DRAW);\n"
        "  gl.bindVertexArray(null);\n"
        "  return { vao, indexCount: geo.idx.length, ib };\n"
        "}\n\n");

    rdset_js__rig_variant_ae758834(out, &js);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

static void geo_cat_fn__rig_dup_687d4150(void *b, const char *s) { bd_cat__rig_variant_99ba61aa((Buf3d*)b, s); }
int rigart_renderer_html__rig_variant_cde2ec35(const RIgArtRendererCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3d html = bd_new__rig_variant_a16f5087(512 * 1024);

    float lx = ctx->light_dir[0] != 0.0f ? ctx->light_dir[0] :  0.577f;
    float ly = ctx->light_dir[1] != 0.0f ? ctx->light_dir[1] :  0.577f;
    float lz = ctx->light_dir[2] != 0.0f ? ctx->light_dir[2] :  0.577f;
    float lr = ctx->light_color[0] > 0 ? ctx->light_color[0] : 1.0f;
    float lg = ctx->light_color[1] > 0 ? ctx->light_color[1] : 0.92f;
    float lb = ctx->light_color[2] > 0 ? ctx->light_color[2] : 0.76f;
    float rough   = ctx->roughness   > 0 ? ctx->roughness   : 0.35f;
    float metal   = ctx->metallic    > 0 ? ctx->metallic    : 0.9f;
    float scale   = ctx->tex_scale   > 0 ? ctx->tex_scale   : 2.0f;
    const char *title = ctx->title[0] ? ctx->title : "RIGADIEL · Motor 3D Soberano";
    const char *bg    = ctx->bg_color[0] ? ctx->bg_color : "#030208";

    bd_printf__rig_variant_87abe70e(&html,
        "<!DOCTYPE html>\n"
        "<html lang=\"es\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\"/>\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,"
        "viewport-fit=cover\"/>\n"
        "<meta name=\"color-scheme\" content=\"dark\"/>\n"
        "<title>%s</title>\n"
        "<style>\n"
        "/* RIgArt v3.0 — §35 Renderer · φ=1.618 · Honor 400 */\n"
        "*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}\n"
        "html,body{\n"
        "  width:100%%;height:100%%;overflow:hidden;\n"
        "  background:%s;\n"
        "  font-family:'Cinzel Decorative','Cinzel','Palatino Linotype','Palatino',Georgia,serif;\n"
        "  color-scheme:dark;\n"
        "}\n"
        "#rg-canvas{\n"
        "  display:block;width:100%%;height:100%%;\n"
        "  touch-action:none;\n"
        "  image-rendering:auto;\n"
        "}\n"
        "#rg-stats{\n"
        "  position:fixed;top:16px;left:16px;\n"
        "  font-family:'JetBrains Mono','Fira Code','Courier New','Courier',monospace;\n"
        "  font-size:11px;line-height:1.6;\n"
        "  color:rgba(200,164,90,0.7);\n"
        "  pointer-events:none;\n"
        "  letter-spacing:0.04em;\n"
        "  text-shadow:0 0 8px rgba(200,164,90,0.3);\n"
        "}\n"
        "#rg-title{\n"
        "  position:fixed;top:16px;right:20px;\n"
        "  font-size:clamp(9px,2vw,13px);\n"
        "  letter-spacing:0.35em;\n"
        "  color:rgba(200,164,90,0.4);\n"
        "  pointer-events:none;\n"
        "  text-transform:uppercase;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<canvas id=\"rg-canvas\"></canvas>\n"
        "<div id=\"rg-stats\">RIgArt v3.0 · init...</div>\n"
        "<div id=\"rg-title\">%s</div>\n"
        "<script>\n"
        "/* ── RIgArt v3.0 · §35 Renderer Loop ─────────────────── */\n"
        "'use strict';\n\n",
        title, bg, title);

    bd_printf__rig_variant_87abe70e(&html,
        "const RG_PHI=%.10f,RG_INV=%.10f,RG_TAU=%.10f,RG_SCH=%.2f;\n\n",
        PHI3, PHI3_INV, TAU3, SCHUMANN3);

    bd_cat__rig_variant_99ba61aa(&html,
        "/* ── Mat4 ────────────────────────────────────────────────── */\n"
        "const Mat4={\n"
        "identity(){return new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]);},\n"
        "multiply(a,b){const r=new Float32Array(16);\n"
        "  for(let i=0;i<4;i++)for(let j=0;j<4;j++){let s=0;\n"
        "    for(let k=0;k<4;k++)s+=a[i+k*4]*b[k+j*4];r[i+j*4]=s;}return r;},\n"
        "perspective(fY,asp,n,f){const t=1/Math.tan(fY*.5),nf=1/(n-f),m=new Float32Array(16);\n"
        "  m[0]=t/asp;m[5]=t;m[10]=(f+n)*nf;m[11]=-1;m[14]=2*f*n*nf;return m;},\n"
        "lookAt(e,c,u){const Vec3=window._RG_Vec3;\n"
        "  const z=Vec3.normalize(Vec3.sub(e,c));\n"
        "  const x=Vec3.normalize(Vec3.cross(u,z));\n"
        "  const y=Vec3.cross(z,x);\n"
        "  const m=new Float32Array(16);\n"
        "  m[0]=x[0];m[4]=x[1];m[8]=x[2];\n"
        "  m[1]=y[0];m[5]=y[1];m[9]=y[2];\n"
        "  m[2]=z[0];m[6]=z[1];m[10]=z[2];\n"
        "  m[12]=-Vec3.dot(x,e);m[13]=-Vec3.dot(y,e);m[14]=-Vec3.dot(z,e);m[15]=1;\n"
        "  return m;},\n"
        "fromQuat(q){const[x,y,z,w]=q,m=new Float32Array(16);\n"
        "  m[0]=1-2*(y*y+z*z);m[1]=2*(x*y+z*w);m[2]=2*(x*z-y*w);\n"
        "  m[4]=2*(x*y-z*w);m[5]=1-2*(x*x+z*z);m[6]=2*(y*z+x*w);\n"
        "  m[8]=2*(x*z+y*w);m[9]=2*(y*z-x*w);m[10]=1-2*(x*x+y*y);m[15]=1;\n"
        "  return m;},\n"
        "translation(tx,ty,tz){const m=Mat4.identity();m[12]=tx;m[13]=ty;m[14]=tz;return m;},\n"
        "scale(sx,sy,sz){const m=Mat4.identity();m[0]=sx;m[5]=sy;m[10]=sz;return m;},\n"
        "normalMatrix(m){const[a,b,c,,d,e,f,,g,h,ii]=m;\n"
        "  const det=a*(e*ii-f*h)-b*(d*ii-f*g)+c*(d*h-e*g);\n"
        "  if(Math.abs(det)<1e-8)return new Float32Array([1,0,0,0,1,0,0,0,1]);\n"
        "  const id=1/det;\n"
        "  return new Float32Array([(e*ii-f*h)*id,-(b*ii-c*h)*id,(b*f-c*e)*id,\n"
        "    -(d*ii-f*g)*id,(a*ii-c*g)*id,-(a*f-c*d)*id,\n"
        "    (d*h-e*g)*id,-(a*h-b*g)*id,(a*e-b*d)*id]);}\n"
        "};\n"
        "const Vec3={add:(a,b)=>[a[0]+b[0],a[1]+b[1],a[2]+b[2]],\n"
        "  sub:(a,b)=>[a[0]-b[0],a[1]-b[1],a[2]-b[2]],\n"
        "  scale:(a,s)=>[a[0]*s,a[1]*s,a[2]*s],\n"
        "  dot:(a,b)=>a[0]*b[0]+a[1]*b[1]+a[2]*b[2],\n"
        "  cross:(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]],\n"
        "  length:a=>Math.sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]),\n"
        "  normalize(a){const l=Vec3.length(a)||1e-8;return[a[0]/l,a[1]/l,a[2]/l];}};\n"
        "window._RG_Vec3=Vec3;\n"
        "const Quat={identity:()=>[0,0,0,1],\n"
        "  fromAxisAngle(ax,a){const s=Math.sin(a*.5);\n"
        "    return[ax[0]*s,ax[1]*s,ax[2]*s,Math.cos(a*.5)];},\n"
        "  multiply([ax,ay,az,aw],[bx,by,bz,bw]){\n"
        "    return[aw*bx+ax*bw+ay*bz-az*by,aw*by-ax*bz+ay*bw+az*bx,\n"
        "           aw*bz+ax*by-ay*bx+az*bw,aw*bw-ax*bx-ay*by-az*bz];},\n"
        "  normalize(q){const l=Math.sqrt(q.reduce((s,v)=>s+v*v,0))||1e-8;\n"
        "    return q.map(v=>v/l);}};\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "/* ── Shaders PBR — §31 adaptado para renderer soberano ────── */\n"
        "const VS_PBR = `#version 300 es\n"
        "precision highp float;\n"
        "/* RIgArt v3.0 — PBR Vertex · §35 · φ=1.618 */\n"
        "in vec3 a_pos;\n"
        "in vec3 a_normal;\n"
        "in vec2 a_uv;\n"
        "in vec4 a_tangent;\n"
        "uniform mat4 u_mvp;\n"
        "uniform mat4 u_model;\n"
        "uniform mat3 u_normal_mat;\n"
        "out vec3 v_pos_ws;\n"
        "out vec3 v_normal;\n"
        "out vec2 v_uv;\n"
        "out mat3 v_tbn;\n"
        "void main(){\n"
        "  vec4 ws=u_model*vec4(a_pos,1.0);\n"
        "  v_pos_ws=ws.xyz;\n"
        "  v_normal=normalize(u_normal_mat*a_normal);\n"
        "  v_uv=a_uv;\n"
        "  vec3 T=normalize(u_normal_mat*a_tangent.xyz);\n"
        "  vec3 N=v_normal;\n"
        "  vec3 B=cross(N,T)*a_tangent.w;\n"
        "  v_tbn=mat3(T,B,N);\n"
        "  gl_Position=u_mvp*vec4(a_pos,1.0);\n"
        "}`;\n\n");

    bd_printf__rig_variant_87abe70e(&html,
        "const FS_PBR = `#version 300 es\n"
        "precision highp float;\n"
        "/* RIgArt v3.0 — PBR Fragment · §35 · Adreno 720 */\n"
        "uniform vec3  u_cam_pos;\n"
        "uniform vec3  u_light_dir;\n"
        "uniform vec3  u_light_col;\n"
        "uniform float u_time;\n"
        "uniform float u_roughness;\n"
        "uniform float u_metallic;\n"
        "uniform float u_scale;\n"
        "in vec3 v_pos_ws;\n"
        "in vec3 v_normal;\n"
        "in vec2 v_uv;\n"
        "in mat3 v_tbn;\n"
        "out vec4 fragColor;\n"
        "#define PHI  1.6180339887\n"
        "#define SCH  7.83\n"
        "float hash(vec2 p){\n"
        "  p=fract(p*vec2(443.897,441.423));\n"
        "  p+=dot(p,p.yx+19.19);\n"
        "  return fract((p.x+p.y)*p.x);\n"
        "}\n"
        "float GGX_D(float NdH,float a){\n"
        "  float a2=a*a,d=NdH*NdH*(a2-1.0)+1.0;\n"
        "  return a2/(3.14159*d*d);\n"
        "}\n"
        "float GGX_V(float NdV,float NdL,float k){\n"
        "  return (NdV/(NdV*(1.0-k)+k))*(NdL/(NdL*(1.0-k)+k));\n"
        "}\n"
        "vec3 Fresnel(float VdH,vec3 F0){\n"
        "  return F0+(1.0-F0)*pow(1.0-VdH,5.0);\n"
        "}\n"
        "vec3 pbr(vec3 N,vec3 V,vec3 L,vec3 alb,float rough,float metal){\n"
        "  vec3 H=normalize(V+L);\n"
        "  float NdL=max(dot(N,L),0.0),NdV=max(dot(N,V),0.001);\n"
        "  float NdH=max(dot(N,H),0.0),VdH=max(dot(V,H),0.0);\n"
        "  float a=rough*rough,k=(rough+1.0)*(rough+1.0)/8.0;\n"
        "  vec3 F0=mix(vec3(0.04),alb,metal);\n"
        "  vec3 F=Fresnel(VdH,F0);\n"
        "  vec3 spec=GGX_D(NdH,a)*GGX_V(NdV,NdL,k)*F/(4.0*NdV*NdL+0.001);\n"
        "  vec3 kD=(1.0-F)*(1.0-metal);\n"
        "  return(kD*alb/3.14159+spec)*u_light_col*NdL;\n"
        "}\n"
        "void main(){\n"
        "  vec2 uv=v_uv*u_scale;\n"
        "  /* Normal map procedural — oro cepillado anisótropo */\n"
        "  float sc1=hash(vec2(floor(uv.y*120.0),u_time*0.01))*0.5;\n"
        "  float sc2=hash(vec2(floor(uv.y*300.0)+1.0,0.0))*0.25;\n"
        "  vec3 Nmap=normalize(vec3(0.0,(sc1+sc2-0.375)*0.4,1.0));\n"
        "  vec3 N=normalize(v_tbn*Nmap);\n"
        "  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
        "  vec3 L=normalize(u_light_dir);\n"
        "  /* Albedo — oro 24K con variación de scratch */\n"
        "  float scratch=hash(vec2(floor(uv.y*200.0),0.5));\n"
        "  vec3 alb=mix(vec3(0.48,0.36,0.08),vec3(0.83,0.68,0.22),scratch);\n"
        "  vec3 col=pbr(N,V,L,alb,u_roughness,u_metallic);\n"
        "  /* Reflexión ambiente IBL simulada */\n"
        "  vec3 R=reflect(-V,N);\n"
        "  vec3 sky=mix(vec3(0.06,0.04,0.01),vec3(0.20,0.15,0.04),R.y*0.5+0.5);\n"
        "  col+=sky*(1.0-u_roughness)*0.4;\n"
        "  /* Schumann shimmer — %.2f Hz */\n"
        "  col*=1.0+0.035*sin(u_time*6.28318*SCH);\n"
        "  /* ACES tonemapping */\n"
        "  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
        "  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
        "}`;\n\n",
        SCHUMANN3);

    bd_cat__rig_variant_99ba61aa(&html,
        "function compileShader(gl, type, src) {\n"
        "  const s = gl.createShader(type);\n"
        "  gl.shaderSource(s, src);\n"
        "  gl.compileShader(s);\n"
        "  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS))\n"
        "    console.error('RIgArt shader:', gl.getShaderInfoLog(s));\n"
        "  return s;\n"
        "}\n"
        "function linkProgram(gl, vs, fs) {\n"
        "  const p = gl.createProgram();\n"
        "  gl.attachShader(p, compileShader(gl, gl.VERTEX_SHADER,   vs));\n"
        "  gl.attachShader(p, compileShader(gl, gl.FRAGMENT_SHADER, fs));\n"
        "  gl.linkProgram(p);\n"
        "  if (!gl.getProgramParameter(p, gl.LINK_STATUS))\n"
        "    console.error('RIgArt link:', gl.getProgramInfoLog(p));\n"
        "  return p;\n"
        "}\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "function uploadMesh(gl, geo, prog) {\n"
        "  const vao = gl.createVertexArray();\n"
        "  gl.bindVertexArray(vao);\n"
        "  function vbuf(data, attrib, size) {\n"
        "    const b = gl.createBuffer();\n"
        "    gl.bindBuffer(gl.ARRAY_BUFFER, b);\n"
        "    gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);\n"
        "    const loc = gl.getAttribLocation(prog, attrib);\n"
        "    if (loc >= 0) {\n"
        "      gl.enableVertexAttribArray(loc);\n"
        "      gl.vertexAttribPointer(loc, size, gl.FLOAT, false, 0, 0);\n"
        "    }\n"
        "  }\n"
        "  vbuf(geo.pos, 'a_pos', 3); vbuf(geo.nrm, 'a_normal', 3);\n"
        "  vbuf(geo.uv,  'a_uv',  2); vbuf(geo.tan, 'a_tangent', 4);\n"
        "  const ib = gl.createBuffer();\n"
        "  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib);\n"
        "  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, geo.idx, gl.STATIC_DRAW);\n"
        "  gl.bindVertexArray(null);\n"
        "  return { vao, indexCount: geo.idx.length };\n"
        "}\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "/* ── Geometría procedural (§34 inline) ─────────────────────── */\n"
        "function geoSphere(R=1,rings=32,secs=48){\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "  for(let r=0;r<=rings;r++){const phi=Math.PI*r/rings,sp=Math.sin(phi),cp=Math.cos(phi);\n"
        "    for(let s=0;s<=secs;s++){const th=RG_TAU*s/secs,st=Math.sin(th),ct=Math.cos(th);\n"
        "      pos.push(R*sp*ct,R*cp,R*sp*st);nrm.push(sp*ct,cp,sp*st);\n"
        "      uv.push(s/secs,r/rings);tan.push(-st,0,ct,1);}}\n"
        "  for(let r=0;r<rings;r++)for(let s=0;s<secs;s++){\n"
        "    const a=r*(secs+1)+s;\n"
        "    idx.push(a,a+secs+1,a+1,a+1,a+secs+1,a+secs+2);}\n"
        "  return{pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "         uv:new Float32Array(uv),tan:new Float32Array(tan),idx:new Uint16Array(idx)};}\n"
        "function geoTorus(R=RG_PHI,r=0.5,segs=64,sides=32){\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "  for(let i=0;i<=segs;i++){const u=RG_TAU*i/segs,cu=Math.cos(u),su=Math.sin(u);\n"
        "    for(let j=0;j<=sides;j++){const v=RG_TAU*j/sides,cv=Math.cos(v),sv=Math.sin(v);\n"
        "      pos.push((R+r*cv)*cu,r*sv,(R+r*cv)*su);\n"
        "      nrm.push(cu*cv,sv,su*cv);uv.push(i/segs,j/sides);tan.push(-su,0,cu,1);}}\n"
        "  for(let i=0;i<segs;i++)for(let j=0;j<sides;j++){\n"
        "    const a=i*(sides+1)+j;\n"
        "    idx.push(a,a+sides+1,a+1,a+1,a+sides+1,a+sides+2);}\n"
        "  return{pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "         uv:new Float32Array(uv),tan:new Float32Array(tan),idx:new Uint16Array(idx)};}\n"
        "function geoLemniscate(sc=1.8,tubeR=0.06,segs=240,sides=12){\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[],spine=[];\n"
        "  for(let i=0;i<=segs;i++){const t=RG_TAU*i/segs,s2=Math.sin(t)*Math.sin(t),d=1+s2;\n"
        "    spine.push([sc*Math.cos(t)/d,sc*0.25*Math.sin(t*2)/(d*1.2),sc*Math.cos(t)*Math.sin(t)/d]);}\n"
        "  const normalize=v=>{const l=Math.sqrt(v.reduce((s,x)=>s+x*x,0))||1e-8;return v.map(x=>x/l);};\n"
        "  const sub=(a,b)=>a.map((v,i)=>v-b[i]);\n"
        "  const cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];\n"
        "  for(let i=0;i<=segs;i++){const p=spine[i%segs],pn=spine[(i+1)%segs];\n"
        "    const tD=normalize(sub(pn,p));\n"
        "    const arb=Math.abs(tD[1])<0.9?[0,1,0]:[1,0,0];\n"
        "    const bn=normalize(cross(tD,arb)),nm=normalize(cross(bn,tD));\n"
        "    for(let j=0;j<=sides;j++){const a=RG_TAU*j/sides,ca=Math.cos(a),sa=Math.sin(a);\n"
        "      const n=[nm[0]*ca+bn[0]*sa,nm[1]*ca+bn[1]*sa,nm[2]*ca+bn[2]*sa];\n"
        "      pos.push(p[0]+n[0]*tubeR,p[1]+n[1]*tubeR,p[2]+n[2]*tubeR);\n"
        "      nrm.push(...n);uv.push(i/segs,j/sides);tan.push(...tD,1);}}\n"
        "  for(let i=0;i<segs;i++)for(let j=0;j<sides;j++){\n"
        "    const a=i*(sides+1)+j;\n"
        "    idx.push(a,a+sides+1,a+1,a+1,a+sides+1,a+sides+2);}\n"
        "  return{pos:new Float32Array(pos),nrm:new Float32Array(nrm),\n"
        "         uv:new Float32Array(uv),tan:new Float32Array(tan),idx:new Uint16Array(idx)};}\n\n");

    bd_printf__rig_variant_87abe70e(&html,
        "/* ── Init — WebGL2 context + escena inicial ─────────────────── */\n"
        "(function rgInit() {\n"
        "  const canvas = document.getElementById('rg-canvas');\n"
        "  const gl = canvas.getContext('webgl2', {\n"
        "    antialias: true,\n"
        "    alpha: false,\n"
        "    depth: true,\n"
        "    stencil: false,\n"
        "    powerPreference: 'high-performance',\n"
        "    colorSpace: 'display-p3'  /* Honor 400 Display P3 */\n"
        "  });\n"
        "  if (!gl) {\n"
        "    document.body.innerHTML = '<p style=\"color:#f44;font-family:monospace;padding:2rem\">"
        "WebGL2 requerido · Honor 400 / Chrome 130+</p>';\n"
        "    return;\n"
        "  }\n\n"
        "  /* ── Compilar programa PBR ──────────────────────────────── */\n"
        "  const prog = linkProgram(gl, VS_PBR, FS_PBR);\n"
        "  gl.useProgram(prog);\n\n"
        "  /* ── Uniforms locations ─────────────────────────────────── */\n"
        "  const U = {\n"
        "    mvp:       gl.getUniformLocation(prog,'u_mvp'),\n"
        "    model:     gl.getUniformLocation(prog,'u_model'),\n"
        "    normalMat: gl.getUniformLocation(prog,'u_normal_mat'),\n"
        "    camPos:    gl.getUniformLocation(prog,'u_cam_pos'),\n"
        "    lightDir:  gl.getUniformLocation(prog,'u_light_dir'),\n"
        "    lightCol:  gl.getUniformLocation(prog,'u_light_col'),\n"
        "    time:      gl.getUniformLocation(prog,'u_time'),\n"
        "    roughness: gl.getUniformLocation(prog,'u_roughness'),\n"
        "    metallic:  gl.getUniformLocation(prog,'u_metallic'),\n"
        "    scale:     gl.getUniformLocation(prog,'u_scale'),\n"
        "  };\n\n"
        "  /* ── Parámetros de luz ──────────────────────────────────── */\n"
        "  const LIGHT_DIR = [%.3ff, %.3ff, %.3ff];\n"
        "  const LIGHT_COL = [%.3ff, %.3ff, %.3ff];\n"
        "  const ROUGHNESS = %.3f, METALLIC = %.3f, SCALE = %.2f;\n\n",
        lx, ly, lz, lr, lg, lb, rough, metal, scale);

    bd_cat__rig_variant_99ba61aa(&html,
        "  /* ── Subir geometría a GPU ──────────────────────────────── */\n"
        "  const meshSphere    = uploadMesh(gl, geoSphere(1.0,32,48), prog);\n"
        "  const meshTorus     = uploadMesh(gl, geoTorus(RG_PHI,0.35,64,32), prog);\n"
        "  const meshLemniscat = uploadMesh(gl, geoLemniscate(2.0,0.055,300,14), prog);\n\n");

    rigart_geo_extra_inject(&html, geo_cat_fn);
    bd_cat__rig_variant_99ba61aa(&html,
        "  const meshDodec  = uploadMesh(gl, geoDodecahedron(1.0), prog);\n"
        "  const meshIco    = uploadMesh(gl, geoIcosphere(1.0, 2), prog);\n"
        "  const meshCyl    = uploadMesh(gl, geoCylinder(0.4,0.4,2.5,32), prog);\n"
        "  const meshCone   = uploadMesh(gl, geoCone(0.8, RG_PHI*2, 32), prog);\n\n"
        "  /* ── Scene graph ────────────────────────────────────────── */\n"
        "  /* Nodo raíz */\n"
        "  const root = { pos:[0,0,0], rot:[0,0,0,1], scl:[1,1,1],\n"
        "    localMat:Mat4.identity(), worldMat:Mat4.identity(),\n"
        "    children:[], mesh:null, visible:true, boundRadius:10 };\n\n"
        "  function makeNode(mesh, px=0,py=0,pz=0, br=1.5) {\n"
        "    return { pos:[px,py,pz], rot:[0,0,0,1], scl:[1,1,1],\n"
        "      localMat:Mat4.identity(), worldMat:Mat4.identity(),\n"
        "      normalMat:new Float32Array(9),\n"
        "      children:[], mesh, visible:true, boundRadius:br };\n"
        "  }\n"
        "  const nodeSphere0 = makeNode(meshSphere,  0,  0,  0, 1.0);\n"
        "  const nodeSphere1 = makeNode(meshSphere,  3.5, 0,  0, 1.0);\n"
        "  const nodeSphere2 = makeNode(meshSphere, -3.5, 0,  0, 1.0);\n"
        "  const nodeTorus   = makeNode(meshTorus,   0,  0,  0, 3.0);\n"
        "  const nodeLemni   = makeNode(meshLemniscat,0, 0,  0, 3.0);\n"
        "  root.children = [nodeSphere0, nodeSphere1, nodeSphere2,\n"
        "                   nodeTorus,   nodeLemni];\n\n"
        "  /* ── Cámara ─────────────────────────────────────────────── */\n"
        "  let camTheta=0.4, camPhi=1.1, camRadius=8.0;\n"
        "  let drag=null, lastPinch=null;\n"
        "  canvas.addEventListener('mousedown',e=>{drag={x:e.clientX,y:e.clientY,\n"
        "    th:camTheta,ph:camPhi};});\n"
        "  window.addEventListener('mousemove',e=>{\n"
        "    if(!drag)return;\n"
        "    camTheta=drag.th+(e.clientX-drag.x)*0.005;\n"
        "    camPhi=Math.max(0.05,Math.min(Math.PI-0.05,drag.ph+(e.clientY-drag.y)*0.005));\n"
        "  });\n"
        "  window.addEventListener('mouseup',()=>drag=null);\n"
        "  canvas.addEventListener('wheel',e=>{\n"
        "    camRadius=Math.max(1.5,Math.min(30,camRadius*Math.pow(RG_PHI,e.deltaY*0.001)));\n"
        "    e.preventDefault();\n"
        "  },{passive:false});\n"
        "  canvas.addEventListener('touchstart',e=>{\n"
        "    e.preventDefault();\n"
        "    if(e.touches.length===1)drag={x:e.touches[0].clientX,y:e.touches[0].clientY,\n"
        "      th:camTheta,ph:camPhi};\n"
        "    else if(e.touches.length===2)\n"
        "      lastPinch=Math.hypot(e.touches[0].clientX-e.touches[1].clientX,\n"
        "                           e.touches[0].clientY-e.touches[1].clientY);\n"
        "  },{passive:false});\n"
        "  canvas.addEventListener('touchmove',e=>{\n"
        "    e.preventDefault();\n"
        "    if(e.touches.length===1&&drag){\n"
        "      camTheta=drag.th+(e.touches[0].clientX-drag.x)*0.007;\n"
        "      camPhi=Math.max(0.05,Math.min(Math.PI-0.05,drag.ph+(e.touches[0].clientY-drag.y)*0.007));\n"
        "    } else if(e.touches.length===2&&lastPinch!=null){\n"
        "      const d=Math.hypot(e.touches[0].clientX-e.touches[1].clientX,\n"
        "                         e.touches[0].clientY-e.touches[1].clientY);\n"
        "      camRadius=Math.max(1.5,Math.min(30,camRadius*(lastPinch/d)));\n"
        "      lastPinch=d;\n"
        "    }\n"
        "  },{passive:false});\n"
        "  canvas.addEventListener('touchend',()=>{drag=null;lastPinch=null;});\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "  /* ── Recalcular transforms ──────────────────────────────── */\n"
        "  function updateNode(node, parentWorld) {\n"
        "    const T=Mat4.translation(...node.pos);\n"
        "    const R=Mat4.fromQuat(node.rot);\n"
        "    const S=Mat4.scale(...node.scl);\n"
        "    node.localMat=Mat4.multiply(Mat4.multiply(T,R),S);\n"
        "    node.worldMat=parentWorld?Mat4.multiply(parentWorld,node.localMat):node.localMat;\n"
        "    node.normalMat=Mat4.normalMatrix(node.worldMat);\n"
        "    for(const ch of node.children) updateNode(ch,node.worldMat);\n"
        "  }\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "  /* ── Frustum culling — sphere test ─────────────────────── */\n"
        "  function inFrustum(vp, cx,cy,cz, r) {\n"
        "    const planes=[\n"
        "      [vp[3]+vp[0],vp[7]+vp[4],vp[11]+vp[8],vp[15]+vp[12]],\n"
        "      [vp[3]-vp[0],vp[7]-vp[4],vp[11]-vp[8],vp[15]-vp[12]],\n"
        "      [vp[3]+vp[1],vp[7]+vp[5],vp[11]+vp[9],vp[15]+vp[13]],\n"
        "      [vp[3]-vp[1],vp[7]-vp[5],vp[11]-vp[9],vp[15]-vp[13]],\n"
        "      [vp[3]+vp[2],vp[7]+vp[6],vp[11]+vp[10],vp[15]+vp[14]],\n"
        "      [vp[3]-vp[2],vp[7]-vp[6],vp[11]-vp[10],vp[15]-vp[14]]\n"
        "    ];\n"
        "    for(const[a,b,c,d]of planes){\n"
        "      const l=Math.sqrt(a*a+b*b+c*c);\n"
        "      if((a*cx+b*cy+c*cz+d)/l<-r)return false;\n"
        "    }\n"
        "    return true;\n"
        "  }\n\n");

    bd_cat__rig_variant_99ba61aa(&html,
        "  /* ── Variables de loop ──────────────────────────────────── */\n"
        "  let t0=performance.now(), frames=0, fps=0, drawCalls=0;\n"
        "  const statsEl = document.getElementById('rg-stats');\n\n"
        "  /* ── Draw node recursivo ────────────────────────────────── */\n"
        "  function drawNode(node, vpMat, eye) {\n"
        "    if (!node.visible) return;\n"
        "    if (node.mesh) {\n"
        "      /* Frustum cull */\n"
        "      const wx=node.worldMat[12],wy=node.worldMat[13],wz=node.worldMat[14];\n"
        "      if (!inFrustum(vpMat, wx,wy,wz, node.boundRadius)) return;\n"
        "      /* Bind uniforms */\n"
        "      const mvp=Mat4.multiply(vpMat, node.worldMat);\n"
        "      gl.uniformMatrix4fv(U.mvp,    false, mvp);\n"
        "      gl.uniformMatrix4fv(U.model,  false, node.worldMat);\n"
        "      gl.uniformMatrix3fv(U.normalMat, false, node.normalMat);\n"
        "      /* Draw */\n"
        "      gl.bindVertexArray(node.mesh.vao);\n"
        "      gl.drawElements(gl.TRIANGLES, node.mesh.indexCount, gl.UNSIGNED_SHORT, 0);\n"
        "      gl.bindVertexArray(null);\n"
        "      drawCalls++;\n"
        "    }\n"
        "    for (const ch of node.children) drawNode(ch, vpMat, eye);\n"
        "  }\n\n"
        "  /* ── requestAnimationFrame loop ─────────────────────────── */\n"
        "  function render(now) {\n"
        "    const t = now * 0.001; /* segundos */\n"
        "    drawCalls = 0;\n\n"
        "    /* Resize canvas a DPR del Honor 400 */\n"
        "    const dpr = Math.min(window.devicePixelRatio || 1, 2.0);\n"
        "    const W = canvas.clientWidth  * dpr | 0;\n"
        "    const H = canvas.clientHeight * dpr | 0;\n"
        "    if (canvas.width !== W || canvas.height !== H) {\n"
        "      canvas.width = W; canvas.height = H;\n"
        "      gl.viewport(0, 0, W, H);\n"
        "    }\n\n"
        "    /* Limpiar buffer */\n"
        "    gl.clearColor(0.012, 0.008, 0.020, 1.0);\n"
        "    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);\n"
        "    gl.enable(gl.DEPTH_TEST);\n"
        "    gl.enable(gl.CULL_FACE);\n"
        "    gl.cullFace(gl.BACK);\n\n"
        "    /* Cámara */\n"
        "    const sp=Math.sin(camPhi),cp=Math.cos(camPhi);\n"
        "    const eye=[\n"
        "      camRadius*sp*Math.cos(camTheta),\n"
        "      camRadius*cp,\n"
        "      camRadius*sp*Math.sin(camTheta)\n"
        "    ];\n"
        "    const proj=Mat4.perspective(Math.PI/(RG_PHI+1), W/H, 0.1, 500.0);\n"
        "    const view=Mat4.lookAt(eye,[0,0,0],[0,1,0]);\n"
        "    const vpMat=Mat4.multiply(proj,view);\n\n"
        "    /* Uniforms globales */\n"
        "    gl.useProgram(prog);\n"
        "    gl.uniform3fv(U.camPos,    eye);\n"
        "    gl.uniform3fv(U.lightDir,  LIGHT_DIR);\n"
        "    gl.uniform3fv(U.lightCol,  LIGHT_COL);\n"
        "    gl.uniform1f(U.time,       t);\n"
        "    gl.uniform1f(U.roughness,  ROUGHNESS);\n"
        "    gl.uniform1f(U.metallic,   METALLIC);\n"
        "    gl.uniform1f(U.scale,      SCALE);\n\n"
        "    /* Animación orbital de los nodos — φ-eased */\n"
        "    nodeSphere1.pos[0]=Math.sin(t*0.7)*3.5;\n"
        "    nodeSphere1.pos[2]=Math.cos(t*0.7)*3.5;\n"
        "    nodeSphere2.pos[0]=Math.sin(t*0.7+Math.PI)*3.5;\n"
        "    nodeSphere2.pos[2]=Math.cos(t*0.7+Math.PI)*3.5;\n"
        "    /* Toro gira lento en Y */\n"
        "    const rotTorus=Quat.fromAxisAngle([0,1,0],t*0.4);\n"
        "    nodeTorus.rot=rotTorus;\n"
        "    /* Lemniscata gira en X — efecto φ-precesión */\n"
        "    nodeLemni.rot=Quat.multiply(\n"
        "      Quat.fromAxisAngle([0,1,0],t*0.3),\n"
        "      Quat.fromAxisAngle([1,0,0],Math.sin(t*0.2)*0.3)\n"
        "    );\n\n"
        "    /* Update transforms */\n"
        "    updateNode(root, null);\n\n"
        "    /* Draw calls */\n"
        "    drawNode(root, vpMat, eye);\n\n"
        "    /* Stats — cada 60 frames */\n"
        "    frames++;\n"
        "    if (frames % 60 === 0) {\n"
        "      const dt = now - t0; t0 = now;\n"
        "      fps = (60000 / dt) | 0;\n"
        "      statsEl.textContent =\n"
        "        `RIgArt v3.0 · §35\\n` +\n"
        "        `FPS  ${fps.toString().padStart(4)}  |  ` +\n"
        "        `DC  ${drawCalls}  |  ` +\n"
        "        `φ  ${RG_PHI.toFixed(6)}\\n` +\n"
        "        `${W}×${H}px · Adreno 720 · WebGL2`;\n"
        "    }\n\n"
        "    requestAnimationFrame(render);\n"
        "  }\n\n"
        "  requestAnimationFrame(render);\n"
        "})();\n"
        "</script>\n"
        "</body>\n"
        "</html>\n");

    rdset_html__rig_variant_0ce90258(out, &html);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

void rigart_3d_dispatch__rig_variant_6b730631(WsServer *srv, const char *cmd, const char *payload)
{
    if (!cmd) return 0;
    RIgArtResultV3 out = {0};
    int ret = -1;

    if (rl_strcmp(cmd, "rigart_scene") == 0) {
        RIgArtSceneCtx ctx = {0};
        ret = rigart_scene_js(&ctx, &out);
    }
    else if (rl_strcmp(cmd, "rigart_renderer") == 0) {
        RIgArtRendererCtx ctx = {0};

        if (payload) {
            const char *p;
            p = rl_strstr(payload,"\"rough\""); if(p){p=rl_strchr(p,':');if(p) ctx.roughness=(float)rl_strtod(p+1);}
            p = rl_strstr(payload,"\"metal\""); if(p){p=rl_strchr(p,':');if(p) ctx.metallic=(float)rl_strtod(p+1);}
            p = rl_strstr(payload,"\"scale\""); if(p){p=rl_strchr(p,':');if(p) ctx.tex_scale=(float)rl_strtod(p+1);}
        }
        ret = rigart_renderer_html(&ctx, &out);
    }

    if (ret == 0 && out.ok) {
        char header[256];
        rl_snprintf(header, sizeof(header),
            "{\"ok\":true,\"cmd\":\"%s\",\"certeza\":%.4f,\"phi\":%.6f,\"data\":\"",
            cmd, out.certeza, out.phi_ratio);
        ws_broadcastf(srv, "%s", header);
        if (out.js)   ws_broadcastf(srv, "%s", out.js);
        if (out.html) ws_broadcastf(srv, "%s", out.html);
        ws_broadcastf(srv, "%s", "\"}");
    } else {
        char err[256];
        rl_snprintf(err, sizeof(err),
            "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"3d dispatch error\"}", cmd);
        ws_broadcastf(srv, "%s", err);
    }
    rigart_free_result_v3(&out);
    return 0;
}

const char *rigart_renderer_name__rig_dup_1c456de8(void)
{
    return "rigart-3d-sovereign-renderer";
}

void rigart_face_session_init__rig_variant_aba42d8e(RIgArtFaceSession *s)
{
    if (!s) return 0;
    rl_memset(s, 0, sizeof(*s));
    s->subdiv_level = 4;
    s->params       = rig_face_default_params();
    return 0;
}

void rigart_face_session_destroy__rig_variant_bf093c26(RIgArtFaceSession *s)
{
    if (!s) return 0;
    if (s->mesh) { rig_face_destroy(s->mesh); s->mesh = NULL; }
    return 0;
}

static float j36_float__rig_variant_4b001364(const char *json, const char *key, float def)
{
    if (!json) return def;
    const char *p = strstr(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    return (float)atof(p + 1);
}
static int j36_int__rig_variant_ff907571(const char *json, const char *key, int def)
{
    if (!json) return def;
    const char *p = strstr(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    return atoi(p + 1);
}
static bool j36_bool__rig_variant_57317ea1(const char *json, const char *key, bool def)
{
    if (!json) return def;
    const char *p = strstr(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    while (*p == ':' || *p == ' ') p++;
    if (*p == 't' || *p == '1') return true;
    if (*p == 'f' || *p == '0') return false;
    return def;
}
void rigart_face_dispatch__rig_variant_b4a4ff00(WsServer *srv, const char *cmd,
                           const char *payload,
                           RIgArtFaceSession *session)
{
    if (!cmd || !session) return 0;
    char resp[2048];

    if (rl_strcmp(cmd, "rigart_face_build") == 0)
    {

        session->params.melanin              = j36_float(payload, "\"melanin\"",    0.35f);
        session->params.hemoglobin           = j36_float(payload, "\"hemoglobin\"", 0.40f);
        session->params.age_factor           = j36_float(payload, "\"age\"",        0.25f);
        session->params.gender_factor        = j36_float(payload, "\"gender\"",     0.50f);
        session->params.cranium_width        = j36_float(payload, "\"cw\"",         session->params.cranium_width);
        session->params.cranium_height       = j36_float(payload, "\"ch\"",         session->params.cranium_height);
        session->params.cranium_depth        = j36_float(payload, "\"cd\"",         session->params.cranium_depth);
        session->params.jaw_width            = j36_float(payload, "\"jaw_w\"",      session->params.jaw_width);
        session->params.jaw_angle            = j36_float(payload, "\"jaw_a\"",      session->params.jaw_angle);
        session->params.nose_length          = j36_float(payload, "\"nose_l\"",     session->params.nose_length);
        session->params.nose_width           = j36_float(payload, "\"nose_w\"",     session->params.nose_width);
        session->params.interocular_dist     = j36_float(payload, "\"iod\"",        session->params.interocular_dist);
        session->params.zygomatic_width      = j36_float(payload, "\"zygo_w\"",     session->params.zygomatic_width);
        session->params.mouth_width          = j36_float(payload, "\"mouth_w\"",    session->params.mouth_width);
        session->params.neck_width           = j36_float(payload, "\"neck_w\"",     session->params.neck_width);
        session->params.neck_length          = j36_float(payload, "\"neck_l\"",     session->params.neck_length);
        session->params.chin_projection      = j36_float(payload, "\"chin_p\"",     session->params.chin_projection);

        uint32_t subdiv = (uint32_t)j36_int(payload, "\"subdiv\"", 4);
        if (subdiv < 2) subdiv = 2;
        if (subdiv > 5) subdiv = 5;
        session->subdiv_level = subdiv;

        bool build_neck     = j36_bool(payload, "\"neck\"",     true);
        bool build_skeleton = j36_bool(payload, "\"skeleton\"", true);
        bool build_shapes   = j36_bool(payload, "\"shapes\"",   true);

        if (session->mesh) { rig_face_destroy(session->mesh); session->mesh = NULL; }
        session->mesh = rig_face_create(&session->params, subdiv);
        session->has_skeleton = false;
        session->has_shapes   = false;

        if (!session->mesh) {
            rl_snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_build\","
                "\"error\":\"arena overflow — reducir subdiv\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        if (build_neck)     rig_face_build_neck(session->mesh, &session->params);
        if (build_skeleton) { rig_face_bind_skeleton(session->mesh); session->has_skeleton = true; }
        if (build_shapes)   { rig_face_build_facs_shapes(session->mesh); session->has_shapes = true; }

        if (build_neck)     rig_face_compute_smooth_normals(session->mesh);

        rl_snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_build\","
            "\"verts\":%u,\"tris\":%u,\"subdiv\":%u,"
            "\"phi_error\":%.6f,\"skeleton\":%s,\"shapes\":%s,"
            "\"certeza\":%.6f}",
            session->mesh->n_verts, session->mesh->n_tris,
            session->mesh->subdivision_level,
            session->mesh->phi_error,
            session->has_skeleton ? "true" : "false",
            session->has_shapes   ? "true" : "false",
            (double)RIG_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (rl_strcmp(cmd, "rigart_face_expression") == 0)
    {
        if (!session->mesh || !session->has_shapes) {
            rl_snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_expression\","
                "\"error\":\"no mesh or shapes — build first\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        float weights[RIG_FACE_BLEND_SHAPES];
        rl_memset(weights, 0, sizeof(weights));
        const char *arr = payload ? rl_strstr(payload, "\"weights\"") : NULL;
        if (arr) {
            arr = rl_strchr(arr, '[');
            if (arr) {
                arr++;
                for (int i = 0; i < RIG_FACE_BLEND_SHAPES; i++) {
                    while (*arr == ' ' || *arr == ',') arr++;
                    if (*arr == ']' || *arr == '\0') break;
                    weights[i] = (float)rl_strtod(arr);
                    while (*arr && *arr != ',' && *arr != ']') arr++;
                }
            }
        }
        rig_face_apply_expression(session->mesh, weights);
        rig_face_solve_psd(session->mesh);
        rig_face_compute_smooth_normals(session->mesh);
        rig_face_compute_tangent_basis(session->mesh);

        rl_snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_expression\","
            "\"verts\":%u,\"certeza\":%.6f}",
            session->mesh->n_verts, (double)RIG_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (rl_strcmp(cmd, "rigart_face_vbo") == 0)
    {
        if (!session->mesh) {
            rl_snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_vbo\","
                "\"error\":\"no mesh — build first\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        RigFaceMesh *m = session->mesh;
        uint32_t n_floats  = m->n_verts * 15;
        uint32_t n_indices = m->n_tris  * 3;
        rl_snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_vbo\","
            "\"verts\":%u,\"tris\":%u,"
            "\"vbo_floats\":%u,\"vbo_bytes\":%u,"
            "\"ibo_indices\":%u,\"ibo_bytes\":%u,"
            "\"stride\":15,\"layout\":\"pos3_nrm3_tan3_uv2_col4\","
            "\"certeza\":%.6f}",
            m->n_verts, m->n_tris,
            n_floats,  n_floats  * 4,
            n_indices, n_indices * 4,
            (double)RIG_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (rl_strcmp(cmd, "rigart_face_glsl") == 0)
    {
        if (!session->mesh) {
            rl_snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_glsl\","
                "\"error\":\"no mesh — build first\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        char glsl_buf[2048];
        rig_face_export_glsl_uniforms(session->mesh, glsl_buf, sizeof(glsl_buf));

        rl_snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_glsl\","
            "\"melanin\":%.4f,\"roughness\":%.4f,"
            "\"sss_r\":%.4f,\"sss_g\":%.4f,\"sss_b\":%.4f,"
            "\"certeza\":%.6f}",
            session->mesh->material.melanin_concentration,
            session->mesh->material.roughness,
            session->mesh->material.sss_radius[0],
            session->mesh->material.sss_radius[1],
            session->mesh->material.sss_radius[2],
            (double)RIG_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        ws_broadcastf(srv, "%s", glsl_buf);
        return 0;
    }

    rl_snprintf(resp, sizeof(resp),
        "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"unknown face command\"}", cmd);
    ws_broadcastf(srv, "%s", resp);
    return 0;
}
