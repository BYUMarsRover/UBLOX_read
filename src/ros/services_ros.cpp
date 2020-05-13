#include <UBLOX/ublox_ros.h>

namespace ublox_ros
{
    bool UBLOX_ROS::cfgValGet(ublox::CfgValGet::Request &req, ublox::CfgValGet::Response &res)
{
    ublox::CFG_VALGET_t::request_t request;
    request.version=0;
    request.layer=req.layer;
    request.position=req.position;
    request.cfgDataKey.keyID=req.key;

    ublox::CFG_VALGET_TUPLE_t response = ublox_->cfgValGet(request);
    std::vector<ublox::CFG_VALGET_t::response_t> cfgVector_ublox = std::get<1>(response);
    for(int i=0; i<cfgVector_ublox.size(); i++)
    {
        ublox::CfgValGetType cfg_ros;
        cfg_ros.version = cfgVector_ublox[i].version;
        cfg_ros.layer = cfgVector_ublox[i].layer;
        cfg_ros.position = cfgVector_ublox[i].position.position;
        cfg_ros.keyID = cfgVector_ublox[i].cfgDataKey.keyID;
        cfg_ros.keyName = std::string(cfgVector_ublox[i].keyName);
        cfg_ros.data = cfgVector_ublox[i].cfgData.data;
        
        res.cfgData.push_back(cfg_ros);
    }
    res.ack=std::get<0>(response).got_ack;
    res.nack=std::get<0>(response).got_nack;
    res.gotcfg=std::get<0>(response).got_cfg_val;
    res.flags=std::get<0>(response).flags;

    return true;
}

bool UBLOX_ROS::cfgValDel(ublox::CfgValDel::Request &req, ublox::CfgValDel::Response &res)
{

    ublox::CFG_VAL_DBG_t response = ublox_->cfgValDel(0, req.layer, req.key);

    res.got_Ack = response.got_ack;
    res.got_Nack = response.got_nack;

    return true;
}



bool UBLOX_ROS::cfgValSet(ublox::CfgValSet::Request &req, ublox::CfgValSet::Response &res)
{
    ublox::CFG_VAL_DBG_t response = ublox_->cfgValSet(0, req.layer, req.cfgData, req.key, req.size);

    res.got_Ack = response.got_ack;
    res.got_Nack = response.got_nack;

    return true;
}

bool UBLOX_ROS::cfgReset(ublox::CfgReset::Request &req, ublox::CfgReset::Response &res)
{
    ublox::navBbrMask_t bitfield =  ublox_->reset(req.navBbrMask, req.resetMode);

    // std::cerr<<"eph: "<< bitfield.eph<<std::endl;

    res.eph = bitfield.eph;
    res.alm = bitfield.alm;
    res.health = bitfield.health;
    res.klob = bitfield.klob;
    res.pos = bitfield.pos;
    res.clkd = bitfield.clkd;
    res.osc = bitfield.osc;
    res.utc = bitfield.utc;
    res.rtc = bitfield.rtc;
    res.aop = bitfield.aop;

    return true;

}

bool UBLOX_ROS::initModule(ublox::initModule::Request &req, ublox::initModule::Response &res)
{
    switch(req.type)
    {
        case 0:
            initBase();
            break;
        case 1:
            initRover();
            break;
        case 2:
            initBrover();
            break;
        default:
            std::cerr<<"Error: initModule invalid type\n";
            return false;
            break;
    }
    return true;
}

}