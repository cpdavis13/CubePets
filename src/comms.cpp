// comms.cpp
#include "comms.h"
#include "battle.h"
#include <esp_system.h>
#include <Arduino.h>

static const int MAX_RECENT_PACKETS = 100;

Comms::Comms(Battle* battle)
    : _battle(battle), _role(ROLE_UNASSIGNED), _hostMac(0), _lastSendTime(0), localSeqNum(0)
{
    for (int i = 0; i < NUM_SIDES; i++) {
        neighbors[i] = {};
        neighbors[i].serial = new HardwareSerial(i + 1);
        latestCommandPackets[i].clear();
        hasPendingCommand[i] = false;
    }
}

void Comms::begin(int baud) {
    _myMac = getMacAddressHash();
    _lastSendTime = millis();

    for (int i = 0; i < NUM_SIDES; ++i) {
        neighbors[i].serial->begin(baud, SERIAL_8N1, 16, 17);  // update pins as needed
        neighbors[i].isConnected = false;
        neighbors[i].lastHeartbeat = 0;
        neighbors[i].mac = 0;
    }

    Serial.printf("[INIT] MAC: %u\n", _myMac);
}

void Comms::update() {
    for (int i = 0; i < NUM_SIDES; ++i) {
        auto& side = neighbors[i];
        auto& serial = *side.serial;

        while (serial.available()) {
            rxBuffers[i].push_back(serial.read());

            while (rxBuffers[i].size() >= 8) {
                uint8_t tag = rxBuffers[i][0];
                uint8_t payloadLen = rxBuffers[i][1];
                size_t expectedLen = 8 + payloadLen;

                if (rxBuffers[i].size() < expectedLen)
                    break;

                //Intercept command packets to buffer only the latest
                if (tag == PACKET_COMMAND) {
                    //Replace any existing buffered command packet for this side
                    latestCommandPackets[i].assign(rxBuffers[i].begin(), rxBuffers[i].begin() + expectedLen);
                    hasPendingCommand[i] = true;
                    //Remove from rx buffer to avoid double processing
                    rxBuffers[i].erase(rxBuffers[i].begin(), rxBuffers[i].begin() + expectedLen);
                } else {
                    //For other tags, process immediately
                    Serial.printf("[RX] Packet from side %d | Tag: %u | PayloadLen: %u | TotalLen: %u\n", i, tag, payloadLen, expectedLen);
                    handleIncomingPacket(i, rxBuffers[i].data(), expectedLen);
                    rxBuffers[i].erase(rxBuffers[i].begin(), rxBuffers[i].begin() + expectedLen);
                }
            }
        }

        uint32_t now = millis();
        if (side.isConnected && (uint32_t)(now - side.lastHeartbeat) > 3000) {
            Serial.printf("[TIMEOUT] Neighbor on side %d disconnected (MAC %u)\n", i, side.mac);

            //notify battle of disconnection
            if (_role == ROLE_HOST) {
                _battle->removeCube(side.mac);
            }

            side.isConnected = false;
            side.mac = 0;
            side.lastHeartbeat = 0;
            reevaluateHost();
        }
    }

    //Process one latest command packet per side per update cycle
    for (int i = 0; i < NUM_SIDES; ++i) {
        if (hasPendingCommand[i]) {
            Serial.printf("[COMMAND] Processing latest buffered command packet from side %d\n", i);
            handleIncomingPacket(i, latestCommandPackets[i].data(), latestCommandPackets[i].size());
            hasPendingCommand[i] = false;
            latestCommandPackets[i].clear();
        }
    }

    uint32_t now = millis();
    if ((uint32_t)(now - _lastSendTime) > 1000) {
        Serial.println("[TX] Sending heartbeat...");
        sendHeartbeat();
        _lastSendTime = now;
    }
}

