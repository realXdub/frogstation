#include "groundstation.h"
#include "ui_groundstation.h"


Groundstation::Groundstation(QWidget *parent) :
    QMainWindow(parent), link(this), imager(this),
    ui(new Ui::Groundstation)
{
    ui->setupUi(this);

    /*Maximize window*/
    setWindowState(windowState() | Qt::WindowMaximized);

    /*Set up console updates from lower classes*/
    connect(&link, SIGNAL(updateConsole()), this, SLOT(connectionUpdateConsole()));
    connect(&imager, SIGNAL(updateConsole()), this, SLOT(imagelinkUpdateConsole()));

    /*Set up Wifi*/
    link.bind();
    link.addTopic(PayloadSensorIMUType);
    link.addTopic(PayloadCounterType);
    link.addTopic(PayloadElectricalType);
    link.addTopic(PayloadMissionType);
    link.addTopic(PayloadLightType);
    connect(&link, SIGNAL(readReady()), this, SLOT(readoutConnection()));

    /*Set up bluetooth menu and LED*/
    imager.initializePort();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
        ui->bluetoothComboBox->addItem(info.portName());
    }
    connect(&imager, SIGNAL(updateStatus()), this, SLOT(updateBluetoothLED()));     /*Updating bluetooth LED*/
    connect(&imager, SIGNAL(updateImage()), this, SLOT(updateImage()));             /*Updating image in groundstation*/

    /*Set up timer for telemetry activity check*/
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(telemetryCheck()));
    timer->start();

    /*---------------*/
    /*CONNECT BUTTONS*/
    /*---------------*/

    /*Top Row menus*/
    connect(ui->openPortButton, SIGNAL(clicked()), this, SLOT(onOpenPortButtonClicked()));                  /*Open selected bluetooth port*/
    connect(ui->closePortButton, SIGNAL(clicked()), this, SLOT(onClosePortButtonClicked()));                /*Close selected bluetooth port*/
    connect(ui->bluetoothComboBox, SIGNAL(activated(int)), this, SLOT(updateBluetoothLED()));               /*update bluetooth status LED when different port is selected*/
    connect(ui->restartWifiButton, SIGNAL(clicked()), this, SLOT(onRestartWifiButtonClicked()));            /*Wifi restart (for those x>1000 times I hang around in RZUWsec instead of yetenet*/
    connect(ui->activateSatelliteButton, SIGNAL(clicked()), this, SLOT(onActivateSatelliteButtonClicked()));/*Activating satellite*/
    connect(ui->emergencyOffButton, SIGNAL(clicked()), this, SLOT(onEmergencyOffButtonClicked()));          /*Turn off moving, power-consuming or dangerous devices instantly*/

    /*Manual Control Tab*/
    connect(ui->deployRacksButton, SIGNAL(clicked()), this, SLOT(onDeployRacksButtonClicked()));
    connect(ui->pullRacksButton, SIGNAL(clicked()), this, SLOT(onPullRacksButtonClicked()));
    connect(ui->stopRacksButton, SIGNAL(clicked()), this, SLOT(onStopRacksButtonClicked()));
    connect(ui->activateElectromagnetButton, SIGNAL(clicked()), this, SLOT(onActivateElectromagnetButtonClicked()));
    connect(ui->deactivateElectromagnetButton, SIGNAL(clicked()), this, SLOT(onDeactivateElectromagnetButtonClicked()));
    connect(ui->takePictureButton, SIGNAL(clicked()), this, SLOT(onTakePictureButtonClicked()));                /*Take picture and receive it via bluetooth*/

    connect(ui->calibrateAccButton, SIGNAL(clicked()), this, SLOT(onCalibrateAccButtonClicked()));              /*Calibrations*/
    connect(ui->calibrateGyroButton, SIGNAL(clicked()), this, SLOT(onCalibrateGyroButtonClicked()));
    connect(ui->calibrateMagnetoButton, SIGNAL(clicked()), this, SLOT(onCalibrateMagnetoButtonClicked()));
    connect(ui->exitCalibrationButton, SIGNAL(clicked()), this, SLOT(onExitCalibrationButtonClicked()));        /*Exit calibration mode*/

    connect(ui->controllerStartButton, SIGNAL(clicked()), this, SLOT(onStartControllerButtonClicked()));        /*Starting controller*/
    connect(ui->controllerStopButton, SIGNAL(clicked()), this, SLOT(onStopControllerButtonClicked()));          /*Stopping controller*/

    /*Attitude Tab*/
    connect(ui->orientationSetButton, SIGNAL(clicked()), this, SLOT(onOrientationSetButtonClicked()));          /*Setting wished orientation angle...*/
    connect(ui->orientationLineEdit, SIGNAL(returnPressed()), this, SLOT(onOrientationSetButtonClicked()));     /*...also works with return in the lineEdit instead of clicking annoying button*/
    connect(ui->setRotationButton, SIGNAL(clicked()), this, SLOT(onSetRotationButtonClicked()));                /*Setting wished rotation speed...*/
    connect(ui->rotationLineEdit, SIGNAL(returnPressed()), this, SLOT(onSetRotationButtonClicked()));           /*...also works with return in the lineEdit instead of clicking annoying button*/

    /*Sun Finder Tab*/
    connect(ui->sunFinderButton, SIGNAL(clicked()), this, SLOT(onSunFinderButtonClicked()));                    /*Start sun acquisition*/

    /*Mission Tab*/
    connect(ui->missionStartButton, SIGNAL(clicked()), this, SLOT(onMissionStartButtonClicked()));              /*Start mission*/
    connect(ui->missionAbortButton, SIGNAL(clicked()), this, SLOT(onMissionAbortButtonClicked()));              /*Stop mission*/

    /*Set up graph widgets*/
    setupGraphs();
}

