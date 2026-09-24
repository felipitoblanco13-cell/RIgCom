


#include "../include/rig_identity.h"
#include "../include/rig_vault.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>


static void mem_zero(void *p, size_t n) { volatile uint8_t *v = p; while(n--) *v++=0; }


static const uint32_t K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,
    0x923f82a4u,0xab1c5ed5u,0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
    0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,0xe49b69c1u,0xefbe4786u,
    0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,
    0x06ca6351u,0x14292967u,0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
    0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,0xa2bfe8a1u,0xa81a664bu,
    0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,
    0x5b9cca4fu,0x682e6ff3u,0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
    0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u,
};
typedef struct { uint32_t s[8]; uint64_t n; uint8_t b[64]; } SHA256Ctx;
#define RR32(x,n) (((x)>>(n))|((x)<<(32u-(n))))
#define SE0(x)    (RR32(x,2)^RR32(x,13)^RR32(x,22))
#define SE1(x)    (RR32(x,6)^RR32(x,11)^RR32(x,25))
#define SG0(x)    (RR32(x,7)^RR32(x,18)^((x)>>3u))
#define SG1(x)    (RR32(x,17)^RR32(x,19)^((x)>>10u))
#define SCH(x,y,z) (((x)&(y))|(~(x)&(z)))
#define SMA(x,y,z) (((x)&(y))|((x)&(z))|((y)&(z)))
static void sha256_compress(SHA256Ctx *c) {
    uint32_t W[64],a,b,cc,d,e,f,g,h,t1,t2;
    for(int i=0;i<16;i++){const uint8_t *p=c->b+i*4;
        W[i]=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3];}
    for(int i=16;i<64;i++) W[i]=SG1(W[i-2])+W[i-7]+SG0(W[i-15])+W[i-16];
    a=c->s[0];b=c->s[1];cc=c->s[2];d=c->s[3];e=c->s[4];f=c->s[5];g=c->s[6];h=c->s[7];
    for(int i=0;i<64;i++){t1=h+SE1(e)+SCH(e,f,g)+K256[i]+W[i];t2=SE0(a)+SMA(a,b,cc);
        h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;}
    c->s[0]+=a;c->s[1]+=b;c->s[2]+=cc;c->s[3]+=d;
    c->s[4]+=e;c->s[5]+=f;c->s[6]+=g;c->s[7]+=h;
}
static void sha256_init(SHA256Ctx *c){
    c->s[0]=0x6a09e667u;c->s[1]=0xbb67ae85u;c->s[2]=0x3c6ef372u;c->s[3]=0xa54ff53au;
    c->s[4]=0x510e527fu;c->s[5]=0x9b05688cu;c->s[6]=0x1f83d9abu;c->s[7]=0x5be0cd19u;
    c->n=0;memset(c->b,0,64);}
static void sha256_update(SHA256Ctx *c,const uint8_t *data,size_t len){
    uint32_t off=(uint32_t)(c->n&63u);c->n+=len;
    while(len--){c->b[off++]=*data++;if(off==64){sha256_compress(c);off=0;}}}
static void sha256_final(SHA256Ctx *c,uint8_t d[32]){
    uint32_t off=(uint32_t)(c->n&63u);c->b[off++]=0x80;
    if(off>56){while(off<64)c->b[off++]=0;sha256_compress(c);off=0;}
    while(off<56)c->b[off++]=0;
    uint64_t bits=c->n*8u;
    for(int i=0;i<8;i++)c->b[56+i]=(uint8_t)(bits>>((7-i)*8));
    sha256_compress(c);
    for(int i=0;i<8;i++){d[i*4+0]=(uint8_t)(c->s[i]>>24);d[i*4+1]=(uint8_t)(c->s[i]>>16);
        d[i*4+2]=(uint8_t)(c->s[i]>>8);d[i*4+3]=(uint8_t)(c->s[i]);}}
static void sha256_hash(const uint8_t *d,size_t n,uint8_t out[32]){
    SHA256Ctx c;sha256_init(&c);sha256_update(&c,d,n);sha256_final(&c,out);}


