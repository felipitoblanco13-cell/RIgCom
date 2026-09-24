/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define RIG_GLES_DYNAMIC_LOAD
#include "../include/rig_gles3.h"

/* ── Definiciones de punteros GL (carga dinámica soberana) ─────────────── */
void      (*glGenTextures)(GLsizei,GLuint*) = NULL;
void      (*glBindTexture)(GLenum,GLuint) = NULL;
void      (*glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid*) = NULL;
void      (*glTexImage2DMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean) = NULL;
void      (*glTexParameteri)(GLenum,GLenum,GLint) = NULL;
void      (*glGenerateMipmap)(GLenum) = NULL;
void      (*glDeleteTextures)(GLsizei,const GLuint*) = NULL;
void      (*glGenFramebuffers)(GLsizei,GLuint*) = NULL;
void      (*glBindFramebuffer)(GLenum,GLuint) = NULL;
void      (*glFramebufferTexture2D)(GLenum,GLenum,GLenum,GLuint,GLint) = NULL;
GLenum    (*glCheckFramebufferStatus)(GLenum) = NULL;
void      (*glDeleteFramebuffers)(GLsizei,const GLuint*) = NULL;
void      (*glGenRenderbuffers)(GLsizei,GLuint*) = NULL;
void      (*glBindRenderbuffer)(GLenum,GLuint) = NULL;
void      (*glRenderbufferStorage)(GLenum,GLenum,GLsizei,GLsizei) = NULL;
void      (*glRenderbufferStorageMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei) = NULL;
void      (*glFramebufferRenderbuffer)(GLenum,GLenum,GLenum,GLuint) = NULL;
void      (*glDeleteRenderbuffers)(GLsizei,const GLuint*) = NULL;
void      (*glGenBuffers)(GLsizei,GLuint*) = NULL;
void      (*glBindBuffer)(GLenum,GLuint) = NULL;
void      (*glBufferData)(GLenum,GLsizeiptr,const GLvoid*,GLenum) = NULL;
void      (*glBufferSubData)(GLenum,GLintptr,GLsizeiptr,const GLvoid*) = NULL;
void      (*glDeleteBuffers)(GLsizei,const GLuint*) = NULL;
void      (*glGenVertexArrays)(GLsizei,GLuint*) = NULL;
void      (*glBindVertexArray)(GLuint) = NULL;
void      (*glDeleteVertexArrays)(GLsizei,const GLuint*) = NULL;
void      (*glVertexAttribPointer)(GLuint,GLint,GLenum,GLboolean,GLsizei,const GLvoid*) = NULL;
void      (*glEnableVertexAttribArray)(GLuint) = NULL;
void      (*glDisableVertexAttribArray)(GLuint) = NULL;
void      (*glVertexAttribDivisor)(GLuint,GLuint) = NULL;
void      (*glDrawArrays)(GLenum,GLint,GLsizei) = NULL;
void      (*glDrawElements)(GLenum,GLsizei,GLenum,const GLvoid*) = NULL;
void      (*glDrawArraysInstanced)(GLenum,GLint,GLsizei,GLsizei) = NULL;
void      (*glDrawElementsInstanced)(GLenum,GLsizei,GLenum,const GLvoid*,GLsizei) = NULL;
GLuint    (*glCreateShader)(GLenum) = NULL;
void      (*glShaderSource)(GLuint,GLsizei,const GLchar**,const GLint*) = NULL;
void      (*glCompileShader)(GLuint) = NULL;
void      (*glGetShaderiv)(GLuint,GLenum,GLint*) = NULL;
void      (*glGetShaderInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = NULL;
void      (*glDeleteShader)(GLuint) = NULL;
GLuint    (*glCreateProgram)(void) = NULL;
void      (*glAttachShader)(GLuint,GLuint) = NULL;
void      (*glLinkProgram)(GLuint) = NULL;
void      (*glGetProgramiv)(GLuint,GLenum,GLint*) = NULL;
void      (*glGetProgramInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = NULL;
void      (*glUseProgram)(GLuint) = NULL;
void      (*glDeleteProgram)(GLuint) = NULL;
GLint     (*glGetUniformLocation)(GLuint,const GLchar*) = NULL;
GLint     (*glGetAttribLocation)(GLuint,const GLchar*) = NULL;
void      (*glUniform1i)(GLint,GLint) = NULL;
void      (*glUniform1f)(GLint,GLfloat) = NULL;
void      (*glUniform2f)(GLint,GLfloat,GLfloat) = NULL;
void      (*glUniform3f)(GLint,GLfloat,GLfloat,GLfloat) = NULL;
void      (*glUniform4f)(GLint,GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void      (*glUniformMatrix4fv)(GLint,GLsizei,GLboolean,const GLfloat*) = NULL;
void      (*glUniform1fv)(GLint,GLsizei,const GLfloat*) = NULL;
void      (*glUniform3fv)(GLint,GLsizei,const GLfloat*) = NULL;
void      (*glActiveTexture)(GLenum) = NULL;
void      (*glEnable)(GLenum) = NULL;
void      (*glDisable)(GLenum) = NULL;
void      (*glDepthFunc)(GLenum) = NULL;
void      (*glDepthMask)(GLboolean) = NULL;
void      (*glBlendFunc)(GLenum,GLenum) = NULL;
void      (*glBlendEquation)(GLenum) = NULL;
void      (*glCullFace)(GLenum) = NULL;
void      (*glFrontFace)(GLenum) = NULL;
void      (*glViewport)(GLint,GLint,GLsizei,GLsizei) = NULL;
void      (*glScissor)(GLint,GLint,GLsizei,GLsizei) = NULL;
void      (*glClearColor)(GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void      (*glClear)(GLbitfield) = NULL;
void      (*glFlush)(void) = NULL;
void      (*glFinish)(void) = NULL;
GLenum    (*glGetError)(void) = NULL;
void      (*glGetIntegerv)(GLenum,GLint*) = NULL;
const GLubyte* (*glGetString)(GLenum) = NULL;
void      (*glReadPixels)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,GLvoid*) = NULL;
void      (*glBlitFramebuffer)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum) = NULL;
void      (*glDrawBuffers)(GLsizei,const GLenum*) = NULL;
void      (*glClearBufferfv)(GLenum,GLint,const GLfloat*) = NULL;
void      (*glBindBufferBase)(GLenum,GLuint,GLuint) = NULL;
GLvoid*   (*glMapBufferRange)(GLenum,GLintptr,GLsizeiptr,GLbitfield) = NULL;
GLboolean (*glUnmapBuffer)(GLenum) = NULL;
void      (*glDispatchCompute)(GLuint,GLuint,GLuint) = NULL;
void      (*glMemoryBarrier)(GLbitfield) = NULL;
void      (*glColorMask)(GLboolean,GLboolean,GLboolean,GLboolean) = NULL;
void      (*glPatchParameteri)(GLenum,GLint) = NULL;
GLsync    (*glFenceSync)(GLenum,GLbitfield) = NULL;
GLenum    (*glClientWaitSync)(GLsync,GLbitfield,uint64_t) = NULL;
void      (*glDeleteSync)(GLsync) = NULL;
void      (*glLineWidth)(GLfloat) = NULL;
void      (*glPolygonOffset)(GLfloat,GLfloat) = NULL;
void      (*glStencilFunc)(GLenum,GLint,GLuint) = NULL;
void      (*glStencilOp)(GLenum,GLenum,GLenum) = NULL;
void      (*glStencilMask)(GLuint) = NULL;
void      (*glBindFragDataLocation)(GLuint,GLuint,const GLchar*) = NULL;
void      (*glGetFloatv)(GLenum, GLfloat*) = NULL;
void      (*glClearDepthf)(GLfloat) = NULL;
GLuint    (*glGetUniformBlockIndex)(GLuint, const GLchar*) = NULL;
void      (*glUniformBlockBinding)(GLuint, GLuint, GLuint) = NULL;
void      (*glTexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei) = NULL;
void      (*glInvalidateFramebuffer)(GLenum, GLsizei, const GLenum*) = NULL;
void      (*glGetActiveUniform)(GLuint,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLchar*) = NULL;
void      (*glUniform2i)(GLint, GLint, GLint) = NULL;
void      (*glUniform4i)(GLint, GLint, GLint, GLint, GLint) = NULL;
void      (*glVertexAttribIPointer)(GLuint,GLint,GLenum,GLsizei,const GLvoid*) = NULL;
void      (*glFramebufferTexture2DMultisampleEXT)(GLenum,GLenum,GLenum,GLuint,GLint,GLsizei) = NULL;
EGLDisplay    (*eglGetDisplay)(EGLNativeDisplayType) = NULL;
EGLint        (*eglInitialize)(EGLDisplay,EGLint*,EGLint*) = NULL;
EGLint        (*eglChooseConfig)(EGLDisplay,const EGLint*,EGLConfig*,EGLint,EGLint*) = NULL;
EGLContext    (*eglCreateContext)(EGLDisplay,EGLConfig,EGLContext,const EGLint*) = NULL;
EGLSurface    (*eglCreatePbufferSurface)(EGLDisplay,EGLConfig,const EGLint*) = NULL;
EGLSurface    (*eglCreateWindowSurface)(EGLDisplay,EGLConfig,EGLNativeWindowType,const EGLint*) = NULL;
EGLint        (*eglMakeCurrent)(EGLDisplay,EGLSurface,EGLSurface,EGLContext) = NULL;
EGLint        (*eglSwapBuffers)(EGLDisplay,EGLSurface) = NULL;
EGLint        (*eglDestroyContext)(EGLDisplay,EGLContext) = NULL;
EGLint        (*eglDestroySurface)(EGLDisplay,EGLSurface) = NULL;
EGLint        (*eglTerminate)(EGLDisplay) = NULL;

#include "rig_gpu.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_noext_types.h"
#include "rig_math.h"
#include "rig_syscall.h"

/*
 * rig_gpu.c — Motor GPU soberano RIGCOM
 *
 * Compilación condicional:
 *   make GPU=1   → stubs desactivados, usa EGL + OpenGL ES 3.0 real
 *   make         → RIG_BACKEND_STUB activo: stubs de CPU para build sin display
 *
 * Los stubs bajo #ifdef RIG_BACKEND_STUB son el fallback legítimo para:
 *   1. Build CI/CD sin display (servidor, compilación cruzada)
 *   2. Android sin contexto EGL activo (background services)
 *   3. Tests unitarios que no necesitan render real
 *
 * Las implementaciones reales están bajo #ifndef RIG_BACKEND_STUB
 * (líneas ~1054+) usando las APIs EGL/GLES3 del sistema.
 */

static const char VS_QUAD[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main(){vUV=aUV;gl_Position=vec4(aPos,0.0,1.0);}\n";

static const char VS_PBR_GEOMETRY[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNorm;\n"
    "layout(location=2) in vec4 aTan;\n"
    "layout(location=3) in vec2 aUV0;\n"
    "layout(location=4) in vec2 aUV1;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_view;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_nmat;\n"
    "out vec3 vPosW;\n"
    "out vec3 vNormW;\n"
    "out vec4 vTanW;\n"
    "out vec2 vUV0;\n"
    "out vec2 vUV1;\n"
    "void main(){\n"
    "  vec4 wpos=u_model*vec4(aPos,1.0);\n"
    "  vPosW=wpos.xyz;\n"
    "  vNormW=mat3(u_nmat)*aNorm;\n"
    "  vTanW=vec4(mat3(u_nmat)*aTan.xyz,aTan.w);\n"
    "  vUV0=aUV0; vUV1=aUV1;\n"
    "  gl_Position=u_proj*u_view*wpos;\n"
    "}\n";

static const char FS_PBR_GEOMETRY[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vPosW;\n"
    "in vec3 vNormW;\n"
    "in vec4 vTanW;\n"
    "in vec2 vUV0;\n"
    "in vec2 vUV1;\n"
    "layout(location=0) out vec4 gAlbRough;\n"
    "layout(location=1) out vec4 gNormMetal;\n"
    "layout(location=2) out vec4 gEmiSSS;\n"
    "uniform sampler2D uAlbedo;\n"
    "uniform sampler2D uNormal;\n"
    "uniform sampler2D uRoughness;\n"
    "uniform sampler2D uMetallic;\n"
    "uniform sampler2D uEmissive;\n"
    "uniform vec3  uAlbVal;\n"
    "uniform float uRoughVal;\n"
    "uniform float uMetalVal;\n"
    "uniform vec3  uEmiVal;\n"
    "uniform float uEmiStr;\n"
    "uniform float uSSSW;\n"
    "uniform int   uHasAlb;\n"
    "uniform int   uHasNorm;\n"
    "uniform int   uHasRough;\n"
    "uniform int   uHasMetal;\n"
    "void main(){\n"
    "  vec3 alb=uHasAlb>0?texture(uAlbedo,vUV0).rgb:uAlbVal;\n"
    "  float ro=uHasRough>0?texture(uRoughness,vUV0).r:uRoughVal;\n"
    "  float me=uHasMetal>0?texture(uMetallic,vUV0).r:uMetalVal;\n"
    "  vec3 N=normalize(vNormW);\n"
    "  if(uHasNorm>0){\n"
    "    vec3 tn=texture(uNormal,vUV0).xyz*2.0-1.0;\n"
    "    vec3 T=normalize(vTanW.xyz);\n"
    "    vec3 B=cross(N,T)*vTanW.w;\n"
    "    N=normalize(mat3(T,B,N)*tn);\n"
    "  }\n"
    "  vec3 emi=uEmiVal*uEmiStr;\n"
    "  gAlbRough=vec4(alb,max(ro,0.04));\n"
    "  gNormMetal=vec4(N*0.5+0.5,me);\n"
    "  gEmiSSS=vec4(emi,uSSSW);\n"
    "}\n";

static const char FS_PBR_LIGHTING[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D gAlbRough;\n"
    "uniform sampler2D gNormMetal;\n"
    "uniform sampler2D gEmiSSS;\n"
    "uniform sampler2D gDepth;\n"
    "uniform sampler2D uShadow;\n"
    "uniform samplerCube uIrr;\n"
    "uniform samplerCube uPref;\n"
    "uniform sampler2D uBRDF;\n"
    "uniform vec3  uLPos[16];\n"
    "uniform vec3  uLCol[16];\n"
    "uniform float uLInt[16];\n"
    "uniform float uLRng[16];\n"
    "uniform int   uLTyp[16];\n"
    "uniform int   uNLights;\n"
    "uniform vec3  uCamPos;\n"
    "uniform mat4  uInvVP;\n"
    "uniform mat4  uLightVP;\n"
    "uniform float uIBL;\n"
    "uniform float uShadBias;\n"
    "const float PI=3.14159265359;\n"
    "vec3 wpos(vec2 uv,float d){\n"
    "  vec4 c=vec4(uv*2.0-1.0,d*2.0-1.0,1.0);\n"
    "  vec4 w=uInvVP*c; return w.xyz/w.w;\n"
    "}\n"
    "float D_GGX(float n,float a){float a2=a*a;float d=n*n*(a2-1.0)+1.0;return a2/(PI*d*d);}\n"
    "float G_Sch(float nv,float nl,float r){float k=(r+1.0)*(r+1.0)/8.0;\n"
    "  float gv=nv/(nv*(1.0-k)+k);float gl=nl/(nl*(1.0-k)+k);return gv*gl;}\n"
    "vec3 F_Sch(float c,vec3 F0){return F0+(1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}\n"
    "vec3 F_SchR(float c,vec3 F0,float r){return F0+(max(vec3(1.0-r),F0)-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}\n"
    "vec3 brdf_dir(vec3 N,vec3 V,vec3 L,vec3 F0,float ro,float me,vec3 alb){\n"
    "  vec3 H=normalize(V+L);\n"
    "  float nl=max(dot(N,L),0.0),nv=max(dot(N,V),0.0),nh=max(dot(N,H),0.0),hv=max(dot(H,V),0.0);\n"
    "  float D=D_GGX(nh,ro*ro),G=G_Sch(nv,nl,ro);\n"
    "  vec3 F=F_Sch(hv,F0);\n"
    "  vec3 spec=D*G*F/max(4.0*nv*nl,1e-4);\n"
    "  vec3 kd=(1.0-F)*(1.0-me);\n"
    "  return (kd*alb/PI+spec)*nl;\n"
    "}\n"
    "float vsm(vec4 lsp){\n"
    "  vec3 p=lsp.xyz/lsp.w; p=p*0.5+0.5;\n"
    "  if(p.z>1.0)return 1.0;\n"
    "  vec2 m=texture(uShadow,p.xy).rg;\n"
    "  float pv=step(p.z,m.x);\n"
    "  float var=max(m.y-m.x*m.x,2e-5);\n"
    "  float dd=p.z-m.x;\n"
    "  return clamp(max(pv,var/(var+dd*dd)),0.0,1.0);\n"
    "}\n"
    "void main(){\n"
    "  vec4 ar=texture(gAlbRough,vUV);\n"
    "  vec4 nm=texture(gNormMetal,vUV);\n"
    "  vec4 es=texture(gEmiSSS,vUV);\n"
    "  float dep=texture(gDepth,vUV).r;\n"
    "  if(dep>=1.0){fragColor=vec4(0.0,0.0,0.0,1.0);return;}\n"
    "  vec3 alb=ar.rgb;float ro=ar.a,me=nm.a;\n"
    "  vec3 N=normalize(nm.rgb*2.0-1.0);\n"
    "  vec3 pos=wpos(vUV,dep);\n"
    "  vec3 V=normalize(uCamPos-pos);\n"
    "  vec3 F0=mix(vec3(0.04),alb,me);\n"
    "  vec3 R=reflect(-V,N);\n"
    "  vec3 Famb=F_SchR(max(dot(N,V),0.0),F0,ro);\n"
    "  vec3 kd=(1.0-Famb)*(1.0-me);\n"
    "  vec3 irr=texture(uIrr,N).rgb;\n"
    "  vec3 pref=textureLod(uPref,R,ro*5.0).rgb;\n"
    "  vec2 bl=texture(uBRDF,vec2(max(dot(N,V),0.0),ro)).rg;\n"
    "  vec3 amb=kd*irr*alb+pref*(Famb*bl.x+bl.y);\n"
    "  vec3 col=amb*uIBL;\n"
    "  for(int i=0;i<uNLights;i++){\n"
    "    vec3 L; float att=1.0;\n"
    "    if(uLTyp[i]==1){L=normalize(-uLPos[i]);}\n"
    "    else{vec3 lv=uLPos[i]-pos;float dist=length(lv);L=lv/dist;\n"
    "      float r=uLRng[i];att=clamp(1.0-(dist/r)*(dist/r),0.0,1.0);att*=att;}\n"
    "    float sh=1.0;\n"
    "    if(i==0){vec4 lsp=uLightVP*vec4(pos,1.0);sh=vsm(lsp);}\n"
    "    col+=brdf_dir(N,V,L,F0,ro,me,alb)*uLCol[i]*uLInt[i]*att*sh;\n"
    "  }\n"
    "  col+=es.rgb;\n"
    "  fragColor=vec4(col,1.0);\n"
    "}\n";

static const char FS_PBR_FORWARD[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vPosW; in vec3 vNormW; in vec4 vTanW; in vec2 vUV0; in vec2 vUV1;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uAlbedo; uniform sampler2D uRoughness; uniform sampler2D uMetallic;\n"
    "uniform samplerCube uIrr; uniform samplerCube uPref; uniform sampler2D uBRDF;\n"
    "uniform vec3 uAlbVal; uniform float uRoughVal; uniform float uMetalVal;\n"
    "uniform float uAlpha; uniform vec3 uCamPos; uniform float uIBL;\n"
    "uniform int uHasAlb; uniform int uHasRough; uniform int uHasMetal;\n"
    "const float PI=3.14159265359;\n"
    "vec3 F_Sch(float c,vec3 F0){return F0+(1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}\n"
    "vec3 F_SchR(float c,vec3 F0,float r){return F0+(max(vec3(1.0-r),F0)-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}\n"
    "void main(){\n"
    "  vec3 alb=uHasAlb>0?texture(uAlbedo,vUV0).rgb:uAlbVal;\n"
    "  float ro=uHasRough>0?texture(uRoughness,vUV0).r:uRoughVal;\n"
    "  float me=uHasMetal>0?texture(uMetallic,vUV0).r:uMetalVal;\n"
    "  vec3 N=normalize(vNormW);\n"
    "  vec3 V=normalize(uCamPos-vPosW);\n"
    "  vec3 F0=mix(vec3(0.04),alb,me);\n"
    "  vec3 R=reflect(-V,N);\n"
    "  vec3 Fa=F_SchR(max(dot(N,V),0.0),F0,ro);\n"
    "  vec3 kd=(1.0-Fa)*(1.0-me);\n"
    "  vec3 amb=kd*texture(uIrr,N).rgb*alb;\n"
    "  vec2 bl=texture(uBRDF,vec2(max(dot(N,V),0.0),ro)).rg;\n"
    "  vec3 pref=textureLod(uPref,R,ro*5.0).rgb;\n"
    "  vec3 col=(amb+pref*(Fa*bl.x+bl.y))*uIBL;\n"
    "  fragColor=vec4(col,uAlpha);\n"
    "}\n";

static const char VS_SHADOW[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_light_vp;\n"
    "void main(){gl_Position=u_light_vp*u_model*vec4(aPos,1.0);}\n";

static const char FS_SHADOW[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) out vec2 fragMoments;\n"
    "void main(){\n"
    "  float d=gl_FragCoord.z;\n"
    "  fragMoments=vec2(d,d*d+(0.25*dFdx(d)*dFdx(d))+(0.25*dFdy(d)*dFdy(d)));\n"
    "}\n";

static const char FS_BLOOM_DOWN[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uSrc;\n"
    "uniform vec2 uTexel;\n"
    "uniform float uThreshold;\n"
    "void main(){\n"
    "  vec3 c=texture(uSrc,vUV).rgb;\n"
    "  c+=texture(uSrc,vUV+vec2(-uTexel.x, uTexel.y)).rgb;\n"
    "  c+=texture(uSrc,vUV+vec2( uTexel.x, uTexel.y)).rgb;\n"
    "  c+=texture(uSrc,vUV+vec2(-uTexel.x,-uTexel.y)).rgb;\n"
    "  c+=texture(uSrc,vUV+vec2( uTexel.x,-uTexel.y)).rgb;\n"
    "  c*=0.2;\n"
    "  float lum=dot(c,vec3(0.2126,0.7152,0.0722));\n"
    "  c*=max(lum-uThreshold,0.0)/max(lum,1e-4);\n"
    "  fragColor=vec4(c,1.0);\n"
    "}\n";

static const char FS_BLOOM_UP[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uSrc;\n"
    "uniform sampler2D uDst;\n"
    "uniform vec2 uTexel;\n"
    "uniform float uStr;\n"
    "void main(){\n"
    "  float w[9]=float[9](1.0,2.0,1.0,2.0,4.0,2.0,1.0,2.0,1.0);\n"
    "  vec2 off[9]=vec2[9](\n"
    "    vec2(-1,-1),vec2(0,-1),vec2(1,-1),\n"
    "    vec2(-1, 0),vec2(0, 0),vec2(1, 0),\n"
    "    vec2(-1, 1),vec2(0, 1),vec2(1, 1));\n"
    "  vec3 c=vec3(0.0);\n"
    "  for(int i=0;i<9;i++) c+=texture(uSrc,vUV+off[i]*uTexel).rgb*w[i];\n"
    "  c/=16.0;\n"
    "  vec3 dst=texture(uDst,vUV).rgb;\n"
    "  fragColor=vec4(mix(dst,dst+c,uStr),1.0);\n"
    "}\n";

static const char FS_TONEMAP[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uHDR;\n"
    "uniform sampler2D uBloom;\n"
    "uniform float uExposure;\n"
    "uniform float uGamma;\n"
    "uniform float uBloomStr;\n"
    "uniform float uGrain;\n"
    "uniform float uVigPow;\n"
    "uniform float uVigSoft;\n"
    "uniform float uCA;\n"
    "uniform float uTime;\n"
    "uniform int   uDoACES;\n"
    "vec3 aces(vec3 v){\n"
    "  v*=0.6;\n"
    "  float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;\n"
    "  return clamp((v*(a*v+b))/(v*(c*v+d)+e),0.0,1.0);\n"
    "}\n"
    "float hash12(vec2 p){\n"
    "  vec3 p3=fract(vec3(p.xyx)*0.1031);\n"
    "  p3+=dot(p3,p3.yzx+33.33);\n"
    "  return fract((p3.x+p3.y)*p3.z);\n"
    "}\n"
    "void main(){\n"
    "  vec3 col=texture(uHDR,vUV).rgb*uExposure;\n"
    "  if(uCA>0.0){\n"
    "    vec2 dir=(vUV-0.5)*uCA;\n"
    "    col.r=texture(uHDR,(vUV-0.5)*(1.0+uCA*0.5)+0.5).r*uExposure;\n"
    "    col.b=texture(uHDR,(vUV-0.5)*(1.0-uCA*0.5)+0.5).b*uExposure;\n"
    "  }\n"
    "  col+=texture(uBloom,vUV).rgb*uBloomStr;\n"
    "  if(uDoACES>0) col=aces(col);\n"
    "  else col=col/(col+1.0);\n"
    "  col=pow(clamp(col,0.0,1.0),vec3(1.0/uGamma));\n"
    "  float g=hash12(vUV+fract(uTime))*uGrain;\n"
    "  col=clamp(col+g-uGrain*0.5,0.0,1.0);\n"
    "  float vd=length(vUV-0.5)*2.0;\n"
    "  col*=1.0-pow(clamp(vd,0.0,1.0)*uVigPow,uVigSoft);\n"
    "  fragColor=vec4(col,1.0);\n"
    "}\n";

static const char FS_SSAO[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uNorm;\n"
    "uniform sampler2D uNoise;\n"
    "uniform vec3  uKernel[16];\n"
    "uniform mat4  uProj;\n"
    "uniform mat4  uInvProj;\n"
    "uniform vec2  uRes;\n"
    "uniform float uRadius;\n"
    "uniform float uBias;\n"
    "vec3 viewpos(vec2 uv,float d){\n"
    "  vec4 c=vec4(uv*2.0-1.0,d*2.0-1.0,1.0);\n"
    "  vec4 v=uInvProj*c; return v.xyz/v.w;\n"
    "}\n"
    "void main(){\n"
    "  float d=texture(uDepth,vUV).r;\n"
    "  if(d>=1.0){fragColor=vec4(1.0);return;}\n"
    "  vec3 vp=viewpos(vUV,d);\n"
    "  vec3 nm=normalize(texture(uNorm,vUV).rgb*2.0-1.0);\n"
    "  vec2 nsc=uv=vUV*uRes/4.0;\n"
    "  vec3 rndv=texture(uNoise,nsc).xyz*2.0-1.0;\n"
    "  vec3 T=normalize(rndv-nm*dot(rndv,nm));\n"
    "  vec3 B=cross(nm,T);\n"
    "  mat3 TBN=mat3(T,B,nm);\n"
    "  float ao=0.0;\n"
    "  for(int i=0;i<16;i++){\n"
    "    vec3 sp=TBN*uKernel[i];\n"
    "    sp=vp+sp*uRadius;\n"
    "    vec4 off=uProj*vec4(sp,1.0);\n"
    "    off.xyz/=off.w;\n"
    "    off.xyz=off.xyz*0.5+0.5;\n"
    "    float sd=texture(uDepth,off.xy).r;\n"
    "    vec3 svp=viewpos(off.xy,sd);\n"
    "    float rng=smoothstep(0.0,1.0,uRadius/abs(vp.z-svp.z));\n"
    "    ao+=step(sp.z+uBias,svp.z)*rng;\n"
    "  }\n"
    "  ao=1.0-ao/16.0;\n"
    "  fragColor=vec4(vec3(ao),1.0);\n"
    "}\n";

static const char FS_SSS[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform sampler2D uColor;\n"
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uSSS;\n"
    "uniform vec2 uDir;\n"
    "uniform float uSSSW;\n"
    "const int N=11;\n"
    "const float weights[N]=float[N](0.064221,0.093913,0.123177,0.144599,0.152781,0.144599,0.123177,0.093913,0.064221,0.064221,0.064221);\n"
    "const float offsets[N]=float[N](-5.0,-4.0,-3.0,-2.0,-1.0,0.0,1.0,2.0,3.0,4.0,5.0);\n"
    "void main(){\n"
    "  float sw=texture(uSSS,vUV).a;\n"
    "  if(sw<0.01){fragColor=texture(uColor,vUV);return;}\n"
    "  float d=texture(uDepth,vUV).r;\n"
    "  float scale=sw*0.001/(d+0.001);\n"
    "  vec3 col=vec3(0.0);\n"
    "  for(int i=0;i<N;i++){\n"
    "    vec2 uv=vUV+uDir*offsets[i]*scale;\n"
    "    col+=texture(uColor,uv).rgb*weights[i];\n"
    "  }\n"
    "  fragColor=vec4(col,1.0);\n"
    "}\n";

static const char VS_SKYBOX[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProj;\n"
    "out vec3 vDir;\n"
    "void main(){\n"
    "  vDir=aPos;\n"
    "  vec4 p=uProj*mat4(mat3(uView))*vec4(aPos,1.0);\n"
    "  gl_Position=p.xyww;\n"
    "}\n";

static const char FS_SKYBOX[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vDir;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform samplerCube uSky;\n"
    "uniform float uRot;\n"
    "uniform float uInt;\n"
    "void main(){\n"
    "  float ca=cos(uRot),sa=sin(uRot);\n"
    "  vec3 d=vec3(vDir.x*ca-vDir.z*sa,vDir.y,vDir.x*sa+vDir.z*ca);\n"
    "  fragColor=vec4(texture(uSky,d).rgb*uInt,1.0);\n"
    "}\n";

static const char VS_SKIN[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNorm;\n"
    "layout(location=2) in vec4 aTan;\n"
    "layout(location=3) in vec2 aUV0;\n"
    "layout(location=4) in uvec4 aBone;\n"
    "layout(location=5) in vec4 aWgt;\n"
    "uniform mat4 uBones[47];\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProj;\n"
    "out vec3 vPosW; out vec3 vNormW; out vec4 vTanW; out vec2 vUV;\n"
    "void main(){\n"
    "  mat4 skin=uBones[aBone.x]*aWgt.x+uBones[aBone.y]*aWgt.y\n"
    "           +uBones[aBone.z]*aWgt.z+uBones[aBone.w]*aWgt.w;\n"
    "  vec4 wp=uModel*skin*vec4(aPos,1.0);\n"
    "  vPosW=wp.xyz;\n"
    "  mat3 nm=transpose(inverse(mat3(uModel*skin)));\n"
    "  vNormW=nm*aNorm; vTanW=vec4(nm*aTan.xyz,aTan.w);\n"
    "  vUV=aUV0;\n"
    "  gl_Position=uProj*uView*wp;\n"
    "}\n";

static const char FS_SKIN[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vPosW; in vec3 vNormW; in vec4 vTanW; in vec2 vUV;\n"
    "layout(location=0) out vec4 gAlbRough;\n"
    "layout(location=1) out vec4 gNormMetal;\n"
    "layout(location=2) out vec4 gEmiSSS;\n"
    "uniform sampler2D uAlb; uniform sampler2D uNorm; uniform sampler2D uPores; uniform sampler2D uSSSTex;\n"
    "uniform float uMelanin; uniform float uHemo; uniform float uCarotene;\n"
    "uniform float uRough; uniform float uAge;\n"
    "const vec3 MELAN=vec3(0.24,0.38,0.48);\n"
    "const vec3 HEMO =vec3(3.67,1.37,0.68);\n"
    "void main(){\n"
    "  vec3 alb=texture(uAlb,vUV).rgb;\n"
    "  vec3 scatter=MELAN*uMelanin+HEMO*uHemo;\n"
    "  alb*=exp(-scatter*0.15);\n"
    "  alb=mix(alb,alb*vec3(1.1,0.9,0.8),uCarotene*0.3);\n"
    "  float pore=texture(uPores,vUV*4.0).r;\n"
    "  float ro=mix(uRough,uRough+0.15,pore);\n"
    "  float sssw=texture(uSSSTex,vUV).r;\n"
    "  sssw=mix(sssw,sssw*1.3,uAge);\n"
    "  vec3 N=normalize(vNormW);\n"
    "  vec3 tn=texture(uNorm,vUV).xyz*2.0-1.0;\n"
    "  vec3 T=normalize(vTanW.xyz);\n"
    "  vec3 B=cross(N,T)*vTanW.w;\n"
    "  N=normalize(mat3(T,B,N)*tn);\n"
    "  gAlbRough=vec4(alb,max(ro,0.04));\n"
    "  gNormMetal=vec4(N*0.5+0.5,0.0);\n"
    "  gEmiSSS=vec4(0.0,0.0,0.0,sssw);\n"
    "}\n";

static const char VS_HAIR[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aTan;\n"
    "layout(location=2) in vec2 aUV;\n"
    "uniform mat4 uModel; uniform mat4 uView; uniform mat4 uProj;\n"
    "out vec3 vPosW; out vec3 vTan; out vec2 vUV;\n"
    "void main(){\n"
    "  vec4 wp=uModel*vec4(aPos,1.0);\n"
    "  vPosW=wp.xyz;\n"
    "  vTan=mat3(uModel)*aTan;\n"
    "  vUV=aUV;\n"
    "  gl_Position=uProj*uView*wp;\n"
    "}\n";

static const char FS_HAIR[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vPosW; in vec3 vTan; in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform vec3  uCamPos;\n"
    "uniform vec3  uLDir;\n"
    "uniform vec3  uLCol;\n"
    "uniform vec3  uHairCol;\n"
    "uniform float uRough;\n"
    "uniform sampler2D uAlpha;\n"
    "float kk_spec(vec3 T,vec3 V,vec3 L,float r){\n"
    "  float tl=dot(T,L),tv=dot(T,V);\n"
    "  float s=sqrt(1.0-tl*tl)*sqrt(1.0-tv*tv)-tl*tv;\n"
    "  return pow(max(s,0.0),1.0/max(r,0.001));\n"
    "}\n"
    "void main(){\n"
    "  float a=texture(uAlpha,vUV).r;\n"
    "  if(a<0.1)discard;\n"
    "  vec3 T=normalize(vTan);\n"
    "  vec3 V=normalize(uCamPos-vPosW);\n"
    "  vec3 L=normalize(-uLDir);\n"
    "  float diff=max(sqrt(1.0-dot(T,L)*dot(T,L)),0.0)*0.7;\n"
    "  float spec=kk_spec(T,V,L,uRough)*0.3;\n"
    "  float spec2=kk_spec(T,V,L,uRough*0.5)*0.1;\n"
    "  vec3 col=uHairCol*(diff+0.1)+uLCol*(spec+spec2);\n"
    "  fragColor=vec4(col,a);\n"
    "}\n";

static const char VS_IRIS[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "uniform mat4 uMVP;\n"
    "out vec2 vUV; out vec3 vPos;\n"
    "void main(){vUV=aUV;vPos=aPos;gl_Position=uMVP*vec4(aPos,1.0);}\n";

static const char FS_IRIS[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV; in vec3 vPos;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform vec3  uIrisCol;\n"
    "uniform float uDilation;\n"
    "uniform float uParallax;\n"
    "uniform float uMoist;\n"
    "uniform vec3  uViewDir;\n"
    "const float PHI=1.6180339887;\n"
    "vec2 hash22(vec2 p){\n"
    "  p=vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)));\n"
    "  return fract(sin(p)*43758.5453);\n"
    "}\n"
    "float voronoi(vec2 uv,float freq){\n"
    "  vec2 sc=uv*freq;\n"
    "  vec2 cell=floor(sc),frac=fract(sc);\n"
    "  float md=8.0;\n"
    "  for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){\n"
    "    vec2 nb=vec2(float(x),float(y));\n"
    "    vec2 h=hash22(cell+nb);\n"
    "    float d=length(nb+h-frac);\n"
    "    if(d<md)md=d;\n"
    "  }\n"
    "  return md;\n"
    "}\n"
    "void main(){\n"
    "  vec2 uv=vUV-0.5;\n"
    "  float r=length(uv);\n"
    "  if(r>0.5)discard;\n"
    "  /* Paralaje corneal */\n"
    "  vec2 puv=uv+uViewDir.xy*uParallax*0.15;\n"
    "  /* 6 capas Voronoi escaladas por PHI */\n"
    "  float stroma=0.0;\n"
    "  for(int i=0;i<6;i++){\n"
    "    float f=pow(PHI,float(i+1))*2.0;\n"
    "    stroma+=voronoi(puv,f)/(float(i)+1.0);\n"
    "  }\n"
    "  stroma/=3.5;\n"
    "  vec3 col=uIrisCol*stroma;\n"
    "  /* Pupila */\n"
    "  float pd=0.18+uDilation*0.1;\n"
    "  if(r<pd) col=vec3(0.0);\n"
    "  /* Limbo */\n"
    "  float limb=smoothstep(0.45,0.5,r);\n"
    "  col*=(1.0-limb);\n"
    "  /* Humedad corneal */\n"
    "  float fresn=pow(1.0-max(dot(normalize(vec3(uv,sqrt(1.0-r*r))),uViewDir),0.0),5.0);\n"
    "  col+=vec3(uMoist)*fresn*0.3;\n"
    "  fragColor=vec4(col,r<0.5?1.0:0.0);\n"
    "}\n";

static const char VS_HOLO[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNorm;\n"
    "layout(location=2) in vec2 aUV;\n"
    "uniform mat4 uMVP; uniform mat4 uModel;\n"
    "out vec3 vPosW; out vec3 vNormW; out vec2 vUV;\n"
    "void main(){\n"
    "  vec4 wp=uModel*vec4(aPos,1.0);\n"
    "  vPosW=wp.xyz;\n"
    "  vNormW=mat3(uModel)*aNorm;\n"
    "  vUV=aUV;\n"
    "  gl_Position=uMVP*vec4(aPos,1.0);\n"
    "}\n";

static const char FS_HOLO[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec3 vPosW; in vec3 vNormW; in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform vec3  uCamPos;\n"
    "uniform float uTime;\n"
    "uniform float uIri;\n"
    "uniform float uDiff;\n"
    "uniform float uAur;\n"
    "const float PI=3.14159265359;\n"
    "const float PHI=1.6180339887;\n"
    "vec3 rainbow(float t){\n"
    "  return vec3(sin(t),sin(t+PI*2.0/3.0),sin(t+PI*4.0/3.0))*0.5+0.5;\n"
    "}\n"
    "void main(){\n"
    "  vec3 N=normalize(vNormW);\n"
    "  vec3 V=normalize(uCamPos-vPosW);\n"
    "  float nv=max(dot(N,V),0.0);\n"
    "  /* Iridiscencia */\n"
    "  vec3 iri=rainbow((1.0-nv)*PI*2.0*uIri+uTime*0.3)*uIri;\n"
    "  /* Difracción */\n"
    "  float dph=dot(N,V)*PI*uDiff*4.0+vUV.x*12.0+uTime;\n"
    "  vec3 diff=rainbow(dph)*uDiff;\n"
    "  /* Aurora — ondas verticales */\n"
    "  float aw=sin(vUV.y*PI*6.0+uTime*1.5)*sin(vUV.x*PI*3.0+uTime)\n"
    "          *uAur;\n"
    "  vec3 aur=rainbow(vUV.y*PHI+uTime*0.5)*max(aw,0.0);\n"
    "  /* Fresnel rim */\n"
    "  float rim=pow(1.0-nv,3.0);\n"
    "  vec3 col=iri+diff+aur;\n"
    "  col=col*(rim*0.6+0.4);\n"
    "  fragColor=vec4(col,0.65+rim*0.35);\n"
    "}\n";

static const char VS_PARTICLE[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec4 aPosLife;\n"
    "layout(location=1) in vec4 aVelSize;\n"
    "layout(location=2) in vec4 aColor;\n"
    "uniform mat4 uVP;\n"
    "uniform vec2 uScreenSize;\n"
    "out vec4 vColor;\n"
    "void main(){\n"
    "  vColor=aColor*aPosLife.w;\n"
    "  vec4 cp=uVP*vec4(aPosLife.xyz,1.0);\n"
    "  gl_Position=cp;\n"
    "  gl_PointSize=max(aVelSize.w*uScreenSize.y*0.5/cp.w,1.0);\n"
    "}\n";

static const char FS_PARTICLE[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec4 vColor;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main(){\n"
    "  vec2 uv=gl_PointCoord*2.0-1.0;\n"
    "  float r=length(uv);\n"
    "  if(r>1.0)discard;\n"
    "  float a=1.0-r*r;\n"
    "  fragColor=vec4(vColor.rgb,vColor.a*a);\n"
    "}\n";

static const char VS_SDF[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main(){vUV=aUV;gl_Position=vec4(aPos,0.0,1.0);}\n";

static const char FS_SDF[] =
    "#version 310 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "uniform vec3  uCamPos;\n"
    "uniform vec3  uCamDir;\n"
    "uniform vec2  uResolution;\n"
    "uniform float uTime;\n"
    "uniform vec3  uPrimPos[8];\n"
    "uniform vec3  uPrimSz[8];\n"
    "uniform int   uPrimType[8];\n"
    "uniform int   uPrimOp[8];\n"
    "uniform int   uNPrim;\n"
    "uniform vec3  uLightDir;\n"
    "const float PHI=1.6180339887;\n"
    "float sdSphere(vec3 p,float r){return length(p)-r;}\n"
    "float sdBox(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0);}\n"
    "float sdCyl(vec3 p,float r,float h){vec2 d=abs(vec2(length(p.xz),p.y))-vec2(r,h);return length(max(d,0.0))+min(max(d.x,d.y),0.0);}\n"
    "float sdTorus(vec3 p,float R,float r){vec2 q=vec2(length(p.xz)-R,p.y);return length(q)-r;}\n"
    "float prim(int i,vec3 p){\n"
    "  vec3 lp=p-uPrimPos[i];\n"
    "  float s=uPrimSz[i].x;\n"
    "  if(uPrimType[i]==0) return sdSphere(lp,s);\n"
    "  if(uPrimType[i]==1) return sdBox(lp,uPrimSz[i]);\n"
    "  if(uPrimType[i]==2) return sdCyl(lp,s,uPrimSz[i].y);\n"
    "  if(uPrimType[i]==3) return sdTorus(lp,s,uPrimSz[i].y);\n"
    "  return sdSphere(lp,s);\n"
    "}\n"
    "float scene(vec3 p){\n"
    "  if(uNPrim==0) return 1e10;\n"
    "  float d=prim(0,p);\n"
    "  for(int i=1;i<uNPrim;i++){\n"
    "    float di=prim(i,p);\n"
    "    if(uPrimOp[i]==0) d=min(d,di);\n"
    "    else if(uPrimOp[i]==1) d=max(d,-di);\n"
    "    else d=max(d,di);\n"
    "  }\n"
    "  return d;\n"
    "}\n"
    "vec3 normal(vec3 p){\n"
    "  float e=0.001;\n"
    "  return normalize(vec3(scene(p+vec3(e,0,0))-scene(p-vec3(e,0,0)),\n"
    "                        scene(p+vec3(0,e,0))-scene(p-vec3(0,e,0)),\n"
    "                        scene(p+vec3(0,0,e))-scene(p-vec3(0,0,e))));\n"
    "}\n"
    "void main(){\n"
    "  vec2 ndc=(vUV*2.0-1.0)*vec2(uResolution.x/uResolution.y,1.0);\n"
    "  vec3 fwd=normalize(uCamDir);\n"
    "  vec3 rgt=normalize(cross(fwd,vec3(0,1,0)));\n"
    "  vec3 up=cross(rgt,fwd);\n"
    "  vec3 rd=normalize(fwd+ndc.x*rgt*0.8+ndc.y*up*0.8);\n"
    "  vec3 ro=uCamPos;\n"
    "  float t=0.0,hit=0.0;\n"
    "  vec3 col=vec3(0.05);\n"
    "  for(int i=0;i<128;i++){\n"
    "    float d=scene(ro+rd*t);\n"
    "    if(d<0.001){hit=1.0;break;}\n"
    "    if(t>20.0) break;\n"
    "    t+=d;\n"
    "  }\n"
    "  if(hit>0.5){\n"
    "    vec3 p=ro+rd*t;\n"
    "    vec3 n=normal(p);\n"
    "    vec3 L=normalize(-uLightDir);\n"
    "    float diff=max(dot(n,L),0.0);\n"
    "    float spec=pow(max(dot(reflect(-L,n),-rd),0.0),32.0);\n"
    "    float ao=1.0;\n"
    "    for(int i=1;i<=5;i++){\n"
    "      float h=float(i)*0.1;\n"
    "      ao-=(h-scene(p+n*h))/(pow(2.0,float(i)));\n"
    "    }\n"
    "    col=vec3(0.9,0.85,0.8)*(diff*ao+0.1)+vec3(spec);\n"
    "    /* Gold tint — §44 */\n"
    "    col=mix(col,col*vec3(1.022,0.782,0.344)*1.2,spec);\n"
    "  }\n"
    "  fragColor=vec4(col,1.0);\n"
    "}\n";

typedef struct { const char *name; const char *src; } GlslEntry;
static const GlslEntry GLSL_TABLE[] = {
    {"pbr_geometry_vert",   VS_PBR_GEOMETRY},
    {"pbr_geometry_frag",   FS_PBR_GEOMETRY},
    {"pbr_lighting_vert",   VS_QUAD},
    {"pbr_lighting_frag",   FS_PBR_LIGHTING},
    {"pbr_forward_vert",    VS_PBR_GEOMETRY},
    {"pbr_forward_frag",    FS_PBR_FORWARD},
    {"shadow_vert",         VS_SHADOW},
    {"shadow_frag",         FS_SHADOW},
    {"bloom_down_vert",     VS_QUAD},
    {"bloom_down_frag",     FS_BLOOM_DOWN},
    {"bloom_up_frag",       FS_BLOOM_UP},
    {"tonemap_frag",        FS_TONEMAP},
    {"ssao_frag",           FS_SSAO},
    {"sss_frag",            FS_SSS},
    {"skybox_vert",         VS_SKYBOX},
    {"skybox_frag",         FS_SKYBOX},
    {"skin_vert",           VS_SKIN},
    {"skin_frag",           FS_SKIN},
    {"hair_vert",           VS_HAIR},
    {"hair_frag",           FS_HAIR},
    {"iris_vert",           VS_IRIS},
    {"iris_frag",           FS_IRIS},
    {"hologram_vert",       VS_HOLO},
    {"hologram_frag",       FS_HOLO},
    {"particle_vert",       VS_PARTICLE},
    {"particle_frag",       FS_PARTICLE},
    {"sdf_sculpt_vert",     VS_SDF},
    {"sdf_sculpt_frag",     FS_SDF},
    {NULL, NULL}
};

static void gpu_err(RigGPUCtx *g, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g->last_error, sizeof(g->last_error), fmt, ap);
    va_end(ap);
    if (g->verbose) fprintf(stderr, "[RigGPU] %s\n", g->last_error);
}
typedef struct { GLenum internal, base, type; } TexFmt;
static TexFmt tex_fmt(RigTexFormat f) {
    switch (f) {
        case RIG_TEX_RGBA8:          return (TexFmt){GL_RGBA8,            GL_RGBA,            GL_UNSIGNED_BYTE};
        case RIG_TEX_RGBA16F:        return (TexFmt){GL_RGBA16F,          GL_RGBA,            GL_HALF_FLOAT};
        case RIG_TEX_RGBA32F:        return (TexFmt){GL_RGBA32F,          GL_RGBA,            GL_FLOAT};
        case RIG_TEX_RG16F:          return (TexFmt){GL_RG16F,            GL_RG,              GL_HALF_FLOAT};
        case RIG_TEX_R8:             return (TexFmt){GL_R8,               GL_RED,             GL_UNSIGNED_BYTE};
        case RIG_TEX_R16F:           return (TexFmt){GL_R16F,             GL_RED,             GL_HALF_FLOAT};
        case RIG_TEX_DEPTH24:        return (TexFmt){GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT};
        case RIG_TEX_DEPTH32F:       return (TexFmt){GL_DEPTH_COMPONENT32F,GL_DEPTH_COMPONENT, GL_FLOAT};
        default:                     return (TexFmt){GL_RGBA8,            GL_RGBA,            GL_UNSIGNED_BYTE};
    }
}
static GLenum buf_target(RigBufType t) {
    switch (t) {
        case RIG_BUF_VERTEX:   return GL_ARRAY_BUFFER;
        case RIG_BUF_INDEX:    return GL_ELEMENT_ARRAY_BUFFER;
        case RIG_BUF_UNIFORM:  return GL_UNIFORM_BUFFER;
        case RIG_BUF_STORAGE:  return GL_SHADER_STORAGE_BUFFER;
        case RIG_BUF_INDIRECT: return GL_DRAW_INDIRECT_BUFFER;
        default:               return GL_ARRAY_BUFFER;
    }
}
static GLenum buf_usage(RigBufUsage u) {
    switch (u) {
        case RIG_BUF_STATIC:  return GL_STATIC_DRAW;
        case RIG_BUF_DYNAMIC: return GL_DYNAMIC_DRAW;
        case RIG_BUF_STREAM:  return GL_STREAM_DRAW;
        default:              return GL_STATIC_DRAW;
    }
}
static GLint gpu_uloc(RigGPUCtx *gpu, uint32_t prog_id, const char *name) {
    if (prog_id >= gpu->n_programs) return -1;
    RigProgram *p = &gpu->programs[prog_id];
    for (uint8_t i = 0; i < p->n_uniforms; i++) {
        if (strncmp(p->uniforms[i].name, name, RIG_GPU_MAX_UNIFORM_NAME - 1) == 0)
            return p->uniforms[i].loc;
    }
    if (p->n_uniforms >= 64) return glGetUniformLocation(p->gl_id, name);
    GLint loc = glGetUniformLocation(p->gl_id, name);
    strncpy(p->uniforms[p->n_uniforms].name, name, RIG_GPU_MAX_UNIFORM_NAME - 1);
    p->uniforms[p->n_uniforms].loc = loc;
    p->n_uniforms++;
    return loc;
}
int rig_gpu_init__rig_variant_8fed6a43(RigGPUCtx *gpu, void *native_display,
                 void *native_window, bool offscreen)
{
    memset(gpu, 0, sizeof(*gpu));
    gpu->verbose = true;

    (void)offscreen;

    gpu->egl_display = eglGetDisplay(
        native_display ? (EGLNativeDisplayType)native_display : EGL_DEFAULT_DISPLAY);
    if (gpu->egl_display == EGL_NO_DISPLAY) {
        gpu_err(gpu, "eglGetDisplay falló"); return -1;
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(gpu->egl_display, &major, &minor)) {
        gpu_err(gpu, "eglInitialize falló"); return -1;
    }

    static const EGLint CFG_ATTRS_PB[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE
    };
    static const EGLint CFG_ATTRS_WIN[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_STENCIL_SIZE,    8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    EGLint ncfg = 0;
    const EGLint *_cfg = offscreen ? CFG_ATTRS_PB : CFG_ATTRS_WIN;
    if (!eglChooseConfig(gpu->egl_display, _cfg, &gpu->egl_config, 1, &ncfg) || ncfg < 1) {
        gpu_err(gpu, "eglChooseConfig falló"); return -1;
    }

    static const EGLint CTX_ATTRS[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    gpu->egl_context = eglCreateContext(gpu->egl_display, gpu->egl_config, EGL_NO_CONTEXT, CTX_ATTRS);
    if (gpu->egl_context == EGL_NO_CONTEXT) {
        gpu_err(gpu, "eglCreateContext falló"); return -1;
    }

    if (offscreen || !native_window) {
        static const EGLint PB_ATTRS[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        gpu->egl_surface = eglCreatePbufferSurface(gpu->egl_display, gpu->egl_config, PB_ATTRS);
    } else {
        gpu->egl_surface = eglCreateWindowSurface(
            gpu->egl_display, gpu->egl_config, (EGLNativeWindowType)native_window, NULL);
    }
    if (!gpu->egl_surface) {
        gpu_err(gpu, "eglCreateSurface falló"); return -1;
    }

    if (!eglMakeCurrent(gpu->egl_display, gpu->egl_surface, gpu->egl_surface, gpu->egl_context)) {
        gpu_err(gpu, "eglMakeCurrent falló"); return -1;
    }

    rig_gpu_detect_caps__rig_variant_bf99f70c(gpu);
    gpu->initialized = true;
    return 0;
}

void rig_gpu_destroy__rig_variant_ce2587a6(RigGPUCtx *gpu) {
    if (!gpu->initialized) return;
    eglMakeCurrent(gpu->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (gpu->egl_surface) eglDestroySurface(gpu->egl_display, gpu->egl_surface);
    if (gpu->egl_context) eglDestroyContext(gpu->egl_display, gpu->egl_context);
    eglTerminate(gpu->egl_display);
    gpu->initialized = false;
}

void rig_gpu_present__rig_variant_21198354(RigGPUCtx *gpu) {
    if (gpu->egl_display && gpu->egl_surface)
        eglSwapBuffers(gpu->egl_display, gpu->egl_surface);
}

bool rig_gpu_make_current__rig_variant_ed4ba848(RigGPUCtx *gpu) {
    return eglMakeCurrent(gpu->egl_display, gpu->egl_surface, gpu->egl_surface, gpu->egl_context) == EGL_TRUE;
}

void rig_gpu_release_thread__rig_variant_d58e0b7b(RigGPUCtx *gpu) {
    eglMakeCurrent(gpu->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void rig_gpu_detect_caps__rig_variant_bf99f70c(RigGPUCtx *gpu) {
    const char *r = (const char*)glGetString(GL_RENDERER);
    const char *v = (const char*)glGetString(GL_VERSION);
    if (r) strncpy(gpu->caps.renderer_string, r, 255);
    if (v) strncpy(gpu->caps.version_string,  v, 63);
    GLint ms = 0; glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ms);
    gpu->caps.max_texture_size = (int)ms;
    GLint mc = 0; glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &mc);
    gpu->caps.max_fbo_attachments = (int)mc;
    gpu->caps.compute_shaders  = true;
    gpu->caps.ssbo             = true;
    gpu->caps.half_float_vertex = true;
    gpu->caps.anisotropic_filter = false;
    gpu->caps.max_anisotropy   = 1.0f;
    GLfloat an = 0.0f;
    glGetFloatv(GL_TEXTURE_MAX_ANISOTROPY, &an);
    if (an > 1.0f) { gpu->caps.anisotropic_filter = true; gpu->caps.max_anisotropy = an; }
}

void rig_gpu_print_caps__rig_variant_42647ddb(const RigGPUCtx *gpu) {
    printf("[RigGPU] Renderer : %s\n", gpu->caps.renderer_string);
    printf("[RigGPU] Version  : %s\n", gpu->caps.version_string);
    printf("[RigGPU] TexMax   : %d\n", gpu->caps.max_texture_size);
    printf("[RigGPU] FBO att  : %d\n", gpu->caps.max_fbo_attachments);
    printf("[RigGPU] Aniso    : %.1f%s\n", gpu->caps.max_anisotropy,
           gpu->caps.anisotropic_filter ? "" : " (off)");
}

void rig_gpu_frame_begin__rig_variant_38c25b08(RigGPUCtx *gpu) {
    gpu->stats.draw_calls    = 0;
    gpu->stats.triangles     = 0;
    gpu->stats.shader_switches = 0;
    gpu->stats.texture_binds = 0;
    gpu->stats.state_changes = 0;
}

uint32_t rig_shader_compile__rig_variant_9ccd03ad(RigGPUCtx *gpu, RigShaderStage stage,
                             const char *src, const char *debug_name)
{
    if (gpu->n_shaders >= RIG_GPU_MAX_SHADERS) {
        gpu_err(gpu, "Pool de shaders lleno"); return RIG_GPU_INVALID_ID;
    }
    GLenum gl_stage = (stage == RIG_SHADER_VERT) ? GL_VERTEX_SHADER
                    : (stage == RIG_SHADER_FRAG) ? GL_FRAGMENT_SHADER
                    :                               GL_COMPUTE_SHADER;
    GLuint id = glCreateShader(gl_stage);
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; int len = 0;
        glGetShaderInfoLog(id, 511, &len, log); log[len] = 0;
        gpu_err(gpu, "Shader '%s' error: %s", debug_name ? debug_name : "?", log);
        glDeleteShader(id);
        return RIG_GPU_INVALID_ID;
    }
    uint32_t idx = gpu->n_shaders++;
    RigShader *s  = &gpu->shaders[idx];
    s->id         = idx;
    s->gl_id      = id;
    s->stage      = stage;
    s->compiled   = true;
    return idx;
}

uint32_t rig_program_link__rig_variant_4dec1879(RigGPUCtx *gpu, uint32_t vert_id, uint32_t frag_id) {
    if (gpu->n_programs >= RIG_GPU_MAX_PROGRAMS) { gpu_err(gpu,"Pool programas lleno"); return RIG_GPU_INVALID_ID; }
    if (vert_id >= gpu->n_shaders || frag_id >= gpu->n_shaders) { gpu_err(gpu,"ID shader inválido"); return RIG_GPU_INVALID_ID; }
    GLuint pid = glCreateProgram();
    glAttachShader(pid, gpu->shaders[vert_id].gl_id);
    glAttachShader(pid, gpu->shaders[frag_id].gl_id);
    glLinkProgram(pid);
    GLint ok = GL_FALSE; glGetProgramiv(pid, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; int len = 0;
        glGetProgramInfoLog(pid, 511, &len, log); log[len] = 0;
        gpu_err(gpu, "Link error: %s", log);
        glDeleteProgram(pid); return RIG_GPU_INVALID_ID;
    }
    uint32_t idx = gpu->n_programs++;
    RigProgram *p = &gpu->programs[idx];
    p->id = idx; p->gl_id = pid; p->linked = true;
    p->vert = &gpu->shaders[vert_id];
    p->frag = &gpu->shaders[frag_id];
    p->n_uniforms = 0;
    return idx;
}

uint32_t rig_program_link_compute__rig_variant_eaffd9f3(RigGPUCtx *gpu, uint32_t comp_id) {
    if (gpu->n_programs >= RIG_GPU_MAX_PROGRAMS) return RIG_GPU_INVALID_ID;
    if (comp_id >= gpu->n_shaders) return RIG_GPU_INVALID_ID;
    GLuint pid = glCreateProgram();
    glAttachShader(pid, gpu->shaders[comp_id].gl_id);
    glLinkProgram(pid);
    GLint ok = GL_FALSE; glGetProgramiv(pid, GL_LINK_STATUS, &ok);
    if (!ok) { glDeleteProgram(pid); return RIG_GPU_INVALID_ID; }
    uint32_t idx = gpu->n_programs++;
    gpu->programs[idx].id = idx; gpu->programs[idx].gl_id = pid;
    gpu->programs[idx].linked = true; gpu->programs[idx].comp = &gpu->shaders[comp_id];
    return idx;
}

void rig_program_bind__rig_variant_16192561(RigGPUCtx *gpu, uint32_t prog_id) {
    if (prog_id >= gpu->n_programs) return;
    GLuint gid = gpu->programs[prog_id].gl_id;
    if (gpu->bound_program != gid) {
        glUseProgram(gid);
        gpu->bound_program = gid;
        gpu->stats.shader_switches++;
    }
}

void rig_uniform_1i__rig_variant_b6f38c1e (RigGPUCtx *g, uint32_t p, const char *n, int   v) { glUniform1i (gpu_uloc(g,p,n), v); }
void rig_uniform_1f__rig_variant_139a6141 (RigGPUCtx *g, uint32_t p, const char *n, float v) { glUniform1f (gpu_uloc(g,p,n), v); }
void rig_uniform_2f__rig_variant_2a838c41 (RigGPUCtx *g, uint32_t p, const char *n, float x, float y) { glUniform2f(gpu_uloc(g,p,n),x,y); }
void rig_uniform_3f__rig_variant_6358b329 (RigGPUCtx *g, uint32_t p, const char *n, float x, float y, float z) { glUniform3f(gpu_uloc(g,p,n),x,y,z); }
void rig_uniform_4f__rig_variant_dc6a1f46 (RigGPUCtx *g, uint32_t p, const char *n, float x, float y, float z, float w) { glUniform4f(gpu_uloc(g,p,n),x,y,z,w); }
void rig_uniform_mat4__rig_variant_97562df5(RigGPUCtx *g, uint32_t p, const char *n, const float *m) { glUniformMatrix4fv(gpu_uloc(g,p,n),1,GL_FALSE,m); }

void rig_shader_free__rig_variant_2a355747 (RigGPUCtx *gpu, uint32_t id) { if(id<gpu->n_shaders){glDeleteShader(gpu->shaders[id].gl_id);gpu->shaders[id].compiled=false;} }
void rig_program_free__rig_variant_4af384b7(RigGPUCtx *gpu, uint32_t id) { if(id<gpu->n_programs){glDeleteProgram(gpu->programs[id].gl_id);gpu->programs[id].linked=false;} }

uint32_t rig_buffer_create__rig_variant_91d1085b(RigGPUCtx *gpu, RigBufType type,
                           const void *data, size_t size, RigBufUsage usage)
{
    if (gpu->n_buffers >= RIG_GPU_MAX_VBOS) { gpu_err(gpu,"Pool buffers lleno"); return RIG_GPU_INVALID_ID; }
    GLuint gid; glGenBuffers(1, &gid);
    GLenum target = buf_target(type);
    glBindBuffer(target, gid);
    glBufferData(target, (GLsizeiptr)size, data, buf_usage(usage));
    glBindBuffer(target, 0);
    uint32_t idx = gpu->n_buffers++;
    gpu->buffers[idx].id         = idx;
    gpu->buffers[idx].gl_id      = gid;
    gpu->buffers[idx].type       = type;
    gpu->buffers[idx].size_bytes = size;
    gpu->buffers[idx].usage      = usage;
    return idx;
}

void rig_buffer_update__rig_variant_a187d868(RigGPUCtx *gpu, uint32_t buf_id,
                       const void *data, size_t size, size_t offset)
{
    if (buf_id >= gpu->n_buffers) return;
    GLenum t = buf_target(gpu->buffers[buf_id].type);
    glBindBuffer(t, gpu->buffers[buf_id].gl_id);
    glBufferSubData(t, (GLintptr)offset, (GLsizeiptr)size, data);
    glBindBuffer(t, 0);
}

void rig_buffer_bind__rig_variant_486e6140(RigGPUCtx *gpu, uint32_t buf_id, uint32_t slot) {
    if (buf_id >= gpu->n_buffers) return;
    RigBuffer *b = &gpu->buffers[buf_id];
    if (b->type == RIG_BUF_UNIFORM || b->type == RIG_BUF_STORAGE)
        glBindBufferBase(buf_target(b->type), slot, b->gl_id);
    else
        glBindBuffer(buf_target(b->type), b->gl_id);
}

void rig_buffer_free__rig_variant_1d577ecd(RigGPUCtx *gpu, uint32_t buf_id) {
    if (buf_id >= gpu->n_buffers) return;
    glDeleteBuffers(1, &gpu->buffers[buf_id].gl_id);
    gpu->buffers[buf_id].gl_id = 0;
}

RigVAO *rig_vao_create__rig_variant_1fca59bb(RigGPUCtx *gpu, uint32_t vbo_id, uint32_t ibo_id,
                        const RigVertexAttrib *attribs, uint8_t n_attribs,
                        uint32_t n_vertices, uint32_t n_indices)
{
    (void)gpu;
    RigVAO *v = (RigVAO*)calloc(1, sizeof(RigVAO));
    if (!v) return NULL;
    glGenVertexArrays(1, &v->gl_vao_id);
    glBindVertexArray(v->gl_vao_id);

    if (vbo_id != RIG_GPU_INVALID_ID && vbo_id < gpu->n_buffers) {
        v->vbo = &gpu->buffers[vbo_id];
        glBindBuffer(GL_ARRAY_BUFFER, v->vbo->gl_id);
    }
    if (ibo_id != RIG_GPU_INVALID_ID && ibo_id < gpu->n_buffers) {
        v->ibo = &gpu->buffers[ibo_id];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v->ibo->gl_id);
    }
    v->n_attribs = n_attribs > 8 ? 8 : n_attribs;
    for (uint8_t i = 0; i < v->n_attribs; i++) {
        v->attribs[i] = attribs[i];
        glEnableVertexAttribArray(attribs[i].attrib_index);
        glVertexAttribPointer(attribs[i].attrib_index,
                              (GLint)attribs[i].num_components,
                              attribs[i].gl_type,
                              attribs[i].normalized ? GL_TRUE : GL_FALSE,
                              (GLsizei)attribs[i].stride,
                              (const void*)attribs[i].offset);
    }
    glBindVertexArray(0);
    v->n_vertices = n_vertices;
    v->n_indices  = n_indices;
    return v;
}

void rig_vao_bind__rig_variant_e93c3290(RigGPUCtx *gpu, RigVAO *vao) {
    if (!vao) return;
    if (gpu->bound_vao != vao->gl_vao_id) {
        glBindVertexArray(vao->gl_vao_id);
        gpu->bound_vao = vao->gl_vao_id;
    }
}

void rig_vao_free__rig_variant_432b2aec(RigGPUCtx *gpu, RigVAO *vao) {
    (void)gpu;
    if (!vao) return;
    glDeleteVertexArrays(1, &vao->gl_vao_id);
    free(vao);
}

uint32_t rig_texture_create__rig_variant_5c7ac5a7(RigGPUCtx *gpu, RigTexFormat fmt,
                             uint32_t w, uint32_t h,
                             const void *data, bool gen_mips)
{
    if (gpu->n_textures >= RIG_GPU_MAX_TEXTURES) { gpu_err(gpu,"Pool texturas lleno"); return RIG_GPU_INVALID_ID; }
    TexFmt tf = tex_fmt(fmt);
    GLuint gid; glGenTextures(1, &gid);
    glBindTexture(GL_TEXTURE_2D, gid);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)tf.internal, (GLsizei)w, (GLsizei)h, 0, tf.base, tf.type, data);
    GLenum filter = gen_mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (gen_mips) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    uint32_t idx = gpu->n_textures++;
    gpu->textures[idx].id       = idx;
    gpu->textures[idx].gl_id    = gid;
    gpu->textures[idx].format   = fmt;
    gpu->textures[idx].width    = w;
    gpu->textures[idx].height   = h;
    gpu->textures[idx].depth    = 1;
    gpu->textures[idx].mipmaps  = gen_mips;
    return idx;
}

uint32_t rig_texture_create_cubemap__rig_variant_f821a64d(RigGPUCtx *gpu, RigTexFormat fmt,
                                     uint32_t size, const void *faces[6])
{
    if (gpu->n_textures >= RIG_GPU_MAX_TEXTURES) return RIG_GPU_INVALID_ID;
    TexFmt tf = tex_fmt(fmt);
    GLuint gid; glGenTextures(1, &gid);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gid);
    for (int i = 0; i < 6; i++) {
        glTexImage2D((GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0,
                     (GLint)tf.internal, (GLsizei)size, (GLsizei)size, 0,
                     tf.base, tf.type, faces ? faces[i] : NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    uint32_t idx = gpu->n_textures++;
    gpu->textures[idx].id = idx; gpu->textures[idx].gl_id = gid;
    gpu->textures[idx].format = fmt;
    gpu->textures[idx].width = gpu->textures[idx].height = size;
    gpu->textures[idx].is_cubemap = true;
    return idx;
}

void rig_texture_bind__rig_variant_d8a658e0(RigGPUCtx *gpu, uint32_t tex_id, uint32_t slot) {
    if (tex_id >= gpu->n_textures) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    GLenum target = gpu->textures[tex_id].is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glBindTexture(target, gpu->textures[tex_id].gl_id);
    gpu->stats.texture_binds++;
}

void rig_texture_free__rig_variant_3cd213ca(RigGPUCtx *gpu, uint32_t tex_id) {
    if (tex_id >= gpu->n_textures) return;
    glDeleteTextures(1, &gpu->textures[tex_id].gl_id);
    gpu->textures[tex_id].gl_id = 0;
}

uint32_t rig_fbo_create__rig_variant_8f08aafd(RigGPUCtx *gpu, uint32_t w, uint32_t h,
                        RigTexFormat color_fmt, bool with_depth)
{
    if (gpu->n_fbos >= RIG_GPU_MAX_FBOS) { gpu_err(gpu,"Pool FBO lleno"); return RIG_GPU_INVALID_ID; }
    uint32_t col = rig_texture_create__rig_variant_5c7ac5a7(gpu, color_fmt, w, h, NULL, false);
    if (col == RIG_GPU_INVALID_ID) return RIG_GPU_INVALID_ID;

    GLuint fbo_gl; glGenFramebuffers(1, &fbo_gl);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_gl);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, gpu->textures[col].gl_id, 0);
    uint32_t dep_idx = RIG_GPU_INVALID_ID;
    if (with_depth) {
        dep_idx = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_DEPTH32F, w, h, NULL, false);
        if (dep_idx != RIG_GPU_INVALID_ID)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, gpu->textures[dep_idx].gl_id, 0);
    }
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (st != GL_FRAMEBUFFER_COMPLETE) { gpu_err(gpu,"FBO incompleto %u",st); return RIG_GPU_INVALID_ID; }

    uint32_t idx = gpu->n_fbos++;
    RigFBO *f = &gpu->fbos[idx];
    f->id = idx; f->gl_id = fbo_gl;
    f->color[0] = &gpu->textures[col];
    f->n_color = 1;
    if (dep_idx != RIG_GPU_INVALID_ID) f->depth = &gpu->textures[dep_idx];
    f->width = w; f->height = h;
    return idx;
}

uint32_t rig_fbo_create_msaa__rig_variant_352471dc(RigGPUCtx *gpu, uint32_t w, uint32_t h, uint8_t samples) {

    (void)samples;
    return rig_fbo_create__rig_variant_8f08aafd(gpu, w, h, RIG_TEX_RGBA16F, true);
}

void rig_fbo_bind__rig_variant_6606cf4a(RigGPUCtx *gpu, uint32_t fbo_id) {
    if (fbo_id >= gpu->n_fbos) return;
    GLuint gid = gpu->fbos[fbo_id].gl_id;
    if (gpu->bound_fbo != gid) {
        glBindFramebuffer(GL_FRAMEBUFFER, gid);
        gpu->bound_fbo = gid;
    }
}

void rig_fbo_unbind__rig_variant_2f18cd10(RigGPUCtx *gpu) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gpu->bound_fbo = 0;
}

void rig_fbo_blit__rig_variant_7640e874(RigGPUCtx *gpu, uint32_t src, uint32_t dst,
                  uint32_t w, uint32_t h, bool color, bool depth)
{
    GLuint sg = src < gpu->n_fbos ? gpu->fbos[src].gl_id : 0;
    GLuint dg = dst < gpu->n_fbos ? gpu->fbos[dst].gl_id : 0;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sg);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dg);
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    glBlitFramebuffer(0,0,(GLint)w,(GLint)h, 0,0,(GLint)w,(GLint)h, mask, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gpu->bound_fbo = 0;
}

void rig_fbo_free__rig_variant_0c5d7480(RigGPUCtx *gpu, uint32_t fbo_id) {
    if (fbo_id >= gpu->n_fbos) return;
    glDeleteFramebuffers(1, &gpu->fbos[fbo_id].gl_id);
    gpu->fbos[fbo_id].gl_id = 0;
}

static uint16_t float_to_half(float f) {
    uint32_t x; memcpy(&x, &f, 4);
    uint32_t s = (x >> 16) & 0x8000u;
    uint32_t e = ((x >> 23) & 0xFFu) - 127 + 15;
    uint32_t m = (x & 0x7FFFFFu) >> 13;
    if (e <= 0)  return (uint16_t)s;
    if (e >= 31) return (uint16_t)(s | 0x7C00u);
    return (uint16_t)(s | (e << 10) | m);
}
static uint32_t van_der_corput(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return bits;
}
static void gen_brdf_lut(uint16_t *out, int sz) {
    const int N = 512;
    for (int yi = 0; yi < sz; yi++) {
        float rough = ((float)yi + 0.5f) / (float)sz;
        float a = rough * rough;
        for (int xi = 0; xi < sz; xi++) {
            float NdotV = ((float)xi + 0.5f) / (float)sz;
            NdotV = NdotV < 0.001f ? 0.001f : NdotV;
            float V_x = sqrtf(1.0f - NdotV * NdotV);
            float V_z = NdotV;
            float A = 0.0f, B = 0.0f;
            for (int i = 0; i < N; i++) {

                float u0 = (float)i / (float)N;
                float u1 = (float)van_der_corput((uint32_t)i) * 2.3283064365386963e-10f;

                float phi   = 6.28318530718f * u0;
                float cos_t = sqrtf((1.0f - u1) / (1.0f + (a*a - 1.0f)*u1));
                float sin_t = sqrtf(1.0f - cos_t*cos_t);
                float Hx = cosf(phi)*sin_t, Hy = sinf(phi)*sin_t, Hz = cos_t;
                (void)Hy;
                float VdotH = V_x*Hx + V_z*Hz;
                float NdotH = Hz, NdotL = 2.0f*VdotH*NdotH - V_z;
                if (NdotL <= 0.0f) continue;
                NdotL = NdotL > 1.0f ? 1.0f : NdotL;
                float k  = (rough+1.0f)*(rough+1.0f)/8.0f;
                float Gv = NdotV / (NdotV*(1.0f-k)+k);
                float Gl = NdotL / (NdotL*(1.0f-k)+k);
                float G  = Gv * Gl;
                float Gc = G * VdotH / (NdotH * NdotV);
                float Fc = powf(1.0f - VdotH, 5.0f);
                A += (1.0f - Fc) * Gc;
                B += Fc * Gc;
            }
            out[(yi*sz+xi)*2+0] = float_to_half(A / (float)N);
            out[(yi*sz+xi)*2+1] = float_to_half(B / (float)N);
        }
    }
}
static const float QUAD_DATA[] = {
    -1.0f,-1.0f, 0.0f,0.0f,
     1.0f,-1.0f, 1.0f,0.0f,
    -1.0f, 1.0f, 0.0f,1.0f,
     1.0f, 1.0f, 1.0f,1.0f
};

static uint32_t pbr_prog(RigGPUCtx *gpu, const char *vname, const char *fname) {
    const char *vs = rig_glsl_src__rig_variant_62e6a98c(vname);
    const char *fs = rig_glsl_src__rig_variant_62e6a98c(fname);
    if (!vs || !fs) { fprintf(stderr,"[RigGPU] Shader no encontrado: %s / %s\n",vname,fname); return RIG_GPU_INVALID_ID; }
    uint32_t vid = rig_shader_compile__rig_variant_9ccd03ad(gpu, RIG_SHADER_VERT, vs, vname);
    uint32_t fid = rig_shader_compile__rig_variant_9ccd03ad(gpu, RIG_SHADER_FRAG, fs, fname);
    if (vid == RIG_GPU_INVALID_ID || fid == RIG_GPU_INVALID_ID) return RIG_GPU_INVALID_ID;
    return rig_program_link__rig_variant_4dec1879(gpu, vid, fid);
}
int rig_pbr_pipeline_init__rig_variant_a1fd90e3(RigGPUCtx *gpu, RigPBRPipeline *pbr, uint32_t w, uint32_t h) {
    memset(pbr, 0, sizeof(*pbr));
    pbr->width = w; pbr->height = h;

    pbr->prog_geometry    = pbr_prog(gpu, "pbr_geometry_vert",   "pbr_geometry_frag");
    pbr->prog_lighting    = pbr_prog(gpu, "pbr_lighting_vert",   "pbr_lighting_frag");
    pbr->prog_pbr_forward = pbr_prog(gpu, "pbr_forward_vert",    "pbr_forward_frag");
    pbr->prog_shadow      = pbr_prog(gpu, "shadow_vert",          "shadow_frag");
    pbr->prog_bloom_down  = pbr_prog(gpu, "bloom_down_vert",      "bloom_down_frag");
    pbr->prog_bloom_up    = pbr_prog(gpu, "pbr_lighting_vert",    "bloom_up_frag");
    pbr->prog_tonemap     = pbr_prog(gpu, "pbr_lighting_vert",    "tonemap_frag");
    pbr->prog_sss         = pbr_prog(gpu, "pbr_lighting_vert",    "sss_frag");
    pbr->prog_ssao        = pbr_prog(gpu, "pbr_lighting_vert",    "ssao_frag");
    pbr->prog_skybox      = pbr_prog(gpu, "skybox_vert",           "skybox_frag");

    {
        GLuint gfbo; glGenFramebuffers(1, &gfbo);
        uint32_t alb = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_RGBA16F, w, h, NULL, false);
        uint32_t nrm = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_RGBA16F, w, h, NULL, false);
        uint32_t emi = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_RGBA16F, w, h, NULL, false);
        uint32_t dep = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_DEPTH32F, w, h, NULL, false);
        glBindFramebuffer(GL_FRAMEBUFFER, gfbo);
        static const GLenum BUFS[3] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, BUFS);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gpu->textures[alb].gl_id, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gpu->textures[nrm].gl_id, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gpu->textures[emi].gl_id, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, gpu->textures[dep].gl_id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLuint gfbo_id = gfbo;
        (void)gfbo_id;
        pbr->fbo_gbuffer = (uint32_t)alb;

        uint32_t fi = gpu->n_fbos++;
        gpu->fbos[fi].gl_id = gfbo;
        gpu->fbos[fi].color[0] = &gpu->textures[alb];
        gpu->fbos[fi].color[1] = &gpu->textures[nrm];
        gpu->fbos[fi].color[2] = &gpu->textures[emi];
        gpu->fbos[fi].depth    = &gpu->textures[dep];
        gpu->fbos[fi].n_color  = 3;
        gpu->fbos[fi].width = w; gpu->fbos[fi].height = h;
        pbr->fbo_gbuffer = fi;
    }

    pbr->fbo_hdr   = rig_fbo_create__rig_variant_8f08aafd(gpu, w, h, RIG_TEX_RGBA16F, false);

    pbr->fbo_bloom = rig_fbo_create__rig_variant_8f08aafd(gpu, w/2, h/2, RIG_TEX_RGBA16F, false);

    pbr->fbo_shadow = rig_fbo_create__rig_variant_8f08aafd(gpu, 2048, 2048, RIG_TEX_RG16F, true);

    uint32_t qvbo = rig_buffer_create__rig_variant_91d1085b(gpu, RIG_BUF_VERTEX, QUAD_DATA, sizeof(QUAD_DATA), RIG_BUF_STATIC);
    static const RigVertexAttrib QA[2] = {
        {0, 2, GL_FLOAT, false, 16, 0},
        {1, 2, GL_FLOAT, false, 16, 8},
    };
    RigVAO *qvao = rig_vao_create__rig_variant_1fca59bb(gpu, qvbo, RIG_GPU_INVALID_ID, QA, 2, 4, 0);
    pbr->vbo_quad = qvbo;
    pbr->vao_quad = qvao ? qvao->gl_vao_id : 0;

    const int LUT = 256;
    uint16_t *lut_data = (uint16_t*)malloc((size_t)LUT*LUT*2*sizeof(uint16_t));
    if (lut_data) {
        gen_brdf_lut(lut_data, LUT);
        pbr->tex_brdf_lut = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_RG16F, (uint32_t)LUT, (uint32_t)LUT, lut_data, false);
        free(lut_data);
    }

    const int BN = 64;
    uint8_t *bn = (uint8_t*)malloc((size_t)BN*BN*4);
    if (bn) {
        for (int i = 0; i < BN*BN; i++) {
            uint32_t h = (uint32_t)i * 2654435761u;
            bn[i*4+0] = (uint8_t)(h & 0xFF);
            bn[i*4+1] = (uint8_t)((h>>8) & 0xFF);
            bn[i*4+2] = (uint8_t)((h>>16) & 0xFF);
            bn[i*4+3] = 255;
        }
        pbr->tex_blue_noise = rig_texture_create__rig_variant_5c7ac5a7(gpu, RIG_TEX_RGBA8, (uint32_t)BN, (uint32_t)BN, bn, false);
        free(bn);
    }

    static const float WHITE6[6*4] = {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};
    const void *faces[6]; for(int i=0;i<6;i++) faces[i]=&WHITE6[i*4];
    pbr->tex_irradiance = rig_texture_create_cubemap__rig_variant_f821a64d(gpu, RIG_TEX_RGBA16F, 1, faces);
    pbr->tex_prefilter  = rig_texture_create_cubemap__rig_variant_f821a64d(gpu, RIG_TEX_RGBA16F, 1, faces);

    pbr->initialized = true;
    return 0;
}

void rig_pbr_pipeline_destroy__rig_variant_2acb32d1(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    if (!pbr->initialized) return;
    rig_fbo_free__rig_variant_0c5d7480(gpu, pbr->fbo_gbuffer);
    rig_fbo_free__rig_variant_0c5d7480(gpu, pbr->fbo_hdr);
    rig_fbo_free__rig_variant_0c5d7480(gpu, pbr->fbo_bloom);
    rig_fbo_free__rig_variant_0c5d7480(gpu, pbr->fbo_shadow);
    rig_texture_free__rig_variant_3cd213ca(gpu, pbr->tex_blue_noise);
    rig_texture_free__rig_variant_3cd213ca(gpu, pbr->tex_brdf_lut);
    rig_texture_free__rig_variant_3cd213ca(gpu, pbr->tex_irradiance);
    rig_texture_free__rig_variant_3cd213ca(gpu, pbr->tex_prefilter);
    pbr->initialized = false;
}

void rig_pbr_pipeline_resize__rig_dup_85b654a6(RigGPUCtx *gpu, RigPBRPipeline *pbr, uint32_t w, uint32_t h) {
    rig_pbr_pipeline_destroy__rig_variant_2acb32d1(gpu, pbr);
    rig_pbr_pipeline_init__rig_variant_a1fd90e3(gpu, pbr, w, h);
}

void rig_pbr_begin_geometry__rig_variant_382ce2b6(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_gbuffer);
    rig_state_clear__rig_variant_f239e16c(gpu, 0,0,0,0, 1.0f);
    rig_state_depth__rig_variant_0afda971(gpu, true, true, GL_LESS);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_geometry);
}

void rig_pbr_end_geometry__rig_variant_fa079a7a(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    (void)pbr;
    rig_fbo_unbind__rig_variant_2f18cd10(gpu);
}

void rig_pbr_lighting_pass__rig_variant_2162bc86(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_hdr);
    rig_state_depth__rig_variant_0afda971(gpu, false, false, GL_ALWAYS);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_lighting);

    RigFBO *gb = &gpu->fbos[pbr->fbo_gbuffer];
    for (uint8_t i = 0; i < gb->n_color; i++) {
        if (gb->color[i]) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, gb->color[i]->gl_id);
        }
    }
    if (gb->depth) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gb->depth->gl_id); }
    if (pbr->tex_brdf_lut != RIG_GPU_INVALID_ID) rig_texture_bind__rig_variant_d8a658e0(gpu, pbr->tex_brdf_lut, 5);
    if (pbr->tex_irradiance != RIG_GPU_INVALID_ID) rig_texture_bind__rig_variant_d8a658e0(gpu, pbr->tex_irradiance, 6);
    if (pbr->tex_prefilter  != RIG_GPU_INVALID_ID) rig_texture_bind__rig_variant_d8a658e0(gpu, pbr->tex_prefilter,  7);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
}

void rig_pbr_ssao_pass__rig_variant_e821b55e(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_hdr);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_ssao);
    if (pbr->tex_blue_noise != RIG_GPU_INVALID_ID) rig_texture_bind__rig_variant_d8a658e0(gpu, pbr->tex_blue_noise, 2);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
}

void rig_pbr_sss_pass__rig_variant_0db493a8(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_hdr);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_sss);
    rig_uniform_2f__rig_variant_2a838c41(gpu, pbr->prog_sss, "uDir", 1.0f, 0.0f);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
    rig_uniform_2f__rig_variant_2a838c41(gpu, pbr->prog_sss, "uDir", 0.0f, 1.0f);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
}

void rig_pbr_bloom_pass__rig_variant_427e44ff(RigGPUCtx *gpu, RigPBRPipeline *pbr,
                        float threshold, float strength)
{
    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_bloom);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_bloom_down);
    if (pbr->fbo_hdr < gpu->n_fbos && gpu->fbos[pbr->fbo_hdr].color[0]) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpu->fbos[pbr->fbo_hdr].color[0]->gl_id);
    }
    rig_uniform_1f__rig_variant_139a6141(gpu, pbr->prog_bloom_down, "uThreshold", threshold);
    float tx = 1.0f/(float)pbr->width, ty = 1.0f/(float)pbr->height;
    rig_uniform_2f__rig_variant_2a838c41(gpu, pbr->prog_bloom_down, "uTexel", tx, ty);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);

    rig_fbo_bind__rig_variant_6606cf4a(gpu, pbr->fbo_hdr);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_bloom_up);
    if (pbr->fbo_bloom < gpu->n_fbos && gpu->fbos[pbr->fbo_bloom].color[0]) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpu->fbos[pbr->fbo_bloom].color[0]->gl_id);
    }
    rig_uniform_1f__rig_variant_139a6141(gpu, pbr->prog_bloom_up, "uStr", strength);
    rig_uniform_2f__rig_variant_2a838c41(gpu, pbr->prog_bloom_up, "uTexel", tx*2.0f, ty*2.0f);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
}

