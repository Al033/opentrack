#ifdef _WIN32
#   include <stdlib.h>
#endif

#include "opentrack/opencv-camera-dialog.hpp"
#include "wizard.h"
#include "ui.h"
#include "ui_install-driver-dialog.h"
#include "opentrack-compat/options.hpp"
using namespace options;
#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QStringList>
#include <QMessageBox>
#include <memory>
#include <cstring>

#ifdef _WIN32
// workaround QTBUG-38598, allow for launching from another directory
static void add_program_library_path()
{
    char* p = _pgmptr;
    char path[MAX_PATH+1];
    strcpy(path, p);
    char* ptr = strrchr(path, '\\');
    if (ptr)
    {
        *ptr = '\0';
        QCoreApplication::addLibraryPath(path);
    }
}
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    add_program_library_path();
#elif !defined(__linux)
    // workaround QTBUG-38598
    QCoreApplication::addLibraryPath(".");
#endif

#if defined(_WIN32) || defined(__APPLE__)
    // qt5 designer-made controls look like shit on 'doze -sh 20140921
    // also our OSX look leaves a lot to be desired -sh 20150726
    {
        const QStringList preferred { "fusion", "windowsvista", "macintosh", "windowsxp" };
        for (const auto& style_name : preferred)
        {
            QStyle* s = QStyleFactory::create(style_name);
            if (s)
            {
                QApplication::setStyle(s);
                break;
            }
        }
    }
#endif

    QApplication::setAttribute(Qt::AA_X11InitThreads, true);
    QApplication app(argc, argv);

    {
        QSettings s(OPENTRACK_ORG);
        if (!s.contains("wizard-run-once"))
        {
            s.setValue("wizard-run-once", true);
            auto w = std::make_shared<Wizard>();
            w->show();
            app.exec();
        }
    }

    if (get_camera_names().contains("PS3Eye Camera"))
    {
        MainWindow::set_working_directory();

        auto w = std::make_shared<MainWindow>();

        w->show();
        app.exec();
    }
    else
    {
        struct Dialog : QDialog
        {
            Ui::DriverDialog dlg;
            Dialog()
            {
                dlg.setupUi(this);
            }
        };
        Dialog().exec();
    }

    // on MSVC crashes in atexit
#ifdef _MSC_VER
    TerminateProcess(GetCurrentProcess(), 0);
#endif
    return 0;
}
