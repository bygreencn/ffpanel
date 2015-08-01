#include "tools.h"
#include <QProcess>

void GetDevice(QStringList &audiodev, QStringList &videodev, QList<QStringList> &videoopt)
{
    enum DevType {
        AUDIOTYPE,
        VIDEOTYPE,
        NONETYPE,
    } devtype = NONETYPE;

    QProcess ffmpegPro;
    QByteArray output;

    audiodev.clear();
    videodev.clear();
    videoopt.clear();
    ffmpegPro.start("ffmpeg -hide_banner -f dshow -list_devices 1 -i dummy");
    ffmpegPro.waitForFinished();
    output = ffmpegPro.readAllStandardError();
    QList<QByteArray> devList = output.split('\r');
    for (int i = 0; i < devList.size(); i++) {
        QByteArray &b = devList[i];
        if (b.contains("DirectShow video devices")) {
            devtype = VIDEOTYPE;
        } else if (b.contains("DirectShow audio devices")) {
            devtype = AUDIOTYPE;
        } else if (devtype == VIDEOTYPE && (b.contains("dshow")) && (b.count('\"') == 2)) {
            videodev.append(b.right(b.length() - b.indexOf('\"')));
        } else if (devtype == AUDIOTYPE && (b.contains("dshow")) && (b.count('\"') == 2)) {
            audiodev.append(b.right(b.length() - b.indexOf('\"')));
        }
    }
    for (int i = 0; i < videodev.size(); i++) {
        ffmpegPro.start("ffmpeg -hide_banner -f dshow -list_options 1 -i video=" + videodev[i]);
        ffmpegPro.waitForFinished();
        output = ffmpegPro.readAllStandardError();
        QList<QByteArray> opList = output.split('\n');
        QStringList opStrList;
        opStrList.clear();
        for (int j = 0; j < opList.size(); j++) {
            QByteArray &b = opList[j];
            int begin = b.lastIndexOf("max s=") + 6;
            int len = b.lastIndexOf(" fps=") - begin;
            if (b.contains("pixel_format") && len > 0 && begin > 0) {
                opStrList.insert(0, QString(b.mid(begin, len)));
            }
        }
        if (!opStrList.size()) {
            opStrList.append(QString("640x480"));
        }
        opStrList.removeDuplicates();
        videoopt.append(opStrList);

    }
}

