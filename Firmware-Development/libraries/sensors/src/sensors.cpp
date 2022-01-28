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
		Servo servo;
		
		initServo(&servo, CART_SERVO_PIN);
		pinMode(OPTICAL_PIN, INPUT); // Init optical sensor
		initRFID(SS_1_PIN, RST_1_PIN);
		readRFIDs(&servo);
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
	
	static void PCD_WriteRegister2(PCD_Register reg, byte count, byte *values)
	{
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));	// Set the settings to work with SPI bus
		digitalWrite(CS_GLOBAL, LOW);		// Select slave
		mySPI->transfer(reg);						// MSB == 0 is for writing. LSB is not used in address. Datasheet section 8.1.2.3.
		for (uint8_t index = 0; index < count; index++) {
			mySPI->transfer(values[index]);
		}
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
	
	static void PCD_ReadRegister2(PCD_Register reg, uint8_t count,	uint8_t *values, uint8_t rxAlign)
	{
		if (count == 0) {
			return;
		}
		//Serial.print(F("Reading ")); 	Serial.print(count); Serial.println(F(" bytes from register."));
		uint8_t address = 0x80 | reg;				// MSB == 1 is for reading. LSB is not used in address. Datasheet section 8.1.2.3.
		uint8_t index = 0;							// Index in values array.
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));	// Set the settings to work with SPI bus
		digitalWrite(CS_GLOBAL, LOW);		// Select slave
		count--;								// One read is performed outside of the loop
		mySPI->transfer(address);					// Tell MFRC522 which address we want to read
		if (rxAlign) {		// Only update bit positions rxAlign..7 in values[0]
			// Create bit mask for bit positions rxAlign..7
			uint8_t mask = (0xFF << rxAlign) & 0xFF;
			// Read value and tell that we want to read the same address again.
			uint8_t value = mySPI->transfer(address);
			// Apply mask to both current value of values[0] and the new data in value.
			values[0] = (values[0] & ~mask) | (value & mask);
			index++;
		}
		while (index < count) {
			values[index] = mySPI->transfer(address);	// Read value and tell that we want to read the same address again.
			index++;
		}
		values[index] = mySPI->transfer(0);			// Read the final byte. Send 0 to stop reading.
		digitalWrite(CS_GLOBAL, HIGH);			// Release slave again
		mySPI->endTransaction(); // Stop using the SPI bus
	} // End PCD_ReadRegister()
	
	static void PCD_AntennaOn()
	{
		uint8_t value = PCD_ReadRegister(TxControlReg);
		if((value & 0x03) != 0x03)
		{
			PCD_WriteRegister(TxControlReg, value | 0x03);
		}
	}
	
	static uint8_t initServo(Servo* servo, uint32_t servo_pin)
	{
		servo->attach(servo_pin, 600, 2300);
		servo->write(0);
	}
	
	static uint8_t readRFIDs(Servo* servo)
	{
		int pos;
		
		for (int INDEX = 0; INDEX < 360; INDEX++)
		{

			pos = INDEX / 2;
			servo->write(pos);

			delay(20);
			
			float proximityADC, proximityV;

			// Read in the ADC and convert it to a voltage:
			proximityADC = analogRead(OPTICAL_PIN);
			proximityV = (float)proximityADC * 5.0 / 1023.0;

			//  delay(100);

			/*Serial.print("INDEX = ");

			Serial.println(INDEX);

			Serial.print("Servo position = ");

			Serial.println(pos);

			Serial.print("OPTICAL_VALUE = ");

			Serial.println(proximityV);

			Serial.println("");*/

			if (PICC_IsNewCardPresent() && proximityV < 20)
			{
			  getCartridgeColor();
			  int a = getCartridgeColor();
			  Serial.print("a = ");
			  Serial.println(a);

			  //Serial.println("RFID + OPTICAL DETECTED");

			  for (int i = 0; i < 11; i++)

			  {

				/*if (a ==  colors[i])

				{
				  degreesOfColors[i] = pos;
				}*/
			  }
			}
	  }
	}
	
	int getCartridgeColor()
	{

	  /*Serial.println("In getCartridgeColor Function");

	  String threeBytes = "";

	  String colorName = "";

	  int color_r = 0;

	  if ( ! rfid_1.PICC_IsNewCardPresent()) {

		Serial.println("IsNewcCardPresent");

		return 0;

	  }


	  // Select one of the cards

	  if ( ! rfid_1.PICC_ReadCardSerial()) {

		Serial.println("ReadCardSerial");

		return 0;

	  }

	  rfid_1.PICC_DumpToSerial(&(rfid_1.uid));

	  Serial.println("UID");

	  temp_CID = "";


	  for (byte i = 0; i < rfid_1.uid.size; i++) {

		if (rfid_1.uid.uidByte[i] < 0x10)

		  //Serial.print(F("0"));

		  temp_CID += "0";

		else

		  //Serial.print(F(""));

		  temp_CID += "";

		//Serial.print(rfid_1.uid.uidByte[(rfid_1.uid.size - 1) - i], HEX);

		temp_CID += String(rfid_1.uid.uidByte[i], HEX);

	  }

	  threeBytes = temp_CID.substring(0, 3);

	  // Search Color

	  for (int i = 0; i < 11; i++)

	  {

		if (threeBytes == CIDS[i])

		{

		  color_r = colors[i];

		  colorName = color_names[i];

		}

	  }

	  Serial.println(temp_CID);

	  Serial.println(threeBytes);

	  Serial.println(color_r);

	  Serial.println(colorName);

	  Serial.println("End getCartridge Fucntion");

	  return color_r;*/

	}
	
	static uint8_t PICC_IsNewCardPresent()
	{
		uint8_t bufferATQA[2];
		uint8_t bufferSize = sizeof(bufferATQA);

		// Reset baud rates
		PCD_WriteRegister(TxModeReg, 0x00);
		PCD_WriteRegister(RxModeReg, 0x00);
		// Reset ModWidthReg
		PCD_WriteRegister(ModWidthReg, 0x26);

		uint8_t result = PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, &bufferSize);
		return (result == STATUS_OK || result == STATUS_COLLISION);
	}
	
	static void PCD_ClearRegisterBitMask(PCD_Register reg, uint8_t mask)
	{
		uint8_t tmp;
		tmp = PCD_ReadRegister(reg);
		PCD_WriteRegister(reg, tmp & (~mask));
	}
	
	static uint8_t PICC_REQA_or_WUPA(uint8_t command, uint8_t *bufferATQA,	uint8_t *bufferSize)
	{
		uint8_t validBits;
		uint8_t status;
		
		if (bufferATQA == nullptr || *bufferSize < 2) {	// The ATQA response is 2 bytes long.
			return STATUS_NO_ROOM;
		}
		PCD_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
		validBits = 7;									// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
		status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
		if (status != STATUS_OK) {
			return status;
		}
		if (*bufferSize != 2 || validBits != 0) {		// ATQA must be exactly 16 bits.
			return STATUS_ERROR;
		}
		return STATUS_OK;
	}
	
	uint8_t PCD_TransceiveData(	uint8_t *sendData,	uint8_t sendLen, uint8_t *backData, uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, uint8_t checkCRC)
	{
		byte waitIRq = 0x30;		// RxIRq and IdleIRq
		return PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
	}
	
	static uint8_t PCD_CommunicateWithPICC(uint8_t command, uint8_t waitIRq, uint8_t *sendData,	uint8_t sendLen, uint8_t *backData,	uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, uint8_t checkCRC) 
	{
		// Prepare values for BitFramingReg
		uint8_t txLastBits = validBits ? *validBits : 0;
		uint8_t bitFraming = (rxAlign << 4) + txLastBits;		// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
		
		PCD_WriteRegister(CommandReg, PCD_Idle);			// Stop any active command.
		PCD_WriteRegister(ComIrqReg, 0x7F);					// Clear all seven interrupt request bits
		PCD_WriteRegister(FIFOLevelReg, 0x80);				// FlushBuffer = 1, FIFO initialization
		PCD_WriteRegister2(FIFODataReg, sendLen, sendData);	// Write sendData to the FIFO
		PCD_WriteRegister(BitFramingReg, bitFraming);		// Bit adjustments
		PCD_WriteRegister(CommandReg, command);				// Execute the command
		if (command == PCD_Transceive) {
			PCD_SetRegisterBitMask(BitFramingReg, 0x80);	// StartSend=1, transmission of data starts
		}
		
		const uint32_t deadline = millis() + 36;
		bool completed = false;

		do {
			byte n = PCD_ReadRegister(ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
			if (n & waitIRq) {					// One of the interrupts that signal success has been set.
				completed = true;
				break;
			}
			if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
				return STATUS_TIMEOUT;
			}
			yield();
		}
		while ((uint32_t) (millis()) < deadline);

		// 36ms and nothing happened. Communication with the MFRC522 might be down.
		if (!completed) {
			return STATUS_TIMEOUT;
		}
		
		// Stop now if any errors except collisions were detected.
		uint8_t errorRegValue = PCD_ReadRegister(ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
		if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
			return STATUS_ERROR;
		}
	  
		byte _validBits = 0;
		
		// If the caller wants data back, get it from the MFRC522.
		if (backData && backLen) {
			byte n = PCD_ReadRegister(FIFOLevelReg);	// Number of bytes in the FIFO
			if (n > *backLen) {
				return STATUS_NO_ROOM;
			}
			*backLen = n;											// Number of bytes returned
			PCD_ReadRegister2(FIFODataReg, n, backData, rxAlign);	// Get received data from FIFO
			_validBits = PCD_ReadRegister(ControlReg) & 0x07;		// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
			if (validBits) {
				*validBits = _validBits;
			}
		}
		
		// Tell about collisions
		if (errorRegValue & 0x08) {		// CollErr
			return STATUS_COLLISION;
		}
		
		// Perform CRC_A validation if requested.
		if (backData && backLen && checkCRC) {
			// In this case a MIFARE Classic NAK is not OK.
			if (*backLen == 1 && _validBits == 4) {
				return STATUS_MIFARE_NACK;
			}
			// We need at least the CRC_A value and all 8 bits of the last byte must be received.
			if (*backLen < 2 || _validBits != 0) {
				return STATUS_CRC_WRONG;
			}
			// Verify CRC_A - do our own calculation and store the control in controlBuffer.
			uint8_t controlBuffer[2];
			uint8_t status = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
			if (status != STATUS_OK) {
				return status;
			}
			if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
				return STATUS_CRC_WRONG;
			}
		}
		
		return STATUS_OK;
	}
	
	static void PCD_SetRegisterBitMask(PCD_Register reg, uint8_t mask)
	{ 
		uint8_t tmp;
		tmp = PCD_ReadRegister(reg);
		PCD_WriteRegister(reg, tmp | mask);
	}
	
	uint8_t PCD_CalculateCRC(uint8_t *data, uint8_t length, uint8_t *result)
	{
		PCD_WriteRegister(CommandReg, PCD_Idle);		// Stop any active command.
		PCD_WriteRegister(DivIrqReg, 0x04);				// Clear the CRCIRq interrupt request bit
		PCD_WriteRegister(FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
		PCD_WriteRegister2(FIFODataReg, length, data);	// Write data to the FIFO
		PCD_WriteRegister(CommandReg, PCD_CalcCRC);		// Start the calculation
		
		// Wait for the CRC calculation to complete. Check for the register to
		// indicate that the CRC calculation is complete in a loop. If the
		// calculation is not indicated as complete in ~90ms, then time out
		// the operation.
		const uint32_t deadline = millis() + 89;

		do {
			// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
			byte n = PCD_ReadRegister(DivIrqReg);
			if (n & 0x04) {									// CRCIRq bit set - calculation done
				PCD_WriteRegister(CommandReg, PCD_Idle);	// Stop calculating CRC for new content in the FIFO.
				// Transfer the result from the registers to the result buffer
				result[0] = PCD_ReadRegister(CRCResultRegL);
				result[1] = PCD_ReadRegister(CRCResultRegH);
				return STATUS_OK;
			}
			yield();
		}
		while (static_cast<uint32_t> (millis()) < deadline);

		// 89ms passed and nothing happened. Communication with the MFRC522 might be down.
		return STATUS_TIMEOUT;
	}
	
}
