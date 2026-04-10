#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/socket.h>

// BLE广播数据：固定格式，最后1字节为音色编号
uint8_t adv_data[] = {
    0x02, 0x01, 0x06,          // BLE广播标志
    0x03, 0x03, 0xAA, 0xFE,    // 自定义服务UUID
    0x10, 0x09, 'P','i','a','n','o','L','i','g','h','t','B','L','E', // 设备名
    0x05, 0xFF, 0x12, 0x34, 0x00, 0x00 // 厂商数据：最后1字节=音色编号
};

int main() {
    int sock = hci_open_dev(hci_get_route(NULL));
    if (sock < 0) { perror("hci_open_dev"); return 1; }

    // 配置广播参数
    le_set_advertising_parameters_cp adv_param;
    memset(&adv_param, 0, sizeof(adv_param));
    adv_param.min_interval = htobs(0x0080); // 80ms广播间隔
    adv_param.max_interval = htobs(0x0080);
    adv_param.adv_type = 0x00; // 非定向广播
    adv_param.own_bdaddr_type = 0x00;

    if (hci_le_set_advertising_parameters(sock, &adv_param, 1000) < 0) {
        perror("hci_le_set_advertising_parameters");
        return 1;
    }

    // 开启广播
    le_set_advertise_enable_cp adv_enable;
    memset(&adv_enable, 0, sizeof(adv_enable));
    adv_enable.enable = 0x01;
    if (hci_le_set_advertise_enable(sock, &adv_enable, 1000) < 0) {
        perror("hci_le_set_advertise_enable");
        return 1;
    }

    int current_tone = 0;
    while (true) {
        // ==============================================
        // 在这里根据你的FFT识别结果更新current_tone
        // 0=无声音, 1=低音, 2=中音, 3=高音, 可自行扩展
        // ==============================================
        
        // 更新广播数据中的音色编号
        adv_data[sizeof(adv_data)-1] = static_cast<uint8_t>(current_tone);
        hci_le_set_advertising_data(sock, sizeof(adv_data), adv_data, 1000);
        
        usleep(50000); // 50ms刷新一次，保证实时性
    }

    close(sock);
    return 0;
}