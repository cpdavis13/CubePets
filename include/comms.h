// comms.h
#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>
#include <vector>
#include <deque>
#include <stdint.h>

#define NUM_SIDES 1 //UPDATE

// Packet types
#define PACKET_HEARTBEAT 0x01
#define PACKET_COMMAND   0x02

enum Role {
    ROLE_UNASSIGNED = 0,
    ROLE_HOST,
    ROLE_CLIENT
};

struct RecentPacket {
    uint32_t senderMac;
    uint16_t seqNum;
};

struct Neighbor {
    uint32_t mac;
    bool isConnected;
    HardwareSerial* serial;
    uint32_t lastHeartbeat;
};

class Battle; // Forward declaration

class Comms {
public:
    Comms(Battle* battle);
    void begin(int baud);
    void update();

    Role getRole() const { return _role; }
    uint32_t getHostMac() const { return _hostMac; }
    uint32_t getMyMac() const { return _myMac; }

    // Sends a packet with tag and payload to all neighbors
    void sendPacketToNeighbors(uint8_t tag, const uint8_t* payload, size_t len);

private:
    Battle* _battle;

    
    Neighbor neighbors[NUM_SIDES];
    std::vector<uint8_t> latestCommandPackets[NUM_SIDES];
    bool hasPendingCommand[NUM_SIDES];

    Role _role;
    uint32_t _hostMac;
    uint32_t _myMac;

    uint32_t _lastSendTime;

    uint16_t localSeqNum;  // Sequence number to uniquely identify outgoing packets

    std::vector<uint8_t> rxBuffers[NUM_SIDES];

    std::deque<RecentPacket> recentPackets; // Keep track of recently seen packets for duplicate suppression

    // Internal methods
    void handleIncomingPacket(int sideIdx, uint8_t* data, size_t len);
    void forwardPacket(int incomingSide, uint8_t* data, size_t len);
    void sendHeartbeat();
    void reevaluateHost();

    uint32_t getMacAddressHash();

    // Check if we have already processed a packet with this sender MAC and sequence number
    bool isDuplicatePacket(uint32_t senderMac, uint16_t seqNum);

    // Add a packet to the recentPackets buffer
    void addRecentPacket(uint32_t senderMac, uint16_t seqNum);
};

#endif //COMMS_H