static const uint64_t K512[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL,
};
typedef struct { uint64_t s[8]; uint64_t n[2]; uint8_t b[128]; } SHA512Ctx;
#define RR64(x,n) (((x)>>(n))|((x)<<(64u-(n))))
#define TE0(x)    (RR64(x,28)^RR64(x,34)^RR64(x,39))
#define TE1(x)    (RR64(x,14)^RR64(x,18)^RR64(x,41))
#define TG0(x)    (RR64(x,1) ^RR64(x,8) ^((x)>>7))
#define TG1(x)    (RR64(x,19)^RR64(x,61)^((x)>>6))
#define TCH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define TMA(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
static void sha512_compress(SHA512Ctx *c) {
    uint64_t W[80],a,b,cc,d,e,f,g,h,t1,t2;
    for(int i=0;i<16;i++){const uint8_t *p=c->b+i*8;
        W[i]=((uint64_t)p[0]<<56)|((uint64_t)p[1]<<48)|((uint64_t)p[2]<<40)|((uint64_t)p[3]<<32)
            |((uint64_t)p[4]<<24)|((uint64_t)p[5]<<16)|((uint64_t)p[6]<<8)|(uint64_t)p[7];}
    for(int i=16;i<80;i++) W[i]=TG1(W[i-2])+W[i-7]+TG0(W[i-15])+W[i-16];
    a=c->s[0];b=c->s[1];cc=c->s[2];d=c->s[3];e=c->s[4];f=c->s[5];g=c->s[6];h=c->s[7];
    for(int i=0;i<80;i++){t1=h+TE1(e)+TCH(e,f,g)+K512[i]+W[i];t2=TE0(a)+TMA(a,b,cc);
        h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;}
    c->s[0]+=a;c->s[1]+=b;c->s[2]+=cc;c->s[3]+=d;
    c->s[4]+=e;c->s[5]+=f;c->s[6]+=g;c->s[7]+=h;}
static void sha512_init(SHA512Ctx *c){
    c->s[0]=0x6a09e667f3bcc908ULL;c->s[1]=0xbb67ae8584caa73bULL;
    c->s[2]=0x3c6ef372fe94f82bULL;c->s[3]=0xa54ff53a5f1d36f1ULL;
    c->s[4]=0x510e527fade682d1ULL;c->s[5]=0x9b05688c2b3e6c1fULL;
    c->s[6]=0x1f83d9abfb41bd6bULL;c->s[7]=0x5be0cd19137e2179ULL;
    c->n[0]=c->n[1]=0;memset(c->b,0,128);}
static void sha512_update(SHA512Ctx *c,const uint8_t *data,size_t len){
    uint64_t off=c->n[0]&127ULL;
    uint64_t prev=c->n[0]; c->n[0]+=len; if(c->n[0]<prev) c->n[1]++;
    while(len--){c->b[off++]=*data++;if(off==128){sha512_compress(c);off=0;}}}
static void sha512_final(SHA512Ctx *c,uint8_t d[64]){
    uint64_t off=c->n[0]&127ULL; c->b[off++]=0x80;
    if(off>112){while(off<128)c->b[off++]=0;sha512_compress(c);off=0;}
    while(off<112)c->b[off++]=0;
    
    uint64_t hi=(c->n[1]<<3)|(c->n[0]>>61), lo=c->n[0]<<3;
    for(int i=0;i<8;i++) c->b[112+i]=(uint8_t)(hi>>((7-i)*8));
    for(int i=0;i<8;i++) c->b[120+i]=(uint8_t)(lo>>((7-i)*8));
    sha512_compress(c);
    for(int i=0;i<8;i++){d[i*8+0]=(uint8_t)(c->s[i]>>56);d[i*8+1]=(uint8_t)(c->s[i]>>48);
        d[i*8+2]=(uint8_t)(c->s[i]>>40);d[i*8+3]=(uint8_t)(c->s[i]>>32);
        d[i*8+4]=(uint8_t)(c->s[i]>>24);d[i*8+5]=(uint8_t)(c->s[i]>>16);
        d[i*8+6]=(uint8_t)(c->s[i]>>8); d[i*8+7]=(uint8_t)(c->s[i]);}}
static void sha512_hash(const uint8_t *d,size_t n,uint8_t out[64]){
    SHA512Ctx c;sha512_init(&c);sha512_update(&c,d,n);sha512_final(&c,out);}


typedef long long gf[16];

static const gf gf0  = {0};
static const gf gf1  = {1};

static const gf D    = {0x78a3,0x1359,0x4dca,0x75eb,0xd8ab,0x4141,0x0a4d,0x0070,
                         0xe898,0x7779,0x4079,0x8cc7,0xfe73,0x2b6f,0x6cee,0x5203};
static const gf D2   = {0xf159,0x26b2,0x9b94,0xebd6,0xb156,0x8283,0x149a,0x00e0,
                         0xd130,0xeef3,0x80f2,0x198e,0xfce7,0x56df,0xd9dc,0x2406};

static const gf I    = {0xa0b0,0x4a0e,0x1b27,0xc4ee,0xe478,0xad2f,0x1806,0x2f43,
                         0xd7a7,0x3dfb,0x0099,0x2b4d,0xdf0b,0x4fc1,0x2480,0x2b83};