Groundstation::~Groundstation()
{
    delete ui;
}


/*-------------*/
/*Wifi readouts*/
/*-------------*/

void Groundstation::readoutConnection(){
    if(!ui->telemetryLED->isChecked()){
        console("Telemetry online.");
    }
    ui->telemetryLED->setChecked(true);
    PayloadSatellite payload = link.read();
    switch(payload.topic){
    case PayloadSensorIMUType:{
        PayloadSensorIMU psimu(payload);
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;

        /*accelerometerWidget update*/
        ui->accelerometerWidget->graph(0)->addData(key, psimu.ax/1000);
        ui->accelerometerWidget->graph(0)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->accelerometerWidget->graph(0)->rescaleValueAxis();
        ui->accelerometerWidget->graph(1)->addData(key, psimu.ay/1000);
        ui->accelerometerWidget->graph(1)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->accelerometerWidget->graph(1)->rescaleValueAxis(true);
        ui->accelerometerWidget->graph(2)->addData(key, psimu.az/1000);
        ui->accelerometerWidget->graph(2)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->accelerometerWidget->graph(2)->rescaleValueAxis(true);
        ui->accelerometerWidget->xAxis->setRange(key+0.25, XAXIS_VISIBLE_TIME, Qt::AlignRight);
        ui->accelerometerWidget->replot();

        /*gyroscopeWidget update*/
        ui->gyroscopeWidget->graph(0)->addData(key, radToDeg(psimu.wx));
        ui->gyroscopeWidget->graph(0)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->gyroscopeWidget->graph(0)->rescaleValueAxis();
        ui->gyroscopeWidget->graph(1)->addData(key, radToDeg(psimu.wy));
        ui->gyroscopeWidget->graph(1)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->gyroscopeWidget->graph(1)->rescaleValueAxis(true);
        ui->gyroscopeWidget->graph(2)->addData(key, radToDeg(psimu.wz));
        ui->gyroscopeWidget->graph(2)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->gyroscopeWidget->graph(2)->rescaleValueAxis(true);
        ui->gyroscopeWidget->xAxis->setRange(key+0.25, XAXIS_VISIBLE_TIME, Qt::AlignRight);
        ui->gyroscopeWidget->replot();

        /*headingWidget update*/
        ui->headingWidget->graph(0)->addData(key, radToDeg(psimu.headingXm));
        ui->headingWidget->graph(0)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->headingWidget->graph(0)->rescaleValueAxis();
        ui->headingWidget->graph(1)->addData(key, radToDeg(psimu.headingGyro));
        ui->headingWidget->graph(1)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->headingWidget->graph(1)->rescaleValueAxis(true);
        ui->headingWidget->graph(2)->addData(key, radToDeg(psimu.headingFusion));
        ui->headingWidget->graph(2)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->headingWidget->graph(2)->rescaleValueAxis(true);
        ui->headingWidget->xAxis->setRange(key+0.25, XAXIS_VISIBLE_TIME, Qt::AlignRight);
        ui->headingWidget->replot();

        /*LCD updates*/
        ui->compassWidget->angle = radToDeg(psimu.headingFusion);
        ui->debrisMapWidget->angle = radToDeg(psimu.headingFusion);
        ui->rotationLCD->display(radToDeg(psimu.wz));
        ui->orientationLCD->display(radToDeg(psimu.headingFusion));
        ui->pitchLCD->display(radToDeg(psimu.pitch));
        ui->rollLCD->display(radToDeg(psimu.roll));

        /*LED updates*/
        if(psimu.calibrationActive){
            console("Calibration running...");
        }

        break;
    }
    case PayloadCounterType:{
        PayloadCounter pscount(payload);
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
        break;
    }
    case PayloadElectricalType:{
        PayloadElectrical pelec(payload);
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;

        /*LED updates*/
        ui->lightsensorLED->setChecked(pelec.lightsensorOn);
        ui->electromagnetLED->setChecked(pelec.electromagnetOn);
        ui->thermalKnifeLED->setChecked(pelec.thermalKnifeOn);
        ui->racksDeployedLED->setChecked(pelec.racksOut);
        ui->solarDeployedLED->setChecked(pelec.solarPanelsOut);

        /*LCD updates*/
        ui->solarVoltageLCD->display(pelec.solarPanelVoltage);
        ui->solarCurrentLCD->display(pelec.solarPanelCurrent);
        ui->batteryCurrentLCD->display(pelec.batteryCurrent);
        ui->batteryVoltageLCD->display(pelec.batteryVoltage);
        ui->powerConsumptionLCD->display(pelec.batteryVoltage * pelec.batteryCurrent / 1000);
        break;
    }
    case PayloadLightType:{
        PayloadLight plight(payload);
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
        ui->lightsensorLCD->display(plight.lightValue);

        /*sunFinderWidget / lightsensor graph update*/
        if(ui->lightsensorLED->isChecked()){
            ui->sunFinderWidget->graph(0)->addData(key, plight.lightValue);
        }else{
            ui->sunFinderWidget->graph(0)->addData(key, 0);
        }
        ui->sunFinderWidget->graph(0)->removeDataBefore(key-XAXIS_VISIBLE_TIME);
        ui->sunFinderWidget->graph(0)->rescaleValueAxis();
        ui->sunFinderWidget->xAxis->setRange(key+0.25, XAXIS_VISIBLE_TIME, Qt::AlignRight);
        ui->sunFinderWidget->replot();
    }
    case PayloadMissionType:{
        PayloadMission pmission(payload);
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
        Debris tc_debris = Debris(pmission.partNumber, pmission.angle, pmission.isCleaned);
        ui->debrisMapWidget->processDebris(&tc_debris);
        ui->debrisFoundLCD->display(ui->debrisMapWidget->getFoundNumber());
        ui->debrisCleanedLCD->display(ui->debrisMapWidget->getCleanedNumber());
    }
    default:
        break;
    }
    return;
}


