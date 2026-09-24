#include "rig_face_sovereign.h"

#include "rig_math.h"
#include "rig_noext_io.h"
#include "rig_lib.h"

static float clampf__rig_dup_29cb97ea_3(float x,float a,float b){return x<a?a:(x>b?b:x);}
static float gray_at(const RigSovImage *im,int x,int y){
    rl_size p;if(x<0)x=0;if(y<0)y=0;if(x>=(int)im->width)x=(int)im->width-1;if(y>=(int)im->height)y=(int)im->height-1;p=((rl_size)y*im->width+(unsigned)x)*im->channels;
    if(im->channels==1)return im->pixels[p]/255.0f;
    return (0.2126f*im->pixels[p]+0.7152f*im->pixels[p+1]+0.0722f*im->pixels[p+2])/255.0f;
}

static int build_skin_net(RigSovMLP *net){
    /* Synthetic, deterministic color training set in normalized YCbCr and chromaticity. */
    enum {N=96};float x[N*5],y[N];rl_size i;int rc;rl_u64 s=0x243f6a8885a308d3ULL;
    rc=rig_sov_mlp_init(net,5,12,1,s);if(rc)return rc;
    for(i=0;i<N;i++){
        int skin=i<N/2;float r,g,b,Y,Cb,Cr,sum;
        s^=s<<13;s^=s>>7;s^=s<<17;
        if(skin){float mel=(float)((s>>16)&255)/255.0f;r=0.95f-0.55f*mel;g=0.72f-0.43f*mel;b=0.58f-0.36f*mel;r+=((int)(s&31)-15)*0.004f;g+=((int)((s>>5)&31)-15)*0.003f;b+=((int)((s>>10)&31)-15)*0.003f;}
        else {int k=(int)(i%6);r=(k==0||k==3)?0.85f:0.12f;g=(k==1||k==3)?0.82f:0.10f;b=(k==2||k==4)?0.88f:0.08f;r+=((int)(s&63)-31)*0.004f;g+=((int)((s>>6)&63)-31)*0.004f;b+=((int)((s>>12)&63)-31)*0.004f;}
        r=clampf__rig_dup_29cb97ea_3(r,0,1);g=clampf__rig_dup_29cb97ea_3(g,0,1);b=clampf__rig_dup_29cb97ea_3(b,0,1);Y=0.299f*r+0.587f*g+0.114f*b;Cb=(b-Y)*0.564f+0.5f;Cr=(r-Y)*0.713f+0.5f;sum=r+g+b+1e-6f;
        x[i*5+0]=Y;x[i*5+1]=Cb;x[i*5+2]=Cr;x[i*5+3]=r/sum;x[i*5+4]=g/sum;y[i]=skin?1.0f:0.0f;
    }
    rc=rig_sov_mlp_train(net,x,y,N,350,0.055f,0.0001f);if(rc)rig_sov_mlp_free(net);return rc;
}

static int skin_pixel(const RigSovImage *im,unsigned x,unsigned y,const RigSovMLP *net,float *score){
    rl_size p=((rl_size)y*im->width+x)*im->channels;float r,g,b,Y,Cb,Cr,sum,in[5],out[1];int classical;
    if(im->channels==1){r=g=b=im->pixels[p]/255.0f;}else{r=im->pixels[p]/255.0f;g=im->pixels[p+1]/255.0f;b=im->pixels[p+2]/255.0f;}
    Y=0.299f*r+0.587f*g+0.114f*b;Cb=(b-Y)*0.564f+0.5f;Cr=(r-Y)*0.713f+0.5f;sum=r+g+b+1e-6f;in[0]=Y;in[1]=Cb;in[2]=Cr;in[3]=r/sum;in[4]=g/sum;
    if(rig_sov_mlp_infer(net,in,out)!=RIG_SOV_OK)out[0]=0;
    classical=(Cr>0.53f&&Cr<0.72f&&Cb>0.27f&&Cb<0.53f&&r>g*0.88f&&r>b*1.02f&&Y>0.06f);
    if(score)*score=0.65f*out[0]+0.35f*(classical?1.0f:0.0f);
    return classical||out[0]>0.58f;
}