static const gf X25  = {0xd51a,0x8f25,0x2d60,0xc956,0xa7b2,0x9525,0xc760,0x692c,
                          0xdc5c,0xfdd6,0xe231,0xc0a4,0x53fe,0xcd6e,0x36d3,0x2169};
static const gf Y25  = {0x6658,0x6666,0x6666,0x6666,0x6666,0x6666,0x6666,0x6666,
                          0x6666,0x6666,0x6666,0x6666,0x6666,0x6666,0x6666,0x6666};

static const uint8_t L[32] = {
    0xed,0xd3,0xf5,0x5c,0x1a,0x63,0x12,0x58,
    0xd6,0x9c,0xf7,0xa2,0xde,0xf9,0xde,0x14,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10
};

#define A(o,a,b) for(int _i=0;_i<16;_i++) (o)[_i]=(a)[_i]+(b)[_i]
#define Z(o,a,b) for(int _i=0;_i<16;_i++) (o)[_i]=(a)[_i]-(b)[_i]
#define C(o,a)   for(int _i=0;_i<16;_i++) (o)[_i]=(a)[_i]

static void car25519(gf o) {
    long long c;
    for(int i=0;i<16;i++){
        o[(i+1)%16]+=(i==15?38:1)*((c=(o[i]+128)>>16));
        o[i]-=c<<16;
    }
}
static void sel25519(gf p,gf q,int b){
    long long t,c=~(b-1);
    for(int i=0;i<16;i++){t=c&(p[i]^q[i]);p[i]^=t;q[i]^=t;}
}
static void pack25519(uint8_t *o,const gf n){
    int i,j,b; gf m,t; C(t,n);
    car25519(t); car25519(t); car25519(t);
    for(j=0;j<2;j++){
        m[0]=t[0]-0xffed;
        for(i=1;i<15;i++){m[i]=t[i]-0xffff-((m[i-1]>>16)&1);m[i-1]&=0xffff;}
        m[15]=t[15]-0x7fff-((m[14]>>16)&1); b=(m[15]>>16)&1; m[14]&=0xffff;
        sel25519(t,m,1-b);
    }
    for(i=0;i<16;i++){o[2*i]=(uint8_t)t[i];o[2*i+1]=(uint8_t)(t[i]>>8);}
}
static int neq25519(const gf a,const gf b){
    uint8_t c[32],d[32]; pack25519(c,a); pack25519(d,b);
    int r=0; for(int i=0;i<32;i++) r|=c[i]^d[i];
    return (1&((r-1)>>8))-1; 
}
static uint8_t par25519(const gf a){uint8_t d[32];pack25519(d,a);return d[0]&1;}
static void unpack25519(gf o,const uint8_t *n){
    for(int i=0;i<16;i++) o[i]=n[2*i]+((long long)n[2*i+1]<<8);
    o[15]&=0x7fff;
}
static void M(gf o,const gf a,const gf b){
    long long t[31]; memset(t,0,sizeof(t));
    for(int i=0;i<16;i++) for(int j=0;j<16;j++) t[i+j]+=a[i]*b[j];
    for(int i=0;i<15;i++) t[i]+=38*t[i+16];
    C(o,t); car25519(o); car25519(o);
}
static void S(gf o,const gf a){ M(o,a,a); }
static void inv25519(gf o,const gf i){
    gf c; C(c,i);
    for(int a=253;a>=0;a--){S(c,c);if(a!=2&&a!=4)M(c,c,i);}
    C(o,c);
}
static void pow2523(gf o,const gf i){
    gf c; C(c,i);
    for(int a=250;a>=0;a--){S(c,c);if(a!=1)M(c,c,i);}
    C(o,c);
}


static void scalarmult_x25519(uint8_t *q, const uint8_t *n, const uint8_t *p) {
    uint8_t z[32];
    gf x, a, b, c, d, e, f;
    
    gf _a24 = {0xdb41, 1};

    memcpy(z, n, 32);
    z[31] = (z[31] & 127) | 64;
    z[0] &= 248;

    unpack25519(x, p);
    C(a, gf1); C(b, gf0); C(c, x); C(d, gf1);

    for (int i = 254; i >= 0; i--) {
        int r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r); sel25519(c, d, r);
        A(e, a, c); Z(a, a, c);
        A(c, b, d); Z(b, b, d);
        S(d, e); S(f, a);
        M(a, c, a); M(c, b, e);
        A(e, a, c); Z(a, a, c);
        S(b, a); Z(c, d, f);
        M(a, c, _a24); A(a, a, d);
        M(c, c, a); M(a, d, f);
        M(d, b, x); S(b, e);
        sel25519(a, b, r); sel25519(c, d, r);
    }

    inv25519(c, c);
    M(a, a, c);
    pack25519(q, a);
}
static const uint8_t _x25519_base[32] = {9};
static void scalarmult_x25519_base(uint8_t *q,const uint8_t *n){
    scalarmult_x25519(q,n,_x25519_base);
}


