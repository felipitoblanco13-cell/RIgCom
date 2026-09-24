/* ==========================================================================
 * 33_rig_face_pose_body_main.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : nested/rig_face_v2_bridge/rig_face_pose_body_main.c
 * Copias fundidas: 1
 * Funciones      : 3      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
#include "rig_face_pose_body_suite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rig_sov_legacy_main(int argc, char **argv);

static void usage(const char *exe)
{
    fprintf(stderr,
        "%s\nUsage:\n"
        "  %s all <output_dir> [archetype_id] [face_subdivision]\n"
        "  %s face <output.obj> [archetype_id] [subdivision]\n"
        "  %s studio <output.html>\n"
        "  %s archetypes\n"
        "  %s selftest <output_dir>\n"
        "  %s body|reconstruct|track|phonemes|render|camera ...\n",
        rig_suite_version(),exe,exe,exe,exe,exe,exe);
}

static int write_text(const char *path,const char *s)
{
    FILE *f=fopen(path,"wb"); if(!f)return -1;
    size_t n=strlen(s); int rc=fwrite(s,1,n,f)==n?0:-1;
    if(fclose(f)!=0)rc=-1;
    return rc;
}

int main(int argc,char **argv)
{
    if(argc<2){usage(argv[0]);return 2;}
    if(!strcmp(argv[1],"all")){
        if(argc<3||argc>5){usage(argv[0]);return 2;}
        RigSuiteBuildOptions o=rig_suite_default_options();
        if(argc>3)o.archetype=(RigArchetypeID)atoi(argv[3]);
        if(argc>4)o.face_subdivision=(unsigned)atoi(argv[4]);
        char report[1024];int rc=rig_suite_generate(argv[2],&o,report,sizeof(report));
        fprintf(rc?stderr:stdout,"%s\n",report);return rc?1:0;
    }
    if(!strcmp(argv[1],"selftest")){
        if(argc!=3){usage(argv[0]);return 2;}
        char report[1536];int rc=rig_suite_selftest(argv[2],report,sizeof(report));
        fprintf(rc?stderr:stdout,"%s\n",report);return rc?1:0;
    }
    if(!strcmp(argv[1],"archetypes")){
        for(int i=0;i<RIG_ARCH_COUNT;i++){const RigFaceArchetype *a=rig_archetype_get((RigArchetypeID)i);if(a)printf("%2d  %s\n",i,a->name);}return 0;
    }
    if(!strcmp(argv[1],"face")){
        if(argc<3||argc>5){usage(argv[0]);return 2;}
        RigArchetypeID id=argc>3?(RigArchetypeID)atoi(argv[3]):RIG_ARCH_HYPERREALIST;
        unsigned sub=argc>4?(unsigned)atoi(argv[4]):6u;
        const RigFaceArchetype *a=rig_archetype_get(id);if(!a){fprintf(stderr,"Invalid archetype\n");return 1;}
        RigFaceMeshV2 *m=rig_face_v2_create(&a->params,sub);if(!m){fprintf(stderr,"Face generation failed\n");return 1;}
        int rc=rig_face_v2_export_obj(m,argv[2]);
        if(!rc)printf("Face: %s — %u vertices, %u triangles\n",argv[2],m->base.n_verts,m->base.n_tris);
        rig_face_v2_destroy(m);return rc?1:0;
    }
    if(!strcmp(argv[1],"studio")){
        if(argc!=3){usage(argv[0]);return 2;}
        RigFaceNGParams p;RigFaceNGRenderCtx r={35.0f,2.35f,1.0f,2.2f,true,true,true,3};RigArtResultNG out={0};
        rig_face_ng_params_default(&p);p.lod=RIG_LOD_CINEMA;
        int rc=rig_face_ng_assemble(&p,&r,&out)<=0?-1:write_text(argv[2],out.html);
        rig_face_ng_result_free(&out);if(!rc)printf("Studio: %s\n",argv[2]);return rc?1:0;
    }
    if(!strcmp(argv[1],"body")||!strcmp(argv[1],"reconstruct")||!strcmp(argv[1],"track")||
       !strcmp(argv[1],"phonemes")||!strcmp(argv[1],"render")||!strcmp(argv[1],"camera"))
        return rig_sov_legacy_main(argc,argv);
    usage(argv[0]);return 2;
}

