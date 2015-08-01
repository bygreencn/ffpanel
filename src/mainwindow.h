#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Qtcore>
#include <QtGui>
#include "playpathdialog.h"



namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onReadFFotherOutput();
    void onReadFFmpegStderr();
    void onRunTimeout();
    void onChangeCmd();
    void onFFotherExit();
    void onStopStreaming();
    void onPPDiagClose();

    void on_streamBtn_released();

    void on_addrInBtn_clicked();
    void on_outUrl1Btn_clicked();
    void on_outUrl2Btn_clicked();
    void on_outUrl3Btn_clicked();

    void on_videoCombo_currentIndexChanged(int index);
    void on_isLoopComb_currentIndexChanged(int index);
    void on_micCheck_stateChanged(int arg1);
    void on_camCheck_stateChanged(int arg1);

    void on_playOutBtn_released();
    void on_playInputBtn_released();
    void on_playOutput1_released();
    void on_playOutput2_released();
    void on_playOutput3_released();

    void on_probeInputBtn_released();
    void on_probeOutput1_released();
    void on_probeOutput2_released();
    void on_probeOutput3_released();

    void on_urlOutLine1_textChanged(const QString &arg1);
    void on_urlOutLine2_textChanged(const QString &arg1);
    void on_urlOutLine3_textChanged(const QString &arg1);

    void on_devRdo_released();
    void on_urlRdo_released();
    void on_mixRdo_released();

private:
    Ui::MainWindow *ui;
    int         m_restartTimes;
    bool        m_isStreaming;
    bool        m_isRunLoop;
    bool        m_needRestart;
    bool        m_infoNeedClean;
    QList<QStringList> m_videoOptList;

    QString            m_cmdStr;
    time_t             m_startTime;
    QProcess           m_ffmpeg;
    QProcess           m_ffother;
    QPushButton       *m_focusBtn;
    QTimer            *m_runTimer;
    PlayPathDialog    *m_ppDialog;
    QSettings         *m_config;

    QString   getSourceCamName();
    QString   getInputPath();
    void      getCmd();
    void      stopStream();

    void      showEvent(QShowEvent *);
    QPoint    m_dragPosition;
    bool      m_moving;
    void      mousePressEvent(QMouseEvent *event);
    void      mouseMoveEvent(QMouseEvent *event);
    void      mouseReleaseEvent(QMouseEvent* event);
};

#endif // MAINWINDOW_H