typedef gf ge[4];  

static void ge_0(ge p){ C(p[0],gf0);C(p[1],gf1);C(p[2],gf1);C(p[3],gf0); }

static void ge_add(ge p,const ge q){
    gf a,b,c,d,e,f,g,h,t;
    Z(a,p[1],p[0]); Z(t,q[1],q[0]); M(a,a,t);
    A(b,p[0],p[1]); A(t,q[0],q[1]); M(b,b,t);
    M(c,p[3],q[3]); M(c,c,D2);
    M(d,p[2],q[2]); A(d,d,d);
    Z(e,b,a); Z(f,d,c); A(g,d,c); A(h,b,a);
    M(p[0],e,f); M(p[1],h,g); M(p[2],g,f); M(p[3],e,h);
}

static void ge_scalarmult(ge p,const ge q,const uint8_t *s){
    ge_0(p);
    ge r; C(r[0],q[0]);C(r[1],q[1]);C(r[2],q[2]);C(r[3],q[3]);
    for(int i=255;i>=0;--i){
        ge_add(p,p);  
        if((s[i>>3]>>(i&7))&1) ge_add(p,r);  
    }
    (void)r;
}

static void ge_pack(uint8_t *r,const ge p){
    gf tx,ty,zi; inv25519(zi,p[2]);
    M(tx,p[0],zi); M(ty,p[1],zi);
    pack25519(r,ty); r[31]^=par25519(tx)<<7;
}

static int ge_frombytes_negate(ge r,const uint8_t *s){
    gf u,v,v3,vxx,chk;
    unpack25519(r[1],s);
    C(r[2],gf1); M(u,r[1],r[1]); M(v,u,D); Z(u,u,r[2]); A(v,r[2],v);
    S(v3,v); M(v3,v3,v); M(r[0],v3,v3); M(r[0],r[0],u); M(r[0],r[0],v);
    pow2523(r[0],r[0]); M(r[0],r[0],u); M(r[0],r[0],v3);
    S(vxx,r[0]); M(vxx,vxx,v); Z(chk,vxx,u);
    if(neq25519(chk,gf0)!=0){ M(r[0],r[0],I); }
    S(vxx,r[0]); M(vxx,vxx,v); Z(chk,vxx,u);
    if(neq25519(chk,gf0)!=0) return -1;
    if(par25519(r[0])==(s[31]>>7)) for(int i=0;i<16;i++) r[0][i]=-r[0][i];
    M(r[3],r[0],r[1]); return 0;
}

static void ge_scalarmult_base(ge r,const uint8_t *s){
    
    ge B; C(B[0],X25); C(B[1],Y25); C(B[2],gf1); M(B[3],X25,Y25);
    ge_scalarmult(r,B,s);
}


static void sc_reduce(uint8_t *r,int64_t x[64]){
    int64_t carry; int i,j;
    for(i=63;i>=32;--i){
        carry=0;
        for(j=i-32;j<i-12;++j){
            x[j]+=carry-16*x[i]*(int64_t)L[j-(i-32)];
            carry=(x[j]+128)>>8; x[j]-=carry<<8;
        }
        x[j]+=carry; x[i]=0;
    }
    carry=0;
    for(j=0;j<32;++j){x[j]+=carry-(x[31]>>4)*(int64_t)L[j];carry=x[j]>>8;x[j]&=255;}
    for(j=0;j<32;++j) x[j]-=carry*(int64_t)L[j];
    for(i=0;i<32;++i){x[i+1]+=x[i]>>8;r[i]=(uint8_t)(x[i]&255);}
}

static void sc_muladd(uint8_t *s,const uint8_t *a,const uint8_t *b,const uint8_t *c){
    int64_t x[64]; memset(x,0,sizeof(x));
    for(int i=0;i<32;i++) for(int j=0;j<32;j++) x[i+j]+=(int64_t)a[i]*(int64_t)b[j];
    for(int i=0;i<32;i++) x[i]+=(int64_t)c[i];
    sc_reduce(s,x);
}


