# lcd_firmware

mipi lcd epprom固件及其烧录工具

## 编译

确认需要烧录的板卡以及接口，修改firmware_burn.c

```
// rk3562、rk3576、rk3588
#define NVMEM_PATH      "/sys/bus/nvmem/devices/eeprom1/nvmem"
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/eeprom2/nvmem"

// rk356x dsi0
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/1-00510/nvmem"
// rk356x dsi1
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/5-00510/nvmem"
```

- 当rk3576、rk3588板卡只接一个屏幕时eeprom无论是哪个接口都注册为eeprom1，当接两个屏幕时，dsi0注册为eeprom1、dsi1注册为eeprom2。
- rk356x板卡根据i2c总线进行注册，dsi0为1-00510表示i2c1上的0x51地址的设备、dsi1为5-00510表示i2c5上的0x51地址的设备。

修改定义后，直接make编译即可
```
# 交叉编译arm64版本
make CROSS_COMPILE=aarch64-linux-gnu-

# 交叉编译armhf版本
make CROSS_COMPILE=arm-linux-gnueabihf-

# 在板子上编译
make 
```

编译得到的 firmware_burn 就是可烧录屏幕eeprom固件的可执行程序。而Makefile里面定义了板卡ip地址，修改为实际板卡ip，可直接将firmware_burn传到板卡。

## 烧录固件

开启xxx-generic-overlay.dtbo通用屏幕插件，重启板卡

然后运行firmware_burn，输入需要屏幕编号，如EBF410173_7inch_1024x600

```
root@lubancat:/home/cat# ./firmware_burn

===============================================
LCD board list:

[1] EBF410125v1_5.5inch_1080x1920
[2] EBF410125_5.5inch_1080x1920
[3] EBF410173_7inch_1024x600
[4] EBF410177_10.1inch_800x1280
[5] EBF410574_7inch_800x1280
[6] EBF410575_10.1inch_800x1280
[7] EBF410630_3.5inch_320x480
[8] lianxin_s8001280b_10.1inch_800x1280

Which board would you like to burn? [1-8]: 3

Selected board: EBF410173_7inch_1024x600
LCD vendor: Embedfire
LCD model: STL7.0-60-132-K
Firmware version: v1.0
Firmware Burning ...

Firmware burning successful

```

烧录完重启板卡即可。

## 自动烧录固件

如鲁班猫2的板卡同时烧录2个屏幕的eeprom，分别选择rk356x dsi0、dsi1定义进行编译，将firmware_burn分别重命名为firmware_burn_dsi0、firmware_burn_dsi1

然后编写firmware_burn.sh，内容如下，注意根据实际屏幕修改参数：

```
#!/bin/bash

FLAG_FILE="/home/cat/eeprom_burn.flag"
NVMEM_PATH_0="/sys/bus/nvmem/devices/1-00510/nvmem"
NVMEM_PATH_1="/sys/bus/nvmem/devices/5-00510/nvmem"

if [ ! -f "$FLAG_FILE" ]; then
    echo 'not_burned' > "$FLAG_FILE"
fi

FLAG_STATUS=$(cat "$FLAG_FILE" 2>/dev/null)

if [ "$FLAG_STATUS" = "not_burned" ]; then

    if [ -f "$NVMEM_PATH_0" ]; then
        # 根据实际屏幕修改参数，此处5.5 v1
        echo "1" | /home/cat/firmware_burn_dsi0
    fi

    if [ -f "$NVMEM_PATH_1" ]; then
        # 根据实际屏幕修改参数，此处5.5 v1
        echo "1" | /home/cat/firmware_burn_dsi1
    fi

    echo "burned" > "$FLAG_FILE"
    sync
    reboot
elif [ "$FLAG_STATUS" = "burned" ]; then
    echo 'not_burned' > "$FLAG_FILE"
    sync
else
    echo 'not_burned' > "$FLAG_FILE"
    sync
    reboot
fi
```

添加执行权限：

```
sudo chmod 777 firmware_burn.sh
```

创建/etc/systemd/system/firmware_burn.service，内容如下：

```
[Unit]
Description = firmware_burn

[Service]
ExecStart = /home/cat/firmware_burn.sh
Restart = never
Type = oneshot

[Install]
WantedBy = multi-user.target
```

