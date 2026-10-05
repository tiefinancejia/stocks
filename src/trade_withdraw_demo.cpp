#include "stdafx.h"
#include "secitpdk/secitpdk.h"
#include "spdlog/spdlog.h"
#include <string>
#include <vector>
#include <iostream>

using namespace std;

// 配置信息
const char* g_WTFS = "32";
const char* g_Node = "PC;IIP=NA;IPORT=NA;LIP=114.248.212.26;MAC=A047D7742169;HD=2K23E27074125260000000001;PCN=WIN-20260811IKN;CPU=BFEBFBFF000306E4;PI=C^NTFS^256G;VOL=1A7E-FE06@TDRJ_LZJF;V1.0;EXT;LAN;";

string g_Khh = "20021182";
string g_Pwd = "203019";
string g_Key = "HTS_normal";
ITPDK_CusReqInfo g_CusReqInfo = {0};
bool g_UseEx = false;

string SECITPDK_GetLastErrorStr() {
    char msg[256] = {0};
    SECITPDK_GetLastError(msg);
    return string(msg);
}

void SetupCallbacks() {
    // 结构化消息回调（委托确认、成交回报、撤单确认）
    SECITPDK_SetStructMsgCallback([](const char* pTime, stStructMsg& stMsg, int nType) {
        const char* typeStr = "";
        if (NOTIFY_PUSH_ORDER == nType) typeStr = "委托确认";
        else if (NOTIFY_PUSH_MATCH == nType) typeStr = "成交回报";
        else if (NOTIFY_PUSH_WITHDRAW == nType) typeStr = "撤单确认";
        else if (NOTIFY_PUSH_INVALID == nType) typeStr = "废单";

        printf("[%s] %s: WTH=%lld, SBWTH=%s, KHH=%s, GDH=%s, ZQDM=%s, TDBH=%d",
               pTime, typeStr,
               (long long)stMsg.OrderId, stMsg.SBWTH, stMsg.AccountId,
               stMsg.SecuAccount, stMsg.StockCode, stMsg.TDBH);
        
        if (NOTIFY_PUSH_WITHDRAW == nType) {
            printf(", CXKFSBDBH=%s", stMsg.CXKFSBDBH);
        }
        printf("\n");
    });

    // 异步功能回调（下单、撤单等异步操作的应答）
    SECITPDK_SetStructOrderFuncCallback([](const char* pTime, stStructOrderFuncMsg& stMsg, int nType) {
        printf("[%s] 异步应答: RetCode=%d, RetNote=%s, OrderId=%lld, KFSBDBH=%lld\n",
               pTime, stMsg.nRetCode, stMsg.sRetNote,
               (long long)stMsg.OrderId, (long long)stMsg.KFSBDBH);
    });
}