/*--------------------*/
/*BUTTONS/TELECOMMANDS*/
/*--------------------*/

/*Command-struct to be found in payload.h / .cpp*/

/*Telecommands*/
void Groundstation::telecommand(int ID, int identifier, int value){
    /*WIFI*/
    Command com(ID, identifier, value);
    link.connectionSendCommand(TELECOMMAND_TOPIC_ID, com);

    /*BLUETOOTH*/
//    imager.sendCommand(com);
}

/*Top Row*/
void Groundstation::onOpenPortButtonClicked(){
    imager.activePortName = ui->bluetoothComboBox->currentText();
    imager.openPort();
}

void Groundstation::onClosePortButtonClicked(){
    imager.activePortName = ui->bluetoothComboBox->currentText();
    imager.closePort();
}

void Groundstation::onRestartWifiButtonClicked(){
    link.bind();
}

void Groundstation::onActivateSatelliteButtonClicked(){
    console("TC: Activate satellite");
    telecommand(ID_MISSION, 5003, 1);
}

void Groundstation::onEmergencyOffButtonClicked(){
    console("TC: Disengage all electrical components");
    telecommand(ID_ELECTRICAL, 3001, 0);    /*Stop racks*/
    telecommand(ID_ELECTRICAL, 3002, 0);    /*Turn off electromagnet*/
    telecommand(ID_ELECTRICAL, 3003, 0);    /*Turn off thermal knife*/
    telecommand(ID_ELECTRICAL, 3004, 0);    /*Stop main motor*/
}

