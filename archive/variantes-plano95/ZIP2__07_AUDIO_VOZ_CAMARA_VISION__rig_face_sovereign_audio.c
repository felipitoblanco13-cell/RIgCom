#include "rig_face_sovereign.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf(float x,float a,float b){return x<a?a:(x>b?b:x);}

typedef struct {
    const char *ipa;
    float f1,f2;      /* Hz, zero for consonants */
    float voiced;     /* expected [0,1] */
    float noise;      /* expected spectral flatness [0,1] */
    float zcr;        /* expected normalized zero-crossing rate */
    float jaw,round,spread,protrude;
} PhonModel;

static const PhonModel PH[RIG_SOV_PHONEME_COUNT] = {
    {"iː",270,2290,1,.08,.05,.20,.05,.85,.00},{"ɪ",390,1990,1,.10,.06,.22,.05,.80,.00},
    {"e",530,1840,1,.09,.05,.35,.05,.70,.00},{"æ",660,1720,1,.10,.05,.55,.02,.60,.00},
    {"ɑː",730,1090,1,.08,.04,.75,.02,.40,.00},{"ɒ",570,840,1,.09,.04,.70,.40,.20,.15},
    {"ɔː",570,840,1,.08,.04,.60,.60,.15,.25},{"ʊ",440,1020,1,.08,.04,.35,.70,.10,.30},
    {"uː",300,870,1,.07,.03,.30,.90,.05,.40},{"ʌ",640,1190,1,.10,.05,.55,.05,.50,.00},
    {"ɜː",490,1350,1,.10,.05,.45,.30,.40,.10},{"ə",500,1500,.9,.13,.06,.30,.15,.50,.05},
    {"eɪ",500,1900,1,.09,.05,.40,.05,.65,.00},{"aɪ",650,1500,1,.09,.05,.65,.05,.50,.00},
    {"ɔɪ",600,1050,1,.09,.05,.58,.50,.20,.20},{"əʊ",480,1100,1,.09,.04,.40,.60,.15,.20},
    {"aʊ",650,1250,1,.09,.05,.62,.20,.35,.08},{"p",0,0,0,.55,.35,.00,.70,.00,.10},
    {"b",0,0,.75,.30,.18,.00,.70,.00,.10},{"m",250,900,1,.12,.03,.00,.65,.00,.05},
    {"f",0,0,0,.80,.48,.10,.20,.40,.00},{"v",0,0,.65,.58,.30,.10,.20,.40,.00},
    {"θ",0,0,0,.72,.42,.12,.05,.55,.00},{"ð",0,0,.65,.48,.25,.12,.05,.55,.00},
    {"t",0,0,.05,.62,.42,.05,.10,.60,.00},{"d",0,0,.70,.34,.22,.05,.10,.60,.00},
    {"s",0,0,0,.92,.66,.08,.05,.75,.00},{"z",0,0,.65,.70,.48,.08,.05,.75,.00},
    {"n",250,1700,1,.14,.05,.05,.10,.55,.00},{"l",350,1500,1,.12,.06,.20,.05,.65,.00},
    {"r",350,1300,1,.12,.06,.25,.30,.35,.15},{"ʃ",0,0,0,.88,.50,.12,.45,.35,.10},
    {"ʒ",0,0,.65,.65,.36,.12,.45,.35,.10},{"tʃ",0,0,.10,.78,.50,.08,.40,.30,.08},
    {"dʒ",0,0,.62,.56,.33,.08,.40,.30,.08},{"k",0,0,.05,.62,.36,.10,.10,.50,.00},
    {"g",0,0,.72,.32,.20,.10,.10,.50,.00},{"ŋ",300,2000,1,.13,.04,.05,.15,.45,.00},
    {"w",320,750,1,.10,.04,.22,.85,.05,.35},{"j",300,2200,1,.10,.04,.20,.05,.80,.00},
    {"h",0,0,0,.72,.44,.40,.05,.45,.00},{"rr",350,1450,.85,.28,.15,.28,.20,.45,.12},
    {"ɲ",300,2100,1,.14,.05,.08,.15,.55,.00},{"x",0,0,0,.78,.46,.15,.05,.45,.00},
    {"ʎ",350,1950,1,.12,.05,.18,.10,.60,.00},{"ɥ",300,1900,1,.10,.04,.25,.90,.05,.40},
    {"ɛ̃",550,1750,1,.15,.05,.38,.05,.68,.00},{"ɔ̃",560,900,1,.15,.04,.55,.55,.18,.22},
    {"ã",700,1100,1,.15,.04,.72,.08,.38,.00},{"",0,0,0,0,0,.00,.10,.30,.00},
    {"breath",0,0,0,.95,.30,.15,.05,.40,.00},{"ʔ",0,0,.05,.35,.18,.02,.10,.30,.00},
    {"ʀ",300,900,.75,.42,.20,.30,.25,.35,.12},{"ç",0,0,0,.82,.54,.12,.10,.55,.00},
    {"ɣ",0,0,.72,.45,.28,.15,.10,.45,.00},{"β",0,0,.72,.42,.24,.08,.30,.35,.08},
    {"ɾ",350,1450,.90,.20,.10,.18,.15,.50,.08},{"ts",0,0,.08,.84,.58,.07,.08,.68,.00},
    {"dz",0,0,.62,.62,.40,.07,.08,.68,.00},{"ǀ",0,0,.02,.75,.55,.02,.20,.30,.00}
};

