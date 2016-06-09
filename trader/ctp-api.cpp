#include "ctp-api.h"

MdSpi::MdSpi() {}
MdSpi::~MdSpi() {}
///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void MdSpi::OnFrontConnected() {}

///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
///@param nReason 错误原因
///        0x1001 网络读失败
///        0x1002 网络写失败
///        0x2001 接收心跳超时
///        0x2002 发送心跳失败
///        0x2003 收到错误报文
void MdSpi::OnFrontDisconnected(int nReason) {}

///心跳超时警告。当长时间未收到报文时，该方法被调用。
///@param nTimeLapse 距离上次接收报文的时间
void MdSpi::OnHeartBeatWarning(int nTimeLapse) {}

///登录请求响应
void MdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                           CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) {}

///登出请求响应
void MdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                            CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) {}

///错误应答
void MdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) {}

///订阅行情应答
void MdSpi::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

///取消订阅行情应答
void MdSpi::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

///订阅询价应答
void MdSpi::OnRspSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

///取消订阅询价应答
void MdSpi::OnRspUnSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

///深度行情通知
void MdSpi::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pDepthMarketData) {}

///询价通知
void MdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {}

MdApi::MdApi() {}
MdApi::~MdApi() {}

///删除接口对象本身
///@remark 不再使用本接口对象时,调用该函数删除接口对象
void MdApi::Release() {}

///初始化
///@remark 初始化运行环境,只有调用后,接口才开始工作
void MdApi::Init() {}

///等待接口线程结束运行
///@return 线程退出代码
int MdApi::Join() {}

///获取当前交易日
///@retrun 获取到的交易日
///@remark 只有登录成功后,才能得到正确的交易日
const char *MdApi::GetTradingDay() {}

///注册前置机网络地址
///@param pszFrontAddress：前置机网络地址。
///@remark
///网络地址的格式为：“protocol://ipaddress:port”，如：”tcp://127.0.0.1:17001”。
///@remark
///“tcp”代表传输协议，“127.0.0.1”代表服务器地址。”17001”代表服务器端口号。
void MdApi::RegisterFront(char *pszFrontAddress) {}

///注册名字服务器网络地址
///@param pszNsAddress：名字服务器网络地址。
///@remark
///网络地址的格式为：“protocol://ipaddress:port”，如：”tcp://127.0.0.1:12001”。
///@remark
///“tcp”代表传输协议，“127.0.0.1”代表服务器地址。”12001”代表服务器端口号。
///@remark RegisterNameServer优先于RegisterFront
void MdApi::RegisterNameServer(char *pszNsAddress) {}

///注册名字服务器用户信息
///@param pFensUserInfo：用户信息。
void MdApi::RegisterFensUserInfo(CThostFtdcFensUserInfoField *pFensUserInfo) {}

///注册回调接口
///@param pSpi 派生自回调接口类的实例
void MdApi::RegisterSpi(CThostFtdcMdSpi *pSpi) {}

///订阅行情。
///@param ppInstrumentID 合约ID
///@param nCount 要订阅/退订行情的合约个数
///@remark
int MdApi::SubscribeMarketData(char *ppInstrumentID[], int nCount) {
  return (0);
}

///退订行情。
///@param ppInstrumentID 合约ID
///@param nCount 要订阅/退订行情的合约个数
///@remark
int MdApi::UnSubscribeMarketData(char *ppInstrumentID[], int nCount) {
  return (0);
}

///订阅询价。
///@param ppInstrumentID 合约ID
///@param nCount 要订阅/退订行情的合约个数
///@remark
int MdApi::SubscribeForQuoteRsp(char *ppInstrumentID[], int nCount) {
  return (0);
}

///退订询价。
///@param ppInstrumentID 合约ID
///@param nCount 要订阅/退订行情的合约个数
///@remark
int MdApi::UnSubscribeForQuoteRsp(char *ppInstrumentID[], int nCount) {
  return (0);
}

///用户登录请求
int MdApi::ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLoginField,
                        int nRequestID) {
  return (0);
}

///登出请求
int MdApi::ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                         int nRequestID) {
  return (0);
}
