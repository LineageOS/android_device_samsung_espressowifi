#include "egl.h"

EGLint eglGetError(void)
{
	return IMGeglGetError();
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	return IMGeglGetDisplay(display_id);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	return IMGeglInitialize(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	return IMGeglTerminate(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	return IMGeglQueryString(dpy, name);
}

void (* eglGetProcAddress(const char *procname))(void)
{
	return IMGeglGetProcAddress(procname);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return IMGeglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	/* When the following conditions are met...
	 * 1. EGL_PBUFFER_BIT is set in EGL_SURFACE_TYPE
	 * 2. EGL_OPENGL_ES2_BIT is set in EGL_RENDERABLE_TYPE
	 * ...EGL_RECORDABLE_ANDROID is forced to EGL_FALSE.
	 * Why this happens is unknown, but the end result is no configs get returned.
	 * Code meeting the above conditions seems to work fine without EGL_RECORDABLE_ANDROID,
	 * so in this shim we'll drop it on their behalf so they actually get an EGLConfig.
	 */
	bool renderable_type_es2 = false, surface_type_pbuffer = false;
	int i, j, attriblist_length = 0, recordable_attrib_pos = -1;

	while( attrib_list[attriblist_length++] != EGL_NONE )
	{
		if( attrib_list[attriblist_length-1] == EGL_SURFACE_TYPE && (attrib_list[attriblist_length] & EGL_PBUFFER_BIT) != 0 )
		{
			surface_type_pbuffer = true;
		}
		else if( attrib_list[attriblist_length-1] == EGL_RENDERABLE_TYPE && (attrib_list[attriblist_length] & EGL_OPENGL_ES2_BIT) != 0 )
		{
			renderable_type_es2 = true;
		}
		else if( attrib_list[attriblist_length-1] == EGL_RECORDABLE_ANDROID && attrib_list[attriblist_length] == EGL_TRUE )
		{
			recordable_attrib_pos = attriblist_length - 1;
		}
		++attriblist_length;
	}

	if( recordable_attrib_pos != -1 && surface_type_pbuffer && renderable_type_es2 )
	{
		EGLint override_attrib_list[attriblist_length-2];

		for( i = 0, j = 0; i < attriblist_length; ++i )
		{
			if( i != recordable_attrib_pos && i != recordable_attrib_pos + 1 )
			{
				override_attrib_list[j++] = attrib_list[i];
			}
		}

		return IMGeglChooseConfig(dpy, override_attrib_list, configs, config_size, num_config);
	}

	return IMGeglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	return IMGeglGetConfigAttrib(dpy, config, attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	return IMGeglCreateWindowSurface(dpy, config, win, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	return IMGeglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	return IMGeglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	return IMGeglDestroySurface(dpy, surface);
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	return IMGeglQuerySurface(dpy, surface, attribute, value);
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	return IMGeglCreateContext(dpy, config, share_context, attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	return IMGeglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return IMGeglMakeCurrent(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext(void)
{
	return IMGeglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
	return IMGeglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
	return IMGeglGetCurrentDisplay();
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	return IMGeglQueryContext(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL(void)
{
	return IMGeglWaitGL();
}

EGLBoolean eglWaitNative(EGLint engine)
{
	return IMGeglWaitNative(engine);
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
	return IMGeglSwapBuffers(dpy, draw);
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	return IMGeglCopyBuffers(dpy, surface, target);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	return IMGeglSwapInterval(dpy, interval);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
	return IMGeglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return IMGeglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return IMGeglReleaseTexImage(dpy, surface, buffer);
}

EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
	return IMGeglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}

EGLBoolean eglBindAPI(EGLenum api)
{
	return IMGeglBindAPI(api);
}

EGLenum eglQueryAPI(void)
{
	return IMGeglQueryAPI();
}

EGLBoolean eglWaitClient(void)
{
	return IMGeglWaitClient();
}

EGLBoolean eglReleaseThread(void)
{
	return IMGeglReleaseThread();
}