void rig_pbr_tonemap_pass__rig_variant_efa45d27(RigGPUCtx *gpu, RigPBRPipeline *pbr, float exposure) {
    rig_fbo_unbind__rig_variant_2f18cd10(gpu);
    rig_state_depth__rig_variant_0afda971(gpu, false, false, GL_ALWAYS);
    rig_program_bind__rig_variant_16192561(gpu, pbr->prog_tonemap);
    if (pbr->fbo_hdr < gpu->n_fbos && gpu->fbos[pbr->fbo_hdr].color[0]) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpu->fbos[pbr->fbo_hdr].color[0]->gl_id);
    }
    rig_uniform_1f__rig_variant_139a6141(gpu, pbr->prog_tonemap, "uExposure", exposure);
    rig_uniform_1f__rig_variant_139a6141(gpu, pbr->prog_tonemap, "uGamma",    2.2f);
    rig_uniform_1i__rig_variant_b6f38c1e(gpu, pbr->prog_tonemap, "uDoACES",   1);
    rig_draw_screen_quad__rig_variant_8c3d6ccd(gpu, pbr);
}

void rig_state_default__rig_variant_c2df75b5(RigGPUCtx *gpu) {
    glEnable(GL_DEPTH_TEST);  glDepthFunc(GL_LESS); glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);   glCullFace(GL_BACK);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    gpu->current_state.depth_test  = true;
    gpu->current_state.depth_write = true;
    gpu->current_state.blend       = false;
    gpu->current_state.cull_face   = true;
    gpu->stats.state_changes++;
}