/*Manual Control Tab*/
void Groundstation::onDeployRacksButtonClicked(){
    console("TC: Deploy racks");
    telecommand(ID_ELECTRICAL, 3001, 1);
}

void Groundstation::onPullRacksButtonClicked(){
    console("TC: Pull in racks");
    telecommand(ID_ELECTRICAL, 3001, -1);
}

void Groundstation::onStopRacksButtonClicked(){
    console("TC: Stop racks");
    telecommand(ID_ELECTRICAL, 3001, 0);
}

void Groundstation::onActivateElectromagnetButtonClicked(){
    console("TC: Activate electromagnet");
    telecommand(ID_ELECTRICAL, 3002, 1);
}

void Groundstation::onDeactivateElectromagnetButtonClicked(){
    console("TC: Deactivate electromagnet");
    telecommand(ID_ELECTRICAL, 3002, 0);
}

void Groundstation::onTakePictureButtonClicked(){
    console("TC: Take picture");
    telecommand(ID_PICTURE, 4001, 1);
}

void Groundstation::onCalibrateAccButtonClicked(){
    console("TC: Start accelerometer calibration");
    telecommand(ID_CALIBRATE, 1002, 1);
}

void Groundstation::onCalibrateGyroButtonClicked(){
    console("TC: Start gyroscope calibration");
    telecommand(ID_CALIBRATE, 1003, 1);
}

void Groundstation::onCalibrateMagnetoButtonClicked(){
    console("TC: Start magnetometer calibration");
    telecommand(ID_CALIBRATE, 1001, 1);
}

void Groundstation::onExitCalibrationButtonClicked(){
    console("TC: Exit calibration mode");
    telecommand(ID_MISSION, 5004, 1);
}

void Groundstation::onStartControllerButtonClicked(){
    console("TC: Start controller");
    telecommand(ID_ATTITUDE, 2003, 1);
}

void Groundstation::onStopControllerButtonClicked(){
    console("TC: Stop controller");
    telecommand(ID_ATTITUDE, 2003, 0);
}

/*Attitude Tab*/
/*also works with orientationLineEdit->returnPressed()*/
void Groundstation::onOrientationSetButtonClicked(){
    int angle;
    bool ok;
    angle = ui->orientationLineEdit->text().toInt(&ok);
    if(ok && (360 >= angle) && (angle >= 0)){
        console(QString("TC: Set orientation to %1 degrees").arg(angle));
        telecommand(ID_ATTITUDE, 2002, angle);
    }
    else
        console("ERROR: Orientation angle invalid");
}

/*also works with rotationLineEdit->returnPressed()*/
void Groundstation::onSetRotationButtonClicked(){
    int angle;
    bool ok;
    angle = ui->rotationLineEdit->text().toInt(&ok);
    if(ok && (360 >= angle) && (angle >= -360)){
        console(QString("TC: Set orientation to %1 degrees").arg(angle));
        telecommand(ID_ATTITUDE, 2001, angle);
    }
    else
        console("ERROR: Orientation angle invalid");
}

/*Mission Tab*/
void Groundstation::onSunFinderButtonClicked(){
    console("TC: Start sun acquisition routine");
    telecommand(ID_MISSION, 5001, 1);
}

void Groundstation::onMissionStartButtonClicked(){
    console("TC: Start mission");
    telecommand(ID_MISSION, 5002, 1);
}

void Groundstation::onMissionAbortButtonClicked(){
    console("TC: Sop mission");
    telecommand(ID_MISSION, 5002, 0);
}

/*------------*/
/*GRAPH SETUPS*/
/*------------*/