static void make_template(RigSovVec2 p[68]){
    int i;float t;
    /* jaw */for(i=0;i<17;i++){t=(float)i/16.0f;p[i].x=0.08f+0.84f*t;p[i].y=0.55f+0.39f*sinf((float)3.141592653589793*(1.0f-fabsf(2*t-1))*0.5f);}
    /* brows */for(i=0;i<5;i++){t=(float)i/4;p[17+i]=(RigSovVec2){0.18f+0.25f*t,0.31f-0.035f*sinf(t*3.14159265f)};p[22+i]=(RigSovVec2){0.57f+0.25f*t,0.31f-0.035f*sinf((1-t)*3.14159265f)};}
    /* nose */for(i=0;i<4;i++)p[27+i]=(RigSovVec2){0.50f,0.34f+0.10f*i};
    p[31]=(RigSovVec2){0.39f,0.66f};p[32]=(RigSovVec2){0.44f,0.69f};p[33]=(RigSovVec2){0.50f,0.70f};p[34]=(RigSovVec2){0.56f,0.69f};p[35]=(RigSovVec2){0.61f,0.66f};
    /* eyes */for(i=0;i<6;i++){static const float ex[6]={-1,-.45,.45,1,.45,-.45};static const float ey[6]={0,-.55,-.55,0,.55,.55};p[36+i]=(RigSovVec2){0.32f+0.105f*ex[i],0.43f+0.045f*ey[i]};p[42+i]=(RigSovVec2){0.68f+0.105f*ex[i],0.43f+0.045f*ey[i]};}
    /* outer mouth */for(i=0;i<12;i++){float a=(float)(2*3.141592653589793*i/12);p[48+i]=(RigSovVec2){0.50f+0.20f*cosf(a),0.79f+0.075f*sinf(a)};}
    /* inner mouth */for(i=0;i<8;i++){float a=(float)(2*3.141592653589793*i/8);p[60+i]=(RigSovVec2){0.50f+0.105f*cosf(a),0.79f+0.035f*sinf(a)};}
}

static float grad_mag(const RigSovImage *im,int x,int y){float gx=gray_at(im,x+1,y)-gray_at(im,x-1,y),gy=gray_at(im,x,y+1)-gray_at(im,x,y-1);return sqrtf(gx*gx+gy*gy);}

int rig_sov_detect_landmarks(const RigSovImage *image,RigSovLandmarks *lm){
    RigSovMLP net;rl_u8 *mask=NULL,*seen=NULL;int *qx=NULL,*qy=NULL;rl_size n,qh,qt,best=0;unsigned W,H,x,y;int minx=0,miny=0,maxx=0,maxy=0,bx0=0,by0=0,bx1=0,by1=0;RigSovVec2 tpl[68];int rc=RIG_SOV_OK,i;
    if(!image||!lm||!image->pixels||image->width<32||image->height<32||(image->channels!=1&&image->channels!=3))return RIG_SOV_EINVAL;
    rl_memset(lm,0,sizeof(*lm));W=image->width;H=image->height;n=(rl_size)W*H;mask=(rl_u8*)rl_calloc(n,1);seen=(rl_u8*)rl_calloc(n,1);qx=(int*)rl_malloc(n*sizeof(int));qy=(int*)rl_malloc(n*sizeof(int));if(!mask||!seen||!qx||!qy){rc=RIG_SOV_ENOMEM;goto done;}
    if((rc=build_skin_net(&net)))goto done;
    for(y=1;y+1<H;y++)for(x=1;x+1<W;x++){float s=0;if(skin_pixel(image,x,y,&net,&s)){mask[(rl_size)y*W+x]=1;}}
    /* morphology: close isolated gaps and remove isolated pixels */
    for(y=1;y+1<H;y++)for(x=1;x+1<W;x++){int c=0,dy,dx;for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)c+=mask[(rl_size)(y+dy)*W+(x+dx)]?1:0;if(c>=5)seen[(rl_size)y*W+x]=1;}
    rl_memcpy(mask,seen,n);rl_memset(seen,0,n);
    for(y=0;y<H;y++)for(x=0;x<W;x++){rl_size start=(rl_size)y*W+x;if(!mask[start]||seen[start])continue;qh=qt=0;qx[qt]=(int)x;qy[qt++]=(int)y;seen[start]=1;minx=maxx=(int)x;miny=maxy=(int)y;while(qh<qt){int cx=qx[qh],cy=qy[qh++],k;static const int d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};if(cx<minx)minx=cx;if(cx>maxx)maxx=cx;if(cy<miny)miny=cy;if(cy>maxy)maxy=cy;for(k=0;k<8;k++){int nx=cx+d[k][0],ny=cy+d[k][1];rl_size z;if(nx<0||ny<0||nx>=(int)W||ny>=(int)H)continue;z=(rl_size)ny*W+nx;if(mask[z]&&!seen[z]){seen[z]=1;qx[qt]=nx;qy[qt++]=ny;}}}if(qt>best){best=qt;bx0=minx;by0=miny;bx1=maxx;by1=maxy;}}
    rig_sov_mlp_free(&net);
    if(best<n/250||bx1-bx0<(int)W/10||by1-by0<(int)H/10){rc=RIG_SOV_EFORMAT;goto done;}
    /* Expand component to include forehead and chin, then normalize to a face-shaped box. */
    {float bw=(float)(bx1-bx0+1),bh=(float)(by1-by0+1),cx=(bx0+bx1)*0.5f,cy=(by0+by1)*0.5f;bw*=1.08f;bh*=1.24f;bx0=(int)(cx-bw*0.5f);bx1=(int)(cx+bw*0.5f);by0=(int)(cy-bh*0.54f);by1=(int)(cy+bh*0.46f);if(bx0<0)bx0=0;if(by0<0)by0=0;if(bx1>=(int)W)bx1=(int)W-1;if(by1>=(int)H)by1=(int)H-1;}
    lm->bbox_x=(float)bx0;lm->bbox_y=(float)by0;lm->bbox_w=(float)(bx1-bx0+1);lm->bbox_h=(float)(by1-by0+1);make_template(tpl);
    for(i=0;i<68;i++){
        int px=(int)(bx0+tpl[i].x*lm->bbox_w),py=(int)(by0+tpl[i].y*lm->bbox_h),rad=(int)(0.025f*(lm->bbox_w+lm->bbox_h));int xx,yy,bestx=px,besty=py;float gbest=-1,gavg=0;unsigned gc=0;if(rad<2)rad=2;if(rad>14)rad=14;
        for(yy=py-rad;yy<=py+rad;yy++)for(xx=px-rad;xx<=px+rad;xx++){float g;if(xx<1||yy<1||xx>=(int)W-1||yy>=(int)H-1)continue;g=grad_mag(image,xx,yy);gavg+=g;gc++;if(g>gbest){gbest=g;bestx=xx;besty=yy;}}
        lm->points[i]=(RigSovVec2){(float)bestx,(float)besty};lm->point_confidence[i]=clampf__rig_dup_29cb97ea_3((gbest-(gc?gavg/gc:0))*5.0f+0.35f,0.1f,1.0f);
    }
    {float area=(float)best/(float)n,shape=lm->bbox_h/(lm->bbox_w+1e-6f);float conf=clampf__rig_dup_29cb97ea_3(area*7.0f,0,1)*clampf__rig_dup_29cb97ea_3(1.0f-fabsf(shape-1.25f)*0.8f,0.2f,1.0f);lm->confidence=conf;}