使能服务：

```
sudo systemctl enable firmware_burn.service
sudo systemctl start firmware_burn.service
```


以上，第一次启动时自动烧录固件，重置标志状态为burned，表示下次启动时无需烧录固件，烧录固件完成后自动重启；第二次启动时，重置标志状态为not_burned，表示下次启动时需烧录固件，此次启动不会自动重启，可观察屏幕有没有成功点亮；以此循环。

## 自动绑定触摸

当同时开启多个屏幕时需要绑定触摸到对应的屏幕，可使用桌面系统方式，当进入桌面时自动运行绑定脚本从而绑定触摸。

在桌面登录用户的家目录下创建启动文件：

```
#以cat用户为例
mkdir /home/cat/.config/autostart

#创建配置文件
vim /home/cat/.config/autostart/bind_touch.desktop
```

在bind_touch.desktop文件中添加以下内容：

```
[Desktop Entry]
Type=Application
Exec=/home/cat/bind_touch.sh
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
Name=My App
Comment=Start My App on login
```

根据上面的自启动配置，创建自启动脚本，cd进入/home/cat/目录下，编写一个bind_touch.sh脚本，脚本内容如下：

```
#!/bin/bash

export DISPLAY=:0.0

TOUCH_IDS=($(xinput list | \
    awk '/Virtual core pointer/,/Virtual core keyboard/ { 
        if ($0 ~ /Goodix Capacitive TouchScreen/ && $0 ~ /id=/) {       
        split($0, arr, "id=");       
        split(arr[2], num, " ");     
        print num[1];               
        }
    }' | grep -E '^[0-9]+$'))               

DSI_LIST=($(xrandr -q | grep "DSI-[0-9] connected" | awk '{print $1}'))

if [ ${#TOUCH_IDS[@]} -ge 2 ] && [ ${#DSI_LIST[@]} -ge 2 ]; then
    # echo "绑定触摸屏ID ${TOUCH_IDS[0]} 到 ${DSI_LIST[0]}"
    xinput map-to-output ${TOUCH_IDS[0]} ${DSI_LIST[0]}
    # echo "绑定触摸屏ID ${TOUCH_IDS[1]} 到 ${DSI_LIST[1]}"
    xinput map-to-output ${TOUCH_IDS[1]} ${DSI_LIST[1]}
else
    TARGET_DSI=""
    for dsi in "${DSI_LIST[@]}"; do
        RES_COUNT=0
        IN_TARGET_DSI=0

        while IFS= read -r line; do
        if echo "$line" | grep -q "^$dsi connected"; then
            IN_TARGET_DSI=1
            continue
        fi
        if echo "$line" | grep -q "^DSI-[0-9] connected" && ! echo "$line" | grep -q "^$dsi connected"; then
            IN_TARGET_DSI=0
            continue
        fi
        if [ $IN_TARGET_DSI -eq 1 ] && echo "$line" | grep -q '[0-9]\+x[0-9]\+'; then
            RES_COUNT=$((RES_COUNT + 1))
        fi
        done <<< "$(xrandr -q)"
        
        if [ $RES_COUNT -eq 1 ]; then
        TARGET_DSI=$dsi
        break
        fi
    done
    
    if [ -z "$TARGET_DSI" ]; then
        TARGET_DSI="DSI-1"
        if ! echo "${DSI_LIST[*]}" | grep -q "DSI-2"; then
        TARGET_DSI=${DSI_LIST[0]}
        fi
    fi
    
    if [ ${#TOUCH_IDS[@]} -ge 1 ]; then
        MAIN_TOUCH_ID=${TOUCH_IDS[0]}
        # echo "绑定触摸屏ID $MAIN_TOUCH_ID 到 $TARGET_DSI"
        xinput map-to-output $MAIN_TOUCH_ID $TARGET_DSI
    else
        # echo "错误：未找到任何Goodix触摸屏ID"
        exit 1
    fi
fi
```

以上脚本实现的功能是如果存在两个屏幕则顺序绑定触摸，如果存在单个屏幕则绑定到对应的屏幕。

添加脚本执行权限：

```
sudo chmod 777 /home/cat/bind_touch.sh
```

需注意，要进入桌面才会自动运行bind_touch.sh脚本，需自行添加桌面自动登录。