//Helper: pack a vector of 2-bit directions into bytes
static void packDirections(const std::vector<uint8_t>& directions, std::vector<uint8_t>& outPacked) {
    outPacked.clear();
    uint8_t currentByte = 0;
    int bitsFilled = 0;

    for (uint8_t dir : directions) {
        currentByte |= (dir & 0x03) << bitsFilled;
        bitsFilled += 2;
        if (bitsFilled == 8) {
            outPacked.push_back(currentByte);
            currentByte = 0;
            bitsFilled = 0;
        }
    }
    if (bitsFilled > 0) {
        outPacked.push_back(currentByte);
    }
}

//Helper: unpack 2-bit directions from bytes
static void unpackDirections(const uint8_t* packed, size_t length, std::vector<uint8_t>& outDirections) {
    outDirections.clear();
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = packed[i];
        for (int shift = 0; shift < 8; shift += 2) {
            uint8_t dir = (byte >> shift) & 0x03;
            outDirections.push_back(dir);
        }
    }
}

void Comms::sendPacketToNeighbors(uint8_t tag, const uint8_t* payload, size_t len) {
    localSeqNum++;
    size_t totalLen = 8 + len; // tag + len + mac(4) + seq(2) + payload
    uint8_t packet[totalLen];

    packet[0] = tag;
    packet[1] = len;
    packet[2] = (_myMac >> 24) & 0xFF;
    packet[3] = (_myMac >> 16) & 0xFF;
    packet[4] = (_myMac >> 8) & 0xFF;
    packet[5] = _myMac & 0xFF;
    packet[6] = (localSeqNum >> 8) & 0xFF;
    packet[7] = localSeqNum & 0xFF;

    if (len > 0) {
        memcpy(packet + 8, payload, len);
    }

    Serial.printf("[TX] Tag: %u | Seq: %u | Len: %u\n", tag, localSeqNum, len);

    for (int i = 0; i < NUM_SIDES; i++) {
        neighbors[i].serial->write(packet, totalLen);
        Serial.printf("[TX] Sent to neighbor %d (MAC %u)\n", i, neighbors[i].mac);
    }
}

bool Comms::isDuplicatePacket(uint32_t senderMac, uint16_t seqNum) {
    for (auto& p : recentPackets) {
        if (p.senderMac == senderMac && p.seqNum == seqNum) {
            Serial.printf("[DUPLICATE] MAC: %u | Seq: %u\n", senderMac, seqNum);
            return true;
        }
    }

    if (recentPackets.size() >= MAX_RECENT_PACKETS) {
        recentPackets.pop_front();
    }
    recentPackets.push_back({senderMac, seqNum});
    return false;
}

void Comms::handleIncomingPacket(int sideIdx, uint8_t* data, size_t len) {
    if (len < 8) return;

    uint8_t tag = data[0];
    uint8_t payloadLen = data[1];
    uint32_t senderMac = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
    uint16_t seqNum = (data[6] << 8) | data[7];

    if (len != 8 + payloadLen) {
        Serial.printf("[ERROR] Length mismatch. Expected: %u, Got: %u\n", 8 + payloadLen, len);
        return;
    }

    Serial.printf("[HANDLE] Side %d | MAC: %u | Seq: %u | Tag: %u | PayloadLen: %u\n",
                  sideIdx, senderMac, seqNum, tag, payloadLen);

    if (isDuplicatePacket(senderMac, seqNum)) return;

    uint8_t* payload = data + 8;

    switch (tag) {
        case PACKET_HEARTBEAT: {
            neighbors[sideIdx].lastHeartbeat = millis();
            neighbors[sideIdx].mac = senderMac;
            bool wasConnected = neighbors[sideIdx].isConnected;
            neighbors[sideIdx].isConnected = true;

            if (!wasConnected) {
                reevaluateHost();

                if (payloadLen >= 5) {
                    //Parse Host MAC
                    uint32_t hostMacInPayload = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
                    uint8_t pathLen = payload[4];
                    size_t packedLen = payloadLen - 5;

                    //Unpack path directions
                    std::vector<uint8_t> directions;
                    unpackDirections(payload + 5, packedLen, directions);
                    if (directions.size() > pathLen) {
                        directions.resize(pathLen);
                    }

                    //Add this cube’s side as the next step in the path
                    directions.push_back(sideIdx);

                    //Update sender’s path in map
                    if (_battle) {
                        _battle->addCubeWithPath(senderMac, directions);
                    }

                    //Create a character
                    if (_myMac == _hostMac && _battle) {
                        _battle->createCharacter(senderMac, 0);
                    }

                    //If this is client, and this learned full path to host, store path
                    if (_myMac != _hostMac && _battle) {
                        std::vector<uint8_t> myPath = directions;
                        _battle->addCubeWithPath(_myMac, myPath);
                    }
                } else {
                    // No path = direct neighbor to host
                    if (_battle) {
                        _battle->addCubeWithPath(senderMac, { (unsigned char)sideIdx });
                    }
                }

                Serial.printf("[HEARTBEAT] Side %d | MAC %u connected\n", sideIdx, senderMac);
            }

            forwardPacket(sideIdx, data, len);
            break;
        }



        case PACKET_COMMAND:
            Serial.printf("[COMMAND] Processing command packet from MAC %u\n", senderMac);
            _battle->processCommands(std::vector<uint8_t>(payload, payload + payloadLen));
            forwardPacket(sideIdx, data, len);
            break;

        default:
            Serial.printf("[ERROR] Unknown tag: %u\n", tag);
            break;
    }
}

