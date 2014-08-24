#include "EyePiece.h"

#include <iomanip>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#if defined(OSMAND_TARGET_OS_windows)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <GL/glew.h>
#if defined(OSMAND_TARGET_OS_macosx)
#   include <OpenGL/gl.h>
#elif defined(OSMAND_TARGET_OS_windows)
#   include <GL/wglew.h>
#   include <GL/gl.h>
#elif defined(OSMAND_TARGET_OS_linux)
#   include <GL/glxew.h>
#   include <GL/gl.h>
#endif
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkBitmap.h>
#include <SkImageEncoder.h>
#include <OsmAndCore/restore_internal_warnings.h>

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
    {
        output << xT("Output image width can not be 0") << std::endl;
        return false;
    }
    if (configuration.outputImageHeight == 0)
    {
        output << xT("Output image width can not be 0") << std::endl;
        return false;
    }

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
        24,
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
#endif

    glewExperimental = GL_TRUE;
    glewInit();
    // For now, silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();

#if defined(OSMAND_TARGET_OS_windows)
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
    // Open default display (specified by DISPLAY env-var)
    const auto xDisplay = XOpenDisplay(nullptr);
    if (xDisplay == nullptr)
    {
        output << xT("Failed to open X display") << std::endl;
        return false;
    }

    // Get the default screen's GLX extension list
    const char* glxExtensionsString = glXQueryExtensionsString(xDisplay, XDefaultScreen(xDisplay));
    const auto glxExtensions = QString::fromLatin1(glxExtensionsString).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    output << xT("GLX extensions: ") << QStringToStlString(glxExtensions.join(' ')) << std::endl;

    // Query available framebuffer configurations
    const int framebufferConfigurationAttribs[] = {
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        None };
    int framebufferConfigurationsCount = 0;
    const auto framebufferConfigurations = glXChooseFBConfig(
        xDisplay,
        XDefaultScreen(xDisplay),
        framebufferConfigurationAttribs,
        &framebufferConfigurationsCount);
    if (framebufferConfigurationsCount == 0 || framebufferConfigurations == nullptr)
    {
        XCloseDisplay(xDisplay);

        output << xT("No valid framebuffer configurations available") << std::endl;
        return false;
    }

    // Check that needed API is present
    if (!glxExtensions.contains(QLatin1String("GLX_ARB_create_context")) ||
        !glxExtensions.contains(QLatin1String("GLX_ARB_create_context_profile")) ||
        glXCreateContextAttribsARB == nullptr ||
        glXMakeContextCurrent == nullptr)
    {
        XFree(framebufferConfigurations);
        XCloseDisplay(xDisplay);

        output << xT("GLX_ARB_create_context, GLX_ARB_create_context_profile and glXMakeContextCurrent have to be supported") << std::endl;
        return false;
    }

    // Create windowless context
    const int windowlessContextAttribs[] = { None };
    const auto windowlessContext = glXCreateContextAttribsARB(xDisplay, framebufferConfigurations[0], 0, True, windowlessContextAttribs);
    if (windowlessContext == nullptr)
    {
        XFree(framebufferConfigurations);
        XCloseDisplay(xDisplay);

        output << xT("Failed to create windowless context") << std::endl;
        return false;
    }

    // Create pbuffer
    const int pbufferAttribs[] = {
        GLX_PBUFFER_WIDTH, (int)configuration.outputImageWidth,
        GLX_PBUFFER_HEIGHT, (int)configuration.outputImageHeight,
        None
    };
    const auto pbuffer = glXCreatePbuffer(xDisplay, framebufferConfigurations[0], pbufferAttribs);
    if (!pbuffer)
    {
        glXDestroyContext(xDisplay, windowlessContext);
        XFree(framebufferConfigurations);
        XCloseDisplay(xDisplay);

        output << xT("Failed to create pbuffer") << std::endl;
        return false;
    }
    XFree(framebufferConfigurations);

    // Activate pbuffer and context
    if (!glXMakeContextCurrent(xDisplay, pbuffer, pbuffer, windowlessContext))
    {
        glXDestroyPbuffer(xDisplay, pbuffer);
        glXDestroyContext(xDisplay, windowlessContext);
        XCloseDisplay(xDisplay);

        output << xT("Failed to activate pbuffer and context") << std::endl;
        return false;
    }
#else
    output << xT("Operating system not supported") << std::endl;
    return false;
#endif

    // Get OpenGL version
    const auto glVersionString = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!glVerifyResult(output))
        return false;
    QRegExp glVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glVersionRegExp.indexIn(QString(QLatin1String(glVersionString)));
    const auto glVersion = glVersionRegExp.cap(1).toUInt() * 10 + glVersionRegExp.cap(2).toUInt();
    output << "OpenGL version " << glVersion << " [" << glVersionString << "]" << std::endl;

    //////////////////////////////////////////////////////////////////////////
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glVerifyResult(output);
    glClear(GL_COLOR_BUFFER_BIT);
    glVerifyResult(output);
    //////////////////////////////////////////////////////////////////////////

    // Read image from render-target
    SkBitmap outputBitmap;
    outputBitmap.setConfig(SkBitmap::kARGB_8888_Config, configuration.outputImageWidth, configuration.outputImageHeight);
    outputBitmap.allocPixels();
    glReadPixels(0, 0, configuration.outputImageWidth, configuration.outputImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, outputBitmap.getPixels());
    glVerifyResult(output);

    // Save bitmap to image (if required)
    if (!configuration.outputImageFilename.isEmpty())
    {
        std::unique_ptr<SkImageEncoder> imageEncoder;
        switch (configuration.outputImageFormat)
        {
            case ImageFormat::PNG:
                imageEncoder.reset(CreatePNGImageEncoder());
                break;

            case ImageFormat::JPEG:
                imageEncoder.reset(CreateJPEGImageEncoder());
                break;
        }
        imageEncoder->encodeFile(configuration.outputImageFilename.toLocal8Bit(), outputBitmap, 100);
    }

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
#endif

    return true;
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
    : outputImageWidth(0)
    , outputImageHeight(0)
    , outputImageFormat(ImageFormat::PNG)
{
}

