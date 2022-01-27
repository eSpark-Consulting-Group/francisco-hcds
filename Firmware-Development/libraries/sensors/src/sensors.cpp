#include "sensor.h"

extern "C"
{
	
	/*********************************************************************************
	 *	@purpose: initialize HX711 weight sensor
	 *	
	 *	@new_accel_divisor: accel rate divisor, refer datasheet for details
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t initHX711(uint32_t DOUT, uint32_t CLK)
	{
		
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: get weight sensor value
	 *	
	 *	@value: reference to value variable to store weight sensor value
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t getValue(double* value)
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
	 
	uint8_t tare(uint8_t times)
	{
		return SUCCESS;
	}
}