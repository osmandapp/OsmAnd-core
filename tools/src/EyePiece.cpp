#include "EyePiece.h"

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iomanip>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/Stopwatch.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/CoreResourcesEmbeddedBundle.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <OsmAndCore/Map/ObfMapObjectsProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapObjectsSymbolsProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Software.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#if defined(OSMAND_TARGET_OS_windows)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <GL/glew.h>
#if defined(OSMAND_TARGET_OS_macosx)
#   include <OpenGL/gl.h>
#   include <OpenGL/OpenGL.h>
#   include <OpenGL/CGLTypes.h>
#   include <OpenGL/CGLCurrent.h>
#elif defined(OSMAND_TARGET_OS_windows)
#   include <GL/wglew.h>
#   include <GL/gl.h>
#elif defined(OSMAND_TARGET_OS_linux)
#   include <GL/glxew.h>
#   include <GL/gl.h>
#   include <X11/Xlib.h>
#   include <X11/extensions/Xrender.h>
#endif
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkBitmap.h>
#include <SkImageEncoder.h>
#include <SkData.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCoreTools.h>
#include <OsmAndCoreTools/Utilities.h>

OsmAndTools::EyePiece::EyePiece(const Configuration& configuration_)
    : configuration(configuration_)
{
}

OsmAndTools::EyePiece::~EyePiece()
{
}

#if defined(_UNICODE) || defined(UNICODE)
bool OsmAndTools::EyePiece::glVerifyResult(std::wostream& output) const
#else
bool OsmAndTools::EyePiece::glVerifyResult(std::ostream& output) const
#endif
{
    const auto result = glGetError();
    if (result == GL_NO_ERROR)
        return true;

    const char* pcsErrorString = reinterpret_cast<const char*>(gluErrorString(result));
    output << "OpenGL error 0x" << std::hex << std::setfill(xT('0')) << std::setw(8) << result << std::dec << " : " << pcsErrorString << std::endl;
    return false;
}