static void ed25519_keygen_seed(uint8_t pk[32],uint8_t sk[64],const uint8_t seed[32]){
    uint8_t h[64]; sha512_hash(seed,32,h);
    h[0]&=248; h[31]&=127; h[31]|=64;
    ge r; ge_scalarmult_base(r,h);
    ge_pack(pk,r);
    memcpy(sk,seed,32); memcpy(sk+32,pk,32);
    mem_zero(h,64);
}

static void ed25519_sign(uint8_t sig[64],const uint8_t *m,size_t mlen,
                          const uint8_t sk[64]){
    uint8_t az[64],nonce[64],hram[64];
    sha512_hash(sk,32,az);
    az[0]&=248; az[31]&=127; az[31]|=64;
    
    SHA512Ctx hc; sha512_init(&hc);
    sha512_update(&hc,az+32,32); sha512_update(&hc,m,mlen);
    sha512_final(&hc,nonce);
    
    int64_t x[64]; memset(x,0,sizeof(x));
    for(int i=0;i<64;i++) x[i]=(int64_t)(uint8_t)nonce[i];
    uint8_t nonce_r[32]; sc_reduce(nonce_r,x);
    ge R; ge_scalarmult_base(R,nonce_r);
    ge_pack(sig,R);
    
    sha512_init(&hc);
    sha512_update(&hc,sig,32); sha512_update(&hc,sk+32,32); sha512_update(&hc,m,mlen);
    sha512_final(&hc,hram);
    int64_t y[64]; memset(y,0,sizeof(y));
    for(int i=0;i<64;i++) y[i]=(int64_t)(uint8_t)hram[i];
    uint8_t k[32]; sc_reduce(k,y);
    
    sc_muladd(sig+32,k,az,nonce_r);
    mem_zero(az,64); mem_zero(nonce,64);
}

static int ed25519_verify(const uint8_t sig[64],const uint8_t *m,size_t mlen,
                           const uint8_t pk[32]){
    if(sig[63]&224) return -1;
    ge A, check; (void)0;
    if(ge_frombytes_negate(A,pk)!=0) return -1;
    uint8_t hram[64]; SHA512Ctx hc; sha512_init(&hc);
    sha512_update(&hc,sig,32); sha512_update(&hc,pk,32); sha512_update(&hc,m,mlen);
    sha512_final(&hc,hram);
    int64_t x[64]; memset(x,0,sizeof(x));
    for(int i=0;i<64;i++) x[i]=(int64_t)(uint8_t)hram[i];
    uint8_t k[32]; sc_reduce(k,x);
    
    
    ge Bp; ge_scalarmult_base(Bp,sig+32);
    ge_scalarmult(check,A,k); 
    ge_add(check,Bp);  
    uint8_t pcheck[32]; ge_pack(pcheck,check);
    
    int diff=0; for(int i=0;i<32;i++) diff|=pcheck[i]^sig[i];
    return diff?-1:0;
}


static void phone_normalize(const char *phone,char out[32]){
    int i=0; const char *p=phone;
    if(*p=='+'){out[i++]='+';p++;}
    while(*p&&i<31){if(isdigit((unsigned char)*p)){out[i++]=(char)*p;}p++;}
    out[i]='\0';
}


void rig_phone_hash(const char *phone,uint8_t out[RIG_PHONE_HASH_LEN]){
    char norm[32]; phone_normalize(phone,norm);
    sha256_hash((const uint8_t*)norm,strlen(norm),out);
}

void rig_hex32(const uint8_t *b,char out[65]){
    static const char hx[]="0123456789abcdef";
    for(int i=0;i<32;i++){out[i*2]=hx[b[i]>>4];out[i*2+1]=hx[b[i]&15];}
    out[64]='\0';
}

int rig_unhex32(const char *hex,uint8_t out[32]){
    if(!hex||strlen(hex)<64) return -1;
    for(int i=0;i<32;i++){
        char hi=hex[i*2],lo=hex[i*2+1];
        uint8_t h,l;
        if(hi>='0'&&hi<='9')h=hi-'0'; else if(hi>='a'&&hi<='f')h=hi-'a'+10;
        else if(hi>='A'&&hi<='F')h=hi-'A'+10; else return -1;
        if(lo>='0'&&lo<='9')l=lo-'0'; else if(lo>='a'&&lo<='f')l=lo-'a'+10;
        else if(lo>='A'&&lo<='F')l=lo-'A'+10; else return -1;
        out[i]=(h<<4)|l;
    }
    return 0;
}

