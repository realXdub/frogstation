#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <QtGlobal>
#include <QByteArray>
#include <QtEndian>
#include <QDebug>

#include "stdint.h"

enum PayloadType{
    PayloadCounterType = 5001,
    PayloadSensorIMUType = 5002,
    PayloadElectricalType = 5003,
    PayloadMissionType = 5004,
    PayloadLightType = 5005
};

struct PayloadCounter;
struct PayloadSensorIMU;
struct PayloadElectrical;
struct PayloadMission;

struct PayloadSatellite{
    quint16 checksum;
    quint32 senderNode;
    quint64 timestamp;
    quint32 senderThread;
    quint32 topic;
    quint16 ttl;
    quint16 userDataLen;
    quint8 userData[998];
    PayloadSatellite();
    PayloadSatellite(const QByteArray &buffer);
};

struct PayloadCounter{
    int counter;
    PayloadCounter(const PayloadSatellite payload);
};

struct PayloadSensorIMU{
    float ax;               /*milli-g*/
    float ay;               /*milli-g*/
    float az;               /*milli-g*/
    float wx;               /*rad/sec*/
    float wy;               /*rad/sec*/
    float wz;               /*rad/sec*/
    float roll;             /*rad*/
    float pitch;            /*rad*/
    float headingFusion;    /*rad*/
    float headingXm;        /*rad*/
    float headingGyro;      /*rad*/
    bool calibrationActive;
    PayloadSensorIMU(const PayloadSatellite payload);
};

struct PayloadElectrical{
    bool lightsensorOn;
    bool electromagnetOn;
    bool thermalKnifeOn;
    bool racksOut;
    bool solarPanelsOut;
    float batteryCurrent;       /*mA*/
    float batteryVoltage;       /*V*/
    float solarPanelCurrent;    /*mA*/
    float solarPanelVoltage;    /*V*/
    PayloadElectrical(const PayloadSatellite payload);
};

struct PayloadLight{
    uint16_t lightValue;        /*raw data*/
    PayloadLight(const PayloadSatellite payload);
};

struct PayloadMission{
    int partNumber;
    float angle;
    bool isCleaned;
    PayloadMission(const PayloadSatellite payload);
};

struct Command{
    int id;             /*[1,5]*/
    int identifier;     /*100x, 200x etc*/
    int value;
    Command(int tc_id, int tc_identifier, int tc_value);
};

#endif // PAYLOAD_H
