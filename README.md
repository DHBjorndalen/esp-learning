# ESP32 Learning Projects

| Project | Concepts | Status |
|---------|----------|--------|
| blink | GPIO, FreeRTOS delays | Complete |


```
cd ~/esp/esp-learning
git add .
git commit -m "commit msg"
git push

### New project setup
```bash
cd ~/esp/esp-learning
idf.py create-project 
cd 
idf.py set-target esp32
idf.py build
```

### Flash and monitor
```bash
idf.py flash monitor
idf.py -p /dev/ttyUSB0 flash monitor   # if port not detected automatically
```

### Useful commands
```bash
idf.py build          # compile project
idf.py flash          # flash to board
idf.py monitor        # open serial monitor
idf.py fullclean      # delete build folder and start fresh
idf.py menuconfig     # open project configuration menu
```

## Learning Roadmap

### Stage 1 — Core ESP-IDF Fundamentals
- [x] Blink (GPIO output)
- [ ] GPIO serial control (UART input/output)
- [ ] Button input with debounce (GPIO input) *requires button*
- [ ] PWM LED brightness control (onboard LED)
- [ ] Multiple GPIO pins controlled via serial commands

### Stage 2 — Connectivity
- [ ] Connect to WiFi
- [ ] Serve a simple webpage from the ESP32
- [ ] HTTP GET request to an external API
- [ ] MQTT messaging

### Stage 3 — Real-Time and Reliability
- [ ] FreeRTOS two tasks running simultaneously
- [ ] Task queues passing data between tasks
- [ ] Semaphores
- [ ] Deep sleep and wake on timer
- [ ] Non-volatile storage (NVS)

### Stage 4 — Product Grade Code
- [ ] Multi-file project structure with header files
- [ ] Error handling and logging
- [ ] OTA over the air firmware updates
- [ ] Unit testing with ESP-IDF test framework

### Stage 5 — Robotics Foundation
*Requires additional hardware — revisit when parts available*
- [ ] PWM motor control
- [ ] Servo control
- [ ] Encoder feedback
- [ ] ESP32 and Raspberry Pi communication
- [ ] PID control loop