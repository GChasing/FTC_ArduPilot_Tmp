/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <AP_Param/AP_Param.h>
#include <AP_SerialManager/AP_SerialManager.h>
#include <AP_Common/AP_ExpandingArray.h>

#ifndef HAL_AIS_ENABLED
#define HAL_AIS_ENABLED !HAL_MINIMIZE_FEATURES
#endif

#if HAL_AIS_ENABLED

#define AIVDM_BUFFER_SIZE 10
#define AIVDM_PAYLOAD_SIZE 65

class AP_AIS
{
public:
    AP_AIS();

    /* Do not allow copies */
    AP_AIS(const AP_AIS &other) = delete;
    AP_AIS &operator=(const AP_AIS&) = delete;

    // return true if AIS is enabled
    bool enabled() const { return AISType(_type.get()) != AISType::NONE; }

    // Initialize the AIS object and prepare it for use
    void init();

    // update AIS, expected to be called at 20hz
    void update();

    // send mavlink AIS message
    void send(mavlink_channel_t chan);

    // parameter block
    static const struct AP_Param::GroupInfo var_info[];

private:

    // parameters
    AP_Int8 _type;             // type of AIS receiver
    AP_Int16 _max_list;        // maximum number of vessels to track at once
    AP_Int16 _time_out;        // time in seconds that a vessel will be dropped from the list
    AP_Int16 _log_options;     // logging options bitmask

    enum class AISType {
        NONE   = 0,
        NMEA   = 1,
    };

    enum options {
        AIS_OPTIONS_LOG_ALL_RAW         = 1<<0,
        AIS_OPTIONS_LOG_UNSUPPORTED_RAW = 1<<1,
        AIS_OPTIONS_LOG_DECODED         = 1<<2,
    };

    struct AIVDM {
        uint8_t num;
        uint8_t total;
        uint8_t ID;
        char payload[AIVDM_PAYLOAD_SIZE];
    };
    AIVDM _incoming;
    AIVDM _AIVDM_buffer[AIVDM_BUFFER_SIZE];

    struct ais_vehicle_t {
        mavlink_ais_vessel_t info;
        uint32_t last_update_ms; // last time this was refreshed, allows timeouts
        uint32_t last_send_ms; // last time this message was sent via mavlink, stops us spamming the link
    };

    // list of the vessels that are being tracked
    AP_ExpandingArray<ais_vehicle_t> _list {8};

    AP_HAL::UARTDriver *_uart;

    uint16_t _send_index; // index of the last vessel send over mavlink

    // removed the given index from the AIVDM buffer shift following elements
    void buffer_shift(uint8_t i);

    // find vessel in existing list, if not then return new index if possible
    bool get_vessel_index(uint32_t mmsi, uint16_t &index, uint32_t lat = 0, uint32_t lon = 0) WARN_IF_UNUSED;
    void clear_list_item(uint16_t index);

    // decode the payload
    bool payload_decode(const char *payload) WARN_IF_UNUSED;

    // decode specific message types
    bool decode_position_report(const char *payload, uint8_t type) WARN_IF_UNUSED;
    bool decode_base_station_report(const char *payload) WARN_IF_UNUSED;
    bool decode_static_and_voyage_data(const char *payload) WARN_IF_UNUSED;

    // read the specified bits from the char array each char giving 6 bits
    void get_char(const char *payload, char *array, uint16_t low, uint16_t high);
    uint32_t get_bits(const char *payload, uint16_t low, uint16_t high);
    int32_t get_bits_signed(const char *payload, uint16_t low, uint16_t high);
    // un-encode the ASCII payload armoring
    uint8_t payload_char_decode(const char c);

    // log a raw AIVDM message
    void log_raw(const AIVDM *msg);

    // try and decode NMEA message
    bool decode(char c) WARN_IF_UNUSED;

    // decode each term
    bool decode_latest_term() WARN_IF_UNUSED;

    // convert from char to hex value for checksum
    int16_t char_to_hex(char a);

    // varables for decoding NMEA sentence
    char _term[AIVDM_PAYLOAD_SIZE]; // buffer for the current term within the current sentence
    uint8_t _term_offset;           // offset within the _term buffer where the next character should be placed
    uint8_t _term_number;           // term index within the current sentence
    uint8_t _checksum;              // checksum accumulator
    bool _term_is_checksum;         // current term is the checksum
    bool _sentence_valid;           // is current sentence valid so far
    bool _sentence_done;            // true if this sentence has already been decoded
};

#endif  // HAL_AIS_ENABLED