#if defined(_UNICODE) || defined(UNICODE)
bool OsmAndTools::EyePiece::rasterize(std::wostream& output)
#else
bool OsmAndTools::EyePiece::rasterize(std::ostream& output)
#endif
{
    if (configuration.outputImageWidth == 0)
        return false;
    if (configuration.outputImageHeight == 0)
        return false;

    OsmAnd::Stopwatch rasterizationStopwatch(true);
#if defined(OSMAND_TARGET_OS_windows)
    // On windows, to create a windowless OpenGL context, a window is needed. Nonsense, totally.
    const auto hInstance = GetModuleHandle(NULL);

    WNDCLASS wndClass;
    memset(&wndClass, 0, sizeof(WNDCLASS));
    wndClass.style = CS_OWNDC;
    wndClass.lpfnWndProc = DefWindowProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = xT("osmand-eyepiece");
    if (!RegisterClass(&wndClass))
    {
        output << xT("Failed to register WNDCLASS for temporary window") << std::endl;
        return false;
    }

    // Adjust window size
    RECT windowRect = { 0, 1, 0, 1 };
    const DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    const DWORD windowExtendedStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    if (!AdjustWindowRectEx(&windowRect, windowStyle, FALSE, windowExtendedStyle))
    {
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to adjust temporary window size") << std::endl;
        return false;
    }

    // Create temporary window
    const auto hTempWindow = CreateWindowEx(
        windowExtendedStyle,
        wndClass.lpszClassName,
        wndClass.lpszClassName,
        windowStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL);
    if (!hTempWindow)
    {
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to create temporary window") << std::endl;
        return false;
    }
    const auto hTempWindowDC = GetDC(hTempWindow);
    if (!hTempWindowDC)
    {
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to get temporary window DC") << std::endl;
        return false;
    }

    // Get pixel format
    const PIXELFORMATDESCRIPTOR temporaryWindowPixelFormatDescriptior =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    const auto tempWindowPixelFormat = ChoosePixelFormat(hTempWindowDC, &temporaryWindowPixelFormatDescriptior);
    if (!tempWindowPixelFormat || !SetPixelFormat(hTempWindowDC, tempWindowPixelFormat, &temporaryWindowPixelFormatDescriptior))
    {
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to select proper pixel format for temporary window") << std::endl;
        return false;
    }
    
    // Create temporary OpenGL context
    const auto hTempGLRC = wglCreateContext(hTempWindowDC);
    if (!hTempGLRC)
    {
        glVerifyResult(output);

        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to create temporary OpenGL context") << std::endl;
        return false;
    }
    if (!wglMakeCurrent(hTempWindowDC, hTempGLRC))
    {
        glVerifyResult(output);

        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to activate temporary OpenGL context") << std::endl;
        return false;
    }

    // Initialize GLEW
    if (glewInit() != GLEW_NO_ERROR)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to initialize GLEW") << std::endl;
        return false;
    }
    // Silence OpenGL errors here, it's inside GLEW, so it's not ours
    (void)glGetError();

    // To find out what current system supports, use wglGetExtensionsString*
    const char* wglExtensionsString = nullptr;
    if (wglGetExtensionsStringEXT != nullptr)
    {
        wglExtensionsString = wglGetExtensionsStringEXT();
        if (!glVerifyResult(output))
            return false;
    }
    else if (wglGetExtensionsStringARB != nullptr)
    {
        wglExtensionsString = wglGetExtensionsStringARB(hTempWindowDC);
        if (!glVerifyResult(output))
            return false;
    }
    else
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("WGL_EXT_extensions_string or WGL_ARB_extensions_string has to be supported") << std::endl;
        return false;
    }
    const auto wglExtensions = QString::fromLatin1(wglExtensionsString).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (configuration.verbose)
        output << xT("WGL extensions: ") << QStringToStlString(wglExtensions.join(' ')) << std::endl;
    
    // Create pbuffer
    int pbufferContextAttribs[] = { 0 };
    void* pBufferHandle = nullptr;
    if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
        wglCreatePbufferARB != nullptr)
    {
        const auto hPBufferARB = wglCreatePbufferARB(
            hTempWindowDC,
            tempWindowPixelFormat,
            configuration.outputImageWidth,
            configuration.outputImageHeight,
            pbufferContextAttribs);
        pBufferHandle = hPBufferARB;
    }
    else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
        wglCreatePbufferEXT != nullptr)
    {
        const auto hPBufferEXT = wglCreatePbufferEXT(
            hTempWindowDC,
            tempWindowPixelFormat,
            configuration.outputImageWidth,
            configuration.outputImageHeight,
            pbufferContextAttribs);
        pBufferHandle = hPBufferEXT;
    }
    else
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("WGL_ARB_pbuffer or WGL_EXT_pbuffer has to be supported (wglCreatePbuffer*)") << std::endl;
        return false;
    }
    if (!pBufferHandle)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to create pbuffer") << std::endl;
        return false;
    }

    // Create OpenGL context
    HDC hPBufferDC = NULL;
    if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
        wglGetPbufferDCARB != nullptr &&
        wglReleasePbufferDCARB != nullptr)
    {
        hPBufferDC = wglGetPbufferDCARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
    }
    else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
        wglGetPbufferDCEXT != nullptr &&
        wglReleasePbufferDCEXT != nullptr)
    {
        hPBufferDC = wglGetPbufferDCEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
    }
    else
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("WGL_ARB_pbuffer or WGL_EXT_pbuffer has to be supported (wglGetPbufferDC*)") << std::endl;
        return false;
    }
    if (!hPBufferDC)
    {
        if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
            wglDestroyPbufferARB != nullptr)
        {
            wglDestroyPbufferARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
        }
        else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
            wglDestroyPbufferEXT != nullptr)
        {
            wglDestroyPbufferEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
        }

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to get pbuffer DC") << std::endl;
        return false;
    }

    // Create windowless OpenGL context
    if (!wglExtensions.contains(QLatin1String("WGL_ARB_create_context")) ||
        !wglExtensions.contains(QLatin1String("WGL_ARB_create_context_profile")) ||
        wglCreateContextAttribsARB == nullptr)
    {
        if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
            wglDestroyPbufferARB != nullptr)
        {
            wglReleasePbufferDCARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
        }
        else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
            wglDestroyPbufferEXT != nullptr)
        {
            wglReleasePbufferDCEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
        }

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("WGL_ARB_create_context, WGL_ARB_create_context_profile and WGL_ARB_make_current_read are required to perform windowless rendering") << std::endl;
        return false;
    }
    const auto hPBufferContext = wglCreateContextAttribsARB(hPBufferDC, NULL, pbufferContextAttribs);
    if (!hPBufferContext)
    {
        glVerifyResult(output);

        if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
            wglDestroyPbufferARB != nullptr)
        {
            wglReleasePbufferDCARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
        }
        else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
            wglDestroyPbufferEXT != nullptr)
        {
            wglReleasePbufferDCEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
        }

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hTempGLRC);
        ReleaseDC(hTempWindow, hTempWindowDC);
        DestroyWindow(hTempWindow);
        UnregisterClass(wndClass.lpszClassName, hInstance);

        output << xT("Failed to create windowless OpenGL context") << std::endl;
        return false;
    }

    // After windowless OpenGL context was create, window can be killed
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hTempGLRC);
    ReleaseDC(hTempWindow, hTempWindowDC);
    DestroyWindow(hTempWindow);
    UnregisterClass(wndClass.lpszClassName, hInstance);

    if (!wglMakeCurrent(hPBufferDC, hPBufferContext))
    {
        glVerifyResult(output);

        wglDeleteContext(hPBufferContext);
        if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
            wglDestroyPbufferARB != nullptr)
        {
            wglReleasePbufferDCARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
        }
        else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
            wglDestroyPbufferEXT != nullptr)
        {
            wglReleasePbufferDCEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle), hPBufferDC);
            wglDestroyPbufferEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
        }

        output << xT("Failed to activate windowless/framebufferless OpenGL context") << std::endl;
        return false;
    }
