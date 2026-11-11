
#ifndef PLANT_H_
#define PLANT_H_

#include <vector>

#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/lifecycle/ILifecycle.h"

namespace inet {


class Plant : public ApplicationBase, public UdpSocket::ICallback
{
  private:
    simsignal_t sensorValueSignal1;
    simsignal_t sensorValueSignal2;
    simsignal_t controlValueSignal;
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

    //state space
    const double Aa[2][2] = {{0.9999, 8.69e-5},
                           {8.69e-5, 0.9999}};

    const double Bb[2] = {6366, 0.2769};

    double p1 = 0;
    double p2 = 0;

    double value1 = 0;
    double value2 = 0;
    double plantInput = 0;

    //damaged pipe plant
    const double Ad[2][2] = {{0.9999, 0},
                               {0, 0.9999}};

    const double Bd[2] = {6366, 0};

    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    std::string controllerAddr = "10.0.0.21";
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
    Plant() {};
    ~Plant();
    void RealPlant(double u);
    void DamagedPlant(double u);
};

}



#endif /* PLANT_H_ */