void rig_state_depth__rig_variant_0afda971(RigGPUCtx *gpu, bool test, bool write, uint32_t func) {
    if (test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(write ? GL_TRUE : GL_FALSE);
    glDepthFunc(func ? func : GL_LESS);
    gpu->current_state.depth_test  = test;
    gpu->current_state.depth_write = write;
    gpu->stats.state_changes++;
}

void rig_state_blend__rig_variant_3d652f9d(RigGPUCtx *gpu, bool enable, uint32_t src, uint32_t dst) {
    if (enable) { glEnable(GL_BLEND); glBlendFunc(src ? src : GL_SRC_ALPHA, dst ? dst : GL_ONE_MINUS_SRC_ALPHA); }
    else glDisable(GL_BLEND);
    gpu->current_state.blend = enable;
    gpu->stats.state_changes++;
}

void rig_state_cull__rig_variant_d3fee156(RigGPUCtx *gpu, bool enable, uint32_t mode) {
    if (enable) { glEnable(GL_CULL_FACE); glCullFace(mode ? mode : GL_BACK); }
    else glDisable(GL_CULL_FACE);
    gpu->current_state.cull_face = enable;
    gpu->stats.state_changes++;
}

void rig_state_viewport__rig_variant_64044d00(RigGPUCtx *gpu, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glViewport((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
    gpu->current_state.viewport_x = x; gpu->current_state.viewport_y = y;
    gpu->current_state.viewport_w = w; gpu->current_state.viewport_h = h;
}

void rig_state_clear__rig_variant_f239e16c(RigGPUCtx *gpu, float r, float g, float b, float a, float depth) {
    (void)gpu;
    glClearColor(r, g, b, a);
    glClearDepthf(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void rig_draw_indexed__rig_variant_69255d7a(RigGPUCtx *gpu, RigVAO *vao, uint32_t offset, uint32_t count) {
    if (!vao) return;
    rig_vao_bind__rig_variant_e93c3290(gpu, vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_SHORT,
                   (const void*)(uintptr_t)(offset * sizeof(uint16_t)));
    gpu->stats.draw_calls++;
    gpu->stats.triangles += count / 3;
}

void rig_draw_arrays__rig_variant_ab61e1db(RigGPUCtx *gpu, uint32_t mode, uint32_t first, uint32_t count) {
    (void)gpu;
    glDrawArrays(mode, (GLint)first, (GLsizei)count);
    gpu->stats.draw_calls++;
}

void rig_draw_screen_quad__rig_variant_8c3d6ccd(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    (void)gpu;
    glBindVertexArray(pbr->vao_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    gpu->stats.draw_calls++;
}

const char *rig_glsl_src__rig_variant_62e6a98c(const char *name) {
    for (int i = 0; GLSL_TABLE[i].name; i++)
        if (strcmp(GLSL_TABLE[i].name, name) == 0)
            return GLSL_TABLE[i].src;
    return NULL;
}

void rb_gpu_apply_catenary_transform__rig_variant_978223e9(RBGpuContext *gpu, void* boxes_raw,
                                      uint32_t count, float scroll_y) {

    if (!gpu || !boxes_raw || count == 0) return;

    typedef struct {
        float x, y, w, h, scroll_x, scroll_y, content_w, content_h;
        void* node;
        float x_ndc, y_ndc, w_ndc, h_ndc, z_ndc;
    } BoxNDC;

    BoxNDC *boxes = (BoxNDC *)boxes_raw;
    float vp_w = (float)gpu->viewport_width;
    float vp_h = (float)gpu->viewport_height;

    static const float RIG_CAT_A     = 0.6180339887f;
    static const float RIG_CAT_DEPTH = 0.3819660113f;

    for (uint32_t i = 0; i < count; i++) {
        boxes[i].x_ndc = (boxes[i].x / vp_w) * 2.0f - 1.0f;
        boxes[i].y_ndc = 1.0f - ((boxes[i].y - scroll_y) / vp_h) * 2.0f;
        boxes[i].w_ndc = (boxes[i].w / vp_w) * 2.0f;
        boxes[i].h_ndc = (boxes[i].h / vp_h) * 2.0f;
        float cx_ndc = boxes[i].x_ndc + boxes[i].w_ndc * 0.5f;
        float cat = RIG_CAT_A * (coshf(cx_ndc / RIG_CAT_A) - 1.0f);
        boxes[i].z_ndc = -RIG_CAT_DEPTH * cat;
    }
}

static GLuint rb__compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            char *log = malloc((size_t)len);
            if (log) { glGetShaderInfoLog(shader, len, NULL, log);
                       fprintf(stderr, "[RigGPU] Shader error: %s\n", log);
                       free(log); }
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
bool rb_gpu_compile_holo_shaders__rig_variant_f8980503(RBGpuContext *gpu,
                                  const char* vert_src, const char* frag_src) {
    if (!gpu || !vert_src || !frag_src) return false;
    GLuint vs = rb__compile_shader(GL_VERTEX_SHADER,   vert_src);
    GLuint fs = rb__compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return false; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            char *log = malloc((size_t)len);
            if (log) { glGetProgramInfoLog(prog, len, NULL, log);
                       fprintf(stderr, "[RigGPU] Link error: %s\n", log);
                       free(log); }
        }
        glDeleteProgram(prog);
        return false;
    }

    GLuint ubo_idx = glGetUniformBlockIndex(prog, "RigidUBOData");
    if (ubo_idx != GL_INVALID_INDEX)
        glUniformBlockBinding(prog, ubo_idx, 0);

    gpu->shader_holo_ui = prog;
    return true;
}

bool rb_gpu_init_fbo_pipeline__rig_variant_928bdd1e(RBGpuContext *gpu, int w, int h) {
    if (!gpu) return false;
    gpu->viewport_width  = w;
    gpu->viewport_height = h;

    GLuint fbos[2], texs[2];
    glGenFramebuffers(2, fbos);
    glGenTextures(2, texs);

    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texs[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gpu->fbo_primary       = fbos[0];
    gpu->fbo_refraction    = fbos[1];
    gpu->tex_primary_color = texs[0];
    gpu->tex_refraction    = texs[1];
    return true;
}

void rb_gpu_swap_buffers__rig_variant_6b60ffe8(RBGpuContext *gpu) {
    if (!gpu) return;
    rig_gpu_present__rig_variant_21198354(gpu);
}