done:rl_free(mask);rl_free(seen);rl_free(qx);rl_free(qy);return rc;
}

static float patch_error(const RigSovImage *a,const RigSovImage *b,float ax,float ay,float bx,float by,int rad){int x,y;float e=0,w=0;for(y=-rad;y<=rad;y++)for(x=-rad;x<=rad;x++){float wa=1.0f/(1.0f+0.12f*(x*x+y*y));float d=gray_at(a,(int)(ax+x),(int)(ay+y))-gray_at(b,(int)(bx+x),(int)(by+y));e+=wa*d*d;w+=wa;}return e/(w+1e-9f);}

int rig_sov_track_landmarks(const RigSovImage *prev,const RigSovImage *cur,const RigSovLandmarks *plm,RigSovLandmarks *clm){
    int i,dx,dy;float mean_dx=0,mean_dy=0;if(!prev||!cur||!plm||!clm||prev->width!=cur->width||prev->height!=cur->height)return RIG_SOV_EINVAL;*clm=*plm;
    for(i=0;i<68;i++){float best=1e30f,second=1e30f;int bdx=0,bdy=0;for(dy=-8;dy<=8;dy++)for(dx=-8;dx<=8;dx++){float e=patch_error(prev,cur,plm->points[i].x,plm->points[i].y,plm->points[i].x+dx,plm->points[i].y+dy,3);if(e<best){second=best;best=e;bdx=dx;bdy=dy;}else if(e<second)second=e;}clm->points[i].x=clampf__rig_dup_29cb97ea_3(plm->points[i].x+bdx,0,(float)cur->width-1);clm->points[i].y=clampf__rig_dup_29cb97ea_3(plm->points[i].y+bdy,0,(float)cur->height-1);clm->point_confidence[i]=clampf__rig_dup_29cb97ea_3((second-best)/(second+1e-6f)*3.0f,0.05f,1.0f);mean_dx+=bdx;mean_dy+=bdy;}
    mean_dx/=68;mean_dy/=68;clm->bbox_x=clampf__rig_dup_29cb97ea_3(plm->bbox_x+mean_dx,0,(float)cur->width-1);clm->bbox_y=clampf__rig_dup_29cb97ea_3(plm->bbox_y+mean_dy,0,(float)cur->height-1);clm->confidence=0;for(i=0;i<68;i++)clm->confidence+=clm->point_confidence[i];clm->confidence/=68.0f;
    if (clm->confidence < 0.18f) return rig_sov_detect_landmarks(cur, clm);
    return RIG_SOV_OK;
}

