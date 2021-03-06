#ifdef _WIN32

#include "dinput.hpp"
#include <QDebug>

std::atomic<int> dinput_handle::refcnt;
std::atomic_flag dinput_handle::init_lock = ATOMIC_FLAG_INIT;
dinput_handle::di_t dinput_handle::handle(dinput_handle::make_di());

LPDIRECTINPUT8& dinput_handle::init_di()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        qDebug() << "dinput: failed CoInitializeEx" << hr << GetLastError();

    static LPDIRECTINPUT8 di_ = nullptr;
    if (di_ == nullptr)
    {
        if (!SUCCEEDED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di_, NULL)))
        {
            di_ = nullptr;
        }
    }
    return di_;
}

dinput_handle::di_t dinput_handle::make_di()
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    LPDIRECTINPUT8& ret = init_di();

    init_lock.clear();

    return di_t(ret);
}

#endif

dinput_handle::di_t::di_t(LPDIRECTINPUT8& handle) : handle(handle)
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    refcnt++;

    init_lock.clear();
}

void dinput_handle::di_t::free_di()
{
    if (handle)
        handle->Release();
    handle = nullptr;
}

dinput_handle::di_t::~di_t()
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    int refcnt_ = refcnt.fetch_sub(1);

    if (refcnt_ == 1)
    {
        qDebug() << "exit: deleting di handle";
        free_di();
    }

    init_lock.clear();
}
