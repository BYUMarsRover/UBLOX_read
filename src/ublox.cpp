#include "UBLOX/ublox.h"
#define DBG(...) fprintf(stderr, __VA_ARGS__)

int p = 1;
int unknown = 0;
int ubx = 0;
int rtcm = 0;
int nmea = 0;

UBLOX::UBLOX(rtk_type_t type, std::string port) :
    serial_(port, 115200),
    ubx_(serial_),
    type_(type)
{
    printf("type = %d", type_);
    fflush(stdout);

    auto cb = [this](const uint8_t* buffer, size_t size)
    {
        this->serial_read_cb(buffer, size);
    };
    serial_.register_receive_callback(cb);
    serial_.init();

    // configure the parsers
    ubx_.set_nav_rate(100);
    ubx_.enable_message(UBX::CLASS_NAV, UBX::NAV_PVT, 10);
    ubx_.enable_message(UBX::CLASS_NAV, UBX::NAV_POSECEF, 10);
    ubx_.enable_message(UBX::CLASS_NAV, UBX::NAV_VELECEF, 10);
    ubx_.enable_message(UBX::CLASS_CFG, UBX::CFG_VALGET, 10);

    // configure RTCM message
    if (type_ & RTK == RTK )
        ubx_.turnOnRTCM();
    if (type_ == ROVER)
    {
        configRover();
        ubx_.config_rover();
    }
    if (type_ == BASE)
    {
        configBase();
        ubx_.config_base();
    }
}

UBLOX::~UBLOX()
{
    if (udp_)
        delete udp_;
}

void UBLOX::udp_read_cb(const uint8_t* buf, size_t size)
{

    assert(type_ == ROVER);

    for (int i = 0; i < size; i++)
    {
        rtcm_.read_cb(buf[i]);
        rtcm++;

    }
}

void UBLOX::serial_read_cb(const uint8_t *buf, size_t size)
{

    for (int i = 0; i < size; i++)
    {
        /// TODO: don't give parsers data they don't need
        if (ubx_.parsing_message())
            {
                ubx_.read_cb(buf[i]);
                ubx++;
                p = 1;
            }
        else if (rtcm_.parsing_message() && type_ != NONE)
            {
                 rtcm_.read_cb(buf[i]);
                 rtcm++;
                 p = 1;
            }
        else if (nmea_.parsing_message())
            {
                nmea_.read_cb(buf[i]);
                nmea++;
                p = 1;
            }
        else
        {
            ubx_.read_cb(buf[i]);
            rtcm_.read_cb(buf[i]);
            nmea_.read_cb(buf[i]);
            if(p>1)
            {
                unknown++;
            }
            p++;
        }

    }
}


void UBLOX::configRover()
{
    assert(udp_ == nullptr);
    // Connect the rtcm_cb callback to forward data to the UBX serial port
    rtcm_.registerCallback([this](uint8_t* buf, size_t size)
    {
        this->rtcm_complete_cb(buf, size);
    });

    // hook up UDP to listen
    /// TODO: configure ports and IP from cli
    udp_ = new async_comm::UDP("localhost", 14620, "localhost", 14625);
    udp_->register_receive_callback([this](const uint8_t* buf, size_t size)
    {
        this->udp_read_cb(buf, size);
    });

    if (!udp_->init())
        throw std::runtime_error("Failed to initialize Rover receive UDP");
}

void UBLOX::configBase()
{
    if (udp_)
        throw std::runtime_error("Unable to create two UDP connections");

    // hook up UDP to send
    udp_ = new async_comm::UDP("localhost", 14625, "localhost", 14620);

    if (!udp_->init())
    {
        throw std::runtime_error("Failed to initialize Rover receive UDP");
    }

    rtcm_.registerCallback([this](uint8_t* buf, size_t size)
    {
        this->udp_->send_bytes(buf, size);
    });
}

void UBLOX::rtcm_complete_cb(const uint8_t *buf, size_t size)
{
    assert (type_ == ROVER || type_ == BASE);
    if (type_ == ROVER)
        serial_.send_bytes(buf, size);
    else if (type_ == BASE)
        udp_->send_bytes(buf, size);
}