void Groundstation::setupGraphs(){

    /*Defining fonts*/
    QFont legendFont = font();
    legendFont.setPointSize(7);

    QFont labelFont1 = font();
    labelFont1.setPointSize(9);

    /*Defining grey gradients*/
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(120, 120, 120));
    plotGradient.setColorAt(1, QColor(80, 80, 80));

    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(120, 120, 120));
    axisRectGradient.setColorAt(1, QColor(80, 80, 80));

    /*Set up accelerometerWidget*/
    ui->accelerometerWidget->xAxis->setLabel("Current Time");
    ui->accelerometerWidget->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->accelerometerWidget->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->accelerometerWidget->xAxis->setAutoTickStep(false);
    ui->accelerometerWidget->xAxis->setTickStep(XAXIS_TICKSTEP);
    ui->accelerometerWidget->yAxis->setLabel("Acceleration (g)");
    ui->accelerometerWidget->axisRect()->setupFullAxesBox();
    ui->accelerometerWidget->xAxis->setLabelFont(labelFont1);
    ui->accelerometerWidget->yAxis->setLabelFont(labelFont1);
    ui->accelerometerWidget->legend->setVisible(true);
    ui->accelerometerWidget->legend->setFont(legendFont);
    ui->accelerometerWidget->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->accelerometerWidget->legend->setIconSize(10,5);
    ui->accelerometerWidget->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);
    ui->accelerometerWidget->addGraph();
    ui->accelerometerWidget->graph(0)->setPen(QPen(Qt::yellow));
    ui->accelerometerWidget->graph(0)->setName("x");
    ui->accelerometerWidget->addGraph();
    ui->accelerometerWidget->graph(1)->setPen(QPen(Qt::red));
    ui->accelerometerWidget->graph(1)->setName("y");
    ui->accelerometerWidget->addGraph();
    ui->accelerometerWidget->graph(2)->setPen(QPen(Qt::green));
    ui->accelerometerWidget->graph(2)->setName("z");
    ui->accelerometerWidget->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis2->setBasePen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis2->setBasePen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis2->setTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis2->setTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->yAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->xAxis->setTickLabelColor(Qt::white);
    ui->accelerometerWidget->yAxis->setTickLabelColor(Qt::white);
    ui->accelerometerWidget->xAxis->setLabelColor(Qt::white);
    ui->accelerometerWidget->yAxis->setLabelColor(Qt::white);
    ui->accelerometerWidget->legend->setBorderPen(QPen(Qt::white, 1));
    ui->accelerometerWidget->legend->setTextColor(Qt::white);
    ui->accelerometerWidget->legend->setBrush(plotGradient);
    ui->accelerometerWidget->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->accelerometerWidget->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->accelerometerWidget->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->accelerometerWidget->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->accelerometerWidget->xAxis->grid()->setSubGridVisible(true);
    ui->accelerometerWidget->yAxis->grid()->setSubGridVisible(true);
    ui->accelerometerWidget->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->accelerometerWidget->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->accelerometerWidget->setBackground(plotGradient);
    ui->accelerometerWidget->axisRect()->setBackground(axisRectGradient);
    /*make left and bottom axes transfer their ranges to right and top axes*/
    connect(ui->accelerometerWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->accelerometerWidget->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->accelerometerWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->accelerometerWidget->yAxis2, SLOT(setRange(QCPRange)));

    /*Set up gyroscopeWidget*/
    ui->gyroscopeWidget->xAxis->setLabel("Current Time");
    ui->gyroscopeWidget->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->gyroscopeWidget->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->gyroscopeWidget->xAxis->setAutoTickStep(false);
    ui->gyroscopeWidget->xAxis->setTickStep(XAXIS_TICKSTEP);
    ui->gyroscopeWidget->yAxis->setLabel("Angular Speed (deg/sec)");
    ui->gyroscopeWidget->xAxis->setLabelFont(labelFont1);
    ui->gyroscopeWidget->yAxis->setLabelFont(labelFont1);
    ui->gyroscopeWidget->axisRect()->setupFullAxesBox();
    ui->gyroscopeWidget->legend->setVisible(true);
    ui->gyroscopeWidget->legend->setFont(legendFont);
    ui->gyroscopeWidget->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->gyroscopeWidget->legend->setIconSize(10,5);
    ui->gyroscopeWidget->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);
    ui->gyroscopeWidget->addGraph();
    ui->gyroscopeWidget->graph(0)->setPen(QPen(Qt::yellow));
    ui->gyroscopeWidget->graph(0)->setName("x");
    ui->gyroscopeWidget->addGraph();
    ui->gyroscopeWidget->graph(1)->setPen(QPen(Qt::red));
    ui->gyroscopeWidget->graph(1)->setName("y");
    ui->gyroscopeWidget->addGraph();
    ui->gyroscopeWidget->graph(2)->setPen(QPen(Qt::green));
    ui->gyroscopeWidget->graph(2)->setName("z");
    ui->gyroscopeWidget->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis2->setBasePen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis2->setBasePen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis2->setTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis2->setTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->yAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->xAxis->setTickLabelColor(Qt::white);
    ui->gyroscopeWidget->yAxis->setTickLabelColor(Qt::white);
    ui->gyroscopeWidget->xAxis->setLabelColor(Qt::white);
    ui->gyroscopeWidget->yAxis->setLabelColor(Qt::white);
    ui->gyroscopeWidget->legend->setBorderPen(QPen(Qt::white, 1));
    ui->gyroscopeWidget->legend->setTextColor(Qt::white);
    ui->gyroscopeWidget->legend->setBrush(plotGradient);
    ui->gyroscopeWidget->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->gyroscopeWidget->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->gyroscopeWidget->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->gyroscopeWidget->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->gyroscopeWidget->xAxis->grid()->setSubGridVisible(true);
    ui->gyroscopeWidget->yAxis->grid()->setSubGridVisible(true);
    ui->gyroscopeWidget->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->gyroscopeWidget->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->gyroscopeWidget->setBackground(plotGradient);
    ui->gyroscopeWidget->axisRect()->setBackground(axisRectGradient);
    /*make left and bottom axes transfer their ranges to right and top axes*/
    connect(ui->gyroscopeWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->gyroscopeWidget->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->gyroscopeWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->gyroscopeWidget->yAxis2, SLOT(setRange(QCPRange)));

    /*Set up headingWidget*/
    ui->headingWidget->xAxis->setLabel("Current Time");
    ui->headingWidget->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->headingWidget->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->headingWidget->xAxis->setAutoTickStep(false);
    ui->headingWidget->xAxis->setTickStep(XAXIS_TICKSTEP);
    ui->headingWidget->yAxis->setLabel("Heading (deg)");
    ui->headingWidget->xAxis->setLabelFont(labelFont1);
    ui->headingWidget->yAxis->setLabelFont(labelFont1);
    ui->headingWidget->axisRect()->setupFullAxesBox();
    ui->headingWidget->legend->setVisible(true);
    ui->headingWidget->legend->setFont(legendFont);
    ui->headingWidget->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->headingWidget->legend->setIconSize(10,5);
    ui->headingWidget->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);
    ui->headingWidget->addGraph();
    ui->headingWidget->graph(0)->setPen(QPen(Qt::yellow));
    ui->headingWidget->graph(0)->setName("Xm");
    ui->headingWidget->addGraph();
    ui->headingWidget->graph(1)->setPen(QPen(Qt::red));
    ui->headingWidget->graph(1)->setName("Gyro");
    ui->headingWidget->addGraph();
    ui->headingWidget->graph(2)->setPen(QPen(Qt::green));
    ui->headingWidget->graph(2)->setName("Combined");
    ui->headingWidget->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis2->setBasePen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis2->setBasePen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis2->setTickPen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis2->setTickPen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->headingWidget->yAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->headingWidget->xAxis->setTickLabelColor(Qt::white);
    ui->headingWidget->yAxis->setTickLabelColor(Qt::white);
    ui->headingWidget->xAxis->setLabelColor(Qt::white);
    ui->headingWidget->yAxis->setLabelColor(Qt::white);
    ui->headingWidget->legend->setBorderPen(QPen(Qt::white, 1));
    ui->headingWidget->legend->setTextColor(Qt::white);
    ui->headingWidget->legend->setBrush(plotGradient);
    ui->headingWidget->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->headingWidget->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->headingWidget->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->headingWidget->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->headingWidget->xAxis->grid()->setSubGridVisible(true);
    ui->headingWidget->yAxis->grid()->setSubGridVisible(true);
    ui->headingWidget->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->headingWidget->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->headingWidget->setBackground(plotGradient);
    ui->headingWidget->axisRect()->setBackground(axisRectGradient);
    /*make left and bottom axes transfer their ranges to right and top axes*/
    connect(ui->headingWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->headingWidget->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->headingWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->headingWidget->yAxis2, SLOT(setRange(QCPRange)));

    /*Set up sunFinderWidget*/
    ui->sunFinderWidget->xAxis->setLabel("Current Time");
    ui->sunFinderWidget->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->sunFinderWidget->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->sunFinderWidget->xAxis->setAutoTickStep(false);
    ui->sunFinderWidget->xAxis->setTickStep(XAXIS_TICKSTEP);
    ui->sunFinderWidget->yAxis->setLabel("Lightsensor Data");
    ui->sunFinderWidget->xAxis->setLabelFont(labelFont1);
    ui->sunFinderWidget->yAxis->setLabelFont(labelFont1);
    ui->sunFinderWidget->axisRect()->setupFullAxesBox();
    ui->sunFinderWidget->addGraph();
    ui->sunFinderWidget->graph(0)->setPen(QPen(Qt::yellow));
    ui->sunFinderWidget->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis2->setBasePen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis2->setBasePen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis2->setTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis2->setTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->yAxis2->setSubTickPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->xAxis->setTickLabelColor(Qt::white);
    ui->sunFinderWidget->yAxis->setTickLabelColor(Qt::white);
    ui->sunFinderWidget->xAxis->setLabelColor(Qt::white);
    ui->sunFinderWidget->yAxis->setLabelColor(Qt::white);
    ui->sunFinderWidget->legend->setBorderPen(QPen(Qt::white, 1));
    ui->sunFinderWidget->legend->setTextColor(Qt::white);
    ui->sunFinderWidget->legend->setBrush(plotGradient);
    ui->sunFinderWidget->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->sunFinderWidget->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->sunFinderWidget->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->sunFinderWidget->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->sunFinderWidget->xAxis->grid()->setSubGridVisible(true);
    ui->sunFinderWidget->yAxis->grid()->setSubGridVisible(true);
    ui->sunFinderWidget->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->sunFinderWidget->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->sunFinderWidget->setBackground(plotGradient);
    ui->sunFinderWidget->axisRect()->setBackground(axisRectGradient);
    /*make left and bottom axes transfer their ranges to right and top axes*/
    connect(ui->sunFinderWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->sunFinderWidget->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->sunFinderWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->sunFinderWidget->yAxis2, SLOT(setRange(QCPRange)));
}