const char *rig_sov_phoneme_ipa(int id){return id>=0&&id<RIG_SOV_PHONEME_COUNT?PH[id].ipa:"?";}

static size_t next_pow2(size_t n){size_t p=1;while(p<n&&p<(1u<<20))p<<=1;return p;}
static void fft(float *re,float *im,size_t n){size_t i,j;for(i=1,j=0;i<n;i++){size_t b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j){float t=re[i];re[i]=re[j];re[j]=t;t=im[i];im[i]=im[j];im[j]=t;}}for(size_t len=2;len<=n;len<<=1){float ang=(float)(-2.0*M_PI/len),wr0=cosf(ang),wi0=sinf(ang);for(i=0;i<n;i+=len){float wr=1,wi=0;for(j=0;j<len/2;j++){size_t u=i+j,v=i+j+len/2;float tr=wr*re[v]-wi*im[v],ti=wr*im[v]+wi*re[v],ur=re[u],ui=im[u];re[u]=ur+tr;im[u]=ui+ti;re[v]=ur-tr;im[v]=ui-ti;{float nr=wr*wr0-wi*wi0;wi=wr*wi0+wi*wr0;wr=nr;}}}}}

typedef struct {float energy,zcr,flat,centroid,pitch,f1,f2,voiced;} Features;
static Features features(const float *s,size_t n,unsigned rate,float *re,float *im,size_t N){Features f={0};size_t i,k;double e=0,z=0,gm=0,am=0,cs=0,sw=0;float prev=s[0];for(i=0;i<n;i++){float w=0.54f-0.46f*cosf((float)(2*M_PI*i/(n-1)));float x=s[i]*w;e+=x*x;if(i&&((s[i]>=0)!=(prev>=0)))z++;prev=s[i];re[i]=x;im[i]=0;}for(i=n;i<N;i++)re[i]=im[i]=0;f.energy=sqrtf((float)(e/n));f.zcr=(float)(z/n);fft(re,im,N);for(k=1;k<N/2;k++){float mag=sqrtf(re[k]*re[k]+im[k]*im[k])+1e-12f;float hz=(float)k*rate/N;am+=mag;gm+=log(mag);cs+=hz*mag;sw+=mag;}f.centroid=sw?(float)(cs/sw):0;f.flat=am?(float)(exp(gm/(N/2-1))/(am/(N/2-1))):0;
    /* autocorrelation pitch */{size_t minlag=rate/450,maxlag=rate/65,lag,bestlag=0;double best=0,zero=0;if(maxlag>=n)maxlag=n-1;for(i=0;i<n;i++)zero+=s[i]*s[i];for(lag=minlag;lag<=maxlag;lag++){double c=0;for(i=0;i+lag<n;i++)c+=s[i]*s[i+lag];if(c>best){best=c;bestlag=lag;}}f.voiced=zero>1e-9?(float)(best/zero):0;f.pitch=bestlag?(float)rate/bestlag:0;}
    /* two strongest separated low-frequency peaks approximate F1/F2 */{float best1=0,best2=0;size_t k1=0,k2=0;size_t maxk=(size_t)(4000.0*N/rate);if(maxk>N/2)maxk=N/2;for(k=2;k+2<maxk;k++){float m=re[k]*re[k]+im[k]*im[k];if(m>re[k-1]*re[k-1]+im[k-1]*im[k-1]&&m>re[k+1]*re[k+1]+im[k+1]*im[k+1]){float hz=(float)k*rate/N;if(hz>180&&hz<1200&&m>best1){best1=m;k1=k;}else if(hz>700&&m>best2){best2=m;k2=k;}}}f.f1=(float)k1*rate/N;f.f2=(float)k2*rate/N;if(f.f2<f.f1+300)f.f2=f.centroid;}
    return f;}

