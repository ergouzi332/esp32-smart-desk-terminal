#include "bsp/board.h"
#include "drv/max30102_drv.h"
#include <Wire.h>
#include <Arduino.h>

static TwoWire &maxWire = Wire1;//使用第二路I2C总线


#define REG_PART_ID       0xFF    //芯片型号ID寄存器
#define REG_REV_ID        0xFE    //芯片版本ID寄存器
#define REG_FIFO_WR_PTR   0x04    //FIFO写指针寄存器
#define REG_FIFO_OVF_COUNTER 0x05 //FIFO溢出计数寄存器
#define REG_FIFO_RD_PTR   0x06    //FIFO读指针寄存器
#define REG_FIFO_DATA     0x07    //FIFO数据寄存器
#define REG_FIFO_CONFIG   0x08    //FIFO配置寄存器
#define REG_MODE_CONFIG   0x09    //工作模式配置寄存器
#define REG_SPO2_CONFIG   0x0A    //血氧功能配置寄存器
#define REG_LED1_PA       0x0C    //LED1驱动电流配置
#define REG_LED2_PA       0x0D    //LED2驱动电流配置
#define REG_PILOT_PA      0x10    //检测指示灯电流配置

MAX30102_Typedef MAX30102_State;
static bool MAX30102_Ready = false;  // 设备就绪标志

static const int BUFFER_SIZE = 100;		// 红外数据缓存长度，存储100组采样数据
static uint32_t irBuffer[BUFFER_SIZE];
static int irIndex = 0;					// 数据缓存索引，标记当前存储位置
static uint32_t lastBeatTime = 0;		// 上一次心跳时间戳

// MAX30102 寄存器写函数
static bool writeReg(uint8_t reg, uint8_t val)
{
	maxWire.beginTransmission(MAX30102_I2C_ADDR);
	maxWire.write(reg);
	maxWire.write(val);
	return maxWire.endTransmission() == 0;  // 传输完成返回0表示成功
}

// MAX30102 寄存器读函数
static int readReg(uint8_t reg)
{
	maxWire.beginTransmission(MAX30102_I2C_ADDR);
	maxWire.write(reg);
	if (maxWire.endTransmission(false) != 0) {
		return -1;
	}
	maxWire.requestFrom((int)MAX30102_I2C_ADDR, (int)1); // 从指定寄存器读取1字节数据
	if (maxWire.available()) return maxWire.read(); // 读取单字节数据
	return -1;
}

// 读取FIFO采样数据
static long readFIFOsample()
{
	// 设备未就绪直接返回
	if (!MAX30102_Ready) {
		return -1;
	}
	// 读取FIFO读写指针
	int writePtr = readReg(REG_FIFO_WR_PTR);  // FIFO写指针
	int readPtr = readReg(REG_FIFO_RD_PTR);   // FIFO读指针
	if (writePtr < 0 || readPtr < 0) {
		return -1;
	}
	
	int available = (writePtr - readPtr + 32) % 32;  // 计算FIFO有效数据个数(芯片FIFO深度32)
	if (available < 2)  // 数据不足2组(红光+红外)，直接返回
	{
		return -2;
	}
    // 连续读取6字节FIFO数据
	maxWire.beginTransmission(MAX30102_I2C_ADDR);
	maxWire.write(REG_FIFO_DATA);
	int err = maxWire.endTransmission(false);
	if (err != 0) {
		Serial.print("MAX30102: FIFO register write failed, err=");
		Serial.println(err);
		return -1;
	}
	int avail = maxWire.requestFrom((int)MAX30102_I2C_ADDR, (int)6);
	if (avail < 6) {
		Serial.print("MAX30102: FIFO read available=");
		Serial.println(avail);
		return -1;
	}
	
	long redSample = 0;
	redSample = ((long)maxWire.read() & 0xFF) << 16;  // 红光数据高8位
	redSample |= ((long)maxWire.read() & 0xFF) << 8;   // 红光数据中8位
	redSample |= ((long)maxWire.read() & 0xFF);		 // 红光数据低8位
	redSample &= 0x3FFFF;							 // 屏蔽无效位，保留18位有效数据
	
	long irSample = 0;
	irSample = ((long)maxWire.read() & 0xFF) << 16;  // 红外数据高8位
	irSample |= ((long)maxWire.read() & 0xFF) << 8;   // 红外数据中8位
	irSample |= ((long)maxWire.read() & 0xFF);	   // 红外数据低8位
	irSample &= 0x3FFFF;

	if (irSample == 0 || irSample == 0x3FFFF) {
		// 红外数据异常，尝试使用红光数据
		if (redSample != 0 && redSample != 0x3FFFF) {
			return redSample;
		}
	}
	return irSample;  // 优先返回红外采样数据
}

