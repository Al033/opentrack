#include "ftnoir_tracker_rift_025.h"
#include "opentrack/plugin-api.hpp"

TrackerControls::TrackerControls()
{
    ui.setupUi( this );

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(doOK()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(doCancel()));

    tie_setting(s.constant_drift, ui.constantDrift);
    tie_setting(s.deadzone, ui.deadzone);
    tie_setting(s.persistence, ui.persistence);
    tie_setting(s.useYawSpring, ui.yawSpring);
}

void TrackerControls::doOK() {
    s.b->save();
    close();
}

void TrackerControls::doCancel() {
    close();
}

