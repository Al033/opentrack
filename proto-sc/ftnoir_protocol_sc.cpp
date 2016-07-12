/* Homepage         http://facetracknoir.sourceforge.net/home/default.htm        *
 *                                                                               *
 * ISC License (ISC)                                                             *
 *                                                                               *
 * Copyright (c) 2015, Wim Vriend
 * Copyright (c) 2014, Stanislaw Halik <sthalik@misaki.pl>
 *                                                                               *
 * Permission to use, copy, modify, and/or distribute this software for any      *
 * purpose with or without fee is hereby granted, provided that the above        *
 * copyright notice and this permission notice appear in all copies.             *
 */
#include "ftnoir_protocol_sc.h"
#include "opentrack/plugin-api.hpp"

FTNoIR_Protocol::FTNoIR_Protocol() : should_stop(false), hSimConnect(nullptr)
{
}

FTNoIR_Protocol::~FTNoIR_Protocol()
{
    should_stop = true;
    wait();
}

void FTNoIR_Protocol::run()
{
    HANDLE event = CreateEvent(NULL, FALSE, FALSE, nullptr);

    if (event == nullptr)
    {
        qDebug() << "simconnect: event create" << GetLastError();
        return;
    }

    while (!should_stop)
    {
        if (SUCCEEDED(simconnect_open(&hSimConnect, "opentrack", NULL, 0, event, 0)))
        {
            simconnect_subscribetosystemevent(hSimConnect, 0, "Frame");

            while (!should_stop)
            {
                if (WaitForSingleObject(event, 10) == WAIT_OBJECT_0)
                {
                    if (FAILED(simconnect_calldispatch(hSimConnect, processNextSimconnectEvent, reinterpret_cast<void*>(this))))
                        break;
                }
            }

            (void) simconnect_close(hSimConnect);
        }

        if (!should_stop)
            Sleep(100);
    }

    CloseHandle(event);
}

void FTNoIR_Protocol::pose( const double *headpose ) {
    // euler degrees
    virtSCRotX = float(-headpose[Pitch]);
    virtSCRotY = float(headpose[Yaw]);
    virtSCRotZ = float(headpose[Roll]);

    // cm to meters
    virtSCPosX = float(headpose[TX]/100);
    virtSCPosY = float(headpose[TY]/100);
    virtSCPosZ = float(-headpose[TZ]/100);
}

#ifdef __GNUC__
#   pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

class ActivationContext {
public:
    ActivationContext(const int resid) : ok(false) {
        hactctx = INVALID_HANDLE_VALUE;
        actctx_cookie = 0;
        ACTCTXA actx = {0};
        actx.cbSize = sizeof(ACTCTXA);
        actx.lpResourceName = MAKEINTRESOURCEA(resid);
        actx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
#ifdef _MSC_VER
#	define PREFIX ""
#else
#	define PREFIX "lib"
#endif
        QString path = QCoreApplication::applicationDirPath() + "/" PREFIX "opentrack-proto-simconnect.dll";
        QByteArray name = QFile::encodeName(path);
        actx.lpSource = name.constData();
        hactctx = CreateActCtxA(&actx);
        actctx_cookie = 0;
        if (hactctx != INVALID_HANDLE_VALUE) {
            if (!ActivateActCtx(hactctx, &actctx_cookie)) {
                qDebug() << "SC: can't set win32 activation context" << GetLastError();
                ReleaseActCtx(hactctx);
                hactctx = INVALID_HANDLE_VALUE;
            }
            else
                ok = true;
        } else {
            qDebug() << "SC: can't create win32 activation context" << GetLastError();
        }
    }
    ~ActivationContext() {
        if (hactctx != INVALID_HANDLE_VALUE)
        {
            DeactivateActCtx(0, actctx_cookie);
            ReleaseActCtx(hactctx);
        }
    }
    bool is_ok() { return ok; }
private:
    ULONG_PTR actctx_cookie;
    HANDLE hactctx;
    bool ok;
};

bool FTNoIR_Protocol::correct()
{
    if (!SCClientLib.isLoaded())
    {
        ActivationContext ctx(142 + static_cast<int>(s.sxs_manifest));

        if (ctx.is_ok())
        {
            SCClientLib.setFileName("SimConnect.dll");
            if (!SCClientLib.load()) {
                qDebug() << "SC load" << SCClientLib.errorString();
                return false;
            }
        }
        else
            return false;
    }

    simconnect_open = (importSimConnect_Open) SCClientLib.resolve("SimConnect_Open");
    if (simconnect_open == NULL) {
        qDebug() << "FTNoIR_Protocol::correct() says: SimConnect_Open function not found in DLL!";
        return false;
    }
    simconnect_set6DOF = (importSimConnect_CameraSetRelative6DOF) SCClientLib.resolve("SimConnect_CameraSetRelative6DOF");
    if (simconnect_set6DOF == NULL) {
        qDebug() << "FTNoIR_Protocol::correct() says: SimConnect_CameraSetRelative6DOF function not found in DLL!";
        return false;
    }
    simconnect_close = (importSimConnect_Close) SCClientLib.resolve("SimConnect_Close");
    if (simconnect_close == NULL) {
        qDebug() << "FTNoIR_Protocol::correct() says: SimConnect_Close function not found in DLL!";
        return false;
    }

    simconnect_calldispatch = (importSimConnect_CallDispatch) SCClientLib.resolve("SimConnect_CallDispatch");
    if (simconnect_calldispatch == NULL) {
        qDebug() << "FTNoIR_Protocol::correct() says: SimConnect_CallDispatch function not found in DLL!";
        return false;
    }

    simconnect_subscribetosystemevent = (importSimConnect_SubscribeToSystemEvent) SCClientLib.resolve("SimConnect_SubscribeToSystemEvent");
    if (simconnect_subscribetosystemevent == NULL) {
        qDebug() << "FTNoIR_Protocol::correct() says: SimConnect_SubscribeToSystemEvent function not found in DLL!";
        return false;
    }

    start();

    return true;
}

void FTNoIR_Protocol::handle()
{
    (void) simconnect_set6DOF(hSimConnect, virtSCPosX, virtSCPosY, virtSCPosZ, virtSCRotX, virtSCRotZ, virtSCRotY);
}

void CALLBACK FTNoIR_Protocol::processNextSimconnectEvent(SIMCONNECT_RECV* pData, DWORD, void *self_)
{
    FTNoIR_Protocol& self = *reinterpret_cast<FTNoIR_Protocol*>(self_);

    switch(pData->dwID)
    {
    default:
        break;
    case SIMCONNECT_RECV_ID_EVENT_FRAME:
        self.handle();
        break;
    }
}

OPENTRACK_DECLARE_PROTOCOL(FTNoIR_Protocol, SCControls, FTNoIR_ProtocolDll)
