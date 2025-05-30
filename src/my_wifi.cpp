#include "../include/my_wifi.hpp"

static MY_WIFI_CFG cfg;
IPAddress local_IP(192, 168, 5, 254);
IPAddress gateway(192, 168, 5, 1);
IPAddress subnet(255, 255, 248, 0); // /21 子网

static void connectWF(void){
    int count = 1;
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(my_wifi.cfg->name, my_wifi.cfg->passwd);
    Serial.print("连接到wifi [" + my_wifi.cfg->name + "]");
    while (WiFi.status() != WL_CONNECTED) {
        count ++;
        // if(Serial.available()){
        //     String getString = Serial.readString();
        //     if(getString == "del")
        //         my_fs.ops->clean("/config.json");
        //     else Serial.println("当前输入:"+getString+"\n输入del清除配置:");
        // }
        // if(count % 100 == 0){
        //     Serial.print(".");
        // }
        // if (count == 25000){
        //     // count = 0;
        //     Serial.println("WIFI连接超时!请检查wifi配置");
        //     Serial.println("WIFI名["+my_wifi.cfg->name+"]");
        //     Serial.println("WIFI密码["+my_wifi.cfg->passwd+"]");
        //     WiFi.begin(my_wifi.cfg->name, my_wifi.cfg->passwd);
        //     Serial.print("连接到wifi [" + my_wifi.cfg->name + "]");
        //     // while (Serial.available() == 0){
        //     //     Serial.print(".");
        //     //     delay(500);
        //     // }
        // }
        
        delay(1);
        if(count >= 50000){
            count = 0;
            Serial.println("WIFI连接失败!正在重启ESP32");
            pinMode(2, OUTPUT);
            digitalWrite(2,0);      //释放IO2
            delay(100);
            ESP.restart();
        }
    }

    my_wifi.cfg->ip = WiFi.localIP().toString();
    Serial.println("");
    Serial.println("WIFI已连接");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    delay(200);
}

static void setSSIDPasswd(String ssid, String passwd)
{
    my_wifi.cfg->name = ssid;
    my_wifi.cfg->passwd = passwd;
}

static MY_WIFI_OPS ops = {
    .connect = connectWF,
    .setSSID = setSSIDPasswd,
};

MY_WIFI my_wifi = {
    .cfg = &cfg,
    .ops = &ops,
};
