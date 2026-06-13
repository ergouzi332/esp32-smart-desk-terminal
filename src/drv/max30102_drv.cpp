#include "drv/max30102_drv.h"
#include "bsp/board.h"
#include <Wire.h>
#include <Arduino.h>

static TwoWire &maxWire = Wire1;//缂佹瑧鈥栨禒绂?C1鐠у嘲鍩嗛崥宄琣xWire

//MAX30102閻ㄥ嚘2C瀵洝鍓?/


#define REG_PART_ID       0xFF    // 閼侯垳澧栭崹瀣娇ID鐎靛嫬鐡ㄩ崳顭掔礄閸ュ搫鐣鹃崐纭风礉閻劋绨拠鍡楀焼閺勵垯绗夐弰鐤X30102閿?
#define REG_REV_ID        0xFE    // 閼侯垳澧栭悧鍫熸拱ID鐎靛嫬鐡ㄩ崳顭掔礄閸樺倸顔嶇涵顑挎閻楀牊婀伴崣鍑ょ礆
#define REG_FIFO_WR_PTR   0x04    // FIFO 閸愭瑦瀵氶柦鍫礄娴肩姵鍔呴崳銊ㄥ殰閸斻劌绶氭潻娆撳櫡閸愭瑥鍙嗛弬鐗堟殶閹诡噯绱?
#define REG_FIFO_OVF_COUNTER 0x05 // FIFO 濠с垹鍤拋鈩冩殶閸ｎ煉绱欓弫鐗堝祦濠娾€茬啊濞屸€冲挤閺冩儼顕版导姘愁吀閺佸府绱?
#define REG_FIFO_RD_PTR   0x06    // FIFO 鐠囩粯瀵氶柦鍫礄閹存垳婊戞禒搴ょ箹闁插矁顕伴崣鏍ㄦ殶閹诡噯绱?
#define REG_FIFO_DATA     0x07    // FIFO 閺佺増宓佺€靛嫬鐡ㄩ崳顭掔礄閻喐顒滈惃鍕妇閻?鐞涒偓濮樠冨斧婵鏆熼幑顔肩摠閸︺劏绻栭柌宀嬬礆
#define REG_FIFO_CONFIG   0x08    // FIFO 闁板秶鐤嗙€靛嫬鐡ㄩ崳顭掔礄鐠佸墽鐤嗛弫鐗堝祦闁插洦鐗遍妴浣圭泊閸斻劍膩瀵骏绱?
#define REG_MODE_CONFIG   0x09    // 濡€崇础闁板秶鐤嗛敍鍫ｎ啎缂冾喖绺鹃悳鍥佸?鐞涒偓濮樠勀佸?瀵板懏婧€閿?
#define REG_SPO2_CONFIG   0x0A   // 鐞涒偓濮樠囧櫚閺嶇兘鍘ょ純顕嗙礄ADC缁儳瀹抽妴渚€鍣伴弽椋庡芳閵嗕浇鍓﹂崘鎻掝啍鎼达讣绱?
#define REG_LED1_PA       0x0C    // 缁俱垹顦婚悘?LED1)娴滎喖瀹?閻㈠灚绁﹂幒褍鍩楅敍鍫ｇШ婢堆備繆閸欑柉绉哄鐚寸礉娑旂喕绉洪懓妤冩暩閿?
#define REG_LED2_PA       0x0D    // 缁俱垹鍘滈悘?LED2)娴滎喖瀹?閻㈠灚绁﹂幒褍鍩?
#define REG_PILOT_PA      0x10    // 閻滎垰顣ㄩ崗澶嬪Х濞?閺嶁€冲櫙閻忣垳鏁稿ù渚婄礄娑撯偓閼割兛绗夐悽銊︽暭閿?


MAX30102_Typedef MAX30102_State;
static bool MAX30102_Ready = false;//娴肩姵鍔呴崳銊ユ皑缂侇亝鐖ｈ箛妞剧秴


static const int BUFFER_SIZE = 100;		//韫囧啰宸奸弫鐗堝祦缂傛挸鍟块崠鍝勩亣鐏忓骏绱扮€涙ê鍋嶉張鈧潻?00缂佸嫮瀛╂径鏍ㄦ殶閹?
static uint32_t irBuffer[BUFFER_SIZE];
static int irIndex = 0;					//缂傛挸鍟块崠铏瑰偍瀵洩绱扮拋鏉跨秿瑜版挸澧犻弫鐗堝祦鐠囥儱鐡ㄩ崗銉︽殶缂佸嫮娈戠粭顒€鍤戞稉顏冪秴缂?
static uint32_t lastBeatTime = 0;		//娑撳﹣绔村▎鈩冾梾濞村鍩岃箛鍐儲閻ㄥ嫭妞傞梻瀛樺煈