RigIdStatus rig_identity_generate(const char *passphrase,const char *phone,
                                   RigIdentity *out){
    if(!passphrase||!phone||!out) return RIG_ID_ERR_PARAM;

    
    rig_phone_hash(phone,out->phone_hash);
    phone_normalize(phone,out->phone_norm);

    
    
    uint8_t seed[RIG_SEED_LEN];
    VaultStatus vs = vault_derive_key(passphrase,out->phone_hash,seed);
    if(vs!=VAULT_OK){ mem_zero(seed,32); return RIG_ID_ERR_CRYPTO; }

    
    ed25519_keygen_seed(out->ed_pk,out->ed_sk,seed);

    
    uint8_t dh_buf[33]; memcpy(dh_buf,seed,32); dh_buf[32]=0x01;
    sha256_hash(dh_buf,33,out->dh_sk);
    out->dh_sk[0]&=248; out->dh_sk[31]&=127; out->dh_sk[31]|=64;
    scalarmult_x25519_base(out->dh_pk,out->dh_sk);

    mem_zero(seed,32); mem_zero(dh_buf,33);
    return RIG_ID_OK;
}

RigIdStatus rig_sign(const RigIdentity *id,
                     const uint8_t *msg,size_t msg_len,
                     uint8_t sig[RIG_ED25519_SIG_LEN]){
    if(!id||!msg||!sig) return RIG_ID_ERR_PARAM;
    ed25519_sign(sig,msg,msg_len,id->ed_sk);
    return RIG_ID_OK;
}

RigIdStatus rig_verify(const uint8_t pk[RIG_ED25519_PK_LEN],
                       const uint8_t *msg,size_t msg_len,
                       const uint8_t sig[RIG_ED25519_SIG_LEN]){
    if(!pk||!msg||!sig) return RIG_ID_ERR_PARAM;
    return ed25519_verify(sig,msg,msg_len,pk)==0 ? RIG_ID_OK : RIG_ID_ERR_CRYPTO;
}

RigIdStatus rig_dh(const RigIdentity *id,
                   const uint8_t their_pk[RIG_X25519_PK_LEN],
                   uint8_t shared_out[32]){
    if(!id||!their_pk||!shared_out) return RIG_ID_ERR_PARAM;
    scalarmult_x25519(shared_out,id->dh_sk,their_pk);
    
    uint8_t zero[32]={0}; int bad=0;
    for(int i=0;i<32;i++) bad|=shared_out[i]^zero[i];
    return bad ? RIG_ID_OK : RIG_ID_ERR_CRYPTO;
}


static const uint8_t ID_MAGIC[8]={'R','I','G','I','D','E','N','T'};

RigIdStatus rig_identity_save(const RigIdentity *id,
                               const char *passphrase,
                               const char *path){
    if(!id||!passphrase||!path) return RIG_ID_ERR_PARAM;

    
    uint8_t secret[128]; memset(secret,0,sizeof(secret));
    memcpy(secret,      id->ed_sk,  64);
    memcpy(secret+64,   id->dh_sk,  32);
    memcpy(secret+96,   id->phone_norm, 31);

    uint8_t *blob=NULL; size_t blen=0;
    VaultStatus vs=vault_seal_pw(passphrase,secret,128,&blob,&blen);
    mem_zero(secret,128);
    if(vs!=VAULT_OK) return RIG_ID_ERR_CRYPTO;

    FILE *f=fopen(path,"wb");
    if(!f){vault_free(blob,blen);return RIG_ID_ERR_IO;}

    fwrite(ID_MAGIC,1,8,f);
    fwrite(id->phone_hash,1,32,f);
    fwrite(id->ed_pk,1,32,f);
    fwrite(id->dh_pk,1,32,f);
    uint8_t lenbuf[4]={(uint8_t)blen,(uint8_t)(blen>>8),(uint8_t)(blen>>16),(uint8_t)(blen>>24)};
    fwrite(lenbuf,1,4,f);
    fwrite(blob,1,blen,f);
    fclose(f);
    vault_free(blob,blen);
    return RIG_ID_OK;
}

