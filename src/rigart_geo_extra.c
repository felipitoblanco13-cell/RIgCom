/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define _POSIX_C_SOURCE 200809L

#include "../include/rigart_v3_3d.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_noext_types.h"

typedef struct { char *buf; size_t pos; size_t cap; } Buf3dX;

static int bdx_cat(Buf3dX *b, const char *s)
{
    if (!b->buf || !s) return 0;
    size_t n = rl_strlen(s);
    if (b->pos + n + 1 >= b->cap) return 0;
    rl_memcpy(b->buf + b->pos, s, n);
    b->pos += n;
    return 0;
}
int rigart_geo_extra_inject(rig_u8 *buf,
                              void (*cat)(void *, const char *))
{
    if (!buf || !cat) return 0;
    cat(buf,
        "/* ── geoDodecahedron — 20 vértices φ-exactos ─────────────── */\n"
        "function geoDodecahedron(size=1.0) {\n"
        "  /* Coordenadas del dodecaedro regular.\n"
        "     Grupo 1 (8 vértices): (±1, ±1, ±1)\n"
        "     Grupo 2 (4 vértices): (0, ±φ, ±1/φ)\n"
        "     Grupo 3 (4 vértices): (±1/φ, 0, ±φ)\n"
        "     Grupo 4 (4 vértices): (±φ, ±1/φ, 0)   */\n"
        "  const ph = RG_PHI, ip = 1/RG_PHI;\n"
        "  const s  = size / Math.sqrt(3);\n"
        "  const rawV = [\n"
        "    /* 0–7  cubo */\n"
        "    [-1,-1,-1], [ 1,-1,-1], [ 1, 1,-1], [-1, 1,-1],\n"
        "    [-1,-1, 1], [ 1,-1, 1], [ 1, 1, 1], [-1, 1, 1],\n"
        "    /* 8–11 rectángulo Y */\n"
        "    [0,-ph,-ip],[0, ph,-ip],[0, ph, ip],[0,-ph, ip],\n"
        "    /* 12–15 rectángulo Z */\n"
        "    [-ip,0,-ph],[ ip,0,-ph],[ ip,0, ph],[-ip,0, ph],\n"
        "    /* 16–19 rectángulo X */\n"
        "    [-ph,-ip,0],[ ph,-ip,0],[ ph, ip,0],[-ph, ip,0]\n"
        "  ].map(v=>v.map(c=>c*s));\n"
        "\n"
        "  /* 12 caras pentagonales — cada cara = 5 vértices\n"
        "     trianguladas como abanico desde vértice 0 de cada cara */\n"
        "  const faces = [\n"
        "    [0,8,11,4,16],[2,14,13,12,3],[5,17,18,6,14],\n"
        "    [7,15,14,6,10],[4,11,10,7,15],[0,16,19,7,3],\n"
        "    [1,17,5,11,8],[2,18,17,1,13],[9,10,6,18,2],\n"
        "    [3,12,9,2,19],[0,13,1,8,12],[9,19,7,15,10]\n"
        "  ];\n"
        "\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "\n"
        "  faces.forEach(face => {\n"
        "    const base = pos.length / 3;\n"
        "    /* Normal de la cara — producto cruzado de 2 aristas */\n"
        "    const v0=rawV[face[0]], v1=rawV[face[1]], v2=rawV[face[2]];\n"
        "    const e1=[v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]];\n"
        "    const e2=[v2[0]-v0[0],v2[1]-v0[1],v2[2]-v0[2]];\n"
        "    const nx=e1[1]*e2[2]-e1[2]*e2[1];\n"
        "    const ny=e1[2]*e2[0]-e1[0]*e2[2];\n"
        "    const nz=e1[0]*e2[1]-e1[1]*e2[0];\n"
        "    const nl=Math.sqrt(nx*nx+ny*ny+nz*nz)||1e-8;\n"
        "    const nn=[nx/nl,ny/nl,nz/nl];\n"
        "    /* Tangente aproximada = e1 normalizada */\n"
        "    const el=Math.sqrt(e1[0]*e1[0]+e1[1]*e1[1]+e1[2]*e1[2])||1e-8;\n"
        "    const tn=[e1[0]/el,e1[1]/el,e1[2]/el];\n"
        "\n"
        "    face.forEach((vi, i) => {\n"
        "      const v = rawV[vi];\n"
        "      pos.push(v[0],v[1],v[2]);\n"
        "      nrm.push(...nn);\n"
        "      /* UV esférico */\n"
        "      const vn = [v[0]/size,v[1]/size,v[2]/size];\n"
        "      const uvU = 0.5 + Math.atan2(vn[2],vn[0]) / (2*Math.PI);\n"
        "      const uvV = 0.5 - Math.asin(Math.max(-1,Math.min(1,vn[1]))) / Math.PI;\n"
        "      uv.push(uvU, uvV);\n"
        "      tan.push(...tn, 1);\n"
        "    });\n"
        "    /* Triangulación en abanico: (0,1,2),(0,2,3),(0,3,4) */\n"
        "    for (let i=1; i<face.length-1; i++)\n"
        "      idx.push(base, base+i, base+i+1);\n"
        "  });\n"
        "\n"
        "  return { pos:new Float32Array(pos), nrm:new Float32Array(nrm),\n"
        "           uv:new Float32Array(uv),   tan:new Float32Array(tan),\n"
        "           idx:new Uint16Array(idx) };\n"
        "}\n\n");

    cat(buf,
        "/* ── geoIcosphere — subdivisión icosaédrica φ-basada ──────── */\n"
        "function geoIcosphere(radius=1.0, level=2) {\n"
        "  /* Icosaedro base — 12 vértices usando φ */\n"
        "  const ph = RG_PHI;\n"
        "  const norm1 = 1 / Math.sqrt(1 + ph*ph);\n"
        "  const a = norm1, b = ph * norm1;\n"
        "  let verts = [\n"
        "    [-a, b, 0],[a, b, 0],[-a,-b, 0],[a,-b, 0],\n"
        "    [ 0,-a, b],[0, a, b],[ 0,-a,-b],[0, a,-b],\n"
        "    [ b, 0,-a],[b, 0, a],[-b, 0,-a],[-b, 0, a]\n"
        "  ].map(v => { const l=Math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);\n"
        "    return [v[0]/l, v[1]/l, v[2]/l]; });\n"
        "\n"
        "  let tris = [\n"
        "    [0,11,5],[0,5,1],[0,1,7],[0,7,10],[0,10,11],\n"
        "    [1,5,9],[5,11,4],[11,10,2],[10,7,6],[7,1,8],\n"
        "    [3,9,4],[3,4,2],[3,2,6],[3,6,8],[3,8,9],\n"
        "    [4,9,5],[2,4,11],[6,2,10],[8,6,7],[9,8,1]\n"
        "  ];\n"
        "\n"
        "  /* Subdivisión — midpoint cache para evitar duplicados */\n"
        "  function midpoint(ia, ib, cache) {\n"
        "    const key = ia < ib ? `${ia}_${ib}` : `${ib}_${ia}`;\n"
        "    if (cache[key] !== undefined) return cache[key];\n"
        "    const va = verts[ia], vb = verts[ib];\n"
        "    const mx = (va[0]+vb[0])*0.5, my = (va[1]+vb[1])*0.5,\n"
        "          mz = (va[2]+vb[2])*0.5;\n"
        "    const ml = Math.sqrt(mx*mx+my*my+mz*mz) || 1e-8;\n"
        "    verts.push([mx/ml, my/ml, mz/ml]);\n"
        "    return (cache[key] = verts.length - 1);\n"
        "  }\n"
        "\n"
        "  for (let l = 0; l < level; l++) {\n"
        "    const cache = {};\n"
        "    const next  = [];\n"
        "    for (const [a,b,c] of tris) {\n"
        "      const ab = midpoint(a,b,cache);\n"
        "      const bc = midpoint(b,c,cache);\n"
        "      const ca = midpoint(c,a,cache);\n"
        "      next.push([a,ab,ca],[b,bc,ab],[c,ca,bc],[ab,bc,ca]);\n"
        "    }\n"
        "    tris = next;\n"
        "  }\n"
        "\n"
        "  /* Construir buffers */\n"
        "  const pos=[],nrm=[],uvs=[],tan=[],idx=[];\n"
        "  verts.forEach(v => {\n"
        "    pos.push(v[0]*radius, v[1]*radius, v[2]*radius);\n"
        "    nrm.push(v[0], v[1], v[2]);\n"
        "    /* UV esférico */\n"
        "    uvs.push(\n"
        "      0.5 + Math.atan2(v[2], v[0]) / (2*Math.PI),\n"
        "      0.5 - Math.asin(Math.max(-1,Math.min(1,v[1]))) / Math.PI\n"
        "    );\n"
        "    /* Tangente: perpendicular a normal en plano XZ */\n"
        "    const tx = -v[2], tz = v[0];\n"
        "    const tl = Math.sqrt(tx*tx+tz*tz)||1e-8;\n"
        "    tan.push(tx/tl, 0, tz/tl, 1);\n"
        "  });\n"
        "  tris.forEach(([a,b,c]) => idx.push(a,b,c));\n"
        "\n"
        "  return { pos:new Float32Array(pos), nrm:new Float32Array(nrm),\n"
        "           uv:new Float32Array(uvs),  tan:new Float32Array(tan),\n"
        "           idx:new Uint16Array(idx) };\n"
        "}\n\n");

    cat(buf,
        "/* ── geoCylinder — tubo con tapas y tangentes correctas ──── */\n"
        "function geoCylinder(radiusTop=0.5, radiusBot=0.5,\n"
        "                     height=2.0, segs=32, rings=1,\n"
        "                     openEnded=false) {\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "  const hh = height * 0.5;\n"
        "\n"
        "  /* ── Lateral ── */\n"
        "  const slope = (radiusBot - radiusTop) / height; /* para normal */\n"
        "  const nLen  = Math.sqrt(1 + slope*slope) || 1e-8;\n"
        "  const nY    = -slope / nLen; /* componente Y de la normal lateral */\n"
        "  const nR    =  1.0   / nLen; /* componente radial */\n"
        "\n"
        "  for (let r=0; r<=rings; r++) {\n"
        "    const t   = r / rings;\n"
        "    const y   = hh - t * height;\n"
        "    const rad = radiusTop + (radiusBot - radiusTop) * t;\n"
        "    for (let s=0; s<=segs; s++) {\n"
        "      const th  = RG_TAU * s / segs;\n"
        "      const ct  = Math.cos(th), st = Math.sin(th);\n"
        "      pos.push(rad*ct, y, rad*st);\n"
        "      nrm.push(nR*ct, nY, nR*st);\n"
        "      uv.push(s/segs, 1-t);\n"
        "      tan.push(-st, 0, ct, 1); /* tangente circunferencial */\n"
        "    }\n"
        "  }\n"
        "  for (let r=0; r<rings; r++) for (let s=0; s<segs; s++) {\n"
        "    const a = r*(segs+1)+s;\n"
        "    idx.push(a, a+segs+1, a+1,  a+1, a+segs+1, a+segs+2);\n"
        "  }\n"
        "\n"
        "  if (!openEnded) {\n"
        "    /* ── Tapas ── */\n"
        "    function addCap(radius, y, normalY) {\n"
        "      const base = pos.length / 3;\n"
        "      /* Centro de la tapa */\n"
        "      pos.push(0, y, 0); nrm.push(0, normalY, 0);\n"
        "      uv.push(0.5, 0.5); tan.push(1, 0, 0, 1);\n"
        "      /* Periferia */\n"
        "      for (let s=0; s<=segs; s++) {\n"
        "        const th = RG_TAU * s / segs;\n"
        "        const ct = Math.cos(th), st = Math.sin(th);\n"
        "        pos.push(radius*ct, y, radius*st);\n"
        "        nrm.push(0, normalY, 0);\n"
        "        uv.push(0.5 + ct*0.5, 0.5 + st*0.5);\n"
        "        tan.push(-st, 0, ct, 1);\n"
        "      }\n"
        "      /* Triángulos en abanico desde el centro */\n"
        "      for (let s=0; s<segs; s++) {\n"
        "        if (normalY > 0)\n"
        "          idx.push(base, base+1+s, base+2+s);\n"
        "        else\n"
        "          idx.push(base, base+2+s, base+1+s);\n"
        "      }\n"
        "    }\n"
        "    addCap(radiusTop, +hh, +1); /* tapa superior */\n"
        "    addCap(radiusBot, -hh, -1); /* tapa inferior */\n"
        "  }\n"
        "\n"
        "  return { pos:new Float32Array(pos), nrm:new Float32Array(nrm),\n"
        "           uv:new Float32Array(uv),   tan:new Float32Array(tan),\n"
        "           idx:new Uint16Array(idx) };\n"
        "}\n\n");

    cat(buf,
        "/* ── geoCone — cono con normal suave en apex φ-proporcional ─ */\n"
        "function geoCone(radius=1.0, height=RG_PHI*2, segs=32) {\n"
        "  /* Inclinación del lateral → normal radial suave */\n"
        "  const slope = radius / height;\n"
        "  const nLen  = Math.sqrt(1 + slope*slope) || 1e-8;\n"
        "  const nY    = slope  / nLen;\n"
        "  const nR    = 1.0    / nLen;\n"
        "  const hh    = height * 0.5;\n"
        "\n"
        "  const pos=[],nrm=[],uv=[],tan=[],idx=[];\n"
        "\n"
        "  /* Apex — vértice compartido, N copias (una por segmento) */\n"
        "  for (let s=0; s<=segs; s++) {\n"
        "    const th = RG_TAU * s / segs;\n"
        "    const ct = Math.cos(th), st = Math.sin(th);\n"
        "    pos.push(0, hh, 0);              /* apex */\n"
        "    nrm.push(nR*ct, nY, nR*st);      /* normal interpolada */\n"
        "    uv.push(s/segs, 1.0);\n"
        "    tan.push(-st, 0, ct, 1);\n"
        "  }\n"
        "  /* Base */\n"
        "  for (let s=0; s<=segs; s++) {\n"
        "    const th = RG_TAU * s / segs;\n"
        "    const ct = Math.cos(th), st = Math.sin(th);\n"
        "    pos.push(radius*ct, -hh, radius*st);\n"
        "    nrm.push(nR*ct, nY, nR*st);\n"
        "    uv.push(s/segs, 0.0);\n"
        "    tan.push(-st, 0, ct, 1);\n"
        "  }\n"
        "  /* Triángulos laterales */\n"
        "  for (let s=0; s<segs; s++)\n"
        "    idx.push(s, segs+1+s, segs+2+s);\n"
        "\n"
        "  /* Tapa base */\n"
        "  const baseCtr = pos.length / 3;\n"
        "  pos.push(0, -hh, 0); nrm.push(0,-1,0); uv.push(0.5,0.5); tan.push(1,0,0,1);\n"
        "  for (let s=0; s<=segs; s++) {\n"
        "    const th = RG_TAU * s / segs;\n"
        "    const ct = Math.cos(th), st = Math.sin(th);\n"
        "    pos.push(radius*ct, -hh, radius*st);\n"
        "    nrm.push(0,-1,0);\n"
        "    uv.push(0.5+ct*0.5, 0.5+st*0.5);\n"
        "    tan.push(-st,0,ct,1);\n"
        "  }\n"
        "  for (let s=0; s<segs; s++)\n"
        "    idx.push(baseCtr, baseCtr+2+s, baseCtr+1+s);\n"
        "\n"
        "  return { pos:new Float32Array(pos), nrm:new Float32Array(nrm),\n"
        "           uv:new Float32Array(uv),   tan:new Float32Array(tan),\n"
        "           idx:new Uint16Array(idx) };\n"
        "}\n\n");

    cat(buf,
        "/* ── RgGeo — extensión con los 4 tipos nuevos ──────────────── */\n"
        "if (typeof RgGeo !== 'undefined') {\n"
        "  RgGeo.dodecahedron = (size=1.0)           => geoDodecahedron(size);\n"
        "  RgGeo.icosphere    = (r=1.0, level=2)     => geoIcosphere(r, level);\n"
        "  RgGeo.cylinder     = (rT=0.5,rB=0.5,h=2)  => geoCylinder(rT,rB,h);\n"
        "  RgGeo.cone         = (r=1.0, h=RG_PHI*2)  => geoCone(r, h);\n"
        "}\n\n");
    return 0;
}

char *rigart_geo_extra_script(void)
{
    size_t cap = 64 * 1024;
    char  *buf = rl_calloc(1, cap);
    if (!buf) return NULL;

    typedef struct { char *b; size_t p; size_t c; } LocBuf;
    static LocBuf lb;
    lb.b = buf; lb.p = 0; lb.c = cap;

    void cat_fn(void *b, const char *s) {
        LocBuf *lb2 = (LocBuf *)b;
        size_t n = rl_strlen(s);
        if (lb2->p + n + 1 >= lb2->c) return 0;
        rl_memcpy(lb2->b + lb2->p, s, n);
        lb2->p += n;
    }

    const char *hdr = "<script>\n/* §34 RIgArt — Geometrías Soberanas Extra */\n"
                      "'use strict';\n";
    cat_fn(&lb, hdr);

    rigart_geo_extra_inject(&lb, cat_fn);

    cat_fn(&lb, "</script>\n");
    buf[lb.p] = '\0';
    return buf;
}
