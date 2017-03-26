#ifndef __PVREGL_H__
#define __PVREGL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef int32_t EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef int EGLNativeDisplayType;
typedef void *EGLNativeWindowType;
typedef void *EGLNativePixmapType;

EGLint IMGeglGetError(void);
EGLDisplay IMGeglGetDisplay(EGLNativeDisplayType display_id);
EGLBoolean IMGeglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
EGLBoolean IMGeglTerminate(EGLDisplay dpy);
const char * IMGeglQueryString(EGLDisplay dpy, EGLint name);
void (* IMGeglGetProcAddress(const char *procname))(void);
EGLBoolean IMGeglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean IMGeglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean IMGeglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
EGLSurface IMGeglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list);
EGLSurface IMGeglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
EGLSurface IMGeglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
EGLBoolean IMGeglDestroySurface(EGLDisplay dpy, EGLSurface surface);
EGLBoolean IMGeglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
EGLContext IMGeglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);
EGLBoolean IMGeglDestroyContext(EGLDisplay dpy, EGLContext ctx);
EGLBoolean IMGeglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLContext IMGeglGetCurrentContext(void);
EGLSurface IMGeglGetCurrentSurface(EGLint readdraw);
EGLDisplay IMGeglGetCurrentDisplay(void);
EGLBoolean IMGeglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
EGLBoolean IMGeglWaitGL(void);
EGLBoolean IMGeglWaitNative(EGLint engine);
EGLBoolean IMGeglSwapBuffers(EGLDisplay dpy, EGLSurface draw);
EGLBoolean IMGeglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
EGLBoolean IMGeglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
EGLBoolean IMGeglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean IMGeglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean IMGeglSwapInterval(EGLDisplay dpy, EGLint interval);
EGLSurface IMGeglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
EGLBoolean IMGeglBindAPI(EGLenum api);
EGLenum    IMGeglQueryAPI(void);
EGLBoolean IMGeglWaitClient(void);
EGLBoolean IMGeglReleaseThread(void);

#ifdef __cplusplus
}
#endif

#endif /* __PVREGL_H__ */
