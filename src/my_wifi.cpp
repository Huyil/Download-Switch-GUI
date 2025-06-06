#include "../include/my_wifi.hpp"

static MY_WIFI_CFG cfg;
IPAddress local_IP(192, 168, 5, 254);
IPAddress gateway(192, 168, 5, 1);
IPAddress subnet(255, 255, 248, 0); // /21 子网

// 回调函数，监听WiFi事件
void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi 已连接");
      Serial.print("IP 地址: ");
      Serial.println(WiFi.localIP());
      // 这里可以更新状态，比如调用showWifiState(true);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi 断开连接，正在重连...");
      WiFi.reconnect();  // 自动重连
      // 这里可以更新状态，比如调用showWifiState(false);
      break;
    default:
      break;
  }
}

static void connectWF(void) {
    WiFi.config(local_IP, gateway, subnet);
    WiFi.onEvent(WiFiEvent);   // 注册事件回调
    WiFi.begin(my_wifi.cfg->name.c_str(), my_wifi.cfg->passwd.c_str());

    Serial.print("尝试连接WiFi: ");
    Serial.println(my_wifi.cfg->name);

    // 不阻塞，直接返回，连接状态由回调管理
}

static void setSSIDPasswd(String ssid, String passwd)
{
    my_wifi.cfg->name = ssid;
    my_wifi.cfg->passwd = passwd;
}

static bool isConnected(void) {
    return WiFi.isConnected();
}

static MY_WIFI_OPS ops = {
    .isconnected = isConnected,
    .connect = connectWF,
    .setSSID = setSSIDPasswd,
};

MY_WIFI my_wifi = {
    .cfg = &cfg,
    .ops = &ops,
};
