#include "rig_face_sovereign.h"

#include "rigdeps/math.h"
#include "rigdeps/stdio.h"
#include "rigdeps/rig_std_base.h"
#include "rigdeps/sys/types.h"
#include "rig_lib.h"

static void join_path(char *out,rl_size n,const char *dir,const char *name){rl_size l=rl_strlen(dir);snprintf(out,n,"%s%s%s",dir,(l&&dir[l-1]=='/')?"":"/",name);}
static void pixel(RigSovImage *im,int x,int y,rl_u8 r,rl_u8 g,rl_u8 b){rl_size p;if(x<0||y<0||x>=(int)im->width||y>=(int)im->height)return;p=((rl_size)y*im->width+x)*3;im->pixels[p]=r;im->pixels[p+1]=g;im->pixels[p+2]=b;}
static void ellipse(RigSovImage *im,float cx,float cy,float rx,float ry,rl_u8 r,rl_u8 g,rl_u8 b){int x,y;for(y=(int)(cy-ry);y<=(int)(cy+ry);y++)for(x=(int)(cx-rx);x<=(int)(cx+rx);x++){float dx=(x-cx)/rx,dy=(y-cy)/ry;if(dx*dx+dy*dy<=1)pixel(im,x,y,r,g,b);}}
static int synthetic_face(RigSovImage *im){int x,y;rl_memset(im,0,sizeof(*im));im->width=256;im->height=256;im->channels=3;im->pixels=(rl_u8*)rl_malloc((rl_size)256*256*3);if(!im->pixels)return RIG_SOV_ENOMEM;for(y=0;y<256;y++)for(x=0;x<256;x++)pixel(im,x,y,34,48,65);ellipse(im,128,130,72,96,188,126,94);ellipse(im,128,112,17,31,205,143,105);ellipse(im,98,113,17,8,238,238,224);ellipse(im,158,113,17,8,238,238,224);ellipse(im,98,113,5,5,45,28,20);ellipse(im,158,113,5,5,45,28,20);ellipse(im,128,163,28,8,116,38,45);ellipse(im,128,160,24,3,221,114,112);for(x=106;x<=122;x++)pixel(im,x,93,65,40,31);for(x=134;x<=150;x++)pixel(im,x,93,65,40,31);return RIG_SOV_OK;}