//閸氭厷AX30102閻ㄥ嫭瀵氱€规艾鐦庣€涙ê娅掗崘娆忓弳娑撯偓娑擃亜鐡ч懞鍌涙殶閹?/
/* MAX30102 寄存器写入（I2C地址 0x57）*/
static bool writeReg(uint8_t reg, uint8_t val)
{
	maxWire.beginTransmission(MAX30102_I2C_ADDR);
	maxWire.write(reg);
	maxWire.write(val);
	return maxWire.endTransmission() == 0;//鏉╂柨娲栭弰顖氭儊閸愭瑥鍙嗛幋鎰
}


/* MAX30102 寄存器读取 */
static int readReg(uint8_t reg)
{
	maxWire.beginTransmission(MAX30102_I2C_ADDR);
	maxWire.write(reg);
	if (maxWire.endTransmission(false) != 0) {
		return -1;
	}
	maxWire.requestFrom((int)MAX30102_I2C_ADDR, (int)1); //閸氭垳绱堕幇鐔锋珤鐠囬攱鐪扮拠璇插絿 1 鐎涙濡弫鐗堝祦
	if (maxWire.available()) return maxWire.read(); //婵″倹鐏夐張澶嬫殶閹诡喖褰查悽顭掔礉鐏忚精顕伴崣鏍ц嫙鏉╂柨娲?
	return -1;
}

//娴犲懂AX30102 FIFO鐠囪褰囨稉鈧紒鍕壉閺?/
/* 读取 MAX30102 FIFO 采样数据 */
static long readFIFOsample()
{
	//閸掋倖鏌囬弰顖氭儊閸掓繂顫愰崠?/
	if (!MAX30102_Ready) {
		return -1;
	}
	//鐠囪鍟撻幐鍥嫛閼惧嘲褰?/
	int writePtr = readReg(REG_FIFO_WR_PTR);//鐠囪褰嘑IFO閸愭瑦瀵氶柦?
	int readPtr = readReg(REG_FIFO_RD_PTR);//鐠囪褰嘑IFO鐠囩粯瀵氶柦?
	if (writePtr < 0 || readPtr < 0) {
		return -1;
	}
	
	int available = (writePtr - readPtr + 32) % 32;//鐠侊紕鐣籉IFO娑擃厼褰查悽銊ф畱閺佺増宓侀弫浼村櫤(閻滎垰鑸扮紓鎾冲暱閸?
	if (available < 2) //1濞嗏剝婀侀弫鍫ュ櫚閺?缁俱垹顦婚崗?缁俱垹鍘?閸楃姷鏁?娑撶嫢IFO娴ｅ秶鐤?
	{
	return -2;
	}
    //鏉╃偟鐢荤拠璇插絿 6 鐎涙濡崢鐔奉潗闁插洦鐗遍崐?/
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
	redSample = ((long)maxWire.read() & 0xFF) << 16;// 鐠囪崵顑?娑擃亜鐡ч懞?閳?瀹革妇些16娴?閳?閺€鎯у煂閺堚偓妤?娴ｅ秳缍呯純?
	redSample |= ((long)maxWire.read() & 0xFF) << 8;// 鐠囪崵顑?娑擃亜鐡ч懞?閳?瀹革妇些8娴? 閳?閺€鎯у煂娑擃參妫?娴?
	redSample |= ((long)maxWire.read() & 0xFF);		// 鐠囪崵顑?娑擃亜鐡ч懞?閳?閻╁瓨甯撮弨?   閳?閺堚偓娴?娴?
	redSample &= 0x3FFFF;							// 閸欘亙绻氶悾?8娴ｅ秵婀侀弫鍫熸殶閹诡噯绱橫AX30102閺?8娴ｅ伯DC閿?
	
	long irSample = 0;
	irSample = ((long)maxWire.read() & 0xFF) << 16;//缁俱垹顦?
	irSample |= ((long)maxWire.read() & 0xFF) << 8;//缁俱垹顦?
	irSample |= ((long)maxWire.read() & 0xFF);	   //缁俱垹顦?
	irSample &= 0x3FFFF;

	if (irSample == 0 || irSample == 0x3FFFF) {
		// 婵″倹鐏夌粭顑跨癌娑擃亝鐗遍張顒佹￥閺佸牞绱濈亸婵婄槸鐏忓棛顑囨稉鈧紒鍕壉閺堫剝顫嬫稉?IR
		if (redSample != 0 && redSample != 0x3FFFF) {
			return redSample;
		}
	}
	return irSample;//鏉╂柨娲栫痪銏狀樆
}


