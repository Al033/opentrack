/* Copyright (c) 2013, 2015 Stanislaw Halik <sthalik@misaki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#pragma once

#include <QtGlobal>
#include <QWidget>
#include <QPixmap>
#include "opentrack/plugin-api.hpp"
#include "opentrack-logic/simple-mat.hpp"
#include "opentrack-compat/timer.hpp"

#ifdef BUILD_pose_widget
#   define POSE_WIDGET_EXPORT Q_DECL_EXPORT
#else
#   define POSE_WIDGET_EXPORT Q_DECL_IMPORT
#endif

class POSE_WIDGET_EXPORT GLWidget : public QWidget
{
public:
    using num = float;
    using vec2 = Mat<num, 2, 1>;
    using vec3 = Mat<num, 3, 1>;
    using rmat = Mat<num, 3, 3>;

    GLWidget(QWidget *parent);
    ~GLWidget();
    void rotateBy(double xAngle, double yAngle, double zAngle, double x, double y, double z);
    void rotateBy_real(double xAngle, double yAngle, double zAngle, double x, double y, double z);
protected:
    void paintEvent (QPaintEvent *event) override;
private:
    vec2 project(const vec3& point);
    vec3 project2(const vec3& point);
    void project_quad_texture();
    static inline vec3 normal(const vec3& p1, const vec3& p2, const vec3& p3);

    rmat rotation;
    vec3 translation;
    QImage front;
    QImage back;
    QImage image;
    Timer visible_timer;
    bool visible;
};