/*--------------------*/
/*CONSOLE TEXT UPDATES*/
/*--------------------*/

/*Console updates from lower classes*/

void Groundstation::console(QString msg){
    ui->consoleWidget->writeString(msg);
}

void Groundstation::connectionUpdateConsole(){
    ui->consoleWidget->writeString(link.consoleText);
}

void Groundstation::imagelinkUpdateConsole(){
    ui->consoleWidget->writeString(imager.consoleText);
}


/*---------------------*/
/*DISPLAY UPDATED IMAGE*/
/*---------------------*/

/*Getting the updated image from imagelink and updating it in the UI*/
void Groundstation::updateImage(){
    QImage scaled = imager.currentImage.scaled(ui->missionInputLabel->width(),ui->missionInputLabel->height(),Qt::KeepAspectRatio);
    ui->missionInputLabel->setPixmap(QPixmap::fromImage(scaled));
}


/*-----------*/
/*LED UPDATES*/
/*-----------*/

/*disable telemetry LED when connection is lost after around 3 seconds*/
void Groundstation::telemetryCheck(){
    if((ui->telemetryLED->isChecked()) && (QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - key) >= 3){
        ui->telemetryLED->setChecked(false);
        console("Telemetry lost.");
    }
}

/*update bluetooth activity LED when a different port is selected from list*/
void Groundstation::updateBluetoothLED(){
    imager.activePortName = ui->bluetoothComboBox->currentText();
    ui->bluetoothLED->setChecked(imager.isOpen());
}

/*Radiants to degrees conversion*/
float Groundstation::radToDeg(float rad){
    return (rad*180)/M_PI;
}