int rig_sov_selftest(const char *dir, char *report, rl_size report_size) {
    RigSovBodyParams bp = rig_sov_body_default_params();
    RigSovSkeleton sk;
    RigSovMesh body, head;
    RigSovMaterial mat = {{0.55f, 0.32f, 0.24f}, 0.46f, 0.0f, 0.0f};
    RigSovImage face = {0}, render = {0};
    RigSovLandmarks lm;
    RigSovReconParams rp;
    RigSovAudio audio = {0};
    RigSovPhonemeTrack track = {0};
    RigSovPTScene scene;
    RigSovPTRenderOptions opt = {96, 128, 2, 4, 1, 0x5be0cd19137e2179ULL};
    char path[1024], err[256] = {0};
    rl_size body_vertices = 0, body_triangles = 0, i;
    int rc = RIG_SOV_OK;

    if (!dir) return RIG_SOV_EINVAL;
    (void)mkdir(dir, 0775);
    rig_sov_mesh_init(&body);
    rig_sov_mesh_init(&head);

    rc = rig_sov_generate_complete_body(&bp, &body, &sk);
    if (rc) goto done;
    rc = rig_sov_mesh_validate(&body, err, sizeof(err));
    if (rc) goto done;
    body_vertices = body.vertex_count;
    body_triangles = body.triangle_count;

    join_path(path, sizeof(path), dir, "sovereign_body.glb");
    rc = rig_sov_export_glb(&body, &sk, &mat, path);
    if (rc) goto done;
    join_path(path, sizeof(path), dir, "sovereign_body.obj");
    rc = rig_sov_mesh_write_obj(&body, path);
    if (rc) goto done;

    rc = synthetic_face(&face);
    if (rc) goto done;
    join_path(path, sizeof(path), dir, "synthetic_face.ppm");
    rc = rig_sov_image_save_ppm(path, &face);
    if (rc) goto done;
    rc = rig_sov_reconstruct_face(&face, &rp, &lm, &head);
    if (rc) goto done;
    join_path(path, sizeof(path), dir, "reconstructed_head.glb");
    rc = rig_sov_export_glb(&head, NULL, &mat, path);
    if (rc) goto done;

    audio.sample_rate = 16000;
    audio.channels = 1;
    audio.sample_count = 32000;
    audio.samples = (float *)rl_malloc(audio.sample_count * sizeof(float));
    if (!audio.samples) { rc = RIG_SOV_ENOMEM; goto done; }
    for (i = 0; i < audio.sample_count; i++) {
        float t = (float)i / audio.sample_rate;
        float f = t < 0.7f ? 220.0f : (t < 1.4f ? 180.0f : 260.0f);
        float form = sinf(2.0f * 3.14159265f * f * t)
                   + 0.35f * sinf(2.0f * 3.14159265f * (f * 3.2f) * t);
        audio.samples[i] = 0.18f * form;
    }
    rc = rig_sov_audio_to_phonemes(&audio, &track);
    if (rc) goto done;
    join_path(path, sizeof(path), dir, "phonemes.csv");
    rc = rig_sov_phoneme_track_write_csv(&track, path);
    if (rc) goto done;

    rig_sov_pt_scene_init(&scene);
    scene.mesh = body;
    rig_sov_mesh_init(&body);
    scene.material = mat;
    scene.camera_position = (RigSovVec3){0.0f, 1.15f, 3.0f};
    scene.camera_target = (RigSovVec3){0.0f, 0.95f, 0.0f};
    rc = rig_sov_pathtrace(&scene, &opt, &render);
    if (!rc) {
        join_path(path, sizeof(path), dir, "pathtrace.ppm");
        rc = rig_sov_image_save_ppm(path, &render);
    }
    rig_sov_pt_scene_free(&scene);
    if (rc) goto done;

    if (report && report_size) {
        snprintf(report, report_size,
                 "OK: body=%zu vertices/%zu triangles, skeleton=%zu joints, "
                 "landmarks=68, phoneme_events=%zu, GLB/OBJ/pathtrace generated",
                 body_vertices, body_triangles, sk.joint_count, track.count);
    }

done:
    if (rc && report && report_size) {
        snprintf(report, report_size, "FAIL: %s (%d)%s%s",
                 rig_sov_error_string(rc), rc, err[0] ? " — " : "", err);
    }
    rig_sov_mesh_free(&body);
    rig_sov_mesh_free(&head);
    rig_sov_image_free(&face);
    rig_sov_image_free(&render);
    rig_sov_audio_free(&audio);
    rig_sov_phoneme_track_free(&track);
    return rc;
}

static void usage(const char *exe){fprintf(stderr,
"Rig Face Sovereign — C11 standalone\n"
"Usage:\n"
"  %s selftest <output_dir>\n"
"  %s body <output.glb> [height_m] [radial_segments]\n"
"  %s reconstruct <input.ppm> <output.glb>\n"
"  %s track <frames_manifest.txt> <output.csv>\n"
"  %s phonemes <input.wav> <output.csv>\n"
"  %s render <output.ppm> [width height spp]\n"
"  %s camera <device> <output.csv> <frames> [width height]\n",exe,exe,exe,exe,exe,exe,exe);}

