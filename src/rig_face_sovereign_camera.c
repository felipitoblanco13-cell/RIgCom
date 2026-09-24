#include "rig_face_sovereign.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <poll.h>
int rig_sov_camera_open__rig_variant_489be054(RigSovCamera **out,const char *device,unsigned width,unsigned height);
int rig_sov_camera_read__rig_variant_2204a3a3(RigSovCamera *c,RigSovImage *frame,unsigned timeout_ms);
void rig_sov_camera_close__rig_variant_2724c592(RigSovCamera *c);

#include "rig_noext_io.h"

#ifdef __linux__
#include "rig_syscall.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_syscall.h"
#include "rig_syscall.h"
#include <linux/videodev2.h>
#include "rig_lib.h"

struct CamBuf { void *ptr; rl_size len; };
struct RigSovCamera {
    int fd;
    unsigned width,height;
    struct CamBuf *buffers;
    unsigned buffer_count;
    int streaming;
};

static int xioctl(int fd,unsigned long req,void *arg){int r;do{r=ioctl(fd,req,arg);}while(r<0&&errno==EINTR);return r;}
static rl_u8 clip8(int v){return (rl_u8)(v<0?0:(v>255?255:v));}
static void yuyv_to_rgb(const rl_u8 *src,rl_u8 *dst,unsigned w,unsigned h){rl_size i,n=(rl_size)w*h;for(i=0;i<n;i+=2){int y0=src[0],u=src[1]-128,y1=src[2],v=src[3]-128;int c0=y0-16,c1=y1-16,d=u,e=v;int r0=(298*c0+409*e+128)>>8,g0=(298*c0-100*d-208*e+128)>>8,b0=(298*c0+516*d+128)>>8;int r1=(298*c1+409*e+128)>>8,g1=(298*c1-100*d-208*e+128)>>8,b1=(298*c1+516*d+128)>>8;dst[0]=clip8(r0);dst[1]=clip8(g0);dst[2]=clip8(b0);dst[3]=clip8(r1);dst[4]=clip8(g1);dst[5]=clip8(b1);src+=4;dst+=6;}}

int rig_sov_camera_open__rig_variant_489be054(RigSovCamera **out,const char *device,unsigned width,unsigned height){RigSovCamera *c=NULL;struct v4l2_capability cap;struct v4l2_format fmt;struct v4l2_requestbuffers req;unsigned i;enum v4l2_buf_type type;if(!out||!device||width<16||height<16)return RIG_SOV_EINVAL;*out=NULL;c=(RigSovCamera*)rl_calloc(1,sizeof(*c));if(!c)return RIG_SOV_ENOMEM;c->fd=open(device,O_RDWR|O_NONBLOCK);if(c->fd<0){rl_free(c);return RIG_SOV_EIO;}if(xioctl(c->fd,VIDIOC_QUERYCAP,&cap)<0||!(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE)||!(cap.capabilities&V4L2_CAP_STREAMING)){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EUNSUPPORTED;}rl_memset(&fmt,0,sizeof(fmt));fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;fmt.fmt.pix.width=width;fmt.fmt.pix.height=height;fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;fmt.fmt.pix.field=V4L2_FIELD_ANY;if(xioctl(c->fd,VIDIOC_S_FMT,&fmt)<0||fmt.fmt.pix.pixelformat!=V4L2_PIX_FMT_YUYV){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EFORMAT;}c->width=fmt.fmt.pix.width;c->height=fmt.fmt.pix.height;rl_memset(&req,0,sizeof(req));req.count=4;req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;req.memory=V4L2_MEMORY_MMAP;if(xioctl(c->fd,VIDIOC_REQBUFS,&req)<0||req.count<2){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EIO;}c->buffers=(struct CamBuf*)rl_calloc(req.count,sizeof(*c->buffers));if(!c->buffers){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_ENOMEM;}c->buffer_count=req.count;for(i=0;i<req.count;i++){struct v4l2_buffer b;rl_memset(&b,0,sizeof(b));b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP;b.index=i;if(xioctl(c->fd,VIDIOC_QUERYBUF,&b)<0){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EIO;}c->buffers[i].len=b.length;c->buffers[i].ptr=mmap(NULL,b.length,PROT_READ|PROT_WRITE,MAP_SHARED,c->fd,b.m.offset);if(c->buffers[i].ptr==MAP_FAILED){c->buffers[i].ptr=NULL;rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EIO;}if(xioctl(c->fd,VIDIOC_QBUF,&b)<0){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EIO;}}type=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(xioctl(c->fd,VIDIOC_STREAMON,&type)<0){rig_sov_camera_close__rig_variant_2724c592(c);return RIG_SOV_EIO;}c->streaming=1;*out=c;return RIG_SOV_OK;}