/* MAX30102 初始化：软复位、配置模式/LED亮度/FIFO */
void MAX30102_Init(void)
{
	MAX30102_Ready = false;
	//閸氼垰濮㊣2C閹崵鍤庨敍灞惧瘹鐎规瓔DA/SCL瀵洝鍓?/
	maxWire.begin(MAX30102_I2C_SDA, MAX30102_I2C_SCL);
	maxWire.setClock(100000);
	//濞撳懐鈹栭悩鑸碘偓浣稿綁闁?/
	MAX30102_State.heartRate = 0;
	MAX30102_State.spo2 = 0;
	MAX30102_State.valid = false;
	irIndex = 0;
	//鐠囪褰囨导鐘冲妳閸ｃ劏濮遍悧鍢擠閿涘本顥呴弻銉︽Ц閸氾箑婀痪?/
	int partId = readReg(REG_PART_ID);
	if (partId < 0) {
		Serial.println("MAX30102: I2C device not found; check wiring and address.");
		MAX30102_Ready = false;
		return;
	}
	MAX30102_Ready = true;

	//鏉烆垰顦叉担宥忕礄闁插秴鎯庢导鐘冲妳閸ｎ煉绱?/
	writeReg(REG_MODE_CONFIG, 0x40);
	delay(100);
	
	writeReg(REG_FIFO_CONFIG, 0x1F);
	//鐠佸墽鐤?LED 娴滎喖瀹抽敍鍫㈡暩濞翠緤绱?/
	writeReg(REG_LED1_PA, 0x20);//缁俱垻浼呴悽鍨ウ
	writeReg(REG_LED2_PA, 0x7F); //缁俱垹顦婚悘顖滄暩濞翠緤绱欓弴鏉戝繁閿?
	//SpO2 闁板秶鐤嗛敍?8娴ｅ秶绨挎惔锔肩礉100Hz 闁插洦鐗遍悳?/
	writeReg(REG_SPO2_CONFIG, 0x27);
	
	writeReg(REG_MODE_CONFIG, 0x02);
	//缁涘绶熸导鐘冲妳閸ｃ劌鎯庨崝銊╁櫚閺?/
	delay(200);

	
	writeReg(REG_FIFO_WR_PTR, 0);
	writeReg(REG_FIFO_OVF_COUNTER, 0);
	writeReg(REG_FIFO_RD_PTR, 0);
	delay(200);
	
	int wrPtr = readReg(REG_FIFO_WR_PTR);
	int rdPtr = readReg(REG_FIFO_RD_PTR);
	//婵″倹鐏?FIFO 娴犲秶鍔ф稉铏光敄閿涘矂鍣哥拠鏇氱濞嗏剝娲垮铏规畱闁板秶鐤?/
	if (wrPtr == 0 && rdPtr == 0) {
		// 婢跺洨鏁ら柊宥囩枂閿涙艾鍙忔禍?LED 楠炶泛鐨剧拠鏇熌佸?0x03
		writeReg(REG_LED1_PA, 0x7F);	//缁俱垻浼呭鈧崚鐗堟付婢?
		writeReg(REG_LED2_PA, 0x7F);	//缁俱垹顦婚悘顖氱磻閸掔増娓舵径?
		writeReg(REG_MODE_CONFIG, 0x03); //閸掑洦宕查崚鐗埬佸?
		delay(500);
	}
}

