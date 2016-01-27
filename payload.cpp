#include "payload.h"

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

PayloadSensorIMU::PayloadSensorIMU(const PayloadSatellite payload):ax(0), ay(0), az(0), wx(0), wy(0), wz(0), roll(0), pitch(0), headingFusion(0), headingXm(0), headingGyro(0){
    if(payload.userDataLen != sizeof(PayloadSensorIMU) || payload.topic != PayloadSensorIMUType)
        return;
    ax = *(float*)(payload.userData);
    ay = *(float*)(payload.userData + 1 * sizeof(float));
    az = *(float*)(payload.userData + 2 * sizeof(float));
    wx = *(float*)(payload.userData + 3 * sizeof(float));
    wy = *(float*)(payload.userData + 4 * sizeof(float));
    wz = *(float*)(payload.userData + 5 * sizeof(float));
    roll = *(float*)(payload.userData + 6 * sizeof(float));
    pitch = *(float*)(payload.userData + 7 * sizeof(float));
    headingFusion = *(float*)(payload.userData + 8 * sizeof(float));
    headingXm = *(float*)(payload.userData + 9 * sizeof(float));
    headingGyro = *(float*)(payload.userData + 10 * sizeof(float));
}

PayloadElectrical::PayloadElectrical(const PayloadSatellite payload): lightsensorOn(0), electromagnetOn(0), thermalKnifeOn(0), light(0){
    if(payload.userDataLen != sizeof(PayloadElectrical) || payload.topic != PayloadElectricalType)
        return;
    lightsensorOn = *(bool*)(payload.userData);
    electromagnetOn = *(bool*)(payload.userData + 1 * sizeof(bool));
    thermalKnifeOn = *(bool*)(payload.userData + 2 * sizeof(bool));
    racksOut = *(bool*)(payload.userData + 3 * sizeof(bool));
    solarPanelsOut = *(bool*)(payload.userData + 4 * sizeof(bool));
    light = *(int32_t*)(payload.userData + 5 * sizeof(bool));
    batteryVoltage = *(float*)(payload.userData + 5 * sizeof(bool) + 1 * sizeof(int32_t));
    batteryCurrent = *(float*)(payload.userData + 5 * sizeof(bool) + 1 * sizeof(int32_t) + 1 * sizeof(float));
    solarPanelVoltage = *(float*)(payload.userData + 5 * sizeof(bool) + 1 * sizeof(int32_t) + 2 * sizeof(float));
    solarPanelCurrent = *(float*)(payload.userData + 5 * sizeof(bool) + 1 * sizeof(int32_t) + 3 * sizeof(float));
}

Command::Command(int tc_id, int tc_identifier, int tc_value): id(tc_id), identifier(tc_identifier), value(tc_value){

}