int rig_sov_camera_read__rig_variant_2204a3a3(RigSovCamera *c,RigSovImage *frame,unsigned timeout_ms){struct pollfd pfd;struct v4l2_buffer b;rl_size need;if(!c||!frame||c->fd<0)return RIG_SOV_EINVAL;rl_memset(frame,0,sizeof(*frame));pfd.fd=c->fd;pfd.events=POLLIN;if(poll(&pfd,1,(int)timeout_ms)<=0)return RIG_SOV_EIO;rl_memset(&b,0,sizeof(b));b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP;if(xioctl(c->fd,VIDIOC_DQBUF,&b)<0)return errno==EAGAIN?RIG_SOV_EIO:RIG_SOV_EIO;if(b.index>=c->buffer_count){return RIG_SOV_ESTATE;}need=(rl_size)c->width*c->height*3;frame->pixels=(rl_u8*)rl_malloc(need);if(!frame->pixels){xioctl(c->fd,VIDIOC_QBUF,&b);return RIG_SOV_ENOMEM;}frame->width=c->width;frame->height=c->height;frame->channels=3;yuyv_to_rgb((const rl_u8*)c->buffers[b.index].ptr,frame->pixels,c->width,c->height);if(xioctl(c->fd,VIDIOC_QBUF,&b)<0){rig_sov_image_free(frame);return RIG_SOV_EIO;}return RIG_SOV_OK;}

void rig_sov_camera_close__rig_variant_2724c592(RigSovCamera *c){unsigned i;if(!c)return;if(c->fd>=0&&c->streaming){enum v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;xioctl(c->fd,VIDIOC_STREAMOFF,&type);}for(i=0;i<c->buffer_count;i++)if(c->buffers&&c->buffers[i].ptr)munmap(c->buffers[i].ptr,c->buffers[i].len);rl_free(c->buffers);if(c->fd>=0)close(c->fd);rl_free(c);}

#else
struct RigSovCamera { int unavailable; };
int rig_sov_camera_open__rig_variant_489be054(RigSovCamera **camera,const char *device,unsigned width,unsigned height){(void)camera;(void)device;(void)width;(void)height;return RIG_SOV_EUNSUPPORTED;}
int rig_sov_camera_read__rig_variant_2204a3a3(RigSovCamera *camera,RigSovImage *frame,unsigned timeout_ms){(void)camera;(void)frame;(void)timeout_ms;return RIG_SOV_EUNSUPPORTED;}
void rig_sov_camera_close__rig_variant_2724c592(RigSovCamera *camera){(void)camera;}
#endif

int rig_sov_camera_track(const char *device,unsigned width,unsigned height,unsigned frame_count,const char *csv_path){RigSovCamera *cam=NULL;RigSovImage prev={0},cur={0};RigSovLandmarks lm={0},next={0};FILE *f=NULL;unsigned frame;int rc;if(!device||!csv_path||!frame_count)return RIG_SOV_EINVAL;rc=rig_sov_camera_open__rig_variant_489be054(&cam,device,width,height);if(rc)return rc;f=fopen(csv_path,"wb");if(!f){rig_sov_camera_close__rig_variant_2724c592(cam);return RIG_SOV_EIO;}fprintf(f,"frame,confidence,bbox_x,bbox_y,bbox_w,bbox_h");{int i;for(i=0;i<68;i++)fprintf(f,",x%d,y%d,c%d",i,i,i);}fputc('\n',f);for(frame=0;frame<frame_count;frame++){int i;rc=rig_sov_camera_read__rig_variant_2204a3a3(cam,&cur,2000);if(rc)break;if(frame==0)rc=rig_sov_detect_landmarks(&cur,&lm);else rc=rig_sov_track_landmarks(&prev,&cur,&lm,&next);if(rc){rig_sov_image_free(&cur);break;}if(frame)lm=next;fprintf(f,"%u,%.6f,%.3f,%.3f,%.3f,%.3f",frame,lm.confidence,lm.bbox_x,lm.bbox_y,lm.bbox_w,lm.bbox_h);for(i=0;i<68;i++)fprintf(f,",%.3f,%.3f,%.4f",lm.points[i].x,lm.points[i].y,lm.point_confidence[i]);fputc('\n',f);rig_sov_image_free(&prev);prev=cur;rl_memset(&cur,0,sizeof(cur));}rig_sov_image_free(&prev);rig_sov_image_free(&cur);if(fclose(f)!=0&&rc==0)rc=RIG_SOV_EIO;rig_sov_camera_close__rig_variant_2724c592(cam);return rc;}