// MAX30102 初始化函数
void MAX30102_Init(void)
{
	MAX30102_Ready = false;
	// 初始化I2C，配置SDA/SCL引脚
	maxWire.begin(MAX30102_I2C_SDA, MAX30102_I2C_SCL);
	maxWire.setClock(100000);
	// 初始化状态变量
	MAX30102_State.heartRate = 0;
	MAX30102_State.spo2 = 0;
	MAX30102_State.valid = false;
	irIndex = 0;
	// 读取芯片ID，检测设备是否在线
	int partId = readReg(REG_PART_ID);
	if (partId < 0) {
		Serial.println("MAX30102: I2C device not found; check wiring and address.");
		MAX30102_Ready = false;
		return;
	}
	MAX30102_Ready = true;

	// 芯片复位
	writeReg(REG_MODE_CONFIG, 0x40);
	delay(100);
	
	writeReg(REG_FIFO_CONFIG, 0x1F);
	// 配置两路LED驱动电流
	writeReg(REG_LED1_PA, 0x20);  // 红光LED电流
	writeReg(REG_LED2_PA, 0x7F);  // 红外LED电流
	// 血氧模式配置，18bit分辨率、100Hz采样率
	writeReg(REG_SPO2_CONFIG, 0x27);
	
	writeReg(REG_MODE_CONFIG, 0x02);
	delay(200);
	
	// 清空FIFO读写指针与溢出计数
	writeReg(REG_FIFO_WR_PTR, 0);
	writeReg(REG_FIFO_OVF_COUNTER, 0);
	writeReg(REG_FIFO_RD_PTR, 0);
	delay(200);
	
	int wrPtr = readReg(REG_FIFO_WR_PTR);
	int rdPtr = readReg(REG_FIFO_RD_PTR);
	// 指针清零成功，切换为双LED正常工作模式
	if (wrPtr == 0 && rdPtr == 0) {
		// 提升LED电流，进入正式检测模式
		writeReg(REG_LED1_PA, 0x7F);
		writeReg(REG_LED2_PA, 0x7F);
		writeReg(REG_MODE_CONFIG, 0x03);
		delay(500);
	}
}

// 心跳检测算法
static bool detectBeat(long value)
{
	// 取前20组数据计算平均值
	uint32_t sum = 0;
	int cnt = 0;
	for (int i = 1; i <= 20; i++) {
		int j = (irIndex - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
		sum += irBuffer[j];
		cnt++;
	}
	uint32_t avg = (cnt > 0) ? sum / cnt : 0;

	static bool above = false;       // 波形上升标志
	static long prevValue = 0;       // 上一次采样值
	const uint32_t threshold = avg + 40;  // 检测阈值 = 平均值 + 偏移量
	const uint32_t minRise = 18;         // 最小上升幅度
	uint32_t rise = (value > prevValue) ? (uint32_t)(value - prevValue) : 0;  // 本次波形上升幅度

	// 满足阈值、上升幅度条件，判定为心跳上升沿
	if (value > threshold && rise > minRise) {
		if (!above) {
			above = true;
			prevValue = value;
			return true;
		}
	} else {
		above = false; // 波形回落，清除标志
	}
	prevValue = value;
	return false;  // 未检测到心跳
}

// 读取并解析MAX30102数据，计算心率
void readMAX30102(void)
{
	static unsigned long lastReadMillis = 0;
	const unsigned long minInterval = 25;  // 最小读取间隔25ms
	unsigned long nowMs = millis();
	if (nowMs - lastReadMillis < minInterval) {
		return;
	}
	lastReadMillis = nowMs;

	// 设备未就绪直接退出
	if (!MAX30102_Ready) {
		static bool warned = false;
		if (!warned) {
			warned = true;
		}
		MAX30102_State.valid = false;
		return;
	}
	// 读取一组FIFO采样数据
	long sample = readFIFOsample();
	if (sample == -2 || sample < 0) {
		MAX30102_State.valid = false;
		return;
	}
	// 剔除无效极值数据
	if (sample == 0x3FFFF || sample == 0) {
		MAX30102_State.valid = false;
		return;
	}
    // 数据存入循环缓存
	irBuffer[irIndex++] = (uint32_t)sample;
	if (irIndex >= BUFFER_SIZE) irIndex = 0;

	// 获取当前最新一组红外数据
	uint32_t irVal = irBuffer[(irIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE];
	
	if (irVal < 30000) {
		MAX30102_State.valid = false;
		return;
	}
	
	// 心跳判定
	if (detectBeat(irVal)) {
		uint32_t now = millis();
		if (lastBeatTime != 0) {
			uint32_t delta = now - lastBeatTime;  // 两次心跳时间间隔(ms)
			if (delta > 300 && delta < 3000)      // 合法心率区间：300ms ~ 3000ms
			{
				int bpm = (int)(60000.0 / delta);
				// 4次滑动平均滤波，平滑心率数值
				static int prevBpm[4] = {0,0,0,0};
				static int spot = 0;
				prevBpm[spot++] = bpm;
				spot %= 4;
				int s = 0; for (int i = 0; i < 4; i++) s += prevBpm[i];
				int avgBpm = s / 4;
				MAX30102_State.heartRate = avgBpm;
				MAX30102_State.valid = true;
			}
		}
		lastBeatTime = now;
	}
}