static int classify(const Features *f,float prev_energy){int i,best=49;float bestc=1e30f;if(f->energy<0.004f)return 49;if(f->flat>0.72f&&f->energy<0.025f)return 50;for(i=0;i<RIG_SOV_PHONEME_COUNT;i++){float c;if(i==49||i==50)continue;c=3.0f*(f->voiced-PH[i].voiced)*(f->voiced-PH[i].voiced)+2.0f*(f->flat-PH[i].noise)*(f->flat-PH[i].noise)+1.5f*(f->zcr-PH[i].zcr)*(f->zcr-PH[i].zcr);if(PH[i].f1>0&&f->voiced>0.25f){float d1=(f->f1-PH[i].f1)/700.0f,d2=(f->f2-PH[i].f2)/1800.0f;c+=d1*d1+d2*d2;}else if(PH[i].f1>0)c+=1.5f;else if(f->voiced>0.65f)c+=0.45f;if((i==17||i==24||i==35||i==51||i==57||i==59)&&f->energy>prev_energy*1.8f)c-=0.4f;if(c<bestc){bestc=c;best=i;}}return best;}

static int track_push(RigSovPhonemeTrack *t,const RigSovPhonemeEvent *e){if(t->count==t->capacity){size_t nc=t->capacity?t->capacity*2:128;RigSovPhonemeEvent *ne=(RigSovPhonemeEvent*)realloc(t->events,nc*sizeof(*ne));if(!ne)return RIG_SOV_ENOMEM;t->events=ne;t->capacity=nc;}t->events[t->count++]=*e;return RIG_SOV_OK;}

int rig_sov_audio_to_phonemes(const RigSovAudio *a,RigSovPhonemeTrack *t){size_t frame,hop,N,pos;float *re=NULL,*im=NULL,prevE=0;int current=-1,rc=0;RigSovPhonemeEvent ev={0};if(!a||!t||!a->samples||a->sample_rate<8000||a->sample_count<a->sample_rate/20)return RIG_SOV_EINVAL;memset(t,0,sizeof(*t));t->sample_rate=a->sample_rate;frame=(size_t)(a->sample_rate*0.025f);hop=(size_t)(a->sample_rate*0.010f);if(frame<128)frame=128;if(hop<32)hop=32;N=next_pow2(frame);re=(float*)malloc(N*sizeof(float));im=(float*)malloc(N*sizeof(float));if(!re||!im){free(re);free(im);return RIG_SOV_ENOMEM;}for(pos=0;pos+frame<=a->sample_count;pos+=hop){Features f=features(a->samples+pos,frame,a->sample_rate,re,im,N);int id=classify(&f,prevE);float start=(float)pos/a->sample_rate,end=(float)(pos+frame)/a->sample_rate,conf=clampf(0.35f+0.45f*f.voiced+0.25f*(1-f.flat),0.1f,1.0f);if(id!=current){if(current>=0){ev.end_s=start;if(ev.end_s-ev.start_s>=0.008f){rc=track_push(t,&ev);if(rc)break;}}current=id;ev.id=id;ev.start_s=start;ev.end_s=end;ev.confidence=conf;ev.jaw_open=PH[id].jaw;ev.lip_rounding=PH[id].round;ev.lip_spreading=PH[id].spread;ev.lip_protrusion=PH[id].protrude;}else{ev.end_s=end;ev.confidence=(ev.confidence+conf)*0.5f;}prevE=f.energy;}if(!rc&&current>=0)rc=track_push(t,&ev);free(re);free(im);if(rc){rig_sov_phoneme_track_free(t);return rc;}return t->count?RIG_SOV_OK:RIG_SOV_EFORMAT;}

int rig_sov_phoneme_track_write_csv(const RigSovPhonemeTrack *t,const char *path){FILE *f;size_t i;if(!t||!path||!t->events)return RIG_SOV_EINVAL;f=fopen(path,"wb");if(!f)return RIG_SOV_EIO;fprintf(f,"id,ipa,start_s,end_s,confidence,jaw_open,lip_rounding,lip_spreading,lip_protrusion\n");for(i=0;i<t->count;i++){const RigSovPhonemeEvent *e=&t->events[i];fprintf(f,"%d,\"%s\",%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",e->id,rig_sov_phoneme_ipa(e->id),e->start_s,e->end_s,e->confidence,e->jaw_open,e->lip_rounding,e->lip_spreading,e->lip_protrusion);}if(fclose(f)!=0)return RIG_SOV_EIO;return RIG_SOV_OK;}
void rig_sov_phoneme_track_free(RigSovPhonemeTrack *t){if(t){free(t->events);memset(t,0,sizeof(*t));}}