bool OsmAndTools::EyePiece::Configuration::parseFromCommandLineArguments(
    const QStringList& commandLineArgs,
    Configuration& outConfiguration,
    QString& outError)
{
    outConfiguration = Configuration();

    for (const auto& arg : commandLineArgs)
    {
        if (arg.startsWith(QLatin1String("-outputImageWidth=")))
        {
            const auto value = arg.mid(strlen("-outputImageWidth="));
            bool ok = false;
            outConfiguration.outputImageWidth = value.toUInt(&ok);
            if (!ok)
            {
                outError = QString("'{0}' can not be parsed as output image width").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputImageHeight=")))
        {
            const auto value = arg.mid(strlen("-outputImageHeight="));
            bool ok = false;
            outConfiguration.outputImageHeight = value.toUInt(&ok);
            if (!ok)
            {
                outError = QString("'{0}' can not be parsed as output image height").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputImageFormat=")))
        {
            const auto value = arg.mid(strlen("-outputImageFormat="));
            if (value.compare(QLatin1String("png"), Qt::CaseInsensitive) == 0)
                outConfiguration.outputImageFormat = ImageFormat::PNG;
            else if (value.compare(QLatin1String("jpeg"), Qt::CaseInsensitive) == 0 || value.compare(QLatin1String("jpg"), Qt::CaseInsensitive) == 0)
                outConfiguration.outputImageFormat = ImageFormat::JPEG;
            else
            {
                outError = QString("'{0}' can not be parsed as output image format").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputImageFilename=")))
        {
            outConfiguration.outputImageFilename = arg.mid(strlen("-outputImageFilename="));
        }
    }

    //    bool wasObfRootSpecified = false;
    //
    //    for (const auto& arg : constOf(cmdLineArgs))
    //    {
    //        auto arg = *itArg;
    //        if (arg == "-verbose")
    //        {
    //            cfg.verbose = true;
    //        }
    //        else if (arg == "-dumpRules")
    //        {
    //            cfg.dumpRules = true;
    //        }
    //        else if (arg == "-map")
    //        {
    //            cfg.drawMap = true;
    //        }
    //        else if (arg == "-text")
    //        {
    //            cfg.drawText = true;
    //        }
    //        else if (arg == "-icons")
    //        {
    //            cfg.drawIcons = true;
    //        }
    //        else if (arg.startsWith("-stylesPath="))
    //        {
    //            auto path = arg.mid(strlen("-stylesPath="));
    //            QDir dir(path);
    //            if (!dir.exists())
    //            {
    //                error = "Style directory '" + path + "' does not exist";
    //                return false;
    //            }
    //
    //            Utilities::findFiles(dir, QStringList() << "*.render.xml", cfg.styleFiles);
    //        }
    //        else if (arg.startsWith("-style="))
    //        {
    //            cfg.styleName = arg.mid(strlen("-style="));
    //        }
    //        else if (arg.startsWith("-obfsDir="))
    //        {
    //            QDir obfRoot(arg.mid(strlen("-obfsDir=")));
    //            if (!obfRoot.exists())
    //            {
    //                error = "OBF directory does not exist";
    //                return false;
    //            }
    //            cfg.obfsDir = obfRoot;
    //            wasObfRootSpecified = true;
    //        }
    //        else if (arg.startsWith("-bbox="))
    //        {
    //            auto values = arg.mid(strlen("-bbox=")).split(",");
    //            cfg.bbox.left = values[0].toDouble();
    //            cfg.bbox.top = values[1].toDouble();
    //            cfg.bbox.right = values[2].toDouble();
    //            cfg.bbox.bottom = values[3].toDouble();
    //        }
    //        else if (arg.startsWith("-zoom="))
    //        {
    //            cfg.zoom = static_cast<ZoomLevel>(arg.mid(strlen("-zoom=")).toInt());
    //        }
    //        else if (arg.startsWith("-tileSide="))
    //        {
    //            cfg.tileSide = arg.mid(strlen("-tileSide=")).toInt();
    //        }
    //        else if (arg.startsWith("-density="))
    //        {
    //            cfg.densityFactor = arg.mid(strlen("-density=")).toFloat();
    //        }
    //        else if (arg == "-32bit")
    //        {
    //            cfg.is32bit = true;
    //        }
    //        else if (arg.startsWith("-output="))
    //        {
    //            cfg.output = arg.mid(strlen("-output="));
    //        }
    //    }
    //
    //    if (!cfg.drawMap && !cfg.drawText && !cfg.drawIcons)
    //    {
    //        cfg.drawMap = true;
    //        cfg.drawText = true;
    //        cfg.drawIcons = true;
    //    }
    //
    //    if (!wasObfRootSpecified)
    //        cfg.obfsDir = QDir::current();
    //*/
    //    return true;

    return true;
}
