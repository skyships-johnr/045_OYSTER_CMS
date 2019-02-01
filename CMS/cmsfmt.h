///////////////////////////////////////////////////////////////////////////////
//
// cmsfmt.h
//
// CMS message format


#ifndef __CMSFMT_H__
#define __CMSFMT_H__

#pragma pack(push)
#pragma pack(1)

// Port
#define CMS_PORT 4321

// Limits
#define CMS_MSG_MAX_DATA 128
#define CMS_MSG_MAX_SIZE sizeof(CMS_MSG_GENERIC)

// IDs
#define CMS_MSG_ID_ACK               0
#define CMS_MSG_ID_DCM_CAN           1
#define CMS_MSG_ID_PARAM_UPDATE      2
#define CMS_MSG_ID_TIME_INFO         4
#define CMS_MSG_ID_APP_PROFILE_RESET 5
#define CMS_MSG_ID_COUNT             6

// Message header
typedef struct
{
    uint32_t seq; // Sequence
	uint8_t src;  // Source device
	uint8_t dest; // Destination device
	uint8_t id;   // Command id
	uint8_t size; // Data size
}
CMS_MSG_HDR;


////////////////////////////////////////////////////////////////////////////////
// Generic message

typedef struct
{
	CMS_MSG_HDR header; // Header
	uint8_t data[ CMS_MSG_MAX_DATA ];
}
CMS_MSG_GENERIC;


////////////////////////////////////////////////////////////////////////////////
// Ack message

typedef struct
{
	CMS_MSG_HDR header; // Header
}
CMS_MSG_ACK;


////////////////////////////////////////////////////////////////////////////////
// DCM can message
// CAN message
/*typedef struct
{
    kzU8 port;
    kzU8 ext;
    kzU8 dlc;
    kzU8 rsvd;
    kzU32 id;
    kzU8 data[ 8 ];
}
CAN_MSG;*/

typedef struct
{
    CMS_MSG_HDR header; // Header
    //CAN_MSG msg; // Msg
}
CMS_MSG_DCM_CAN;


////////////////////////////////////////////////////////////////////////////////
// Param update (up to 4 for now)

#define CMS_PARAM_UPDATE_MAX 4
typedef struct
{
    uint32_t numparams; // Nof params
    struct
    {
        uint32_t hash; // Hash
        double value; // Value
    }
    params[ CMS_PARAM_UPDATE_MAX ];
}
CMS_PARAM_UPDATE;

typedef struct
{
    CMS_MSG_HDR header; // Header
    CMS_PARAM_UPDATE update; // Update
}
CMS_MSG_PARAM_UPDATE;


////////////////////////////////////////////////////////////////////////////////
// Time info

// GPS info
typedef struct
{
    uint64_t timeUtc;              // Ms
    uint64_t timeLocal;            // Ms
    uint32_t timeSunrise;          // Min
    uint32_t timeSunset;           // Min
    int32_t timeZoneOffset;        // Ms
    int32_t timeProfile;           // -1 (inv), 0 (day), 1 (dusk), 2 (night)
    int32_t timeProfileTimes[ 4 ]; // 4 profile times
    double gpsLat;                 // WGS84
    double gpsLong;                // Deg 10E-7
}
CMS_TIME_INFO;

typedef struct
{
    CMS_MSG_HDR header; // Header
    CMS_TIME_INFO info; // Info
}
CMS_MSG_TIME_INFO;



#pragma pack(pop)

#endif // __CMSFMT_H__

///////////////////////////////////////////////////////////////////////////////
