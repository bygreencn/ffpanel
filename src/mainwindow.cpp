#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <time.h>
#include "tools.h"

QString probeCmdPrefix = QString("ffprobe -hide_banner -show_streams -probesize 500000 -analyzeduration 500000 ");
QString playCmdPrefix = QString("ffplay -hide_banner -x 640 -y 360 ");
QString audioParam = QString(" -sample_rate 44100 -sample_size 16 -channels 2 ");

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QStringList videoList, audioList;
    ui->setupUi(this);
    setFixedSize(800, 468);
    m_restartTimes = 0;
    m_needRestart = false;
    m_infoNeedClean = false;
    m_isRunLoop = false;
    m_isStreaming = false;
    m_focusBtn = NULL;
    m_startTime = 0;
    m_runTimer = new QTimer(this);
    m_ppDialog = new PlayPathDialog(this);
    m_config = new QSettings("ffpanel.ini", QSettings::IniFormat);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    connect(ui->minBtn, SIGNAL(released()), this, SLOT(showMinimized()));
    connect(ui->quitBtn, SIGNAL(released()), this, SLOT(close()));
    connect(ui->audioCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onChangeCmd()));
    connect(ui->videoInSize, SIGNAL(currentIndexChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->inputPanel, SIGNAL(currentChanged(int)), this, SLOT(onChangeCmd()));
    connect(ui->customInputLine, SIGNAL(editTextChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->inParamLine, SIGNAL(editTextChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->urlinLine, SIGNAL(textChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->outParamLine1, SIGNAL(editTextChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->outParamLine2, SIGNAL(editTextChanged(QString)), this, SLOT(onChangeCmd()));
    connect(ui->outParamLine3, SIGNAL(editTextChanged(QString)), this, SLOT(onChangeCmd()));
    connect(&m_ffmpeg, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onStopStreaming()));
    connect(&m_ffmpeg, SIGNAL(readyReadStandardError()), this, SLOT(onReadFFmpegStderr()));
    connect(&m_ffother, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onFFotherExit()));
    connect(&m_ffother, SIGNAL(readyReadStandardError()), this, SLOT(onReadFFotherOutput()));
    connect(&m_ffother, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadFFotherOutput()));
    connect(m_runTimer, SIGNAL(timeout()), this, SLOT(onRunTimeout()));
    connect(m_ppDialog, SIGNAL(finished(int)), this, SLOT(onPPDiagClose()));
    m_ffmpeg.setReadChannel(QProcess::StandardError);

    GetDevice(audioList, videoList, m_videoOptList);
    ui->videoCombo->insertItems(0, videoList);
    ui->audioCombo->insertItems(0, audioList);


    QString urlcust = m_config->value("urlcust").toString(); if (!urlcust.isEmpty()) ui->customInputLine->setEditText(urlcust);
    QString paramin = m_config->value("paramin").toString(); if (!paramin.isEmpty()) ui->inParamLine->setEditText(paramin);
    QString urlin = m_config->value("urlin").toString(); if (!urlin.isEmpty()) ui->urlinLine->setText(urlin);
    QString param1 = m_config->value("param1").toString(); if (!param1.isEmpty()) ui->outParamLine1->setEditText(param1);
    QString url1 = m_config->value("url1").toString(); if (!url1.isEmpty()) ui->urlOutLine1->setText(url1);
    QString param2 = m_config->value("param2").toString(); if (!param2.isEmpty()) ui->outParamLine2->setEditText(param2);
    QString url2 = m_config->value("url2").toString(); if (!url2.isEmpty()) ui->urlOutLine2->setText(url2);
    QString param3 = m_config->value("param3").toString(); if (!param3.isEmpty()) ui->outParamLine3->setEditText(param3);
    QString url3 = m_config->value("url3").toString(); if (!url3.isEmpty()) ui->urlOutLine3->setText(url3);
}

MainWindow::~MainWindow()
{
    if (m_ffmpeg.state() != QProcess::NotRunning) {
        m_ffmpeg.kill();
        m_ffmpeg.waitForFinished(2000);
    }
    if (m_ffother.state() != QProcess::NotRunning) {
        m_ffother.kill();
        m_ffother.waitForFinished(2000);
    }

    m_config->sync();
    delete m_config;
    delete m_runTimer;
    delete m_ppDialog;
    delete ui;
}

void MainWindow::stopStream()
{
    m_isStreaming = false;
    m_runTimer->stop();
    ui->statusTag->setTabText(0, "status: no job");
    ui->streamBtn->setText(QString("start"));
    ui->playInputBtn->setEnabled(true);
    ui->probeInputBtn->setEnabled(true);
    ui->quitBtn->setEnabled(true);
    ui->inputTab->setEnabled(true);
    ui->outputTab->setEnabled(true);
    ui->inputSelect->setEnabled(true);
}

QString MainWindow::getSourceCamName()
{
    QString videoParam = "-video_size " + ui->videoInSize->currentText();
    QString audioName = ui->audioCombo->currentText();
    QString videoName = ui->videoCombo->currentText();
    if (ui->videoCombo->count() && ui->camCheck->isChecked() && ui->audioCombo->count() && ui->micCheck->isChecked())
        return " -f dshow " + videoParam + " " + audioParam + " -i video=" + videoName + ":audio=" + audioName + " ";
    else if (ui->videoCombo->count() && ui->camCheck->isChecked())
        return " -f dshow " + videoParam + " -i video=" + videoName + " ";
    else if (ui->audioCombo->count() && ui->micCheck->isChecked())
        return " -f dshow " + audioParam + " -i audio=" + audioName + " ";
    else
        return QString("");
}

QString MainWindow::getInputPath()
{
    int index = ui->inputPanel->currentIndex();
    if (0 == index)
        return getSourceCamName();
    else if (1 == index)
        return ui->urlinLine->text();
    else
        return ui->customInputLine->currentText();
}

void MainWindow::getCmd()
{
    int     index = ui->inputPanel->currentIndex();
    QString srcDevName = getSourceCamName();
    QString inParam = ui->inParamLine->currentText();
    QString custInput = ui->customInputLine->currentText();
    QString inUrl = ui->urlinLine->text();
    QString out = ui->outParamLine1->currentText() + " " + ui->urlOutLine1->text() + " " + ui->outParamLine2->currentText()
            + " " + ui->urlOutLine2->text() + " " + ui->outParamLine3->currentText() + " " + ui->urlOutLine3->text();

    if (!index) {
        m_cmdStr = "ffmpeg " + srcDevName + " " + out;
    } else if (1 == index) {
        m_cmdStr = "ffmpeg " + inParam + " -i " + inUrl + " " + out;
    } else {
        m_cmdStr = "ffmpeg " + custInput + " " + out;
    }
}

void MainWindow::showEvent(QShowEvent *)
{
    repaint();
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_moving = true;
        m_dragPosition = event->globalPos();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if( event->buttons().testFlag(Qt::LeftButton) && m_moving) {
        this->move(this->pos() + (event->globalPos() - m_dragPosition));
        m_dragPosition = event->globalPos();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) m_moving = false;
}

void MainWindow::onChangeCmd()
{
    getCmd();
    ui->statusBox->setCurrentIndex(0);
    ui->ffmpegCmdLine->setPlainText(m_cmdStr);
}

void MainWindow::onReadFFmpegStderr()
{
    QString s;
    QByteArray ba;
    while (!(ba = m_ffmpeg.readLine()).isEmpty()) {
        if (ba.endsWith('\n')) {
            ba.truncate(ba.size() - 2);
        }
        if (ba.endsWith("\r")) {
           ba.truncate(ba.size()-1);
           ui->runLogEdit->document()->undo();
        }
        if (ui->runLogEdit->document()->blockCount() > 2000) {
            s = ui->runLogEdit->toPlainText();
            s.remove(0, s.size() / 2);
            ui->runLogEdit->clear();
            ui->runLogEdit->appendPlainText(s);
        }
        ui->runLogEdit->appendPlainText(QString(ba));
    }
}

void MainWindow::onReadFFotherOutput()
{
    static QByteArray last;
    QByteArray ba;
    QList<QByteArray> baList;
    if (m_infoNeedClean) {
        m_infoNeedClean = false;
        ui->infoLogEdit->clear();
        ui->statusBox->setCurrentIndex(2);
    }
    ba = m_ffother.readAllStandardError();
    if (ba.isEmpty()) {
        last.clear();
        ba = m_ffother.readAllStandardOutput();
        if (ba.count() > 2) {
            ui->infoLogEdit->appendPlainText(QString(ba));
        }
        return;
    }
    if (!last.isEmpty()) {
        ba = last + ba;
        last.clear();
    }
    baList =  ba.split('\n');
    for (int i = 0; i < baList.size(); i++) {
        if (i == baList.size() - 1 && !baList[i].endsWith('\r')) {
            last = baList[i];
            continue;
        }
        if (baList[i].endsWith('\r') && !baList[i].endsWith(" \r")) {
            baList[i].truncate(baList[i].size()-1);
            if (baList[i].contains('\r')) {
                baList[i] = baList[i].right(baList[i].size() - baList[i].lastIndexOf('\r') - 1);
            }
            ui->infoLogEdit->appendPlainText(QString(baList[i]));
        }
    }

}

void MainWindow::onStopStreaming()
{
    if (m_isRunLoop && m_isStreaming) {
        m_needRestart = 1;
    } else {
        stopStream();
    }
}

void MainWindow::onFFotherExit()
{
    if (m_focusBtn) {
        m_focusBtn->setChecked(false);
        m_focusBtn = NULL;
        m_infoNeedClean = false;
    }
}

void MainWindow::onPPDiagClose()
{
    ui->centralWidget->setEnabled(true);
    ui->playOutBtn->setChecked(m_ppDialog->m_isPlay);
    if (m_ppDialog->m_isPlay) {
        m_ppDialog->m_isPlay = false;
        m_focusBtn = ui->playOutBtn;
        m_ffother.start(playCmdPrefix + m_ppDialog->getPlayUrl());
        m_infoNeedClean = true;
    }
}

void MainWindow::onRunTimeout()
{
    QString str;
    int interval;

    if (m_needRestart && m_isStreaming && m_isRunLoop) {
        m_ffmpeg.start(m_cmdStr);
        m_restartTimes++;
        m_needRestart = false;
        m_startTime = time(NULL);
    }
    interval = time(NULL) - m_startTime;

    if (!m_restartTimes) {
        str.sprintf("status: running %02d:%02d:%02d", interval / 3600, interval % 3600 / 60, interval % 60);
    } else {
        str.sprintf("status: restart %d times, last running %02d:%02d:%02d", m_restartTimes, interval / 3600, interval % 3600 / 60, interval % 60);
    }

    ui->statusTag->setTabText(0, str);
}

void MainWindow::on_streamBtn_released()
{
    if (!m_isStreaming) {
        m_isStreaming = true;
        m_restartTimes = 0;
        m_needRestart = 0;
        m_startTime = time(NULL);
        m_runTimer->start(1000);
        ui->runLogEdit->clear();
        ui->streamBtn->setText(QString("stop"));
        ui->statusTag->setTabText(0, QString("status: running 00:00:00"));
        ui->inputTab->setEnabled(false);
        ui->outputTab->setEnabled(false);
        ui->quitBtn->setEnabled(false);
        ui->inputSelect->setEnabled(false);

        if (ui->urlOutLine1->text().isEmpty()) {
            ui->outParamLine1->clearEditText();
        }
        if (ui->urlOutLine2->text().isEmpty()) {
            ui->outParamLine2->clearEditText();
        }
        if (ui->urlOutLine3->text().isEmpty()) {
            ui->outParamLine3->clearEditText();
        }
        if (!ui->inputPanel->currentIndex()) {
            ui->playInputBtn->setEnabled(false);
            ui->probeInputBtn->setEnabled(false);
            if (m_focusBtn == ui->playInputBtn) {
                m_ffother.kill();
                m_ffother.waitForFinished(1000);
            }
        }
        onChangeCmd();
        ui->statusBox->setCurrentIndex(1);
        m_config->setValue("urlcust", ui->customInputLine->currentText());
        m_config->setValue("paramin", ui->inParamLine->currentText());
        m_config->setValue("urlin", ui->urlinLine->text());
        m_config->setValue("param1", ui->outParamLine1->currentText());
        m_config->setValue("url1", ui->urlOutLine1->text());
        m_config->setValue("param2", ui->outParamLine2->currentText());
        m_config->setValue("url2", ui->urlOutLine2->text());
        m_config->setValue("param3", ui->outParamLine3->currentText());
        m_config->setValue("url3", ui->urlOutLine3->text());
        m_ffmpeg.start(m_cmdStr);
    } else {
        m_isStreaming = false;
        m_ffmpeg.kill();
        stopStream();
    }
}

void MainWindow::on_probeInputBtn_released()
{
    if (m_focusBtn == ui->probeInputBtn) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->probeInputBtn;
        m_ffother.start(probeCmdPrefix + getInputPath());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_probeOutput1_released()
{
    if (m_focusBtn == ui->probeOutput1) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->probeOutput1;
        m_ffother.start(probeCmdPrefix + ui->urlOutLine1->text());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_probeOutput2_released()
{
    if (m_focusBtn == ui->probeOutput2) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->probeOutput2;
        m_ffother.start(probeCmdPrefix + ui->urlOutLine2->text());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_probeOutput3_released()
{
    if (m_focusBtn == ui->probeOutput3) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->probeOutput3;
        m_ffother.start(probeCmdPrefix + ui->urlOutLine3->text());
        m_infoNeedClean = true;
    }
}

void MainWindow::on_playInputBtn_released()
{
    if (m_focusBtn == ui->playInputBtn) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->playInputBtn;
        if (!ui->inputPanel->currentIndex()) {
            m_ffother.start("ffmpeg -hide_banner " + getInputPath() + " -pix_fmt yuv420p -window_size 640x360 -f sdl ffpanel");
        } else {
            m_ffother.start(playCmdPrefix + getInputPath());
        }
        m_infoNeedClean = true;
    }
}
void MainWindow::on_playOutput1_released()
{
    if (m_focusBtn == ui->playOutput1) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->playOutput1;
        m_ffother.start(playCmdPrefix + ui->urlOutLine1->text());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_playOutput2_released()
{
    if (m_focusBtn == ui->playOutput2) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->playOutput2;
        m_ffother.start(playCmdPrefix + ui->urlOutLine2->text());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_playOutput3_released()
{
    if (m_focusBtn == ui->playOutput3) {
        m_ffother.kill();
    } else {
        if (m_focusBtn) {
            m_ffother.kill();
            m_ffother.waitForFinished();
        }
        m_focusBtn = ui->playOutput3;
        m_ffother.start(playCmdPrefix + ui->urlOutLine3->text());
        m_infoNeedClean = true;
    }
}
void MainWindow::on_playOutBtn_released()
{
    if (m_focusBtn) {
        if (m_ffother.state() != QProcess::NotRunning)
            m_ffother.kill();
    }
    if (m_focusBtn != ui->playOutBtn) {
        ui->centralWidget->setEnabled(false);
        m_ppDialog->setGeometry(this->geometry().x(), this->geometry().y() - 30, 624, 30);
        m_ppDialog->show();
    }
}

void MainWindow::on_devRdo_released()
{
    ui->inputPanel->setCurrentIndex(0);
}

void MainWindow::on_urlRdo_released()
{
    ui->inputPanel->setCurrentIndex(1);
}

void MainWindow::on_mixRdo_released()
{
    ui->inputPanel->setCurrentIndex(2);
}

void MainWindow::on_addrInBtn_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "open file", " ",  "Allfile(*.*)");
    if (!filename.isEmpty()) {
        ui->urlinLine->setText(QString("\"") + filename + QString("\""));
    }
}

void MainWindow::on_outUrl1Btn_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "open file", " ",  "Allfile(*.*)");
    if (!filename.isEmpty()) {
        ui->urlOutLine1->setText(QString("\"") + filename + QString("\""));
    }
}