int rig_face_sovereign_cli_main(int argc,char **argv){int rc=RIG_SOV_EINVAL;if(argc<2){usage(argv[0]);return 2;}if(!rl_strcmp(argv[1],"selftest")){char report[512];if(argc!=3){usage(argv[0]);return 2;}rc=rig_sov_selftest(argv[2],report,sizeof(report));fprintf(rc?stderr:stdout,"%s\n",report);}
    else if(!rl_strcmp(argv[1],"body")){RigSovBodyParams p=rig_sov_body_default_params();RigSovMesh m;RigSovSkeleton s;RigSovMaterial mat={{0.55f,0.32f,0.24f},0.46f,0,0};if(argc<3||argc>5){usage(argv[0]);return 2;}if(argc>3)p.height_m=(float)atof(argv[3]);if(argc>4)p.radial_segments=(unsigned)atoi(argv[4]);rig_sov_mesh_init(&m);rc=rig_sov_generate_complete_body(&p,&m,&s);if(!rc)rc=rig_sov_export_glb(&m,&s,&mat,argv[2]);if(!rc)printf("GLB: %s — %zu vertices, %zu triangles, %zu joints\n",argv[2],m.vertex_count,m.triangle_count,s.joint_count);rig_sov_mesh_free(&m);}
    else if(!rl_strcmp(argv[1],"reconstruct")){RigSovImage im={0};RigSovMesh m;RigSovLandmarks l;RigSovReconParams p;RigSovMaterial mat={{0.55f,0.32f,0.24f},0.46f,0,0};if(argc!=4){usage(argv[0]);return 2;}rig_sov_mesh_init(&m);rc=rig_sov_image_load(argv[2],&im);if(!rc)rc=rig_sov_reconstruct_face(&im,&p,&l,&m);if(!rc)rc=rig_sov_export_glb(&m,NULL,&mat,argv[3]);if(!rc)printf("Face confidence %.3f, GLB: %s\n",l.confidence,argv[3]);rig_sov_image_free(&im);rig_sov_mesh_free(&m);}
    else if(!rl_strcmp(argv[1],"track")){if(argc!=4){usage(argv[0]);return 2;}rc=rig_sov_track_manifest(argv[2],argv[3]);}
    else if(!rl_strcmp(argv[1],"phonemes")){RigSovAudio a={0};RigSovPhonemeTrack t={0};if(argc!=4){usage(argv[0]);return 2;}rc=rig_sov_audio_load_wav(argv[2],&a);if(!rc)rc=rig_sov_audio_to_phonemes(&a,&t);if(!rc)rc=rig_sov_phoneme_track_write_csv(&t,argv[3]);if(!rc)printf("%zu phoneme events: %s\n",t.count,argv[3]);rig_sov_audio_free(&a);rig_sov_phoneme_track_free(&t);}
    else if(!rl_strcmp(argv[1],"render")){RigSovPTScene s;RigSovPTRenderOptions o={320,480,8,5,1,0x9e3779b97f4a7c15ULL};RigSovBodyParams p=rig_sov_body_default_params();RigSovSkeleton sk;RigSovImage im={0};if(argc<3||argc>6){usage(argv[0]);return 2;}if(argc>3)o.width=(unsigned)atoi(argv[3]);if(argc>4)o.height=(unsigned)atoi(argv[4]);if(argc>5)o.samples_per_pixel=(unsigned)atoi(argv[5]);rig_sov_pt_scene_init(&s);rc=rig_sov_generate_complete_body(&p,&s.mesh,&sk);if(!rc)rc=rig_sov_pathtrace(&s,&o,&im);if(!rc)rc=rig_sov_image_save_ppm(argv[2],&im);rig_sov_image_free(&im);rig_sov_pt_scene_free(&s);}
    else if(!rl_strcmp(argv[1],"camera")){unsigned frames,w=640,h=480;if(argc<5||argc>7){usage(argv[0]);return 2;}frames=(unsigned)atoi(argv[4]);if(argc>5)w=(unsigned)atoi(argv[5]);if(argc>6)h=(unsigned)atoi(argv[6]);rc=rig_sov_camera_track(argv[2],w,h,frames,argv[3]);}
    else {usage(argv[0]);return 2;}if(rc){fprintf(stderr,"Error: %s (%d)\n",rig_sov_error_string(rc),rc);return 1;}return 0;}