#elif defined(OSMAND_TARGET_OS_linux)
    // Open default display (specified by DISPLAY environment variable)
    const auto xDisplay = XOpenDisplay(nullptr);
    if (xDisplay == nullptr)
    {
        output << xT("Failed to open X display") << std::endl;
        return false;
    }

    // Get the default screen's GLX extension list
    const auto glxExtensionsString = glXQueryExtensionsString(xDisplay, DefaultScreen(xDisplay));
    const auto glxExtensions = QString::fromLatin1(glxExtensionsString).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (configuration.verbose)
        output << xT("GLX extensions: ") << QStringToStlString(glxExtensions.join(' ')) << std::endl;

    // GLX client extensions
    const auto glxClientExtensionsString = glXGetClientString(xDisplay, GLX_EXTENSIONS);
    const auto glxClientExtensions = QString::fromLatin1(glxClientExtensionsString).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (configuration.verbose)
        output << xT("GLX client extensions: ") << QStringToStlString(glxClientExtensions.join(' ')) << std::endl;

    // GLX server extensions
    const auto glxServerExtensionsString = glXQueryServerString(xDisplay, DefaultScreen(xDisplay), GLX_EXTENSIONS);
    const auto glxServerExtensions = QString::fromLatin1(glxServerExtensionsString).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (configuration.verbose)
        output << xT("GLX server extensions: ") << QStringToStlString(glxServerExtensions.join(' ')) << std::endl;

    // Check if X contains Render extension
    int xRenderEventBaseP = 0, xRenderErrorBaseP = 0;
    if (!XRenderQueryExtension(xDisplay, &xRenderEventBaseP, &xRenderErrorBaseP))
    {
        XCloseDisplay(xDisplay);

        output << xT("X server doesn't support Render extension") << std::endl;
        return false;
    }

    // Query available framebuffer configurations
    const auto p_glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((const GLubyte *)"glXChooseFBConfig");
    const auto p_glXChooseFBConfigSGIX = (PFNGLXCHOOSEFBCONFIGSGIXPROC)glXGetProcAddress((const GLubyte *)"glXChooseFBConfigSGIX");
    int framebufferConfigurationsCount = 0;
    GLXFBConfig* framebufferConfigurations = nullptr;
    const int framebufferConfigurationAttribs[] = {
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        None };
    if (p_glXChooseFBConfig != nullptr)
    {
        framebufferConfigurations = p_glXChooseFBConfig(
            xDisplay,
            DefaultScreen(xDisplay),
            framebufferConfigurationAttribs,
            &framebufferConfigurationsCount);
    }
    else if (glxExtensions.contains(QLatin1String("GLX_SGIX_fbconfig")) && p_glXChooseFBConfigSGIX != nullptr)
    {
        framebufferConfigurations = p_glXChooseFBConfigSGIX(
            xDisplay,
            DefaultScreen(xDisplay),
            framebufferConfigurationAttribs,
            &framebufferConfigurationsCount);
    }
    else
    {
        XCloseDisplay(xDisplay);

        output << xT("glXChooseFBConfig or glXChooseFBConfigSGIX/GLX_SGIX_fbconfig have to be supported") << std::endl;
        return false;
    }
    if (framebufferConfigurationsCount == 0 || framebufferConfigurations == nullptr)
    {
        XCloseDisplay(xDisplay);

        output << xT("No valid framebuffer configurations available") << std::endl;
        return false;
    }
    const auto framebufferConfiguration = framebufferConfigurations[0];
    XFree(framebufferConfigurations);

    // Check that needed API is present
    const auto p_glXCreatePbuffer = (PFNGLXCREATEPBUFFERPROC)glXGetProcAddress((const GLubyte *)"glXCreatePbuffer");
    const auto p_glXDestroyPbuffer = (PFNGLXDESTROYPBUFFERPROC)glXGetProcAddress((const GLubyte *)"glXDestroyPbuffer");
    if (p_glXCreatePbuffer == nullptr || p_glXDestroyPbuffer == nullptr)
    {
        XCloseDisplay(xDisplay);

        output << xT("glXCreatePbuffer and glXDestroyPbuffer have to be supported") << std::endl;
        return false;
    }

    // Create pbuffer
    const int pbufferAttribs[] = {
        GLX_PBUFFER_WIDTH, (int)configuration.outputImageWidth,
        GLX_PBUFFER_HEIGHT, (int)configuration.outputImageHeight,
        None
    };
    const auto pbuffer = p_glXCreatePbuffer(xDisplay, framebufferConfiguration, pbufferAttribs);
    if (!pbuffer)
    {
        XCloseDisplay(xDisplay);

        output << xT("Failed to create pbuffer") << std::endl;
        return false;
    }
    
    GLXContext windowlessContext = nullptr;
    if (configuration.useLegacyContext)
    {
        // Check that needed API is present
        const auto p_glXCreateNewContext = (PFNGLXCREATENEWCONTEXTPROC)glXGetProcAddress((const GLubyte *)"glXCreateNewContext");
        if (p_glXCreateNewContext == nullptr)
        {
            p_glXDestroyPbuffer(xDisplay, pbuffer);
            XCloseDisplay(xDisplay);

            output << xT("glXCreateNewContext has to be supported") << std::endl;
            return false;
        }

        // Create windowless context
        windowlessContext = p_glXCreateNewContext(xDisplay, framebufferConfiguration, GLX_RGBA_TYPE, 0, True);
    }
    else
    {
        // Check that needed API is present
        //NOTE: according to https://www.opengl.org/registry/specs/ARB/glx_create_context.txt if GLX_ARB_create_context is present,
        //NOTE: it's safe to use this method. But, on several particular configurations, glXCreateContextAttribsARB returns proper address
        //NOTE: and GLX_ARB_create_context is present, but actual call glXCreateContextAttribsARB crashes.
        //NOTE: Probably this means that GLX supports this extension, but underlying graphics driver has no clue about it.
        const auto p_glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");
        if (p_glXCreateContextAttribsARB == nullptr ||
            !glxClientExtensions.contains(QLatin1String("GLX_ARB_create_context")))
        {
            p_glXDestroyPbuffer(xDisplay, pbuffer);
            XCloseDisplay(xDisplay);

            output << xT("GLX_ARB_create_context/glXCreateContextAttribsARB has to be supported") << std::endl;
            return false;
        }

        // Create windowless context
        const int windowlessContextAttribs[] = {
            None };
        windowlessContext = p_glXCreateContextAttribsARB(xDisplay, framebufferConfiguration, nullptr, True, windowlessContextAttribs);
    }
    if (windowlessContext == nullptr)
    {
        p_glXDestroyPbuffer(xDisplay, pbuffer);
        XCloseDisplay(xDisplay);

        output << xT("Failed to create windowless context") << std::endl;
        return false;
    }

    // Activate pbuffer and context
    const auto p_glXMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)glXGetProcAddress((const GLubyte *)"glXMakeContextCurrent");
    if (!p_glXMakeContextCurrent || !p_glXMakeContextCurrent(xDisplay, pbuffer, pbuffer, windowlessContext))
    {
        p_glXDestroyPbuffer(xDisplay, pbuffer);
        glXDestroyContext(xDisplay, windowlessContext);
        XCloseDisplay(xDisplay);

        output << xT("Failed to activate pbuffer and context") << std::endl;
        return false;
    }

    // Initialize GLEW
    if (glewInit() != GLEW_NO_ERROR)
    {
        p_glXDestroyPbuffer(xDisplay, pbuffer);
        glXDestroyContext(xDisplay, windowlessContext);
        XCloseDisplay(xDisplay);

        output << xT("Failed to initialize GLEW") << std::endl;
        return false;
    }
    // Silence OpenGL errors here, it's inside GLEW, so it's not ours
    (void)glGetError();