void MainWindow::on_outUrl2Btn_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "open file", " ",  "Allfile(*.*)");
    if (!filename.isEmpty()) {
        ui->urlOutLine2->setText(QString("\"") + filename + QString("\""));
    }
}

void MainWindow::on_outUrl3Btn_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "open file", " ",  "Allfile(*.*)");
    if (!filename.isEmpty()) {
        ui->urlOutLine3->setText(QString("\"") + filename + QString("\""));
    }
}

void MainWindow::on_videoCombo_currentIndexChanged(int index)
{
    ui->videoInSize->clear();
    ui->videoInSize->addItems(m_videoOptList[index]);
    onChangeCmd();
}

void MainWindow::on_micCheck_stateChanged(int arg1)
{
    ui->audioCombo->setEnabled(arg1);
    onChangeCmd();
}

void MainWindow::on_camCheck_stateChanged(int arg1)
{
    ui->videoCombo->setEnabled(arg1);
    ui->videoInSize->setEnabled(arg1);
    onChangeCmd();
}

void MainWindow::on_urlOutLine1_textChanged(const QString &arg1)
{
    ui->probeOutput1->setEnabled(!arg1.isEmpty());
    ui->playOutput1->setEnabled(!arg1.isEmpty());
    onChangeCmd();
}

void MainWindow::on_urlOutLine2_textChanged(const QString &arg1)
{
    ui->probeOutput2->setEnabled(!arg1.isEmpty());
    ui->playOutput2->setEnabled(!arg1.isEmpty());
    onChangeCmd();
}

void MainWindow::on_urlOutLine3_textChanged(const QString &arg1)
{
    ui->probeOutput3->setEnabled(!arg1.isEmpty());
    ui->playOutput3->setEnabled(!arg1.isEmpty());
    onChangeCmd();
}


void MainWindow::on_isLoopComb_currentIndexChanged(int index)
{
    m_isRunLoop = index;
}
