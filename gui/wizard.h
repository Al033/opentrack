#pragma once

#include "opentrack-compat/options.hpp"
#include "opentrack-logic/main-settings.hpp"
#include "opentrack-logic/mappings.hpp"
#include "ui_trackhat-wizard.h"
#include <QObject>
#include <QWizard>

class Wizard : public QWizard
{
    Q_OBJECT
    Ui_wizard ui;
public:
    Wizard();

    enum Model { Cap = 0, ClipRight = 1, ClipLeft = 2 };
    enum { ClipRightX = 135, ClipLeftX = -135 };
private slots:
    void set_data();
};