#elif defined(OSMAND_TARGET_OS_macosx)
    bool coreProfile = true;

    // First select pixel format using attributes
    CGLPixelFormatObj pixelFormat;
    GLint matchingPixelFormats;
    const CGLPixelFormatAttribute pixelFormatAttrins[] = {
        kCGLPFAAccelerated,
        kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
        kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
        kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)(coreProfile ? kCGLOGLPVersion_3_2_Core : kCGLOGLPVersion_Legacy),
        (CGLPixelFormatAttribute)0
    };
    auto cglError = CGLChoosePixelFormat(pixelFormatAttrins, &pixelFormat, &matchingPixelFormats);
    if (cglError != kCGLNoError)
    {
        // Failed to find Core profile, fallback to any
        coreProfile = false;

        const CGLPixelFormatAttribute pixelFormatAttrins[] = {
            kCGLPFAAccelerated,
            kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
            kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
            kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
            kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)(coreProfile ? kCGLOGLPVersion_3_2_Core : kCGLOGLPVersion_Legacy),
            (CGLPixelFormatAttribute)0
        };
        cglError = CGLChoosePixelFormat(pixelFormatAttrins, &pixelFormat, &matchingPixelFormats);
        if (cglError != kCGLNoError)
        {
            output << xT("Failed find proper pixel format: ") << CGLErrorString(cglError) << std::endl;
            return false;
        }
    }
    if (configuration.verbose)
        output << xT("Using ") << (coreProfile ? xT("Core") : xT("Legacy") ) << xT(" OpenGL profile") << std::endl;

    // Create context
    CGLContextObj windowlessContext;
    cglError = CGLCreateContext(pixelFormat, NULL, &windowlessContext);
    if (cglError != kCGLNoError)
    {
        CGLDestroyPixelFormat(pixelFormat);

        output << xT("Failed to create windowless context: ") << CGLErrorString(cglError) << std::endl;
        return false;
    }
    CGLDestroyPixelFormat(pixelFormat);

    // Activate context
    cglError = CGLSetCurrentContext(windowlessContext);
    if (cglError != kCGLNoError)
    {
        CGLDestroyContext(windowlessContext);

        output << xT("Failed to activate windowless context: ") << CGLErrorString(cglError) << std::endl;
        return false;
    }

    // Initialize GLEW
    if (coreProfile)
        glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_NO_ERROR)
    {
        CGLSetCurrentContext(NULL);
        CGLDestroyContext(windowlessContext);

        output << xT("Failed to initialize GLEW") << std::endl;
        return false;
    }
    // Silence OpenGL errors here, it's inside GLEW, so it's not ours
    (void)glGetError();

    // Create PBuffer or Framebuffer, depending on profile
    CGLPBufferObj pbuffer;
    GLuint framebuffer;
    GLuint framebufferColorTexture;
    GLuint framebufferDepthRenderbuffer;
    if (coreProfile)
    {
        // Create color texture
        glGenTextures(1, &framebufferColorTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferColorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, configuration.outputImageWidth, configuration.outputImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (!glVerifyResult(output))
        {
            glDeleteTextures(1, &framebufferColorTexture);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to create color texture for framebuffer") << std::endl;
            return false;
        }

        // Create depth renderbuffer
        glGenRenderbuffers(1, &framebufferDepthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, framebufferDepthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, configuration.outputImageWidth, configuration.outputImageHeight);
        if (!glVerifyResult(output))
        {
            glDeleteTextures(1, &framebufferColorTexture);
            glDeleteRenderbuffers(1, &framebufferDepthRenderbuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to create color texture for framebuffer") << std::endl;
            return false;
        }

        // Create framebuffer
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebufferColorTexture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDepthRenderbuffer);
        if (!glVerifyResult(output))
        {
            glDeleteTextures(1, &framebufferColorTexture);
            glDeleteRenderbuffers(1, &framebufferDepthRenderbuffer);
            glDeleteFramebuffers(1, &framebuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to create framebuffer") << std::endl;
            return false;
        }

        // Set the list of draw buffers.
        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);
        if (!glVerifyResult(output))
        {
            glDeleteTextures(1, &framebufferColorTexture);
            glDeleteRenderbuffers(1, &framebufferDepthRenderbuffer);
            glDeleteFramebuffers(1, &framebuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to set draw-buffer") << std::endl;
            return false;
        }

        // Always check that our framebuffer is ok
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            glDeleteTextures(1, &framebufferColorTexture);
            glDeleteRenderbuffers(1, &framebufferDepthRenderbuffer);
            glDeleteFramebuffers(1, &framebuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Framebuffer incomlete") << std::endl;
            return false;
        }
    }
    else
    {
        cglError = CGLCreatePBuffer(configuration.outputImageWidth, configuration.outputImageHeight, GL_TEXTURE_2D, GL_RGBA, 0, &pbuffer);
        if (cglError != kCGLNoError)
        {
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to create PBuffer: ") << CGLErrorString(cglError) << std::endl;
            return false;
        }

        // Get virtual screen
        GLint virtualScreen = 0;
        cglError = CGLGetVirtualScreen(windowlessContext, &virtualScreen);
        if (cglError != kCGLNoError)
        {
            CGLReleasePBuffer(pbuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to get virtual screen: ") << CGLErrorString(cglError) << std::endl;
            return false;
        }

        // Attach PBuffer to context
        cglError = CGLSetPBuffer(windowlessContext, pbuffer, 0, 0, virtualScreen);
        if (cglError != kCGLNoError)
        {
            CGLReleasePBuffer(pbuffer);
            CGLSetCurrentContext(NULL);
            CGLDestroyContext(windowlessContext);

            output << xT("Failed to attach PBuffer: ") << CGLErrorString(cglError) << std::endl;
            return false;
        }
    }
#else
    output << xT("Operating system not supported") << std::endl;
    return false;
#endif
    if (configuration.verbose)
        output << xT("Prepared OpenGL context") << std::endl;

    bool success = true;
    for (;;)
    {
        // Find style
        if (configuration.verbose)
            output << xT("Resolving style '") << QStringToStlString(configuration.styleName) << xT("'...") << std::endl;
        const auto mapStyle = configuration.stylesCollection->getResolvedStyleByName(configuration.styleName);
        if (!mapStyle)
        {
            if (configuration.verbose)
            {
                output << "Failed to resolve style '" << QStringToStlString(configuration.styleName) << "' from collection:" << std::endl;
                for (const auto& style : configuration.stylesCollection->getCollection())
                {
                    if (style->isMetadataLoaded())
                    {
                        if (style->isStandalone())
                            output << "\t" << QStringToStlString(style->name) << std::endl;
                        else
                            output << "\t" << QStringToStlString(style->name) << "::" << QStringToStlString(style->parentName) << std::endl;
                    }
                    else
                        output << "\t[missing metadata]" << std::endl;
                }
            }
            else
            {
                output << "Failed to resolve style '" << QStringToStlString(configuration.styleName) << "' from collection:" << std::endl;
            }

            success = false;
            break;
        }

        // Prepare all resources for renderer
        if (configuration.verbose)
        {
            output
                << xT("Initializing map presentation environment with display density ")
                << configuration.displayDensityFactor
                << xT(", map scale ")
                << configuration.mapScale
                << xT(", symbols scale ")
                << configuration.symbolsScale
                << xT(" and locale '")
                << QStringToStlString(configuration.locale)
                << xT("'...") << std::endl;
        }
        const std::shared_ptr<OsmAnd::MapPresentationEnvironment> mapPresentationEnvironment(new OsmAnd::MapPresentationEnvironment(
            mapStyle,
            configuration.displayDensityFactor,
            configuration.mapScale,
            configuration.symbolsScale,
            configuration.locale));

        if (configuration.verbose)
            output << xT("Applying extra style settings to map presentation environment...") << std::endl;
        mapPresentationEnvironment->setSettings(configuration.styleSettings);

        if (configuration.verbose)
            output << xT("Creating primitiviser...") << std::endl;
        const std::shared_ptr<OsmAnd::MapPrimitiviser> primitiviser(new OsmAnd::MapPrimitiviser(
            mapPresentationEnvironment));

        if (configuration.verbose)
            output << xT("Creating binary map objects provider...") << std::endl;
        const std::shared_ptr<OsmAnd::ObfMapObjectsProvider> binaryMapDataProvider(new OsmAnd::ObfMapObjectsProvider(
            configuration.obfsCollection));

        if (configuration.verbose)
            output << xT("Creating map primitives provider...") << std::endl;
        const std::shared_ptr<OsmAnd::MapPrimitivesProvider> binaryMapPrimitivesProvider(new OsmAnd::MapPrimitivesProvider(
            binaryMapDataProvider,
            primitiviser,
            configuration.referenceTileSize));

        if (configuration.verbose)
            output << xT("Creating map objects symbols provider...") << std::endl;
        const std::shared_ptr<OsmAnd::MapObjectsSymbolsProvider> binaryMapStaticSymbolProvider(new OsmAnd::MapObjectsSymbolsProvider(
            binaryMapPrimitivesProvider,
            configuration.referenceTileSize));

        if (configuration.verbose)
            output << xT("Creating map raster layer provider...") << std::endl;
        const std::shared_ptr<OsmAnd::MapRasterLayerProvider_Software> binaryMapRasterTileProvider(new OsmAnd::MapRasterLayerProvider_Software(
            binaryMapPrimitivesProvider));
        
        // Create renderer
        if (configuration.verbose)
            output << xT("Creating OpenGL 2.0+ atlas map renderer...") << std::endl;
        const auto mapRenderer = OsmAnd::createMapRenderer(OsmAnd::MapRendererClass::AtlasMapRenderer_OpenGL2plus);
        if (!mapRenderer)
        {
            output << xT("No supported OsmAnd renderer found") << std::endl;

            success = false;
            break;
        }

        // Setup renderer
        if (configuration.verbose)
            output << xT("Setting-up map renderer...") << std::endl;
        OsmAnd::MapRendererSetupOptions mapRendererSetupOptions;
        mapRendererSetupOptions.gpuWorkerThreadEnabled = false;
        if (!mapRenderer->setup(mapRendererSetupOptions))
        {
            output << xT("Failed to setup OsmAnd map renderer") << std::endl;

            success = false;
            break;
        }

        // Apply configuration to map renderer
        if (configuration.verbose)
            output << xT("Setting map renderer configuration and state...") << std::endl;
        const auto mapRendererConfiguration = std::static_pointer_cast<OsmAnd::AtlasMapRendererConfiguration>(mapRenderer->getConfiguration());
        mapRendererConfiguration->referenceTileSizeOnScreenInPixels = configuration.referenceTileSize;
        mapRenderer->setConfiguration(mapRendererConfiguration);
        mapRenderer->setTarget(configuration.target31);
        mapRenderer->setZoom(configuration.zoom);
        mapRenderer->setAzimuth(configuration.azimuth);
        mapRenderer->setElevationAngle(configuration.elevationAngle);
        mapRenderer->setWindowSize(OsmAnd::PointI(configuration.outputImageWidth, configuration.outputImageHeight));
        mapRenderer->setViewport(OsmAnd::AreaI(0, 0, configuration.outputImageHeight, configuration.outputImageWidth));
        mapRenderer->setFieldOfView(configuration.fov);

        // Add providers
        if (configuration.verbose)
            output << xT("Adding providers to map renderer...") << std::endl;
        mapRenderer->addSymbolsProvider(binaryMapStaticSymbolProvider);
        mapRenderer->setMapLayerProvider(0, binaryMapRasterTileProvider);
        
        // Initialize rendering
        if (configuration.verbose)
            output << xT("Initializing rendering...") << std::endl;
        if(!mapRenderer->initializeRendering())
        {
            output << xT("Failed to initialize rendering") << std::endl;

            success = false;
            break;
        }

        // Repeat processing and rendering until everything is complete
        if (configuration.verbose)
            output << xT("Rendering frames...") << std::endl;
        OsmAnd::Stopwatch renderingStopwatch(true);
        auto framesCounter = 0u;
        bool wasInterrupted = false;
        for (;; framesCounter++)
        {
            // Update must be performed before each frame
            if (!mapRenderer->update())
                output << xT("Map renderer: update failed") << std::endl;

            // If frame was prepared, it means there's something to render
            if (mapRenderer->prepareFrame())
            {
                const auto ok = mapRenderer->renderFrame();
                glVerifyResult(output);

                if (!ok)
                    output << xT("Map renderer: frame rendering failed") << std::endl;
            }

            // Send everything to GPU
            glFlush();
            glVerifyResult(output);

            // Check if map renderer finished processing
            if (mapRenderer->isIdle())
                break;

            if (renderingStopwatch.elapsed() > (10 * 60 /* 10 minutes */))
            {
                wasInterrupted = true;
                break;
            }
        }
        const auto timeElapsedOnRendering = renderingStopwatch.elapsed();
        if (configuration.verbose)
            output << xT("Rendered ") << framesCounter << xT(" frames in ") << timeElapsedOnRendering << xT("s") << std::endl;

        if (wasInterrupted)
        {
            output
                << xT("ERROR: Rendering was interrupted since it took too long (more than 10 minutes). Probably it's stuck: ")
                << QStringToStlString(mapRenderer->getNotIdleReason())
                << std::endl;
            success = false;
        }

        // Wait until everything is ready on GPU
        if (configuration.verbose)
            output << xT("Waiting for GPU to complete all stuff requested...") << std::endl;
        glFinish();
        glVerifyResult(output);

        // Release rendering
        if (configuration.verbose)
            output << xT("Releasing rendering...") << std::endl;
        if (!mapRenderer->releaseRendering())
        {
            output << xT("Failed to release rendering") << std::endl;

            success = false;
            break;
        }

        break;
    }

    // Read image from render-target
    if (configuration.verbose)
        output << xT("Reading result image from GPU...") << std::endl;
    SkBitmap outputBitmap;
    outputBitmap.allocPixels(SkImageInfo::MakeN32Premul(configuration.outputImageWidth, configuration.outputImageHeight));
    glReadPixels(0, 0, configuration.outputImageWidth, configuration.outputImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, outputBitmap.getPixels());
    glVerifyResult(output);

    // Flip image vertically
    SkBitmap filledOutputBitmap;
    filledOutputBitmap.allocPixels(SkImageInfo::MakeN32Premul(configuration.outputImageWidth, configuration.outputImageHeight));
    const auto rowSizeInBytes = outputBitmap.rowBytes();
    for (int row = 0; row < configuration.outputImageHeight; row++)
    {
        const auto pSrcRow = reinterpret_cast<uint8_t*>(outputBitmap.getPixels()) + (row * rowSizeInBytes);
        const auto pDstRow = reinterpret_cast<uint8_t*>(filledOutputBitmap.getPixels()) + ((configuration.outputImageHeight - row - 1) * rowSizeInBytes);
        memcpy(pDstRow, pSrcRow, rowSizeInBytes);
    }

    // Save bitmap to image (if required)
    // if (!configuration.outputImageFilename.isEmpty())
    // {
    //     if (configuration.verbose)
    //         output << xT("Saving image to '") << QStringToStlString(configuration.outputImageFilename) << xT("'...") << std::endl;

    //     std::unique_ptr<SkImageEncoder> imageEncoder;
    //     switch (configuration.outputImageFormat)
    //     {
    //         case ImageFormat::PNG:
    //             imageEncoder.reset(CreatePNGImageEncoder());
    //             break;

    //         case ImageFormat::JPEG:
    //             imageEncoder.reset(CreateJPEGImageEncoder());
    //             break;
    //     }

    //     const auto imageData = imageEncoder->encodeData(filledOutputBitmap, 100);
    //     if (!imageData)
    //     {
    //         output << xT("Failed to encode image") << std::endl;
    //         success = false;
    //     }
    //     else
    //     {
    //         QFile imageFile(configuration.outputImageFilename);

    //         // Just in case try to create entire path
    //         auto containingDirectory = QFileInfo(imageFile).absoluteDir();
    //         containingDirectory.mkpath(QLatin1String("."));
    //         if (!QFileInfo(containingDirectory.absolutePath()).isWritable())
    //         {
    //             output << xT("'") << QStringToStlString(containingDirectory.absolutePath()) << xT("' is not writable") << std::endl;
    //             success = false;
    //         }
    //         else
    //         {
    //             if (!imageFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    //             {
    //                 output << xT("Failed to open destination file '") << QStringToStlString(configuration.outputImageFilename) << xT("'") << std::endl;
    //                 success = false;
    //             }
    //             else
    //             {
    //                 if (imageFile.write(reinterpret_cast<const char*>(imageData->bytes()), imageData->size()) != imageData->size())
    //                 {
    //                     output << xT("Failed to write image to '") << QStringToStlString(configuration.outputImageFilename) << xT("'") << std::endl;
    //                     success = false;
    //                 }
    //                 imageFile.close();
    //             }
    //         }

    //         imageData->unref();
    //     }
    // }

    if (configuration.verbose)
        output << xT("Going to release OpenGL context...") << std::endl;
#if defined(OSMAND_TARGET_OS_windows)
    // Finally release the PBuffer
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hPBufferContext);
    if (wglExtensions.contains(QLatin1String("WGL_ARB_pbuffer")) &&
        wglDestroyPbufferARB != nullptr)
    {
        wglReleasePbufferDCARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle), hPBufferDC);
        wglDestroyPbufferARB(reinterpret_cast<HPBUFFERARB>(pBufferHandle));
    }
    else if (wglExtensions.contains(QLatin1String("WGL_EXT_pbuffer")) &&
        wglDestroyPbufferEXT != nullptr)
    {
        wglReleasePbufferDCEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle), hPBufferDC);
        wglDestroyPbufferEXT(reinterpret_cast<HPBUFFEREXT>(pBufferHandle));
    }