bool Initialize() {
    SECITPDK_SetLogPath("");
    SECITPDK_SetWriteLog(true);
    SECITPDK_SetFixWriteLog(true);
    SECITPDK_SetWTFS(g_WTFS);
    SECITPDK_SetNode(g_Node);

    if (!SECITPDK_CheckCZZD(g_Key.c_str(), g_Khh.c_str(), g_Node)) {
        printf("签章验证失败\n");
        return false;
    }

    if (g_UseEx) {
        strcpy(g_CusReqInfo.AccountId, g_Khh.c_str());
        strcpy(g_CusReqInfo.Password, g_Pwd.c_str());
        strcpy(g_CusReqInfo.NodeInfo, g_Node);
        long long token = SECITPDK_TradeLoginEx(g_Key.c_str(), g_CusReqInfo);
        if (token <= 0) {
            printf("登录失败(Ex): %s\n", SECITPDK_GetLastErrorStr().c_str());
            return false;
        }
        printf("登录成功(Ex), Token=%lld\n", token);
    } else {
        long long token = SECITPDK_TradeLogin(g_Key.c_str(), g_Khh.c_str(), g_Pwd.c_str());
        if (token <= 0) {
            printf("登录失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
            return false;
        }
        printf("登录成功, Token=%lld\n", token);
    }
    return true;
}

void Cleanup() {
    SECITPDK_TradeLogout(g_Khh.c_str());
    SECITPDK_Exit();
}

// 演示：发送普通股票买单并撤销
void DemoOrderAndWithdraw() {
    printf("\n========== 发送委托并撤销演示 ==========\n");
    
    // 委托参数
    const char* sJys = "SH";          // 交易市场
    const char* sZqdm = "600000";     // 证券代码
    int nJylb = JYLB_BUY;             // 买入
    int64 nWtsl = 100;                // 委托数量
    double dWtjg = 10.50;             // 委托价格
    int64 nDdlx = DDLX_LIMIT;         // 限价单
    const char* sGdh = "A123456789";  // 股东代码
    int32 nTdbh = 0;                  // 通道编号(0=默认)
    int32 nTSWTFS = 0;                // 委托方式(0=默认)
    int64 nKFSBDBH = 123456789;       // 自定义委托流水号

    long long nOrderId = 0;

    // 1. 发送委托 (同步)
    printf("\n1. 发送买入委托...\n");
    if (g_UseEx) {
        nOrderId = SECITPDK_OrderEntrustEx(g_CusReqInfo, sJys, sZqdm, nJylb, nWtsl, dWtjg, nDdlx, nTdbh, nTSWTFS);
    } else {
        nOrderId = SECITPDK_OrderEntrust(g_Khh.c_str(), sJys, sZqdm, nJylb, nWtsl, dWtjg, nDdlx, sGdh, nTdbh, nTSWTFS);
    }

    if (nOrderId <= 0) {
        printf("委托失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
        return;
    }
    printf("委托成功! 系统委托号(WTH)=%lld\n", nOrderId);

    // 等待委托确认回调
    Sleep(1000);

    // 2. 查询委托状态确认可撤
    printf("\n2. 查询委托状态...\n");
    vector<ITPDK_DRWT> orders;
    int64 nRet = SECITPDK_QueryOrdersEx(g_CusReqInfo, 0, 0, 10, 0, sJys, sZqdm, nOrderId, orders);
    if (nRet > 0 && !orders.empty()) {
        auto& order = orders[0];
        printf("委托状态: 当前数量=%lld, 已成交=%lld, 冻结金额=%.2f\n",
               (long long)order.OrderQty, (long long)order.MatchQty, order.FrozenBalance);
        if (order.OrderQty > order.MatchQty) {
            printf("委托可撤销\n");
        }
    }

    // 3. 撤销委托 - 方式A: 使用系统委托号(WTH)
    printf("\n3. 撤销委托 (使用WTH=%lld)...\n", nOrderId);
    if (g_UseEx) {
        nRet = SECITPDK_OrderWithdrawEx(g_CusReqInfo, sJys, nOrderId);
    } else {
        nRet = SECITPDK_OrderWithdraw(g_Khh.c_str(), sJys, nOrderId);
    }

    if (nRet > 0) {
        printf("撤单指令发送成功, 撤单流水号=%lld\n", nRet);
    } else {
        printf("撤单失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
    }

    // 等待撤单确认回调
    Sleep(1000);

    // 4. 验证撤单结果
    printf("\n4. 验证撤单结果...\n");
    orders.clear();
    nRet = SECITPDK_QueryOrdersEx(g_CusReqInfo, 0, 0, 10, 0, sJys, sZqdm, nOrderId, orders);
    if (nRet > 0 && !orders.empty()) {
        auto& order = orders[0];
        printf("最新状态: 当前数量=%lld, 已撤数量=%lld\n",
               (long long)order.OrderQty, (long long)order.WithdrawQty);
    }
}

// 演示：发送委托并使用自定义流水号(KFSBDBH)撤销
void DemoOrderWithCustomIdAndWithdraw() {
    printf("\n========== 使用自定义流水号(KFSBDBH)发送并撤销 ==========\n");
    
    const char* sJys = "SZ";
    const char* sZqdm = "000001";
    int nJylb = JYLB_SALE;
    int64 nWtsl = 200;
    double dWtjg = 12.80;
    int64 nDdlx = DDLX_LIMIT;
    const char* sGdh = "B987654321";
    int64 nKFSBDBH = 999888777;  // 自定义流水号

    printf("\n1. 发送卖出委托 (KFSBDBH=%lld)...\n", nKFSBDBH);
    long long nRet = 0;
    
    if (g_UseEx) {
        nRet = SECITPDK_OrderEntrustEx_ASync(g_CusReqInfo, sJys, sZqdm, nJylb, nWtsl, dWtjg, nDdlx, nKFSBDBH, 0, 0);
    } else {
        nRet = SECITPDK_OrderEntrust_ASync(g_Khh.c_str(), sJys, sZqdm, nJylb, nWtsl, dWtjg, nDdlx, sGdh, nKFSBDBH, 0, 0);
    }

    if (nRet <= 0) {
        printf("异步委托发送失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
        return;
    }
    printf("异步委托发送成功, 批次号=%lld\n", nRet);

    // 等待异步回调返回真实OrderId
    Sleep(2000);

    // 2. 使用KFSBDBH撤单
    printf("\n2. 使用KFSBDBH=%lld 撤销委托...\n", nKFSBDBH);
    if (g_UseEx) {
        nRet = SECITPDK_OrderWithdrawByKFSBDBHEx(g_CusReqInfo, sJys, nKFSBDBH);
    } else {
        nRet = SECITPDK_OrderWithdrawByKFSBDBH(g_Khh.c_str(), sJys, nKFSBDBH);
    }

    if (nRet > 0) {
        printf("撤单指令发送成功\n");
    } else {
        printf("撤单失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
    }

    Sleep(1000);
}

// 演示：批量下单并批量撤单
void DemoBatchOrderAndWithdraw() {
    printf("\n========== 批量下单并批量撤单 ==========\n");
    
    BatchOrderInfo orders[2] = {0};
    
    // 订单1
    strncpy(orders[0].Jys, "SH", sizeof(orders[0].Jys));
    strncpy(orders[0].Zqdm, "600000", sizeof(orders[0].Zqdm));
    orders[0].Jylb = JYLB_BUY;
    orders[0].Wtsl = 100;
    orders[0].Wtjg = 10.00;
    orders[0].Ddlx = DDLX_LIMIT;
    strncpy(orders[0].Gdh, "A111111111", sizeof(orders[0].Gdh));
    orders[0].kfsbdbh = 1001;
    orders[0].Tdbh = 0;

    // 订单2
    strncpy(orders[1].Jys, "SH", sizeof(orders[1].Jys));
    strncpy(orders[1].Zqdm, "600036", sizeof(orders[1].Zqdm));
    orders[1].Jylb = JYLB_BUY;
    orders[1].Wtsl = 200;
    orders[1].Wtjg = 35.00;
    orders[1].Ddlx = DDLX_LIMIT;
    strncpy(orders[1].Gdh, "A111111111", sizeof(orders[1].Gdh));
    orders[1].kfsbdbh = 1002;
    orders[1].Tdbh = 0;

    vector<BatchOrderInfo> vecOrders(orders, orders + 2);
    int64 nWTPCH = 20260928001;  // 批次号

    printf("\n1. 批量下单 (批次=%lld)...\n", nWTPCH);
    long long nRet = 0;
    if (g_UseEx) {
        nRet = SECITPDK_BatchOrderEntrustEx(g_CusReqInfo, vecOrders, nWTPCH);
    } else {
        nRet = SECITPDK_BatchOrderEntrust(g_Khh.c_str(), vecOrders, nWTPCH);
    }

    if (nRet == 0) {
        printf("批量下单全部成功\n");
        for (auto& o : vecOrders) {
            printf("  KFSBDBH=%lld -> WTH=%lld\n", (long long)o.kfsbdbh, (long long)o.Wth);
        }
    } else if (nRet > 0) {
        printf("批量下单部分失败, 失败数=%lld\n", nRet);
        for (auto& o : vecOrders) {
            if (o.Wth <= 0) printf("  KFSBDBH=%lld 失败: %s\n", (long long)o.kfsbdbh, o.Msg);
        }
    } else {
        printf("批量下单整体失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
        return;
    }

    Sleep(2000);

    // 批量撤单
    printf("\n2. 批量撤单 (批次=%lld)...\n", nWTPCH);
    if (g_UseEx) {
        nRet = SECITPDK_BatchOrderWithdrawEx(g_CusReqInfo, nWTPCH);
    } else {
        nRet = SECITPDK_BatchOrderWithdraw(g_Khh.c_str(), nWTPCH);
    }

    if (nRet > 0) {
        printf("批量撤单指令发送成功\n");
    } else {
        printf("批量撤单失败: %s\n", SECITPDK_GetLastErrorStr().c_str());
    }

    Sleep(1000);
}

// 演示：一键撤单
void DemoOneKeyWithdraw() {
    printf("\n========== 一键撤单演示 ==========\n");
    
    vector<ITPDK_CANBE_WITHDRAW> withdrawList;
    
    // 查询可撤单列表
    printf("查询可撤委托...\n");
    long long nRet = SECITPDK_OnekeyWithdraw(g_Khh.c_str(), "A123456789", "SH", JYLB_BUY, "A", "", withdrawList);
    
    if (nRet <= 0) {
        printf("查询可撤单失败或无可撤单: %s\n", SECITPDK_GetLastErrorStr().c_str());
        return;
    }

    printf("找到 %zu 笔可撤委托:\n", withdrawList.size());
    for (auto& w : withdrawList) {
        printf("  WTH=%lld, ZQDM=%s, 可撤数量=%lld\n", 
               (long long)w.WTH, w.StockCode, (long long)w.CanWithdrawQty);
    }

    // 执行一键撤单（实际撤单需要逐笔调用或使用批量撤单）
    // 这里演示逐笔撤销
    for (auto& w : withdrawList) {
        if (w.CanWithdrawQty > 0) {
            printf("撤单: WTH=%lld...\n", (long long)w.WTH);
            nRet = SECITPDK_OrderWithdraw(g_Khh.c_str(), "SH", w.WTH);
            if (nRet > 0) printf("  撤单指令成功\n");
            Sleep(200);
        }
    }
}

int main() {
    printf("HTS ITPDK 交易下单与撤单演示\n");
    printf("==============================\n");

    char version[64] = {0};
    SECITPDK_GetVersion(version);
    printf("SDK版本: %s\n", version);

    SetupCallbacks();

    if (!Initialize()) {
        return -1;
    }

    // 演示1: 普通下单并撤销
    DemoOrderAndWithdraw();

    // 演示2: 使用自定义流水号
    DemoOrderWithCustomIdAndWithdraw();

    // 演示3: 批量下单撤单
    DemoBatchOrderAndWithdraw();

    // 演示4: 一键撤单
    DemoOneKeyWithdraw();

    Cleanup();
    
    printf("\n演示完成，按任意键退出...\n");
    getchar();
    return 0;
}