static float dist2(RigSovVec2 a,RigSovVec2 b){float x=a.x-b.x,y=a.y-b.y;return sqrtf(x*x+y*y);}
int rig_sov_reconstruct_face(const RigSovImage *image,RigSovReconParams *p,RigSovLandmarks *lm,RigSovMesh *head){
    RigSovLandmarks local;RigSovLandmarks *L=lm?lm:&local;float scale;int rc;if(!image||!p||!head)return RIG_SOV_EINVAL;if((rc=rig_sov_detect_landmarks(image,L)))return rc;scale=0.16f/(L->bbox_w+1e-6f);rl_memset(p,0,sizeof(*p));p->face_width=L->bbox_w*scale;p->face_height=L->bbox_h*scale;p->jaw_width=dist2(L->points[4],L->points[12])*scale;p->jaw_taper=dist2(L->points[6],L->points[10])/(dist2(L->points[2],L->points[14])+1e-6f);p->eye_distance=dist2(L->points[39],L->points[42])*scale;p->eye_width=(dist2(L->points[36],L->points[39])+dist2(L->points[42],L->points[45]))*0.5f*scale;p->brow_height=((L->points[37].y-L->points[19].y)+(L->points[44].y-L->points[24].y))*0.5f*scale;p->nose_length=dist2(L->points[27],L->points[33])*scale;p->nose_width=dist2(L->points[31],L->points[35])*scale;p->mouth_width=dist2(L->points[48],L->points[54])*scale;p->upper_lip=dist2(L->points[51],L->points[62])*scale;p->lower_lip=dist2(L->points[66],L->points[57])*scale;p->chin_height=dist2(L->points[57],L->points[8])*scale;p->depth=p->face_width*(0.68f+0.18f*clampf__rig_dup_29cb97ea_3(p->nose_length/(p->face_height+1e-6f),0,1));p->asymmetry=((L->points[30].x-(L->bbox_x+L->bbox_w*0.5f))/(L->bbox_w+1e-6f))*2.0f;return rig_sov_generate_head_from_reconstruction(p,64,96,head);
}

static int csv_landmarks(FILE *f,rl_size frame,const RigSovLandmarks *l){int i;if(fprintf(f,"%zu,%.6f,%.3f,%.3f,%.3f,%.3f",frame,l->confidence,l->bbox_x,l->bbox_y,l->bbox_w,l->bbox_h)<0)return RIG_SOV_EIO;for(i=0;i<68;i++)if(fprintf(f,",%.3f,%.3f,%.4f",l->points[i].x,l->points[i].y,l->point_confidence[i])<0)return RIG_SOV_EIO;if(fputc('\n',f)==EOF)return RIG_SOV_EIO;return RIG_SOV_OK;}

int rig_sov_track_manifest(const char *manifest,const char *csv){FILE *mf=NULL,*out=NULL;char line[4096];RigSovImage prev={0},cur={0};RigSovLandmarks lm={0},next={0};rl_size frame=0;int rc=RIG_SOV_OK;if(!manifest||!csv)return RIG_SOV_EINVAL;mf=fopen(manifest,"rb");if(!mf)return RIG_SOV_EIO;out=fopen(csv,"wb");if(!out){fclose(mf);return RIG_SOV_EIO;}fprintf(out,"frame,confidence,bbox_x,bbox_y,bbox_w,bbox_h");{int i;for(i=0;i<68;i++)fprintf(out,",x%d,y%d,c%d",i,i,i);}fputc('\n',out);while(fgets(line,sizeof(line),mf)){rl_size n=rl_strlen(line);while(n&&((unsigned char)line[n-1]<=32))line[--n]='\0';if(!n||line[0]=='#')continue;if((rc=rig_sov_image_load(line,&cur)))break;if(frame==0)rc=rig_sov_detect_landmarks(&cur,&lm);else rc=rig_sov_track_landmarks(&prev,&cur,&lm,&next);if(rc){rig_sov_image_free(&cur);break;}if(frame>0)lm=next;if((rc=csv_landmarks(out,frame,&lm))){rig_sov_image_free(&cur);break;}rig_sov_image_free(&prev);prev=cur;rl_memset(&cur,0,sizeof(cur));frame++;}rig_sov_image_free(&prev);rig_sov_image_free(&cur);if(fclose(out)!=0&&rc==0)rc=RIG_SOV_EIO;fclose(mf);return frame?rc:(rc?rc:RIG_SOV_EFORMAT);}
