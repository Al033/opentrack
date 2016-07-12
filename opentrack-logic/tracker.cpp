/* Copyright (c) 2012-2015 Stanislaw Halik <sthalik@misaki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

/*
 * this file appeared originally in facetracknoir, was rewritten completely
 * following opentrack fork.
 *
 * originally written by Wim Vriend.
 */


#include "tracker.h"
#include <cmath>
#include <algorithm>

#if defined(_WIN32)
#   include <windows.h>
#endif

Tracker::Tracker(Mappings &m, SelectedLibraries &libs) :
    m(m),
    newpose {0,0,0, 0,0,0},
    centerp(s.center_at_startup),
    enabledp(true),
    zero_(false),
    should_quit(false),
    libs(libs),
    r_b(rmat::eye()),
    t_b {0,0,0}
{
}

Tracker::~Tracker()
{
    should_quit = true;
    wait();
}

double Tracker::map(double pos, Mapping& axis)
{
    bool altp = (pos < 0) && axis.opts.altp;
    axis.curve.setTrackingActive( !altp );
    axis.curveAlt.setTrackingActive( altp );
    auto& fc = altp ? axis.curveAlt : axis.curve;
    return fc.getValue(pos);
}

void Tracker::t_compensate(const rmat& rmat, const double* xyz, double* output, bool rz)
{
    // TY is really yaw axis. need swapping accordingly.
    dmat<3, 1> tvec( xyz[2], -xyz[0], -xyz[1] );
    const dmat<3, 1> ret = rmat * tvec;
    if (!rz)
        output[2] = ret(0);
    else
        output[2] = xyz[2];
    output[1] = -ret(2);
    output[0] = -ret(1);
}

#include "nan.hpp"

static inline double elide_nan(double value, double def)
{
    if (nanp(value))
    {
        if (nanp(def))
            return 0;
        return def;
    }
    return value;
}

static bool is_nan(const dmat<3,3>& r, const dmat<3, 1>& t)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (nanp(r(i, j)))
                return true;

    for (int i = 0; i < 3; i++)
        if (nanp(t(i)))
            return true;

    return false;
}

static bool is_nan(const Pose& value)
{
    for (int i = 0; i < 6; i++)
        if (nanp(value(i)))
            return true;
    return false;
}

void Tracker::logic()
{
    bool inverts[6] = {
        m(0).opts.invert,
        m(1).opts.invert,
        m(2).opts.invert,
        m(3).opts.invert,
        m(4).opts.invert,
        m(5).opts.invert,
    };

    static constexpr double pi = 3.141592653;
    static constexpr double r2d = 180. / pi;
    static constexpr double d2r = pi / 180.;

    using namespace euler;

    Pose value, raw;

    for (int i = 0; i < 6; i++)
    {
        auto& axis = m(i);
        int k = axis.opts.src;
        if (k < 0 || k >= 6)
            value(i) = 0;
        else
            value(i) = newpose[k];
        raw(i) = newpose[i];
    }

    if (is_nan(raw))
        raw = last_raw;

    const double off[] =
    {
        d2r * (double)-s.camera_yaw,
        d2r * (double)-s.camera_pitch,
        d2r * (double)-s.camera_roll
    };
    const rmat cam = euler_to_rmat(off);
    rmat r;
    {
        euler_t tmp(&value[Yaw]);
        tmp = d2r * tmp;
        r = euler_to_rmat(&tmp[0]);
    }
    euler_t t(value(0), value(1), value(2));

    r = cam * r;

    t_compensate(cam, t, t, true);

    bool can_center = false;
    const bool nan = is_nan(r, t);

    if (centerp && !nan)
    {
        for (int i = 0; i < 6; i++)
            if (fabs(newpose[i]) != 0)
            {
                can_center = true;
                break;
            }
    }

    if (can_center)
    {
        if (libs.pFilter)
            libs.pFilter->center();
        centerp = false;
        for (int i = 0; i < 3; i++)
            t_b[i] = t(i);
        r_b = r;
    }

    {
        double tmp[3] = { t(0) - t_b[0], t(1) - t_b[1], t(2) - t_b[2] };
        rmat m_;
        euler_t degs = rmat_to_euler(r_b.t() * r);
        degs(2) = rmat_to_euler(r * r_b.t())(2);
        m_ = euler_to_rmat(degs);

        const euler_t euler = r2d * rmat_to_euler(m_);

        for (int i = 0; i < 3; i++)
        {
            value(i) = tmp[i];
            value(i+3) = euler(i);
        }
    }

    bool nan_ = false;
    // whenever something can corrupt its internal state due to nan/inf, elide the call
    if (is_nan(value))
    {
        nan_ = true;
    }
    else
    {
        {
            Pose tmp = value;

            if (libs.pFilter)
                libs.pFilter->filter(tmp, value);
        }

        for (int i = 0; i < 6; i++)
            value(i) = map(value(i), m(i));

        if (s.tcomp_p)
        {
            euler_t value_(value(Yaw) * d2r,
                           value(Pitch) * d2r,
                           value(Roll) * d2r);
            t_compensate(euler_to_rmat(&value_[0]),
                         value,
                         value,
                         s.tcomp_tz);
        }

        for (int i = 0; i < 6; i++)
            value[i] *= inverts[i] ? -1. : 1.;

        if (zero_)
            for (int i = 0; i < 6; i++)
                value(i) = 0;

        if (is_nan(value))
            nan_ = true;
    }

    if (nan_)
    {
        value = last_mapped;

        // for widget last value display
        for (int i = 0; i < 6; i++)
            (void) map(value(i), m(i));
    }

    libs.pProtocol->pose(value);

    last_mapped = value;
    last_raw = raw;

    QMutexLocker foo(&mtx);
    output_pose = value;
    raw_6dof = raw;
}

void Tracker::run()
{
    const int sleep_ms = 3;

#if defined(_WIN32)
    (void) timeBeginPeriod(1);
#endif

    while (!should_quit)
    {
        t.start();

        double tmp[6] {0,0,0, 0,0,0};
        libs.pTracker->data(tmp);

        if (enabledp)
            for (int i = 0; i < 6; i++)
                newpose[i] = elide_nan(tmp[i], newpose[i]);

        logic();

        long q = long(sleep_ms * 1000L - t.elapsed()/1000L);
        using std::max;
        using ulong = unsigned long;
        usleep(ulong(max(1L, q)));
    }

    {
        // filter may inhibit exact origin
        Pose p;
        libs.pProtocol->pose(p);
    }

#if defined(_WIN32)
    (void) timeEndPeriod(1);
#endif

    for (int i = 0; i < 6; i++)
    {
        m(i).curve.setTrackingActive(false);
        m(i).curveAlt.setTrackingActive(false);
    }
}

void Tracker::get_raw_and_mapped_poses(double* mapped, double* raw) const {
    QMutexLocker foo(&const_cast<Tracker&>(*this).mtx);
    for (int i = 0; i < 6; i++)
    {
        raw[i] = raw_6dof(i);
        mapped[i] = output_pose(i);
    }
}