RigIdStatus rig_identity_load(const char *passphrase,
                               const char *path,
                               RigIdentity *out){
    if(!passphrase||!path||!out) return RIG_ID_ERR_PARAM;

    FILE *f=fopen(path,"rb");
    if(!f) return RIG_ID_ERR_IO;

    uint8_t magic[8]; fread(magic,1,8,f);
    if(memcmp(magic,ID_MAGIC,8)!=0){fclose(f);return RIG_ID_ERR_FORMAT;}

    fread(out->phone_hash,1,32,f);
    fread(out->ed_pk,1,32,f);
    fread(out->dh_pk,1,32,f);

    uint8_t lenbuf[4]; fread(lenbuf,1,4,f);
    size_t blen=(size_t)lenbuf[0]|((size_t)lenbuf[1]<<8)
               |((size_t)lenbuf[2]<<16)|((size_t)lenbuf[3]<<24);
    if(blen>8192){fclose(f);return RIG_ID_ERR_FORMAT;}

    uint8_t *blob=(uint8_t*)malloc(blen);
    if(!blob){fclose(f);return RIG_ID_ERR_MEM;}
    fread(blob,1,blen,f); fclose(f);

    uint8_t *plain=NULL; size_t plen=0;
    VaultStatus vs=vault_open_pw(passphrase,blob,blen,&plain,&plen);
    free(blob);
    if(vs!=VAULT_OK) return RIG_ID_ERR_CRYPTO;
    if(plen<128){vault_free(plain,plen);return RIG_ID_ERR_FORMAT;}

    memcpy(out->ed_sk,  plain,    64);
    memcpy(out->dh_sk,  plain+64, 32);
    memcpy(out->phone_norm, (char*)plain+96, 31); out->phone_norm[31]='\0';
    vault_free(plain,plen);
    return RIG_ID_OK;
}