#elif defined(OSMAND_TARGET_OS_linux)
    glXDestroyPbuffer(xDisplay, pbuffer);
    glXDestroyContext(xDisplay, windowlessContext);
    XCloseDisplay(xDisplay);
#elif defined(OSMAND_TARGET_OS_macosx)
    if (coreProfile)
    {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &framebufferColorTexture);
        glDeleteRenderbuffers(1, &framebufferDepthRenderbuffer);
    }
    else
    {
        CGLReleasePBuffer(pbuffer);
    }
    CGLSetCurrentContext(NULL);
    CGLDestroyContext(windowlessContext);
#endif
    if (configuration.verbose)
        output << xT("Rasterization took ") << rasterizationStopwatch.elapsed() << xT("s") << std::endl;

    return success;
}

bool OsmAndTools::EyePiece::rasterize(QString *pLog /*= nullptr*/)
{
    if (pLog != nullptr)
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wostringstream output;
        const bool success = rasterize(output);
        *pLog = QString::fromStdWString(output.str());
        return success;
#else
        std::ostringstream output;
        const bool success = rasterize(output);
        *pLog = QString::fromStdString(output.str());
        return success;
#endif
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return rasterize(std::wcout);
#else
        return rasterize(std::cout);
#endif
    }
}

OsmAndTools::EyePiece::Configuration::Configuration()
    : styleName(QLatin1String("default"))
    , outputImageWidth(0)
    , outputImageHeight(0)
    , outputImageFormat(ImageFormat::PNG)
    , zoom(15.0f)
    , azimuth(0.0f)
    , elevationAngle(90.0f)
    , fov(16.5f)
    , referenceTileSize(256)
    , displayDensityFactor(1.0f)
    , mapScale(1.0f)
    , symbolsScale(1.0f)
    , locale(QLatin1String("en"))
    , verbose(false)
