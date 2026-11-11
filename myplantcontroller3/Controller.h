/*
 * Controller.h
 *
 *  Created on: Feb 24, 2021
 *      Author: Hosein
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <vector>
#include <string>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/lifecycle/ILifecycle.h"

namespace inet {


class Controller : public ApplicationBase, public UdpSocket::ICallback
{
  private:
        simsignal_t referenceValueSignal;
        simsignal_t sensorValueSignal1;
        simsignal_t sensorValueSignal2;
        simsignal_t controlValueSignal;
        simsignal_t errorValueSignal;
        simsignal_t DEVSignal1;
        simsignal_t DEVSignal2;
        simsignal_t normSignal;
        simsignal_t corrSignal1;
        simsignal_t corrSignal2;
        simsignal_t corrSignal3;
        simsignal_t corrSignal4;

  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    // parameters
    std::vector<L3Address> destAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    simtime_t startTime;
    simtime_t stopTime;
    bool dontFragment = false;
    const char *packetName = nullptr;

    //PI Controller
    const double Ts = 0.01;
    const double Kp = 3.22e-7;
    const double Ki = 2.26e-10;
    double Integral = 0;
    double controlError = 0;
    double referenceValue = 0;
    double value1 = 0;
    double value2 = 0;
    double controlSignal = 0;

    //reference signal
    const double t1 = 220;
    const double t2 = 380;
    const double t3 = 460;
    const double t4 = 620;

    //state space correct sys
    const double Aa[2][2] = {{0.9999, 8.69e-5},
                             {8.69e-5, 0.9999}};

    const double Bb[2] = {6366, 0.2769};

    //filter
    const double G[2][2] = {{0.8, 0},
                            {0, 0.8}};
    const double D[2][2] = {{0.1999, 8.69e-5},
                            {8.69e-5, 0.1999}};
    const double Q[2][2] = {{4.4682, 3.9869},
                            {3.9869, 4.4682}};
    double p1 = 0;
    double p2 = 0;

    double z1 = 0;
    double z2 = 0;

    double Y1 = 0;
    double Y2 = 0;
    double norm_Y = 0;
    double R1 = 0;
    double R2 = 0;

    double S1 = 0;
    double S2 = 0;

    double corr1 = 0;
    double corr2 = 0;
    double corr3 = 0;
    double corr4 = 0;

    double f1 = 0;
    double f2 = 0;

    double norm_R = 0;

    double x=0;
    double u[2] = {0, 0};
    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    std::string plantAddr = "10.0.0.2";
    // statistics
    int numSent = 0;
    int numReceived = 0;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr();
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;


  public:
    Controller() {};
    ~Controller();
    void PI_Controller(double y);
    void Evaluation();
    double ReferenceSignal();
    void Plant_Model();

};

}




#endif /* CONTROLLER_H_ */