void Comms::forwardPacket(int incomingSide, uint8_t* data, size_t len) {
    for (int i = 0; i < NUM_SIDES; ++i) {
        if (i != incomingSide && neighbors[i].isConnected) {
            neighbors[i].serial->write(data, len);
            Serial.printf("[FORWARD] Packet forwarded from side %d to side %d\n", incomingSide, i);
        }
    }
}

void Comms::sendHeartbeat() {
    std::vector<uint8_t> path;

    //For host, path is empty
    if (_role == ROLE_HOST) {
        path.clear();
    } else {
        //For clients, get their path from _battle 
        path = _battle->getPathFromHost();
    }

    std::vector<uint8_t> packedPath;
    packDirections(path, packedPath);

    std::vector<uint8_t> payload;
    //Append 4 bytes of Host MAC
    payload.push_back((_hostMac >> 24) & 0xFF);
    payload.push_back((_hostMac >> 16) & 0xFF);
    payload.push_back((_hostMac >> 8) & 0xFF);
    payload.push_back(_hostMac & 0xFF);

    //Append path length 
    payload.push_back((uint8_t)path.size());

    //Append packed path bytes
    payload.insert(payload.end(), packedPath.begin(), packedPath.end());

    sendPacketToNeighbors(PACKET_HEARTBEAT, payload.data(), payload.size());
}

void Comms::reevaluateHost() {
    bool anyConnected = false;
    for (int i = 0; i < NUM_SIDES; ++i) {
        if (neighbors[i].isConnected) {
            anyConnected = true;
            break;
        }
    }

    if (!anyConnected) {
        if (_role != ROLE_UNASSIGNED) {
            _role = ROLE_UNASSIGNED;
            _hostMac = 0;
            Serial.println("[ROLE] Became UNASSIGNED (no connected neighbors)");
        }
        return;
    }

    uint32_t highestMac = _myMac;

    for (int i = 0; i < NUM_SIDES; ++i) {
        if (neighbors[i].isConnected && neighbors[i].mac > highestMac) {
            highestMac = neighbors[i].mac;
        }
    }

    if (highestMac == _myMac && _role != ROLE_HOST) {
        _role = ROLE_HOST;
        _hostMac = _myMac;
        Serial.println("[ROLE] Became HOST");

        if (_battle) {
            std::vector<uint8_t> emptyPath;
            _battle->addCubeWithPath(_myMac, emptyPath); // host at origin
        }
    } else if (_role != ROLE_CLIENT || _hostMac != highestMac) {
        _role = ROLE_CLIENT;
        _hostMac = highestMac;
        Serial.printf("[ROLE] Became CLIENT to %u\n", highestMac);
    }
}


uint32_t Comms::getMacAddressHash() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    uint32_t hash = 0;
    for (int i = 0; i < 6; i++) {
        hash ^= ((uint32_t)mac[i]) << ((i % 4) * 8);
    }
    return hash;
}

