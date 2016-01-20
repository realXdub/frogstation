#include <QByteArray>
#include <QtEndian>
#include "payload.h"
#include "stdint.h"

PayloadSatellite::PayloadSatellite() : checksum(0), senderNode(0), timestamp(0), senderThread(0), topic(0), ttl(0), userDataLen(0){
    userData[0] = 0;
}

PayloadSatellite::PayloadSatellite(const QByteArray &buffer) : checksum(0), senderNode(0), timestamp(0), senderThread(0), topic(0), ttl(0), userDataLen(0){
    userData[0] = 0;
    if(buffer.size() < 1023)
        return;

    checksum = qFromBigEndian(*((quint16*)(buffer.constData() + 0)));
    senderNode = qFromBigEndian(*((quint32*)(buffer.constData() + 2)));
    timestamp = qFromBigEndian(*((quint64*)(buffer.constData() + 6)));
    senderThread = qFromBigEndian(*((quint32*)(buffer.constData() + 14)));
    topic = qFromBigEndian(*((quint32*)(buffer.constData() + 18)));
    ttl = qFromBigEndian(*((quint16*)(buffer.constData() + 22)));
    userDataLen = qFromBigEndian(*((quint16*)(buffer.constData() + 24)));
    memcpy(userData, buffer.constData() + 26, userDataLen);
    userData[userDataLen] = 0x00;
}

PayloadCounter::PayloadCounter(const PayloadSatellite payload): counter(0){
    if(payload.userDataLen != sizeof(PayloadCounter) || payload.topic != PayloadCounterType)
        return;
    counter = *(int*)(payload.userData);
}

PayloadSensorIMU::PayloadSensorIMU(const PayloadSatellite payload):wz(0), roll(0), pitch(0), yaw(0){
    if(payload.userDataLen != sizeof(PayloadSensorIMU) || payload.topic != PayloadSensorIMUType)
        return;
    wz = *(float*)(payload.userData);
    roll = *(float*)(payload.userData + 1 * sizeof(float));
    pitch = *(float*)(payload.userData + 2 * sizeof(float));
    yaw = *(float*)(payload.userData + 3 * sizeof(float));
}

PayloadElectrical::PayloadElectrical(const PayloadSatellite payload): lightsensorOn(0), electromagnetOn(0), thermalKnifeOn(0), light(0){
    if(payload.userDataLen != sizeof(PayloadElectrical) || payload.topic != PayloadElectricalType)
        return;
    lightsensorOn = *(bool*)(payload.userData);
    electromagnetOn = *(bool*)(payload.userData + 1 * sizeof(bool));
    thermalKnifeOn = *(bool*)(payload.userData + 2 * sizeof(bool));
    light = *(int16_t*)(payload.userData + 3 * sizeof(bool));
}

PayloadImage::PayloadImage(const PayloadSatellite payload){
    for(int i = 0; i<(121*160*2); i++){
        image[i] = *(uint8_t*)(payload.userData + 1 * sizeof(uint8_t));
    }
}