#if defined(OSMAND_TARGET_OS_linux)
    , useLegacyContext(false)
#endif
{
}

bool OsmAndTools::EyePiece::Configuration::parseFromCommandLineArguments(
    const QStringList& commandLineArgs,
    Configuration& outConfiguration,
    QString& outError)
{
    outConfiguration = Configuration();

    const std::shared_ptr<OsmAnd::ObfsCollection> obfsCollection(new OsmAnd::ObfsCollection());
    outConfiguration.obfsCollection = obfsCollection;

    const std::shared_ptr<OsmAnd::MapStylesCollection> stylesCollection(new OsmAnd::MapStylesCollection());
    outConfiguration.stylesCollection = stylesCollection;

    for (const auto& arg : commandLineArgs)
    {
        if (arg.startsWith(QLatin1String("-obfsPath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfsPath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            obfsCollection->addDirectory(value, false);
        }
        else if (arg.startsWith(QLatin1String("-obfsRecursivePath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfsRecursivePath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            obfsCollection->addDirectory(value, true);
        }
        else if (arg.startsWith(QLatin1String("-obfFile=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfFile=")));
            if (!QFile(value).exists())
            {
                outError = QString("'%1' file does not exist").arg(value);
                return false;
            }

            obfsCollection->addFile(value);
        }
        else if (arg.startsWith(QLatin1String("-stylesPath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-stylesPath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            QFileInfoList styleFilesList;
            OsmAnd::Utilities::findFiles(QDir(value), QStringList() << QLatin1String("*.render.xml"), styleFilesList, false);
            for (const auto& styleFile : styleFilesList)
                stylesCollection->addStyleFromFile(styleFile.absoluteFilePath());
        }
        else if (arg.startsWith(QLatin1String("-stylesRecursivePath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-stylesRecursivePath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            QFileInfoList styleFilesList;
            OsmAnd::Utilities::findFiles(QDir(value), QStringList() << QLatin1String("*.render.xml"), styleFilesList, true);
            for (const auto& styleFile : styleFilesList)
                stylesCollection->addStyleFromFile(styleFile.absoluteFilePath());
        }
        else if (arg.startsWith(QLatin1String("-styleName=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-styleName=")));
            outConfiguration.styleName = value;
        }
        else if (arg.startsWith(QLatin1String("-styleSetting:")))
        {
            const auto settingValue = arg.mid(strlen("-styleSetting:"));
            const auto settingKeyValue = settingValue.split(QLatin1Char('='));
            if (settingKeyValue.size() != 2)
            {
                outError = QString("'%1' can not be parsed as style settings key and value").arg(settingValue);
                return false;
            }

            outConfiguration.styleSettings[settingKeyValue[0]] = Utilities::purifyArgumentValue(settingKeyValue[1]);
        }
        else if (arg.startsWith(QLatin1String("-outputImageWidth=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-outputImageWidth=")));
            bool ok = false;
            outConfiguration.outputImageWidth = value.toUInt(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as output image width").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputImageHeight=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-outputImageHeight=")));
            bool ok = false;
            outConfiguration.outputImageHeight = value.toUInt(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as output image height").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputImageFilename=")))
        {
            outConfiguration.outputImageFilename = Utilities::resolvePath(arg.mid(strlen("-outputImageFilename=")));
        }
        else if (arg.startsWith(QLatin1String("-outputImageFormat=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-outputImageFormat=")));
            if (value.compare(QLatin1String("png"), Qt::CaseInsensitive) == 0)
                outConfiguration.outputImageFormat = ImageFormat::PNG;
            else if (value.compare(QLatin1String("jpeg"), Qt::CaseInsensitive) == 0 || value.compare(QLatin1String("jpg"), Qt::CaseInsensitive) == 0)
                outConfiguration.outputImageFormat = ImageFormat::JPEG;
            else
            {
                outError = QString("'%1' can not be parsed as output image format").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-latLon=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-latLon=")));
            const auto latLonValues = value.split(QLatin1Char(';'));
            if (latLonValues.size() != 2)
            {
                outError = QString("'%1' can not be parsed as latitude and longitude").arg(value);
                return false;
            }

            OsmAnd::LatLon latLon;
            bool ok = false;
            latLon.latitude = latLonValues[0].toDouble(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as latitude").arg(latLonValues[0]);
                return false;
            }

            ok = false;
            latLon.longitude = latLonValues[1].toDouble(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as longitude").arg(latLonValues[1]);
                return false;
            }

            outConfiguration.target31 = OsmAnd::Utilities::convertLatLonTo31(latLon);
        }
        else if (arg.startsWith(QLatin1String("-target31=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-target31=")));
            const auto target31Values = value.split(QLatin1Char(';'));
            if (target31Values.size() != 2)
            {
                outError = QString("'%1' can not be parsed as target31 point").arg(value);
                return false;
            }

            bool ok = false;
            outConfiguration.target31.x = target31Values[0].toInt(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as target31.x").arg(target31Values[0]);
                return false;
            }

            ok = false;
            outConfiguration.target31.y = target31Values[1].toInt(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as target31.y").arg(target31Values[1]);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-zoom=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-zoom=")));

            bool ok = false;
            outConfiguration.zoom = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as zoom").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-azimuth=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-azimuth=")));

            bool ok = false;
            outConfiguration.azimuth = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as azimuth").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-elevationAngle=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-elevationAngle=")));

            bool ok = false;
            outConfiguration.elevationAngle = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as elevation angle").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-fov=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-fov=")));

            bool ok = false;
            outConfiguration.fov = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as field-of-view angle").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-referenceTileSize=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-referenceTileSize=")));

            bool ok = false;
            outConfiguration.referenceTileSize = value.toUInt(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as reference tile size in pixels").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-displayDensityFactor=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-displayDensityFactor=")));

            bool ok = false;
            outConfiguration.displayDensityFactor = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as display density factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-mapScale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-mapScale=")));

            bool ok = false;
            outConfiguration.mapScale = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as map scale factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-symbolsScale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-symbolsScale=")));

            bool ok = false;
            outConfiguration.symbolsScale = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as symbols scale factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-locale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-locale=")));

            outConfiguration.locale = value;
        }
        else if (arg == QLatin1String("-verbose"))
        {
            outConfiguration.verbose = true;
        }
#if defined(OSMAND_TARGET_OS_linux)
        else if (arg == QLatin1String("-useLegacyContext"))
        {
            outConfiguration.useLegacyContext = true;
        }
#endif
        else
        {
            outError = QString("Unrecognized argument: '%1'").arg(arg);
            return false;
        }
    }

    // Validate
    if (outConfiguration.styleName.isEmpty())
    {
        outError = QLatin1String("'styleName' can not be empty");
        return false;
    }
    if (outConfiguration.outputImageWidth == 0)
    {
        outError = QLatin1String("'outputImageWidth' can not be 0");
        return false;
    }
    if (outConfiguration.outputImageHeight == 0)
    {
        outError = QLatin1String("'outputImageHeight' can not be 0");
        return false;
    }

    return true;
}