//閸掋倖鏌囬弰顖氭儊閺勵垰绺剧捄?/
/* 心搏检测：比较当前值是否超过平均值+阈值 */
static bool detectBeat(long value)
{
	//鐠侊紕鐣婚張鈧潻?0娑擃亜宸婚崣鍙夋殶閹诡喚娈戦獮鍐叉綆閸?/
	uint32_t sum = 0;
	int cnt = 0;
	for (int i = 1; i <= 20; i++) {
		int j = (irIndex - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
		sum += irBuffer[j];
		cnt++;
	}
	uint32_t avg = (cnt > 0) ? sum / cnt : 0;

	static bool above = false;//鐠佹澘缍嶈ぐ鎾冲閺勵垰鎯侀崷銊╂閸婇棿绗傞弬?
	static long prevValue = 0;//鐎涙ü绗傛稉鈧▎锛勬畱閺佹澘鈧》绱濋悽銊ょ艾鐠侊紕鐣绘稉濠傚磳楠炲懎瀹?
	const uint32_t threshold = avg + 40; // 闂冨牆鈧?= 楠炲啿娼庨崐?+ 閸ュ搫鐣鹃崑蹇曅?
	const uint32_t minRise = 18;         // 閺堚偓鐏忓繋绗傞崡鍥х畽鎼达讣绱濋柆鍨帳閹舵牕濮╃拠顖澬曢崣?
	uint32_t rise = (value > prevValue) ? (uint32_t)(value - prevValue) : 0;// 鐠侊紕鐣昏ぐ鎾冲閸婂吋鐦稉濠佺濞嗏€茬瑐閸楀洣绨℃径姘毌
	// 閺夆€叉1閿涙艾缍嬮崜宥呪偓?> 楠炲啿娼庨崐濂告閸?
	// 閺夆€叉2閿涙碍鏆熼崐鐓庢彥闁喍绗傞崡?
	// 閺夆€叉3閿涙矮绗傛稉鈧▎鈥茬瑝閸︺劌鍢查崐鐓庡隘閿涘牓妲诲銏ゅ櫢婢跺秷袝閸欐埊绱?
	if (value > threshold && rise > minRise) {
		if (!above) {
			above = true;//閺嶅洩顔囨潻娑樺弳韫囧啳鐑﹀畡鏉库偓?
			prevValue = value;//娣囨繂鐡ㄨぐ鎾冲閸?
			return true;//濡偓濞村鍩屾稉鈧▎鈩冩箒閺佸牆绺剧捄?
		}
	} else {
		above = false; //娴ｅ簼绨梼鍫濃偓纭风礉閺嶅洩顔囨稉鐑樺皾鐠?
	}
	prevValue = value;	//閺囧瓨鏌婃稉濠佺濞嗏剝鏆熼崐?
	return false;//閺堫亝顥呭ù瀣煂韫囧啳鐑?
}

/* 定时读取 MAX30102 数据 → 检测心搏 → 计算 BPM */
void readMAX30102(void)
{
	static unsigned long lastReadMillis = 0;
	const unsigned long minInterval = 25;//濮?5ms鐠囪绔村▎?
	unsigned long nowMs = millis();
	if (nowMs - lastReadMillis < minInterval) {
		return;
	}
	lastReadMillis = nowMs;
	//娴肩姵鍔呴崳銊︽弓鐏忚京鍗庨敍宀€娲块幒銉┾偓鈧崙?/
	if (!MAX30102_Ready) {
		static bool warned = false;
		if (!warned) {
			warned = true;
		}
		MAX30102_State.valid = false;
		return;
	}
	//鐠囪褰囨稉鈧紒鍕婢舵牠鍣伴弽宄扳偓?/
	long sample = readFIFOsample();
	if (sample == -2 || sample < 0) {
		MAX30102_State.valid = false;
		return;
	}
	//閺冪姵鏅ラ崐鑹扮箖濠?/
	if (sample == 0x3FFFF || sample == 0) {
		MAX30102_State.valid = false;
		return;
	}
    //閹跺﹪鍣伴弽宄扳偓鐓庣摠閸忋儳骞嗚ぐ銏㈢处閸愭彃灏?/
	irBuffer[irIndex++] = (uint32_t)sample;
	if (irIndex >= BUFFER_SIZE) irIndex = 0;

	//閹峰灝鍩岄張鈧弬鎵畱缁俱垹顦婚崐?/
	uint32_t irVal = irBuffer[(irIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE];
	
	if (irVal < 30000) {
		MAX30102_State.valid = false;
		return;
	}
	
	if (detectBeat(irVal)) {
		uint32_t now = millis();
		if (lastBeatTime != 0) {
			uint32_t delta = now - lastBeatTime;// 娑撱倖顐艰箛鍐儲闂傛挳娈ч弮鍫曟？(ms)
			if (delta > 300 && delta < 3000) // 閸氬牊纭惰箛鍐芳閼煎啫娲块敍?00ms ~ 3000ms
			{
				int bpm = (int)(60000.0 / delta);
				//4濞嗏€抽挬濠婃垶鎶ゅ▔顫礉鐠佲晛绺鹃悳鍥ㄦ纯缁嬪啿鐣?/
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