int rig_identity_main(int argc,char *argv[]){
    if(argc<2){
        fprintf(stderr,
            "Uso: rigcom identity <subcomando> [opciones]\n"
            "  keygen  <phone> [--out <path>]  — Genera identidad desde passphrase\n"
            "  show    <path>                  — Muestra claves públicas\n"
            "  sign    <path> <file>           — Firma archivo con Ed25519\n"
            "  verify  <pk_hex> <file> <sig>   — Verifica firma\n"
            "  dh      <path> <their_pk_hex>   — Calcula shared secret\n");
        return 1;
    }
    const char *sub=argv[1];

    if(strcmp(sub,"keygen")==0){
        if(argc<3){fprintf(stderr,"Falta número de teléfono\n");return 1;}
        const char *phone=argv[2];
        const char *outpath="~/.rig_identity";
        for(int i=3;i<argc;i++)
            if(strcmp(argv[i],"--out")==0&&i+1<argc){outpath=argv[++i];}
        
        char pw[256]={0};
        fprintf(stderr,"Passphrase: "); fflush(stderr);
        if(!fgets(pw,sizeof(pw),stdin)){fprintf(stderr,"Error\n");return 1;}
        pw[strcspn(pw,"\n")]='\0';
        RigIdentity id; memset(&id,0,sizeof(id));
        RigIdStatus rs=rig_identity_generate(pw,phone,&id);
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error generando identidad: %d\n",rs);return 1;}
        
        char path[512];
        if(outpath[0]=='~'){const char *h=getenv("HOME");
            snprintf(path,sizeof(path),"%s%s",h?h:".",outpath+1);}
        else snprintf(path,sizeof(path),"%s",outpath);
        rs=rig_identity_save(&id,pw,path);
        mem_zero(&id,sizeof(id)); mem_zero(pw,256);
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error guardando: %d\n",rs);return 1;}
        fprintf(stdout,"Identidad guardada en %s\n",path);
        return 0;

    } else if(strcmp(sub,"show")==0){
        if(argc<3){fprintf(stderr,"Falta ruta\n");return 1;}
        char pw[256]={0};
        fprintf(stderr,"Passphrase: "); fflush(stderr);
        if(!fgets(pw,sizeof(pw),stdin)){fprintf(stderr,"Error\n");return 1;}
        pw[strcspn(pw,"\n")]='\0';
        RigIdentity id; memset(&id,0,sizeof(id));
        RigIdStatus rs=rig_identity_load(pw,argv[2],&id);
        mem_zero(pw,256);
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error cargando: %d\n",rs);return 1;}
        char hex[65];
        rig_hex32(id.phone_hash,hex); printf("phone_hash : %s\n",hex);
        rig_hex32(id.ed_pk,hex);      printf("ed_pk      : %s\n",hex);
        rig_hex32(id.dh_pk,hex);      printf("dh_pk      : %s\n",hex);
        printf("phone_norm : %s\n",id.phone_norm);
        mem_zero(&id,sizeof(id));
        return 0;

    } else if(strcmp(sub,"sign")==0){
        if(argc<4){fprintf(stderr,"Uso: identity sign <identity_path> <file>\n");return 1;}
        char pw[256]={0};
        fprintf(stderr,"Passphrase: "); fflush(stderr);
        if(!fgets(pw,sizeof(pw),stdin)){fprintf(stderr,"Error\n");return 1;}
        pw[strcspn(pw,"\n")]='\0';
        RigIdentity id; memset(&id,0,sizeof(id));
        RigIdStatus rs=rig_identity_load(pw,argv[2],&id);
        mem_zero(pw,256);
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error cargando identidad: %d\n",rs);return 1;}
        
        FILE *f=fopen(argv[3],"rb");
        if(!f){fprintf(stderr,"No se puede abrir %s\n",argv[3]);return 1;}
        fseek(f,0,SEEK_END); long fsz=ftell(f); fseek(f,0,SEEK_SET);
        if(fsz<=0||fsz>1048576){fclose(f);fprintf(stderr,"Archivo demasiado grande\n");return 1;}
        uint8_t *msg=(uint8_t*)malloc((size_t)fsz);
        if(!msg){fclose(f);return 1;}
        fread(msg,1,(size_t)fsz,f); fclose(f);
        uint8_t sig[RIG_ED25519_SIG_LEN];
        rs=rig_sign(&id,msg,(size_t)fsz,sig);
        free(msg); mem_zero(&id,sizeof(id));
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error firmando: %d\n",rs);return 1;}
        char hex[129]; 
        static const char hx[]="0123456789abcdef";
        for(int i=0;i<64;i++){hex[i*2]=hx[sig[i]>>4];hex[i*2+1]=hx[sig[i]&15];}
        hex[128]='\0';
        printf("sig:%s\n",hex);
        return 0;

    } else if(strcmp(sub,"verify")==0){
        if(argc<5){fprintf(stderr,"Uso: identity verify <pk_hex> <file> <sig_hex>\n");return 1;}
        uint8_t pk[32];
        if(rig_unhex32(argv[2],pk)!=0){fprintf(stderr,"pk_hex inválido\n");return 1;}
        FILE *f=fopen(argv[3],"rb");
        if(!f){fprintf(stderr,"No se puede abrir %s\n",argv[3]);return 1;}
        fseek(f,0,SEEK_END); long fsz=ftell(f); fseek(f,0,SEEK_SET);
        uint8_t *msg=(uint8_t*)malloc((size_t)fsz);
        if(!msg){fclose(f);return 1;}
        fread(msg,1,(size_t)fsz,f); fclose(f);
        
        const char *shex=argv[4];
        if(strlen(shex)<128){free(msg);fprintf(stderr,"sig_hex inválido\n");return 1;}
        uint8_t sig[64]; int bad=0;
        for(int i=0;i<64;i++){
            char hi=shex[i*2],lo=shex[i*2+1]; uint8_t h,l;
            if(hi>='0'&&hi<='9')h=hi-'0'; else if(hi>='a'&&hi<='f')h=hi-'a'+10;
            else if(hi>='A'&&hi<='F')h=hi-'A'+10; else{bad=1;break;}
            if(lo>='0'&&lo<='9')l=lo-'0'; else if(lo>='a'&&lo<='f')l=lo-'a'+10;
            else if(lo>='A'&&lo<='F')l=lo-'A'+10; else{bad=1;break;}
            sig[i]=(h<<4)|l;
        }
        if(bad){free(msg);fprintf(stderr,"sig_hex inválido\n");return 1;}
        RigIdStatus rs=rig_verify(pk,msg,(size_t)fsz,sig);
        free(msg);
        if(rs==RIG_ID_OK){printf("OK — firma válida\n");return 0;}
        printf("FALLO — firma inválida\n"); return 1;

    } else if(strcmp(sub,"dh")==0){
        if(argc<4){fprintf(stderr,"Uso: identity dh <identity_path> <their_pk_hex>\n");return 1;}
        char pw[256]={0};
        fprintf(stderr,"Passphrase: "); fflush(stderr);
        if(!fgets(pw,sizeof(pw),stdin)){fprintf(stderr,"Error\n");return 1;}
        pw[strcspn(pw,"\n")]='\0';
        RigIdentity id; memset(&id,0,sizeof(id));
        RigIdStatus rs=rig_identity_load(pw,argv[2],&id);
        mem_zero(pw,256);
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error cargando identidad: %d\n",rs);return 1;}
        uint8_t their_pk[32];
        if(rig_unhex32(argv[3],their_pk)!=0){
            mem_zero(&id,sizeof(id));fprintf(stderr,"pk_hex inválido\n");return 1;}
        uint8_t shared[32];
        rs=rig_dh(&id,their_pk,shared);
        mem_zero(&id,sizeof(id));
        if(rs!=RIG_ID_OK){fprintf(stderr,"Error DH: %d\n",rs);return 1;}
        char hex[65]; rig_hex32(shared,hex); mem_zero(shared,32);
        printf("shared_secret:%s\n",hex);
        return 0;

    } else {
        fprintf(stderr,"Sub-comando desconocido: %s\n",sub);
        return 1;
    }
}
