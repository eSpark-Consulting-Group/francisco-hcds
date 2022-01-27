#include "sensors.h"


uint8_t GAIN = 0;
long OFFSET = 0;
uint32_t DOUT_GLOBAL = NULL;
uint32_t CLK_GLOBAL = NULL;
uint32_t CS_GLOBAL = NULL;
uint32_t RST_GLOBAL = NULL;
float SCALE = 1;
SPIClass* mySPI;
Uid uid;


extern "C"
{
	
	/*********************************************************************************
	 *	@purpose: initialize HX711 weight sensor
	 *	
	 *	@new_accel_divisor: accel rate divisor, refer datasheet for details
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t initHX711(uint32_t DOUT, uint32_t CLK, uint8_t gain)
	{
		pinMode(CLK, OUTPUT);
		pinMode(DOUT, INPUT_PULLUP);
		
		DOUT_GLOBAL = DOUT;
		CLK_GLOBAL = CLK;
		
		setGain(gain);
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: get weight sensor value in grams
	 *	
	 *	@grams: reference to grams variable to store weight sensor value
	 *	@times: times to take average
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t getGrams(float* grams, uint8_t times)
	{
		double value;
		
		getValue(&value);
		
		*grams = value / SCALE;
		
		return SUCCESS;
	}

	
	/*********************************************************************************
	 *	@purpose: tare scale
	 *	
	 *	@times: number of times to tare
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t tare(uint8_t times)
	{
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: tare scale
	 *	
	 *	@times: number of times to tare
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	static void setGain(uint8_t gain)
	{
		GAIN = gain;
	}
	
	/*********************************************************************************
	 *	@purpose: get weight sensor value
	 *	
	 *	@value: reference to value variable to store weight sensor value
	 *	@times: times to take average
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	static uint8_t getValue(double* value, uint8_t times)
	{
		*value = read_average(times) - OFFSET;
		
		return SUCCESS;
	}
	
	static long readMeasurements()
	{

		// Wait for the chip to become ready.
		wait_ready();

		// Define structures for reading data into.
		unsigned long value = 0;
		uint8_t data[3] = { 0 };
		uint8_t filler = 0x00;

		// Protect the read sequence from system interrupts.  If an interrupt occurs during
		// the time the PD_SCK signal is high it will stretch the length of the clock pulse.
		// If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
		// power down mode during the middle of the read sequence.  While the device will
		// wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
		// forces DOUT high until that cycle is completed.
		
		// Disable interrupts.
		noInterrupts();

		// Pulse the clock pin 24 times to read the data.
		data[2] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);
		data[1] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);
		data[0] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);

		// Set the channel and the gain factor for the next reading using the clock pin.
		for (unsigned int i = 0; i < GAIN; i++) {
			digitalWrite(CLK_GLOBAL, HIGH);
			delayMicroseconds(1);
			digitalWrite(CLK_GLOBAL, LOW);
			delayMicroseconds(1);
		}

		// Enable interrupts again.
		interrupts();

		// Replicate the most significant bit to pad out a 32-bit signed integer
		if (data[2] & 0x80) {
			filler = 0xFF;
		} else {
			filler = 0x00;
		}

		// Construct a 32-bit signed integer
		value = ( (unsigned long)((filler) << 24)
				| (unsigned long)((data[2]) << 16)
				| (unsigned long)((data[1]) << 8)
				| (unsigned long)((data[0]) ));

		return static_cast<long>(value);
	}
	
	uint8_t shiftInSlow(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
	{
		uint8_t value = 0;
		uint8_t i;

		for(i = 0; i < 8; ++i) {
			digitalWrite(clockPin, HIGH);
			delayMicroseconds(1);
			if(bitOrder == LSBFIRST)
				value |= digitalRead(dataPin) << i;
			else
				value |= digitalRead(dataPin) << (7 - i);
			digitalWrite(clockPin, LOW);
			delayMicroseconds(1);
		}
		return value;
	}
	
	static long read_average(uint8_t times)
	{
		long sum = 0;
		for (uint8_t i = 0; i < times; i++) {
			sum += readMeasurements();
			delay(0);
		}
		return sum / times;		
	}
	
	static void wait_ready(unsigned long delay_ms) 
	{
		while (!is_ready()) {
			delay(delay_ms);
		}
	}
	
	static uint8_t is_ready()
	{
		return digitalRead(DOUT_GLOBAL) == LOW;
	}
	
	/*********************************************************************************
	 *	@purpose: get cartridges color id and store their corresponding degree angles
	 *	
	 *	@cartridges_t: cartridge data structure reference to store colors, degrees and quantity remaining
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t processCartridges(cartridges_t* cartridge)
	{
		initRFID(SS_1_PIN, RST_1_PIN);
	}
	
	/*********************************************************************************
	 *	@purpose: check if tray is placed
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t isTrayPlaced()
	{
		
	}
	
	/*********************************************************************************
	 *	@purpose: move tray to specific color cartridge
	 *	
	 *	@cartridges_t: cartridge data structure reference to store colors, degrees and quantity remaining
	 *	@color_id: color id of color
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t moveToCartridge(cartridges_t* cartridge, int color_id)
	{
		
	}
	
	static uint8_t initRFID(uint32_t CS, uint32_t RST)
	{
		CS_GLOBAL = CS;
		RST_GLOBAL = RST;
		
		mySPI = new SPIClass();
		
		mySPI->begin();
		
		PCD_Init();
	}
	
	static uint8_t PCD_Init()
	{
		uint8_t hardReset = false;
		
		pinMode(CS_GLOBAL, OUTPUT);
		digitalWrite(CS_GLOBAL, HIGH);
		
		if(RST_GLOBAL != NULL)
		{
			pinMode(RST_GLOBAL, INPUT);
			if(digitalRead(RST_GLOBAL) == LOW)
			{
				pinMode(RST_GLOBAL, OUTPUT);
				digitalWrite(RST_GLOBAL, LOW);
				delayMicroseconds(2);
				digitalWrite(RST_GLOBAL, HIGH);
				
				delay(50);
				hardReset = true;
			}
		}
		
		if(!hardReset)
		{
			PCD_Reset();
		}
		

		PCD_WriteRegister(TxModeReg, 0x00);
		PCD_WriteRegister(RxModeReg, 0x00);

		PCD_WriteRegister(ModWidthReg, 0x26);	
		PCD_WriteRegister(TModeReg, 0x80);			
		PCD_WriteRegister(TPrescalerReg, 0xA9);		
		PCD_WriteRegister(TReloadRegH, 0x03);		
		PCD_WriteRegister(TReloadRegL, 0xE8);
		
		PCD_WriteRegister(TxASKReg, 0x40);		
		PCD_WriteRegister(ModeReg, 0x3D);		
		PCD_AntennaOn();						
		
	}
	
	static uint8_t PCD_Reset() {
		PCD_WriteRegister(CommandReg, PCD_SoftReset);
		uint8_t count = 0;
		do {
			delay(50);
		} while ((PCD_ReadRegister(CommandReg) & (1 << 4)) && (++count) < 3);
	}
	
	static uint8_t PCD_WriteRegister(PCD_Register reg, uint8_t value)
	{
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));
		digitalWrite(CS_GLOBAL, LOW);
		
		mySPI->transfer(reg);
		mySPI->transfer(value);
		
		digitalWrite(CS_GLOBAL, HIGH);
		mySPI->endTransaction();
	}
	
	static uint8_t PCD_ReadRegister(PCD_Register reg)
	{
		uint8_t value;
		
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));
		
		digitalWrite(CS_GLOBAL, LOW);
		
		mySPI->transfer(0x80 | reg);
		value = mySPI->transfer(0);
		
		digitalWrite(CS_GLOBAL, HIGH);
		
		mySPI->endTransaction();
		
		return value;
	}
	
	static void PCD_AntennaOn()
	{
		uint8_t value = PCD_ReadRegister(TxControlReg);
		if((value & 0x03) != 0x03)
		{
			PCD_WriteRegister(TxControlReg, value | 0x03);
		}
	}
	
}
